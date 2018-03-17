#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_font_test_fbo_body(const LWCONTEXT* pLwc, const char* html_body);
void lwc_render_font_test_fbo(const LWCONTEXT* pLwc, const char* html_path);
void lwc_render_font_test(const LWCONTEXT* pLwc);
void lwc_render_font_test_enable_render_world(int v);
void lwc_render_font_test_enable_render_world_map(int v);
void lwc_render_font_test_enable_render_route_line(int v);
#ifdef __cplusplus
}
#endif
