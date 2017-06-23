// ReSharper disable CppUnusedIncludeDirective
#include <inttypes.h> // Needed for suppressing 'PRId6' redefined warning.
// ReSharper restore CppUnusedIncludeDirective
#include <stdlib.h>
#include <czmq.h>
#include <ode/ode.h>
#include "field.h"
#include "lwanim.h"
#include "laidoff.h"
#include "input.h"
#include "lwlog.h"
#include "file.h"
#include "nav.h"
#include "mq.h"
#include "extrapolator.h"
#include "playersm.h"
#include "pcg_basic.h"
#include "ps.h"

#define MAX_BOX_GEOM (100)
#define MAX_RAY_RESULT_COUNT (10)
#define MAX_FIELD_CONTACT (10)

typedef struct _LWFIELDCUBEOBJECT {
	float x, y, z;
	float dimx, dimy, dimz;
	float axis_angle[4];
} LWFIELDCUBEOBJECT;

typedef enum _LW_SPACE_GROUP {
	LSG_PLAYER,
	LSG_WORLD,
	LSG_RAY,
	LSG_BULLET,
	LSG_ENEMY,

	LSG_COUNT,
} LW_SPACE_GROUP;

typedef struct _LWFIELD {
	// ODE world instance
	dWorldID world;
	// Root space
	dSpaceID space;
	// Space array
	dSpaceID space_group[LSG_COUNT];
	// Worldwide ground geom
	dGeomID ground;
	// Box(world collider) geom array
	dGeomID box_geom[MAX_BOX_GEOM];
	// Box(world collider) geom count
	int box_geom_count;
	// Player geom
	dGeomID player_geom;
	// Test player geom
	dGeomID test_player_geom;
	// Player collision capsule radius
	dReal player_radius;
	// Player collision capsule length
	dReal player_length;
	// User(remote player) collision capsule radius
	dReal user_radius;
	// User(remote player) collision capsule length
	dReal user_length;
	// Ray test max length
	dReal ray_max_length;
	// Ray geom array
	dGeomID ray[LRI_COUNT];
	// Ray result array
	dContact ray_result[LRI_COUNT][MAX_RAY_RESULT_COUNT];
	// Ray result count
	int ray_result_count[LRI_COUNT];
	// Nearest ray depth value
	dReal ray_nearest_depth[LRI_COUNT];
	// Nearest ray result index
	int ray_nearest_index[LRI_COUNT];
	// Sphere(bullet) geom (this bullet is fired by player)
	dGeomID sphere[MAX_FIELD_SPHERE];
	// Sphere(bullet) constant velocity (this bullet is fired by player)
	vec3 sphere_vel[MAX_FIELD_SPHERE];
	// Sphere(bullet) ID (this bullet is fired by player)
	int sphere_bullet_id[MAX_FIELD_SPHERE];
	// Sphere(bullet) body (this bullet is fired by player)
	dBodyID sphere_body[MAX_FIELD_SPHERE];
	// User(remote player) geom array
	dGeomID user[MAX_USER_GEOM];
	// Remote sphere(bullet) world position
	vec3 remote_sphere_pos[MAX_FIELD_REMOTE_SPHERE];
	// Remote sphere(bullet) constant velocity
	vec3 remote_sphere_vel[MAX_FIELD_REMOTE_SPHERE];
	// Remote sphere(bullet) valid flag
	int remote_sphere_valid[MAX_FIELD_REMOTE_SPHERE];
	// Remote sphere(bullet) ID
	int remote_sphere_bullet_id[MAX_FIELD_REMOTE_SPHERE];
	// Remote sphere(bullet) owner key (string) array
	char remote_sphere_owner_key[MAX_FIELD_REMOTE_SPHERE][LW_KVMSG_KEY_MAX];
	// Current player position
	dVector3 player_pos;
	// Player position delta since last tick
	dVector3 player_pos_delta;
	// Ground normal at player standing
	dVector3 ground_normal;
	// Player velocity
	dVector3 player_vel;
	// Field cube object array
	LWFIELDCUBEOBJECT* field_cube_object;
	// Field cube object count
	int field_cube_object_count;
	// Field raw file data
	char* field_raw_data;
	// Navigation data instance
	void* nav;
	// Path query result for testing
	LWPATHQUERY path_query_test;
	// Test player position
	vec3 path_query_test_player_pos;
	// Test player orientation
	float path_query_test_player_rot;
	// Field VBO
	LW_VBO_TYPE field_vbo;
	// Field texture ID
	GLuint field_tex_id;
	// 1 if field texture is mipmapped, 0 if otherwise
	int field_tex_mip;
	// Player skinned mesh scale
	float skin_scale;
	// Field world camera mode (0 or 1)
	int field_camera_mode;
	// Random number generator instance
	pcg32_random_t rng;
	// Test player flash effect opacity (0 ~ 1)
	float test_player_flash;
	// Player flash effect opacity (0 ~ 1)
	float player_flash;
	// 1 if network is enabled, 0 if otherwise
	int network;
	// Message queue instance (copied from context)
	void* mq;
	// Particle system manager instance
	void* ps;
} LWFIELD;

static void s_rotation_matrix_from_vectors(dMatrix3 r, const dReal* vec_a, const dReal* vec_b);
static void field_set_user_pos(LWFIELD* field, int idx, const vec3 pos);

dReal get_dreal_max() {
#ifdef dDOUBLE
	return DBL_MAX;
#else
	return FLT_MAX;
#endif
}

dReal get_dreal_min() {
#ifdef dDOUBLE
	return DBL_MIN;
#else
	return FLT_MIN;
#endif
}

