#ifndef VDP_H
#define VDP_H
#include <stdint.h>
/* parameters for the paced asm uploaders (globals: no calling-convention games) */
extern uint16_t vr_addr;          /* VRAM byte address */
extern const uint8_t *vr_src;
extern uint8_t vr_n;
extern uint8_t cur_bank;          /* what we believe is mapped in slot 2 */
void up_tile2(void);   /* 16-byte 2bpp tile -> 32-byte 4bpp (planes 2,3 = 0), paced */
void up_tile3(void);   /* 24-byte 3bpp tile -> 4bpp (plane 3 = 0), paced          */
void up_raw(void);     /* vr_n bytes (1..255; 0 = 256), paced                      */
void up_words(void);   /* vr_n 16-bit nametable entries, paced                     */
void up_fill(void);    /* write vr_n zero WORDS at vr_addr, paced                  */
#define MAPB(b) do { cur_bank = (b); ROM_bank_to_be_mapped_on_slot2 = (b); } while (0)
#endif
