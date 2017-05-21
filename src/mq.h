#pragma once

void* init_mq(const char* addr, void* sm);
void deinit_mq(void* _mq);
void mq_poll(void* pLwc, void* sm, void* _mq);
void mq_shutdown();

typedef struct _LWANIMACTION LWANIMACTION;

typedef struct _LWMQMSG {
	float x;				// Current position X
	float y;				// Current position Y
	float z;				// Current position Z
	float dx;				// Last moved delta X
	float dy;				// Last moved delta y
	//int moving;				// 1 if moving, 0 if stopped
	//int attacking;			// 1 if attacking, 0 if stopped
	int stop;				// 1 if movement stopped, 0 if not
	int action;				// LW_ACTION enum
	double t;				// Time
} LWMQMSG;

typedef struct _LWPOSSYNCMSG {
	float x;							// Last position X (extrapolated)
	float y;							// Last position Y (extrapolated)
	float z;							// Last position Z (extrapolated)
	float a;							// Last orientation (extrapolated)
	void* extrapolator;					// Extrapolator for a remote entity
	//int moving;						// 1 if moving, 0 if stopped
	//int attacking;					// 1 if attacking, 0 if stopped
	int action;							// LW_ACTION enum
	const LWANIMACTION* anim_action;	// Last anim action
} LWPOSSYNCMSG;

const LWMQMSG* mq_sync_first(void* _mq);
const char* mq_sync_cursor(void* _mq);
const LWMQMSG* mq_sync_next(void* _mq);

LWPOSSYNCMSG* mq_possync_first(void* _mq);
const char* mq_possync_cursor(void* _mq);
LWPOSSYNCMSG* mq_possync_next(void* _mq);

const char* mq_uuid_str(void* _mq);
const char* mq_subtree(void* _mq);

void mq_publish_now(void* pLwc, void* _mq, int stop);

double mq_mono_clock();
double mq_sync_mono_clock(void* _mq);
int mq_cursor_player(void* _mq, const char* cursor);
void mq_interrupt();
