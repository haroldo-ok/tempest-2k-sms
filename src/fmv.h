#ifndef FMV_H
#define FMV_H
#include <stdint.h>
enum { FMV_IDLE = 0, FMV_UPLOAD, FMV_NTWAIT, FMV_DONE };
extern uint8_t fmv_state, fmv_aux, fmv_frames, fmv_loops;
void fmv_start(uint8_t bank, uint16_t addr);
void fmv_set_loop(uint8_t bank, uint16_t addr);
void fmv_vblank(void);
void fmv_upload(uint8_t stop_line);
void fmv_run_blocking(void);           /* apply whole clip now (display off) */
#endif
