#pragma once

void init_lua(LWCONTEXT* pLwc);
void spawn_all_field_object(LWCONTEXT* pLwc);
void despawn_all_field_object(LWCONTEXT* pLwc);
int script_run_file_ex(LWCONTEXT* pLwc, const char* filename, int pop_result);
void script_run_file(LWCONTEXT* pLwc, const char* filename);
void script_set_context(LWCONTEXT* pLwc);
LWCONTEXT* script_context();
const char* script_prefix_path();
