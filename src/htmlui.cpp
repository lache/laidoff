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