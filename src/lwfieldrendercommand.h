#pragma once

#include "lwtimepoint.h"

typedef enum _LW_RENDER_COMMAND_TYPE {
	LRCT_SPAWN,
	LRCT_DESPAWN,
	LRCT_POS,
	LRCT_TURN,
	LRCT_ANIM,
} LW_RENDER_COMMAND_TYPE;

typedef struct _LWFIELDRENDERCOMMAND {
	// Command type
	LW_RENDER_COMMAND_TYPE cmdtype;
	// Object key (issued in field.lua)
	int key;
	// Object type
	int objtype;
	// X position
	float x;
	// Y position
	float y;
	// Face direction
	float angle;
	// Animation action ID
	int actionid;
	// Animation start time
	double animstarttime;
	// 1 if loop animation mode
	int loop;
} LWFIELDRENDERCOMMAND;
