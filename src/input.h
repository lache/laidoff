#pragma once
#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;

void lw_press_key_left(LWCONTEXT* pLwc);
void lw_press_key_right(LWCONTEXT* pLwc);
void lw_press_key_up(LWCONTEXT* pLwc);
void lw_press_key_down(LWCONTEXT* pLwc);
void lw_press_key_space(LWCONTEXT* pLwc);
void lw_press_key_a(LWCONTEXT* pLwc);
void lw_press_key_z(LWCONTEXT* pLwc);
void lw_press_key_x(LWCONTEXT* pLwc);
void lw_press_key_q(LWCONTEXT* pLwc);
void lw_press_key_w(LWCONTEXT* pLwc);
void lw_release_key_left(LWCONTEXT* pLwc);
void lw_release_key_right(LWCONTEXT* pLwc);
void lw_release_key_up(LWCONTEXT* pLwc);
void lw_release_key_down(LWCONTEXT* pLwc);
void lw_release_key_space(LWCONTEXT* pLwc);
void lw_release_key_a(LWCONTEXT* pLwc);
void lw_release_key_z(LWCONTEXT* pLwc);
void lw_release_key_x(LWCONTEXT* pLwc);
void lw_release_key_q(LWCONTEXT* pLwc);
void lw_go_back(LWCONTEXT* pLwc, void* native_context);
#ifdef __cplusplus
};
#endif