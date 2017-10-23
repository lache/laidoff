#pragma once
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWPUCKGAME LWPUCKGAME;

void update_puck_game(LWCONTEXT* pLwc, LWPUCKGAME* puck_game, double delta_time);
void puck_game_dash(LWCONTEXT* pLwc, LWPUCKGAME* puck_game);;
