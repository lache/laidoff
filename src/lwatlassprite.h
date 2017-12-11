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

typedef enum _LW_ATLAS_CONF {
    LAC_RESULT_TITLE,
    
    LAC_COUNT,
} LW_ATLAS_CONF;

static const char *atlas_conf_filename[] = {
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "result-title-atlas-a.json",
};

typedef struct _LWATLASSPRITE {
    char name[64];
    int x;
    int y;
    int width;
    int height;
} LWATLASSPRITE;

typedef struct _LWATLASSPRITEARRAY {
    int count;
    LWATLASSPRITE* first;
} LWATLASSPRITEARRAY;

LwStaticAssert(ARRAY_SIZE(SPRITE_DATA[0]) == LAS_COUNT, "LAS_COUNT error");
LwStaticAssert(ARRAY_SIZE(atlas_conf_filename) == LAC_COUNT, "LAC_COUNT error");
