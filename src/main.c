/* Tempest 2000 (T2K mode) for the Sega Master System.
 * Platform layer: states, FMV transitions, sprite image streaming, lit lane, HUD. */
#include <SMSlib.h>
#include "vdp.h"
#include "fmv.h"
#include "game.h"
#include "sound.h"
#include "assets.h"

SMS_EMBED_SEGA_ROM_HEADER(9999, 0);
SMS_EMBED_SDSC_HEADER_AUTO_DATE(1, 0, "SMS port", "T2K SMS", "Tempest 2000 mode, SMS port with pre-rendered FMV");

/* ---- VRAM map ------------------------------------------------------------ */
#define SPR_OTHER   0        /* sprite tiles (VRAM 256+): laser, enemy shots, explosions */
#define SPR_SLOTS   28       /* 36 enemy slots x 4 tiles */
#define SPR_CLAW    176      /* VRAM 432: FMV-safe block from here */
#define SPR_SHOT    180
#define SPR_SPIKETIP 186
#define SPR_SPIKEDOT 188
#define SPR_DROID   190
#define LIT_POOL0   192

uint8_t disp_on;
uint8_t keys_prev;
static uint16_t shown_img[MAX_EN], shown_base[MAX_EN];
static uint16_t claw_img = 0xFFFF, claw_want;
static uint8_t up_rr;
static uint16_t hud[32];
static uint8_t lit_shown = 0xFF, lit_target = 0xFF, lit_pool, lit_stage, lit_i, lit_up_lane;
static uint8_t sprites_ok;     /* 'other' sprite tiles + slots valid (not clobbered by FMV) */
uint8_t dbg_frames;

void display(uint8_t on) { disp_on = on; if (on) SMS_displayOn(); else SMS_displayOff(); }

/* ---- palettes ------------------------------------------------------------ */
static const uint8_t hue[8][3] = {
    { 0x10, 0x34, 0x39 }, { 0x01, 0x03, 0x17 }, { 0x05, 0x0F, 0x2F }, { 0x14, 0x3C, 0x3E },
    { 0x04, 0x0C, 0x2E }, { 0x11, 0x33, 0x3B }, { 0x06, 0x0B, 0x1F }, { 0x25, 0x3A, 0x3F } };
static const uint8_t sprpal[16] = { 0x00, 0x3F, 0x0F, 0x0B, 0x03, 0x02, 0x33, 0x22, 0x0C, 0x08, 0x3C, 0x34, 0x0F, 0x33, 0x23, 0x2A };
static uint8_t bgpal[16];
void set_web_palette(uint8_t set)
{
    uint8_t i;
    for (i = 0; i < 16; i++) bgpal[i] = 0;
    bgpal[1] = hue[set][0]; bgpal[2] = hue[set][1]; bgpal[3] = hue[set][2];
    bgpal[5] = 0x05; bgpal[6] = 0x0F; bgpal[7] = 0x2F;
    bgpal[14] = 0x2A; bgpal[15] = 0x3F;
    SMS_loadBGPalette(bgpal);
}

/* ---- HUD (row 0, font tiles 1..) ------------------------------------------ */
uint16_t glyph(char c)
{
    if (c >= '0' && c <= '9') return 1 + c - '0';
    if (c >= 'A' && c <= 'Z') return 11 + c - 'A';
    if (c == '-') return 37; if (c == '!') return 38; if (c == '.') return 39; if (c == '*') return 40;
    return 0;
}
void hud_text(uint8_t x, const char *s) { x++; while (*s && x < 32) hud[x++] = glyph(*s++); }
void hud_num(uint8_t x, uint16_t v, uint8_t digits)
{
    x += digits + 1;
    while (digits--) { hud[--x] = 1 + v % 10; v /= 10; }
}
void build_hud(void)
{
    uint8_t i;
    for (i = 0; i < 32; i++) hud[i] = 0;
    if (gstate == GS_TITLE) {
        hud_text(0, "HI"); hud_num(2, (uint16_t)(hiscore / 10), 5); hud[8] = 1;
        hud_text(11, "SMS FMV EDITION");
        return;
    }
    if (gstate == GS_SELECT) {
        hud_text(1, "START AT LEVEL"); hud_num(16, select_level, 2);
        hud_text(20, "1-GO");
        return;
    }
    for (i = 0; i < 6; i++) hud[i + 1] = 1 + sdig[i];
    hud[8] = glyph('L'); hud_num(8, level, 2);
    hud[12] = glyph('*'); hud_num(12, lives > 9 ? 9 : lives, 1);
    hud[15] = glyph('Z'); hud_num(15, zaps > 9 ? 9 : zaps, 1);
    if (msg_t) { const char *s = msg; uint8_t n = 0; while (s[n]) n++; hud_text(31 - n, s); }
    else if (gstate == GS_PAUSE) hud_text(25, "PAUSED");
    else if (gstate == GS_GAMEOVER) hud_text(22, "GAME OVER");
}
void hud_flush(void)
{
    if (!hud_dirty) return;
    hud_dirty = 0; build_hud();
    vr_addr = 0x3800; vr_src = (const uint8_t *)hud; vr_n = 32; up_words();
}

