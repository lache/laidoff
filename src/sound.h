#pragma once

#ifdef __cplusplus
extern "C" {;
#endif

typedef enum _LW_SOUND {
	LWS_DIE,
	LWS_HIT,
	LWS_POINT,
	LWS_SWOOSHING,
	LWS_WING,

	LWS_COUNT,
} LW_SOUND;

void play_sound(LW_SOUND lws);

#ifdef __cplusplus
}
#endif