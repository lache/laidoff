#include "text_container.h"
#include <stdio.h>
#include <locale>
#include "lwgl.h"
#include "lwtextblock.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "lwlog.h"
#include "htmlui.h"
#include "lwtcpclient.h"
#include "el_luascript.h"

litehtml::text_container::text_container(LWCONTEXT* pLwc, int w, int h)
    : pLwc(pLwc), w(w), h(h), default_font_size(36) {
}

litehtml::text_container::~text_container() {
}

litehtml::uint_ptr litehtml::text_container::create_font(const litehtml::tchar_t * faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics * fm) {
    LOGIx("create_font: faceName=%s, size=%d, weight=%d\n", faceName, size, weight);
    size_t font_idx = font_sizes.size();
    font_sizes.push_back(size);
    fm->height = static_cast<int>(roundf(size * 0.8f * pLwc->height / 720.f));
    fm->descent = static_cast<int>(roundf(size * 0.1f * pLwc->height / 720.f));
    return litehtml::uint_ptr(font_idx);// litehtml::uint_ptr();
}

void litehtml::text_container::delete_font(litehtml::uint_ptr hFont) {
}

static float conv_size_x(const LWCONTEXT* pLwc, int x) {
    return 2 * ((float)x / pLwc->width * pLwc->aspect_ratio);
}

static float conv_size_y(const LWCONTEXT* pLwc, int y) {
    return 2 * ((float)y / pLwc->height);
}

static float conv_coord_x(const LWCONTEXT* pLwc, int x) {
    return -pLwc->aspect_ratio + conv_size_x(pLwc, x);
}

static float conv_coord_y(const LWCONTEXT* pLwc, int y) {
    return 1.0f - conv_size_y(pLwc, y);
}

static void fill_text_block(const LWCONTEXT* pLwc, LWTEXTBLOCK* text_block, int x, int y, const char* text, int size, const litehtml::web_color& color) {
    text_block->text_block_width = 999.0f;// 2.00f * aspect_ratio;
    LOGIx("font size: %d", size);
    text_block->text_block_line_height = size / 72.0f;
    text_block->size = size / 72.0f;
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_glyph, color.red / 255.0f, color.green / 255.0f, color.blue / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_normal_outline, (255 - color.red) / 255.0f, (255 - color.green) / 255.0f, (255 - color.blue) / 255.0f, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block->color_emp_outline, 0, 0, 0, 1);
    text_block->text = text;
    text_block->text_bytelen = (int)strlen(text_block->text);
    text_block->begin_index = 0;
    text_block->end_index = text_block->text_bytelen;
    text_block->multiline = 1;
    text_block->text_block_x = conv_coord_x(pLwc, x);
    text_block->text_block_y = conv_coord_y(pLwc, y);
    text_block->align = LTBA_LEFT_BOTTOM;
}

int litehtml::text_container::text_width(const litehtml::tchar_t * text, litehtml::uint_ptr hFont) {
    LWTEXTBLOCK text_block;
    LWTEXTBLOCKQUERYRESULT query_result;
    int size = font_sizes[(int)(size_t)hFont];
    litehtml::web_color c;
    fill_text_block(pLwc, &text_block, 0, 0, text, size, c);
    render_query_only_text_block(pLwc, &text_block, &query_result);
    return static_cast<int>(query_result.total_glyph_width / (2 * pLwc->aspect_ratio) * pLwc->width);
}

void litehtml::text_container::draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t * text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position & pos) {
    //wprintf(_t("draw_text: '%s' (x=%d,y=%d,color=0x%02X%02X%02X|%02X)\n"), text, pos.x, pos.y, color.red, color.green, color.blue, color.alpha);
    int size = font_sizes[(int)(size_t)hFont];
    LWTEXTBLOCK text_block;
    fill_text_block(pLwc, &text_block, pos.x, pos.y, text, size, color);
    render_text_block(pLwc, &text_block);
}

int litehtml::text_container::pt_to_px(int pt) {
    return static_cast<int>(roundf(pt * 3.6f * pLwc->width / 640.0f));
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
    LOGI("get_image_size: src=%s,baseurl=%s", src, baseurl);
    sz.width = static_cast<int>(roundf(180 * pLwc->width / 640.0f));
    sz.height = static_cast<int>(roundf(180 * pLwc->height / 360.0f));
}

