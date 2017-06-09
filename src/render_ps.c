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

void lwc_render_ps(const LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int shader_index = LWST_EMITTER;
	glUseProgram(pLwc->shader[shader_index].program);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
#if LW_PLATFORM_WIN32
	//glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	glDisableVertexAttribArray(pLwc->shader[shader_index].theta_location);
	glDisableVertexAttribArray(pLwc->shader[shader_index].shade_location);
}