void move_player(LWCONTEXT *pLwc) {
	if (pLwc->game_scene == LGS_FIELD) {
		const float move_speed = 3.5f;
		const float move_speed_delta = (float)(lwcontext_delta_time(pLwc) * move_speed);

		// Using keyboard
		pLwc->player_pos_x += (float)((pLwc->player_move_right - pLwc->player_move_left) *
			move_speed_delta);
		pLwc->player_pos_y += (float)((pLwc->player_move_up - pLwc->player_move_down) *
			move_speed_delta);

		// Using mouse
		float dx = 0, dy = 0, dlen = 0;
		if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen) && (dx || dy)) {
			pLwc->player_pos_x += dx * move_speed_delta;
			pLwc->player_pos_y += dy * move_speed_delta;
			pLwc->player_state_data.rot_z = atan2f(dy, dx);
			pLwc->player_pos_last_moved_dx = dx;
			pLwc->player_pos_last_moved_dy = dy;
			//pLwc->player_moving = 1;

			set_field_player_delta(pLwc->field, dx * move_speed_delta, dy * move_speed_delta, 0);
		} else {
			//pLwc->player_moving = 0;
		}
	}
}

static float resolve_collision_one_fixed_axis(float fp, float fs, float mp, float ms) {
	const float f_lo = fp - fs / 2;
	const float f_hi = fp + fs / 2;
	const float m_lo = mp - ms / 2;
	const float m_hi = mp + ms / 2;
	float displacement = 0;
	if ((f_lo < m_lo && m_lo < f_hi) || (f_lo < m_hi && m_hi < f_hi) ||
		(m_lo < f_lo && f_hi < m_hi)) {
		if (fp < mp) {
			displacement = f_hi - m_lo;
		} else {
			displacement = -(m_hi - f_lo);
		}
	}

	return displacement;
}

static void resolve_collision_one_fixed(LWCONTEXT* pLwc, const LWBOX2DCOLLIDER *fixed, LWBOX2DCOLLIDER *movable) {
	const float dx = resolve_collision_one_fixed_axis(fixed->x, fixed->w, movable->x, movable->w);

	if (dx) {
		const float dy = resolve_collision_one_fixed_axis(fixed->y, fixed->h, movable->y,
			movable->h);

		if (dy) {

			// Enemy encounter
			if (fixed->field_event_id >= 1 && fixed->field_event_id <= 5) {
				pLwc->field_event_id = fixed->field_event_id;
				change_to_battle(pLwc);
			}

			if (fixed->field_event_id == 6) {
				pLwc->field_event_id = fixed->field_event_id;
				change_to_dialog(pLwc);
			}


			if (fabs(dx) < fabs(dy)) {
				movable->x += dx;
			} else {
				movable->y += dy;
			}
		}
	}
}

void resolve_player_collision(LWCONTEXT *pLwc) {
	LWBOX2DCOLLIDER player_collider;
	player_collider.x = pLwc->player_pos_x;
	player_collider.y = pLwc->player_pos_y;
	player_collider.w = 1.0f;
	player_collider.h = 1.0f;

	for (int i = 0; i < MAX_BOX_COLLIDER; i++) {
		if (pLwc->box_collider[i].valid) {
			resolve_collision_one_fixed(pLwc, &pLwc->box_collider[i], &player_collider);
		}
	}

	pLwc->player_pos_x = player_collider.x;
	pLwc->player_pos_y = player_collider.y;
}

LWFIELD* load_field(const char* filename) {
	// Initialize OpenDE
	dInitODE2(0);
	// Create a new field instance
	LWFIELD* field = (LWFIELD*)calloc(1, sizeof(LWFIELD));
	// Create a world and a root space 
	field->world = dWorldCreate();
	field->space = dHashSpaceCreate(0);
	// Create space groups
	for (int i = 0; i < LSG_COUNT; i++) {
		field->space_group[i] = dHashSpaceCreate(field->space);
	}
	// Create ground infinite plane geom
	field->ground = dCreatePlane(field->space_group[LSG_WORLD], 0, 0, 1, 0);
	// Set player geom constants
	field->player_radius = (dReal)0.75;
	field->player_length = (dReal)3.0;
	field->player_pos[0] = 0;
	field->player_pos[1] = 0;
	field->player_pos[2] = 10;
	// Set user geom constants
	field->user_radius = field->player_radius * 2;
	field->user_length = field->player_length;
	// Create player geom
	field->player_geom = dCreateCapsule(field->space_group[LSG_PLAYER], field->player_radius, field->player_length);
	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	// Create test player geom
	field->test_player_geom = dCreateCapsule(field->space_group[LSG_ENEMY], field->player_radius, field->player_length);
	// Create ray geoms (see LW_RAY_ID enum for usage)
	field->ray_max_length = 50;
	for (int i = 0; i < LRI_COUNT; i++) {
		field->ray[i] = dCreateRay(field->space_group[LSG_RAY], field->ray_max_length);
		dGeomSetData(field->ray[i], (void*)i); // Use ray ID as custom data for quick indexing in near callback
		dGeomSetPosition(field->ray[i], field->player_pos[0], field->player_pos[1], field->player_pos[2]);
		dMatrix3 r;
		dRFromAxisAndAngle(r, 1, 0, 0, M_PI); // Make ray direction downward (-Z axis)
		dGeomSetRotation(field->ray[i], r);
	}
	// Aim sector rays are disabled from start (will be enabled just after aiming has started)
	field_set_aim_sector_ray(field, 0);
	// Set initial ground normal as +Z axis
	dAssignVector3(field->ground_normal, 0, 0, 1);
	// Load field cube objects (field colliders) from specified file
	size_t size;
	char* d = create_binary_from_file(filename, &size);
	field->field_raw_data = d;
	field->field_cube_object = (LWFIELDCUBEOBJECT*)d;
	field->field_cube_object_count = size / sizeof(LWFIELDCUBEOBJECT);
	// Create geoms for field cube objects
	for (int i = 0; i < field->field_cube_object_count; i++) {
		const LWFIELDCUBEOBJECT* lco = &field->field_cube_object[i];

		field->box_geom[field->box_geom_count] = dCreateBox(field->space_group[LSG_WORLD], lco->dimx, lco->dimy, lco->dimz);
		dMatrix3 r;
		dRFromAxisAndAngle(r, lco->axis_angle[0], lco->axis_angle[1], lco->axis_angle[2], lco->axis_angle[3]);
		dGeomSetPosition(field->box_geom[field->box_geom_count], lco->x, lco->y, lco->z);
		dGeomSetRotation(field->box_geom[field->box_geom_count], r);

		field->box_geom_count++;
	}
	// Create sphere geom pool for bullets, projectiles, ...
	// which are initially disabled and enabled before they are used.
	for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
		field->sphere[i] = dCreateSphere(field->space_group[LSG_BULLET], 0.5f);
		field->sphere_body[i] = dBodyCreate(field->world);
		//dMassSetSphere(field->sphere_body[i], 1, 1);
		//dMassAdjust(field->sphere_body[i], 1);
		dBodySetKinematic(field->sphere_body[i]);
		//dBodyDisable(field->sphere_body[i]);
		dGeomSetBody(field->sphere[i], field->sphere_body[i]);
		dGeomDisable(field->sphere[i]);
	}
	// Create user(enemy) geom pool
	// which are initially disabled and enabled before they are used.
	for (int i = 0; i < MAX_USER_GEOM; i++) {
		field->user[i] = dCreateCapsule(field->space_group[LSG_ENEMY], field->user_radius, field->user_length);
		dGeomDisable(field->user[i]);
	}
	// Seed a random number generator
	pcg32_srandom_r(&field->rng, 0x0DEEC2CBADF00D77, 0x15881588CA11DAC1);
	//pcg32_srandom_r(&field->rng, 3, 56);
	// Particle system
	field->ps = ps_new();
	// Here it is. A brand new field instance just out of the oven!
	return field;
}

