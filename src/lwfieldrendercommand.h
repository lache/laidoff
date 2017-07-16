#pragma once

typedef enum _LW_RENDER_COMMAND_TYPE {
	LRCT_SPAWN,
	LRCT_DESPAWN,
	LRCT_POS,
	LRCT_TURN,
	LRCT_ANIM,
} LW_RENDER_COMMAND_TYPE;

typedef struct _LWFIELDRENDERCOMMAND {
	LW_RENDER_COMMAND_TYPE cmdtype;
	int key;
	int objtype;
	float x;
	float y;
	float angle;
	int actionid;
} LWFIELDRENDERCOMMAND;
