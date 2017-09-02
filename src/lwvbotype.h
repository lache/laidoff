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
	LVT_FLOOR2,
	LVT_SPHERE,
	LVT_APT,
	LVT_BEAM,
	LVT_PUMP,
	LVT_OIL_TRUCK,
	LVT_ROOM,
	LVT_BATTLEGROUND_FLOOR,
	LVT_BATTLEGROUND_WALL,
	LVT_CROSSBOW_ARROW,
	LVT_CATAPULT_BALL,
	LVT_DEVIL,

	LVT_UI_SCRAP_BG,
	LVT_UI_TOWER_BUTTON_BG,

	LVT_COUNT,
} LW_VBO_TYPE;

typedef enum _LW_SKIN_VBO_TYPE {
	LSVT_TRIANGLE,
	LSVT_TREEPLANE,
	LSVT_HUMAN,
	LSVT_DETACHPLANE,
	LSVT_GUNTOWER,
	LSVT_TURRET,
	LSVT_CROSSBOW,
	LSVT_CATAPULT,
	LSVT_PYRO,

	LSVT_COUNT,
} LW_SKIN_VBO_TYPE;

typedef enum _LW_FAN_VBO_TYPE {
	LFVT_DEFAULT,

	LFVT_COUNT,
} LW_FAN_VBO_TYPE;

typedef enum _LW_PS_VBO_TYPE {
	LPVT_DEFAULT,

	LPVT_COUNT,
} LW_PS_VBO_TYPE;
