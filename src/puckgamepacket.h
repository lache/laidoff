#pragma once

enum _LW_PUCK_GAME_PACKET {
	LPGP_LWPGETTOKEN = 0,
	LPGP_LWPTOKEN,
	LPGP_LWPQUEUE,
	LPGP_LWPMATCHED,
	LPGP_LWPPLAYERDAMAGED,
	
	LPGP_LWPMOVE = 100,
	LPGP_LWPSTOP,
	LPGP_LWPDASH,
	LPGP_LWPPULLSTART,
	LPGP_LWPPULLSTOP,
	LPGP_LWPSTATE,

	// tcp
	LPGP_LWPQUEUE2 = 200,
	LPGP_LWPMAYBEMATCHED = 201,
	LPGP_LWPMATCHED2 = 202,
	LPGP_LWPQUEUEOK = 203,
	LPGP_LWPRETRYQUEUE = 204,

	// internal admin tcp
	LPGP_LWPCREATEBATTLE = 1000,
	LPGP_LWPCREATEBATTLEOK = 1001,
} LW_PUCK_GAME_PACKET;

// Client -> Server
typedef struct _LWPGETTOKEN {
	int type;
} LWPGETTOKEN;

// Server --> Client
typedef struct _LWPTOKEN {
	int type;
	int token;
} LWPTOKEN;

// Client --> Server
typedef struct _LWPQUEUE {
	int type;
	int token;
} LWPQUEUE;

// Server --> Client
typedef struct _LWPMATCHED {
	int type;
	int master;
} LWPMATCHED;

typedef struct _LWPPLAYERDAMAGED {
	int type;
} LWPPLAYERDAMAGED;




typedef struct _LWPMOVE {
	int type;
	int token;
	float dx;
	float dy;
} LWPMOVE;

typedef struct _LWPSTOP {
	int type;
	int token;
} LWPSTOP;

typedef struct _LWPDASH {
	int type;
	int token;
} LWPDASH;

typedef struct _LWPPULLSTART {
	int type;
	int token;
} LWPPULLSTART;

typedef struct _LWPPULLSTOP {
	int type;
	int token;
} LWPPULLSTOP;

typedef struct _LWPSTATE {
	int type;
	int token;
	int update_tick;
	// player
	float player[3];
	float player_rot[4][4];
	float player_speed;
	float player_move_rad;
	// puck
	float puck[3];
	float puck_rot[4][4];
	float puck_speed;
	float puck_move_rad;
	// target
	float target[3];
	float target_rot[4][4];
	float target_speed;
	float target_move_rad;
} LWPSTATE;



// should be 4-byte aligned...
// (Cgo compatability issue)
//#pragma pack(push, 1)
typedef struct _LWPQUEUE2 {
	unsigned short size;
	unsigned short type;
} LWPQUEUE2;

typedef struct _LWPQUEUEOK {
	unsigned short size;
	unsigned short type;
} LWPQUEUEOK;

typedef struct _LWPRETRYQUEUE {
	unsigned short size;
	unsigned short type;
} LWPRETRYQUEUE;

typedef struct _LWPMAYBEMATCHED {
	unsigned short size;
	unsigned short type;
} LWPMAYBEMATCHED;

typedef struct _LWPMATCHED2 {
	unsigned short size;
	unsigned short type;
	unsigned short port;
	unsigned short __padding_unused;
	unsigned char ipaddr[4];
	int battle_id;
} LWPMATCHED2;


typedef struct _LWPCREATEBATTLE {
	unsigned short size;
	unsigned short type;
} LWPCREATEBATTLE;

typedef struct _LWPCREATEBATTLEOK {
	unsigned short Size;
	unsigned short Type;
	int Battle_id;
} LWPCREATEBATTLEOK;

//#pragma pack(pop)
