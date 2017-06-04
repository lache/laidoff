#include "lwcontext.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwanim.h"
#include "lwlog.h"

void render_solid_box_ui_lvt_flip_y_uv(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index, enum _LW_VBO_TYPE lvt, int flip_y_uv)
{
	render_solid_vb_ui_flip_y_uv(pLwc, x, y, w, h, tex_index, lvt, 1, 1, 1, 1, 0, flip_y_uv);
}

void render_solid_box_ui(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index)
{
	render_solid_vb_ui(pLwc, x, y, w, h, tex_index, LVT_LEFT_BOTTOM_ANCHORED_SQUARE, 1, 1, 1, 1, 0);
}

void render_solid_box_ui_alpha(const LWCONTEXT* pLwc, float x, float y, float w, float h, GLuint tex_index, float alpha_multiplier)
{
	render_solid_vb_ui(pLwc, x, y, w, h, tex_index, LVT_LEFT_BOTTOM_ANCHORED_SQUARE, alpha_multiplier, 1, 1, 1, 0);
}

void render_solid_vb_ui_flip_y_uv(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or, float og, float ob, float oratio, int flip_y_uv)
{
	int shader_index = LWST_DEFAULT;

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, flip_y_uv ? default_flip_y_uv_scale : default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, or , og, ob);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, oratio);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

	float ui_scale_x = w / 2;
	float ui_scale_y = h / 2;

	mat4x4 model_translate;
	mat4x4 model;
	mat4x4 identity_view; mat4x4_identity(identity_view);
	mat4x4 view_model;
	mat4x4 proj_view_model;
	mat4x4 model_scale;

	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, ui_scale_x, ui_scale_y, 1.0f);
	mat4x4_translate(model_translate, x, y, 0);
	mat4x4_identity(model);
	mat4x4_mul(model, model_translate, model_scale);
	mat4x4_mul(view_model, identity_view, model);
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, pLwc->proj, view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	//set_tex_filter(GL_NEAREST, GL_NEAREST);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

void render_solid_vb_ui_alpha(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index, GLuint tex_alpha_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or, float og, float ob, float oratio)
{
	int shader_index = LWST_ETC1;

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, or , og, ob);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, oratio);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

	float ui_scale_x = w / 2;
	float ui_scale_y = h / 2;

	mat4x4 model_translate;
	mat4x4 model;
	mat4x4 identity_view; mat4x4_identity(identity_view);
	mat4x4 view_model;
	mat4x4 proj_view_model;
	mat4x4 model_scale;

	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, ui_scale_x, ui_scale_y, 1.0f);
	mat4x4_translate(model_translate, x, y, 0);
	mat4x4_identity(model);
	mat4x4_mul(model, model_translate, model_scale);
	mat4x4_mul(view_model, identity_view, model);
	mat4x4_identity(proj_view_model);
	mat4x4_mul(proj_view_model, pLwc->proj, view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	bind_all_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex_alpha_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);

	glActiveTexture(GL_TEXTURE0);
}

void render_solid_vb_ui(const LWCONTEXT* pLwc,
	float x, float y, float w, float h,
	GLuint tex_index,
	enum _LW_VBO_TYPE lvt,
	float alpha_multiplier, float or, float og, float ob, float oratio) {

	render_solid_vb_ui_flip_y_uv(pLwc, x, y, w, h, tex_index, lvt, alpha_multiplier, or , og, ob, oratio, 0);

}
