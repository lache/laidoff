#pragma once

#include "sprite_data.h"
#include "lwmacro.h"

typedef struct _LWCONTEXT LWCONTEXT;
typedef enum _LW_VBO_TYPE LW_VBO_TYPE;
typedef enum _LW_ATLAS_ENUM LW_ATLAS_ENUM;

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
    LAC_PREPARE_TITLE,
    
    LAC_COUNT,
} LW_ATLAS_CONF;

static const char *atlas_conf_filename[] = {
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "result-title-atlas-a.json",
    ASSETS_BASE_PATH "atlas" PATH_SEPARATOR "prepare-title-atlas-a.json",
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

const LWATLASSPRITE* atlas_sprite_name(const LWCONTEXT* pLwc, LW_ATLAS_CONF lac, const char* name);
void atlas_sprite_uv(const LWATLASSPRITE* sprite, int width, int height, float uv_offset[2], float uv_scale[2]);
void render_atlas_sprite(const LWCONTEXT* pLwc,
                         LW_ATLAS_CONF atlas_conf,
                         const char* sprite_name,
                         LW_ATLAS_ENUM lae,
                         LW_ATLAS_ENUM lae_alpha,
                         float sprite_width,
                         float x,
                         float y,
                         float ui_alpha,
                         LW_VBO_TYPE lvt);
