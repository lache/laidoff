#pragma once

#include "vertices.h"
#include "linmath.h"
#include "lwbattlecreature.h"
#include "lwsimpleanim.h"
#include "lwatlassprite.h"

typedef struct _LWENEMY
{
	int valid;
	enum _LW_ATLAS_SPRITE las;
	float scale;
	float shadow_scale;
	float time_offset;
	float shake_duration;
	float shake_magitude;
	LWANIM1D evasion_anim;
	LWANIM5D death_anim;
	LWBATTLECREATURE c;

	vec2 left_top_ui_point;
	vec2 right_bottom_ui_point;

	vec3 render_pos;
} LWENEMY;

#define ENEMY_ICECREAM { 1, LAS_ICECREAM, 0.15f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_ICECREAM }
#define ENEMY_HANNIBAL { 1, LAS_HANNIBAL, 0.3f, 0.15f, 0.2f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_HANNIBAL }
#define ENEMY_KEYBOARD { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_KEYBOARD }
#define ENEMY_FISH { 1, LAS_HANNIBAL, 0.3f, 0.15f, 0.2f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_FISH }
#define ENEMY_ANIMAL { 1, LAS_ICECREAM, 0.3f, 0.1f, 0.3f, 0.f, 0.f, {0}, {{0}}, BATTLECREATURE_ANIMAL }

const static LWENEMY ENEMY_DATA_LIST[] = {
    ENEMY_ICECREAM,
    ENEMY_HANNIBAL,
	ENEMY_KEYBOARD,
	ENEMY_FISH,
	ENEMY_ANIMAL,
};

struct _LWCONTEXT;

void update_enemy(const struct _LWCONTEXT* pLwc, int enemy_slot_index, LWENEMY* enemy);
float get_battle_enemy_x_center(int enemy_slot_index);
void update_render_enemy_position(const struct _LWCONTEXT* pLwc, int enemy_slot_index, const LWENEMY* enemy, vec3 pos);
