#ifndef GAME_H
#define GAME_H
#include <stdint.h>

/* buttons */
#define B_LEFT  0x01
#define B_RIGHT 0x02
#define B_UP    0x04
#define B_DOWN  0x08
#define B_FIRE  0x10   /* button 1 */
#define B_ZAP   0x20   /* button 2 */
#define B_JUMP  0x40   /* up / or 1+2 */
#define B_START 0x80   /* PAUSE / button 2 on title */

enum { T_NONE = 0, T_FLIPPER, T_MUTANT, T_TANKER, T_TANKER_P, T_TANKER_F, T_SPIKER, T_FUSE,
       T_PULSAR, T_DEMON, T_MIRROR, T_POWERUP };
enum { PU_LASER = 0, PU_JUMP, PU_DROID, PU_ZAPPO, PU_OUTTA, PU_WARP, PU_COUNT };

#define MAX_EN    36          /* one 4-tile sprite slot each */
#define MAX_SHOT  12
#define MAX_ESHOT 8
#define MAX_EXPL  6
#define DEPTH     4096        /* 16.0 in 8.8 */

typedef struct {
    uint8_t type, lane, phase, timer;
    int16_t z, dz;
    uint8_t lp, aux, dir, hp;
    uint16_t want;            /* image id wanted this frame */
} enemy_t;

typedef struct { uint8_t alive, lane, laser; int16_t z; } shot_t;
typedef struct { uint8_t alive, lane; int16_t z, dz; } eshot_t;
typedef struct { uint8_t t, x, y; } expl_t;

enum { GS_TITLE = 0, GS_SELECT, GS_WARP, GS_PLAY, GS_DIED, GS_ZOOM, GS_GAMEOVER, GS_PAUSE };

extern uint8_t gstate; extern uint16_t gstate_t;
extern uint8_t level, uweb, lanes, closed, lives, zaps, player_lane;
extern uint16_t player_pos; extern int16_t player_z;
extern uint32_t score, hiscore;
extern uint8_t best_level, select_level;
extern enemy_t en[MAX_EN];
extern shot_t shots[MAX_SHOT];
extern eshot_t eshots[MAX_ESHOT];
extern expl_t expl[MAX_EXPL];
extern uint16_t spikes[18];
extern uint8_t laser, jump_on, droid, jumping, zap_active, zap_t, pu_count, warp_tokens;
extern uint8_t enemies_alive, frame8, shake;
extern const char *msg; extern uint8_t msg_t;
extern uint8_t web_bank; extern uint16_t web_tbl;
extern uint8_t hud_dirty;
extern int16_t zoom_cam;

void game_init(void);
void score_reset(void);
void score_add_start(uint16_t s);
void game_message(const char *m, uint8_t t);
extern uint8_t sdig[6];
extern uint8_t pool[8];
void level_setup(uint8_t lvl);        /* tables, pools, objects */
void play_tick(uint8_t btn, uint8_t press);
void died_tick(void);
void zoom_tick(uint8_t btn, uint8_t press);
uint8_t level_cleared(void);
void draw_sprites(void);
void kill_all_for_zoom(void);

/* web table access (web bank must be mapped) */
#define WT ((const uint8_t *)web_tbl)
const uint8_t *lane_at(uint8_t lane, int16_t z);   /* -> x, y, size */
uint8_t wrapl(uint8_t lane, uint8_t dir);
#endif
