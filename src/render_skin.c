#include "lwcontext.h"
#include "render_skin.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"

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

	switch (pLwc->kp) {
	case 1:
		render_skin_ui(pLwc, 0, 0, 0.5f,
			pLwc->tex_atlas[LAE_C_TOFU_KTX],
			LSVT_TRIANGLE,
			&pLwc->action[LWAC_TRIANGLEACTION],
			&pLwc->armature[LWAR_TRIANGLEARMATURE],
			1, 0, 0, 0, 0);
		break;
	case 2:
		render_skin_ui(pLwc, 0, 0, 0.5f,
			pLwc->tex_atlas[LAE_C_TOFU_KTX],
			LSVT_TREEPLANE,
			&pLwc->action[LWAC_TREEPLANEACTION],
			&pLwc->armature[LWAR_TREEPLANEARMATURE],
			1, 0, 0, 0, 0);
		break;
	case 3:
		render_skin_ui(pLwc, 0, 0, 0.5f,
			pLwc->tex_atlas[LAE_C_TOFU_KTX],
			LSVT_DETACHPLANE,
			&pLwc->action[LWAC_DETACHPLANEACTION_CHILDTRANS],
			&pLwc->armature[LWAR_DETACHPLANEARMATURE],
			1, 0, 0, 0, 0);
		break;
	default:
		break;
	}
	
	glEnable(GL_DEPTH_TEST);
}


void render_skin(const LWCONTEXT* pLwc,
	GLuint tex_index,
	enum _LW_SKIN_VBO_TYPE lvt,
	const struct _LWANIMACTION* action,
	const struct _LWARMATURE* armature,
	float alpha_multiplier, float or , float og, float ob, float oratio,
	const mat4x4 proj, const mat4x4 view, const mat4x4 model) {

	int shader_index = LWST_SKIN;

	// MAX_BONE should be matched with a shader code
#define MAX_BONE (40)

	if (armature->count > MAX_BONE) {
		LOGE("Armature bone count (=%d) exceeding the supported bone count(=%d)!", armature->count, MAX_BONE);
		return;
	}

	vec3 bone_trans[MAX_BONE] = { 0, };
	quat bone_q[MAX_BONE];
	for (int i = 0; i < ARRAY_SIZE(bone_q); i++) {
		quat_identity(bone_q[i]);
	}

	float f = (float)(pLwc->skin_time * action->fps);
	f = fmodf(f, action->last_key_f);

	//const float f = 0;// 89;

	for (int i = 0; i < action->curve_num; i++) {
		const LWANIMCURVE* curve = &action->anim_curve[i];

		int bi = curve->bone_index;
		int ci = curve->anim_curve_index;
		const LWANIMKEY* anim_key = action->anim_key + curve->key_offset;

		if (curve->anim_curve_type == LACT_LOCATION) {
			get_curve_value(anim_key, curve->key_num, f, &bone_trans[bi][ci]);
		}

		if (curve->anim_curve_type == LACT_ROTATION_QUATERNION) {
			// Anim curve saved in w, x, y, z order, but client needs x, y, z, w order.
			get_curve_value(anim_key, curve->key_num, f, &bone_q[bi][(ci + 3) % 4]);
		}

	}

	// Renormalize quaternion since linear interpolation applied by anim curve.
	for (int i = 0; i < armature->count; i++) {
		quat_norm(bone_q[i], bone_q[i]);
	}

	mat4x4 bone[MAX_BONE];
	for (int i = 0; i < armature->count; i++) {
		mat4x4_identity(bone[i]);
	}

	mat4x4 bone_unmod_world[MAX_BONE];
	for (int i = 0; i < armature->count; i++) {
		if (armature->parent_index[i] >= 0) {
			mat4x4_mul(bone_unmod_world[i], bone_unmod_world[armature->parent_index[i]], armature->mat[i]);
		} else {
			mat4x4_dup(bone_unmod_world[i], armature->mat[i]);
		}
	}

	for (int i = 0; i < armature->count; i++) {
		mat4x4 bone_mat_anim_trans;

		// Connected bones should not have their own translation.
		if (armature->use_connect[i]) {
			mat4x4_identity(bone_mat_anim_trans);
		} else {
			mat4x4_translate(bone_mat_anim_trans, bone_trans[i][0], bone_trans[i][1], bone_trans[i][2]);
		}

		mat4x4 bone_mat_anim_rot;
		mat4x4_from_quat(bone_mat_anim_rot, bone_q[i]);

		mat4x4 bone_unmod_world_inv;
		mat4x4_invert(bone_unmod_world_inv, bone_unmod_world[i]);

		mat4x4 trans_bone_to_origin, trans_bone_to_its_position;
		mat4x4_translate(trans_bone_to_origin, -bone_unmod_world[i][3][0], -bone_unmod_world[i][3][1], -bone_unmod_world[i][3][2]);
		mat4x4_translate(trans_bone_to_its_position, bone_unmod_world[i][3][0], bone_unmod_world[i][3][1], bone_unmod_world[i][3][2]);

		mat4x4_identity(bone[i]);
		mat4x4_mul(bone[i], bone_unmod_world_inv, bone[i]);
		mat4x4_mul(bone[i], bone_mat_anim_rot, bone[i]);
		mat4x4_mul(bone[i], bone_mat_anim_trans, bone[i]);
		mat4x4_mul(bone[i], bone_unmod_world[i], bone[i]);

		if (armature->parent_index[i] >= 0) {
			mat4x4_mul(bone[i], bone[armature->parent_index[i]], bone[i]);
		}
	}

	glUseProgram(pLwc->shader[shader_index].program);
	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
	glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
	glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
	glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
	glUniform3f(pLwc->shader[shader_index].overlay_color_location, or , og, ob);
	glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, oratio);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);
	glUniformMatrix4fv(pLwc->shader[shader_index].bone_location, armature->count, GL_FALSE, (const GLfloat*)bone);

	mat4x4 view_model;
	mat4x4 proj_view_model;

	mat4x4_mul(view_model, view, model);
	mat4x4_mul(proj_view_model, proj, view_model);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->skin_vertex_buffer[lvt].vertex_buffer);
	bind_all_skin_vertex_attrib(pLwc, lvt);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_index);
	set_tex_filter(GL_LINEAR, GL_LINEAR);
	glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
	glDrawArrays(GL_TRIANGLES, 0, pLwc->skin_vertex_buffer[lvt].vertex_count);

	glActiveTexture(GL_TEXTURE0);
}

void render_skin_ui(const LWCONTEXT* pLwc,
	float x, float y, float scale,
	GLuint tex_index,
	enum _LW_SKIN_VBO_TYPE lvt,
	const struct _LWANIMACTION* action,
	const struct _LWARMATURE* armature,
	float alpha_multiplier, float or , float og, float ob, float oratio) {

	mat4x4 model;
	mat4x4 model_translate;
	mat4x4 model_scale;

	mat4x4_identity(model_scale);
	mat4x4_scale_aniso(model_scale, model_scale, scale, scale, scale);
	mat4x4_translate(model_translate, x, y, 0);
	mat4x4_identity(model);
	mat4x4_mul(model, model_translate, model_scale);

	mat4x4 identity_view;
	mat4x4_identity(identity_view);

	render_skin(pLwc, tex_index, lvt, action, armature, alpha_multiplier, or , og, ob, oratio, pLwc->proj, identity_view, model);
}
