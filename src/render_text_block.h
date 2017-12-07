#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;

void render_text_block(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block);
void render_text_block_alpha(const LWCONTEXT* pLwc, const LWTEXTBLOCK* text_block, float ui_alpha);
void toggle_font_texture_test_mode(LWCONTEXT* pLwc);
