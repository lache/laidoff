#include "el_luascript.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "script.h"

litehtml::el_luascript::el_luascript(const std::shared_ptr<litehtml::document>& doc, LWCONTEXT* pLwc) : litehtml::html_tag(doc), pLwc(pLwc)
{
}

litehtml::el_luascript::~el_luascript()
{
}

void litehtml::el_luascript::parse_attributes()
{
	tstring text;
	get_text(text);
    LOGI("<script> tag: %s", text.c_str());

    script_evaluate_async(pLwc, text.c_str(), text.size());
}

const litehtml::tchar_t* litehtml::el_luascript::get_tagName() const {
    return "script";
}
