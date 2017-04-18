#pragma once

typedef void(*touch_proc)(LWCONTEXT*);

typedef struct _LWTOUCHPROC {
	int valid;
	float x, y, w, h;
	touch_proc touch_proc_callback;
} LWTOUCHPROC;
