#include "lwringbuffer.h"
#include <string.h>

int ringbuffer_init(LWRINGBUFFER* rb, void* buf0, int stride, int capacity) {
	if (rb == 0) {
		return -1;
	}
	if (buf0 == 0) {
		return -2;
	}
	if (stride <= 0) {
		return -3;
	}
	if (stride % sizeof(int) != 0) {
		return -4;
	}
	if (capacity <= 0) {
		return -5;
	}
	memset(rb, 0, sizeof(LWRINGBUFFER));
	rb->buf0 = buf0;
	rb->stride = stride;
	rb->capacity = capacity;
	return 0;
}

void ringbuffer_queue(LWRINGBUFFER* rb, const void* p) {
	memcpy((char*)rb->buf0 + rb->tail * rb->stride, p, rb->stride);
	if (rb->full) {
		rb->head = (rb->head + 1) % rb->capacity;
	}
	rb->tail = (rb->tail + 1) % rb->capacity;
	if (rb->full == 0 && rb->head == rb->tail) {
		rb->full = 1;
	}
}

const void* ringbuffer_dequeue(LWRINGBUFFER* rb) {
	if (rb->full == 0 && rb->head == rb->tail) {
		return 0;
	}
	void* p = (char*)rb->buf0 + rb->head * rb->stride;
	rb->head = (rb->head + 1) % rb->capacity;
	rb->full = 0;
	return p;
}

int ringbuffer_size(LWRINGBUFFER* rb) {
	if (rb->full) {
		return rb->capacity;
	}
	else if (rb->head <= rb->tail) {
		return rb->tail - rb->head;
	}
	return rb->capacity - (rb->head - rb->tail);
}

int ringbuffer_capacity(LWRINGBUFFER* rb) {
	return rb->capacity;
}
