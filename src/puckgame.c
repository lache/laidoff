#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
//#include "lwcontext.h"
//#include "input.h"

static void testgo_move_callback(dBodyID b) {
	LWPUCKGAMEOBJECT* go = (LWPUCKGAMEOBJECT*)dBodyGetData(b);
	LWPUCKGAME* puck_game = go->puck_game;

	// Position
	const dReal* p = dBodyGetPosition(b);
	go->pos[0] = (float)p[0];
	go->pos[1] = (float)p[1];
	go->pos[2] = (float)p[2];
	// Orientation
	const dReal* r = dBodyGetRotation(b); // dMatrix3
	mat4x4 rot;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			rot[i][j] = (float)r[4*i + j];
		}
	}
	rot[3][0] = 0;
	rot[3][1] = 0;
	rot[3][2] = 0;
	rot[3][3] = 1;

	mat4x4_transpose(go->rot, rot);
	const dReal* vel = dBodyGetLinearVel(go->body);
	go->speed = (float)dLENGTH(vel);
	if (vel[0]) {
		go->move_rad = atan2f((float)vel[1], (float)vel[0]) + (float)M_PI / 2;
	}
}

static void create_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo, float mass, float radius, float x, float y) {
	LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
	go->puck_game = puck_game;
	go->radius = radius;
	const float testgo_radius = go->radius;
	const float initial_height = testgo_radius * 1;
	go->geom = dCreateSphere(puck_game->space, testgo_radius);
	go->body = dBodyCreate(puck_game->world);
	dMass m;
	dMassSetSphereTotal(&m, mass, testgo_radius);
	dBodySetMass(go->body, &m);
	dBodySetPosition(go->body, x, y, initial_height);
	dBodySetData(go->body, go);
	dBodySetMovedCallback(go->body, testgo_move_callback);
	//dBodySetLinearVel(go->body, -2.0f, 3.0f, 0);
	dGeomSetBody(go->geom, go->body);
	//dBodySetDamping(go->body, 1e-2f, 1e-2f);
	//dBodySetAutoDisableLinearThreshold(go->body, 0.05f);
}

LWPUCKGAME* new_puck_game() {
	// Static game data
	LWPUCKGAME* puck_game = malloc(sizeof(LWPUCKGAME));
	memset(puck_game, 0, sizeof(LWPUCKGAME));
	const int hp = 10;
	puck_game->world_size = 4.0f;
	puck_game->world_size_half = puck_game->world_size / 2.0f;
	puck_game->dash_interval = 1.5f;
	puck_game->dash_duration = 0.1f;
	puck_game->dash_speed_ratio = 8.0f;
	puck_game->dash_shake_time = 0.3f;
	puck_game->hp_shake_time = 0.3f;
	puck_game->puck_damage_contact_speed_threshold = 1.1f;
	puck_game->player.total_hp = hp;
	puck_game->player.current_hp = hp;
	puck_game->target.total_hp = hp;
	puck_game->target.current_hp = hp;
    puck_game->sphere_mass = 0.1f;
    puck_game->sphere_radius = 0.16f; //0.125f;
	// ------

	// Initialize OpenDE
	dInitODE2(0);
	puck_game->world = dWorldCreate();
	puck_game->space = dHashSpaceCreate(0);
	puck_game->boundary[LPGB_GROUND] = dCreatePlane(puck_game->space, 0, 0, 1, 0);
	puck_game->boundary[LPGB_E] = dCreatePlane(puck_game->space, -1, 0, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_W] = dCreatePlane(puck_game->space, 1, 0, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_S] = dCreatePlane(puck_game->space, 0, -1, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_N] = dCreatePlane(puck_game->space, 0, 1, 0, -puck_game->world_size_half);
	dWorldSetGravity(puck_game->world, 0, 0, -9.81f);
	dWorldSetCFM(puck_game->world, 1e-5f);
	
	create_go(puck_game, LPGO_PUCK, puck_game->sphere_mass, puck_game->sphere_radius, 0.0f, 0.0f);
	create_go(puck_game, LPGO_PLAYER, puck_game->sphere_mass, puck_game->sphere_radius, -1.0f, 0.0f);
	create_go(puck_game, LPGO_TARGET, puck_game->sphere_mass, puck_game->sphere_radius, 1.0f, 0.0f);

	puck_game->contact_joint_group = dJointGroupCreate(0);
	puck_game->player_control_joint_group = dJointGroupCreate(0);

	// Create target control joint
	puck_game->target_control_joint = dJointCreateLMotor(puck_game->world, puck_game->target_control_joint_group);
	dJointID tcj = puck_game->target_control_joint;
	dJointSetLMotorNumAxes(tcj, 2);
	dJointSetLMotorAxis(tcj, 0, 0, 1, 0, 0); // x-axis actuator
	dJointSetLMotorAxis(tcj, 1, 0, 0, 1, 0); // y-axis actuator
	dJointAttach(tcj, puck_game->go[LPGO_TARGET].body, 0);
	dJointSetLMotorParam(tcj, dParamFMax1, 10.0f);
	dJointSetLMotorParam(tcj, dParamFMax2, 10.0f);

	// Create player control joint
	puck_game->player_control_joint = dJointCreateLMotor(puck_game->world, puck_game->player_control_joint_group);
	dJointID pcj = puck_game->player_control_joint;
	dJointSetLMotorNumAxes(pcj, 2);
	dJointSetLMotorAxis(pcj, 0, 0, 1, 0, 0); // x-axis actuator
	dJointSetLMotorAxis(pcj, 1, 0, 0, 1, 0); // y-axis actuator
	dJointAttach(pcj, puck_game->go[LPGO_PLAYER].body, 0);
	dJointSetLMotorParam(pcj, dParamFMax1, 10.0f);
	dJointSetLMotorParam(pcj, dParamFMax2, 10.0f);

	//dBodySetPosition(puck_game->go[LPGO_TARGET].body, 0.0f, 0.0f, puck_game->go[LPGO_TARGET].radius);

	// Create puck pull control joint
	/*puck_game->puck_pull_control_joint = dJointCreateSlider(puck_game->world, puck_game->puck_pull_control_joint_group);
	dJointID ppcj = puck_game->puck_pull_control_joint;
	const dReal* player_pos = dBodyGetPosition(puck_game->go[LPGO_PLAYER].body);
	const dReal* puck_pos = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);
	dJointSetSliderAxis(ppcj,
		puck_pos[0] - player_pos[0],
		puck_pos[1] - player_pos[1],
		puck_pos[2] - player_pos[2]
	);
	dJointAttach(ppcj, puck_game->go[LPGO_PLAYER].body, puck_game->go[LPGO_PUCK].body);*/
	//dJointSetSliderParam(ppcj, dParamFMax, 10.0f);

	// Create target pull control joint


	
	
	//dBodySetKinematic(puck_game->go[LPGO_PUCK].body);

	puck_game->go[LPGO_PUCK].red_overlay = 1;
	puck_game->init_ready = 1;
	return puck_game;
}