void clear_nametable(void)
{
    uint8_t i;
    for (i = 1; i < 24; i++) { vr_addr = 0x3800 + i * 64; vr_n = 32; up_fill(); }
}

/* ---- static tiles ------------------------------------------------------------ */
void load_font_and_safe(void)
{
    MAPB(FONT_BANK);
    vr_addr = 32; vr_src = (const uint8_t *)FONT_ADDR; vr_n = 0;   /* 40 tiles = 1280 bytes */
    { uint8_t k; for (k = 0; k < 5; k++) { up_raw(); vr_addr += 256; vr_src += 256; } }
    MAPB(SAFE_BANK);
    vr_addr = (256 + SPR_SHOT) * 32; vr_src = (const uint8_t *)SAFE_ADDR; vr_n = SAFE_SIZE / 2; up_raw();
    vr_addr += SAFE_SIZE / 2; vr_src += SAFE_SIZE / 2; up_raw();
}
void load_other_sprites(void)
{
    uint8_t i, saved = cur_bank;
    MAPB(OTHER_BANK);
    vr_addr = (256 + SPR_OTHER) * 32; vr_src = (const uint8_t *)OTHER_ADDR; vr_n = 0;
    for (i = 0; i < OTHER_SIZE / 256; i++) { up_raw(); vr_addr += 256; vr_src += 256; }
    vr_n = OTHER_SIZE & 255; if (vr_n) up_raw();
    for (i = 0; i < MAX_EN; i++) { shown_img[i] = 0xFFFF; shown_base[i] = 0xFFFF; }
    claw_img = 0xFFFF; sprites_ok = 1;
    MAPB(saved);
}

void upload_image(uint16_t id, uint16_t vaddr)
{
    MAPB(IMG_BANK0 + (id >> 7));
    vr_addr = vaddr; vr_src = (const uint8_t *)(0x8000 + ((id & 127) << 7)); vr_n = 128; up_raw();
}

/* ---- lit lane ------------------------------------------------------------- */
static const uint8_t *lit_rec(uint8_t lane)
{
    const uint8_t *ix;
    MAPB(web_bank);
    ix = WT + 0x1700 + lane * 3;
    { uint8_t b = ix[0]; uint16_t a = ix[1] | (ix[2] << 8); MAPB(b); return (const uint8_t *)a; }
}
void lit_restore(uint8_t lane)
{
    const uint8_t *r = lit_rec(lane); uint8_t n = r[0], i;
    r++;
    for (i = 0; i < n; i++, r += 4) { vr_addr = r[0] | (r[1] << 8); vr_src = r + 2; vr_n = 1; up_words(); }
}
void lit_job(void)
{
    uint8_t saved = cur_bank;
    if (lit_stage == 0 && lit_target != lit_shown) { lit_stage = 1; lit_i = 0; lit_up_lane = lit_target; }
    if (lit_stage == 1) {
        const uint8_t *r; uint8_t n, k = 0;
        if (lit_up_lane != lit_target) { lit_up_lane = lit_target; lit_i = 0; if (lit_target == lit_shown) { lit_stage = 0; goto out; } }
        r = lit_rec(lit_up_lane); n = r[0];
        while (lit_i < n && k < 6) {
            vr_addr = (uint16_t)(LIT_POOL0 + (lit_pool ^ 1) * 32 + lit_i) << 5;
            vr_src = r + 1 + n * 4 + lit_i * 24; up_tile3();
            lit_i++; k++;
        }
        if (lit_i >= n) lit_stage = 2;
    } else if (lit_stage == 2) {
        const uint8_t *r; uint8_t n, i; uint16_t e;
        if (lit_up_lane != lit_target) { lit_stage = 1; lit_i = 0; lit_up_lane = lit_target; goto out; }
        if (lit_shown != 0xFF) lit_restore(lit_shown);
        r = lit_rec(lit_up_lane); n = r[0];
        e = LIT_POOL0 + (lit_pool ^ 1) * 32;
        for (i = 0; i < n; i++, e++) {
            const uint8_t *c = r + 1 + i * 4;
            vr_addr = c[0] | (c[1] << 8); vr_src = (const uint8_t *)&e; vr_n = 1; up_words();
        }
        lit_pool ^= 1; lit_shown = lit_up_lane; lit_stage = 0;
    }
out:
    MAPB(saved);
}
void lit_off_now(void)
{
    uint8_t saved = cur_bank;
    if (lit_shown != 0xFF) lit_restore(lit_shown);
    lit_shown = 0xFF; lit_target = 0xFF; lit_stage = 0;
    MAPB(saved);
}

