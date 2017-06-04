#pragma once

struct _LWCONTEXT;

void update_battle(struct _LWCONTEXT* pLwc);
void exec_attack_p2e_with_screen_point(struct _LWCONTEXT* pLwc, float x, float y);
int exec_attack_p2e(struct _LWCONTEXT *pLwc, int enemy_slot);
int spawn_damage_text(LWCONTEXT *pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord);
int spawn_exp_text(LWCONTEXT *pLwc, float x, float y, float z, const char *text, LW_DAMAGE_TEXT_COORD coord);
void update_attack_trail(LWCONTEXT *pLwc);
void update_damage_text(LWCONTEXT *pLwc);
