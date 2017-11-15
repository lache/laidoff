#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWDIRPAD LWDIRPAD;

int lw_get_normalized_dir_pad_input(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float *dx, float *dy, float *dlen);
void reset_dir_pad_position(LWCONTEXT* pLwc, LWDIRPAD* dir_pad);
void lw_press_key_left(LWCONTEXT* pLwc);
void lw_press_key_right(LWCONTEXT* pLwc);
void lw_press_key_up(LWCONTEXT* pLwc);
void lw_press_key_down(LWCONTEXT* pLwc);
void lw_press_key_space(LWCONTEXT* pLwc);
void lw_press_key_a(LWCONTEXT* pLwc);
void lw_press_key_z(LWCONTEXT* pLwc);
void lw_press_key_x(LWCONTEXT* pLwc);
void lw_press_key_q(LWCONTEXT* pLwc);
void lw_release_key_left(LWCONTEXT* pLwc);
void lw_release_key_right(LWCONTEXT* pLwc);
void lw_release_key_up(LWCONTEXT* pLwc);
void lw_release_key_down(LWCONTEXT* pLwc);
void lw_release_key_space(LWCONTEXT* pLwc);
void lw_release_key_a(LWCONTEXT* pLwc);
void lw_release_key_z(LWCONTEXT* pLwc);
void lw_release_key_x(LWCONTEXT* pLwc);
void lw_release_key_q(LWCONTEXT* pLwc);
