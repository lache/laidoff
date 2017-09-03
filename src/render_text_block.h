#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;

void render_text_block(const struct _LWCONTEXT *pLwc, const struct _LWTEXTBLOCK* text_block);
void toggle_font_texture_test_mode(LWCONTEXT* pLwc);
