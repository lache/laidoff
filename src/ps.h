#pragma once

#include <linmath.h>

typedef struct _LWCONTEXT LWCONTEXT;

#define NUM_PARTICLES (360)
#define NUM_PARTICLES2 (100)

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
	vec3 pos;
} LWEMITTER2OBJECT;

typedef struct _LWPS LWPS;

void ps_load_particles(LWCONTEXT* pLwc);
void ps_load_emitter(LWCONTEXT* pLwc);
void ps_test_update(LWCONTEXT* pLwc);
void* ps_new();
void ps_update(LWPS* ps, double delta_time);
void ps_destroy(LWPS** ps);
LWEMITTER2OBJECT* ps_emit_object_begin(LWPS* ps);
LWEMITTER2OBJECT* ps_emit_object_next(LWPS* ps, LWEMITTER2OBJECT* cursor);
void ps_play_new_pos(LWPS* ps, const vec3 pos);
void ps_play_new(LWPS* ps);
