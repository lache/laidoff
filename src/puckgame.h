#pragma once
#include "lwatlasenum.h"
#include "lwvbotype.h"
#include "armature.h"
#include "lwanim.h"
#include "puckgamepacket.h"
#include <ode/ode.h>

typedef struct _LWPUCKGAME LWPUCKGAME;

typedef enum _LW_PUCK_GAME_BOUNDARY {
	LPGB_GROUND, LPGB_E, LPGB_W, LPGB_S, LPGB_N,
	LPGB_COUNT
} LW_PUCK_GAME_BOUNDARY;

typedef enum _LW_PUCK_GAME_OBJECT {
	LPGO_PUCK,
	LPGO_PLAYER,
	LPGO_TARGET,
	LPGO_COUNT,
} LW_PUCK_GAME_OBJECT;

typedef struct _LWPUCKGAMEOBJECT {
	dGeomID geom;
	dBodyID body;
	float pos[3]; // read-only
	float move_rad; // angle on x-y plane
	float radius;
	mat4x4 rot;
	LWPUCKGAME* puck_game;
	int wall_hit_count;
	float speed;
	int red_overlay;
} LWPUCKGAMEOBJECT;

typedef struct _LWPUCKGAMEJUMP {
    float last_time;
    float remain_time;
    float shake_remain_time;
} LWPUCKGAMEJUMP;

typedef struct _LWPUCKGAMEDASH {
	float last_time;
	float remain_time;
	float dir_x;
	float dir_y;
	float shake_remain_time;
} LWPUCKGAMEDASH;

typedef struct _LWPUCKGAMEFIRE {
    float last_time;
    float remain_time;
    float dir_x;
    float dir_y;
    float dir_len;
    float shake_remain_time;
} LWPUCKGAMEFIRE;

typedef struct _LWPUCKGAMEPLAYER {
	int total_hp;
	int current_hp;
	dBodyID last_contact_puck_body;
	int puck_contacted;
	float hp_shake_remain_time;
} LWPUCKGAMEPLAYER;

typedef struct _LWREMOTEPLAYERCONTROL {
	int dir_pad_dragging;
	float dx;
	float dy;
    float dlen;
	int pull_puck;
} LWREMOTEPLAYERCONTROL;

typedef struct _LWPUCKGAMETOWER {
    dGeomID geom;
    int hp;
    int owner_player_no;
    double last_damaged_at;
    float shake_remain_time;
} LWPUCKGAMETOWER;

#define LW_PUCK_GAME_TOWER_COUNT (2)

typedef struct _LWPUCKGAME {
	// ---- Static game data begin
	float world_size;
	float dash_interval;
	float dash_duration;
	float dash_shake_time;
	float hp_shake_time;
    float jump_force;
    float jump_interval;
    float jump_shake_time;
	float puck_damage_contact_speed_threshold;
    float sphere_mass;
    float sphere_radius;
	float total_time;
    float fire_max_force;
    float fire_max_vel;
    float fire_interval;
    float fire_duration;
    float fire_shake_time;
    float tower_pos;
    float tower_radius;
    float tower_mesh_radius;
    int tower_total_hp;
    float tower_shake_time;
    float go_start_pos;
    int hp;
    float player_max_move_speed;
    float player_dash_speed;
    // ---- Static game data end
    float world_size_half;
    float tower_pos_multiplier[LW_PUCK_GAME_TOWER_COUNT][2];
	dWorldID world;
	dSpaceID space;
	dGeomID boundary[LPGB_COUNT];
    LWPUCKGAMETOWER tower[LW_PUCK_GAME_TOWER_COUNT];
	LWPUCKGAMEOBJECT go[LPGO_COUNT];
	dJointGroupID contact_joint_group;
	dJointGroupID  player_control_joint_group;
	dJointGroupID  target_control_joint_group;
	dJointGroupID  puck_pull_control_joint_group;
	dJointID player_control_joint; // player 1
	dJointID target_control_joint; // player 2
	dJointID puck_pull_control_joint;
	int push;
	float time;
	LWPUCKGAMEPLAYER player;
	LWPUCKGAMEPLAYER target;
	float last_remote_dx;
	float last_remote_dy;
	void(*on_player_damaged)(LWPUCKGAME*);
	void(*on_target_damaged)(LWPUCKGAME*);
	void* server;
	int battle_id;
	unsigned int token;
	unsigned int player_no;
	unsigned int c1_token;
	unsigned int c2_token;
	LWREMOTEPLAYERCONTROL remote_control[2];
	LWPUCKGAMEDASH remote_dash[2];
    LWPUCKGAMEJUMP remote_jump[2];
    LWPUCKGAMEFIRE remote_fire[2];
	int init_ready;
	int finished;
	int update_tick;
	char nickname[LW_NICKNAME_MAX_LEN];
	char target_nickname[LW_NICKNAME_MAX_LEN];
    unsigned int id1[4];
    unsigned int id2[4];
    int puck_owner_player_no;
    float puck_reflect_size;
    int world_roll_axis; // 0: x-axis, 1: y-axis, 2: z-axis
    int world_roll_dir; // 0: positive, 1: negative
    float world_roll;
    float world_roll_target;
    float world_roll_target_follow_ratio;
    int world_roll_dirty;
    void* pLwc; // opaque pointer to LWCONTEXT
    float target_dx;
    float target_dy;
    float target_dlen_ratio;
} LWPUCKGAME;

LWPUCKGAME* new_puck_game();
void delete_puck_game(LWPUCKGAME** puck_game);
void puck_game_push(LWPUCKGAME* puck_game);
float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game);
float puck_game_dash_cooltime(LWPUCKGAME* puck_game);
float puck_game_jump_cooltime(LWPUCKGAME* puck_game);
int puck_game_dashing(LWPUCKGAMEDASH* dash);
int puck_game_jumping(LWPUCKGAMEJUMP* jump);
void puck_game_near_callback(void *data, dGeomID o1, dGeomID o2);
void puck_game_commit_jump(LWPUCKGAME* puck_game, LWPUCKGAMEJUMP* jump, int player_no);
void puck_game_commit_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, float dx, float dy);
void puck_game_commit_dash_to_puck(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no);
void puck_game_player_decrease_hp_test(LWPUCKGAME* puck_game);
void puck_game_target_decrease_hp_test(LWPUCKGAME* puck_game);
float puck_game_remain_time(float total_time, int update_tick);
void puck_game_go_decrease_hp_test(LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* go, LWPUCKGAMEDASH* dash, LWPUCKGAMETOWER* tower);
void puck_game_commit_fire(LWPUCKGAME* puck_game, LWPUCKGAMEFIRE* fire, int player_no, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen);
void update_puck_ownership(LWPUCKGAME* puck_game);
void update_puck_reflect_size(LWPUCKGAME* puck_game, float delta_time);
void puck_game_reset(LWPUCKGAME* puck_game);
void puck_game_remote_state_reset(LWPUCKGAME* puck_game, LWPSTATE* state);
void puck_game_tower_pos(vec4 p_out, const LWPUCKGAME* puck_game, int owner_player_no);
void puck_game_control_bogus(LWPUCKGAME* puck_game);
void puck_game_update_remote_player(LWPUCKGAME* puck_game, float delta_time, int i);
LWPUCKGAMEDASH* puck_game_single_play_dash_object(LWPUCKGAME* puck_game);
LWPUCKGAMEJUMP* puck_game_single_play_jump_object(LWPUCKGAME* puck_game);
