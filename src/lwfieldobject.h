#pragma once

#include "lwgl.h"

typedef struct {
	int valid;
	float x, y;
	float sx, sy;
	enum _LW_VBO_TYPE lvt;
	GLuint tex_id;
} LWFIELDOBJECT;