static int s_is_ray_or_player_geom(const LWFIELD* field, dGeomID o) {
	if (field->player_geom == o) {
		return 1;
	}

	for (int i = 0; i < LRI_COUNT; i++) {
		if (field->ray[i] == o) {
			return 1;
		}
	}

	return 0;
}

static void field_world_ray_near(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;
	assert(dGeomGetSpace(o1) == field->space_group[LSG_WORLD]);
	assert(dGeomGetSpace(o2) == field->space_group[LSG_RAY]);

	dContact contact[MAX_FIELD_CONTACT];
	int n = dCollide(o1, o2, MAX_FIELD_CONTACT, &contact[0].geom, sizeof(dContact));
	for (int i = 0; i < n; i++) {
		// Get ray index from custom data
		// Note that the return value of dGeomGetData is coerced to integer since LW_RAY_ID is a sparse enum
		const LW_RAY_ID lri = (int)dGeomGetData(o2);
		// Negate normal direction of contact result data
		dNegateVector3(contact[i].geom.normal);
		// Copy to ours
		field->ray_result[lri][field->ray_result_count[lri]] = contact[i];
		// Increase ray result count
		field->ray_result_count[lri]++;
	}
}

static void field_player_world_near(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;
	assert(dGeomGetSpace(o1) == field->space_group[LSG_PLAYER]);
	assert(dGeomGetSpace(o2) == field->space_group[LSG_WORLD]);

	dContact contact[MAX_FIELD_CONTACT];
	int n = dCollide(o1, o2, MAX_FIELD_CONTACT, &contact[0].geom, sizeof(dContact));
	for (int i = 0; i < n; i++) {

		dReal sign = 0;
		if (contact[i].geom.g1 == field->player_geom) {
			sign = 1;
		} else if (contact[i].geom.g2 == field->player_geom) {
			sign = -1;
		}

		if (sign) {
			dReal* normal = contact[i].geom.normal;
			dReal depth = contact[i].geom.depth;

			field->player_pos[0] += sign * normal[0] * depth;
			field->player_pos[1] += sign * normal[1] * depth;
		}
	}
}

static void field_player_bullet_near(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;
	assert(dGeomGetSpace(o1) == field->space_group[LSG_PLAYER]);
	assert(dGeomGetSpace(o2) == field->space_group[LSG_BULLET]);

	dContact contact[MAX_FIELD_CONTACT];
	int n = dCollide(o1, o2, MAX_FIELD_CONTACT, &contact[0].geom, sizeof(dContact));
	for (int i = 0; i < n; i++) {
	}
}

static void field_world_bullet_near(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;
	assert(dGeomGetSpace(o1) == field->space_group[LSG_WORLD]);
	assert(dGeomGetSpace(o2) == field->space_group[LSG_BULLET]);

	dContact contact[1];
	int n = dCollide(o1, o2, 1, &contact[0].geom, sizeof(dContact));
	if (n > 0) {
		LOGI("Bullet at %f,%f,%f (wall)",
			contact[0].geom.pos[0],
			contact[0].geom.pos[1],
			contact[0].geom.pos[2]);
		dGeomDisable(o2);
		int idx = (int)dGeomGetData(o2);
		mq_send_despawn_bullet(field->mq, field->sphere_bullet_id[idx]);
		ps_play_new(field->ps);
	}
}

void field_send_hit(LWFIELD* field, LWPOSSYNCMSG* possyncmsg) {
	mq_send_hit(field->mq, possyncmsg);
}

static void field_test_player_bullet_near(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;
	assert(dGeomGetSpace(o1) == field->space_group[LSG_ENEMY]);
	assert(dGeomGetSpace(o2) == field->space_group[LSG_BULLET]);

	dContact contact[1];
	int n = dCollide(o1, o2, 1, &contact[0].geom, sizeof(dContact));
	if (n > 0) {
		LOGI("Bullet HIT at %f,%f,%f (enemy)",
			contact[0].geom.pos[0],
			contact[0].geom.pos[1],
			contact[0].geom.pos[2]);
		dGeomDisable(o2);
		void* geom_data = dGeomGetData(o1);
		if (geom_data) {
			// Player hit enemy
			LWPOSSYNCMSG* possyncmsg = (LWPOSSYNCMSG*)geom_data;
			possyncmsg->flash = 1.0f;
			if (field->network) {
				field_send_hit(field, possyncmsg);
			}
			int idx = (int)dGeomGetData(o2);
			mq_send_despawn_bullet(field->mq, field->sphere_bullet_id[idx]);
		} else {
			field->test_player_flash = 1.0f;
		}
	}
}

