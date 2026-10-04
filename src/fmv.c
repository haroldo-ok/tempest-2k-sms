/* Pre-rendered video player.  The stream is a list of steps (see tools/build.py):
 *   flags, hold, aux, u16 nup, nup x {slot lo, slot hi|tid hi<<1, tid lo16}, u16 nruns,
 *   nruns x {u16 cell, u8 len, len x u16 nametable entry};  0xFF = continue in next bank.
 * Tile pixels come from the shared 2bpp dictionary (1024 tiles per bank).
 * The encoder modelled VRAM exactly, so uploads only ever hit slots that are not on
 * screen and the nametable writes for a visible step are done right after VBlank. */
#include <SMSlib.h>
#include "vdp.h"
#include "fmv.h"
#include "assets.h"

uint8_t fmv_state, fmv_aux, fmv_frames, fmv_loops;

uint8_t fmv_bank, vstop;
const uint8_t dict_bank0 = DICT_BANK0;
static uint8_t  fmv_flags, fmv_hold, fmv_wait, loop_bank;
uint16_t fmv_nup;
const uint8_t *fmv_p;
static const uint8_t  *loop_p;

static void header(void)
{
    MAPB(fmv_bank);
    if (*fmv_p == 0xFF) { fmv_bank++; MAPB(fmv_bank); fmv_p = (const uint8_t *)0x8000; }
    fmv_flags = fmv_p[0]; fmv_hold = fmv_p[1]; fmv_aux = fmv_p[2];
    fmv_nup = fmv_p[3] | ((uint16_t)fmv_p[4] << 8);
    fmv_p += 5;
    fmv_state = FMV_UPLOAD;
}

void fmv_start(uint8_t bank, uint16_t addr)
{
    fmv_bank = bank; fmv_p = (const uint8_t *)addr; fmv_frames = 0; fmv_wait = 0; loop_bank = 0; fmv_loops = 0;
    header();
}
void fmv_set_loop(uint8_t bank, uint16_t addr) { loop_bank = bank; loop_p = (const uint8_t *)addr; }


/* Upload tiles from the step's upload list until done or the beam reaches vstop.
 * Entry: {slot lo, slot hi, tid lo, tid hi}; dictionary tile = bank dict_bank0 + tid>>10,
 * address 0x8000 + (tid & 1023) * 16, written as 2bpp -> 4bpp with paced OUTs. */
static void upload_loop(void) __naked
{
    __asm
    ld hl,(_fmv_p)
00100$:
    ld de,(_fmv_nup)
    ld a,d
    or a,e
    jp z,00190$
    in a,(0x7E)
    cp a,#0xC0
    jr nc,00110$
    ld b,a
    ld a,(_vstop)
    cp a,b
    jp z,00190$
    jp c,00190$
00110$:
    dec de
    ld (_fmv_nup),de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    push hl
    ld a,b
    srl a
    srl a
    ld hl,#_dict_bank0
    add a,(hl)
    ld (#0xFFFF),a
    ld a,b
    and a,#3
    ld b,a
    sla c
    rl b
    sla c
    rl b
    sla c
    rl b
    sla c
    rl b
    set 7,b
    ld a,d
    and a,#1
    ld h,a
    ld l,e
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    di
    ld a,l
    out (0xBF),a
    ld a,h
    or a,#0x40
    out (0xBF),a
    ei
    ld h,b
    ld l,c
    ld c,#0xBE
    ld b,#8
00120$:
    ld a,(hl)
    out (c),a
    inc hl
    ld a,(hl)
    inc hl
    out (c),a
    xor a,a
    nop
    nop
    nop
    out (c),a
    nop
    nop
    nop
    nop
    out (c),a
    djnz 00120$
    ld a,(_fmv_bank)
    ld (#0xFFFF),a
    pop hl
    jp 00100$
00190$:
    ld (_fmv_p),hl
    ret
    __endasm;
}

static void write_nt(void)
{
    uint16_t nr = fmv_p[0] | ((uint16_t)fmv_p[1] << 8);
    fmv_p += 2;
    while (nr--) {
        uint16_t cell = fmv_p[0] | ((uint16_t)fmv_p[1] << 8);
        uint8_t len = fmv_p[2];
        fmv_p += 3;
        vr_addr = 0x3800 + (cell << 1); vr_src = fmv_p; vr_n = len;
        up_words();
        fmv_p += (uint16_t)len << 1;
    }
}

static void after_step(void)
{
    if (fmv_flags & 1) { fmv_frames++; fmv_wait = fmv_hold; }
    if (fmv_flags & 2) {
        if (loop_bank) { fmv_bank = loop_bank; fmv_p = loop_p; fmv_loops++; header(); }
        else fmv_state = FMV_DONE;
    } else header();
}


/* call right after VBlank: shows a ready frame (nametable writes) */
void fmv_vblank(void)
{
    uint8_t saved = cur_bank;
    if (fmv_state == FMV_IDLE || fmv_state == FMV_DONE) return;
    if (fmv_wait) fmv_wait--;
    if (fmv_state == FMV_NTWAIT && !fmv_wait) {
        MAPB(fmv_bank);
        write_nt(); after_step();
        MAPB(saved);
    }
}

/* call at the end of the frame's work: upload tiles until the beam reaches stop_line */
void fmv_upload(uint8_t stop_line)
{
    uint8_t saved = cur_bank;
    MAPB(fmv_bank);
    while (fmv_state == FMV_UPLOAD) {
        vstop = stop_line; upload_loop();
        if (fmv_nup) break;
        if (fmv_flags & 1) { fmv_state = FMV_NTWAIT; break; }
        write_nt(); after_step();          /* invisible (converge) step */
    }
    MAPB(saved);
}

void fmv_run_blocking(void)
{
    uint8_t saved = cur_bank;
    while (fmv_state == FMV_UPLOAD) {
        MAPB(fmv_bank);
        vstop = 0xFF; upload_loop();
        write_nt();
        fmv_flags &= ~1;
        after_step();
    }
    MAPB(saved);
}
