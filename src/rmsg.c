#include "rmsg.h"
#include "mq.h"
#include "zmq.h"
#include "lwcontext.h"
#include <string.h>

void rmsg_spawn(LWCONTEXT* pLwc, int key, int objtype, float x, float y, float z, float angle) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	memset(cmd, 0, sizeof(LWFIELDRENDERCOMMAND));
	cmd->cmdtype = LRCT_SPAWN;
	cmd->key = key;
	cmd->objtype = objtype;
	cmd->pos[0] = x;
	cmd->pos[1] = y;
	cmd->pos[2] = z;
	cmd->angle = angle;
	cmd->actionid = LWAC_RECOIL;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);
}

void rmsg_despawn(LWCONTEXT* pLwc, int key) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_DESPAWN;
	cmd->key = key;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);
}

void rmsg_pos(LWCONTEXT* pLwc, int key, float x, float y, float z) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_POS;
	cmd->key = key;
	cmd->pos[0] = x;
	cmd->pos[1] = y;
	cmd->pos[2] = z;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);	
}

void rmsg_turn(LWCONTEXT* pLwc, int key, float angle) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_TURN;
	cmd->key = key;
	cmd->angle = angle;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);
}

void rmsg_anim(LWCONTEXT* pLwc, int key, int actionid) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_ANIM;
	cmd->key = key;
	cmd->actionid = actionid;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);	
}

void rmsg_rparams(LWCONTEXT* pLwc, int key, int atlas, int skin_vbo, int armature) {
	zmq_msg_t rmsg;
	zmq_msg_init_size(&rmsg, sizeof(LWFIELDRENDERCOMMAND));
	LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
	cmd->cmdtype = LRCT_RPARAMS;
	cmd->key = key;
	cmd->atlas = atlas;
	cmd->skin_vbo = skin_vbo;
	cmd->armature = armature;
	zmq_msg_send(&rmsg, mq_rmsg_writer(lwcontext_mq(pLwc)), 0);
	zmq_msg_close(&rmsg);
}