void delete_puck_game(LWPUCKGAME** puck_game) {
	for (int i = 0; i < LPGB_COUNT; i++) {
		dGeomDestroy((*puck_game)->boundary[i]);
	}
	dJointGroupDestroy((*puck_game)->player_control_joint_group);
	dJointGroupDestroy((*puck_game)->contact_joint_group);
	dSpaceDestroy((*puck_game)->space);
	dWorldDestroy((*puck_game)->world);

	free(*puck_game);
	*puck_game = 0;
}

void puck_game_go_decrease_hp_test(LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* go, LWPUCKGAMEDASH* dash) {
	LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
	const float puck_speed = (float)dLENGTH(dBodyGetLinearVel(puck->body));

	if (go->last_contact_puck_body != puck->body
		&& puck_speed > puck_game->puck_damage_contact_speed_threshold
		&& !puck_game_dashing(dash)) {
		// Decrease player hp
		go->last_contact_puck_body = puck->body;
		go->current_hp--;
		if (go->current_hp < 0) {
			//go->current_hp = go->total_hp;
		}
		go->hp_shake_remain_time = puck_game->hp_shake_time;
	}
}

void puck_game_player_decrease_hp_test(LWPUCKGAME* puck_game) {
	puck_game_go_decrease_hp_test(puck_game, &puck_game->player, &puck_game->remote_dash[0]);
}

void puck_game_target_decrease_hp_test(LWPUCKGAME* puck_game) {
	puck_game_go_decrease_hp_test(puck_game, &puck_game->target, &puck_game->remote_dash[1]);
}

static void near_puck_player(LWPUCKGAME* puck_game) {
	LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
	LWPUCKGAMEOBJECT* player = &puck_game->go[LPGO_PLAYER];
	if (dBodyIsKinematic(puck->body))
	{
		dBodySetDynamic(puck->body);

		const float dx = puck->pos[0] - player->pos[0];
		const float dy = puck->pos[1] - player->pos[1];
		const float fscale = 50.0f;
		dBodyAddForce(puck->body, dx*fscale, dy*fscale, 0);
	}

	puck_game->player.puck_contacted = 1;

	if (puck_game->on_player_damaged) {
		puck_game->on_player_damaged(puck_game);
	}

	const float puck_speed = (float)dLENGTH(dBodyGetLinearVel(puck->body));
	//LOGI("Contact puck velocity: %.2f", puck_speed);
}

static void near_puck_target(LWPUCKGAME* puck_game) {
	LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
	LWPUCKGAMEOBJECT* target = &puck_game->go[LPGO_TARGET];
	puck_game->target.puck_contacted = 1;
	if (puck_game->on_target_damaged) {
		puck_game->on_target_damaged(puck_game);
	}

	const float puck_speed = (float)dLENGTH(dBodyGetLinearVel(puck->body));
	//LOGI("Contact puck velocity: %.2f", puck_speed);
}

