#pragma once

void update_battle(struct _LWCONTEXT* pLwc);
void exec_attack_with_screen_point(LWCONTEXT* pLwc, float x, float y);
int exec_attack(LWCONTEXT *pLwc, int enemy_slot);