static void collide_between_spaces(LWFIELD* field, dSpaceID o1, dSpaceID o2, dNearCallback* callback) {
	dSpaceCollide2((dGeomID)o1, (dGeomID)o2, field, callback);
	// collision between objects within the same space.
	//dSpaceCollide(o1, field, callback);
	//dSpaceCollide(o2, field, callback);
}

void reset_ray_result(LWFIELD* field) {
	for (int i = 0; i < LRI_COUNT; i++) {
		field->ray_result_count[i] = 0;
		field->ray_nearest_depth[i] = field->ray_max_length;// get_dreal_max();
		field->ray_nearest_index[i] = -1;
	}
}

void set_field_player_delta(LWFIELD* field, float x, float y, float z) {
	field->player_pos_delta[0] = x;
	field->player_pos_delta[1] = y;
	field->player_pos_delta[2] = z;
}

void set_field_player_position(LWFIELD* field, float x, float y, float z) {
	field->player_pos[0] = x;
	field->player_pos[1] = y;
	field->player_pos[2] = z + field->player_length / 2 + field->player_radius;
}

void get_field_player_position(const LWFIELD* field, float* x, float* y, float* z) {
	*x = (float)field->player_pos[0];
	*y = (float)field->player_pos[1];
	*z = (float)(field->player_pos[2] - field->player_length / 2 - field->player_radius);
}

void get_field_player_geom_position(const LWFIELD* field, float* x, float* y, float* z) {
	*x = (float)field->player_pos[0];
	*y = (float)field->player_pos[1];
	*z = (float)field->player_pos[2];
}

static void s_rotation_matrix_from_vectors(dMatrix3 r, const dReal* vec_a, const dReal* vec_b) {
	// Calculate rotation matrix 'r' which rotates 'vec_a' to 'vec_b'.
	// Assumes that both 'vec_a' and 'vec_b' are unit vectors.
	// http://stackoverflow.com/questions/1171849/finding-quaternion-representing-the-rotation-from-one-vector-to-another
	dVector3 a_x_b;
	dCalcVectorCross3(a_x_b, vec_a, vec_b);
	dReal dot = dCalcVectorDot3(vec_a, vec_b);
	dQuaternion q;
	q[0] = 1 + dot;
	q[1] = a_x_b[0];
	q[2] = a_x_b[1];
	q[3] = a_x_b[2];
	dNormalize4(q);
	dQtoR(q, r);
}

void rotation_matrix_from_vectors(mat4x4 m, const vec3 a, const vec3 b) {
	dMatrix3 r;
	dVector3 vec_a = { a[0], a[1], a[2] };
	dVector3 vec_b = { b[0], b[1], b[2] };
	s_rotation_matrix_from_vectors(r, vec_a, vec_b);
	m[0][0] = (float)r[0]; m[0][1] = (float)r[4]; m[0][2] = (float)r[8]; m[0][3] = 0;
	m[1][0] = (float)r[1]; m[1][1] = (float)r[5]; m[1][2] = (float)r[9]; m[1][3] = 0;
	m[2][0] = (float)r[2]; m[2][1] = (float)r[6]; m[2][2] = (float)r[10]; m[2][3] = 0;
	m[3][0] = (float)r[3]; m[3][1] = (float)r[7]; m[3][2] = (float)r[11]; m[3][3] = 1;
}

