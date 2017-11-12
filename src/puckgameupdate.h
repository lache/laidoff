#pragma once
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWPUCKGAME LWPUCKGAME;

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time);
void puck_game_dash(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_target_move(LWPUCKGAME* puck_game, float dx, float dy);
void puck_game_target_stop(LWPUCKGAME* puck_game);
void puck_game_target_dash(LWPUCKGAME* puck_game, int player_no);
void puck_game_pull_puck_start(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_pull_puck_stop(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_pull_puck_toggle(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_rematch(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
void puck_game_reset_view_proj(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);
