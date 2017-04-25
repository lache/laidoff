#pragma once

#include "linmath.h"

typedef struct _LWARMATURE {
	int count;
	mat4x4* mat;
} LWARMATURE;

int load_armature(const char* filename, LWARMATURE* ar);
void unload_armature(LWARMATURE* ar);
