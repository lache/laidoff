#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#endif
#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "input.h"
#include "lwudp.h"
#include "puckgamepacket.h"

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time) {
	if (!puck_game->world) {
		return;
	}
	if (!pLwc->udp) {
		return;
	}
	puck_game->remote = !pLwc->udp->master;
	puck_game->on_player_damaged = puck_game->remote ? 0 : puck_game_player_decrease_hp_test;
	puck_game->time += (float)delta_time;
	puck_game->player.puck_contacted = 0;
	dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
	//dWorldStep(puck_game->world, 0.005f);
	dWorldQuickStep(puck_game->world, 1.0f / 60);
	dJointGroupEmpty(puck_game->contact_joint_group);
	if (puck_game->player.puck_contacted == 0) {
		puck_game->player.last_contact_puck_body = 0;
	}
	const dReal* p = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);

	//LOGI("pos %.2f %.2f %.2f", p[0], p[1], p[2]);

	dJointID pcj = puck_game->player_control_joint;
	float player_speed = 0.5f;
	//dJointSetLMotorParam(pcj, dParamVel1, player_speed * (pLwc->player_move_right - pLwc->player_move_left));
	//dJointSetLMotorParam(pcj, dParamVel2, player_speed * (pLwc->player_move_up - pLwc->player_move_down));

	switch (pLwc->udp->state) {
	case LUS_INIT:
	case LUS_GETTOKEN:
	{
		LWPGETTOKEN p;
		p.type = LPGP_LWPGETTOKEN;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
		break;
	}
	case LUS_QUEUE:
	{
		LWPQUEUE p;
		p.type = LPGP_LWPQUEUE;
		p.token = pLwc->udp->token;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
		break;
	}	
	case LUS_MATCHED:
	{
		if (pLwc->udp->master) {
			LWPSTATE p;
			p.type = LPGP_LWPSTATE;
			p.token = pLwc->udp->token;
			p.puck[0] = puck_game->go[LPGO_PUCK].pos[0];
			p.puck[1] = puck_game->go[LPGO_PUCK].pos[1];
			p.puck[2] = puck_game->go[LPGO_PUCK].pos[2];
			p.player[0] = puck_game->go[LPGO_PLAYER].pos[0];
			p.player[1] = puck_game->go[LPGO_PLAYER].pos[1];
			p.player[2] = puck_game->go[LPGO_PLAYER].pos[2];
			p.target[0] = puck_game->go[LPGO_TARGET].pos[0];
			p.target[1] = puck_game->go[LPGO_TARGET].pos[1];
			p.target[2] = puck_game->go[LPGO_TARGET].pos[2];
			memcpy(p.puck_rot, puck_game->go[LPGO_PUCK].rot, sizeof(mat4x4));
			memcpy(p.player_rot, puck_game->go[LPGO_PLAYER].rot, sizeof(mat4x4));
			memcpy(p.target_rot, puck_game->go[LPGO_TARGET].rot, sizeof(mat4x4));
			udp_send(pLwc->udp, (const char*)&p, sizeof(p));
		}
		break;
	}
		break;
	}

	//pLwc->player_pos_last_moved_dx
	float dx, dy, dlen;
	if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * dx);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * dy);
		/*const float last_move_delta_len = sqrtf(pLwc->last_mouse_move_delta_x * pLwc->last_mouse_move_delta_x + pLwc->last_mouse_move_delta_y * pLwc->last_mouse_move_delta_y);
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * pLwc->last_mouse_move_delta_x / last_move_delta_len);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * pLwc->last_mouse_move_delta_y / last_move_delta_len);*/

		if (!pLwc->udp->master && pLwc->udp->state == LUS_MATCHED) {
			LWPMOVE packet_move;
			packet_move.type = LPGP_LWPMOVE;
			packet_move.token = pLwc->udp->token;
			packet_move.dx = dx;
			packet_move.dy = dy;
			udp_send(pLwc->udp, (const char*)&packet_move, sizeof(packet_move));
		}
	}
	else {
		dJointSetLMotorParam(pcj, dParamVel1, 0);
		dJointSetLMotorParam(pcj, dParamVel2, 0);

		if (!pLwc->udp->master && pLwc->udp->state == LUS_MATCHED) {
			LWPSTOP packet_stop;
			packet_stop.type = LPGP_LWPSTOP;
			packet_stop.token = pLwc->udp->token;
			udp_send(pLwc->udp, (const char*)&packet_stop, sizeof(packet_stop));
		}
		
	}

	// Move direction fixed while dashing
	if (puck_game->dash.remain_time > 0) {
		player_speed *= puck_game->dash_speed_ratio;
		dx = puck_game->dash.dir_x;
		dy = puck_game->dash.dir_y;
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * dx);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * dy);
		puck_game->dash.remain_time = LWMAX(0, puck_game->dash.remain_time - (float)delta_time);
	}
	// Decrease shake remain time
	if (puck_game->dash.shake_remain_time > 0) {
		puck_game->dash.shake_remain_time = LWMAX(0, puck_game->dash.shake_remain_time - (float)delta_time);
	}
	// Decrease HP remain time
	if (puck_game->player.hp_shake_remain_time > 0) {
		puck_game->player.hp_shake_remain_time = LWMAX(0, puck_game->player.hp_shake_remain_time - (float)delta_time);
	}

	if (puck_game->pull_puck) {
		const dReal* puck_pos = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);
		const dReal* player_pos = dBodyGetPosition(puck_game->go[LPGO_PLAYER].body);
		const dVector3 f = {
			player_pos[0] - puck_pos[0],
			player_pos[1] - puck_pos[1],
			player_pos[2] - puck_pos[2]
		};
		const dReal flen = dLENGTH(f);
		const dReal power = 0.1f;
		const dReal scale = power / flen;
		dBodyAddForce(puck_game->go[LPGO_PUCK].body, f[0] * scale, f[1] * scale, f[2] * scale);

		LWPPULLSTART p;
		p.type = LPGP_LWPPULLSTART;
		p.token = pLwc->udp->token;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
	}
	else {
		LWPPULLSTOP p;
		p.type = LPGP_LWPPULLSTOP;
		p.token = pLwc->udp->token;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
	}
}

