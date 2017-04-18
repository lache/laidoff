#pragma once
#include <linmath.h>

typedef struct _LWKEYFRAME LWKEYFRAME;
typedef struct _LWCONTEXT LWCONTEXT;

typedef void(*custom_render_proc)(const LWCONTEXT*, float, float, float);
typedef void(*anim_finalized_proc)(LWCONTEXT*);

typedef struct _LWANIM {
	int valid;
	int las;
	float elapsed;
	float fps;
	const LWKEYFRAME* animdata;
	int animdata_length;
	//int current_animdata_index;
	mat4x4 mvp;
	float x;
	float y;
	float alpha;
	custom_render_proc custom_render_proc_callback;
	anim_finalized_proc anim_finalized_proc_callback;
	int finalized;
} LWANIM;
