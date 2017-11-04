#pragma once

#include "lwcontext.h"
#include "lwvbotype.h"
#include "lwatlasenum.h"
#include "lwatlassprite.h"

#define MAX_COMMAND_SLOT (6)

#ifdef __cplusplus
extern "C" {;
#endif

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct GLFWwindow GLFWwindow;

LWCONTEXT* lw_init_initial_size(int width, int height);
LWCONTEXT* lw_init(void);
void lw_deinit(LWCONTEXT* pLwc);
void lwc_update(LWCONTEXT* pLwc, double delta_time);
void lw_set_size(LWCONTEXT* pLwc, int w, int h);
void lw_set_window(LWCONTEXT* pLwc, struct GLFWwindow* window);
GLFWwindow* lw_get_window(const LWCONTEXT* pLwc);
void lwc_prerender_mutable_context(LWCONTEXT* pLwc);
void lwc_render(const LWCONTEXT* pLwc);

void lw_trigger_touch(LWCONTEXT* pLwc, float xpos, float ypos, int pointer_id);
void lw_trigger_mouse_press(LWCONTEXT* pLwc, float xpos, float ypos, int pointer_id);
void lw_trigger_mouse_release(LWCONTEXT* pLwc, float xpos, float ypos, int pointer_id);
void lw_trigger_mouse_move(LWCONTEXT* pLwc, float x, float y, int pointer_id);
void lw_trigger_reset(LWCONTEXT* pLwc);
void lw_trigger_play_sound(LWCONTEXT* pLwc);
void lw_trigger_key_left(LWCONTEXT* pLwc);
void lw_trigger_key_right(LWCONTEXT* pLwc);
void lw_trigger_key_enter(LWCONTEXT* pLwc);

int lw_get_update_count(LWCONTEXT* pLwc);
int lw_get_render_count(LWCONTEXT* pLwc);

void lw_on_destroy(LWCONTEXT* pLwc);

void get_dir_pad_center(float aspect_ratio, float* x, float* y);

void change_to_field(LWCONTEXT* pLwc);
void change_to_dialog(LWCONTEXT* pLwc);
void change_to_battle(LWCONTEXT* pLwc);
void change_to_font_test(LWCONTEXT* pLwc);
void change_to_admin(LWCONTEXT* pLwc);
void change_to_battle_result(LWCONTEXT* pLwc);
void change_to_skin(LWCONTEXT* pLwc);
void change_to_physics(LWCONTEXT* pLwc);
void change_to_particle_system(LWCONTEXT* pLwc);
void connect_to_server_0(LWCONTEXT* pLwc);
void connect_to_server_1(LWCONTEXT* pLwc);

void lw_clear_color();


void reset_runtime_context(LWCONTEXT* pLwc);
void reset_time(LWCONTEXT* pLwc);
void lw_set_kp(LWCONTEXT* pLwc, int kp);

#ifndef __cplusplus
enum _LW_ATLAS_ENUM;
enum _LW_ATLAS_SPRITE;
#define LWENUM enum
#else
#define LWENUM
#endif

void set_tex_filter(int min_filter, int mag_filter);

void bind_all_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_vertex_attrib_font(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_skin_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_fan_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);
void bind_all_ps_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index);

void set_texture_parameter_values(const LWCONTEXT* pLwc, float x, float y, float w, float h, float atlas_w, float atlas_h, int shader_index);
int get_tex_index_by_hash_key(const LWCONTEXT* pLwc, const char* hash_key);
void set_texture_parameter(const LWCONTEXT* pLwc, LWENUM _LW_ATLAS_ENUM lae, LWENUM _LW_ATLAS_SPRITE las);

void lw_app_quit(LWCONTEXT* pLwc);
unsigned long hash(const unsigned char *str);
void reset_battle_context(LWCONTEXT* pLwc);
void lwc_start_logic_thread(LWCONTEXT* pLwc);
void delete_all_rmsgs(LWCONTEXT* pLwc);

extern const float default_uv_offset[2];
extern const float default_uv_scale[2];
extern const float default_flip_y_uv_scale[2];

#ifdef __cplusplus
};
#endif