void puck_game_dash(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
	// Check params
	if (!pLwc || !puck_game) {
		return;
	}
	// Check already effective dash
	if (puck_game_dashing(puck_game)) {
		return;
	}
	// Check effective move input
	//float dx, dy, dlen;
	/*if (!lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
		return;
	}*/

	// Check cooltime
	if (puck_game_dash_cooltime(puck_game) < puck_game->dash_interval) {
		puck_game->dash.shake_remain_time = puck_game->dash_shake_time;
		return;
	}

	// Start dash!
	puck_game_commit_dash_to_puck(puck_game, &puck_game->dash);
	//puck_game_commit_dash(puck_game, &puck_game->dash, dx, dy);

	if (!pLwc->udp->master) {
		LWPDASH packet_dash;
		packet_dash.type = LPGP_LWPDASH;
		packet_dash.token = pLwc->udp->token;
		udp_send(pLwc->udp, (const char*)&packet_dash, sizeof(packet_dash));
	}
}

void puck_game_target_move(LWPUCKGAME* puck_game, float dx, float dy) {
	dJointID tcj = puck_game->target_control_joint;
	float player_speed = 0.5f;
	dJointSetLMotorParam(tcj, dParamVel1, player_speed * dx);
	dJointSetLMotorParam(tcj, dParamVel2, player_speed * dy);
	puck_game->last_remote_dx = dx;
	puck_game->last_remote_dy = dy;
}

void puck_game_target_stop(LWPUCKGAME* puck_game) {
	dJointID tcj = puck_game->target_control_joint;
	float player_speed = 0.5f;
	dJointSetLMotorParam(tcj, dParamVel1, 0);
	dJointSetLMotorParam(tcj, dParamVel2, 0);
}

void puck_game_target_dash(LWPUCKGAME* puck_game) {
	puck_game_commit_dash(puck_game, &puck_game->remote_dash, puck_game->last_remote_dx, puck_game->last_remote_dy);
}

void puck_game_pull_puck_start(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
	puck_game->pull_puck = 1;
}

void puck_game_pull_puck_stop(LWCONTEXT* pLwc, LWPUCKGAME* puck_game) {
	puck_game->pull_puck = 0;
}
