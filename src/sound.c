/* Sound driver: original music sequencer + 2-slot effect player, run from the VBlank
 * interrupt so tempo never depends on the game's frame rate.  Game code never touches
 * the PSG; it only posts requests (all PSG writes happen here, so no latch races).
 *   music: ch0 bass, ch1 lead, ch2 chord arpeggio, noise drums
 *   sfx  : tone slot on ch2 (steals the arpeggio), noise slot (steals the drums) */
#include <SMSlib.h>
#include "sound.h"
__sfr __at 0x7F PSG;

/* ---------------------------------------------------------- requests */
#define NREQ 6
static volatile uint8_t req[NREQ], nreq;
static volatile uint8_t mus_req = 0xFF;    /* 0xFF none, 0xFE stop, else song */
static volatile uint8_t paused, mus_att_req;
void sfx(uint8_t id) { __critical { if (nreq < NREQ) req[nreq++] = id; } }
void music_play(uint8_t s) { mus_req = s; }
void music_stop(void) { mus_req = 0xFE; }
void music_volume(uint8_t a) { mus_att_req = a; }
void sound_pause(uint8_t on) { paused = on; }

/* ---------------------------------------------------------- instruments */
static const uint8_t env_pluck[] = { 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };
static const uint8_t env_lead[]  = { 1, 0, 0, 1, 1, 2, 2, 2, 3 };
static const uint8_t env_pad[]   = { 4, 5, 5, 6, 6, 7, 7, 8 };
static const uint8_t *const envs[3] = { env_pluck, env_lead, env_pad };
static const uint8_t envlen[3] = { sizeof env_pluck, sizeof env_lead, sizeof env_pad };
static const uint8_t arp_tab[3][4] = { { 0, 0, 0, 0 }, { 0, 3, 7, 12 }, { 0, 4, 7, 12 } };
/* drums: (noise mode << 4) | attenuation, 0xFF end */
static const uint8_t dr_k[] = { 0x20, 0x22, 0x25, 0x28, 0x2B, 0x2F, 0xFF };
static const uint8_t dr_s[] = { 0x51, 0x52, 0x54, 0x56, 0x58, 0x5A, 0x5C, 0x5F, 0xFF };
static const uint8_t dr_h[] = { 0x46, 0x4A, 0x4F, 0xFF };
static const uint8_t dr_o[] = { 0x45, 0x46, 0x48, 0x4A, 0x4C, 0x4E, 0x4F, 0xFF };
static const uint8_t *const drums[4] = { dr_k, dr_s, dr_h, dr_o };

