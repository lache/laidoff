#pragma once
#include "lwgl.h"
#include "lwvbotype.h"

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTCP LWTCP;

typedef struct _LWFIELDMESH {
	LW_VBO_TYPE vbo;
	GLuint tex_id;
	int tex_mip;
} LWFIELDMESH;
#ifdef __cplusplus
extern "C" {;
#endif
const char* logic_server_addr(int idx);
void logic_update_default_projection(LWCONTEXT* pLwc);
void reset_runtime_context(LWCONTEXT* pLwc);
void reset_runtime_context_async(LWCONTEXT* pLwc);
void logic_start_logic_update_job(LWCONTEXT* pLwc);
void logic_stop_logic_update_job(LWCONTEXT* pLwc);
void logic_start_logic_update_job_async(LWCONTEXT* pLwc);
void logic_stop_logic_update_job_async(LWCONTEXT* pLwc);
void logic_emit_ui_event_async(LWCONTEXT* pLwc, const char* id, float w_ratio, float h_ratio);
void logic_emit_evalute_async(LWCONTEXT* pLwc, const char* code, size_t code_len);
void load_field_1_init_runtime_data(LWCONTEXT* pLwc);
void load_field_2_init_runtime_data(LWCONTEXT* pLwc);
void load_field_3_init_runtime_data(LWCONTEXT* pLwc);
void load_field_4_init_runtime_data(LWCONTEXT* pLwc);
void load_field_5_init_runtime_data(LWCONTEXT* pLwc);
void show_leaderboard(LWCONTEXT* pLwc);
void change_to_leaderboard(LWCONTEXT* pLwc);
void change_to_physics(LWCONTEXT* pLwc);
void request_player_reveal_leaderboard(LWTCP* tcp);
void request_leaderboard(LWTCP* tcp, int one_based_page);
void start_nickname_text_input_activity(LWCONTEXT* pLwc);
#ifdef __cplusplus
};
#endif
