#include "logic.h"
#include <czmq.h>
#include "lwcontext.h"
#include "lwtimepoint.h"
#include "lwlog.h"
#include "mq.h"
#include "script.h"
#include "net.h"
#include "dialog.h"
#include "battle.h"
#include "sysmsg.h"
#include "render_font_test.h"
#include "battle_result.h"
#include "font.h"
#include "file.h"
#include "ps.h"

void toggle_font_texture_test_mode(LWCONTEXT* pLwc);

static const char* server_addr[] = {
	"s.popsongremix.com", // AWS Tokyo
	"222.110.4.119", // private dev
};

static void reinit_mq(LWCONTEXT *pLwc);

void change_to_field(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_FIELD;
}

void change_to_dialog(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_DIALOG;
}

void change_to_battle(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_BATTLE;
}

void change_to_font_test(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_FONT_TEST;
}

void change_to_admin(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_ADMIN;
}

void change_to_battle_result(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_BATTLE_RESULT;
}

void change_to_skin(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_SKIN;
}

void change_to_physics(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_PHYSICS;
}

void change_to_particle_system(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_PARTICLE_SYSTEM;
}

typedef enum _LW_MSG {
	LM_ZERO,
	LM_LWMSGINITFIELD,
} LW_MSG;

typedef struct _LWMSGINITFIELD {
	LW_MSG type;
	const char* field_filename;
	const char* nav_filename;
	LW_VBO_TYPE vbo;
	GLuint tex_id;
	int tex_mip;
	float skin_scale;
	int follow_cam;
} LWMSGINITFIELD;

void load_field_1_init_runtime_data_async(LWCONTEXT *pLwc, zactor_t* actor) {
	pLwc->next_game_scene = LGS_FIELD;
	zmsg_t* msg = zmsg_new();
	LWMSGINITFIELD m = {
		LM_LWMSGINITFIELD,
		ASSETS_BASE_PATH "field" PATH_SEPARATOR "testfield.field",
		ASSETS_BASE_PATH "nav" PATH_SEPARATOR "apt.nav",
		LVT_APT,
		pLwc->tex_atlas[LAE_3D_APT_TEX_MIP_KTX],
		1,
		0.9f,
		0,
	};
	zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
	if (zactor_send(actor, &msg) < 0) {
		zmsg_destroy(&msg);
		LOGE("Send message to logic worker failed!");
	}
}

