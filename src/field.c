#include <stdlib.h>
#include "field.h"
#include "lwanim.h"
#include "laidoff.h"
#include "input.h"
#include "lwlog.h"
#include "file.h"
#include "nav.h"
#include "mq.h"
#include "extrapolator.h"
#include <czmq.h>

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
		if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
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

	const dReal ray_length = 50;
	dMatrix3 R;

	// Player center ray
	field->player_center_ray = dCreateRay(field->space, ray_length);
	dGeomSetPosition(field->player_center_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	dGeomSetRotation(field->player_center_ray, R);

	// Player contact ray
	field->player_contact_ray = dCreateRay(field->space, ray_length);
	dGeomSetPosition(field->player_contact_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	dGeomSetRotation(field->player_contact_ray, R);

	field->ground_normal[0] = 0;
	field->ground_normal[1] = 0;
	field->ground_normal[2] = 1;

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

		int i, n;

		// only collide things with the ground
		int g1 = (o1 == field->player_center_ray || o1 == field->player_contact_ray || o1 == field->player_geom);
		int g2 = (o2 == field->player_center_ray || o2 == field->player_contact_ray || o2 == field->player_geom);
		if (!(g1 ^ g2)) return;

		dContact contact[MAX_FIELD_CONTACT];
		n = dCollide(o1, o2, MAX_FIELD_CONTACT, &contact[0].geom, sizeof(dContact));
		if (n > 0) {
			for (i = 0; i < n; i++) {

				if (contact[i].geom.g1 == field->player_center_ray || contact[i].geom.g2 == field->player_center_ray) {
					field->center_ray_result[field->center_ray_result_count] = contact[i];

					if (contact[i].geom.g2 == field->player_center_ray) {
						field->center_ray_result[field->center_ray_result_count].geom.normal[0] = -field->center_ray_result[field->center_ray_result_count].geom.normal[0];
						field->center_ray_result[field->center_ray_result_count].geom.normal[1] = -field->center_ray_result[field->center_ray_result_count].geom.normal[1];
						field->center_ray_result[field->center_ray_result_count].geom.normal[2] = -field->center_ray_result[field->center_ray_result_count].geom.normal[2];
					}

					field->center_ray_result_count++;
				}
				if (contact[i].geom.g1 == field->player_contact_ray || contact[i].geom.g2 == field->player_contact_ray) {
					field->contact_ray_result[field->contact_ray_result_count] = contact[i];

					if (contact[i].geom.g2 == field->player_contact_ray) {
						field->contact_ray_result[field->contact_ray_result_count].geom.normal[0] = -field->contact_ray_result[field->contact_ray_result_count].geom.normal[0];
						field->contact_ray_result[field->contact_ray_result_count].geom.normal[1] = -field->contact_ray_result[field->contact_ray_result_count].geom.normal[1];
						field->contact_ray_result[field->contact_ray_result_count].geom.normal[2] = -field->contact_ray_result[field->contact_ray_result_count].geom.normal[2];
					}

					field->contact_ray_result_count++;
				}

				dReal sign = 0;
				if (contact[i].geom.g1 == field->player_geom) {
					sign = 1;
				} else if (contact[i].geom.g2 == field->player_geom) {
					sign = -1;
				}

				if (sign) {

					const dReal * p = dGeomGetPosition(field->player_geom);

					dReal* n = contact[i].geom.normal;
					dReal d = contact[i].geom.depth;

					field->player_pos[0] += sign * n[0] * d;
					field->player_pos[1] += sign * n[1] * d;
				}
			}
		}
	}
}

