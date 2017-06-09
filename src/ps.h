#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

#define NUM_PARTICLES (360)

typedef struct _LWPARTICLE {
	float theta;
	float shade[3];
} LWPARTICLE;

typedef struct _LWEMITTER {
	LWPARTICLE particles[NUM_PARTICLES];
	float k;
	float color[3];
} LWEMITTER;

void ps_load_particles(LWCONTEXT* pLwc);
void ps_load_emitter(LWCONTEXT* pLwc);
void ps_update(LWCONTEXT* pLwc);
