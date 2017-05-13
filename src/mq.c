#include "zhelpers.h"
#include "mq.h"
#include "lwlog.h"

void init_zmq() {
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

	//  Process 100 updates
	int update_nbr;
	long total_temp = 0;
	for (update_nbr = 0; update_nbr < 100; update_nbr++) {
		char *string = s_recv(subscriber);

		int zipcode, temperature, relhumidity;
		sscanf(string, "%d %d %d",
			&zipcode, &temperature, &relhumidity);
		total_temp += temperature;
		free(string);
	}
	LOGI("Average temperature for zipcode '%s' was %dF",
		filter, (int)(total_temp / update_nbr));

	zmq_close(subscriber);
	zmq_ctx_destroy(context);
}

