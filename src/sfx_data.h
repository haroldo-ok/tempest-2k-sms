#ifndef SFX_DATA_H
#define SFX_DATA_H
#include <stdint.h>
extern const uint8_t *const sfx_tone[]; extern const uint8_t *const sfx_noise[];
extern const uint8_t sfx_tpri[]; extern const uint8_t sfx_npri[];
#define SFX_SHOT 0
#define SFX_LASER 1
#define SFX_EXPLODE 2
#define SFX_FLIP 3
#define SFX_DIE 4
#define SFX_ZAP 5
#define SFX_POWERUP 6
#define SFX_CLEAR 7
#define SFX_SPIKE 8
#define SFX_JUMP 9
#define SFX_MENU 10
#define SFX_WARP 11
#define SFX_ONEUP 12
#define SFX_SPLIT 13
#define SFX_COUNT 14
#endif
