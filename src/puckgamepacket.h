#pragma once
typedef enum _LW_PUCK_GAME_PACKET_TYPE {
	LPGPT_MOVE,
	LPGPT_STOP,
	LPGPT_DASH,
	LPGPT_STATE,
} LW_PUCK_GAME_PACKET_TYPE;

typedef struct _LWPUCKGAMEPACKETMOVE {
	int type;
	float dx;
	float dy;
} LWPUCKGAMEPACKETMOVE;

typedef struct _LWPUCKGAMEPACKETSTOP {
	int type;
} LWPUCKGAMEPACKETSTOP;

typedef struct _LWPUCKGAMEPACKETDASH {
	int type;
} LWPUCKGAMEPACKETDASH;

typedef struct _LWPUCKGAMEPACKETSTATE {
	int type;
	float player[3];
	float player_rot[4];
	float puck[3];
	float puck_rot[4];
	float target[3];
	float target_rot[4];
}LWPUCKGAMEPACKETSTATE;
