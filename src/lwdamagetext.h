#pragma once

#include "lwtextblock.h"

typedef enum _LW_DAMAGE_TEXT_COORD {
	LDTC_3D,
	LDTC_UI,
} LW_DAMAGE_TEXT_COORD;

typedef struct _LWDAMAGETEXT
{
	int valid;
	float x, y, z;
	float age;
	float max_age;
	char text[32];
	LWTEXTBLOCK text_block;
	LW_DAMAGE_TEXT_COORD coord;
} LWDAMAGETEXT;
