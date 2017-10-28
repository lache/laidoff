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
#include "spherebattlepacket.h"

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time) {
	if (!puck_game->world) {
		return;
	}
	puck_game->time += (float)delta_time;
	puck_game->player.puck_contacted = 0;
	dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
	//dWorldStep(puck_game->world, 0.005f);
	dWorldQuickStep(puck_game->world, 0.02f);
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
		LWSPHEREBATTLEPACKETGETTOKEN p;
		p.type = LSBPT_GETTOKEN;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
		break;
	}	
	case LUS_QUEUE:
	{
		LWSPHEREBATTLEPACKETQUEUE p;
		p.type = LSBPT_QUEUE;
		p.token = pLwc->udp->token;
		udp_send(pLwc->udp, (const char*)&p, sizeof(p));
		break;
	}	
	case LUS_MATCHED:
	{
		if (pLwc->udp->master) {
			LWPUCKGAMEPACKETSTATE p;
			p.type = LPGPT_STATE;
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
			LWPUCKGAMEPACKETMOVE packet_move;
			packet_move.type = LPGPT_MOVE;
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
			LWPUCKGAMEPACKETSTOP packet_stop;
			packet_stop.type = LPGPT_STOP;
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
	float dx, dy, dlen;
	if (!lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
		return;
	}
	// Check cooltime
	if (puck_game_dash_cooltime(puck_game) < puck_game->dash_interval) {
		puck_game->dash.shake_remain_time = puck_game->dash_shake_time;
		return;
	}
	// Start dash!
	puck_game_commit_dash(puck_game, &puck_game->dash, dx, dy);

	if (!pLwc->udp->master) {
		LWPUCKGAMEPACKETDASH packet_dash;
		packet_dash.type = LPGPT_DASH;
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