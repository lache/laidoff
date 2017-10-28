#pragma once
#include "linmath.h"

typedef enum _LW_SPHERE_BATTLE_PACKET_TYPE {
	LSBPT_GETTOKEN,
	LSBPT_TOKEN,
	LSBPT_WAIT,
	LSBPT_MATCHED,
} LW_PUCK_GAME_PACKET_TYPE;

// Client -> Server
typedef struct _LWSPHEREBATTLEPACKETGETTOKEN {
	int type;
}

// Server --> Client
typedef struct _LWSPHEREBATTLEPACKETTOKEN {
	int type;
	int token;
}

// Client --> Server
typedef struct _LWSPHEREBATTLEPACKETWAIT {
	int type;
	int token;
}

// Server --> Client
typedef struct _LWSPHEREBATTLEPACKETMATCHED {
	int type;
}
