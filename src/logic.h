#pragma once
#include "lwgl.h"
#include "lwvbotype.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWFIELDMESH {
	LW_VBO_TYPE vbo;
	GLuint tex_id;
	int tex_mip;
} LWFIELDMESH;

const char* logic_server_addr(int idx);
void logic_udate_default_projection(LWCONTEXT* pLwc);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_runtime_context_async(LWCONTEXT *pLwc);
