#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

#define NUM_PARTICLES (360)
#define NUM_PARTICLES2 (180)

typedef struct _LWPARTICLE {
	float theta;
	float shade[3];
} LWPARTICLE;

typedef struct _LWEMITTER {
	LWPARTICLE particles[NUM_PARTICLES];
	float k;
	float color[3];
} LWEMITTER;

typedef struct _LWPARTICLE2 {
	float       pId;
	float       pRadiusOffset;
	float       pVelocityOffset;
	float       pDecayOffset;
	float       pSizeOffset;
	float		pColorOffset[3];
} LWPARTICLE2;

typedef struct _LWEMITTER2 {
	LWPARTICLE2    eParticles[NUM_PARTICLES2];
	float       eRadius;
	float       eVelocity;
	float       eDecay;
	float       eSizeStart;
	float       eSizeEnd;
	float		eColorStart[3];
	float		eColorEnd[3];
} LWEMITTER2;

typedef struct _LWEMITTER2OBJECT {
	float gravity[2];
	float life;
	float time;
} LWEMITTER2OBJECT;

void ps_load_particles(LWCONTEXT* pLwc);
void ps_load_emitter(LWCONTEXT* pLwc);
void ps_update(LWCONTEXT* pLwc);