void move_player_geom_by_input(LWFIELD* field) {
	dMatrix3 r;
	dVector3 up;
	up[0] = 0;
	up[1] = 0;
	up[2] = 1;
	s_rotation_matrix_from_vectors(r, up, field->ground_normal);

	// Match 'player_pos_delta' (XY-plane movement) to the ground.
	dVector3 geom_pos_delta_rotated;
	dMultiply0_331(geom_pos_delta_rotated, r, field->player_pos_delta);

	for (int i = 0; i < 3; i++) {
		field->player_pos[i] += geom_pos_delta_rotated[i];
		// Reset to zero to make movement stop in subsequent frames.
		// (can only move again if another player_pos_delta is set by user input.)
		field->player_pos_delta[i] = 0;
	}
	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dGeomSetPosition(field->ray[LRI_PLAYER_CENTER], field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	const dReal* player_contact_ray_pos = dGeomGetPosition(field->ray[LRI_PLAYER_CONTACT]);
	dGeomSetPosition(field->ray[LRI_PLAYER_CONTACT],
		player_contact_ray_pos[0] + geom_pos_delta_rotated[0],
		player_contact_ray_pos[1] + geom_pos_delta_rotated[1],
		player_contact_ray_pos[2] + geom_pos_delta_rotated[2]);
}

void move_player_to_ground(LWFIELD* field) {
	const int center_nearest_ray_index = field->ray_nearest_index[LRI_PLAYER_CENTER];

	if (center_nearest_ray_index >= 0) {

		dCopyVector3(field->ground_normal, field->ray_result[LRI_PLAYER_CENTER][center_nearest_ray_index].geom.normal);

		dReal tz = field->ray_result[LRI_PLAYER_CENTER][center_nearest_ray_index].geom.pos[2] + field->player_radius / field->ray_result[LRI_PLAYER_CENTER][center_nearest_ray_index].geom.normal[2] + field->player_length / 2;

		dReal d1 = tz - field->player_pos[2];

		dReal d2 = get_dreal_min();
		const int contact_nearest_ray_index = field->ray_nearest_index[LRI_PLAYER_CONTACT];
		if (contact_nearest_ray_index >= 0) {
			dReal d = field->ray_nearest_depth[LRI_PLAYER_CONTACT] - field->player_length / 2 - field->player_radius;
			d2 = -d;
		}

		if (d1 > d2) {
			field->player_pos[2] += d2;
		} else {
			field->player_pos[2] += d1;
		}
	}

	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dGeomSetPosition(field->ray[LRI_PLAYER_CENTER], field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	if (center_nearest_ray_index >= 0) {
		dGeomSetPosition(field->ray[LRI_PLAYER_CONTACT],
			field->player_pos[0] - field->player_radius * field->ray_result[LRI_PLAYER_CENTER][center_nearest_ray_index].geom.normal[0],
			field->player_pos[1] - field->player_radius * field->ray_result[LRI_PLAYER_CENTER][center_nearest_ray_index].geom.normal[1],
			field->player_pos[2]);

	} else {
		dGeomSetPosition(field->ray[LRI_PLAYER_CONTACT], field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	}
}

void move_aim_ray(LWFIELD* field, float aim_theta, float rot_z) {
	dVector3 up = { 1, 0, 0 };
	for (int i = LRI_AIM_SECTOR_FIRST_INCLUSIVE; i <= LRI_AIM_SECTOR_LAST_INCLUSIVE; i++) {
		dGeomSetPosition(field->ray[i], field->player_pos[0], field->player_pos[1], field->player_pos[2] - field->player_length / 2 /* foot level */);

		dMatrix3 r_minus_y;
		dMatrix3 r_z;
		dMatrix3 r;
		const double dtheta = -aim_theta / 2 + aim_theta / MAX_AIM_SECTOR_RAY * (i - LRI_AIM_SECTOR_FIRST_INCLUSIVE);

		// rot_z == 0 --> global +X
		dRFromAxisAndAngle(r_minus_y, 1, 0, 0, M_PI / 2); // ray direction: global -Y
		dRFromAxisAndAngle(r_z, 0, 0, 1, rot_z + LWDEG2RAD(90) + dtheta); // rotation around global +Z
		dMultiply0_333(r, r_z, r_minus_y);
		dGeomSetRotation(field->ray[i], r);
	}
}

void gather_ray_result(LWFIELD* field) {
	for (int i = 0; i < LRI_COUNT; i++) {
		dReal nearest_ray_length = field->ray_max_length; // get_dreal_max();
		int nearest_ray_index = -1;
		for (int j = 0; j < field->ray_result_count[i]; j++) {
			if (nearest_ray_length > field->ray_result[i][j].geom.depth) {
				nearest_ray_length = field->ray_result[i][j].geom.depth;
				nearest_ray_index = j;
			}
		}
		field->ray_nearest_depth[i] = nearest_ray_length;
		field->ray_nearest_index[i] = nearest_ray_index;
	}
}

void field_enable_ray_test(LWFIELD* field, int enable) {
	for (int i = LRI_AIM_SECTOR_FIRST_INCLUSIVE; i <= LRI_AIM_SECTOR_LAST_INCLUSIVE; i++) {
		if (dGeomIsEnabled(field->ray[i])) {
			dGeomDisable(field->ray[i]);
		} else {
			dGeomEnable(field->ray[i]);
		}
	}
}

static void s_deactivate_out_of_domain_sphere(LWFIELD* field, float delta_time) {
	for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
		if (dGeomIsEnabled(field->sphere[i])) {
			const dReal* pos = dGeomGetPosition(field->sphere[i]);

			const dReal new_x = pos[0] + delta_time * field->sphere_vel[i][0];
			const dReal new_y = pos[1] + delta_time * field->sphere_vel[i][1];
			const dReal new_z = pos[2] + delta_time * field->sphere_vel[i][2];

			const dReal domain_size = 100.0f;

			if (new_x > domain_size || new_x < -domain_size
				|| new_y > domain_size || new_y < -domain_size
				|| new_z > domain_size || new_z < -domain_size) {
				// Out-of-domain geom should be disabled.
				LOGI("Field sphere[%d] out-of-bound. It will be disabled.", i);
				dGeomDisable(field->sphere[i]);
			}
		}
	}

	for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
		if (field->remote_sphere_valid[i]) {
			const float x = field->remote_sphere_pos[i][0];
			const float y = field->remote_sphere_pos[i][1];
			const float z = field->remote_sphere_pos[i][2];

			const dReal domain_size = 100.0f;

			if (x > domain_size || x < -domain_size
				|| y > domain_size || y < -domain_size
				|| z > domain_size || z < -domain_size) {
				// Out-of-domain geom should be disabled.
				LOGI("Field remote sphere[%d] out-of-bound. It will be disabled.", i);
				field->remote_sphere_valid[i] = 0;
			}
		}
	}
}

static void s_step_remote_sphere(LWFIELD* field, double delta_time) {
	for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
		if (!field->remote_sphere_valid[i]) {
			continue;
		}
		for (int j = 0; j < 3; j++) {
			field->remote_sphere_pos[i][j] += (float)(delta_time * field->remote_sphere_vel[i][j]);
		}
	}
}

void start_new_path_query_test(void* nav, LWPATHQUERY* pq) {
	set_random_start_end_pos(nav, pq);
	nav_query(nav, pq);
	pq->path_t = 0;
}

void update_path_query_test(void* nav, LWPATHQUERY* pq, float move_speed, float delta_time, float* out_pos, float* out_rot, dGeomID geom) {
	if (pq->n_smooth_path) {

		pq->path_t += delta_time;

		const float idx_f = fmodf((float)(pq->path_t * move_speed), (float)pq->n_smooth_path);
		const int idx = (int)idx_f;
		const float* p1 = &pq->smooth_path[3 * idx];
		// path query result's coordinates is different from world coordinates.
		const vec3 p1vec = { p1[0], -p1[2], p1[1] };
		if (idx < pq->n_smooth_path - 1) {
			const float* p2 = &pq->smooth_path[3 * (idx + 1)];
			// path query result's coordinates is different from world coordinates.
			const vec3 p2vec = { p2[0], -p2[2], p2[1] };
			// Calculate a midpoint between p1 and p2 to get interpolated position according to idx_f value
			const float p2coeff = idx_f - idx;
			const float p1coeff = 1.0f - p2coeff;
			const vec3 p1p2midvec = {
				p1vec[0] * p1coeff + p2vec[0] * p2coeff,
				p1vec[1] * p1coeff + p2vec[1] * p2coeff,
				p1vec[2] * p1coeff + p2vec[2] * p2coeff,
			};
			memcpy(out_pos, p1p2midvec, sizeof(vec3));
			// Set orientation
			*out_rot = atan2f(p2vec[1] - p1vec[1], p2vec[0] - p1vec[0]);
		} else {
			memcpy(out_pos, p1vec, sizeof(vec3));
		}
		// Update test player collider geom
		if (geom) {
			dGeomSetPosition(geom,
				out_pos[0],
				out_pos[1],
				out_pos[2]);
		}
		// Start querying another random path after the current pathfinding is completed
		if (idx >= pq->n_smooth_path - 1) {
			start_new_path_query_test(nav, pq);
		}
	}
}

