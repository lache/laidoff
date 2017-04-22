#include "sound.h"

extern "C" int init_ext_sound_lib()
{
    return 0;
}

void request_play_sound(const char* sound_type);
void request_stop_sound(const char* sound_type);

extern "C" void play_sound(LW_SOUND lws)
{
}

extern "C" void stop_sound(LW_SOUND lws)
{
}
