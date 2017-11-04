#pragma once

typedef struct _LWRINGBUFFER {
	void* buf0;
	int stride;
	int capacity;
	int head;
	int tail;
	int full;
} LWRINGBUFFER;

int ringbuffer_init(LWRINGBUFFER* rb, void* buf0, int stride, int capacity);
void ringbuffer_queue(LWRINGBUFFER* rb, const void* p);
const void* ringbuffer_dequeue(LWRINGBUFFER* rb);
const void* ringbuffer_peek(LWRINGBUFFER* rb);
int ringbuffer_size(LWRINGBUFFER* rb);
int ringbuffer_capacity(LWRINGBUFFER* rb);
