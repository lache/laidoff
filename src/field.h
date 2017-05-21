#pragma once
#include "lwgl.h"
#include "lwanim.h"
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

typedef struct _LWFIELD LWFIELD;

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
void field_path_query_spos(const LWFIELD* field, float* p);
void field_path_query_epos(const LWFIELD* field, float* p);
void field_set_path_query_spos(LWFIELD* field, float x, float y, float z);
void field_set_path_query_epos(LWFIELD* field, float x, float y, float z);
int field_path_query_n_smooth_path(const LWFIELD* field);
const float* field_path_query_test_player_pos(const LWFIELD* field);
float field_path_query_test_player_rot(const LWFIELD* field);
float field_skin_scale(const LWFIELD* field);
int field_follow_cam(const LWFIELD* field);
LW_VBO_TYPE field_field_vbo(const LWFIELD* field);
GLuint field_field_tex_id(const LWFIELD* field);
int field_field_tex_mip(const LWFIELD* field);
double field_ray_nearest_depth(const LWFIELD* field, LW_RAY_ID lri);
void field_nav_query(LWFIELD* field);
void init_field(LWCONTEXT* pLwc, const char* field_filename, const char* nav_filename, LW_VBO_TYPE vbo, GLuint tex_id, int tex_mip, float skin_scale, int follow_cam);
