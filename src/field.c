#include <stdlib.h>
#include "field.h"
#include "lwanim.h"
#include "laidoff.h"
#include "input.h"
#include "lwlog.h"

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
		float dx, dy, dlen;
		if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
			pLwc->player_pos_x += dx * move_speed_delta;
			pLwc->player_pos_y += dy * move_speed_delta;
			pLwc->player_rot_z = atan2f(dy, dx);
			pLwc->player_moving = 1;
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

LWFIELD* load_field() {
	
	dInitODE2(0);

	LWFIELD* field = (LWFIELD*)calloc(1, sizeof(LWFIELD));
	field->player_radius = (dReal)1.0;
	field->player_length = (dReal)3.0;
	field->world = dWorldCreate();
	field->space = dHashSpaceCreate(0);
	field->ground = dCreatePlane(field->space, 0, 0, 1, 0);

	// Player geom
	field->geom_pos[0] = 0;
	field->geom_pos[1] = 0;
	field->geom_pos[2] = 10;
	field->player_geom = dCreateCapsule(field->space, field->player_radius, field->player_length);
	dGeomSetPosition(field->player_geom, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);

	const dReal ray_length = 50;
	dMatrix3 R;

	// Player center ray
	field->player_center_ray = dCreateRay(field->space, ray_length);
	dGeomSetPosition(field->player_center_ray, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	dGeomSetRotation(field->player_center_ray, R);

	// Player contact ray
	field->player_contact_ray = dCreateRay(field->space, ray_length);
	dGeomSetPosition(field->player_contact_ray, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	dRFromAxisAndAngle(R, 1, 0, 0, M_PI); // ray direction: downward (-Z)
	dGeomSetRotation(field->player_contact_ray, R);

	field->ground_normal[0] = 0;
	field->ground_normal[1] = 0;
	field->ground_normal[2] = 1;

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

					field->geom_pos[0] += sign * n[0] * d;
					field->geom_pos[1] += sign * n[1] * d;
				}
			}
		}
	}
}

void reset_ray_result(LWFIELD* field) {
	field->center_ray_result_count = 0;
	field->contact_ray_result_count = 0;
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
	dMultiply0_331(geom_pos_delta_rotated, qmoveR, field->geom_pos_delta);

	for (int i = 0; i < 3; i++) {
		field->geom_pos[i] += geom_pos_delta_rotated[i];
		field->geom_pos_delta[i] = 0;
	}
	dGeomSetPosition(field->player_geom, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	dGeomSetPosition(field->player_center_ray, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
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

		LOGI("min ray length:%.3f / g pos:%.2f,%.2f,%.2f / g nor:%.2f,%.2f,%.2f",
			field->center_ray_result[min_ray_index].geom.depth,
			field->center_ray_result[min_ray_index].geom.pos[0],
			field->center_ray_result[min_ray_index].geom.pos[1],
			field->center_ray_result[min_ray_index].geom.pos[2],
			field->center_ray_result[min_ray_index].geom.normal[0],
			field->center_ray_result[min_ray_index].geom.normal[1],
			field->center_ray_result[min_ray_index].geom.normal[2]);

		field->ground_normal[0] = field->center_ray_result[min_ray_index].geom.normal[0];
		field->ground_normal[1] = field->center_ray_result[min_ray_index].geom.normal[1];
		field->ground_normal[2] = field->center_ray_result[min_ray_index].geom.normal[2];

		dReal tz = field->center_ray_result[min_ray_index].geom.pos[2] + field->player_radius / field->center_ray_result[min_ray_index].geom.normal[2] + field->player_length / 2;

		dReal d1 = tz - field->geom_pos[2];

		dReal d2 = FLT_MIN;
		if (min_side_ray_index >= 0) {
			dReal d = min_side_ray_length - field->player_length / 2 - field->player_radius;
			d2 = -d;
		}

		if (d1 > d2) {
			field->geom_pos[2] += d2;
		} else {
			field->geom_pos[2] += d1;
		}
	}

	dGeomSetPosition(field->player_geom, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	dGeomSetPosition(field->player_center_ray, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	if (min_ray_index >= 0) {
		dGeomSetPosition(field->player_contact_ray,
			field->geom_pos[0] - field->player_radius * field->center_ray_result[min_ray_index].geom.normal[0],
			field->geom_pos[1] - field->player_radius * field->center_ray_result[min_ray_index].geom.normal[1],
			field->geom_pos[2]);

	} else {
		dGeomSetPosition(field->player_contact_ray, field->geom_pos[0], field->geom_pos[1], field->geom_pos[2]);
	}
}

void update_field(LWFIELD* field) {
	if (!field) {
		return;
	}

	reset_ray_result(field);

	move_player_geom_by_input(field);
	
	dSpaceCollide(field->space, field, &field_near_callback);
	
	//dWorldStep(field->world, 0.05);

	move_player_to_ground(field);
}
