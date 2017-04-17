#pragma once

struct _LWCONTEXT;

typedef void (*LW_BUTTON_COMMAND_HANDLER)(struct _LWCONTEXT* pLwc);

typedef struct _LWBUTTONCOMMAND {
	const char* name;
	LW_BUTTON_COMMAND_HANDLER command_handler;
} LWBUTTONCOMMAND;
