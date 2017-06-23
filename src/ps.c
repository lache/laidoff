#include "ps.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "lwgl.h"
#include "pcg_basic.h"
#include <stdlib.h>

#define NUM_PS_INSTANCE (16)

LWEMITTER emitter = { 0 };
pcg32_random_t rng;
float time_current;
float time_max;
int time_direction;
LWEMITTER2OBJECT emitter2_object = { 0 };
LWEMITTER2 emitter2 = { 0 };

typedef struct _LWPSINST {
	int valid;
	LWEMITTER2OBJECT emit_object;
} LWPSINST;

typedef struct _LWPS {
	LWPSINST inst[NUM_PS_INSTANCE];
} LWPS;

float oDecay = 0.25f;       // Time

// return [0, 1) double
double random_double() {
	return ldexp(pcg32_random_r(&rng), -32);
}

float random_float_between(float min, float max) {
	float range = max - min;
	return (float)(min + random_double() * range);
}

void load_emitter2(LWCONTEXT* pLwc) {
	// 3
	// Offset bounds
	float oRadius = 0.10f;      // 0.0 = circle; 1.0 = ring
	float oVelocity = 0.50f;    // Speed
	
	float oSize = 8.00f;        // Pixels
	float oColor = 0.25f;       // 0.5 = 50% shade offset

	// 4
	// Load Particles
	for (int i = 0; i<NUM_PARTICLES2; i++) {
		// Assign a unique ID to each particle, between 0 and 360 (in radians)
		emitter2.eParticles[i].pId = (float)LWDEG2RAD(((float)i / (float)NUM_PARTICLES2)*360.0f);

		// Assign random offsets within bounds
		emitter2.eParticles[i].pRadiusOffset = random_float_between(oRadius, 1.00f);
		emitter2.eParticles[i].pVelocityOffset = random_float_between(-oVelocity, oVelocity);
		emitter2.eParticles[i].pDecayOffset = random_float_between(-oDecay, oDecay);
		emitter2.eParticles[i].pSizeOffset = random_float_between(-oSize, oSize);
		float r = random_float_between(-oColor, oColor);
		float g = random_float_between(-oColor, oColor);
		float b = random_float_between(-oColor, oColor);
		emitter2.eParticles[i].pColorOffset[0] = r;
		emitter2.eParticles[i].pColorOffset[1] = g;
		emitter2.eParticles[i].pColorOffset[2] = b;
	}

	// 5
	// Load Properties
	emitter2.eRadius = 0.75f;                                     // Blast radius
	emitter2.eVelocity = 3.00f;                                   // Explosion velocity
	emitter2.eDecay = 2.00f;                                      // Explosion decay
	emitter2.eSizeStart = 32.00f;                                 // Fragment size
	emitter2.eSizeEnd = 8.00f;                                 // Fragment size
	emitter2.eColorStart[0] = 1.0f;
	emitter2.eColorStart[1] = 0.5f;
	emitter2.eColorStart[2] = 0.0f;
	emitter2.eColorEnd[0] = 0.25f;
	emitter2.eColorEnd[1] = 0.0f;
	emitter2.eColorEnd[2] = 0.0f;
																// Set global factors
	float growth = emitter2.eRadius / emitter2.eVelocity;       // Growth time
	emitter2_object.life = growth + emitter2.eDecay + oDecay;                    // Simulation lifetime

	float drag = 10.00f;                                            // Drag (air resistance)
	emitter2_object.gravity[0] = 0;
	emitter2_object.gravity[1] = -9.81f * (1.0f / drag);			  // 7
	
	glGenBuffers(1, &pLwc->particle_buffer2);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(emitter2.eParticles), emitter2.eParticles, GL_STATIC_DRAW);
}

void ps_load_particles(LWCONTEXT* pLwc) {
	pcg32_srandom_r(&rng, 0x0DEEC2CBADF00D77, 0x15881588CA11DAC1);
	//pcg32_srandom_r(&rng, 3, 56);

	for (int i = 0; i < NUM_PARTICLES; i++) {
		emitter.particles[i].theta = (float)LWDEG2RAD(i);
		emitter.particles[i].shade[0] = random_float_between(-0.25f, 0.25f);
		emitter.particles[i].shade[1] = random_float_between(-0.25f, 0.25f);
		emitter.particles[i].shade[2] = random_float_between(-0.25f, 0.25f);
	}
	glGenBuffers(1, &pLwc->particle_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(emitter.particles), emitter.particles, GL_STATIC_DRAW);

	load_emitter2(pLwc);
}

void ps_load_emitter(LWCONTEXT* pLwc) {
	emitter.k = 4.0f;
	emitter.color[0] = 0.76f;
	emitter.color[1] = 0.12f;
	emitter.color[2] = 0.34f;

	time_current = 0.0f;
	time_max = 3.0f;
	time_direction = 1;

	emitter2_object.gravity[0] = 0;
	emitter2_object.gravity[1] = 0;
	emitter2_object.life = 0;
	emitter2_object.time = 0;
}

void ps_test_update(LWCONTEXT* pLwc) {
	if (time_current > time_max) {
		time_direction = -1;
	} else if (time_current < 0.0f) {
		time_direction = 1;
	}
	const float time_elapsed = (float)lwcontext_delta_time(pLwc);
	time_current += (float)(time_direction * time_elapsed);

	emitter2_object.time += time_elapsed;
	if (emitter2_object.time > emitter2_object.life) {
		emitter2_object.time = 0.0f;
	}
}

void ps_update(LWPS* ps, double delta_time) {
	for (int i = 0; i < NUM_PS_INSTANCE; i++) {
		if (!ps->inst[i].valid) {
			continue;
		}
		ps->inst[i].emit_object.time += (float)delta_time;
		if (ps->inst[i].emit_object.time > ps->inst[i].emit_object.life) {
			ps->inst[i].valid = 0;
		}
	}
}

void* ps_new() {
	return calloc(1, sizeof(LWPS));
}

void ps_destroy(LWPS** ps) {
	free(*ps);
	*ps = 0;
}

void ps_play_new(LWPS* ps) {
	for (int i = 0; i < NUM_PS_INSTANCE; i++) {
		if (ps->inst[i].valid) {
			continue;
		}
		ps->inst[i].valid = 1;

		float growth = emitter2.eRadius / emitter2.eVelocity;
		ps->inst[i].emit_object.life = growth + emitter2.eDecay + oDecay;

		float drag = 10.00f;
		ps->inst[i].emit_object.gravity[0] = 0;
		ps->inst[i].emit_object.gravity[1] = -9.81f * (1.0f / drag);
		ps->inst[i].emit_object.time = 0;
		break;
	}
}

static LWEMITTER2OBJECT* s_ps_emit_object_find_first_valid(LWPS* ps, int start_index) {
	if (!ps) {
		return 0;
	}
	for (int i = start_index; i < NUM_PS_INSTANCE; i++) {
		if (ps->inst[i].valid) {
			return &ps->inst[i].emit_object;
		}
	}
	return 0;
}

LWEMITTER2OBJECT* ps_emit_object_begin(LWPS* ps) {
	return s_ps_emit_object_find_first_valid(ps, 0);
}

LWEMITTER2OBJECT* ps_emit_object_next(LWPS* ps, LWEMITTER2OBJECT* cursor) {
	int start_index = ((char*)cursor - (char*)&ps->inst[0].emit_object) / sizeof(LWPSINST);
	return s_ps_emit_object_find_first_valid(ps, start_index + 1);
}
