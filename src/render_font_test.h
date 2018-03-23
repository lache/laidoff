#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LWCONTEXT LWCONTEXT;

void lwc_render_font_test_fbo_body(const LWCONTEXT* pLwc, const char* html_body);
void lwc_render_font_test_fbo(const LWCONTEXT* pLwc, const char* html_path);
void lwc_render_font_test(const LWCONTEXT* pLwc);
int lwc_render_font_test_render(const char* name);
void lwc_render_font_test_enable_render(const char* name, int v);
#ifdef __cplusplus
}
#endif
