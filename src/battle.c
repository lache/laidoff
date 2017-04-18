#include "lwcontext.h"
#include "lwmacro.h"
#include "lwbattlecommand.h"
#include "lwbattlecommandresult.h"
#include "battlelogic.h"

int spawn_attack_trail(LWCONTEXT *pLwc, float x, float y, float z) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (!pLwc->trail[i].valid) {
			pLwc->trail[i].x = x;
			pLwc->trail[i].y = y;
			pLwc->trail[i].z = z;
			pLwc->trail[i].age = 0;
			pLwc->trail[i].max_age = 1;
			pLwc->trail[i].tex_coord_speed = 5;
			pLwc->trail[i].tex_coord = -1;
			pLwc->trail[i].valid = 1;
			return i;
		}
	}

	return -1;
}

int update_next_player_turn_creature(LWCONTEXT* pLwc) {

	if (pLwc->player_turn_creature_index >= 0) {
		pLwc->player[pLwc->player_turn_creature_index].selected = 0;
		pLwc->player[pLwc->player_turn_creature_index].turn_consumed = 1;
		const int next_turn_token = pLwc->player[pLwc->player_turn_creature_index].turn_token + 1;

		for (int i = 0; i < MAX_BATTLE_CREATURE; i++) {
			if (pLwc->player[i].turn_token == next_turn_token) {
				pLwc->player[i].selected = 1;
				pLwc->player_turn_creature_index = i;
				return pLwc->player_turn_creature_index;
			}
		}

		pLwc->player_turn_creature_index = -1;
	}

	return pLwc->player_turn_creature_index;
}

void revert_battle_cam_and_update_player_turn(LWCONTEXT* pLwc) {
	pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
	pLwc->battle_cam_center_x = 0;

	int next_player_turn_creature_index = update_next_player_turn_creature(pLwc);

	pLwc->selected_command_slot = 0;

	if (next_player_turn_creature_index < 0) {
		// Player turn finished.
		pLwc->enemy_turn_command_wait_time = 2.0f;
		pLwc->battle_state = LBS_START_ENEMY_TURN;
	} else {
		// Turn goes to the Player's next creature.
		pLwc->battle_state = LBS_SELECT_COMMAND;
	}
}

void update_attack_trail(LWCONTEXT *pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->trail[i].valid) {
			pLwc->trail[i].age += (float)pLwc->delta_time;
			pLwc->trail[i].tex_coord += (float)(pLwc->trail[i].tex_coord_speed * pLwc->delta_time);

			if (pLwc->trail[i].age >= pLwc->trail[i].max_age) {

				pLwc->trail[i].valid = 0;

				revert_battle_cam_and_update_player_turn(pLwc);
			}
		}
	}
}

int spawn_damage_text(LWCONTEXT *pLwc, float x, float y, float z, const char *text) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (!pLwc->damage_text[i].valid) {
			LWDAMAGETEXT *dt = &pLwc->damage_text[i];

			dt->x = x;
			dt->y = y;
			dt->z = z;
			dt->age = 0;
			dt->max_age = 1;

			dt->valid = 1;
			strncpy(dt->text, text, ARRAY_SIZE(dt->text));
			dt->text[ARRAY_SIZE(dt->text) - 1] = '\0';

			LWTEXTBLOCK *tb = &dt->text_block;

			tb->text = dt->text;
			tb->text_bytelen = (int)strlen(tb->text);
			tb->begin_index = 0;
			tb->end_index = tb->text_bytelen;

			tb->text_block_x = x;
			tb->text_block_y = y;
			tb->text_block_width = 1;
			tb->text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT;
			tb->size = DEFAULT_TEXT_BLOCK_SIZE;

			SET_COLOR_RGBA_FLOAT(tb->color_normal_glyph, 1, 0.25f, 0.25f, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_normal_outline, 0.1f, 0.1f, 0.1f, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_glyph, 1, 1, 0, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_outline, 0, 0, 0, 1);

			return i;
		}
	}

	return -1;
}

void update_damage_text(LWCONTEXT *pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->damage_text[i].valid) {
			pLwc->damage_text[i].age += (float)pLwc->delta_time;

			//pLwc->damage_text[i].text_block.text_block_x += (float)(0.2 * cos(LWDEG2RAD(60)) * pLwc->delta_time);
			pLwc->damage_text[i].text_block.text_block_y += (float)(0.2 * sin(LWDEG2RAD(60)) *
				pLwc->delta_time);

			const float t = pLwc->damage_text[i].age;
			const float expand_time = 0.15f;
			const float retain_time = 0.5f;
			const float contract_time = 0.15f;
			const float max_size = 1.0f;
			if (t < expand_time) {
				pLwc->damage_text[i].text_block.size = max_size / expand_time * t;
			} else if (expand_time <= t && t < expand_time + retain_time) {
				pLwc->damage_text[i].text_block.size = max_size;
			} else if (t < expand_time + retain_time + contract_time) {
				pLwc->damage_text[i].text_block.size = -(max_size / contract_time) * (t -
					(expand_time +
						retain_time +
						contract_time));
			} else {
				pLwc->damage_text[i].text_block.size = 0;
			}

			if (pLwc->damage_text[i].age >= pLwc->damage_text[i].max_age) {
				pLwc->damage_text[i].valid = 0;
			}
		}
	}
}