void load_field_1_init_runtime_data(LWCONTEXT *pLwc) {
	load_field_1_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

void load_field_2_init_runtime_data_async(LWCONTEXT *pLwc, zactor_t* actor) {
	pLwc->next_game_scene = LGS_FIELD;
	zmsg_t* msg = zmsg_new();
	LWMSGINITFIELD m = {
		LM_LWMSGINITFIELD,
		ASSETS_BASE_PATH "field" PATH_SEPARATOR "testfield.field",
		ASSETS_BASE_PATH "nav" PATH_SEPARATOR "test.nav",
		LVT_FLOOR,
		pLwc->tex_atlas[LAE_3D_FLOOR_TEX_KTX],
		0,
		0.5f,
		1,
	};
	zmsg_addmem(msg, &m, sizeof(LWMSGINITFIELD));
	if (zactor_send(actor, &msg) < 0) {
		zmsg_destroy(&msg);
		LOGE("Send message to logic worker failed!");
	}
}

void load_field_2_init_runtime_data(LWCONTEXT *pLwc) {
	load_field_2_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

static void reinit_mq(LWCONTEXT *pLwc) {

	//mq_interrupt();

	deinit_mq(pLwc->mq);

	pLwc->mq = init_mq(server_addr[pLwc->server_index], pLwc->def_sys_msg);
}

void connect_to_server_0(LWCONTEXT *pLwc) {
	pLwc->server_index = 0;

	reinit_mq(pLwc);
}

void connect_to_server_1(LWCONTEXT *pLwc) {
	pLwc->server_index = 1;

	reinit_mq(pLwc);
}

void reset_time(LWCONTEXT* pLwc) {
	pLwc->player_skin_time = 0;
}

void set_creature_data(LWBATTLECREATURE* c, const char* name, int lv, int hp, int max_hp, int mp, int max_mp,
	int turn_token, int exp, int max_exp) {
	c->valid = 1;
	//strcpy(c->name, name);
	c->lv = lv;
	c->hp = hp;
	c->max_hp = max_hp;
	c->mp = mp;
	c->max_mp = max_hp;
	c->selected_r = 0.5f;
	c->selected_g = 0.5f;
	c->selected_b = 0.5f;
	c->selected_a = 0.5f;
	c->turn_token = turn_token;
	c->exp = exp;
	c->max_exp = max_exp;
}

void reset_battle_context(LWCONTEXT* pLwc) {
	pLwc->battle_fov_deg_0 = 49.134f;
	pLwc->battle_fov_mag_deg_0 = 35.134f;
	pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
	pLwc->selected_enemy_slot = -1;

	const int enemy_count = LWMIN(pLwc->field_event_id, MAX_ENEMY_SLOT);
	// Fill enemy slots with test enemies
	for (int i = 0; i < enemy_count; i++) {
		pLwc->enemy[i] = ENEMY_DATA_LIST[i];

		for (int j = 0; j < MAX_SKILL_PER_CREATURE; j++) {
			pLwc->enemy[i].c.skill[j] = &SKILL_DATA_LIST[j];
		}
	}
	// Invalidate empty enemy slot
	for (int i = enemy_count; i < MAX_ENEMY_SLOT; i++) {
		pLwc->enemy[i].valid = 0;
	}

	memset(pLwc->player, 0, sizeof(pLwc->player));

	for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
		pLwc->player[i] = ENEMY_DATA_LIST[LET_TEST_PLAYER_1 + i].c;
		pLwc->player[i].valid = 0;
	}

	set_creature_data(
		&pLwc->player[0],
		u8"주인공",
		1,
		50,
		50,
		30,
		30,
		1,
		5,
		38
	);
	pLwc->player[0].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[0].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[0].skill[1] = &SKILL_DATA_LIST[1];
	pLwc->player[0].skill[2] = &SKILL_DATA_LIST[2];
	pLwc->player[0].skill[3] = &SKILL_DATA_LIST[3];
	pLwc->player[0].skill[4] = &SKILL_DATA_LIST[4];
	pLwc->player[0].skill[5] = &SKILL_DATA_LIST[5];

	set_creature_data(
		&pLwc->player[1],
		u8"극작가",
		2,
		25,
		50,
		30,
		30,
		2,
		1,
		30
	);
	pLwc->player[1].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[1].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[1].skill[1] = &SKILL_DATA_LIST[2];

	set_creature_data(
		&pLwc->player[2],
		u8"손톱깎이",
		3,
		46,
		46,
		15,
		55,
		3,
		0,
		50
	);
	pLwc->player[2].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[2].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[2].skill[1] = &SKILL_DATA_LIST[3];

	set_creature_data(
		&pLwc->player[3],
		u8"대문호",
		8,
		105,
		105,
		0,
		20,
		4,
		45,
		56
	);
	pLwc->player[3].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[3].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[3].skill[1] = &SKILL_DATA_LIST[4];

	for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
		pLwc->player[i].selected = 0;
	}

	pLwc->player[0].selected = 1;

	pLwc->player_turn_creature_index = 0;

	pLwc->battle_state = LBS_SELECT_COMMAND;

	pLwc->selected_command_slot = 0;
}

void reset_field_context(LWCONTEXT* pLwc) {

	despawn_all_field_object(pLwc);

	spawn_all_field_object(pLwc);

	/*
	spawn_field_object(pLwc, 0, 5, 2, 2, LVT_HOME, pLwc->tex_programmed[LPT_SOLID_GREEN], 1, 1, 1, 0);

	spawn_field_object(pLwc, 0, -7, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 6,
	1, 1, 0);
	*/

	set_field_player_position(pLwc->field, 0, 0, 10); // should fall from the sky (ray check...)

	pLwc->player_pos_x = 0;
	pLwc->player_pos_y = 0;
	//pLwc->player_aim_theta = (float)(M_PI / 8);
}

void toggle_ray_test(LWCONTEXT *pLwc) {
	pLwc->ray_test = !pLwc->ray_test;

	field_enable_ray_test(pLwc->field, pLwc->ray_test);
}

void toggle_network_poll(LWCONTEXT *pLwc) {
	field_set_network(pLwc->field, !field_network(pLwc->field));
}