void update_field(LWCONTEXT* pLwc, LWFIELD* field) {
	// Check for invalid input
	if (!pLwc) {
		//LOGE("%s: context null", __func__);
		return;
	}
	if (!field) {
		//LOGE("%s: field null", __func__);
		return;
	}
	// Get player position before integration (for calculating player velocity)
	dVector3 player_pos_0;
	dCopyVector3(player_pos_0, field->player_pos);
	// Move player geom by user input.
	// Collision not resolved after this function is called which means that
	// the player geom may overlapped with other geoms.
	move_player_geom_by_input(field);
	// Move aim (sector) rays accordingly.
	move_aim_ray(field, pLwc->player_state_data.aim_theta, pLwc->player_state_data.rot_z);
	// Ray result mutex
	mq_lock_mutex(pLwc->mq);
	{
		// Clear ray result from the previous frame
		reset_ray_result(field);
		// Resolve collision (ray-world)
		collide_between_spaces(field, field->space_group[LSG_WORLD], field->space_group[LSG_RAY], field_world_ray_near);
		// Gather ray result reported by collision report.
		// Specifically, get minimum ray length collision point.
		gather_ray_result(field);
	}
	mq_unlock_mutex(pLwc->mq);
	// Resolve collision (player-world collision)
	collide_between_spaces(field, field->space_group[LSG_PLAYER], field->space_group[LSG_WORLD], field_player_world_near);
	// Resolve collision (player-bullet collision)
	collide_between_spaces(field, field->space_group[LSG_PLAYER], field->space_group[LSG_BULLET], field_player_bullet_near);
	// Resolve collision (world-bullet collision)
	collide_between_spaces(field, field->space_group[LSG_WORLD], field->space_group[LSG_BULLET], field_world_bullet_near);
	// Resolve collision (test player-bullet collision)
	collide_between_spaces(field, field->space_group[LSG_ENEMY], field->space_group[LSG_BULLET], field_test_player_bullet_near);
	// Stepping the physics world (player-spawned bullet movement update)
	if (lwcontext_delta_time(pLwc) > 0) {
		dWorldStep(field->world, lwcontext_delta_time(pLwc));
	}
	// Remote-spawned movement update
	s_step_remote_sphere(field, lwcontext_delta_time(pLwc));
	// maybe-overlapped player geom's collision is resolved.
	// Bring the player straight to the ground.
	move_player_to_ground(field);
	// Calculate player velocity using backward euler formula.
	dSubtractVectors3(field->player_vel, field->player_pos, player_pos_0);
	dScaleVector3(field->player_vel, (dReal)1.0 / lwcontext_delta_time(pLwc));
	//LOGI("%f, %f, %f", field->player_vel[0], field->player_vel[1], field->player_vel[2]);
	// Get current player anim action
	LW_ACTION player_anim_0 = get_anim_by_state(pLwc->player_state_data.state, &pLwc->player_action_loop);
	pLwc->player_action = &pLwc->action[player_anim_0];
	// Get current frame index of the anim action
	float f = (float)(pLwc->player_state_data.skin_time * pLwc->player_action->fps);
	// Check current anim action sequence reaches its end
	const int player_action_animfin = f > pLwc->player_action->last_key_f;
	// Update player state data:
	// (1) Set inputs
	pLwc->player_state_data.delta_time = (float)lwcontext_delta_time(pLwc);
	pLwc->player_state_data.dir = pLwc->dir_pad_dragging;
	pLwc->player_state_data.atk = pLwc->atk_pad_dragging;
	pLwc->player_state_data.animfin = player_action_animfin;
	pLwc->player_state_data.aim_last_skin_time = pLwc->action[LWAC_HUMANACTION_STAND_AIM].last_key_f / pLwc->action[LWAC_HUMANACTION_STAND_AIM].fps;
	pLwc->player_state_data.field = field;
	pLwc->player_state_data.mq = pLwc->mq;
	pLwc->player_state_data.pLwc = pLwc;
	// (2) Get outputs - which is a new state
	pLwc->player_state_data.state = run_state(pLwc->player_state_data.state, &pLwc->player_state_data);
	// Get updated player anim action
	LW_ACTION player_anim_1 = get_anim_by_state(pLwc->player_state_data.state, &pLwc->player_action_loop);
	pLwc->player_action = &pLwc->action[player_anim_1];
	// Test player move speed
	const float move_speed = 10.0f;
	const float anim_speed = move_speed / 30.0f;
	// Update skin time
	pLwc->player_skin_time += (float)lwcontext_delta_time(pLwc);
	pLwc->test_player_skin_time += (float)lwcontext_delta_time(pLwc) * anim_speed;
	// Update pathfinding result and set the test player's position
	update_path_query_test(field->nav, &field->path_query_test, move_speed, (float)lwcontext_delta_time(pLwc),
		field->path_query_test_player_pos, &field->path_query_test_player_rot, field->test_player_geom);
	// Remote player update mutex begin
	mq_lock_mutex(pLwc->mq);
	{
		// Update remote players' position and orientation, anim action:
		LWPOSSYNCMSG* value = mq_possync_first(pLwc->mq);
		while (value) {
			const char* cursor = mq_possync_cursor(pLwc->mq);
			// Exclude the player
			if (!mq_cursor_player(pLwc->mq, cursor)) {
				float dx = 0, dy = 0;
				vec4_extrapolator_read(value->extrapolator, mq_sync_mono_clock(pLwc->mq), &value->x, &value->y, &value->z, &dx, &dy);

				//LOGI("READ: POS (%.2f, %.2f, %.2f) DXY (%.2f, %.2f)", value->x, value->y, value->z, dx, dy);

				value->a = atan2f(dy, dx);
				if (value->action >= 0 && value->action < LWAC_COUNT) {
					value->anim_action = &pLwc->action[(LW_ACTION)value->action];
				} else {
					LOGE("Unknown action enum detected on remote player. Has message structure been changed?");
					value->anim_action = &pLwc->action[LWAC_HUMANACTION_IDLE];
				}
				// Update(move) collider geom
				if (value->field == field) {
					if (value->geom_index >= 0) {
						const vec3 pos = { value->x, value->y, value->z };
						field_set_user_pos(field, value->geom_index, pos);
					} else {
						LOGE("Negative LWPOSSYNCMSG.geom_index! (%d)", value->geom_index);
					}
				} else {
					LOGE("LWPOSSYNCMSG.field (%p) is inconsistent with the current field! (%p)", value->field, field);
				}
				// Hit flash reduction of enemy
				value->flash = LWMAX(0, value->flash - (float)lwcontext_delta_time(pLwc) * 4);
			}
			value = mq_possync_next(pLwc->mq);
		}
	}
	mq_unlock_mutex(pLwc->mq);
	// Remote player update mutex ended
	s_deactivate_out_of_domain_sphere(field, (float)lwcontext_delta_time(pLwc));
	// Hit flash reduction of test player
	field->test_player_flash = LWMAX(0, field->test_player_flash - (float)lwcontext_delta_time(pLwc) * 4);
	field->player_flash = LWMAX(0, field->player_flash - (float)lwcontext_delta_time(pLwc) * 4);
	// Update particle system
	ps_update(field->ps, lwcontext_delta_time(pLwc));
}

