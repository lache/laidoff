#include "mq.h"
#include "lwlog.h"
#include <czmq.h>
#include "sysmsg.h"
#include "kvmsg.h"
#include "field.h"
#include "lwcontext.h"

typedef enum _LW_MESSAGE_QUEUE_STATE {
	LMQS_INIT,
	LMQS_SNAPSHOT,
	LMQS_READY,
	LMQS_TERM,
} LW_MESSAGE_QUEUE_STATE;

typedef struct _LWMESSAGEQUEUE {
	zhash_t* kvmap;
	int64_t sequence;
	zsock_t* snapshot;
	zsock_t* subscriber;
	zsock_t* publisher;
	int port;
	char* subtree;
	LW_MESSAGE_QUEUE_STATE state;
	int64_t alarm;
	zuuid_t* uuid;
} LWMESSAGEQUEUE;

#define SERVER_ADDR "222.110.4.119"

void* init_mq() {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)malloc(sizeof(LWMESSAGEQUEUE));
	mq->state = LMQS_INIT;
	mq->subtree = "/l/";
	mq->port = 5556;
	// Prepare our context and subscriber
	mq->snapshot = zsock_new(ZMQ_DEALER);
	zsock_connect(mq->snapshot, "tcp://%s:%d", SERVER_ADDR, mq->port);
	mq->subscriber = zsock_new(ZMQ_SUB);
	zsock_set_subscribe(mq->subscriber, mq->subtree);
	zsock_connect(mq->subscriber, "tcp://%s:%d", SERVER_ADDR, mq->port + 1);
	mq->publisher = zsock_new(ZMQ_PUSH);
	zsock_connect(mq->publisher, "tcp://%s:%d", SERVER_ADDR, mq->port + 2);

	mq->kvmap = zhash_new();
	mq->sequence = 0;

	mq->uuid = zuuid_new();
	
	// We first request a state snapshot:
	zstr_sendm(mq->snapshot, "ICANHAZ?");
	zstr_send(mq->snapshot, mq->subtree);
	mq->state = LMQS_SNAPSHOT;
	return mq;
}

static void s_mq_poll_snapshot(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	zmq_pollitem_t items[] = { { zsock_resolve(mq->snapshot), 0, ZMQ_POLLIN, 0 } };
	int rc = zmq_poll(items, 1, 0);
	if (rc == -1) {
		return;
	}

	if (items[0].revents & ZMQ_POLLIN) {
		kvmsg_t* kvmsg = kvmsg_recv(zsock_resolve(mq->snapshot));
		if (!kvmsg) {
			return;
		}
		if (streq(kvmsg_key(kvmsg), "KTHXBAI")) {
			mq->sequence = kvmsg_sequence(kvmsg);
			LOGI("received snapshot=%"PRId64, mq->sequence);
			kvmsg_destroy(&kvmsg);
			LOGI("== Dump Sequence %"PRId64" ==", mq->sequence);
			mq->state = LMQS_READY;
			mq->alarm = zclock_time() + 1000;
			//s_zhash_dump(kvmap);
			return;
		}
		kvmsg_store(&kvmsg, mq->kvmap);
	}
}

static void s_send_pos(const LWCONTEXT* pLwc, LWMESSAGEQUEUE* mq) {
	kvmsg_t* kvmsg = kvmsg_new(0);
	kvmsg_fmt_key(kvmsg, "%s%s", mq->subtree, zuuid_str(mq->uuid));
	LWMQMSG msg;
	get_field_player_position(pLwc->field, &msg.x, &msg.y, &msg.z);
	msg.a = pLwc->player_rot_z;
	kvmsg_set_body(kvmsg, (byte*)&msg, sizeof(msg));
	kvmsg_set_prop(kvmsg, "ttl", "%d", 2);
	kvmsg_send(kvmsg, zsock_resolve(mq->publisher));
	kvmsg_destroy(&kvmsg);
}

static void s_mq_poll_ready(void* _pLwc, void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
	zmq_pollitem_t items[] = { { zsock_resolve(mq->subscriber), 0, ZMQ_POLLIN, 0 } };
	int rc = zmq_poll(items, 1, 0);
	if (rc == -1) {
		return;
	}
	// Polling subscriber socket to get update
	if (items[0].revents & ZMQ_POLLIN) {
		kvmsg_t* kvmsg = kvmsg_recv(zsock_resolve(mq->subscriber));
		if (!kvmsg) {
			return;
		}

		// Discard out-of-sequence kvmsgs, incl. heartbeats
		if (kvmsg_sequence(kvmsg) > mq->sequence) {
			mq->sequence = kvmsg_sequence(kvmsg);
			LOGI("Update: %"PRId64" key:%s, bodylen:%zd\n", mq->sequence, kvmsg_key(kvmsg), kvmsg_size(kvmsg));
			kvmsg_store(&kvmsg, mq->kvmap);

			LOGI("== Dump Sequence %"PRId64" ==\n", mq->sequence);
			//s_zhash_dump(kvmap);
		} else {
			kvmsg_destroy(&kvmsg);
		}
	}
	
	if (zclock_time() >= mq->alarm) {
		s_send_pos(pLwc, mq);
		mq->alarm = zclock_time() + 1000;
	}
}

const LWMQMSG* mq_sync_first(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	kvmsg_t* value = (kvmsg_t*)zhash_first(mq->kvmap);
	if (value) {
		return (const LWMQMSG*)kvmsg_body(value);
	}
	return 0;
}

const char* mq_sync_cursor(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	return zhash_cursor(mq->kvmap);
}

const LWMQMSG* mq_sync_next(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	kvmsg_t* value = (kvmsg_t*)zhash_next(mq->kvmap);
	if (value) {
		return (const LWMQMSG*)kvmsg_body(value);
	}
	return 0;
}

const char* mq_uuid_str(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	return zuuid_str(mq->uuid);
}

const char* mq_subtree(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	return mq->subtree;
}

void mq_poll(void* pLwc, void* sm, void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	switch (mq->state) {
	case LMQS_INIT:
		break;
	case LMQS_SNAPSHOT:
		s_mq_poll_snapshot(mq);
		break;
	case LMQS_READY:
		s_mq_poll_ready(pLwc, mq);
		break;
	default:
		break;
	}
}

void deinit_mq(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	zhash_destroy(&mq->kvmap);
	zsock_destroy(&mq->publisher);
	zsock_destroy(&mq->snapshot);
	zsock_destroy(&mq->subscriber);
	zuuid_destroy(&mq->uuid);
	mq->state = LMQS_TERM;
	free(mq);
}

void init_czmq() {
	zsock_t* frontend = zsock_new(ZMQ_REQ);
	zsock_destroy(&frontend);
}

void mq_shutdown() {
	zsys_shutdown();
}
