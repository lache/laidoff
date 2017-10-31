#pragma once

#include "lwgl.h"
#include "linmath.h"
#include "lwdamagetext.h"
#include "lwbattlecreature.h"
#include "lwenemy.h"
#include "lwshader.h"
#include "lwvbo.h"
#include "lwvbotype.h"
#include "lwmacro.h"
#include "lwatlasenum.h"
#include "lwbuttoncommand.h"
#include "lwprogrammedtex.h"
#include "lwbattlestate.h"
#include "lwgamescene.h"
#include "lwanim.h"
#include "lwtouchproc.h"
//#include "lwbox2dcollider.h"
//#include "lwfieldobject.h"
#include "lwfbo.h"
#include "lwtrail.h"
#include "armature.h"
#include "playersm.h"
#include "lwdeltatime.h"
#include "lwfieldrendercommand.h"
#include "lwbutton.h"
#include "construct.h"
#include "puckgamepacket.h"

#define MAX_RENDER_QUEUE_CAPACITY (512)

typedef enum _LW_SHADER_TYPE {
	LWST_DEFAULT,
	LWST_FONT,
	LWST_ETC1,
	LWST_SKIN,
	LWST_FAN,
	LWST_EMITTER,
	LWST_EMITTER2,
	LWST_COLOR,
	LWST_PANEL,
	LWST_SPHERE_REFLECT,

	LWST_COUNT,
} LW_SHADER_TYPE;

#define MAX_SHADER (LWST_COUNT)
#define MAX_ANIM_COUNT (10)
#define ANIM_FPS (60)
#define MAX_TOUCHPROC_COUNT (10)

extern const vec4 EXP_COLOR;
extern const char* tex_font_atlas_filename[2];

#define MAX_TEX_FONT_ATLAS (ARRAY_SIZE(tex_font_atlas_filename))


#define MAX_FIELD_OBJECT (128)
#define MAX_BOX_COLLIDER MAX_FIELD_OBJECT
#define MAX_TRAIL (16)
#define SET_COLOR_RGBA_FLOAT(v, r, g, b, a) \
	v[0] = r; \
	v[1] = g; \
	v[2] = b; \
	v[3] = a;
#define SET_COLOR_RGBA_FLOAT_ARRAY(v, rgba) \
	v[0] = rgba[0]; \
	v[1] = rgba[1]; \
	v[2] = rgba[2]; \
	v[3] = rgba[3];
#define MAX_DAMAGE_TEXT (16)
#define MAX_PLAYER_SLOT (4)
#define MAX_ENEMY_SLOT (5)
#define VERTEX_BUFFER_COUNT LVT_COUNT
#define SKIN_VERTEX_BUFFER_COUNT LSVT_COUNT
#define FAN_VERTEX_BUFFER_COUNT LFVT_COUNT
#define PS_VERTEX_BUFFER_COUNT LPVT_COUNT
#define MAX_DELTA_TIME_HISTORY (60)

typedef struct _LWPUCKGAME LWPUCKGAME;
typedef struct _LWUDP LWUDP;

