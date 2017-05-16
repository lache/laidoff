#pragma once

void* init_mq();
void deinit_mq(void* _mq);
void mq_poll(void* pLwc, void* sm, void* _mq);
void init_czmq();
void mq_shutdown();

typedef struct _LWANIMACTION LWANIMACTION;

typedef struct _LWMQMSG {
	double t;				// Time

	float x;				// Current position X
	float y;				// Current position Y
	float z;				// Current position Z
	float dx;				// Last moved delta X
	float dy;				// Last moved delta y
	int moving;				// 1 if moving, 0 if stopped
	int attacking;			// 1 if attacking, 0 if stopped

} LWMQMSG;

typedef struct _LWPOSSYNCMSG {
	float x;					// Last position X (extrapolated)
	float y;					// Last position Y (extrapolated)
	float z;					// Last position Z (extrapolated)
	float a;					// Last orientation (extrapolated)
	void* extrapolator;			// Extrapolator for a remote entity
	int moving;					// 1 if moving, 0 if stopped
	int attacking;				// 1 if attacking, 0 if stopped
	const LWANIMACTION* action; // Last anim action
} LWPOSSYNCMSG;

const LWMQMSG* mq_sync_first(void* _mq);
const char* mq_sync_cursor(void* _mq);
const LWMQMSG* mq_sync_next(void* _mq);

LWPOSSYNCMSG* mq_possync_first(void* _mq);
const char* mq_possync_cursor(void* _mq);
LWPOSSYNCMSG* mq_possync_next(void* _mq);

const char* mq_uuid_str(void* _mq);
const char* mq_subtree(void* _mq);

void mq_publish_now(void* _mq);

double mq_sync_time(void* _mq);