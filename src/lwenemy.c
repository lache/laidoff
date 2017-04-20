#include <stdlib.h> // for android rand(), RAND_MAX
#include "lwenemy.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "lwsimpleanim.h"

float get_battle_enemy_x_center(int enemy_slot_index)
{
	return -1.6f + 0.8f * enemy_slot_index;
}

void update_render_enemy_position(const struct _LWCONTEXT* pLwc, int enemy_slot_index, const LWENEMY* enemy, vec3 pos) {
	
	// base position
	pos[0] = get_battle_enemy_x_center(enemy_slot_index);
	pos[1] = 0;
	pos[2] = 0;

	const float shake_magnitude = enemy->c.shake_duration > 0 ? enemy->c.shake_magitude : 0;

	// offset by shake anim
	if (shake_magnitude) {
		pos[0] += (float)rand() / RAND_MAX * shake_magnitude;
		pos[1] += (float)rand() / RAND_MAX * shake_magnitude;
	}

	// offset by evasion anim
	pos[0] += lwanim_get_1d(&enemy->evasion_anim);

	// offset by z(height) oscillation
	const float T = (float)M_PI;
	const float osc = (float)sin((2 * M_PI) * (pLwc->app_time + enemy->time_offset) / T);
	pos[2] += 0.015f * osc;
}

void calculate_ui_point_from_world_point(
	const float aspect_ratio, const mat4x4 proj_view, const vec4 world_point, vec2 ui_point) {

	vec4 clip_space_point;
	mat4x4_mul_vec4(clip_space_point, proj_view, world_point);

	const vec3 ndc_pos = {
		clip_space_point[0] / clip_space_point[3],
		clip_space_point[1] / clip_space_point[3],
		clip_space_point[2] / clip_space_point[3]
	};

	ui_point[0] = ndc_pos[0] * aspect_ratio;
	ui_point[1] = ndc_pos[1];
}

void update_enemy_scope_ui_point(const LWCONTEXT* pLwc, LWENEMY* enemy) {

	mat4x4 proj_view;
	mat4x4_identity(proj_view);
	mat4x4_mul(proj_view, pLwc->battle_proj, pLwc->battle_view);

	const float sprite_aspect_ratio = (float)SPRITE_DATA[0][enemy->las].w / SPRITE_DATA[0][enemy->las].h;

	const float creature_half_width = enemy->scale * sprite_aspect_ratio;
	const float creature_height = 2 * enemy->scale;
	const float enemy_scope_x_half_margin = 0.15f;
	const float enemy_scope_z_top_margin = 0.15f;
	const float enemy_scope_z_bottom_margin = 0.10f;

	const vec4 left_top_world_point = {
		enemy->render_pos[0] - (creature_half_width + enemy_scope_x_half_margin),
		enemy->render_pos[1],
		enemy->render_pos[2] + creature_height + enemy_scope_z_top_margin,
		1
	};

	const vec4 right_bottom_world_point = {
		enemy->render_pos[0] + (creature_half_width + enemy_scope_x_half_margin),
		enemy->render_pos[1],
		enemy->render_pos[2] - enemy_scope_z_bottom_margin,
		1
	};

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	calculate_ui_point_from_world_point(aspect_ratio, proj_view, left_top_world_point, enemy->left_top_ui_point);
	calculate_ui_point_from_world_point(aspect_ratio, proj_view, right_bottom_world_point, enemy->right_bottom_ui_point);

	float enemy_scope_ui_width = enemy->right_bottom_ui_point[0] - enemy->left_top_ui_point[0];
	float enemy_scope_ui_height = enemy->left_top_ui_point[1] - enemy->right_bottom_ui_point[1];

	if (enemy_scope_ui_width < 0.4f) {
		const float d = 0.4f - enemy_scope_ui_width;
		enemy->left_top_ui_point[0] -= d / 2.0f;
	}

	if (enemy_scope_ui_height < 0.4f) {
		const float d = 0.4f - enemy_scope_ui_height;
		enemy->left_top_ui_point[1] += d;
	}
}

void update_enemy(const struct _LWCONTEXT* pLwc, int enemy_slot_index, LWENEMY* enemy) {
	enemy->c.shake_duration = (float)LWMAX(0, enemy->c.shake_duration - (float)pLwc->delta_time);
	enemy->evasion_anim.t = (float)LWMAX(0, enemy->evasion_anim.t - (float)pLwc->delta_time);
	enemy->death_anim.anim_1d.t = (float)LWMAX(0, enemy->death_anim.anim_1d.t - (float)pLwc->delta_time);

	update_render_enemy_position(pLwc, enemy_slot_index, enemy, enemy->render_pos);
	update_enemy_scope_ui_point(pLwc, enemy);
}