/* ---- sprites ---------------------------------------------------------------- */
static uint8_t spr_used;
void spr(uint8_t x, uint8_t y, uint8_t tile, uint8_t wide)
{
    if (y < 16 || y > 196 || x < 8 || x > 248) return;
    if (wide) { SMS_addSprite(x - 8, y - 8, tile); SMS_addSprite(x, y - 8, tile + 2); }
    else SMS_addSprite(x - 4, y - 8, tile);
}

uint16_t enemy_image(enemy_t *e, uint8_t ang, uint8_t sz, uint16_t *base)
{
    uint16_t b; uint16_t k;
    switch (e->type) {
    case T_FLIPPER: b = IMG_FLIPPER; k = ang; break;
    case T_MUTANT:  b = IMG_MUTANT; k = ang; break;
    case T_TANKER:  b = IMG_TANKER; k = ang; break;
    case T_TANKER_P: b = IMG_TANKERP; k = ang; break;
    case T_TANKER_F: b = IMG_TANKERF; k = ang; break;
    case T_SPIKER:  b = IMG_SPIKER; k = (frame8 >> 3) & 3; break;
    case T_FUSE:    b = IMG_FUSE; k = (frame8 >> 2) & 1; break;
    case T_PULSAR:  b = IMG_PULSAR; k = ang * 2 + e->aux; break;
    case T_DEMON:   b = IMG_DEMON; k = ang; break;
    case T_MIRROR:  b = IMG_MIRROR; k = ang; break;
    default:        b = IMG_POWERUP; k = (frame8 >> 3) & 1; break;
    }
    *base = b;
    return b + k * 5 + sz;
}

void draw_claw(int16_t zrel)
{
    const uint8_t *p; uint8_t x, y, sz, ang;
    ang = WT[4 + player_lane];
    if (player_z < 0) { p = lane_at(player_lane, player_z); x = p[0]; y = p[1]; sz = p[2]; }
    else {
        uint8_t k = (uint8_t)(((player_pos - ((uint16_t)player_lane << 8) + 128) >> 5) & 7);
        const uint8_t *c = WT + 0x20 + (player_lane * 8 + k) * 2;
        x = c[0]; y = c[1]; sz = lane_at(player_lane, 0)[2];
    }
    (void)zrel;
    claw_want = IMG_CLAW + ang * 5 + sz;
    if (claw_img == claw_want || claw_img != 0xFFFF) spr(x, y, SPR_CLAW, 1);
    if (droid && gstate == GS_PLAY) {
        uint8_t ph = frame8 & 15;
        spr(x + (ph < 8 ? ph : 16 - ph) - 4, y - 10, SPR_DROID, 0);
    }
}

void draw_shots(int16_t zrel)
{
    uint8_t i;
    for (i = 0; i < MAX_SHOT; i++) {
        shot_t *s = &shots[i]; const uint8_t *p; uint8_t sz;
        if (!s->alive) continue;
        if (s->z - zrel < -1024) continue;
        p = lane_at(s->lane, s->z - zrel); sz = p[2] >> 1; if (sz > 2) sz = 2;
        spr(p[0], p[1], s->laser ? (sprites_ok ? SPR_OTHER + sz * 2 : SPR_SHOT + sz * 2) : SPR_SHOT + sz * 2, 0);
    }
}

