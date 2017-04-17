#include "constants.h"

extern "C" int init_ext_sound_lib()
{
    return 0;
}

void request_play_sound(const char* sound_type);
void request_stop_sound(const char* sound_type);

extern "C" void play_sound(enum LW_SOUND lws)
{
    switch (lws)
    {
        case LWS_DIE:
            request_play_sound("die");
            break;
        case LWS_HIT:
            request_play_sound("hit");
            break;
        case LWS_POINT:
            request_play_sound("point");
            break;
        case LWS_SWOOSHING:
            request_play_sound("swooshing");
            break;
        case LWS_TOUCH:
            request_play_sound("touch");
            break;
        case LWS_START:
            request_play_sound("start");
            break;
        case LWS_COMPLETE:
            request_play_sound("complete");
            break;
        case LWS_BGM:
            request_play_sound("bgm");
            break;
    }
}

extern "C" void stop_sound(enum LW_SOUND lws)
{
    switch (lws)
    {
        case LWS_BGM:
            request_stop_sound("bgm");
            break;
        default:
            // do nothing
            break;
    }
}
