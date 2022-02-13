//
//  EditorList.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorList_h__
#define __EditorList_h__

#include <ostream>
#include <string>
#include <nanogui/label.h>
#include "editorwidget.h"
#include "palette.h"
#include <vector>
#include <time.h>

class EditorList : public nanogui::Widget, public EditorWidget, public Palette {

public:

	EditorList(NamedObject *owner, Widget *parent, const std::string nam,
				LinkableProperty *lp, const std::string caption,
				const std::string &font = "sans", int fontSize = -1, int icon = 0);

	virtual nanogui::Widget *asWidget() override { return this; }
	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
    virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	const std::map<std::string, std::string> & property_map() const override;
	const std::map<std::string, std::string> & reverse_property_map() const override;
	virtual Value getPropertyValue(const std::string &prop) override;
	virtual void setProperty(const std::string &prop, const std::string value) override;
	virtual void draw(NVGcontext *ctx) override;
    void performLayout(NVGcontext *ctx) override;

    const nanogui::Color &backgroundColor() { return mBackgroundColor; }
    const nanogui::Color &textColor() { return mTextColor; }

    /// Sets the background color of this Label.
    void setBackgroundColor(const nanogui::Color &backgroundColor) { mBackgroundColor = backgroundColor; }
    void setTextColor(const nanogui::Color &textColor) { mTextColor = textColor; }

    void loadItems();
	void loadItems(const std::string &str);
    const std::string & items_str();
    void setItems(const std::string & str);
	void setItemFilename(const std::string &filename);
	const std::string & item_filename();

protected:
    nanogui::Color mBackgroundColor;
    nanogui::Color mTextColor;
    nanogui::VScrollPanel *palette_scroller;
	nanogui::Widget *palette_content;
    std::vector<std::string> mItems;
    std::string m_item_str;
	std::string m_item_file;
	timespec last_file_mod = {0, 0};
	int alignment;
	int valign;
	bool wrap_text;
	char item_delimiter = ';';
};

#endif
