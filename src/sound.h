#pragma once

#include "lwmacro.h"

#ifdef __cplusplus
extern "C" { ;
#endif

typedef enum _LW_SOUND {
    LWS_COLLAPSE,
    LWS_COLLISION,
    LWS_DAMAGE,
    LWS_DASH1,
    LWS_DASH2,
    LWS_DEFEAT,
    LWS_INTROBGM,
    LWS_VICTORY,
    LWS_SWOOSH,
    LWS_CLICK,
    LWS_READY,
    LWS_STEADY,
    LWS_GO,

    LWS_COUNT,
} LW_SOUND;

void play_sound(LW_SOUND lws);

#ifdef __cplusplus
}
#endif