#include "lwcontext.h"
#include "render_field.h"
#include "lwlog.h"
#include "lwmacro.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_skin.h"
#include "field.h"
#include "mq.h"
#include "render_fan.h"

static void render_field_object(const LWCONTEXT* pLwc, int vbo_index, GLuint tex_id, mat4x4 view, mat4x4 proj, float x, float y, float z, float sx, float sy, float sz, float alpha_multiplier, int mipmap)
{
	int shader_index = LWST_DEFAULT;

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, 0);
	mat4x4_scale_aniso(model, model, sx, sy, sz);
	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, z);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glUseProgram(pLwc->shader[shader_index].program);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	set_tex_filter(mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR, GL_LINEAR);

	const float gird_uv_offset[2] = { 0, 0 };
	const float grid_uv_scale[2] = { 1, 1 };

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, gird_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, grid_uv_scale);
	glUniform1fv(pLwc->shader[shader_index].alpha_multiplier_location, 1, &alpha_multiplier);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

static void render_ground(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj)
{
	int shader_index = LWST_DEFAULT;
	const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

	const float quad_scale = 10;
	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, 0);
	mat4x4_scale_aniso(model, model, quad_scale, quad_scale, quad_scale);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_GRID]);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

	const float gird_uv_offset[2] = { 0, 0 };
	const float grid_uv_scale[2] = { quad_scale, quad_scale };

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, gird_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, grid_uv_scale);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

static void render_ui(const LWCONTEXT* pLwc)
{
	int shader_index = LWST_DEFAULT;
	const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

	//float aspect_ratio = (float)pLwc->width / pLwc->height;
	//int dir_pad_size_pixel = pLwc->width < pLwc->height ? pLwc->width : pLwc->height;

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, 0);
	mat4x4_scale_aniso(model, model, 0.05f, 0.05f, 0.05f);
	mat4x4 model_translate;
	mat4x4_translate(model_translate, pLwc->dir_pad_x, pLwc->dir_pad_y, 0);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4 view; mat4x4_identity(view);
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, pLwc->proj, view_model);

	glUseProgram(pLwc->shader[shader_index].program);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	const float fist_icon_margin_x = 0.3f;
	const float fist_icon_margin_y = 0.2f;

	render_solid_vb_ui_alpha(pLwc, aspect_ratio - fist_icon_margin_x, -1 + fist_icon_margin_y, 0.75f, 0.75f,
		pLwc->tex_atlas[LAE_U_FIST_ICON_KTX], pLwc->tex_atlas[LAE_U_FIST_ICON_ALPHA_KTX],
		LVT_RIGHT_BOTTOM_ANCHORED_SQUARE, 1, 0, 0, 0, 0);
}

void render_debug_sphere(const LWCONTEXT* pLwc, GLuint tex_id, mat4x4 perspective, mat4x4 view, float x, float y, float z) {

	render_field_object(
		pLwc,
		LVT_SPHERE,
		tex_id,
		view,
		perspective,
		x,
		y,
		z,
		1.0f,
		1.0f,
		1.0f,
		1,
		1
	);
}

void render_path_query_test_player(const LWCONTEXT* pLwc, mat4x4 perspective, mat4x4 view) {

	float spos[3], epos[3];
	field_path_query_spos(pLwc->field, spos);
	field_path_query_epos(pLwc->field, epos);

	render_debug_sphere(pLwc,
		pLwc->tex_programmed[LPT_SOLID_RED],
		perspective,
		view,
		spos[0],
		spos[1],
		spos[2]);

	render_debug_sphere(pLwc,
		pLwc->tex_programmed[LPT_SOLID_BLUE],
		perspective,
		view,
		epos[0],
		epos[1],
		epos[2]);

	if (!pLwc->fps_mode && field_path_query_n_smooth_path(pLwc->field)) {

		const float* path_query_test_player_pos = field_path_query_test_player_pos(pLwc->field);
		const float skin_scale_f = field_skin_scale(pLwc->field);

		mat4x4 skin_trans;
		mat4x4_identity(skin_trans);
		mat4x4_translate(skin_trans, path_query_test_player_pos[0], path_query_test_player_pos[1], path_query_test_player_pos[2]);
		mat4x4 skin_scale;
		mat4x4_identity(skin_scale);
		mat4x4_scale_aniso(skin_scale, skin_scale, skin_scale_f, skin_scale_f, skin_scale_f);
		mat4x4 skin_rot;
		mat4x4_identity(skin_rot);
		mat4x4_rotate_Z(skin_rot, skin_rot, field_path_query_test_player_rot(pLwc->field) + (float)LWDEG2RAD(90));

		mat4x4 skin_model;
		mat4x4_identity(skin_model);
		mat4x4_mul(skin_model, skin_rot, skin_model);
		mat4x4_mul(skin_model, skin_scale, skin_model);
		mat4x4_mul(skin_model, skin_trans, skin_model);

		render_skin(pLwc,
			pLwc->tex_atlas[LAE_3D_PLAYER_TEX_KTX],
			LSVT_HUMAN,
			&pLwc->action[LWAC_HUMANACTION_WALKPOLISH],
			&pLwc->armature[LWAR_HUMANARMATURE],
			1, 0, 0, 0, 0, perspective, view, skin_model, pLwc->test_player_skin_time * 5, 1);
	}
}