void draw_spikes(int16_t zrel)
{
    uint8_t l; uint16_t *sp = spikes;
    for (l = 0; l < lanes; l++, sp++) {
        uint16_t h = *sp; int16_t zt;
        if (!h) continue;
        zt = DEPTH - (int16_t)h - zrel;
        if (zt >= -768) { const uint8_t *p = lane_at(l, zt); spr(p[0], p[1], SPR_SPIKETIP, 0); }
        if (h > 1024) {
            int16_t zm = DEPTH - (int16_t)(h >> 1) - zrel;
            if (zm >= -768) { const uint8_t *p = lane_at(l, zm); spr(p[0], p[1], SPR_SPIKEDOT, 0); }
        }
    }
}

static static uint8_t rot;
static uint8_t shown_sz[MAX_EN];
void draw_enemy(enemy_t *e, uint8_t i)
{
    const uint8_t *p; uint8_t x, y, sz, ang, t; uint16_t base, id;
    ang = WT[4 + e->lane];
    p = lane_at(e->lane, e->z); x = p[0]; y = p[1]; sz = p[2];
    t = e->type;
    if ((t == T_FLIPPER || t == T_MUTANT) && e->z == 0 && e->lp) {
        uint8_t k = e->lp - 1; const uint8_t *f;
        if (k > 7) k = 7;
        f = WT + 0x160 + (uint16_t)((uint8_t)((e->lane << 1) + (e->dir == 1)) * 8 + k) * 3;
        x = f[0]; y = f[1]; ang = f[2];
    } else if (t == T_FUSE && e->phase == 1) {
        const uint8_t *q = lane_at(wrapl(e->lane, e->dir), e->z);
        x += (int8_t)((((int16_t)q[0] - x) * e->lp * 13) >> 8);
        y += (int8_t)((((int16_t)q[1] - y) * e->lp * 13) >> 8);
    }
    id = enemy_image(e, ang, sz, &base);
    e->want = id;
    if (shown_img[i] == id) spr(x, y, SPR_SLOTS + (i << 2), sz != 0);
    else if (shown_base[i] == base) spr(x, y, SPR_SLOTS + (i << 2), shown_sz[i] != 0);
}

void draw_sprites(void)
{
    uint8_t i;
    if (gstate == GS_PLAY || gstate == GS_PAUSE || (gstate == GS_DIED && gstate_t < 2)) draw_claw(0);
    if (gstate == GS_ZOOM) { draw_claw(zoom_cam); draw_shots(zoom_cam); draw_spikes(zoom_cam); return; }
    draw_shots(0);
    if (sprites_ok) {
        eshot_t *q = eshots; enemy_t *e;
        for (i = MAX_ESHOT; i; i--, q++) if (q->alive) {
            const uint8_t *p = lane_at(q->lane, q->z); uint8_t sz = p[2] >> 1; if (sz > 2) sz = 2;
            spr(p[0], p[1], 6 + sz * 2, 0);
        }
        rot += 7; if (rot >= MAX_EN) rot -= MAX_EN;
        e = &en[rot];
        for (i = rot; i < MAX_EN; i++, e++) if (e->type) draw_enemy(e, i);
        e = en;
        for (i = 0; i < rot; i++, e++) if (e->type) draw_enemy(e, i);
        draw_spikes(0);
        { expl_t *x = expl; for (i = MAX_EXPL; i; i--, x++) if (x->t) spr(x->x, x->y, SPR_OTHER + 12 + ((16 - x->t) >> 2) * 4, 1); }
    }
}

void image_jobs(void)
{
    uint8_t n = 0, k, i, saved = cur_bank;
    if (claw_want != claw_img) { upload_image(claw_want, (256 + SPR_CLAW) * 32); claw_img = claw_want; n++; }
    if (!sprites_ok) { MAPB(saved); return; }
    for (k = 0; k < MAX_EN && n < 2; k++) {
        i = up_rr; if (++up_rr >= MAX_EN) up_rr = 0;
        if (en[i].type != T_NONE && en[i].want != shown_img[i]) {
            uint16_t id = en[i].want;
            upload_image(id, (uint16_t)(256 + SPR_SLOTS + i * 4) << 5);
            shown_img[i] = id;
            { uint16_t b = 0; enemy_image(&en[i], 0, 0, &b); shown_base[i] = b; shown_sz[i] = (uint8_t)((id - b) % 5); }
            n++;
        }
    }
    MAPB(saved);
}