void reset_runtime_context(LWCONTEXT* pLwc) {

	reset_time(pLwc);

	pLwc->sprite_data = SPRITE_DATA[0];

	pLwc->next_game_scene = LGS_FIELD;

	pLwc->font_texture_texture_mode = 0;

	reset_battle_context(pLwc);

	reset_field_context(pLwc);

	pLwc->font_fbo.dirty = 1;

	pLwc->admin_button_command[0].name = LWU("신:필드");
	pLwc->admin_button_command[0].command_handler = change_to_field;
	pLwc->admin_button_command[1].name = LWU("신:대화");
	pLwc->admin_button_command[1].command_handler = change_to_dialog;
	pLwc->admin_button_command[2].name = LWU("신:전투");
	pLwc->admin_button_command[2].command_handler = change_to_battle;
	pLwc->admin_button_command[3].name = LWU("신:글꼴");
	pLwc->admin_button_command[3].command_handler = change_to_font_test;
	pLwc->admin_button_command[4].name = LWU("런타임리셋");
	pLwc->admin_button_command[4].command_handler = reset_runtime_context;
	pLwc->admin_button_command[5].name = LWU("글꼴디버그");
	pLwc->admin_button_command[5].command_handler = toggle_font_texture_test_mode;
	pLwc->admin_button_command[6].name = LWU("UDP");
	pLwc->admin_button_command[6].command_handler = net_rtt_test;
	pLwc->admin_button_command[7].name = LWU("신:스킨");
	pLwc->admin_button_command[7].command_handler = change_to_skin;
	pLwc->admin_button_command[8].name = LWU("신:물리");
	pLwc->admin_button_command[8].command_handler = change_to_physics;
	pLwc->admin_button_command[9].name = LWU("신:파티클");
	pLwc->admin_button_command[9].command_handler = change_to_particle_system;
	pLwc->admin_button_command[10].name = LWU("신:필드1로드");
	pLwc->admin_button_command[10].command_handler = load_field_1_init_runtime_data;
	pLwc->admin_button_command[11].name = LWU("신:필드2로드");
	pLwc->admin_button_command[11].command_handler = load_field_2_init_runtime_data;
	pLwc->admin_button_command[12].name = LWU("Server #0");
	pLwc->admin_button_command[12].command_handler = connect_to_server_0;
	pLwc->admin_button_command[13].name = LWU("Server #1");
	pLwc->admin_button_command[13].command_handler = connect_to_server_1;
	pLwc->admin_button_command[14].name = LWU("레이테스트토글");
	pLwc->admin_button_command[14].command_handler = toggle_ray_test;
	pLwc->admin_button_command[15].name = LWU("네트워크토글");
	pLwc->admin_button_command[15].command_handler = toggle_network_poll;
}

static void update_battle_wall(LWCONTEXT* pLwc) {
	const float delta_time = (float)deltatime_delta_time(pLwc->update_dt);

	pLwc->battle_wall_tex_v += delta_time / 34;
	pLwc->battle_wall_tex_v = fmodf(pLwc->battle_wall_tex_v, 1.0f);
}

