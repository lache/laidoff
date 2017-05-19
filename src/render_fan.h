#pragma once
#include <linmath.h>

typedef struct _LWCONTEXT LWCONTEXT;

void render_fan(const struct _LWCONTEXT *pLwc, const mat4x4 proj, const mat4x4 view, float x, float y, float z, float a, float sector_theta);
