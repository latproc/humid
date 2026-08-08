//
//  editorhtmlview.h
//  Project: humid
//
//  Embedded HTML document viewer: URL from CW remote, litehtml + Cairo render.
//  Supports in-document #anchors, keyboard scroll, and a Jump to Top control.
//

#ifndef __editorhtmlview_h__
#define __editorhtmlview_h__

#include "editorwidget.h"
#include <nanogui/widget.h>
#include <string>
#include <memory>
#include <vector>

class HtmlViewContainer;

namespace litehtml {
class document;
}

class EditorHtmlView : public nanogui::Widget, public EditorWidget {
public:
	EditorHtmlView(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp);
	~EditorHtmlView() override;

	nanogui::Widget *asWidget() override { return this; }

	bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
						  int modifiers) override;
	bool mouseEnterEvent(const nanogui::Vector2i &p, bool enter) override;
	bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
	bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
	void draw(NVGcontext *ctx) override;

	void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper *properties) override;
	void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	const std::map<std::string, std::string> &property_map() const override;
	const std::map<std::string, std::string> &reverse_property_map() const override;
	Value getPropertyValue(const std::string &prop) override;
	void setProperty(const std::string &prop, const std::string value) override;

	void setUrl(const std::string &url, bool force_reload = false);
	const std::string &url() const { return m_url; }
	const std::string &status() const { return m_status; }

	void setScrollY(int y);
	int scrollY() const { return m_scroll_y; }
	int contentHeight() const { return m_content_height; }
	void jumpToTop();
	bool jumpToFragment(const std::string &fragment);

private:
	void reload();
	void paintViewport();
	void releaseNvgImage();
	void releaseDocument();
	void onAnchorClick(const std::string &href);
	void applyPendingFragment();
	nanogui::Vector2i docCoords(const nanogui::Vector2i &widget_p) const;
	bool hitTopButton(const nanogui::Vector2i &p) const;
	nanogui::Vector4i topButtonRect() const; // x,y,w,h in widget coords

	std::string m_url;
	std::string m_status;
	std::string m_pending_fragment; // fragment to apply after load/layout
	std::unique_ptr<HtmlViewContainer> m_container;
	std::shared_ptr<litehtml::document> m_doc;
	int m_scroll_y = 0;
	int m_content_height = 0;
	int m_content_width = 0;
	bool m_need_paint = true;
	std::vector<unsigned char> m_rgba;
	int m_rgba_w = 0;
	int m_rgba_h = 0;
	int m_nvg_image = -1;
	int m_drag_y = -1;
	int m_drag_scroll0 = 0;
	bool m_mouse_down = false;
	bool m_click_on_top_btn = false;
};

#endif
