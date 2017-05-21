#pragma once
#include "field.h"

typedef enum _LW_ACTION LW_ACTION;

typedef enum _LW_PLAYER_STATE {
	LPS_IDLE,
	LPS_MOVE,
	LPS_AIM,
	LPS_FIRE,
	LPS_UNAIM,

	LPS_COUNT,
} LW_PLAYER_STATE;

typedef struct _LWPLAYERSTATEDATA {
	LW_PLAYER_STATE state;
	int dir;			// 1 if direction control pressed
	int atk;			// 1 if attack control pressed
	//int firemarker;		// 1 if 'fire' animation marker raised
	int animfin;		// 1 if current animation reaches its end
	float aim_theta;
	float aim_theta_speed;
	float delta_time;
	float skin_time;
	float aim_last_skin_time; // hack hack hack
} LWPLAYERSTATEDATA;

LW_PLAYER_STATE run_state(LW_PLAYER_STATE cur_state, LWPLAYERSTATEDATA* data);
LW_ACTION get_anim_by_state(LW_PLAYER_STATE cur_state, int* loop);