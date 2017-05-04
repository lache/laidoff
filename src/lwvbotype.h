#pragma once

typedef enum _LW_VBO_TYPE
{
	LVT_BATTLE_BOWL_OUTER,
	LVT_BATTLE_BOWL_INNER,
	LVT_ENEMY_SCOPE,
	LVT_LEFT_TOP_ANCHORED_SQUARE,		LVT_CENTER_TOP_ANCHORED_SQUARE,		LVT_RIGHT_TOP_ANCHORED_SQUARE,
	LVT_LEFT_CENTER_ANCHORED_SQUARE,	LVT_CENTER_CENTER_ANCHORED_SQUARE,	LVT_RIGHT_CENTER_ANCHORED_SQUARE,
	LVT_LEFT_BOTTOM_ANCHORED_SQUARE,	LVT_CENTER_BOTTOM_ANCHORED_SQUARE,	LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
	LVT_PLAYER,
	LVT_CUBE_WALL,
	LVT_HOME,
	LVT_TRAIL,
	LVT_FLOOR,
	LVT_SPHERE,

	LVT_COUNT,
} LW_VBO_TYPE;

typedef enum _LW_SKIN_VBO_TYPE {
	LSVT_TRIANGLE,
	LSVT_TREEPLANE,
	LSVT_HUMAN,
	LSVT_DETACHPLANE,

	LSVT_COUNT,
} LW_SKIN_VBO_TYPE;
