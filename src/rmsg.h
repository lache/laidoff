#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void rmsg_spawn(LWCONTEXT* pLwc, int key, int objtype, float x, float y, float z, float angle);
void rmsg_despawn(LWCONTEXT* pLwc, int key);
void rmsg_pos(LWCONTEXT* pLwc, int key, float x, float y, float z);
void rmsg_turn(LWCONTEXT* pLwc, int key, float angle);
void rmsg_anim(LWCONTEXT* pLwc, int key, int actionid);
void rmsg_rparams(LWCONTEXT* pLwc, int key, int atlas, int skin_vbo, int armature, float sx, float sy, float sz);
