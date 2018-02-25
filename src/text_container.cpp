#include "text_container.h"
#include <stdio.h>
#include <locale>
#include "lwgl.h"
#include "lwtextblock.h"
#include "lwcontext.h"
#include "render_text_block.h"
using namespace litehtml;

litehtml::text_container::text_container(const LWCONTEXT* pLwc, int w, int h) : pLwc(pLwc), w(w), h(h), default_font_size(36) {
}

litehtml::text_container::~text_container() {
}

litehtml::uint_ptr litehtml::text_container::create_font(const litehtml::tchar_t * faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics * fm) {
	//wprintf(_t("create_font: faceName=%s, size=%d, weight=%d\n"), faceName, size, weight);
	fm->height = 36;
	return litehtml::uint_ptr(1);// litehtml::uint_ptr();
}

void litehtml::text_container::delete_font(litehtml::uint_ptr hFont) {
}

int litehtml::text_container::text_width(const litehtml::tchar_t * text, litehtml::uint_ptr hFont) {
	return static_cast<int>(50);
}

void litehtml::text_container::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t * text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position & pos) {
	//wprintf(_t("draw_text: '%s' (x=%d,y=%d,color=0x%02X%02X%02X|%02X)\n"), text, pos.x, pos.y, color.red, color.green, color.blue, color.alpha);

	LWTEXTBLOCK test_text_block;
	test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
	test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D;
	test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_B;
	SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
	test_text_block.text = text;
	test_text_block.text_bytelen = (int)strlen(test_text_block.text);
	test_text_block.begin_index = 0;
	test_text_block.end_index = test_text_block.text_bytelen;
	test_text_block.multiline = 1;
	test_text_block.text_block_x = -pLwc->aspect_ratio + 2 * ((float)pos.x / pLwc->width * pLwc->aspect_ratio);
	test_text_block.text_block_y = 1.0f - 2 * ((float)pos.y / pLwc->height);
	test_text_block.align = LTBA_LEFT_BOTTOM;
	render_text_block(pLwc, &test_text_block);
}

int litehtml::text_container::pt_to_px(int pt) {
	return static_cast<int>(pt / 10.0 * 36.0);
}

int litehtml::text_container::get_default_font_size() const {
	return default_font_size;
}

const litehtml::tchar_t * litehtml::text_container::get_default_font_name() const {
	return _t("Arial");
}

void litehtml::text_container::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker & marker) {
	//printf("draw_list_marker");
}

void litehtml::text_container::load_image(const litehtml::tchar_t * src, const litehtml::tchar_t * baseurl, bool redraw_on_ready) {
	//wprintf(_t("load_image: src=%s,baseurl=%s,redraw_on_ready=%d\n"), src, baseurl, redraw_on_ready);
}

void litehtml::text_container::get_image_size(const litehtml::tchar_t * src, const litehtml::tchar_t * baseurl, litehtml::size & sz) {
	sz.width = 100;
	sz.height = 50;
	//wprintf(_t("get_image_size: src=%s,baseurl=%s\n"), src, baseurl);
}

void litehtml::text_container::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint & bg) {
	//wprintf(_t("draw_background: x=%d,y=%d,w=%d,h=%d,color=0x%02X%02X%02X|%02X,image=%s,baseurl=%s\n"), bg.border_box.x, bg.border_box.y, bg.border_box.width, bg.border_box.height, bg.color.red, bg.color.green, bg.color.blue, bg.color.alpha, bg.image.c_str(), bg.baseurl.c_str());
}

void litehtml::text_container::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders & borders, const litehtml::position & draw_pos, bool root) {
	/*printf("draw_borders: x=%d y=%d w=%d h=%d Lw=%d Rw=%d Tw=%d Bw=%d root=%d\n", draw_pos.x, draw_pos.y, draw_pos.width, draw_pos.height,
		borders.left.width, borders.right.width, borders.top.width, borders.bottom.width, root);*/
}

void litehtml::text_container::set_caption(const litehtml::tchar_t * caption) {
	//wprintf(_t("set_caption: %s\n"), caption);
}

void litehtml::text_container::set_base_url(const litehtml::tchar_t * base_url) {
	//wprintf(_t("set_base_url: %s\n"), base_url);
}

void litehtml::text_container::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr & el) {
	//printf("link\n");
}

void litehtml::text_container::on_anchor_click(const litehtml::tchar_t * url, const litehtml::element::ptr & el) {
	//wprintf(_t("on_anchor_click: %s\n"), url);
}

void litehtml::text_container::set_cursor(const litehtml::tchar_t * cursor) {
	//printf("set_cursor\n");
}

void litehtml::text_container::transform_text(litehtml::tstring & text, litehtml::text_transform tt) {
	//printf("transform_text\n");
}

void litehtml::text_container::import_css(litehtml::tstring & text, const litehtml::tstring & url, litehtml::tstring & baseurl) {
	//printf("import_css\n");
}

void litehtml::text_container::set_clip(const litehtml::position & pos, const litehtml::border_radiuses & bdr_radius, bool valid_x, bool valid_y) {
	//printf("set_clip\n");
}

void litehtml::text_container::del_clip() {
	//printf("del_clip\n");
}

void litehtml::text_container::get_client_rect(litehtml::position & client) const {
	client.x = 0;
	client.y = 0;
	client.width = w;
	client.height = h;
	//printf("get_client_rect\n");
}

std::shared_ptr<litehtml::element> litehtml::text_container::create_element(const litehtml::tchar_t * tag_name, const litehtml::string_map & attributes, const std::shared_ptr<litehtml::document>& doc) {
	return std::shared_ptr<litehtml::element>();
}

void litehtml::text_container::get_media_features(litehtml::media_features & media) const {
}

void litehtml::text_container::get_language(litehtml::tstring & language, litehtml::tstring & culture) const {
}
