#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void rmsg_spawn(LWCONTEXT* pLwc, int key, int objtype, float x, float y, float angle);
void rmsg_despawn(LWCONTEXT* pLwc, int key);
void rmsg_pos(LWCONTEXT* pLwc, int key, float x, float y);
void rmsg_turn(LWCONTEXT* pLwc, int key, float angle);
void rmsg_anim(LWCONTEXT* pLwc, int key, int actionid);
