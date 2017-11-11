#include "lwcontext.h"
#include "render_physics.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"
#include "puckgame.h"
#include "render_field.h"
#include "lwtextblock.h"
#include "render_text_block.h"
#include "lwudp.h"

typedef struct _LWSPHERERENDERUNIFORM {
	float sphere_col_ratio[3];
	float sphere_pos[3][3];
	float sphere_col[3][3];
	float sphere_speed[3];
	float sphere_move_rad[3];
} LWSPHERERENDERUNIFORM;

static void render_go(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAMEOBJECT* go, int tex_index, float render_scale, const float* remote_pos, const mat4x4 remote_rot, int remote, float speed) {
	int shader_index = LWST_DEFAULT;
	const LWSHADER* shader = &pLwc->shader[shader_index];
	glUseProgram(shader->program);
	glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
	glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
	glUniform1f(shader->alpha_multiplier_location, 1.0f);
	glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(shader->overlay_color_location, 1, 1, 1);
	glUniform1f(shader->overlay_color_ratio_location, 0);

	mat4x4 rot;
	mat4x4_identity(rot);
	float sx = render_scale * go->radius, sy = render_scale * go->radius, sz = render_scale * go->radius;
	float x = render_scale * (remote_pos ? remote_pos[0] : go->pos[0]);
	float y = render_scale * (remote_pos ? remote_pos[1] : go->pos[1]);
	float z = render_scale * (remote_pos ? remote_pos[2] : go->pos[2]);

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_mul(model, model, rot);
	mat4x4_scale_aniso(model, model, sx, sy, sz);
	//mat4x4_scale_aniso(model, model, 5, 5, 5);

	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, z);

	mat4x4_mul(model, remote ? remote_rot : go->rot, model);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	const float red_overlay_ratio = go->red_overlay ? LWMIN(1.0f, speed / go->puck_game->puck_damage_contact_speed_threshold) : 0;
	const float red_overlay_logistic_ratio = 1 / (1 + powf(2.718f, -(20.0f * (red_overlay_ratio - 0.8f))));
	glUniform3f(shader->overlay_color_location, 1, 0, 0);
	glUniform1f(shader->overlay_color_ratio_location, red_overlay_logistic_ratio);

	const LW_VBO_TYPE lvt = LVT_PUCK;
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	//set_tex_filter(GL_NEAREST, GL_NEAREST);
	glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_timer(const LWCONTEXT* pLwc, float remain_sec) {
	// Render text
	LWTEXTBLOCK text_block;
	text_block.align = LTBA_CENTER_TOP;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
	text_block.multiline = 1;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	char str[32];
	if (remain_sec < 0) {
		sprintf(str, u8"--");
	} else {
		sprintf(str, u8"%.0f", remain_sec);
	}
	text_block.text = str;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = 0.0f;
	text_block.text_block_y = 1.0f - 0.05f;
	render_text_block(pLwc, &text_block);
}

static void render_match_state(const LWCONTEXT* pLwc) {
	// Render text
	LWTEXTBLOCK text_block;
	text_block.align = LTBA_CENTER_BOTTOM;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
	text_block.multiline = 1;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	char str[128];
	if (pLwc->puck_game_state.finished) {
		int hp_diff = pLwc->puck_game_state.player_current_hp - pLwc->puck_game_state.target_current_hp;
		if (hp_diff == 0) {
			sprintf(str, u8"종료! 무승부..BID:%d (대시버튼 눌러서 다시하기)", pLwc->puck_game->battle_id);
		} else if (hp_diff > 0) {
			if (pLwc->puck_game->battle_id == 0) {
				sprintf(str, u8"[연습중]~~~승리~~~BID:%d (대전상대 찾고 있다)", pLwc->puck_game->battle_id);
			} else {
				sprintf(str, u8"~~~승리~~~BID:%d (대시버튼 눌러서 다시하기)", pLwc->puck_game->battle_id);
			}
		} else {
			if (pLwc->puck_game->battle_id == 0) {
				sprintf(str, u8"[연습중]패배...BID:%d (대전상대 찾고 있다)", pLwc->puck_game->battle_id);
			} else {
				sprintf(str, u8"패배...BID:%d (대시버튼 눌러서 다시하기)", pLwc->puck_game->battle_id);
			}
		}
	} else {
		if (pLwc->puck_game->token) {
			sprintf(str, u8"전투중...BID:%d", pLwc->puck_game->battle_id);
		} else {
			sprintf(str, u8"[연습중] --- (대전 상대를 찾고 있다능...)");
		}
	}

	text_block.text = str;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = 0;
	text_block.text_block_y = -0.9f;
	render_text_block(pLwc, &text_block);
}

