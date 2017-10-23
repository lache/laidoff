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

	//pLwc->player_pos_last_moved_dx
	float dx, dy, dlen;
	if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * dx);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * dy);
		/*const float last_move_delta_len = sqrtf(pLwc->last_mouse_move_delta_x * pLwc->last_mouse_move_delta_x + pLwc->last_mouse_move_delta_y * pLwc->last_mouse_move_delta_y);
		dJointSetLMotorParam(pcj, dParamVel1, player_speed * pLwc->last_mouse_move_delta_x / last_move_delta_len);
		dJointSetLMotorParam(pcj, dParamVel2, player_speed * pLwc->last_mouse_move_delta_y / last_move_delta_len);*/

		LWPUCKGAMEPACKETMOVE packet_move;
		packet_move.type = LPGPT_MOVE;
		packet_move.dx = dx;
		packet_move.dy = dy;
		udp_send(pLwc->udp, (const char*)&packet_move, sizeof(packet_move));
	}
	else {
		dJointSetLMotorParam(pcj, dParamVel1, 0);
		dJointSetLMotorParam(pcj, dParamVel2, 0);

		LWPUCKGAMEPACKETSTOP packet_stop;
		packet_stop.type = LPGPT_STOP;
		udp_send(pLwc->udp, (const char*)&packet_stop, sizeof(packet_stop));
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
	puck_game->dash.remain_time = puck_game->dash_duration;
	puck_game->dash.dir_x = dx;
	puck_game->dash.dir_y = dy;
	puck_game->dash.last_time = puck_game->time;
}
