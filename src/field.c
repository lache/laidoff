#include "field.h"
#include "lwanim.h"
#include "laidoff.h"
#include "input.h"

void move_player(LWCONTEXT *pLwc) {
	const float move_speed = 5.0f;
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

static void resolve_collision_one_fixed(const LWBOX2DCOLLIDER *fixed, LWBOX2DCOLLIDER *movable) {
	const float dx = resolve_collision_one_fixed_axis(fixed->x, fixed->w, movable->x, movable->w);

	if (dx) {
		const float dy = resolve_collision_one_fixed_axis(fixed->y, fixed->h, movable->y,
			movable->h);

		if (dy) {
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
			resolve_collision_one_fixed(&pLwc->box_collider[i], &player_collider);
		}
	}

	pLwc->player_pos_x = player_collider.x;
	pLwc->player_pos_y = player_collider.y;
}
