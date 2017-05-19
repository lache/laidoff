#pragma once

struct _LWCONTEXT;

void lwc_render_skin(const struct _LWCONTEXT* pLwc);

void render_skin_ui(const LWCONTEXT* pLwc,
	float x, float y, float scale,
	GLuint tex_index,
	enum _LW_SKIN_VBO_TYPE lvt,
	const struct _LWANIMACTION* action,
	const struct _LWARMATURE* armature,
	float alpha_multiplier, float or , float og, float ob, float oratio, double skin_time);

void render_skin(const LWCONTEXT* pLwc,
	GLuint tex_index,
	enum _LW_SKIN_VBO_TYPE lvt,
	const struct _LWANIMACTION* action,
	const struct _LWARMATURE* armature,
	float alpha_multiplier, float or , float og, float ob, float oratio,
	const mat4x4 proj, const mat4x4 view, const mat4x4 model, double skin_time);