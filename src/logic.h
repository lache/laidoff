#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

const char* logic_server_addr(int idx);
void logic_udate_default_projection(LWCONTEXT* pLwc);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_runtime_context_async(LWCONTEXT *pLwc);