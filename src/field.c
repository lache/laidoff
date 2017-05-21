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

#define MAX_BOX_GEOM (100)
#define MAX_RAY_RESULT_COUNT (10)
#define MAX_FIELD_CONTACT (10)

typedef struct _LWFIELDCUBEOBJECT {
	float x, y, z;
	float dimx, dimy, dimz;
	float axis_angle[4];
} LWFIELDCUBEOBJECT;

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

void rotation_matrix_from_vectors(dMatrix3 r, const dReal* vec_a, const dReal* vec_b);

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
		const float move_speed_delta = (float)(pLwc->delta_time * move_speed);

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
			pLwc->player_rot_z = atan2f(dy, dx);
			pLwc->player_pos_last_moved_dx = dx;
			pLwc->player_pos_last_moved_dy = dy;
			pLwc->player_moving = 1;

			set_field_player_delta(pLwc->field, dx * move_speed_delta, dy * move_speed_delta, 0);
		} else {
			pLwc->player_moving = 0;
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

	dInitODE2(0);

	LWFIELD* field = (LWFIELD*)calloc(1, sizeof(LWFIELD));

	field->player_radius = (dReal)0.75;
	field->player_length = (dReal)3.0;
	field->world = dWorldCreate();
	field->space = dHashSpaceCreate(0);
	field->ground = dCreatePlane(field->space, 0, 0, 1, 0);

	// Player geom
	field->player_pos[0] = 0;
	field->player_pos[1] = 0;
	field->player_pos[2] = 10;
	field->player_geom = dCreateCapsule(field->space, field->player_radius, field->player_length);
	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);

	field->ray_max_length = 50;
	
	for (int i = 0; i < LRI_COUNT; i++) {
		field->ray[i] = dCreateRay(field->space, field->ray_max_length);
		dGeomSetPosition(field->ray[i], field->player_pos[0], field->player_pos[1], field->player_pos[2]);
		dMatrix3 R;
		dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
		dGeomSetRotation(field->ray[i], R);
	}
	//// Player center ray
	//field->player_center_ray = dCreateRay(field->space, ray_length);
	//dGeomSetPosition(field->player_center_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	//dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	//dGeomSetRotation(field->player_center_ray, R);

	//// Player contact ray
	//field->player_contact_ray = dCreateRay(field->space, ray_length);
	//dGeomSetPosition(field->player_contact_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	//dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	//dGeomSetRotation(field->player_contact_ray, R);
	
	dAssignVector3(field->ground_normal, 0, 0, 1);
	
	size_t size;
	char* d = create_binary_from_file(filename, &size);
	field->d = d;
	field->field_cube_object = (LWFIELDCUBEOBJECT*)d;
	field->field_cube_object_count = size / sizeof(LWFIELDCUBEOBJECT);

	for (int i = 0; i < field->field_cube_object_count; i++) {
		const LWFIELDCUBEOBJECT* lco = &field->field_cube_object[i];

		field->box_geom[field->box_geom_count] = dCreateBox(field->space, lco->dimx, lco->dimy, lco->dimz);
		dMatrix3 r;
		dRFromAxisAndAngle(r, lco->axis_angle[0], lco->axis_angle[1], lco->axis_angle[2], lco->axis_angle[3]);
		dGeomSetPosition(field->box_geom[field->box_geom_count], lco->x, lco->y, lco->z);
		dGeomSetRotation(field->box_geom[field->box_geom_count], r);

		field->box_geom_count++;
	}

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

