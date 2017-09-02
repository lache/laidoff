#include "render_ui.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"
#include "render_text_block.h"
#include "lwmacro.h"

const float scrap_bg_width = 7.0f;
const float scrap_bg_height = 1.0f;

static void s_create_scrap_bg_vbo(LWCONTEXT* pLwc) {
	const float c0[3] = { 0.2f, 0.2f, 0.2f };
	const float c1[3] = { 0.3f, 0.3f, 0.3f };
	const float c2[3] = { 0.4f, 0.4f, 0.4f };
	const LWVERTEX scrap_bg[] =
	{
		{ 0,								-scrap_bg_height,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
		{ scrap_bg_width - scrap_bg_height,	-scrap_bg_height,	0, c1[0], c1[1], c1[2], 1, 1, 0, 0 },
		{ scrap_bg_width,					0,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
		{ scrap_bg_width,					0,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
		{ scrap_bg_height,					0,					0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
		{ 0,								-scrap_bg_height,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
	};
	LWVERTEX square_vertices[ARRAY_SIZE(scrap_bg)];
	memcpy(square_vertices, scrap_bg, sizeof(scrap_bg));

	const LW_VBO_TYPE lvt = LVT_UI_SCRAP_BG;

	glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(scrap_bg),
		square_vertices, GL_STATIC_DRAW);
	pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(scrap_bg);
}

const float tower_button_height = 1.0f;
const float tower_button_width = 1.0f;
const float tower_button_tag_width = 1.2f;
const float tower_button_tag_height = 0.35f;
const float tower_button_height_margin = 0.2f;
const float tower_button_border = 0.075f;

static void s_create_tower_button_bg_vbo(LWCONTEXT* pLwc) {
	const float c0[3] = { 0.2f, 0.2f, 0.2f };
	const float c1[3] = { 0.3f, 0.3f, 0.3f };
	const float c2[3] = { 0.4f, 0.4f, 0.4f };
	const LWVERTEX tower_button_bg[] =
	{
		{ 0,								-tower_button_height,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
		{ tower_button_width,	-scrap_bg_height,	0, c1[0], c1[1], c1[2], 1, 1, 0, 0 },
		{ tower_button_width,					-tower_button_tag_height,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },

		{ tower_button_width,					-tower_button_tag_height,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
		{ tower_button_width + tower_button_tag_width,					-tower_button_tag_height,					0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
		{ tower_button_width + tower_button_tag_width,					0,					0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },

		{ tower_button_width + tower_button_tag_width,					0,					0, c1[0], c1[1], c1[2], 0, 0, 0, 0 },
		{ 0,								0,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
		{ tower_button_width,					-tower_button_tag_height,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },

		{ tower_button_width,					-tower_button_tag_height,					0, c2[0], c2[1], c2[2], 1, 0, 0, 0 },
		{ 0,								0,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
		{ 0,								-tower_button_height,	0, c0[0], c0[1], c0[2], 0, 1, 0, 0 },
	};
	LWVERTEX square_vertices[ARRAY_SIZE(tower_button_bg)];
	memcpy(square_vertices, tower_button_bg, sizeof(tower_button_bg));

	const LW_VBO_TYPE lvt = LVT_UI_TOWER_BUTTON_BG;

	glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * ARRAY_SIZE(tower_button_bg),
		square_vertices, GL_STATIC_DRAW);
	pLwc->vertex_buffer[lvt].vertex_count = ARRAY_SIZE(tower_button_bg);
}

void lwc_create_ui_vbo(LWCONTEXT* pLwc) {
	s_create_scrap_bg_vbo(pLwc);
	s_create_tower_button_bg_vbo(pLwc);
}

static void s_render_scrap(const LWCONTEXT* pLwc) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	const float scrap_bg_size_nor = 0.15f;
	const float scrap_bg_x_nor = aspect_ratio - scrap_bg_width * scrap_bg_size_nor;
	const float scrap_bg_y_nor = 1.0f;

	render_solid_vb_ui_flip_y_uv_shader(pLwc, scrap_bg_x_nor, scrap_bg_y_nor, 2 * scrap_bg_size_nor, 2 * scrap_bg_size_nor, pLwc->tex_atlas[LAE_3D_APT_TEX_MIP_KTX], LVT_UI_SCRAP_BG,
		1, 0, 0, 0, 0, 0, LWST_COLOR);

	LWTEXTBLOCK text_block;
	text_block.align = LTBA_LEFT_CENTER;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 0);
	char msg[128];
	sprintf(msg, "[=] 888,888");
	text_block.text = msg;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = scrap_bg_x_nor + scrap_bg_size_nor;
	text_block.text_block_y = scrap_bg_y_nor - (scrap_bg_size_nor / 2);
	text_block.multiline = 1;
	render_text_block(pLwc, &text_block);
}

static void s_render_tower_button(const LWCONTEXT* pLwc) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	for (int i = 0; i < 4; i++) {
		const float btn_bg_size_nor = 0.3f;
		const float btn_bg_x_nor = aspect_ratio - (tower_button_width + tower_button_tag_width) * btn_bg_size_nor;
		const float btn_bg_y_nor = 1.0f - (tower_button_height_margin + tower_button_height) * btn_bg_size_nor * (i + 1);
		const float border_scaled = tower_button_border * btn_bg_size_nor;
		const float sprite_size_nor = btn_bg_size_nor * (1.0f - 2.0f * tower_button_border);
		// Button background (frame)
		render_solid_vb_ui_flip_y_uv_shader(pLwc, btn_bg_x_nor, btn_bg_y_nor, 2 * btn_bg_size_nor, 2 * btn_bg_size_nor,
			0, LVT_UI_TOWER_BUTTON_BG,
			1, 0, 0, 0, 0, 0, LWST_COLOR);
		// Tower thumbnail sprite
		render_solid_vb_ui(pLwc, btn_bg_x_nor + border_scaled, btn_bg_y_nor - border_scaled,
			sprite_size_nor, sprite_size_nor, pLwc->tex_atlas[LAE_BB_CATAPULT + i], LVT_LEFT_TOP_ANCHORED_SQUARE, 1, 0, 0, 0, 0);
		// Scrap price
		LWTEXTBLOCK text_block;
		text_block.align = LTBA_LEFT_CENTER;
		text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
		text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
		text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
		SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
		SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 0);
		SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
		SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 0);
		char msg[128];
		sprintf(msg, "[=] %d,000", i + 1);
		text_block.text = msg;
		text_block.text_bytelen = (int)strlen(text_block.text);
		text_block.begin_index = 0;
		text_block.end_index = text_block.text_bytelen;
		text_block.text_block_x = btn_bg_x_nor + (tower_button_width) * btn_bg_size_nor;
		text_block.text_block_y = btn_bg_y_nor - tower_button_tag_height / 2 * btn_bg_size_nor;
		text_block.multiline = 1;
		render_text_block(pLwc, &text_block);
	}
}

void lwc_render_ui(const LWCONTEXT* pLwc) {
	glViewport(0, 0, pLwc->width, pLwc->height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	s_render_scrap(pLwc);
	s_render_tower_button(pLwc);
}
