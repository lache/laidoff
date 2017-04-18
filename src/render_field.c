#include "lwcontext.h"
#include "render_field.h"
#include "lwlog.h"
#include "lwmacro.h"

static void render_field_object(const LWCONTEXT* pLwc, int vbo_index, GLuint tex_id, mat4x4 view, mat4x4 proj, float x, float y, float sx, float sy)
{
	int shader_index = 0;

	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, 0);
	mat4x4_scale_aniso(model, model, sx, sy, 1);
	mat4x4 model_translate;
	mat4x4_translate(model_translate, x, y, 0);
	mat4x4_mul(model, model_translate, model);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

	const float gird_uv_offset[2] = { 0, 0 };
	const float grid_uv_scale[2] = { 1, 1 };

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, gird_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, grid_uv_scale);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

static void render_ground(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj)
{
	int shader_index = 0;
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
	int shader_index = 0;
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

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

void lwc_render_field(const LWCONTEXT* pLwc)
{
	glViewport(0, 0, pLwc->width, pLwc->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int shader_index = 0;

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[pLwc->tex_atlas_index]);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

	const float screen_aspect_ratio = (float)pLwc->width / pLwc->height;

	mat4x4 perspective;
	mat4x4_perspective(perspective, (float)(LWDEG2RAD(49.134) / screen_aspect_ratio), screen_aspect_ratio, 1.0f, 50.0f);

	const float cam_dist = 20;
	mat4x4 view;
	vec3 eye = { pLwc->player_pos_x, pLwc->player_pos_y - cam_dist, cam_dist };
	vec3 center = { pLwc->player_pos_x, pLwc->player_pos_y, 0.0f };
	vec3 up = { 0, 0, 1 };
	mat4x4_look_at(view, eye, center, up);

	render_ground(pLwc, view, perspective);
	
	render_field_object(pLwc, LVT_PLAYER, pLwc->tex_programmed[LPT_SOLID_RED], view, perspective, pLwc->player_pos_x, pLwc->player_pos_y, 1, 1);

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
				pLwc->field_object[i].sx,
				pLwc->field_object[i].sy
			);
		}
	}

	render_ui(pLwc);
	
	// give up const-ness
	((LWCONTEXT*)pLwc)->render_count++;
}
