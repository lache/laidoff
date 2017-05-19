#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;

#define LW_MAX_FAN_VERTEX_COUNT (180)

typedef struct _LWFAN {
	int count;
	float radius[LW_MAX_FAN_VERTEX_COUNT];
	float angle;
} LWFAN;

void* init_fan();
void render_fan(const struct _LWCONTEXT *pLwc, const LWFAN* fan);
