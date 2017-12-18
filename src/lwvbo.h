#pragma once

#include "lwgl.h"

typedef struct _LWVBO
{
	GLuint vertex_buffer;
	int vertex_count;
} LWVBO;

void lw_load_vbo(LWCONTEXT* pLwc, const char* filename, LWVBO* pVbo);
void lw_load_all_vbo(LWCONTEXT* pLwc);
