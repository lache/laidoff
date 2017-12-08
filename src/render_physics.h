#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_physics(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj);
void mult_world_roll(mat4x4 model, int axis, int dir, float angle);
