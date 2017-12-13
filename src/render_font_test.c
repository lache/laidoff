#include <string.h>
#include "render_font_test.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "laidoff.h"

void lwc_render_font_test_fbo(const struct _LWCONTEXT* pLwc) {
	glBindFramebuffer(GL_FRAMEBUFFER, pLwc->font_fbo.fbo);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, pLwc->font_fbo.width, pLwc->font_fbo.height);
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	LWTEXTBLOCK test_text_block;
	test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
	test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D;
	test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
	SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
	test_text_block.text = LWU("lqpM^_^123-45");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.multiline = 1;

	// The first column (left aligned)

	test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = 0.25f;
	test_text_block.align = LTBA_LEFT_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = 0;
	test_text_block.align = LTBA_LEFT_CENTER;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text_block_x = -0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = -0.25f;
	test_text_block.align = LTBA_LEFT_BOTTOM;
	render_text_block(pLwc, &test_text_block);

	// The second column (center aligned)

	test_text_block.text = LWU("lqpM^__^Mpql");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = 0.25f;
	test_text_block.align = LTBA_CENTER_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text = LWU("가가가가가가가가가가가가가가가가가가가가");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = 0.50f;
	test_text_block.align = LTBA_CENTER_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text = LWU("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = 0.75f;
	test_text_block.align = LTBA_CENTER_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text = LWU("한글이 됩니다~~~");
	//test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = 1.0f;
	test_text_block.align = LTBA_CENTER_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = 0;
	test_text_block.align = LTBA_CENTER_CENTER;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text = LWU("이제 진정하십시오...");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0;
	test_text_block.text_block_y = -0.25f;
	test_text_block.align = LTBA_CENTER_BOTTOM;
	render_text_block(pLwc, &test_text_block);


	// The third Column (right aligned)

	test_text_block.text = LWU("lmqpMQ^__^ 123-45");
	//test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = 0.25f;
	test_text_block.align = LTBA_RIGHT_TOP;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = 0;
	test_text_block.align = LTBA_RIGHT_CENTER;
	render_text_block(pLwc, &test_text_block);

	test_text_block.text = LWU("국민 여러분!");
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.text_block_x = 0.9f * pLwc->aspect_ratio;
	test_text_block.text_block_y = -0.25f;
	test_text_block.align = LTBA_RIGHT_BOTTOM;
	render_text_block(pLwc, &test_text_block);

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwc_render_font_test(const struct _LWCONTEXT* pLwc) {
	LW_GL_VIEWPORT();
	lw_clear_color();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	render_solid_box_ui_lvt_flip_y_uv(pLwc, 0, 0, 2 * pLwc->aspect_ratio, 2/*flip_y*/, pLwc->font_fbo.color_tex, LVT_CENTER_CENTER_ANCHORED_SQUARE, 1);
}
