#pragma once

void render_solid_box_ui(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index);
void render_solid_vb_ui(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index, enum _LW_VBO_TYPE lvt, float alpha_multiplier, float or , float og, float ob, float oratio);
void get_player_creature_ui_box(int pos, float screen_aspect_ratio, float* left_top_x, float* left_top_y, float* area_width, float* area_height);
void render_damage_text(const LWCONTEXT* pLwc);
