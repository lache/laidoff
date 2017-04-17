#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int request_get_today_played_count();
int request_get_today_playing_limit_count();
void request_on_game_start();
void request_on_game_over(int point);
void request_boast(int point);
int request_is_retryable();
int request_is_boasted();
int request_get_highscore();
void request_set_highscore(int highscore);

#ifdef __cplusplus
};
#endif