void unload_field(LWFIELD* field) {
	if (!field) {
		return;
	}
	if (field->nav) {
		unload_nav(field->nav);
	}

	release_binary(field->field_raw_data);

	dGeomDestroy(field->ground);
	dGeomDestroy(field->player_geom);
	dGeomDestroy(field->test_player_geom);
	for (int i = 0; i < LRI_COUNT; i++) {
		dGeomDestroy(field->ray[i]);
	}
	for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
		dGeomDestroy(field->sphere[i]);
	}
	for (int i = 0; i < MAX_USER_GEOM; i++) {
		dGeomDestroy(field->user[i]);
	}
	for (int i = 0; i < field->box_geom_count; i++) {
		dGeomDestroy(field->box_geom[i]);
	}
	for (int i = 0; i < LSG_COUNT; i++) {
		dSpaceDestroy(field->space_group[i]);
	}
	dSpaceDestroy(field->space);
	dWorldDestroy(field->world);

	dCloseODE();

	ps_destroy((LWPS**)&field->ps);

	memset(field, 0, sizeof(LWFIELD));
}

void field_attack(LWCONTEXT* pLwc) {
	//if (!pLwc->player_attacking) {
	//	// start attack anim
	//	pLwc->player_attacking = 1;
	//	pLwc->player_skin_time = 0;
	//}
}

void field_path_query_spos(const LWFIELD* field, float* p) {
	p[0] = field->path_query_test.spos[0];
	p[1] = -field->path_query_test.spos[2];
	p[2] = field->path_query_test.spos[1];
}

void field_path_query_epos(const LWFIELD* field, float* p) {
	p[0] = field->path_query_test.epos[0];
	p[1] = -field->path_query_test.epos[2];
	p[2] = field->path_query_test.epos[1];
}

void field_set_path_query_spos(LWFIELD* field, float x, float y, float z) {
	field->path_query_test.spos[0] = x;
	field->path_query_test.spos[1] = z;
	field->path_query_test.spos[2] = -y;
}

void field_set_path_query_epos(LWFIELD* field, float x, float y, float z) {
	field->path_query_test.epos[0] = x;
	field->path_query_test.epos[1] = z;
	field->path_query_test.epos[2] = -y;
}

int field_path_query_n_smooth_path(const LWFIELD* field) {
	return field->path_query_test.n_smooth_path;
}

const float* field_path_query_test_player_pos(const LWFIELD* field) {
	return field->path_query_test_player_pos;
}

float field_path_query_test_player_rot(const LWFIELD* field) {
	return field->path_query_test_player_rot;
}

float field_skin_scale(const LWFIELD* field) {
	return field->skin_scale;
}

int field_follow_cam(const LWFIELD* field) {
	return field->field_camera_mode;
}

LW_VBO_TYPE field_field_vbo(const LWFIELD* field) {
	return field->field_vbo;
}

GLuint field_field_tex_id(const LWFIELD* field) {
	return field->field_tex_id;
}

int field_field_tex_mip(const LWFIELD* field) {
	return field->field_tex_mip;
}

double field_ray_nearest_depth(const LWFIELD* field, LW_RAY_ID lri) {
	return field->ray_nearest_depth[lri];
}

void field_nav_query(LWFIELD* field) {
	nav_query(field->nav, &field->path_query_test);
}

void init_field(LWCONTEXT* pLwc, const char* field_filename, const char* nav_filename, LW_VBO_TYPE vbo, GLuint tex_id, int tex_mip, float skin_scale, int follow_cam) {
	if (pLwc->field) {
		unload_field(pLwc->field);
	}

	pLwc->field = load_field(field_filename);
	pLwc->field->nav = load_nav(nav_filename);
	pLwc->field->field_vbo = vbo;
	pLwc->field->field_tex_id = tex_id;
	pLwc->field->field_tex_mip = tex_mip;
	pLwc->field->skin_scale = skin_scale;
	pLwc->field->field_camera_mode = follow_cam;
	pLwc->field->mq = pLwc->mq;	

	start_new_path_query_test(pLwc->field->nav, &pLwc->field->path_query_test);
}

