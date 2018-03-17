#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _LWCONTEXT LWCONTEXT;
	int test_html_ui(LWCONTEXT* pLwc);
	void* htmlui_new(LWCONTEXT* pLwc);
	void htmlui_destroy(void** c);
	void htmlui_load_render_draw(void* c, const char* html_path);
    void htmlui_load_render_draw_body(void* c, const char* html_body);
	void htmlui_on_lbutton_down(void* c, float nx, float ny);
	void htmlui_on_lbutton_up(void* c, float nx, float ny);
	void htmlui_on_over(void* c, float nx, float ny);
    void htmlui_set_next_html_path(void* c, const char* html_path);
    void htmlui_load_next_html_path(void* c);
    void htmlui_set_refresh_html_body(void* c, int v);
    void htmlui_load_next_html_body(void* c);
#ifdef __cplusplus
}
#endif
