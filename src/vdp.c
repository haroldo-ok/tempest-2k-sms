/* Paced VRAM writers.  Every OUT to the data port is >= 28 T-states after the
 * previous one, so they are safe with the display on (see the skill's VDP
 * pacing rule).  The address is set with interrupts off so the frame IRQ's
 * status read cannot split the two control-port bytes. */
#include <SMSlib.h>
#include "vdp.h"
uint16_t vr_addr;
const uint8_t *vr_src;
uint8_t vr_n;
uint8_t cur_bank;

static void set_addr(void) __naked
{
    __asm
    ld hl,(_vr_addr)
    di
    ld a,l
    out (0xBF),a
    ld a,h
    or a,#0x40
    out (0xBF),a
    ei
    ld hl,(_vr_src)
    ld c,#0xBE
    ret
    __endasm;
}

void up_tile2(void) __naked
{
    __asm
    call _set_addr
    ld b,#8
1$:
    ld a,(hl)
    out (c),a
    inc hl
    ld a,(hl)
    inc hl
    out (c),a
    xor a
    nop
    nop
    nop
    out (c),a
    nop
    nop
    nop
    nop
    out (c),a
    djnz 1$
    ret
    __endasm;
}

void up_tile3(void) __naked
{
    __asm
    call _set_addr
    ld b,#8
1$:
    ld a,(hl)
    out (c),a
    inc hl
    ld a,(hl)
    inc hl
    out (c),a
    ld a,(hl)
    inc hl
    nop
    out (c),a
    xor a
    nop
    nop
    nop
    out (c),a
    djnz 1$
    ret
    __endasm;
}

void up_raw(void) __naked
{
    __asm
    call _set_addr
    ld a,(_vr_n)
    ld b,a
1$:
    ld a,(hl)
    out (c),a
    inc hl
    nop
    djnz 1$
    ret
    __endasm;
}

void up_words(void) __naked
{
    __asm
    call _set_addr
    ld a,(_vr_n)
    ld b,a
1$:
    ld a,(hl)
    out (c),a
    inc hl
    ld a,(hl)
    inc hl
    out (c),a
    djnz 1$
    ret
    __endasm;
}

void up_fill(void) __naked
{
    __asm
    call _set_addr
    ld a,(_vr_n)
    ld b,a
    xor a
1$:
    out (c),a
    nop
    nop
    nop
    nop
    out (c),a
    nop
    nop
    nop
    djnz 1$
    ret
    __endasm;
}
