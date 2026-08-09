//
//  editordocview.h
//  Project: humid
//
//  Multi-page drawing viewer: offline PDF→PNG pages served over HTTP.
//  Operator controls are keyboard pan/zoom (+ page buttons); mouse pan is off.
//

#ifndef __editordocview_h__
#define __editordocview_h__

#include <ostream>
#include <string>
#include <nanogui/imageview.h>
#include "editorwidget.h"

class EditorDocView : public nanogui::ImageView, public EditorWidget {

public:
	EditorDocView(NamedObject *owner, Widget *parent, const std::string nam,
				  LinkableProperty *lp, GLuint image_id = 0);
	~EditorDocView() override;

	nanogui::Widget *asWidget() override { return this; }

	bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
						  int modifiers) override;
	bool mouseDragEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
						int modifiers) override;
	bool mouseEnterEvent(const nanogui::Vector2i &p, bool enter) override;
	bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
	bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
	bool keyboardCharacterEvent(unsigned int codepoint) override;
	void draw(NVGcontext *ctx) override;

	void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper *properties) override;
	void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	const std::map<std::string, std::string> &property_map() const override;
	const std::map<std::string, std::string> &reverse_property_map() const override;
	Value getPropertyValue(const std::string &prop) override;
	void setProperty(const std::string &prop, const std::string value) override;

	/// Base URL or directory for pages, e.g. "http://host:8767/elec-0297/"
	void setSource(const std::string &src, bool reload = true);
	const std::string &source() const { return m_source; }

	void setPage(int page, bool reload = true);
	int page() const { return m_page; }

	void setPageCount(int n);
	int pageCount() const { return m_page_count; }

	void setInteractive(bool value);
	bool interactive() const { return m_interactive; }

	void nextPage();
	void prevPage();
	void fitView();

	/// Built page URL for the current page (source + page-00N.png).
	std::string pageUrl() const;

protected:
	void loadCurrentPage(bool reload);
	void bindImageId(GLuint img);
	void drawChrome(NVGcontext *ctx);
	nanogui::Vector4i prevButtonRect() const;
	nanogui::Vector4i nextButtonRect() const;
	nanogui::Vector4i fitButtonRect() const;
	bool hitRect(const nanogui::Vector4i &r, const nanogui::Vector2i &local_p) const;
	nanogui::Vector2i toLocal(const nanogui::Vector2i &parent_p) const { return parent_p - mPos; }

	std::string m_source;
	int m_page = 1;
	int m_page_count = 1;
	bool m_interactive = true;
	bool m_need_fit = true;
	std::string m_status; // brief status for failed fetch
};

#endif