void puck_game_near_callback(void *data, dGeomID o1, dGeomID o2)
{
	LWPUCKGAME* puck_game = (LWPUCKGAME*)data;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {

		// colliding a space with something :
		dSpaceCollide2(o1, o2, data, &puck_game_near_callback);

		// collide all geoms internal to the space(s)
		if (dGeomIsSpace(o1))
			dSpaceCollide((dSpaceID)o1, data, &puck_game_near_callback);
		if (dGeomIsSpace(o2))
			dSpaceCollide((dSpaceID)o2, data, &puck_game_near_callback);

	} else {
		const int max_contacts = 5;
		dContact contact[5];
		// colliding two non-space geoms, so generate contact
		// points between o1 and o2
		int num_contact = dCollide(o1, o2, max_contacts, &contact->geom, sizeof(dContact));
		// add these contact points to the simulation ...
		for (int i = 0; i < num_contact; i++) {
			// All objects - ground contacts
			if (o1 == puck_game->boundary[LPGB_GROUND] || o2 == puck_game->boundary[LPGB_GROUND]) {
				contact[i].surface.mode = dContactSoftCFM | dContactRolling;// | dContactFDir1;
				contact[i].surface.rho = 0.001f;
				contact[i].surface.rho2 = 0.001f;
				contact[i].surface.rhoN = 0.001f;
				//contact[i].fdir1[0] = 1.0f;
				//contact[i].fdir1[1] = 0.0f;
				//contact[i].fdir1[2] = 0.0f;
				contact[i].surface.mu = 100.9f;
			}
			// Player - puck contacts
			else if (o1 == puck_game->go[LPGO_PUCK].geom && o2 == puck_game->go[LPGO_PLAYER].geom)
			{
				contact[i].surface.mode = dContactSoftCFM | dContactBounce;
				contact[i].surface.mu = 1.9f;

				near_puck_player(puck_game);
			}
			else if (o1 == puck_game->go[LPGO_PLAYER].geom && o2 == puck_game->go[LPGO_PUCK].geom)
			{
				contact[i].surface.mode = dContactSoftCFM | dContactBounce;
				contact[i].surface.mu = 1.9f;

				near_puck_player(puck_game);
			}
			// others contacts
			else {
				contact[i].surface.mode = dContactSoftCFM | dContactBounce;
				contact[i].surface.mu = 1.9f;
			}

			if ((o1 == puck_game->go[LPGO_PUCK].geom && o2 == puck_game->go[LPGO_TARGET].geom)
				|| (o1 == puck_game->go[LPGO_PUCK].geom && o2 == puck_game->go[LPGO_TARGET].geom)) {
				near_puck_target(puck_game);
			}
			

			// bounce is the amount of "bouncyness".
			contact[i].surface.bounce = 0.9f;
			// bounce_vel is the minimum incoming velocity to cause a bounce
			contact[i].surface.bounce_vel = 0.1f;
			// constraint force mixing parameter
			contact[i].surface.soft_cfm = 0.001f;
			dJointID c = dJointCreateContact(puck_game->world, puck_game->contact_joint_group, &contact[i]);
			dBodyID b1 = dGeomGetBody(o1);
			dBodyID b2 = dGeomGetBody(o2);

			dJointAttach(c, b1, b2);
		}

	}
}

void puck_game_push(LWPUCKGAME* puck_game) {
	//puck_game->push = 1;

	dBodySetLinearVel(puck_game->go[LPGO_PUCK].body, 1, 1, 0);
}

float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game) {
	return LWMIN(1.0f, puck_game_dash_cooltime(puck_game) / puck_game->dash_interval);
}

float puck_game_dash_cooltime(LWPUCKGAME* puck_game) {
	return puck_game->time - puck_game->dash.last_time;
}

int puck_game_dashing(LWPUCKGAMEDASH* dash) {
	return dash->remain_time > 0;
}

void puck_game_commit_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, float dx, float dy) {
	dash->remain_time = puck_game->dash_duration;
	dash->dir_x = dx;
	dash->dir_y = dy;
	dash->last_time = puck_game->time;
}

void puck_game_commit_dash_to_puck(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no) {
	float dx = puck_game->go[LPGO_PUCK].pos[0] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[0];
	float dy = puck_game->go[LPGO_PUCK].pos[1] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[1];
	const float ddlen = sqrtf(dx * dx + dy * dy);
	dx /= ddlen;
	dy /= ddlen;
	puck_game_commit_dash(puck_game, dash, dx, dy);
}

float puck_game_remain_time(int update_tick) {
	return ceilf(LWMAX(0, 80.0f - update_tick * 1.0f / 125));
}
