#pragma once

#include "linmath.h"
#include "lwmacro.h"

typedef struct _LWARMATURE {
	int count;
	mat4x4* mat;
	int* parent_index;
} LWARMATURE;

typedef enum _LW_ARMATURE {
	LWAR_ARMATURE,
	LWAR_TREEARMATURE,

	LWAR_COUNT,
} LW_ARMATURE;

static const char* armature_filename[] = {
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "Armature.arm",
	ASSETS_BASE_PATH "armature" PATH_SEPARATOR "TreeArmature.arm",
};

int load_armature(const char* filename, LWARMATURE* ar);
void unload_armature(LWARMATURE* ar);