void logic_udate_default_projection(LWCONTEXT* pLwc) {
	float ratio = pLwc->width / (float)pLwc->height;

	//LOGV("Update(): width: %d height: %d ratio: %f", pLwc->width, pLwc->height, ratio);

	if (ratio > 1) {
		mat4x4_ortho(pLwc->proj, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	} else {
		mat4x4_ortho(pLwc->proj, -1.f, 1.f, -1 / ratio, 1 / ratio, 1.f, -1.f);
	}
}

void lwc_update(LWCONTEXT *pLwc, double delta_time) {

	deltatime_tick_delta(pLwc->update_dt, delta_time);

	//const float delta_time = (float)deltatime_delta_time(pLwc->update_dt);

	if (pLwc->next_game_scene == LGS_INVALID && pLwc->game_scene == LGS_INVALID) {
		// Default game scene
		pLwc->next_game_scene = LGS_FIELD;
	}

	if (pLwc->next_game_scene != LGS_INVALID && pLwc->next_game_scene != pLwc->game_scene) {

		pLwc->game_scene = pLwc->next_game_scene;
		pLwc->next_game_scene = LGS_INVALID;
		pLwc->scene_time = 0;

		if (pLwc->game_scene == LGS_BATTLE) {
			reset_battle_context(pLwc);
		}
	}

	// accumulate time since app startup
	pLwc->app_time += delta_time;
	pLwc->scene_time += delta_time;

	if (pLwc->field && field_network(pLwc->field)) {
		mq_poll(pLwc, pLwc->def_sys_msg, pLwc->mq, pLwc->field);
	}

	update_dialog(pLwc);

	//****//
	// fix delta time
	//lwcontext_delta_time(pLwc) = 1.0f / 60;
	//****//

	update_battle(pLwc);

	move_player(pLwc);

	resolve_player_collision(pLwc);

	update_attack_trail(pLwc);

	update_damage_text(pLwc);

	update_battle_wall(pLwc);

	update_sys_msg(pLwc->def_sys_msg, (float)delta_time);

	if (pLwc->battle_state == LBS_START_PLAYER_WIN || pLwc->battle_state == LBS_PLAYER_WIN_IN_PROGRESS) {
		update_battle_result(pLwc);
	}

	if (pLwc->game_scene == LGS_FIELD) {
		update_field(pLwc, pLwc->field);
	}

	if (pLwc->game_scene == LGS_PARTICLE_SYSTEM) {
		ps_test_update(pLwc);
		ps_update(pLwc->ps, lwcontext_delta_time(pLwc));
	}

	((LWCONTEXT *)pLwc)->update_count++;

	//LOGI("X");
}

void init_lwc_runtime_data(LWCONTEXT *pLwc) {
	init_lua(pLwc);

	reset_runtime_context(pLwc);

	//pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold.fnt");
	
	pLwc->dialog = create_string_from_file(ASSETS_BASE_PATH "d" PATH_SEPARATOR "d1.txt");
	if (pLwc->dialog) {
		pLwc->dialog_bytelen = (int)strlen(pLwc->dialog);
	} else {
		LOGE("dialog loading failed.");
	}
}

static int loop_logic_update(zloop_t* loop, int timer_id, void* args) {
	LWCONTEXT* pLwc = args;
	if (pLwc->quit_request) {
		//zloop_timer_end(loop, timer_id);
		// End the reactor
		return -1;
	}
	lwc_update(pLwc, pLwc->update_interval);
	return 0;
}

static int loop_pipe_reader(zloop_t* loop, zsock_t* pipe, void* args) {
	LWCONTEXT* pLwc = args;
	zmsg_t* msg = zmsg_recv(pipe);
	LOGI(LWLOGPOS "Message received through pipe");
	zframe_t* f = zmsg_first(msg);
	while (f) {
		byte* d = zframe_data(f);
		size_t s = zframe_size(f);
		if (d && s == sizeof(LWMSGINITFIELD) && *(int*)d == LM_LWMSGINITFIELD) {
			LWMSGINITFIELD* m = (LWMSGINITFIELD*)d;
			init_field(pLwc,
				m->field_filename,
				m->nav_filename,
				m->vbo,
				m->tex_id,
				m->tex_mip,
				m->skin_scale,
				m->follow_cam);

			init_lwc_runtime_data(pLwc);
		}
		f = zmsg_next(msg);
	}
	zmsg_destroy(&msg);
	return 0;
}

static void s_logic_worker(zsock_t *pipe, void *args) {
	LWCONTEXT* pLwc = args;
	// Send 'worker ready' signal to parent thread
	zsock_signal(pipe, 0);
	LWTIMEPOINT last_time;
	lwtimepoint_now(&last_time);
	double delta_time_accum = 0;
	pLwc->update_interval = 1 / 125.0;// 1 / 120.0;// 0.02; // seconds

	zloop_t* loop = zloop_new();
	zloop_timer(loop, (size_t)(pLwc->update_interval * 1000), 0, loop_logic_update, pLwc);
	zloop_reader(loop, pipe, loop_pipe_reader, pLwc);
	// Start the reactor loop
	zloop_start_noalloc(loop);
	// Reactor loop finished.
	// Send 'worker finished' signal to parent thread
	zsock_signal(pipe, 0);
}

void lwc_start_logic_thread(LWCONTEXT* pLwc) {
	// Start logic thread
	pLwc->logic_actor = zactor_new(s_logic_worker, pLwc);
	// Load initial stage
	load_field_2_init_runtime_data_async(pLwc, pLwc->logic_actor);
}

const char* logic_server_addr(int idx) {
	return server_addr[idx];
}

void toggle_font_texture_test_mode(LWCONTEXT *pLwc) {
	pLwc->font_texture_texture_mode = !pLwc->font_texture_texture_mode;
}
