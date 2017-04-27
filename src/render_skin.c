#include "lwcontext.h"
#include "render_skin.h"
#include "render_solid.h"
#include "laidoff.h"

static void render_ground(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
	int shader_index = LWST_DEFAULT;
	const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

	const float quad_scale = 1.0f;
	mat4x4 model;
	mat4x4_identity(model);
	mat4x4_rotate_X(model, model, 0);
	mat4x4_scale_aniso(model, model, quad_scale, quad_scale, quad_scale);

	mat4x4 view_model;
	mat4x4_mul(view_model, view, model);

	mat4x4 proj_view_model;
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glUseProgram(pLwc->shader[shader_index].program);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
	bind_all_vertex_attrib(pLwc, vbo_index);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_GRID]);
	set_tex_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

	const float gird_uv_offset[2] = { 0, 0 };
	const float grid_uv_scale[2] = { 2, 2 };

	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, 1.0f);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0.0f);

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, gird_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, grid_uv_scale);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

void lwc_render_skin(const struct _LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	mat4x4 identity;
	mat4x4_identity(identity);

	render_ground(pLwc, identity, pLwc->proj);

	render_solid_vb_ui_skin(pLwc, 0, 0, 0.5f,
		pLwc->tex_atlas[LAE_C_TOFU_KTX], pLwc->tex_atlas[LAE_C_TOFU_ALPHA_KTX],
		LSVT_TRIANGLE,
		&pLwc->action[LWAC_TESTACTION2],
		&pLwc->armature[LWAR_ARMATURE],
		1, 0, 0, 0, 0);

	render_solid_vb_ui_skin(pLwc, 0, 0, 0.5f,
		pLwc->tex_atlas[LAE_C_TOFU_KTX], pLwc->tex_atlas[LAE_C_TOFU_ALPHA_KTX],
		LSVT_TREEPLANE,
		&pLwc->action[LWAC_TREEARMATUREACTION],
		&pLwc->armature[LWAR_TREEARMATURE],
		1, 0, 0, 0, 0);

	glEnable(GL_DEPTH_TEST);
}
