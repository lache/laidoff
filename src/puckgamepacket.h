#pragma once
#include "linmath.h"

typedef enum _LW_PUCK_GAME_PACKET_TYPE {
	LPGPT_MOVE = 100,
	LPGPT_STOP,
	LPGPT_DASH,
	LPGPT_PULL_START,
	LPGPT_PULL_STOP,
	LPGPT_STATE,
} LW_PUCK_GAME_PACKET_TYPE;

typedef struct _LWPUCKGAMEPACKETMOVE {
	int type;
	int token;
	float dx;
	float dy;
} LWPUCKGAMEPACKETMOVE;

typedef struct _LWPUCKGAMEPACKETSTOP {
	int type;
	int token;
} LWPUCKGAMEPACKETSTOP;

typedef struct _LWPUCKGAMEPACKETDASH {
	int type;
	int token;
} LWPUCKGAMEPACKETDASH;

typedef struct _LWPUCKGAMEPACKETPULLSTART {
	int type;
	int token;
} LWPUCKGAMEPACKETPULLSTART;

typedef struct _LWPUCKGAMEPACKETPULLSTOP {
	int type;
	int token;
} LWPUCKGAMEPACKETPULLSTOP;

typedef struct _LWPUCKGAMEPACKETSTATE {
	int type;
	int token;
	int update_tick;
	// player
	float player[3];
	mat4x4 player_rot;
	float player_speed;
	float player_move_rad;
	// puck
	float puck[3];
	mat4x4 puck_rot;
	float puck_speed;
	float puck_move_rad;
	// target
	float target[3];
	mat4x4 target_rot;
	float target_speed;
	float target_move_rad;
}LWPUCKGAMEPACKETSTATE;