void render_player_model(const LWCONTEXT* pLwc, mat4x4 perspective, mat4x4 view, float x, float y, float z, float a, const LWANIMACTION* action, float skin_time, int loop) {
	const float skin_scale_f = field_skin_scale(pLwc->field);

	mat4x4 skin_trans;
	mat4x4_identity(skin_trans);
	mat4x4_translate(skin_trans, x, y, z);
	mat4x4 skin_scale;
	mat4x4_identity(skin_scale);
	mat4x4_scale_aniso(skin_scale, skin_scale, skin_scale_f, skin_scale_f, skin_scale_f);
	mat4x4 skin_rot;
	mat4x4_identity(skin_rot);
	mat4x4_rotate_Z(skin_rot, skin_rot, a + (float)LWDEG2RAD(90));

	mat4x4 skin_model;
	mat4x4_identity(skin_model);
	mat4x4_mul(skin_model, skin_rot, skin_model);
	mat4x4_mul(skin_model, skin_scale, skin_model);
	mat4x4_mul(skin_model, skin_trans, skin_model);

	if (pLwc->player_action) {
		render_skin(pLwc,
			pLwc->tex_atlas[LAE_3D_PLAYER_TEX_KTX],
			LSVT_HUMAN,
			action,
			&pLwc->armature[LWAR_HUMANARMATURE],
			1, 0, 0, 0, 0, perspective, view, skin_model, skin_time, loop);
	}
}

void lwc_render_field(const LWCONTEXT* pLwc)
{
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int shader_index = LWST_DEFAULT;

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[pLwc->tex_atlas_index]);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4 perspective;
	mat4x4_perspective(perspective, (float)(LWDEG2RAD(49.134) / screen_aspect_ratio), screen_aspect_ratio, 1.0f, 500.0f);

	float player_x = 0, player_y = 0, player_z = 0;
	get_field_player_position(pLwc->field, &player_x, &player_y, &player_z);

	const float* path_query_test_player_pos = field_path_query_test_player_pos(pLwc->field);
	const float path_query_test_player_rot = field_path_query_test_player_rot(pLwc->field);

	mat4x4 view;
	vec3 eye = {
		path_query_test_player_pos[0],
		path_query_test_player_pos[1],
		path_query_test_player_pos[2] + 5
	};

	vec3 center = {
		path_query_test_player_pos[0] + cosf(path_query_test_player_rot),
		path_query_test_player_pos[1] + sinf(path_query_test_player_rot),
		path_query_test_player_pos[2] + 5
	};

	if (!pLwc->fps_mode) {
		if (field_follow_cam(pLwc->field)) {
			const float cam_dist = 30;

			eye[0] = player_x;
			eye[1] = player_y - cam_dist;
			eye[2] = player_z + cam_dist;

			center[0] = player_x;
			center[1] = player_y;
			center[2] = player_z;
		} else {
			const float cam_dist = 230;

			eye[0] = 270 + player_x;
			eye[1] = player_y - cam_dist + 200;
			eye[2] = cam_dist - 100;

			center[0] = player_x;
			center[1] = player_y;
			center[2] = player_z + 60;
		}
	}

	vec3 up = { 0, 0, 1 };
	mat4x4_look_at(view, eye, center, up);

	//render_ground(pLwc, view, perspective);

	if (!pLwc->hide_field) {
		render_field_object(
			pLwc,
			field_field_vbo(pLwc->field),
			field_field_tex_id(pLwc->field),
			view,
			perspective,
			0,
			0,
			0,
			1,
			1,
			1,
			1,
			field_field_tex_mip(pLwc->field)
		);
	}
	
	for (int i = 0; i < MAX_FIELD_OBJECT; i++)
	{
		if (pLwc->field_object[i].valid)
		{
			render_field_object(
				pLwc,
				pLwc->field_object[i].lvt,
				pLwc->field_object[i].tex_id,
				view,
				perspective,
				pLwc->field_object[i].x,
				pLwc->field_object[i].y,
				0,
				pLwc->field_object[i].sx,
				pLwc->field_object[i].sy,
				1.0f,
				pLwc->field_object[i].alpha_multiplier,
				1
			);
		}
	}

	render_player_model(pLwc, perspective, view, player_x, player_y, player_z, pLwc->player_rot_z, pLwc->player_action, pLwc->player_state_data.skin_time, pLwc->player_action_loop);

	LWPOSSYNCMSG* value = mq_possync_first(pLwc->mq);
	while (value) {
		const char* cursor = mq_possync_cursor(pLwc->mq);
		// Exclude the player
		if (!mq_cursor_player(pLwc->mq, cursor)) {
			render_player_model(pLwc, perspective, view, value->x, value->y, value->z, value->a, value->action, pLwc->player_skin_time, 1);
		}
		value = mq_possync_next(pLwc->mq);
	}

	render_path_query_test_player(pLwc, perspective, view);

	float rscale[FAN_VERTEX_COUNT_PER_ARRAY];
	rscale[0] = 0; // center vertex has no meaningful rscale
	for (int i = 1; i < FAN_VERTEX_COUNT_PER_ARRAY; i++) {
		rscale[i] = (float)field_ray_nearest_depth(pLwc->field, LRI_AIM_SECTOR_FIRST_INCLUSIVE + i - 1);
	}

	if (pLwc->player_state_data.state == LPS_AIM || pLwc->player_state_data.state == LPS_FIRE) {
		render_fan(pLwc, perspective, view,
			player_x, player_y, player_z, pLwc->player_rot_z, pLwc->player_state_data.aim_theta, rscale);
	}

	render_ui(pLwc);

	// give up const-ness
	((LWCONTEXT*)pLwc)->render_count++;
}
