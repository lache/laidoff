#pragma once
#include <ode/ode.h>
#include "lwgl.h"
#include "lwanim.h"
#include "nav.h"
#include "lwvbotype.h"
#include "vertices.h"

#define MAX_BOX_GEOM (100)
#define MAX_RAY_RESULT_COUNT (10)
#define MAX_FIELD_CONTACT (10)
#define MAX_AIM_SECTOR_RAY (FAN_SECTOR_COUNT_PER_ARRAY + 1) // 1 for the end vertex

typedef struct _LWFIELDCUBEOBJECT {
	float x, y, z;
	float dimx, dimy, dimz;
	float axis_angle[4];
} LWFIELDCUBEOBJECT;

typedef enum _LW_RAY_ID {
	LRI_PLAYER_CENTER,
	LRI_PLAYER_CONTACT,
	LRI_AIM_SECTOR_FIRST_INCLUSIVE,
	LRI_AIM_SECTOR_LAST_INCLUSIVE = LRI_AIM_SECTOR_FIRST_INCLUSIVE + MAX_AIM_SECTOR_RAY,

	LRI_COUNT
} LW_RAY_ID;

typedef struct _LWFIELD {
	dWorldID world;
	dSpaceID space;
	dGeomID ground;
	dGeomID box_geom[MAX_BOX_GEOM];
	int box_geom_count;
	dGeomID player_geom;
	dReal player_radius;
	dReal player_length;

	dReal ray_max_length;
	dGeomID ray[LRI_COUNT];
	dContact ray_result[LRI_COUNT][MAX_RAY_RESULT_COUNT];
	int ray_result_count[LRI_COUNT];
	dReal ray_nearest_depth[LRI_COUNT];
	int ray_nearest_index[LRI_COUNT];
	
	dVector3 player_pos;
	dVector3 player_pos_delta;
	dVector3 ground_normal;
	dVector3 player_vel;
	LWFIELDCUBEOBJECT* field_cube_object;
	int field_cube_object_count;
	char* d;

	void* nav;
	LWPATHQUERY path_query;
	float path_query_time;
	vec3 path_query_test_player_pos;
	float path_query_test_player_rot;

	LW_VBO_TYPE field_vbo;
	GLuint field_tex_id;
	int field_tex_mip;
	float skin_scale;
	int follow_cam;
} LWFIELD;

void move_player(LWCONTEXT *pLwc);
void resolve_player_collision(LWCONTEXT *pLwc);
LWFIELD* load_field(const char* filename);
void unload_field(LWFIELD* field);
void update_field(LWCONTEXT* pLwc, LWFIELD* field);
void set_field_player_delta(LWFIELD* field, float x, float y, float z);
void set_field_player_position(LWFIELD* field, float x, float y, float z);
void get_field_player_position(const LWFIELD* field, float* x, float* y, float* z);
void field_attack(LWCONTEXT* pLwc);
void field_enable_ray_test(LWFIELD* field, int enable);