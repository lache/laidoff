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
#include "lwtcp.h"
#include "lwmutex.h"

class LWHTMLUI {
public:
    LWHTMLUI(LWCONTEXT* _pLwc, int w, int h)
        : pLwc(_pLwc)
        , container(_pLwc, w, h)
        , client_width(w)
        , client_height(h)
        , refresh_html_body(0) {
        LWMUTEX_INIT(parsing_mutex);
        std::shared_ptr<char> master_css_str(create_string_from_file(ASSETS_BASE_PATH "css" PATH_SEPARATOR "master.css"), free);
        browser_context.load_master_stylesheet(master_css_str.get());

        browser_context.set_loop_var_map(
            {
                { "button-loop" ,
            {
                {
                    "name", { "button1", "button2" }
                },{
                    "desc", { "button1-desc", "button2-desc" }
                },{
                    "script",{ "script:call_me(1)", "script:call_me(2)" }
                }, } }, });
    }
    ~LWHTMLUI() {
        LWMUTEX_DESTROY(parsing_mutex);
    }
    void set_loop_var_map_entry(const std::string& loop_name, const std::map<std::string, std::vector<std::string> >& var_map) {
        browser_context.set_loop_var_map_entry(loop_name, var_map);
    }
    void set_loop_key_value(const std::string& loop_name, const std::string& key, const std::string& value) {
        lock();
        browser_context.set_loop_key_value(loop_name, key, value);
        unlock();
    }
    void clear_loop(const std::string& loop_name) {
        lock();
        browser_context.clear_loop(loop_name);
        unlock();
    }
    void load_page(const char* html_path) {
        std::shared_ptr<char> html_str(create_string_from_file(html_path), free);
        lock();
        doc = litehtml::document::createFromString(html_str.get(), &container, &browser_context);
        unlock();
    }
    void load_body(const char* html_body) {
        lock();
        doc = litehtml::document::createFromString(html_body, &container, &browser_context);
        unlock();
    }
    void lock() {
        LWMUTEX_LOCK(parsing_mutex);
    }
    void unlock() {
        LWMUTEX_UNLOCK(parsing_mutex);
    }
    void render_page() {
        doc->render(client_width);
    }
    void draw() {
        litehtml::position clip(0, 0, client_width, client_height);
        doc->draw(0, 0, 0, &clip);
    }
    void on_lbutton_down(float nx, float ny) {
        if (doc) {
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height));
            litehtml::position::vector redraw_boxes;
            doc->on_mouse_over(x, y, x, y, redraw_boxes);
            doc->on_lbutton_down(x, y, x, y, redraw_boxes);
            last_lbutton_down_element = doc->root()->get_element_by_point(x, y, x, y);
        }
    }
    void on_lbutton_up(float nx, float ny) {
        if (doc) {
            litehtml::position::vector redraw_boxes;
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height));
            if (last_lbutton_down_element == doc->root()->get_element_by_point(x, y, x, y)) {
                doc->on_mouse_over(x, y, x, y, redraw_boxes);
                doc->on_lbutton_up(x, y, x, y, redraw_boxes);
            }
            last_lbutton_down_element.reset();
        }
    }
    void on_over(float nx, float ny) {
        if (doc) {
            litehtml::position::vector redraw_boxes;
            int x = static_cast<int>(roundf(nx * client_width));
            int y = static_cast<int>(roundf(ny * client_height));
            doc->on_mouse_over(x, y, x, y, redraw_boxes);
        }
    }
    void set_next_html_path(const char* html_path) {
        next_html_path = html_path;
    }
    void set_refresh_html_body(int v) {
        refresh_html_body = v;
    }
    void load_next_html_path() {
        if (next_html_path.empty() == false) {
            lwc_render_font_test_fbo(pLwc, next_html_path.c_str());
            next_html_path.clear();
        }
    }
    void load_next_html_body() {
        if (refresh_html_body) {
            lwc_render_font_test_fbo_body(pLwc, pLwc->tcp_ttl->html_body);
            refresh_html_body = 0;
        }
    }
    void set_online(bool b) {
        container.set_online(b);
    }
private:
    LWHTMLUI();
    LWHTMLUI(const LWHTMLUI&);
    const LWCONTEXT* pLwc;
    litehtml::context browser_context;
    litehtml::text_container container;
    litehtml::document::ptr doc;
    int client_width;
    int client_height;
    litehtml::element::ptr last_lbutton_down_element;
    std::string next_html_path;
    int refresh_html_body;
    LWMUTEX parsing_mutex;
};

void* htmlui_new(LWCONTEXT* pLwc) {
    return new LWHTMLUI(pLwc, pLwc->width, pLwc->height);
}

void htmlui_destroy(void** c) {
    LWHTMLUI** htmlui = (LWHTMLUI**)c;
    delete(*htmlui);
    *htmlui = 0;
}

void htmlui_load_render_draw(void* c, const char* html_path) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_page(html_path);
    htmlui->render_page();
    htmlui->draw();
}

void htmlui_load_render_draw_body(void* c, const char* html_body) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_body(html_body);
    htmlui->render_page();
    htmlui->draw();
}

void htmlui_on_lbutton_down(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_lbutton_down(nx, ny);
}

void htmlui_on_lbutton_up(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_lbutton_up(nx, ny);
}

void htmlui_on_over(void* c, float nx, float ny) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->on_over(nx, ny);
}

int test_html_ui(LWCONTEXT* pLwc) {
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

void htmlui_set_refresh_html_body(void* c, int v) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_refresh_html_body(v);
}

void htmlui_load_next_html_body(void* c) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->load_next_html_body();
}

void htmlui_clear_loop(void* c, const char* loop_name) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->clear_loop(loop_name);
}

void htmlui_set_loop_key_value(void* c, const char* loop_name, const char* key, const char* value) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_loop_key_value(loop_name, key, value);
}

void htmlui_set_online(void* c, int b) {
    LWHTMLUI* htmlui = (LWHTMLUI*)c;
    htmlui->set_online(b ? true : false);
}
