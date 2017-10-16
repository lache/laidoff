#pragma once
#include "lwatlasenum.h"
#include "lwvbotype.h"
#include "armature.h"
#include "lwanim.h"
#include <ode/ode.h>

typedef enum _LW_PUCK_GAME_BOUNDARY {
	LPGB_GROUND, LPGB_E, LPGB_W, LPGB_S, LPGB_N,
	LPGB_COUNT
} LW_PUCK_GAME_BOUNDARY;

typedef struct _LWPUCKGAMEOBJECT {
	dGeomID geom;
	dBodyID body;
	float pos[3]; // read-only
	float radius;
} LWPUCKGAMEOBJECT;

typedef struct _LWPUCKGAME {
	float world_size;
	float world_size_half;
	dWorldID world;
	dSpaceID space;
	dGeomID boundary[LPGB_COUNT];
	LWPUCKGAMEOBJECT testgo;
	dJointGroupID contact_joint_group;
	int push;
} LWPUCKGAME;

LWPUCKGAME* new_puck_game();
void delete_puck_game(LWPUCKGAME** puck_game);
void update_puck_game(LWPUCKGAME* puck_game);
void puck_game_push(LWPUCKGAME* puck_game);