static void render_hp_gauge(const LWCONTEXT* pLwc,
							float x, float y, int current_hp, int total_hp, float hp_shake_remain_time, int right, const char* str) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;
	const float gauge_width = 1.2f;
	const float gauge_height = 0.1f;
	const float gauge_flush_height = 0.07f;
	const float base_color = 0.1f;
	// Positioinal offset by shake
	if (hp_shake_remain_time > 0) {
		const float ratio = hp_shake_remain_time / pLwc->puck_game->hp_shake_time;
		const float shake_magnitude = 0.02f;
		x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * aspect_ratio;
		y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude;
	}
	// Render background (gray)
	render_solid_vb_ui(pLwc,
					   x, y, gauge_width, gauge_height,
					   0,
					   LVT_CENTER_TOP_ANCHORED_SQUARE,
					   1, base_color, base_color, base_color, 1);
	const float cell_border = 0.015f;
	if (total_hp > 0) {
		const float cell_width = (gauge_width - cell_border * (total_hp + 1)) / total_hp;
		const float cell_x_0 = x - gauge_width / 2 + cell_border;
		const float cell_x_stride = cell_width + cell_border;
		for (int i = 0; i < total_hp; i++) {
			float r = base_color, g = base_color, b = base_color;
			if (right) {
				if (total_hp - current_hp > i) {
					r = 1;
				} else {
					g = 1;
				}
			} else {
				if (i < current_hp) {
					g = 1;
				} else {
					r = 1;
				}
			}

			// Render background (green)
			render_solid_vb_ui(pLwc,
							   cell_x_0 + cell_x_stride * i, y - gauge_height / 2, cell_width, gauge_height - cell_border * 2,
							   0,
							   LVT_LEFT_CENTER_ANCHORED_SQUARE,
							   1, r, g, b, 1);
		}
	}
	// Render text
	LWTEXTBLOCK text_block;
	text_block.align = LTBA_CENTER_BOTTOM;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
	text_block.multiline = 1;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	text_block.text = str;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = x;
	text_block.text_block_y = y;
	render_text_block(pLwc, &text_block);
}

static void render_dash_gauge(const LWCONTEXT* pLwc) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;
	const float margin_x = 0.3f;
	const float margin_y = 0.2f * 5;
	const float gauge_width = 0.75f;
	const float gauge_height = 0.07f;
	const float gauge_flush_height = 0.07f;
	const float base_color = 0.3f;
	float x = aspect_ratio - margin_x;
	float y = -1 + margin_y;
	const float boost_gauge_ratio = puck_game_dash_gauge_ratio(pLwc->puck_game);
	// Positioinal offset by shake
	if (pLwc->puck_game->dash.shake_remain_time > 0) {
		const float ratio = pLwc->puck_game->dash.shake_remain_time / pLwc->puck_game->dash_shake_time;
		const float shake_magnitude = 0.02f;
		x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * aspect_ratio;
		y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude;
	}
	// Render background (red)
	if (boost_gauge_ratio < 1.0f) {
		render_solid_vb_ui(pLwc,
						   x - gauge_width * boost_gauge_ratio, y, gauge_width * (1.0f - boost_gauge_ratio), gauge_height,
						   0,
						   LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
						   1, 1, base_color, base_color, 1);
	}
	// Render foreground (green)
	render_solid_vb_ui(pLwc,
					   x, y, gauge_width * boost_gauge_ratio, gauge_height,
					   0,
					   LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
					   1, base_color, 1, base_color, 1);
	if (boost_gauge_ratio < 1.0f) {
		// Render flush gauge
		render_solid_vb_ui(pLwc,
						   x, y + boost_gauge_ratio / 4.0f, gauge_width, gauge_height,
						   0,
						   LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
						   powf(1.0f - boost_gauge_ratio, 3.0f), base_color, 1, base_color, 1);
	}
	// Render text
	LWTEXTBLOCK text_block;
	text_block.align = LTBA_RIGHT_BOTTOM;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
	text_block.multiline = 1;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	char str[32];
	sprintf(str, "%.1f%% BOOST GAUGE", boost_gauge_ratio * 100);
	text_block.text = str;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = x;
	text_block.text_block_y = y;
	render_text_block(pLwc, &text_block);
}