static void field_near_callback(void *data, dGeomID o1, dGeomID o2) {
	LWFIELD* field = (LWFIELD*)data;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {

		// colliding a space with something :
		dSpaceCollide2(o1, o2, data, &field_near_callback);

		// collide all geoms internal to the space(s)
		if (dGeomIsSpace(o1))
			dSpaceCollide((dSpaceID)o1, data, &field_near_callback);
		if (dGeomIsSpace(o2))
			dSpaceCollide((dSpaceID)o2, data, &field_near_callback);

	} else {

		// only collide things with the ground
		int g1 = s_is_ray_or_player_geom(field, o1);
		int g2 = s_is_ray_or_player_geom(field, o2);
		if (!(g1 ^ g2)) {
			return;
		}

		dContact contact[MAX_FIELD_CONTACT];
		int n = dCollide(o1, o2, MAX_FIELD_CONTACT, &contact[0].geom, sizeof(dContact));
		if (n > 0) {
			for (int i = 0; i < n; i++) {

				for (int j = 0; j < LRI_COUNT; j++) {
					if (contact[i].geom.g1 == field->ray[j] || contact[i].geom.g2 == field->ray[j]) {
						field->ray_result[j][field->ray_result_count[j]] = contact[i];

						// Negate normal direction if 'g2' is ray. (in other words, not negate if 'g1' is ray)
						if (contact[i].geom.g2 == field->ray[j]) {
							dNegateVector3(field->ray_result[j][field->ray_result_count[j]].geom.normal);
						}

						field->ray_result_count[j]++;
					}
				}
				
				//if (contact[i].geom.g1 == field->player_center_ray || contact[i].geom.g2 == field->player_center_ray) {
				//	field->center_ray_result[field->center_ray_result_count] = contact[i];

				//	// Negate normal direction if 'g2' is ray. (in other words, not negate if 'g1' is ray)
				//	if (contact[i].geom.g2 == field->player_center_ray) {
				//		dNegateVector3(field->center_ray_result[field->center_ray_result_count].geom.normal);
				//	}

				//	field->center_ray_result_count++;
				//}

				//if (contact[i].geom.g1 == field->player_contact_ray || contact[i].geom.g2 == field->player_contact_ray) {
				//	field->contact_ray_result[field->contact_ray_result_count] = contact[i];

				//	// Negate normal direction if 'g2' is ray. (in other words, not negate if 'g1' is ray)
				//	if (contact[i].geom.g2 == field->player_contact_ray) {
				//		dNegateVector3(field->contact_ray_result[field->contact_ray_result_count].geom.normal);
				//	}

				//	field->contact_ray_result_count++;
				//}

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
	}
}

void reset_ray_result(LWFIELD* field) {
	for (int i = 0; i < LRI_COUNT; i++) {
		field->ray_result_count[i] = 0;
		field->ray_nearest_depth[i] = field->ray_max_length;// get_dreal_max();
		field->ray_nearest_index[i] = -1;
	}
	/*field->center_ray_result_count = 0;
	field->contact_ray_result_count = 0;*/
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

void rotation_matrix_from_vectors(dMatrix3 r, const dReal* vec_a, const dReal* vec_b) {
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

void move_player_geom_by_input(LWFIELD* field) {
	dMatrix3 r;
	dVector3 up;
	up[0] = 0;
	up[1] = 0;
	up[2] = 1;
	rotation_matrix_from_vectors(r, up, field->ground_normal);

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
	/*dReal min_ray_length = FLT_MAX;
	int min_ray_index = -1;
	for (int i = 0; i < field->ray_result_count[LRI_PLAYER_CENTER]; i++) {
		if (min_ray_length > field->ray_result[LRI_PLAYER_CENTER][i].geom.depth) {
			min_ray_length = field->ray_result[LRI_PLAYER_CENTER][i].geom.depth;
			min_ray_index = i;
		}
	}

	dReal min_side_ray_length = FLT_MAX;
	int min_side_ray_index = -1;
	for (int i = 0; i < field->ray_result_count[LRI_PLAYER_CONTACT]; i++) {
		if (min_side_ray_length > field->ray_result[LRI_PLAYER_CONTACT][i].geom.depth) {
			min_side_ray_length = field->ray_result[LRI_PLAYER_CONTACT][i].geom.depth;
			min_side_ray_index = i;
		}
	}*/

	const int center_nearest_ray_index = field->ray_nearest_index[LRI_PLAYER_CENTER];

	if (center_nearest_ray_index >= 0) {

		/*LOGI("min ray length:%.3f / g pos:%.2f,%.2f,%.2f / g nor:%.2f,%.2f,%.2f",
			field->center_ray_result[min_ray_index].geom.depth,
			field->center_ray_result[min_ray_index].geom.pos[0],
			field->center_ray_result[min_ray_index].geom.pos[1],
			field->center_ray_result[min_ray_index].geom.pos[2],
			field->center_ray_result[min_ray_index].geom.normal[0],
			field->center_ray_result[min_ray_index].geom.normal[1],
			field->center_ray_result[min_ray_index].geom.normal[2]);*/

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

void update_field(LWCONTEXT* pLwc, LWFIELD* field) {
	if (!field) {
		return;
	}

	reset_ray_result(field);

	dVector3 player_pos_0;
	dCopyVector3(player_pos_0, field->player_pos);

	move_player_geom_by_input(field);

	move_aim_ray(field, pLwc->player_state_data.aim_theta, pLwc->player_rot_z);
	
	dSpaceCollide(field->space, field, &field_near_callback);
	
	// No physics simulation needed for ray testing.
	//dWorldStep(field->world, 0.05);

	gather_ray_result(field);

	move_player_to_ground(field);

	dSubtractVectors3(field->player_vel, field->player_pos, player_pos_0);
	dScaleVector3(field->player_vel, (dReal)1.0 / pLwc->delta_time);

	//LOGI("%f, %f, %f", field->player_vel[0], field->player_vel[1], field->player_vel[2]);

	LW_ACTION player_anim_0 = get_anim_by_state(pLwc->player_state_data.state, &pLwc->player_action_loop);
	pLwc->player_action = &pLwc->action[player_anim_0];

	float f = (float)(pLwc->player_state_data.skin_time * pLwc->player_action->fps);
	const int player_action_animfin = f > pLwc->player_action->last_key_f;

	// Update player state data
	// Set inputs
	pLwc->player_state_data.delta_time = (float)pLwc->delta_time;
	pLwc->player_state_data.dir = pLwc->dir_pad_dragging;
	pLwc->player_state_data.atk = pLwc->atk_pad_dragging;
	pLwc->player_state_data.animfin = player_action_animfin;
	pLwc->player_state_data.aim_last_skin_time = pLwc->action[LWAC_HUMANACTION_STAND_AIM].last_key_f / pLwc->action[LWAC_HUMANACTION_STAND_AIM].fps;
	// Get outputs
	pLwc->player_state_data.state = run_state(pLwc->player_state_data.state, &pLwc->player_state_data);
	
	LW_ACTION player_anim_1 = get_anim_by_state(pLwc->player_state_data.state, &pLwc->player_action_loop);
	/*if (pLwc->player_attacking) {
		player_anim = LWAC_HUMANACTION_ATTACK;
		pLwc->player_action_loop = 0;
	} else if (pLwc->player_aiming) {
		player_anim = LWAC_HUMANACTION_STAND_AIM;
		pLwc->player_action_loop = 0;
	} else if (pLwc->player_moving) {
		player_anim = LWAC_HUMANACTION_WALKPOLISH;
		pLwc->player_action_loop = 1;
	} else {
		player_anim = LWAC_HUMANACTION_IDLE;
		pLwc->player_action_loop = 1;
	}*/

	pLwc->player_action = &pLwc->action[player_anim_1];
	if (pLwc->player_attacking && pLwc->player_action && player_action_animfin) {
		//pLwc->player_attacking = 0;
	}
	// Update skin time
	pLwc->player_skin_time += (float)pLwc->delta_time;
	pLwc->test_player_skin_time += (float)pLwc->delta_time;
	// Read pathfinding result and set the test player's position
	if (pLwc->field->path_query.n_smooth_path) {

		pLwc->field->path_query_time += (float)pLwc->delta_time;
		const float move_speed = 30.0f;
		int idx = (int)fmodf((float)(pLwc->field->path_query_time * move_speed), (float)pLwc->field->path_query.n_smooth_path);
		const float* p = &pLwc->field->path_query.smooth_path[3 * idx];
		// path query result's coordinates is different from world coordinates.
		const vec3 pvec = { p[0], -p[2], p[1] };
		memcpy(pLwc->field->path_query_test_player_pos, pvec, sizeof(vec3));
		
		if (idx < pLwc->field->path_query.n_smooth_path - 1) {
			const float* p2 = &pLwc->field->path_query.smooth_path[3 * (idx + 1)];
			// path query result's coordinates is different from world coordinates.
			const vec3 p2vec = { p2[0], -p2[2], p2[1] };
			pLwc->field->path_query_test_player_rot = atan2f(p2vec[1] - pvec[1], p2vec[0] - pvec[0]);
		}

		// query other random path
		if (idx >= pLwc->field->path_query.n_smooth_path - 1) {
			set_random_start_end_pos(pLwc->field->nav, &pLwc->field->path_query);
			nav_query(pLwc->field->nav, &pLwc->field->path_query);
			pLwc->field->path_query_time = 0;
		}
	}
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
			LW_ACTION remote_player_anim;
			if (value->attacking) {
				remote_player_anim = LWAC_HUMANACTION_ATTACK;
			} else if (value->moving) {
				remote_player_anim = LWAC_HUMANACTION_WALKPOLISH;
			} else {
				remote_player_anim = LWAC_HUMANACTION_IDLE;
			}
			value->action = &pLwc->action[remote_player_anim];
		}
		value = mq_possync_next(pLwc->mq);
	}
}

void unload_field(LWFIELD* field) 	{
	if (field->nav) {
		unload_nav(field->nav);
	}

	release_binary(field->d);

	dGeomDestroy(field->ground);
	dGeomDestroy(field->player_geom);
	for (int i = 0; i < LRI_COUNT; i++) {
		dGeomDestroy(field->ray[i]);
	}
	/*dGeomDestroy(field->player_center_ray);
	dGeomDestroy(field->player_contact_ray);*/
	for (int i = 0; i < field->box_geom_count; i++) {
		dGeomDestroy(field->box_geom[i]);
	}
	dSpaceDestroy(field->space);
	dWorldDestroy(field->world);

	dCloseODE();

	memset(field, 0, sizeof(LWFIELD));
}

void field_attack(LWCONTEXT* pLwc) {
	if (!pLwc->player_attacking) {
		// start attack anim
		pLwc->player_attacking = 1;
		pLwc->player_skin_time = 0;
	}
}

void field_path_query_spos(const LWFIELD* field, float* p) {
	p[0] = field->path_query.spos[0];
	p[1] = -field->path_query.spos[2];
	p[2] = field->path_query.spos[1];
}

void field_path_query_epos(const LWFIELD* field, float* p) {
	p[0] = field->path_query.epos[0];
	p[1] = -field->path_query.epos[2];
	p[2] = field->path_query.epos[1];
}

void field_set_path_query_spos(LWFIELD* field, float x, float y, float z) {
	field->path_query.spos[0] = x;
	field->path_query.spos[1] = z;
	field->path_query.spos[2] = -y;
}

void field_set_path_query_epos(LWFIELD* field, float x, float y, float z) {
	field->path_query.epos[0] = x;
	field->path_query.epos[1] = z;
	field->path_query.epos[2] = -y;
}

int field_path_query_n_smooth_path(const LWFIELD* field) {
	return field->path_query.n_smooth_path;
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
	return field->follow_cam;
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
	nav_query(field->nav, &field->path_query);
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
	pLwc->field->follow_cam = follow_cam;

	set_random_start_end_pos(pLwc->field->nav, &pLwc->field->path_query);
	nav_query(pLwc->field->nav, &pLwc->field->path_query);
}
