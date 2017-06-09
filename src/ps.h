#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

#define NUM_PARTICLES (360)

typedef struct _LWPARTICLE {
	float theta;
} LWPARTICLE;

typedef struct _LWEMITTER {
	LWPARTICLE particles[NUM_PARTICLES];
	float k;
} LWEMITTER;

void ps_load_particles(LWCONTEXT* pLwc);
void ps_load_emitter(LWCONTEXT* pLwc);
