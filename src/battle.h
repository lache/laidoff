#pragma once

struct _LWCONTEXT;

void update_battle(struct _LWCONTEXT* pLwc);
void exec_attack_p2e_with_screen_point(struct _LWCONTEXT* pLwc, float x, float y);
int exec_attack_p2e(struct _LWCONTEXT *pLwc, int enemy_slot);
