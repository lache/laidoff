#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _LWCONTEXT LWCONTEXT;
	int test_html_ui(const LWCONTEXT* pLwc);
	void* htmlui_new(const LWCONTEXT* pLwc);
	void htmlui_destroy(void** c);
	void htmlui_load_render_draw(void* c, const char* html_path);
	void htmlui_on_lbutton_down(void* c, int x, int y);
	void htmlui_on_lbutton_up(void* c, int x, int y);
	void htmlui_on_over(void* c, int x, int y);
#ifdef __cplusplus
}
#endif
