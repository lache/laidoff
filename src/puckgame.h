#pragma once
#include "lwatlasenum.h"
#include "lwvbotype.h"
#include "armature.h"
#include "lwanim.h"
#include <ode/ode.h>

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWPUCKGAME LWPUCKGAME;

typedef enum _LW_PUCK_GAME_BOUNDARY {
	LPGB_GROUND, LPGB_E, LPGB_W, LPGB_S, LPGB_N,
	LPGB_COUNT
} LW_PUCK_GAME_BOUNDARY;

typedef enum _LW_PUCK_GAME_OBJECT {
	LPGO_PUCK,
	LPGO_PLAYER,
	LPGO_TARGET,
	LPGO_COUNT,
} LW_PUCK_GAME_OBJECT;

typedef struct _LWPUCKGAMEOBJECT {
	dGeomID geom;
	dBodyID body;
	float pos[3]; // read-only
	float radius;
	mat4x4 rot;
	LWPUCKGAME* puck_game;
	int wall_hit_count;
} LWPUCKGAMEOBJECT;

typedef struct _LWPUCKGAME {
	float world_size;
	float world_size_half;
	dWorldID world;
	dSpaceID space;
	dGeomID boundary[LPGB_COUNT];
	LWPUCKGAMEOBJECT go[LPGO_COUNT];
	dJointGroupID contact_joint_group;
	dJointGroupID  player_control_joint_group;
	dJointID player_control_joint;
	int push;
} LWPUCKGAME;

LWPUCKGAME* new_puck_game();
void delete_puck_game(LWPUCKGAME** puck_game);
void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_push(LWPUCKGAME* puck_game);
