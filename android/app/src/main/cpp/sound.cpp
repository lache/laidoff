#include "sound.h"

void request_void_string_command(const char* command_name, const char* param1);

extern "C" int init_ext_sound_lib()
{
    return 0;
}

void request_play_sound(const char* sound_type);
void request_stop_sound(const char* sound_type);

extern "C" void play_sound(LW_SOUND lws)
{
	if (lws == LWS_METAL_HIT) {
		request_void_string_command("startPlayMetalHitSound", "dummy");
	}
}

extern "C" void stop_sound(LW_SOUND lws)
{
}