typedef struct _LWCONTEXT {
	// Window instance
	struct GLFWwindow* window;
	// Screen width
	int width;
	// Screen height
	int height;
	// Shader array
	LWSHADER shader[MAX_SHADER];
	// General VBO
	LWVBO vertex_buffer[VERTEX_BUFFER_COUNT];
	// Skinned mesh VBO
	LWVBO skin_vertex_buffer[SKIN_VERTEX_BUFFER_COUNT];
	// Aim sector(fan) VBO
	LWVBO fan_vertex_buffer[FAN_VERTEX_BUFFER_COUNT];
	// General mesh VAO
	GLuint vao[VERTEX_BUFFER_COUNT];
	// Skinned mesh VAO
	GLuint skin_vao[SKIN_VERTEX_BUFFER_COUNT];
	// Aim sector(fan) VAO
	GLuint fan_vao[FAN_VERTEX_BUFFER_COUNT];
	// Particle system VAO
	GLuint ps_vao[PS_VERTEX_BUFFER_COUNT];
	// General texture atlas
	GLuint tex_atlas[MAX_TEX_ATLAS];
	// Width for text atlas array
	int tex_atlas_width[MAX_TEX_ATLAS];
	// Height for text atlas array
	int tex_atlas_height[MAX_TEX_ATLAS];
	// Font texture atlas
	GLuint tex_font_atlas[MAX_TEX_FONT_ATLAS];
	// Hashes for tex atlas resource
	unsigned long tex_atlas_hash[MAX_TEX_ATLAS];
	// Textures created from code during runtime
	GLuint tex_programmed[MAX_TEX_PROGRAMMED];
	// Delta time tracker for logic update loop
	LWDELTATIME* update_dt;
	// Delta time tracker for rendering loop
	LWDELTATIME* render_dt;
	// Default projection matrix for UI
	mat4x4 proj;
	// Current scene
	LW_GAME_SCENE game_scene;
	// The scene going to be visible at the next tick
	LW_GAME_SCENE next_game_scene;
	// Logic updated count since app started
	int update_count;
	// Rendered count since app started
	int render_count;
	// last keypad number (debug)
	int kp;
	// Elapsed time since app started
	double app_time;
	// Elapsed time since enter the current scene
	double scene_time;
	// ???
	int tex_atlas_index;
	// ???
	LWSPRITE* sprite_data;
	// Font instance
	void* pFnt;
	// Dialog text data
	char* dialog;
	// Total dialog text data's length in bytes
	int dialog_bytelen;
	// Current page's current character index of dialog text
	int render_char;
	// Last time when the last character of dialog text is rendered
	double last_render_char_incr_time;
	// Current page's start index of dialog text
	int dialog_start_index;
	// 1 if next dialog input received, 0 if otherwise
	int dialog_move_next;
	// Dialog background image index
	int dialog_bg_tex_index;
	// Dialog portrait image index
	int dialog_portrait_tex_index;
	// 1 if player move left key is down, 0 if otherwise
	int player_move_left;
	// 1 if player move right key is down, 0 if otherwise
	int player_move_right;
	// 1 if player move up key is down, 0 if otherwise
	int player_move_up;
	// 1 if player move down key is down, 0 if otherwise
	int player_move_down;
	// 1 if player space key is down, 0 if otherwise
	int player_space;
	// Current player x coordinate
	float player_pos_x;
	// Current player y coordinate
	float player_pos_y;
	// Last valid player x coordinate move delta (value when the dir pad is released)
	float player_pos_last_moved_dx;
	// Last valid player y coordinate move delta (value when the dir pad is released)
	float player_pos_last_moved_dy;
	// Current player action
	const LWANIMACTION* player_action;
	//float player_aim_theta;
	LWPLAYERSTATEDATA player_state_data;
	// Current Dir pad x coordinate (screen coordinate)
	float dir_pad_x;
	// Current Dir pad y coordinate (screen coordinate)
	float dir_pad_y;
	// 1 if dir pad (left button) is dragged, 0 if otherwise
	int dir_pad_dragging;
	// dir_pad_dragging pointer index
	int dir_pad_pointer_id;
	// 1 if attack pad (right button) is dragged, 0 if otherwise
	int atk_pad_dragging;
	// Current field event ID
	int field_event_id;
	// Next field event ID
	int next_field_event_id;
	// Battle: Normal attack effect instance array
	LWTRAIL trail[MAX_TRAIL];
	// Battle: Current FOV
	float battle_fov_deg;
	// Battle: FOV when normal state
	float battle_fov_deg_0;
	// Battle: FOV when camera is zoomed(magnified)
	float battle_fov_mag_deg_0;
	// Battle: Camera zooming x coordinate
	float battle_cam_center_x;
	// Battle: Current command slot index
	int selected_command_slot;
	// Battle: Current enemy slot index
	int selected_enemy_slot;
	// Battle: State
	LW_BATTLE_STATE battle_state;
	// Battle: Command banner
	LWANIM1D command_banner_anim;
	// Battle: Damage text instance array
	LWDAMAGETEXT damage_text[MAX_DAMAGE_TEXT];
	// Battle: Player slots
	LWBATTLECREATURE player[MAX_PLAYER_SLOT];
	// Battle: Enemy slots
	LWENEMY enemy[MAX_ENEMY_SLOT];
	// Battle: Current player turn
	int player_turn_creature_index;
	// Battle: Current enemy turn
	int enemy_turn_creature_index;
	// Battle: Delay before executing the enemy action (mimics thinking time of human)
	float enemy_turn_command_wait_time;
	// Battle: Wall UV animation (v coordinate)
	float battle_wall_tex_v;
	// Battle: Projection amtrix
	mat4x4 battle_proj;
	// Battle: View matrix
	mat4x4 battle_view;
	// Battle: Turn change UI animator
	LWANIM1D center_image_anim;
	// Battle: Turn change UI image
	LW_ATLAS_ENUM center_image;
	// Font: Render font debug UI
	int font_texture_texture_mode;
	// Test font FBO
	LWFBO font_fbo;
	// Input: Last mouse press x coordinate
	float last_mouse_press_x;
	// Input: Last mouse press y coordinate
	float last_mouse_press_y;
	// Input: Last mouse move x coordinate
	float last_mouse_move_x;
	// Input: Last mouse move y coordinate
	float last_mouse_move_y;
	// Input: Last mouse move delta x
	float last_mouse_move_delta_x;
	// Input: Last mouse move delta y
	float last_mouse_move_delta_y;
	// Admin button command handler array
	LWBUTTONCOMMAND admin_button_command[6 * 5];
	// lua VM instance
	void* L;
	void* script;
	// Skin: Armature data
	LWARMATURE armature[LWAR_COUNT];
	// Skin: Anim action data
	LWANIMACTION action[LWAC_COUNT];
	// Abstract time for player animation
	float player_skin_time;
	// Abstract time for test player animation
	float test_player_skin_time;
	// 1 if player animation action is looped, 0 if otherwise
	int player_action_loop;
	// Field instance
	struct _LWFIELD* field;
	// 1 if FPS camera mode enabled, 0 if otherwise
	int fps_mode;
	// 1 if field mesh hidden, 0 if otherwise
	int hide_field;
	// Default system message instance
	void* def_sys_msg;
	// Message queue instance
	void* mq;
	// Server index for online play
	int server_index;
	// 1 if aim sector ray test is executed for every logic frame, 0 if ray test disabled
	int ray_test;
	// 1 if application quit is requested
	int quit_request;
	// CZMQ actor for logic thread
	void* logic_actor;
	// CZMQ actor for script watch thread (win32 dev only)
	void* scriptwatch_actor;
	// Particle system buffer (rose)
	GLuint particle_buffer;
	// Particle system buffer (explosion)
	GLuint particle_buffer2;
	// Logic update interval (in seconds)
	double update_interval;
	// 1 if safe to render, 0 if otherwise
	/*volatile*/ int safe_to_start_render;
	// 1 if rendering in progress, 0 if otherwise
	/*volatile*/ int rendering;
	// Asynchronous render command queue
	LWFIELDRENDERCOMMAND render_command[MAX_RENDER_QUEUE_CAPACITY];
	// Render command message send count
	int rmsg_send_count;
	// Render command message recv(handle) count
	int rmsg_recv_count;
	// ZeroMQ logic loop
	void* logic_loop;
	// ZeroMQ logic update timer job
	int logic_update_job;
	// Last now (seconds)
	double last_now;
	// Button list
	LWBUTTONLIST button_list;
	// Construct
	LWCONSTRUCT construct;
	// Puck game sub-context
	LWPUCKGAME* puck_game;
	// UDP context
	LWUDP* udp;
	// Puck game remote(server) state
	LWPUCKGAMEPACKETSTATE puck_game_state;
} LWCONTEXT;

#ifdef __cplusplus
extern "C" {;
#endif
double lwcontext_delta_time(const LWCONTEXT* pLwc);
int lwcontext_safe_to_start_render(const LWCONTEXT* pLwc);
void lwcontext_set_safe_to_start_render(LWCONTEXT* pLwc, int v);
int lwcontext_rendering(const LWCONTEXT* pLwc);
void lwcontext_set_rendering(LWCONTEXT* pLwc, int v);
void* lwcontext_mq(LWCONTEXT* pLwc);
LWFIELD* lwcontext_field(LWCONTEXT* pLwc);
void lwcontext_inc_rmsg_send(LWCONTEXT* pLwc);
void lwcontext_inc_rmsg_recv(LWCONTEXT* pLwc);
#ifdef __cplusplus
};
#endif