static void render_wall(const LWCONTEXT *pLwc, vec4 *proj, const LWPUCKGAME *puck_game,
						int shader_index, vec4 *view, float x, float y, float z,
						float x_rot, float y_rot, LW_VBO_TYPE lvt, float sx, float sy, float sz, const LWSPHERERENDERUNIFORM* sphere_render_uniform) {
	const LWSHADER* shader = &pLwc->shader[shader_index];
	glUseProgram(shader->program);
	glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
	glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
	glUniform1f(shader->alpha_multiplier_location, 1.0f);
	glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(shader->overlay_color_location, 1, 1, 1);
	glUniform1f(shader->overlay_color_ratio_location, 0);
	glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
	glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
	glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
	glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
	glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);

	const int tex_index = 0;
	mat4x4 rot_x;
	mat4x4_identity(rot_x);
	mat4x4_rotate_X(rot_x, rot_x, x_rot);
	mat4x4 rot_y;
	mat4x4_identity(rot_y);
	mat4x4_rotate_Y(rot_y, rot_y, y_rot);
	mat4x4 rot;
	mat4x4_mul(rot, rot_x, rot_y);

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_mul(model, model, rot);
	mat4x4_scale_aniso(model, model, sx, sy, sz);
	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, z);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glUniform3f(shader->overlay_color_location, 0.2f, 0.2f, 0.2f);
	glUniform1f(shader->overlay_color_ratio_location, 1.0f);

	//glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	//set_tex_filter(GL_NEAREST, GL_NEAREST);
	glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_floor(const LWCONTEXT *pLwc, vec4 *proj, const LWPUCKGAME *puck_game, int shader_index, vec4 *view,
						 const LWSPHERERENDERUNIFORM* sphere_render_uniform) {

	const LWSHADER* shader = &pLwc->shader[shader_index];
	glUseProgram(shader->program);
	glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
	const float uv_scale[2] = { 5, 5 };
	glUniform2fv(shader->vuvscale_location, 1, uv_scale);
	glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
	glUniform1f(shader->alpha_multiplier_location, 1.0f);
	glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(shader->overlay_color_location, 1, 1, 1);
	glUniform1f(shader->overlay_color_ratio_location, 0);
	glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
	glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
	glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
	glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
	glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);

	const int tex_index = pLwc->tex_atlas[LAE_PUCK_FLOOR_KTX];
	mat4x4 rot;
	mat4x4_identity(rot);

	float sx = 2.0f;
	float sy = 2.0f;
	float sz = 2.0f;
	float x = 0.0f, y = 0.0f, z = 0.0f;

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_mul(model, model, rot);
	mat4x4_scale_aniso(model, model, sx, sy, sz);
	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, z);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glUniform3f(shader->overlay_color_location, 0.5f, 0.5f, 0.5f);
	glUniform1f(shader->overlay_color_ratio_location, 0.0f);

	//glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

	const LW_VBO_TYPE lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	//set_tex_filter(GL_NEAREST, GL_NEAREST);
	glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

