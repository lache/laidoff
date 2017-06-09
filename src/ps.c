#include "ps.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "pcg_basic.h"

LWEMITTER emitter = { 0 };
pcg32_random_t rng;
float time_current;
float time_max;
int time_direction;

// return [0, 1) double
double random_double() {
	return ldexp(pcg32_random_r(&rng), -32);
}

float random_float_between(float min, float max) {
	float range = max - min;
	return (float)(min + random_double() * range);
}

void ps_load_particles(LWCONTEXT* pLwc) {
	pcg32_srandom_r(&rng, 0x0DEEC2CBADF00D77, 0x15881588CA11DAC1);

	for (int i = 0; i < NUM_PARTICLES; i++) {
		emitter.particles[i].theta = (float)LWDEG2RAD(i);
		emitter.particles[i].shade[0] = random_float_between(-0.25f, 0.25f);
		emitter.particles[i].shade[1] = random_float_between(-0.25f, 0.25f);
		emitter.particles[i].shade[2] = random_float_between(-0.25f, 0.25f);
	}
	glGenBuffers(1, &pLwc->particle_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(emitter.particles), emitter.particles, GL_STATIC_DRAW);
}

void ps_load_emitter(LWCONTEXT* pLwc) {
	emitter.k = 4.0f;
	emitter.color[0] = 0.76f;
	emitter.color[1] = 0.12f;
	emitter.color[2] = 0.34f;

	time_current = 0.0f;
	time_max = 3.0f;
	time_direction = 1;
}

void ps_update(LWCONTEXT* pLwc) {
	if (time_current > time_max) {
		time_direction = -1;
	} else if (time_current < 0.0f) {
		time_direction = 1;
	}
	time_current += (float)(time_direction * lwcontext_delta_time(pLwc));
}