int field_spawn_user(LWFIELD* field, vec3 pos, void* owner) {
	for (int i = 0; i < MAX_USER_GEOM; i++) {
		if (dGeomIsEnabled(field->user[i])) {
			continue;
		}
		dGeomSetPosition(field->user[i], pos[0], pos[1], pos[2]);
		dGeomEnable(field->user[i]);
		dGeomSetData(field->user[i], owner);
		LOGI("user geom enabled index %d", i);
		return i;
	}
	LOGE("%s: maximum spawn exceeded.", __func__);
	return -1;
}

void field_despawn_user(LWFIELD* field, int idx) {
	if (idx < 0 || idx >= MAX_USER_GEOM) {
		LOGE("%s: idx out of range (%d)", __func__, idx);
		return;
	}
	if (field->user[idx]) {
		dGeomDisable(field->user[idx]);
	}
	LOGI("user geom disabled index %d", idx);
}

static void field_set_user_pos(LWFIELD* field, int idx, const vec3 pos) {
	if (idx < 0 || idx >= MAX_USER_GEOM) {
		LOGE("%s: idx out of range (%d)", __func__, idx);
		return;
	}
	dGeomSetPosition(field->user[idx], pos[0], pos[1], pos[2]);
}

void field_spawn_remote_sphere(LWFIELD* field, vec3 pos, vec3 vel, int bullet_id, const char* owner_key) {
	for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
		if (field->remote_sphere_valid[i]) {
			continue;
		}
		field->remote_sphere_valid[i] = 1;
		memcpy(field->remote_sphere_pos[i], pos, sizeof(vec3));
		memcpy(field->remote_sphere_vel[i], vel, sizeof(vec3));
		field->remote_sphere_bullet_id[i] = bullet_id;
		strcpy(field->remote_sphere_owner_key[i], owner_key);
		break;
	}
}

void field_spawn_sphere(LWFIELD* field, vec3 pos, vec3 vel, int bullet_id) {
	for (int i = 0; i < MAX_FIELD_SPHERE; i++) {
		if (dGeomIsEnabled(field->sphere[i])) {
			continue;
		}
		//dGeomSetPosition(field->sphere[i], pos[0], pos[1], pos[2]);
		dBodySetPosition(field->sphere_body[i], pos[0], pos[1], pos[2]);
		dBodySetLinearVel(field->sphere_body[i], vel[0], vel[1], vel[2]);
		dGeomSphereSetRadius(field->sphere[i], 0.1f);
		dGeomEnable(field->sphere[i]);
		dGeomSetData(field->sphere[i], (void*)i);
		field->sphere_bullet_id[i] = bullet_id;
		field->sphere_vel[i][0] = vel[0];
		field->sphere_vel[i][1] = vel[1];
		field->sphere_vel[i][2] = vel[2];
		break;
	}
}

int field_sphere_pos(const LWFIELD* field, int i, float* pos) {
	if (!field->sphere[i] || !dGeomIsEnabled(field->sphere[i])) {
		return 0;
	}
	const dReal* p = dGeomGetPosition(field->sphere[i]);
	pos[0] = (float)p[0];
	pos[1] = (float)p[1];
	pos[2] = (float)p[2];
	return 1;
}

int field_sphere_vel(const LWFIELD* field, int i, float* vel) {
	if (!field->sphere[i] || !dGeomIsEnabled(field->sphere[i])) {
		return 0;
	}
	vel[0] = (float)field->sphere_vel[i][0];
	vel[1] = (float)field->sphere_vel[i][1];
	vel[2] = (float)field->sphere_vel[i][2];
	return 1;
}

int field_remote_sphere_pos(const LWFIELD* field, int i, float* pos) {
	if (!field->remote_sphere_valid[i]) {
		return 0;
	}
	memcpy(pos, field->remote_sphere_pos + i, sizeof(vec3));
	return 1;
}

int field_remote_sphere_vel(const LWFIELD* field, int i, float* vel) {
	if (!field->remote_sphere_valid[i]) {
		return 0;
	}
	memcpy(vel, field->remote_sphere_vel + i, sizeof(vec3));
	return 1;
}

float field_sphere_radius(const LWFIELD* field, int i) {
	if (!field->sphere[i] || !dGeomIsEnabled(field->sphere[i])) {
		return 0;
	}
	return (float)dGeomSphereGetRadius(field->sphere[i]);
}

// return 0 <= x < bound unsigned integer
unsigned int field_random_unsigned_int(LWFIELD* field, unsigned int bound) {
	return pcg32_boundedrand_r(&field->rng, bound);
}

// return [0, 1) double
double field_random_double(LWFIELD* field) {
	uint32_t v = pcg32_random_r(&field->rng);
	//uint32_t v = pcg32_random();
	//LOGI("v = 0x%08x", v);
	return ldexp(v, -32);
}

float field_test_player_flash(const LWFIELD* field) {
	return field->test_player_flash;
}

float field_player_flash(const LWFIELD* field) {
	return field->player_flash;
}

int field_network(const LWFIELD* field) {
	return field ? field->network : 0;
}

void field_set_network(LWFIELD* field, int network_poll) {
	if (field) {
		field->network = network_poll;
	}
}

void field_set_aim_sector_ray(LWFIELD* field, int enable) {
	for (int i = LRI_AIM_SECTOR_FIRST_INCLUSIVE; i <= LRI_AIM_SECTOR_LAST_INCLUSIVE; i++) {
		(enable ? dGeomEnable : dGeomDisable)(field->ray[i]);
	}
}

void field_hit_player(LWFIELD* field) {
	field->player_flash = 1.0f;
}

void field_despawn_remote_sphere(LWFIELD* field, int bullet_id, const char* owner_key) {
	for (int i = 0; i < MAX_FIELD_REMOTE_SPHERE; i++) {
		if (!field->remote_sphere_valid[i]) {
			continue;
		}
		if (field->remote_sphere_bullet_id[i] == bullet_id && strcmp(field->remote_sphere_owner_key[i], owner_key) == 0) {
			field->remote_sphere_valid[i] = 0;
			break;
		}
	}
}

void* field_ps(LWFIELD* field) {
	return field->ps;
}
