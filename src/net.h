#pragma once

struct _LWCONTEXT;

void init_net(struct _LWCONTEXT* pLwc);
void net_rtt_test(struct _LWCONTEXT* pLwc);
void deinit_net(struct _LWCONTEXT* pLwc);
