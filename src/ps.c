#include "ps.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "lwgl.h"

LWEMITTER emitter = { 0 };

void ps_load_particles(LWCONTEXT* pLwc) {
	for (int i = 0; i < NUM_PARTICLES; i++) {
		emitter.particles[i].theta = (float)LWDEG2RAD(i);
	}
	glGenBuffers(1, &pLwc->particle_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(emitter.particles), emitter.particles, GL_STATIC_DRAW);
}

void ps_load_emitter(LWCONTEXT* pLwc) {
	emitter.k = 4.0f;
}