/* ---- palette effects ---------------------------------------------------- */
static uint8_t pal_dirty;
void palette_fx(void)
{
    static const uint8_t cyc[6] = { 0x0F, 0x3C, 0x33, 0x3F, 0x0C, 0x03 };
    SMS_setSpritePaletteColor(12, cyc[(frame8 >> 2) % 6]);
    SMS_setSpritePaletteColor(13, cyc[((frame8 >> 2) + 3) % 6]);
    SMS_setSpritePaletteColor(10, ((frame8 >> 5) & 3) == 0 && (frame8 & 2) ? 0x3F : 0x3C);
    if (zap_active) {
        uint8_t c = (zap_t & 2) ? 0x3F : 0x3C;
        SMS_setBGPaletteColor(1, c); SMS_setBGPaletteColor(2, c); SMS_setBGPaletteColor(3, c); pal_dirty = 1;
    } else if (pal_dirty) { pal_dirty = 0; SMS_loadBGPalette(bgpal); }
}

/* ---- state transitions -------------------------------------------------- */
void web_clip(uint8_t which)   /* 0 canon, 1 warp, 2 zoom */
{
    const uint8_t *p;
    MAPB(web_bank);
    p = WT + 0x1700 + 54 + which * 3;
    fmv_start(p[0], p[1] | (p[2] << 8));
}

void enter_title(void)
{
    display(0);
    SMS_initSprites(); SMS_finalizeSprites(); SMS_copySpritestoSAT();
    gstate = GS_TITLE; gstate_t = 0;
    set_web_palette(0);
    clear_nametable();
    sprites_ok = 0;
    fmv_start(TITLE_BANK, TITLE_ADDR);
    fmv_set_loop(TITLE_LOOP_BANK, TITLE_LOOP_ADDR);
    music_volume(0); music_play(SONG_TITLE);
    hud_dirty = 1; hud_flush();
    display(1);
}

void show_select(void)
{
    display(0);
    level_setup(select_level);
    set_web_palette((select_level - 1) & 7);
    clear_nametable();
    web_clip(0); fmv_run_blocking();
    hud_dirty = 1; hud_flush();
    display(1);
}

void enter_warp(void)
{
    display(0);
    SMS_initSprites(); SMS_finalizeSprites(); SMS_copySpritestoSAT();
    level_setup(level);
    set_web_palette((level - 1) & 7);
    clear_nametable();
    lit_shown = lit_target = 0xFF; lit_stage = 0;
    sprites_ok = 0;
    gstate = GS_WARP; gstate_t = 0;
    web_clip(1);
    hud_dirty = 1; hud_flush();
    display(1);
    music_volume(0); music_play(SONG_WARP);
}

void enter_play(void)
{
    load_other_sprites();
    gstate = GS_PLAY; gstate_t = 0;
    game_message("TEMPEST 2000", 60);
    music_volume(1); music_play(SONG_LEVELA + (uint8_t)(((level - 1) >> 4) % 3));
}

