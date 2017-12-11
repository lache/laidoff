#pragma once

#include "sprite_data.h"
#include "lwmacro.h"

typedef enum _LW_ATLAS_SPRITE
{
    LAS_COMMAND_SELECTED_BG,
    LAS_HANNIBAL_FAT,
    LAS_HANNIBAL,
    LAS_ICECREAM_FAT,
    LAS_ICECREAM,
    LAS_PSLOT1,
    LAS_SHADOW,
    LAS_UI_ALL,
    
    LAS_COUNT,
    
    LAS_DONTCARE,
} LW_ATLAS_SPRITE;

typedef struct _LWATLASSPRITE {
    char name[64];
    int x;
    int y;
    int width;
    int height;
} LWATLASSPRITE;

LwStaticAssert(ARRAY_SIZE(SPRITE_DATA[0]) == LAS_COUNT, "LAS_COUNT error");
