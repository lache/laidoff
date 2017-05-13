#pragma once

void* init_mq();
void deinit_mq(void* _mq);
void mq_poll(void* sm, void* _mq);
void init_czmq();
void mq_shutdown();
