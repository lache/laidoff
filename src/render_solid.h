#pragma once

void render_solid_box_ui_lvt_flip_y_uv(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index, enum _LW_VBO_TYPE lvt, int flip_y_uv);
void render_solid_box_ui(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index);
void render_solid_box_ui_alpha(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index, float alpha_multiplier);
void render_solid_vb_ui(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or , float og, float ob, float oratio);
void render_solid_vb_ui_alpha(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index, GLuint tex_alpha_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or , float og, float ob, float oratio);
void render_solid_vb_ui_flip_y_uv(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or , float og, float ob, float oratio, int flip_y_uv);