/* ---- main ------------------------------------------------------------------- */
void main(void)
{
    uint8_t k, btn, press, prev = 0;
    SMS_setSpriteMode(SPRITEMODE_TALL);
    SMS_useFirstHalfTilesforSprites(0);
    SMS_VDPturnOnFeature(VDPFEATURE_LEFTCOLBLANK);
    SMS_loadSpritePalette(sprpal);
    display(0);
    load_font_and_safe();
    vr_addr = 0; vr_n = 16; up_fill();          /* blank tile 0 */
    game_init();
    sound_init();
    enter_title();
    for (;;) {
        k = SMS_getKeysStatus();
        btn = 0;
        if (k & PORT_A_KEY_LEFT) btn |= B_LEFT;
        if (k & PORT_A_KEY_RIGHT) btn |= B_RIGHT;
        if (k & PORT_A_KEY_UP) btn |= B_UP | B_JUMP;
        if (k & PORT_A_KEY_DOWN) btn |= B_DOWN;
        if (k & PORT_A_KEY_1) btn |= B_FIRE;
        if (k & PORT_A_KEY_2) btn |= B_ZAP;
        press = btn & ~prev; prev = btn;
        if (SMS_queryPauseRequested()) { SMS_resetPauseRequest(); press |= B_START; }
        gstate_t++;
        MAPB(web_bank);
        switch (gstate) {
        case GS_TITLE:
            if (press & (B_FIRE | B_ZAP | B_START)) {
                sfx(SFX_MENU);
                gstate = GS_SELECT; gstate_t = 0;
                if (select_level > best_level) select_level = 1;
                show_select();
            }
            break;
        case GS_SELECT:
            if ((press & B_RIGHT) && select_level + 2 <= best_level) { select_level += 2; sfx(SFX_MENU); show_select(); }
            if ((press & B_LEFT) && select_level > 2) { select_level -= 2; sfx(SFX_MENU); show_select(); }
            if (press & (B_FIRE | B_START)) {
                score_reset(); lives = 3; warp_tokens = 0;
                if (select_level > 1) score_add_start((select_level - 1) * 1000);
                level = select_level; enter_warp();
            }
            if (press & B_ZAP) enter_title();
            break;
        case GS_WARP:
            if (fmv_state == FMV_DONE) enter_play();
            break;
        case GS_PLAY:
            play_tick(btn, press);
            lit_target = player_lane;
            if (gstate == GS_PLAY && level_cleared() && gstate_t > 60) {
                gstate = GS_ZOOM; gstate_t = 0;
                game_message(level & 1 ? "EXCELLENT!" : "YES YES YES!", 90);
                sfx(SFX_CLEAR);
                kill_all_for_zoom();
                zap_active = 0; pal_dirty = 0; SMS_loadBGPalette(bgpal);
                lit_off_now();
                sprites_ok = 0;
                zoom_cam = 0;
                web_clip(2);
            }
            if (press & B_START) { gstate = GS_PAUSE; hud_dirty = 1; sound_pause(1); }
            break;
        case GS_PAUSE:
            if (press & (B_START | B_FIRE)) { gstate = GS_PLAY; hud_dirty = 1; sound_pause(0); }
            break;
        case GS_DIED:
            died_tick();
            if (gstate_t > 100) {
                uint8_t i;
                if (lives == 0) { gstate = GS_GAMEOVER; gstate_t = 0; hud_dirty = 1; if (score > hiscore) hiscore = score; music_volume(0); music_play(SONG_GAMEOVER); }
                else {
                    for (i = 0; i < MAX_EN; i++) en[i].type = T_NONE;
                    for (i = 0; i < MAX_SHOT; i++) shots[i].alive = 0;
                    enemies_alive = 0; zap_active = 0; jumping = 0; player_z = 0;
                    if (zaps < 1) zaps = 1;
                    gstate = GS_PLAY; gstate_t = 0; hud_dirty = 1;
                }
            }
            break;
        case GS_ZOOM:
            zoom_cam = (int16_t)fmv_aux << 6;
            zoom_tick(btn, press);
            if (fmv_state == FMV_DONE) {
                if (lives == 0) { gstate = GS_GAMEOVER; gstate_t = 0; hud_dirty = 1; if (score > hiscore) hiscore = score; music_volume(0); music_play(SONG_GAMEOVER); }
                else { score_add_start(1000 * (level < 10 ? level : 10)); level = level >= 99 ? 1 : level + 1; enter_warp(); }
            }
            break;
        case GS_GAMEOVER:
            if (gstate_t > 200 || (gstate_t > 60 && (press & (B_FIRE | B_START)))) enter_title();
            break;
        }
        MAPB(web_bank);
        SMS_initSprites();
        draw_sprites();
        SMS_finalizeSprites();
        if (fmv_state == FMV_UPLOAD) fmv_upload(gstate == GS_ZOOM ? 0xB4 : 0xB8);
        SMS_waitForVBlank();
        SMS_copySpritestoSAT();
        fmv_vblank();
        if (gstate == GS_PLAY || gstate == GS_DIED || gstate == GS_PAUSE || gstate == GS_GAMEOVER) {
            palette_fx();
            lit_job();
        }
        hud_flush();
        image_jobs();
        dbg_frames++;
    }
}