void reset_ray_result(LWFIELD* field) {
	field->center_ray_result_count = 0;
	field->contact_ray_result_count = 0;
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

void move_player_geom_by_input(LWFIELD* field) {
	dVector3 u_x_n, up;

	up[0] = 0;
	up[1] = 0;
	up[2] = 1;

	dCalcVectorCross3(u_x_n, up, field->ground_normal);
	dReal dot = dCalcVectorDot3(up, field->ground_normal);

	dQuaternion qmove;
	qmove[0] = 1 + dot;
	qmove[1] = u_x_n[0];
	qmove[2] = u_x_n[1];
	qmove[3] = u_x_n[2];
	dNormalize4(qmove);
	dMatrix3 qmoveR;
	dQtoR(qmove, qmoveR);

	dVector3 geom_pos_delta_rotated;
	dMultiply0_331(geom_pos_delta_rotated, qmoveR, field->player_pos_delta);

	for (int i = 0; i < 3; i++) {
		field->player_pos[i] += geom_pos_delta_rotated[i];
		field->player_pos_delta[i] = 0;
	}
	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dGeomSetPosition(field->player_center_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	const dReal* player_contact_ray_pos = dGeomGetPosition(field->player_contact_ray);
	dGeomSetPosition(field->player_contact_ray,
		player_contact_ray_pos[0] + geom_pos_delta_rotated[0],
		player_contact_ray_pos[1] + geom_pos_delta_rotated[1],
		player_contact_ray_pos[2] + geom_pos_delta_rotated[2]);
}

void move_player_to_ground(LWFIELD* field) {
	dReal min_ray_length = FLT_MAX;
	int min_ray_index = -1;
	for (int i = 0; i < field->center_ray_result_count; i++) {
		if (min_ray_length > field->center_ray_result[i].geom.depth) {
			min_ray_length = field->center_ray_result[i].geom.depth;
			min_ray_index = i;
		}
	}

	dReal min_side_ray_length = FLT_MAX;
	int min_side_ray_index = -1;
	for (int i = 0; i < field->contact_ray_result_count; i++) {
		if (min_side_ray_length > field->contact_ray_result[i].geom.depth) {
			min_side_ray_length = field->contact_ray_result[i].geom.depth;
			min_side_ray_index = i;
		}
	}

	if (min_ray_index >= 0) {

		/*LOGI("min ray length:%.3f / g pos:%.2f,%.2f,%.2f / g nor:%.2f,%.2f,%.2f",
			field->center_ray_result[min_ray_index].geom.depth,
			field->center_ray_result[min_ray_index].geom.pos[0],
			field->center_ray_result[min_ray_index].geom.pos[1],
			field->center_ray_result[min_ray_index].geom.pos[2],
			field->center_ray_result[min_ray_index].geom.normal[0],
			field->center_ray_result[min_ray_index].geom.normal[1],
			field->center_ray_result[min_ray_index].geom.normal[2]);*/

		field->ground_normal[0] = field->center_ray_result[min_ray_index].geom.normal[0];
		field->ground_normal[1] = field->center_ray_result[min_ray_index].geom.normal[1];
		field->ground_normal[2] = field->center_ray_result[min_ray_index].geom.normal[2];

		dReal tz = field->center_ray_result[min_ray_index].geom.pos[2] + field->player_radius / field->center_ray_result[min_ray_index].geom.normal[2] + field->player_length / 2;

		dReal d1 = tz - field->player_pos[2];

		dReal d2 = FLT_MIN;
		if (min_side_ray_index >= 0) {
			dReal d = min_side_ray_length - field->player_length / 2 - field->player_radius;
			d2 = -d;
		}

		if (d1 > d2) {
			field->player_pos[2] += d2;
		} else {
			field->player_pos[2] += d1;
		}
	}

	dGeomSetPosition(field->player_geom, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	dGeomSetPosition(field->player_center_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
	if (min_ray_index >= 0) {
		dGeomSetPosition(field->player_contact_ray,
			field->player_pos[0] - field->player_radius * field->center_ray_result[min_ray_index].geom.normal[0],
			field->player_pos[1] - field->player_radius * field->center_ray_result[min_ray_index].geom.normal[1],
			field->player_pos[2]);

	} else {
		dGeomSetPosition(field->player_contact_ray, field->player_pos[0], field->player_pos[1], field->player_pos[2]);
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
	
	dSpaceCollide(field->space, field, &field_near_callback);
	
	//dWorldStep(field->world, 0.05);

	move_player_to_ground(field);

	dSubtractVectors3(field->player_vel, field->player_pos, player_pos_0);
	dScaleVector3(field->player_vel, (dReal)1.0 / pLwc->delta_time);

	//LOGI("%f, %f, %f", field->player_vel[0], field->player_vel[1], field->player_vel[2]);
	
	LW_ACTION player_anim;
	if (pLwc->player_attacking) {
		player_anim = LWAC_HUMANACTION_ATTACK;
	} else if (pLwc->player_moving) {
		player_anim = LWAC_HUMANACTION_WALKPOLISH;
	} else {
		player_anim = LWAC_HUMANACTION_IDLE;
	}

	pLwc->player_action = &pLwc->action[player_anim];

	float f = (float)(pLwc->player_skin_time * pLwc->player_action->fps);
	if (pLwc->player_attacking && pLwc->player_action && f > pLwc->player_action->last_key_f) {
		pLwc->player_attacking = 0;
	}
	// Update skin time
	pLwc->player_skin_time += pLwc->delta_time;
	pLwc->test_player_skin_time += pLwc->delta_time;
	// Read pathfinding result and set the test player's position
	if (pLwc->field->path_query.n_smooth_path) {

		pLwc->field->path_query_time += (float)pLwc->delta_time;

		int idx = (int)fmodf((float)(pLwc->field->path_query_time * 30), (float)pLwc->field->path_query.n_smooth_path);
		const float* p = &pLwc->field->path_query.smooth_path[3 * idx];
		pLwc->field->path_query_test_player_pos[0] = p[0];
		pLwc->field->path_query_test_player_pos[1] = -p[2];
		pLwc->field->path_query_test_player_pos[2] = p[1];

		if (idx < pLwc->field->path_query.n_smooth_path - 1) {
			const float* p2 = &pLwc->field->path_query.smooth_path[3 * (idx + 1)];
			pLwc->field->path_query_test_player_rot = atan2f((-p2[2]) - (-p[2]), (p2[0]) - (p[0]));
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
			vec4_extrapolator_read(value->extrapolator, mq_sync_time(pLwc->mq), &value->x, &value->y, &value->z, &dx, &dy);
			
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
	release_binary(field->d);

	dGeomDestroy(field->ground);
	dGeomDestroy(field->player_geom);
	dGeomDestroy(field->player_center_ray);
	dGeomDestroy(field->player_contact_ray);
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
