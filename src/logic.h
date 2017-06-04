#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

const char* logic_server_addr(int idx);
void logic_udate_default_projection(LWCONTEXT* pLwc);
