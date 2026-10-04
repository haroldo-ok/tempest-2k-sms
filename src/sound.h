#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>
#include "sfx_data.h"
#include "music_data.h"
void sound_init(void);          /* installs the VBlank sound driver */
void sfx(uint8_t id);           /* queue a sound effect (safe from game code) */
void music_play(uint8_t song);
void music_stop(void);
void music_volume(uint8_t att); /* extra attenuation for the music (0 = full) */
void sound_pause(uint8_t on);   /* silence everything while paused */
extern volatile uint8_t mus_song, mus_bar, mus_row;
#endif
