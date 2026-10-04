#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H
#include <stdint.h>
typedef struct { uint8_t tempo, nbars, loop; uint16_t order0; } song_t;
extern const uint16_t note_period[62];
extern const uint8_t mus_pat[][16];
extern const uint8_t mus_order[];
extern const song_t mus_songs[];
#define SONG_TITLE 0
#define SONG_LEVELA 1
#define SONG_LEVELB 2
#define SONG_LEVELC 3
#define SONG_GAMEOVER 4
#define SONG_WARP 5
#endif
