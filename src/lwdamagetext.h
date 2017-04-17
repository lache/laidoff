#pragma once

#include "lwtextblock.h"

typedef struct
{
	int valid;
	float x, y, z;
	float age;
	float max_age;
	char text[32];
	LWTEXTBLOCK text_block;
} LWDAMAGETEXT;
