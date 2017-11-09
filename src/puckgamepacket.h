#pragma once

enum _LW_PUCK_GAME_PACKET {
	LPGP_LWPGETTOKEN = 0,
	LPGP_LWPTOKEN,
	LPGP_LWPQUEUE,
	LPGP_LWPMATCHED,
	LPGP_LWPPLAYERDAMAGED,
	LPGP_LWPTARGETDAMAGED,
	
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
	LPGP_LWPSUDDENDEATH = 205,
	LPGP_LWPNEWUSER = 206,
	LPGP_LWPNEWUSERDATA = 207,
	LPGP_LWPQUERYNICK = 208,
	LPGP_LWPNICK = 209,

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

typedef struct _LWPTARGETDAMAGED {
	int type;
} LWPTARGETDAMAGED;

typedef struct _LWPUDPHEADER {
	int type;
	int battle_id;
	int token;
} LWPUDPHEADER;

// UDP
typedef struct _LWPMOVE {
	int type;
	int battle_id;
	int token;
	float dx;
	float dy;
} LWPMOVE;

// UDP
typedef struct _LWPSTOP {
	int type;
	int battle_id;
	int token;
} LWPSTOP;

// UDP
typedef struct _LWPDASH {
	int type;
	int battle_id;
	int token;
} LWPDASH;

// UDP
typedef struct _LWPPULLSTART {
	int type;
	int battle_id;
	int token;
} LWPPULLSTART;

// UDP
typedef struct _LWPPULLSTOP {
	int type;
	int battle_id;
	int token;
} LWPPULLSTOP;

// UDP
typedef struct _LWPSTATE {
	int type;
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
	// HP
	int player_current_hp;
	int player_total_hp;
	int target_current_hp;
	int target_total_hp;
} LWPSTATE;

// should be 4-byte aligned...
// (Cgo compatability issue)
//#pragma pack(push, 1)
typedef struct _LWPNEWUSER {
	unsigned short size;
	unsigned short type;
} LWPNEWUSER;

typedef struct _LWPQUERYNICK {
	unsigned short Size;
	unsigned short Type;
	unsigned int Id[4];
} LWPQUERYNICK;

#define LW_NICKNAME_MAX_LEN (32)

typedef struct _LWPNICK {
	unsigned short size;
	unsigned short type;
	char nickname[LW_NICKNAME_MAX_LEN];
} LWPNICK;

typedef struct _LWPNEWUSERDATA {
	unsigned short size;
	unsigned short type;
	unsigned int id[4];
	char nickname[32];
} LWPNEWUSERDATA;

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
	unsigned short padding_unused;
	unsigned char ipaddr[4];
	int battle_id;
	unsigned int token;
	int player_no;
} LWPMATCHED2;

typedef struct _LWPBASE {
	unsigned short size;
	unsigned short type;
} LWPBASE;

typedef struct _LWPCREATEBATTLE {
	unsigned short size;
	unsigned short type;
} LWPCREATEBATTLE;

typedef struct _LWPCREATEBATTLEOK {
	unsigned short Size;
	unsigned short Type;
	int Battle_id;
	unsigned int C1_token;
	unsigned int C2_token;
	unsigned char IpAddr[4];
	unsigned short Port;
	unsigned short Padding_unused;
} LWPCREATEBATTLEOK;

typedef struct _LWPSUDDENDEATH {
	unsigned short Size;
	unsigned short Type;
	int Battle_id;
	unsigned int Token;
} LWPSUDDENDEATH;

//#pragma pack(pop)
