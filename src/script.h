#pragma once

void init_lua(LWCONTEXT* pLwc);
void spawn_all_field_object(LWCONTEXT* pLwc);
int script_run_file_ex(LWCONTEXT* pLwc, const char* filename, int pop_result);
void script_run_file(LWCONTEXT* pLwc, const char* filename);
void script_set_context(LWCONTEXT* pLwc);
LWCONTEXT* script_context();
const char* script_prefix_path();
void script_update(LWCONTEXT* pLwc);
void script_cleanup_all_coros(LWCONTEXT* pLwc);
int script_emit_anim_marker(LWCONTEXT* pLwc, int key, const char* name);
