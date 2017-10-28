#pragma once
#include "linmath.h"

typedef enum _LW_SPHERE_BATTLE_PACKET_TYPE {
	LSBPT_GETTOKEN,
	LSBPT_TOKEN,
	LSBPT_QUEUE,
	LSBPT_MATCHED,
} LW_SPHERE_BATTLE_PACKET_TYPE;

// Client -> Server
typedef struct _LWSPHEREBATTLEPACKETGETTOKEN {
	int type;
} LWSPHEREBATTLEPACKETGETTOKEN;

// Server --> Client
typedef struct _LWSPHEREBATTLEPACKETTOKEN {
	int type;
	int token;
} LWSPHEREBATTLEPACKETTOKEN;

// Client --> Server
typedef struct _LWSPHEREBATTLEPACKETQUEUE {
	int type;
	int token;
} LWSPHEREBATTLEPACKETQUEUE;

// Server --> Client
typedef struct _LWSPHEREBATTLEPACKETMATCHED {
	int type;
	int master;
} LWSPHEREBATTLEPACKETMATCHED;