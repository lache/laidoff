#include "lwcontext.h"

const vec4 EXP_COLOR = { 90 / 255.0f, 173 / 255.0f, 255 / 255.0f, 1 };

const char* tex_font_atlas_filename[] = {
	//ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_0.tga",
	//ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_1.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6_0.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6_1.tga",
};

int lwcontext_safe_to_start_render(const LWCONTEXT* pLwc) {
	return pLwc->safe_to_start_render;
}

void lwcontext_set_safe_to_start_render(LWCONTEXT* pLwc, int v) {
	pLwc->safe_to_start_render = v;
}

int lwcontext_rendering(const LWCONTEXT* pLwc) {
	return pLwc->rendering;
}

void lwcontext_set_rendering(LWCONTEXT* pLwc, int v) {
	pLwc->rendering = v;
}

void* lwcontext_mq(LWCONTEXT* pLwc) {
	return pLwc->mq;
}

LWFIELD* lwcontext_field(LWCONTEXT* pLwc) {
	return pLwc->field;
}