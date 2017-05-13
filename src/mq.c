#include "zhelpers.h"
#include "mq.h"
#include "lwlog.h"
#include <czmq.h>
#include "sysmsg.h"

typedef struct _LWMESSAGEQUEUE {
	void* context;
	void* subscriber;
} LWMESSAGEQUEUE;

void* init_mq() {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)malloc(sizeof(LWMESSAGEQUEUE));

	//  Socket to talk to server
	LOGI("Collecting updates from weather server...");
	void *context = zmq_ctx_new();
	void *subscriber = zmq_socket(context, ZMQ_SUB);
	int rc = zmq_connect(subscriber, "tcp://222.110.4.119:5556");
	assert(rc == 0);

	//  Subscribe to zipcode, default is NYC, 10001
	char *filter = "10001 ";
	rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE,
		filter, strlen(filter));
	assert(rc == 0);

	mq->context = context;
	mq->subscriber = subscriber;
	return mq;
}

void mq_poll(void* sm, void* _mq) {
	char msg_str[256];
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	int size = zmq_recv(mq->subscriber, msg_str, 255, ZMQ_DONTWAIT);
	if (size == -1) {
		return;
	} else {
		msg_str[size] = '\0';
		show_sys_msg(sm, msg_str);
	}

	//int zipcode, temperature, relhumidity;
	//sscanf(string, "%d %d %d",
	//	&zipcode, &temperature, &relhumidity);
	//total_temp += temperature;
}

void deinit_mq(void* _mq) {
	LWMESSAGEQUEUE* mq = (LWMESSAGEQUEUE*)_mq;
	zmq_close(mq->subscriber);
	zmq_ctx_destroy(mq->context);
}

void init_czmq() {
	zsock_t* frontend = zsock_new(ZMQ_REQ);
	zsock_destroy(&frontend);
}

void mq_shutdown() {
	zsys_shutdown();
}
