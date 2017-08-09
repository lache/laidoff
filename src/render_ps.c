#include "render_ps.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "ps.h"
#include "laidoff.h"
#include "platform_detection.h"
#include "lwmacro.h"

extern LWEMITTER emitter;
extern float time_current;
extern float time_max;

extern LWEMITTER2 emitter2;
extern LWEMITTER2OBJECT emitter2_object;

static void s_render_rose(const LWCONTEXT* pLwc) {
	int shader_index = LWST_EMITTER;
	glUseProgram(pLwc->shader[shader_index].program);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	mat4x4 identity;
	mat4x4_identity(identity);
	glUniformMatrix4fv(pLwc->shader[shader_index].projection_matrix_location, 1, 0, (const GLfloat*)identity);
	glUniform1f(pLwc->shader[shader_index].k_location, emitter.k);
	glUniform3f(pLwc->shader[shader_index].color_location, emitter.color[0], emitter.color[1], emitter.color[2]);
	glUniform1f(pLwc->shader[shader_index].time_location, time_current / time_max);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
	glUniform1i(pLwc->shader[shader_index].texture_location, 0);
	glEnableVertexAttribArray(pLwc->shader[shader_index].theta_location);
	glVertexAttribPointer(pLwc->shader[shader_index].theta_location, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE), (void*)LWOFFSETOF(LWPARTICLE, theta));
	glEnableVertexAttribArray(pLwc->shader[shader_index].shade_location);
	glVertexAttribPointer(pLwc->shader[shader_index].shade_location, 3, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE), (void*)LWOFFSETOF(LWPARTICLE, shade));
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	glDisableVertexAttribArray(pLwc->shader[shader_index].theta_location);
	glDisableVertexAttribArray(pLwc->shader[shader_index].shade_location);
}

void ps_render_explosion(const LWCONTEXT* pLwc, const LWEMITTER2OBJECT* emit_object, const mat4x4 proj_view, const mat4x4 model) {
	int shader_index = LWST_EMITTER2;
	glUseProgram(pLwc->shader[shader_index].program);
	// Uniforms
	glUniformMatrix4fv(pLwc->shader[shader_index].u_ProjectionViewMatrix, 1, 0, (const GLfloat*)proj_view);
	glUniformMatrix4fv(pLwc->shader[shader_index].u_ModelMatrix, 1, 0, (const GLfloat*)model);
	glUniform2f(pLwc->shader[shader_index].u_Gravity, emit_object->gravity[0], emit_object->gravity[1]);
	glUniform1f(pLwc->shader[shader_index].u_Time, emit_object->time);
	glUniform1f(pLwc->shader[shader_index].u_eRadius, emitter2.eRadius);
	glUniform1f(pLwc->shader[shader_index].u_eVelocity, emitter2.eVelocity);
	glUniform1f(pLwc->shader[shader_index].u_eDecay, emitter2.eDecay);
	glUniform1f(pLwc->shader[shader_index].u_eSizeStart, emitter2.eSizeStart);
	glUniform1f(pLwc->shader[shader_index].u_eSizeEnd, emitter2.eSizeEnd);
	glUniform1f(pLwc->shader[shader_index].u_eScreenWidth, (float)pLwc->width);
	glUniform3fv(pLwc->shader[shader_index].u_eColorStart, 1, emitter2.eColorStart);
	glUniform3fv(pLwc->shader[shader_index].u_eColorEnd, 1, emitter2.eColorEnd);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[LAE_U_GLOW]);
	glUniform1i(pLwc->shader[shader_index].u_Texture, 0);
	set_tex_filter(GL_LINEAR, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[LAE_U_GLOW_ALPHA]);
	glUniform1i(pLwc->shader[shader_index].u_TextureAlpha, 1);
	set_tex_filter(GL_LINEAR, GL_LINEAR);

	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);

	// Attributes
	/*glEnableVertexAttribArray(pLwc->shader[shader_index].a_pID);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pRadiusOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pVelocityOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pDecayOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pSizeOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pColorOffset);

	glVertexAttribPointer(pLwc->shader[shader_index].a_pID, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pId)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pRadiusOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pRadiusOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pVelocityOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pVelocityOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pDecayOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pDecayOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pSizeOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pSizeOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pColorOffset, 3, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pColorOffset)));
*/
	bind_all_ps_vertex_attrib(pLwc, LPVT_DEFAULT);

	// Draw particles
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES2);
    
	/*glDisableVertexAttribArray(pLwc->shader[shader_index].a_pID);
	glDisableVertexAttribArray(pLwc->shader[shader_index].a_pRadiusOffset);
	glDisableVertexAttribArray(pLwc->shader[shader_index].a_pVelocityOffset);
	glDisableVertexAttribArray(pLwc->shader[shader_index].a_pDecayOffset);
	glDisableVertexAttribArray(pLwc->shader[shader_index].a_pSizeOffset);
	glDisableVertexAttribArray(pLwc->shader[shader_index].a_pColorOffset);*/
}

void lwc_render_ps(const LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if LW_PLATFORM_WIN32
	//glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
	// Test render rose
	s_render_rose(pLwc);
	// Test render explosion
	mat4x4 identity;
	mat4x4_identity(identity);
	ps_render_explosion(pLwc, &emitter2_object, identity, identity);
}

void ps_render_with_projection(const LWCONTEXT* pLwc, const LWEMITTER2OBJECT* emit, const float* proj) {
	
}
