#include "lwcontext.h"
#include "render_physics.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"
#include "puckgame.h"
#include "render_field.h"
#include "lwtextblock.h"
#include "render_text_block.h"

static void render_go(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAMEOBJECT* go, int tex_index) {
	int shader_index = LWST_DEFAULT;
	mat4x4 rot;
	mat4x4_identity(rot);
	float sx = go->radius, sy = go->radius, sz = go->radius;
	float x = go->pos[0], y = go->pos[1], z = go->pos[2];

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_mul(model, model, rot);
	mat4x4_scale_aniso(model, model, sx, sy, sz);
	//mat4x4_scale_aniso(model, model, 5, 5, 5);

	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, z);

	mat4x4_mul(model, go->rot, model);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glUniform3f(pLwc->shader[shader_index].overlay_color_location, 1, 1, 1);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);

	const LW_VBO_TYPE lvt = LVT_PUCK;
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	//set_tex_filter(GL_NEAREST, GL_NEAREST);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_dash_gauge(const LWCONTEXT* pLwc) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	const float margin_x = 0.3f;
	const float margin_y = 0.2f * 5;
	const float gauge_width = 0.75f;
	const float gauge_height = 0.07f;
	const float gauge_flush_height = 0.07f;
	const float base_color = 0.3f;
	const float x = aspect_ratio - margin_x;
	const float y = -1 + margin_y;
	const float boost_gauge_ratio = puck_game_dash_gauge_ratio(pLwc->puck_game);
	
	// Background
	if (boost_gauge_ratio < 1.0f) {
		render_solid_vb_ui(pLwc,
			x - gauge_width * boost_gauge_ratio, y, gauge_width * (1.0f - boost_gauge_ratio), gauge_height,
			0,
			LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
			1, 1, base_color, base_color, 1);
	}
	// Foreground
	render_solid_vb_ui(pLwc,
		x, y, gauge_width * boost_gauge_ratio, gauge_height,
		0,
		LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
		1, base_color, 1, base_color, 1);
	if (boost_gauge_ratio < 1.0f) {
		// Flush
		render_solid_vb_ui(pLwc,
			x, y + boost_gauge_ratio / 4.0f, gauge_width, gauge_height,
			0,
			LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
			powf(1.0f - boost_gauge_ratio, 3.0f), base_color, 1, base_color, 1);
	}
	// Text
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

void lwc_render_physics(const LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int shader_index = LWST_DEFAULT;

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4 proj;
	mat4x4_perspective(proj, (float)(LWDEG2RAD(49.134) / screen_aspect_ratio), screen_aspect_ratio, 1.0f, 500.0f);

	mat4x4 view;
	vec3 eye = { 0.0f, -2.0f, 8.5f };

	vec3 center = { 0, 0, 0 };

	vec3 up = { 0, 1, 0 };
	mat4x4_look_at(view, eye, center, up);

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform2fv(pLwc->shader[shader_index].vs9offset_location, 1, default_uv_offset);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1.0f);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, 1, 1, 1);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
	

	const LWPUCKGAME* puck_game = pLwc->puck_game;

	{
		const int tex_index = 0;
		mat4x4 rot;
		mat4x4_identity(rot);

		float sx = 2.0f, sy = 2.0f, sz = 2.0f;
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

		glUniform3f(pLwc->shader[shader_index].overlay_color_location, 0.5f, 0.5f, 0.5f);
		glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 1.0f);

		//glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);

		const LW_VBO_TYPE lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;
		
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
		bind_all_vertex_attrib(pLwc, lvt);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_index);
		set_tex_filter(GL_LINEAR, GL_LINEAR);
		//set_tex_filter(GL_NEAREST, GL_NEAREST);
		glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
		glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
	}

	render_go(pLwc, view, proj, &puck_game->go[LPGO_PUCK], pLwc->tex_atlas[LAE_PUCK_KTX]);
	render_go(pLwc, view, proj, &puck_game->go[LPGO_PLAYER], pLwc->tex_atlas[LAE_PUCK_PLAYER_KTX]);
	render_go(pLwc, view, proj, &puck_game->go[LPGO_TARGET], pLwc->tex_atlas[LAE_PUCK_ENEMY_KTX]);

	render_dir_pad(pLwc);
	render_fist_button(pLwc);
	render_dash_gauge(pLwc);
}
