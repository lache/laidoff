#pragma once

struct _LWCONTEXT;
struct _LWUIDIM;

void lwc_render_battle_result(const struct _LWCONTEXT* pLwc);
void get_player_creature_result_ui_box(int pos, float screen_aspect_ratio, float* left_top_x, float* left_top_y, float* area_width, float* area_height);
void get_battle_result_next_button_dim(struct _LWUIDIM* d);