/* ---------------------------------------------------------- state */
volatile uint8_t mus_song = 0xFF, mus_bar, mus_row;
static uint8_t tick, mus_att;
static const song_t *song;
static const uint8_t *drum_p; static uint8_t drum_mode = 0xFF;
static const uint8_t *st, *sn;           /* active sfx programs */
static uint8_t st_pri, sn_pri, noise_mode_out = 0xFF;
static uint16_t gp; static uint8_t ga;
/* per-channel state as flat globals (absolute addressing) - one macro body per channel */
#define CH_DECL(k) static uint8_t on##k, nt##k, ar##k, ev##k, vb##k, la##k = 0xFF; static uint16_t lp##k = 0xFFFF;
CH_DECL(0) CH_DECL(1) CH_DECL(2)
#define CH_RUN(k, ENV, ENVLEN, LEAD) do {                                         \
    if (!on##k) { if (la##k != 15) { la##k = 15; PSG = 0x9F | (k << 5); } break; } \
    { uint8_t n = nt##k;                                                         \
      if (ar##k) n += arp_tab[ar##k][(vb##k >> 1) & 3];                          \
      gp = note_period[n > 61 ? 61 : n]; }                                        \
    if (LEAD && vb##k > 12) { uint8_t d = (uint8_t)(gp >> 7) + 1;                \
        if ((uint8_t)((vb##k & 7) - 2) < 4) gp -= d; else gp += d; }              \
    ga = ENV[ev##k < ENVLEN ? ev##k : ENVLEN - 1] + mus_att; if (ga > 15) ga = 15; \
    if (ev##k < 32) ev##k++;                                                     \
    vb##k++;                                                                     \
    if (gp != lp##k) { lp##k = gp; PSG = 0x80 | (k << 5) | (gp & 15); PSG = (gp >> 4) & 0x3F; } \
    if (ga != la##k) { la##k = ga; PSG = 0x90 | (k << 5) | ga; }                \
} while (0)

static void stop_song(void) { mus_song = 0xFF; on0 = on1 = on2 = 0; drum_p = 0; }

void noise_out(uint8_t v)
{
    uint8_t mode = v >> 4, att = (v & 15) + mus_att;
    if (mode != noise_mode_out) { noise_mode_out = mode; PSG = 0xE0 | mode; }
    PSG = 0xF0 | (att > 15 ? 15 : att);
}
static void silence(void)
{
    PSG = 0x9F; PSG = 0xBF; PSG = 0xDF; PSG = 0xFF;
    la0 = la1 = la2 = 15;
}
static void start_song(uint8_t s)
{
    mus_song = s; song = &mus_songs[s];
    mus_bar = 0; mus_row = 0; tick = 1;
    stop_song(); mus_song = s;
}
static void stop_song(void);

void do_row(void)
{
    const uint8_t *ord = mus_order + ((song->order0 + mus_bar) << 2);
    uint8_t c, b;
    (void)c;
    b = mus_pat[ord[0]][mus_row]; if (b == 1) on0 = 0; else if (b >= 2) { nt0 = (b & 63) - 2; ar0 = b >> 6; ev0 = 0; vb0 = 0; on0 = 1; }
    b = mus_pat[ord[1]][mus_row]; if (b == 1) on1 = 0; else if (b >= 2) { nt1 = (b & 63) - 2; ar1 = b >> 6; ev1 = 0; vb1 = 0; on1 = 1; }
    b = mus_pat[ord[2]][mus_row]; if (b == 1) on2 = 0; else if (b >= 2) { nt2 = (b & 63) - 2; ar2 = b >> 6; ev2 = 0; vb2 = 0; on2 = 1; }
    b = mus_pat[ord[3]][mus_row];
    if (b >= 2) drum_p = drums[b - 2];
    if (++mus_row == 16) {
        mus_row = 0;
        if (++mus_bar == song->nbars) { if (song->loop == 0xFF) stop_song(); else mus_bar = song->loop; }
    }
}

void sound_isr(void)
{
    uint8_t i;
    if (mus_req != 0xFF) { uint8_t r = mus_req; mus_req = 0xFF; if (r == 0xFE) stop_song(); else start_song(r); }
    mus_att = mus_att_req;
    for (i = 0; i < nreq; i++) {
        uint8_t id = req[i];
        if (sfx_tone[id] && (!st || sfx_tpri[id] >= st_pri)) { st = sfx_tone[id]; st_pri = sfx_tpri[id]; }
        if (sfx_noise[id] && (!sn || sfx_npri[id] >= sn_pri)) { sn = sfx_noise[id]; sn_pri = sfx_npri[id]; }
    }
    nreq = 0;
    if (paused) { silence(); noise_mode_out = 0xFF; return; }
    if (mus_song != 0xFF && --tick == 0) { tick = song->tempo; do_row(); }
    CH_RUN(0, env_pluck, sizeof env_pluck, 0);
    CH_RUN(1, env_lead, sizeof env_lead, 1);
    if (st) {
        if (st[1] == 0xFF) { st = 0; st_pri = 0; la2 = 15; PSG = 0xDF; }
        else {
            gp = st[0] | ((uint16_t)st[1] << 8); ga = st[2];
            if (gp != lp2) { lp2 = gp; PSG = 0xC0 | (gp & 15); PSG = (gp >> 4) & 0x3F; }
            if (ga != la2) { la2 = ga; PSG = 0xD0 | ga; }
            st += 3;
        }
    } else CH_RUN(2, env_pad, sizeof env_pad, 0);
    if (sn) {
        if (*sn == 0xFF) { sn = 0; sn_pri = 0; PSG = 0xFF; }
        else { uint8_t v = *sn; uint8_t m = mus_att; mus_att = 0; noise_out(v); mus_att = m; sn++; }
        if (drum_p && *drum_p != 0xFF) drum_p++;
    } else if (drum_p) {
        if (*drum_p == 0xFF) { drum_p = 0; PSG = 0xFF; }
        else { noise_out(*drum_p); drum_p++; }
    }
}

void sound_init(void)
{
    silence();
    SMS_setFrameInterruptHandler(sound_isr);
}