void lwc_render_physics(const LWCONTEXT* pLwc) {
	const LWPUCKGAME* puck_game = pLwc->puck_game;
	const LWPSTATE* state = &pLwc->puck_game_state;
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int shader_index = LWST_DEFAULT;
	int wall_shader_index = LWST_SPHERE_REFLECT;

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4 proj;
	mat4x4_perspective(proj, (float)(LWDEG2RAD(49.134) / screen_aspect_ratio), screen_aspect_ratio, 1.0f, 500.0f);

	mat4x4 view;
	vec3 eye = { 0.0f, 0.0f, 12.0f };

	vec3 center = { 0, 0, 0 };

	vec3 up = { 0, 1, 0 };
	if (puck_game->player_no == 2) {
		up[1] = -1.0f;
	}
	mat4x4_look_at(view, eye, center, up);

	int single_play = pLwc->puck_game->battle_id == 0;
	int remote = !single_play && !pLwc->udp->master;
	const float* player_pos = puck_game->go[LPGO_PLAYER].pos;
	const float* target_pos = puck_game->go[LPGO_TARGET].pos;
	const float* puck_pos = puck_game->go[LPGO_PUCK].pos;
	const float* remote_player_pos = 0;
	const float* remote_puck_pos = 0;
	const float* remote_target_pos = 0;
	if (remote) {
		remote_player_pos = state->player;
		remote_puck_pos = state->puck;
		remote_target_pos = state->target;
	} else {
		remote_player_pos = player_pos;
		remote_puck_pos = puck_pos;
		remote_target_pos = target_pos;
	}

	LWSPHERERENDERUNIFORM sphere_render_uniform = {
		// float sphere_col_ratio[3];
		{ 1.0f, 1.0f, 1.0f },
		// float sphere_pos[3][3];
		{
			{ remote_player_pos[0], remote_player_pos[1], remote_player_pos[2] },
			{ remote_target_pos[0], remote_target_pos[1], remote_target_pos[2] },
			{ remote_puck_pos[0], remote_puck_pos[1], remote_puck_pos[2] }
		},
		// float sphere_col[3][3];
		{
			{ 0.0f, 1.0f, 0.8f },
			{ 1.0f, 0.0f, 0.0f },
			{ 0.2f, 0.3f, 1.0f }
		},
		// float sphere_speed[3];
		{
			!remote ? puck_game->go[LPGO_PLAYER].speed : state->player_speed,
			!remote ? puck_game->go[LPGO_TARGET].speed : state->target_speed,
			!remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed
		},
		// float sphere_move_rad[3];
		{
			!remote ? puck_game->go[LPGO_PLAYER].move_rad : state->player_move_rad,
			!remote ? puck_game->go[LPGO_TARGET].move_rad : state->target_move_rad,
			!remote ? puck_game->go[LPGO_PUCK].move_rad : state->puck_move_rad
		},
	};

	if (puck_game->player_no == 2) {
		sphere_render_uniform.sphere_col[0][0] = 1.0f;
		sphere_render_uniform.sphere_col[0][1] = 0.0f;
		sphere_render_uniform.sphere_col[0][2] = 0.0f;

		sphere_render_uniform.sphere_col[1][0] = 0.0f;
		sphere_render_uniform.sphere_col[1][1] = 1.0f;
		sphere_render_uniform.sphere_col[1][2] = 0.8f;
	}
	const float wall_height = 0.8f;

	const LWPUCKGAMEPLAYER* player = &puck_game->player;
	const LWPUCKGAMEPLAYER* target = &puck_game->target;

	if (single_play || puck_game->battle_id) {
		// Floor
		render_floor(pLwc, proj, puck_game, wall_shader_index, view, &sphere_render_uniform);
		// North wall
		render_wall(pLwc, proj, puck_game, wall_shader_index, view, 0, 2, 0, (float)LWDEG2RAD(90), 0,
					LVT_CENTER_BOTTOM_ANCHORED_SQUARE, 2.0f, wall_height, 2.0f, &sphere_render_uniform);
		// South wall
		render_wall(pLwc, proj, puck_game, wall_shader_index, view, 0, -2, 0, (float)LWDEG2RAD(-90), 0,
					LVT_CENTER_TOP_ANCHORED_SQUARE, 2.0f, wall_height, 2.0f, &sphere_render_uniform);
		// East wall
		render_wall(pLwc, proj, puck_game, wall_shader_index, view, 2, 0, 0, 0, (float)LWDEG2RAD(90),
					LVT_LEFT_CENTER_ANCHORED_SQUARE, wall_height, 2.0f, 2.0f, &sphere_render_uniform);
		// West wall
		render_wall(pLwc, proj, puck_game, wall_shader_index, view, -2, 0, 0, 0, (float)LWDEG2RAD(-90),
					LVT_RIGHT_CENTER_ANCHORED_SQUARE, wall_height, 2.0f, 2.0f, &sphere_render_uniform);

		const int player_no = pLwc->puck_game->player_no;

		render_go(pLwc, view, proj, &puck_game->go[LPGO_PUCK], pLwc->tex_atlas[LAE_PUCK_KTX],
				  1.0f, remote_puck_pos, state->puck_rot, remote, !remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed);
		render_go(pLwc, view, proj, &puck_game->go[LPGO_PLAYER], pLwc->tex_atlas[player_no == 2 ? LAE_PUCK_ENEMY_KTX : LAE_PUCK_PLAYER_KTX],
				  1.0f, remote_player_pos, state->player_rot, remote, 0);
		render_go(pLwc, view, proj, &puck_game->go[LPGO_TARGET], pLwc->tex_atlas[player_no == 2 ? LAE_PUCK_PLAYER_KTX : LAE_PUCK_ENEMY_KTX],
				  1.0f, remote_target_pos, state->target_rot, remote, 0);

		render_dir_pad(pLwc, pLwc->dir_pad_x, pLwc->dir_pad_y);
		if (pLwc->dir_pad_dragging) {
			render_dir_pad(pLwc, pLwc->dir_pad_touch_start_x, pLwc->dir_pad_touch_start_y);
		}
		render_fist_button(pLwc);
		render_top_button(pLwc);
		render_dash_gauge(pLwc);
	}
	const char* target_nickname = puck_game->battle_id ? puck_game->target_nickname : "? ? ?";
	render_hp_gauge(pLwc, -0.8f, 1.0f - 0.1f, state->player_current_hp, state->player_total_hp, player->hp_shake_remain_time, 1, puck_game->nickname);
	render_hp_gauge(pLwc, +0.8f, 1.0f - 0.1f, state->target_current_hp, state->target_total_hp, target->hp_shake_remain_time, 0, target_nickname);
	render_timer(pLwc, puck_game_remain_time(pLwc->puck_game->total_time, state->update_tick));
	render_match_state(pLwc);
}
