#pragma once
#include "lwanim.h"
#include <ode/ode.h>

#define MAX_GROUND_BOX_ARRAY (100)
#define MAX_CENTER_RAY_RESULT (10)
#define MAX_CONTACT_RAY_RESULT (10)
#define MAX_FIELD_CONTACT (10)

typedef struct _LWFIELD {
	dWorldID world;
	dSpaceID space;
	dGeomID ground;
	dGeomID ground_box_array[MAX_GROUND_BOX_ARRAY];
	dGeomID player_geom;
	dGeomID player_center_ray;
	dGeomID player_contact_ray;
	dReal player_radius;
	dReal player_length;
	dContact center_ray_result[MAX_CENTER_RAY_RESULT];
	int center_ray_result_count;
	dContact contact_ray_result[MAX_CONTACT_RAY_RESULT];
	int contact_ray_result_count;
	dVector3 geom_pos;
	dVector3 geom_pos_delta;
	dVector3 ground_normal;
} LWFIELD;

void move_player(LWCONTEXT *pLwc);
void resolve_player_collision(LWCONTEXT *pLwc);
LWFIELD* load_field();
void update_field(LWFIELD* field);
