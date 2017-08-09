#pragma once
#include "lwgl.h"
#include "lwvbotype.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWFIELDMESH {
	LW_VBO_TYPE vbo;
	GLuint tex_id;
	int tex_mip;
} LWFIELDMESH;
#ifdef __cplusplus
extern "C" {;
#endif
const char* logic_server_addr(int idx);
void logic_udate_default_projection(LWCONTEXT* pLwc);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_runtime_context_async(LWCONTEXT *pLwc);
void logic_start_logic_update_job(LWCONTEXT* pLwc);
void logic_stop_logic_update_job(LWCONTEXT* pLwc);
void logic_start_logic_update_job_async(LWCONTEXT* pLwc);
void logic_stop_logic_update_job_async(LWCONTEXT* pLwc);
#ifdef __cplusplus
};
#endif
