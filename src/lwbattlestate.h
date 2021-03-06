#pragma once

typedef enum _LW_BATTLE_STATE {
	// Player turn
	LBS_SELECT_COMMAND,
	LBS_SELECT_TARGET,
	LBS_COMMAND_IN_PROGRESS,
	// Enemy turn
	LBS_START_ENEMY_TURN,
	LBS_ENEMY_TURN_WAIT,
	LBS_ENEMY_COMMAND_IN_PROGRESS,
	// Result
	LBS_START_PLAYER_WIN,
	LBS_PLAYER_WIN_IN_PROGRESS,
	LBS_FINISH_PLAYER_WIN,
} LW_BATTLE_STATE;