void reset_enemy_turn(struct _LWCONTEXT* pLwc) {
	int turn_token_counter = 0;
	ARRAY_ITERATE_VALID(LWENEMY, pLwc->enemy) {
		if (e->c.hp > 0) {
			e->c.turn_consumed = 0;
			e->c.turn_token = ++turn_token_counter;
		}
	}
	ARRAY_ITERATE_VALID_END();
}

void setup_enemy_turn(struct _LWCONTEXT* pLwc) {
	reset_enemy_turn(pLwc);
}

void update_enemy_turn(struct _LWCONTEXT* pLwc) {
	if (pLwc->battle_state == LBS_START_ENEMY_TURN) {

		setup_enemy_turn(pLwc);

		pLwc->battle_state = LBS_ENEMY_TURN_IN_PROGRESS;
	}

	if (pLwc->enemy_turn_command_wait_time > 0) {
		pLwc->enemy_turn_command_wait_time -= (float)pLwc->delta_time;
	} else {
		pLwc->enemy_turn_command_wait_time = 0;
		
	}
}

int exec_attack(LWCONTEXT *pLwc, int enemy_slot) {
	if (pLwc->battle_state == LBS_SELECT_TARGET && pLwc->player_turn_creature_index >= 0) {

		LWBATTLECREATURE *ca = &pLwc->player[pLwc->player_turn_creature_index];
		const LWSKILL *s = ca->skill[pLwc->selected_command_slot];
		if (s && s->valid) {
			if (s->consume_hp > ca->hp) {
				return -1;
			}

			if (s->consume_mp > ca->mp) {
				return -2;
			}

			ca->hp -= s->consume_hp;
			ca->mp -= s->consume_mp;
		} else {
			return -3;
		}

		pLwc->battle_state = LBS_COMMAND_IN_PROGRESS;
		pLwc->command_in_progress_anim.t = pLwc->command_in_progress_anim.max_t = 1;
		pLwc->command_in_progress_anim.max_v = 1;


		LWENEMY* enemy = &pLwc->enemy[enemy_slot];
		LWBATTLECREATURE* cb = &enemy->c;

		LWBATTLECOMMAND cmd;
		cmd.skill = s;

		LWBATTLECOMMANDRESULT cmd_result_a;
		LWBATTLECOMMANDRESULT cmd_result_b;

		calculate_battle_command_result(ca, cb, &cmd, &cmd_result_a, &cmd_result_b);

		apply_battle_command_result(ca, &cmd_result_a);
		apply_battle_command_result(cb, &cmd_result_b);

		if (cb->hp <= 0) {
			enemy->death_anim.v0[4] = 1; // Phase 0 (last): alpha remove max
			enemy->death_anim.v1[0] = 1; enemy->death_anim.v1[3] = 1; // Phase 1 (middle): full red
			enemy->death_anim.v2[3] = 1; // Phase 2 (start): full black
			enemy->death_anim.anim_1d.t = enemy->death_anim.anim_1d.max_t = 0.45f;
		}

		const float enemy_x = get_battle_enemy_x_center(enemy_slot);

		char damage_str[128];

		if (cmd_result_a.type == LBCR_MISSED) {
			snprintf(damage_str, ARRAY_SIZE(damage_str), "MISSED");

			enemy->evasion_anim.t = enemy->evasion_anim.max_t = 0.25f;
			enemy->evasion_anim.max_v = 0.15f;
		} else {
			// 데미지는 음수이기 때문에 - 붙여서 양수로 바꿔줌
			snprintf(damage_str, ARRAY_SIZE(damage_str), "%d", -cmd_result_b.delta_hp);

			enemy->shake_duration = 0.15f;
			enemy->shake_magitude = 0.03f;
		}

		// TODO: MISSED 일 때 트레일을 그리지 않으면 전투가 도중에 멈추는 문제가 있어서 무조건 그려줌
		spawn_attack_trail(pLwc, enemy_x, -0.1f, 0.5f);

		spawn_damage_text(pLwc, 0, 0, 0, damage_str);

		pLwc->battle_fov_deg = pLwc->battle_fov_mag_deg_0;

		pLwc->battle_cam_center_x = enemy_x;

		return 0;
	}

	return -4;
}

void exec_attack_with_screen_point(LWCONTEXT* pLwc, float x, float y) {

	for (int i = 0; i < MAX_ENEMY_SLOT; i++) {
		if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
			if (pLwc->enemy[i].left_top_ui_point[0] <= x
				&& x <= pLwc->enemy[i].right_bottom_ui_point[0]
				&& y <= pLwc->enemy[i].left_top_ui_point[1]
				&& pLwc->enemy[i].right_bottom_ui_point[1] <= y) {

				pLwc->battle_state = LBS_SELECT_TARGET;
				pLwc->selected_enemy_slot = i;
				exec_attack(pLwc, i);
			}
		}
	}
}

void update_battle(struct _LWCONTEXT* pLwc) {

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4_perspective(pLwc->battle_proj, (float)(LWDEG2RAD(pLwc->battle_fov_deg) / screen_aspect_ratio), screen_aspect_ratio, 0.1f, 1000.0f);

	vec3 eye = { 0, -5.0f, 0.9f };
	vec3 center = { pLwc->battle_cam_center_x, 0, 0.3f };
	vec3 up = { 0, 0, 1 };
	mat4x4_look_at(pLwc->battle_view, eye, center, up);

	pLwc->command_in_progress_anim.t = (float)LWMAX(0, pLwc->command_in_progress_anim.t - (float)pLwc->delta_time);

	if (pLwc->battle_state == LBS_ENEMY_TURN_IN_PROGRESS) {
		update_enemy_turn(pLwc);
	}
}
