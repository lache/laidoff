#pragma once

typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWTEXTBLOCK LWTEXTBLOCK;

void render_text_block(const struct _LWCONTEXT *pLwc, const struct _LWTEXTBLOCK* text_block);
