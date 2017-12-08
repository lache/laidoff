#pragma once

#include "lwmacro.h"

#ifdef __cplusplus
extern "C" {;
#endif

typedef enum _LW_SOUND {
    LWS_METAL_HIT,
	LWS_DIE,
	LWS_HIT,
	LWS_POINT,
	LWS_SWOOSHING,
	LWS_WING,

	LWS_COUNT,
} LW_SOUND;

const static char* SOUND_FILE[] = {
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_metal_hit.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_die.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_hit.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_point.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_swooshing.ogg",
    ASSETS_BASE_PATH "ogg" PATH_SEPARATOR "sfx_wing.ogg",
};

void play_sound(LW_SOUND lws);

#ifdef __cplusplus
}
#endif