void litehtml::text_container::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint & bg) {
    // ignore body background-color
    if (bg.is_root) {
        return;
    }
    bool show_test_image = false;
    int lae = LAE_TTL_TITLE;
    int lae_alpha = LAE_TTL_TITLE_ALPHA;
    if (bg.image.length()) {
        LOGI("draw_background [IMAGE]: x=%d,y=%d,w=%d,h=%d,color=0x%02X%02X%02X|%02X,image=%s,baseurl=%s",
             bg.border_box.x,
             bg.border_box.y,
             bg.border_box.width,
             bg.border_box.height,
             bg.color.red,
             bg.color.green,
             bg.color.blue,
             bg.color.alpha,
             bg.image.c_str(),
             bg.baseurl.c_str());
        show_test_image = 1;
        lazy_tex_atlas_glBindTexture(pLwc, lae);
        lazy_tex_atlas_glBindTexture(pLwc, lae_alpha);
    }
    if (show_test_image) {
        render_solid_vb_ui_alpha(
            pLwc,
            conv_coord_x(pLwc, bg.border_box.x),
            conv_coord_y(pLwc, bg.border_box.y),
            conv_size_x(pLwc, bg.border_box.width),
            conv_size_y(pLwc, bg.border_box.height),
            show_test_image ? pLwc->tex_atlas[lae] : 0,
            show_test_image ? pLwc->tex_atlas[lae_alpha] : 0,
            LVT_LEFT_TOP_ANCHORED_SQUARE,
            bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
            show_test_image ? 1.0f : bg.color.red / 255.0f,
            show_test_image ? 1.0f : bg.color.green / 255.0f,
            show_test_image ? 1.0f : bg.color.blue / 255.0f,
            show_test_image ? 0.0f : 1.0f
        );
    } else {
        render_solid_vb_ui_flip_y_uv_shader(
            pLwc,
            conv_coord_x(pLwc, bg.border_box.x),
            conv_coord_y(pLwc, bg.border_box.y),
            conv_size_x(pLwc, bg.border_box.width),
            conv_size_y(pLwc, bg.border_box.height),
            show_test_image ? pLwc->tex_atlas[lae] : 0,
            LVT_LEFT_TOP_ANCHORED_SQUARE,
            bg.is_root ? 0.0f : show_test_image ? 1.0f : bg.color.alpha / 255.0f,
            show_test_image ? 1.0f : bg.color.red / 255.0f,
            show_test_image ? 1.0f : bg.color.green / 255.0f,
            show_test_image ? 1.0f : bg.color.blue / 255.0f,
            show_test_image ? 0.0f : 1.0f,
            0,
            LWST_DEFAULT
        );
    }
}

void litehtml::text_container::draw_border_rect(const litehtml::border& border, int x, int y, int w, int h, LW_VBO_TYPE lvt, const litehtml::web_color& color) const {
    if (w <= 0 || h <= 0) {
        return;
    }
    if (border.style == border_style_none || border.style == border_style_hidden) {
        return;
    }
    render_solid_vb_ui_flip_y_uv_shader(
        pLwc,
        conv_coord_x(pLwc, x),
        conv_coord_y(pLwc, y),
        conv_size_x(pLwc, w),
        conv_size_y(pLwc, h),
        0,
        lvt,
        color.alpha / 255.0f,
        color.red / 255.0f,
        color.green / 255.0f,
        color.blue / 255.0f,
        1.0f,
        0,
        LWST_DEFAULT
    );
}

void litehtml::text_container::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders & borders, const litehtml::position & draw_pos, bool root) {
    LOGIx("draw_borders: x=%d y=%d w=%d h=%d Lw=%d Rw=%d Tw=%d Bw=%d root=%d",
          draw_pos.x,
          draw_pos.y,
          draw_pos.width,
          draw_pos.height,
          borders.left.width,
          borders.right.width,
          borders.top.width,
          borders.bottom.width,
          root);
    // left border
    draw_border_rect(borders.left, draw_pos.x, draw_pos.y, borders.left.width, draw_pos.height, LVT_LEFT_TOP_ANCHORED_SQUARE, borders.left.color);
    // right border
    draw_border_rect(borders.right, draw_pos.x + draw_pos.width, draw_pos.y, borders.right.width, draw_pos.height, LVT_RIGHT_TOP_ANCHORED_SQUARE, borders.right.color);
    // top border
    draw_border_rect(borders.top, draw_pos.x, draw_pos.y, draw_pos.width, borders.top.width, LVT_LEFT_TOP_ANCHORED_SQUARE, borders.top.color);
    // bottom border
    draw_border_rect(borders.bottom, draw_pos.x, draw_pos.y + draw_pos.height, draw_pos.width, borders.bottom.width, LVT_LEFT_BOTTOM_ANCHORED_SQUARE, borders.bottom.color);
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
    LOGI("on_anchor_click: %s", url);
    if (strcmp(url, "script:go_online()") == 0) {
        tcp_request_landing_page(pLwc->tcp_ttl, pLwc->user_data_path);
    } else if (strcmp(url, "script:toggle_world()") == 0) {
        //enable_render_world = !enable_render_world;
    } else if (strcmp(url, "script:toggle_world_map()") == 0) {
        //enable_render_world_map = !enable_render_world_map;
    } else if (strcmp(url, "script:toggle_route_line()") == 0) {
        //enable_render_route_line = !enable_render_route_line;
    } else {
        const char* path_prefix = ASSETS_BASE_PATH "html" PATH_SEPARATOR;
        char path[1024] = { 0, };
        strcat(path, path_prefix);
        strcat(path, url);
        //htmlui_set_next_html_path(pLwc->htmlui, path);
        tcp_send_httpget(pLwc->tcp_ttl, url);
    }
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
    if (strcmp(tag_name, "script") == 0) {
        auto ait = attributes.find("type");
        if (ait != attributes.cend() && ait->second == "text/x-lua") {
            return std::make_shared<litehtml::el_luascript>(doc, pLwc);
        }
    }
    return std::shared_ptr<litehtml::element>();
}

void litehtml::text_container::get_media_features(litehtml::media_features & media) const {
}

void litehtml::text_container::get_language(litehtml::tstring & language, litehtml::tstring & culture) const {
}
