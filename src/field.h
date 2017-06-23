#pragma once
#include "lwgl.h"
#include "lwanim.h"
#include "lwvbotype.h"
#include "vertices.h"

#define MAX_AIM_SECTOR_RAY (FAN_SECTOR_COUNT_PER_ARRAY + 1) // 1 for the end vertex
#define MAX_FIELD_SPHERE (50)
#define MAX_FIELD_REMOTE_SPHERE (50)
#define MAX_USER_GEOM (16)

typedef enum _LW_RAY_ID {
	LRI_PLAYER_CENTER,
	LRI_PLAYER_CONTACT,
	LRI_AIM_SECTOR_FIRST_INCLUSIVE,
	LRI_AIM_SECTOR_LAST_INCLUSIVE = LRI_AIM_SECTOR_FIRST_INCLUSIVE + MAX_AIM_SECTOR_RAY,

	LRI_COUNT
} LW_RAY_ID;

typedef struct _LWFIELD LWFIELD;
typedef struct _LWFIELDOBJECT LWFIELDOBJECT;

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
int field_sphere_pos(const LWFIELD* field, int i, float* pos);
int field_sphere_vel(const LWFIELD* field, int i, float* vel);
int field_spawn_user(LWFIELD* field, vec3 pos, void* owner);
void field_despawn_user(LWFIELD* field, int idx);
//void field_set_user_pos(LWFIELD* field, int idx, vec3 pos);
void field_spawn_sphere(LWFIELD* field, vec3 pos, vec3 vel, int bullet_id);
float field_sphere_radius(const LWFIELD* field, int i);
unsigned int field_random_unsigned_int(LWFIELD* field, unsigned int bound);
double field_random_double(LWFIELD* field);
void get_field_player_geom_position(const LWFIELD* field, float* x, float* y, float* z);
void rotation_matrix_from_vectors(mat4x4 m, const vec3 a, const vec3 b);
float field_test_player_flash(const LWFIELD* field);
float field_player_flash(const LWFIELD* field);
int field_network(const LWFIELD* field);
void field_set_network(LWFIELD* field, int enable_mq_poll);
void field_set_aim_sector_ray(LWFIELD* field, int enable);
void field_spawn_remote_sphere(LWFIELD* field, vec3 pos, vec3 vel, int bulelt_id, const char* owner_key);
int field_remote_sphere_pos(const LWFIELD* field, int i, float* pos);
int field_remote_sphere_vel(const LWFIELD* field, int i, float* vel);
void field_hit_player(LWFIELD* field);
void field_despawn_remote_sphere(LWFIELD* field, int bullet_id, const char* owner_key);
void* field_ps(LWFIELD* field);
int field_new_path_query(LWFIELD* field);
int field_update_output_path_query(LWFIELD* field, int idx, int val);
int spawn_field_object(struct _LWCONTEXT* pLwc, float x, float y, float w, float h, enum _LW_VBO_TYPE lvt, unsigned int tex_id, float sx, float sy, float alpha_multiplier, int field_event_id);
int despawn_field_object(struct _LWCONTEXT *pLwc, int idx);
void field_remove_field_object(LWFIELD* field, int field_event_id);
void despawn_all_field_object(LWFIELD* field);
LWFIELDOBJECT* field_object(LWFIELD* field, int idx);
int field_bind_path_query_output_location(LWFIELD* field, int idx, int field_object_idx);
