#include <stdio.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <locale>
#include "text_container.h"
#include "htmlui.h"
#include "lwmacro.h"
#include "lwcontext.h"
#include "file.h"
#include "render_font_test.h"

class LWHTMLUI {
public:
	LWHTMLUI(const LWCONTEXT* pLwc, int w, int h)
		: pLwc(pLwc), container(pLwc, w, h), client_width(w), client_height(h)
	{
		std::shared_ptr<char> master_css_str(create_string_from_file(ASSETS_BASE_PATH "css" PATH_SEPARATOR "master.css"), free);
		browser_context.load_master_stylesheet(master_css_str.get());
	}
	void load_page(const char* html_path) {
		std::shared_ptr<char> html_str(create_string_from_file(html_path), free);
		doc = litehtml::document::createFromString(html_str.get(), &container, &browser_context);
	}
	void render_page() {
		doc->render(client_width);
	}
	void draw() {
		litehtml::position clip(0, 0, client_width, client_height);
		doc->draw(0, 0, 0, &clip);
	}
	void on_lbutton_down(int x, int y) {
		if (doc) {
			litehtml::position::vector redraw_boxes;
			doc->on_mouse_over(x, y, x, y, redraw_boxes);
			doc->on_lbutton_down(x, y, x, y, redraw_boxes);
			last_lbutton_down_element = doc->root()->get_element_by_point(x, y, x, y);
		}
	}
	void on_lbutton_up(int x, int y) {
		if (doc) {
			litehtml::position::vector redraw_boxes;
			if (last_lbutton_down_element == doc->root()->get_element_by_point(x, y, x, y)) {
				doc->on_mouse_over(x, y, x, y, redraw_boxes);
				doc->on_lbutton_up(x, y, x, y, redraw_boxes);
			}
			last_lbutton_down_element.reset();
		}
	}
	void on_over(int x, int y) {
		if (doc) {
			litehtml::position::vector redraw_boxes;
			doc->on_mouse_over(x, y, x, y, redraw_boxes);
		}
	}
    void set_next_html_path(const char* html_path) {
        next_html_path = html_path;
    }
    void load_next_html_path() {
        if (next_html_path.empty() == false) {
            lwc_render_font_test_fbo(pLwc, next_html_path.c_str());
            next_html_path.clear();
        }
    }
private:
	LWHTMLUI();
	LWHTMLUI(const LWHTMLUI&);
	litehtml::context browser_context;
	litehtml::text_container container;
	litehtml::document::ptr doc;
	int client_width;
	int client_height;
	litehtml::element::ptr last_lbutton_down_element;
    std::string next_html_path;
    const LWCONTEXT* pLwc;
};

void* htmlui_new(const LWCONTEXT* pLwc) {
	return new LWHTMLUI(pLwc, pLwc->width, pLwc->height);
}

void htmlui_destroy(void** c) {
	delete(*c);
	*c = 0;
}

void htmlui_load_render_draw(void* c, const char* html_path) {
	LWHTMLUI* htmlui = (LWHTMLUI*)c;
	htmlui->load_page(html_path);
	htmlui->render_page();
	htmlui->draw();
}

void htmlui_on_lbutton_down(void* c, int x, int y) {
	LWHTMLUI* htmlui = (LWHTMLUI*)c;
	htmlui->on_lbutton_down(x, y);
}

void htmlui_on_lbutton_up(void* c, int x, int y) {
	LWHTMLUI* htmlui = (LWHTMLUI*)c;
	htmlui->on_lbutton_up(x, y);
}

void htmlui_on_over(void* c, int x, int y) {
	LWHTMLUI* htmlui = (LWHTMLUI*)c;
	htmlui->on_over(x, y);
}

int test_html_ui(const LWCONTEXT* pLwc) {
	litehtml::context browser_context;
	//browser_context.load_master_stylesheet()

	char* master_css_str = create_string_from_file(ASSETS_BASE_PATH "css" PATH_SEPARATOR "master.css");
	browser_context.load_master_stylesheet(master_css_str);

	const int w = pLwc->width;
	const int h = pLwc->height;
	std::shared_ptr<litehtml::text_container> container(new litehtml::text_container(pLwc, w, h));

	char* test_html_str = create_string_from_file(ASSETS_BASE_PATH "html" PATH_SEPARATOR "HTMLPage1.html");
	auto doc = litehtml::document::createFromString(test_html_str, container.get(), &browser_context);

	//auto elm = doc->root()->select_one(_t("#value3"));
	//elm->get_child(0)->set_data(_t("HELLO!!!"));
	//litehtml::tstring str;
	//elm->get_text(str);

	doc->render(w);
	litehtml::position clip(0, 0, w, h);
	doc->draw(0, 0, 0, &clip);
	
	litehtml::position::vector position_vector;
	doc->on_mouse_over(13, 19, 13, 19, position_vector);
	doc->on_lbutton_up(13, 19, 13, 19, position_vector);
	printf("Hey\n");

	free(master_css_str);
	free(test_html_str);

	return 0;
}

void htmlui_set_next_html_path(void* c, const char* html_path) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_next_html_path(html_path);
}

void htmlui_load_next_html_path(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_next_html_path();
}
