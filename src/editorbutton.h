//
//  EditorButton.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorButton_h__
#define __EditorButton_h__

#include <ostream>
#include <string>
#include <map>

#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <nanogui/button.h>
#include "editorwidget.h"

using nanogui::Widget;
class EditorGUI;

class EditorButton : public nanogui::Button, public EditorWidget {

public:

	EditorButton(NamedObject *owner, Widget *parent, const std::string &btn_name, LinkableProperty *lp,
            const std::string &caption = "Untitled", bool toggle = false, int icon = 0);

	virtual nanogui::Widget *asWidget() override { return this; }
	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;
	virtual void setProperty(const std::string &prop, const std::string value) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;
	void setCommand(std::string cmd);
	const std::string &command() const;

	void setupButtonCallbacks(LinkableProperty *lp, EditorGUI *gui);

	void setOnColor(nanogui::Color c) { bg_on_color = c; }
	nanogui::Color &onColor() { return bg_on_color; }
	void setOnTextColor(nanogui::Color c) { on_text_colour = c; }
	nanogui::Color &onTextColor() { return on_text_colour; }

	void setOnCaption(const std::string c) { on_caption = c; }
	const std::string &onCaption() { return on_caption; }

	virtual void draw(NVGcontext *ctx) override;
protected:
	bool is_toggle;
	std::string command_str;
	nanogui::Color bg_on_color;
	nanogui::Color on_text_colour;
	std::string on_caption;
	int alignment;
	int valign;
	bool wrap_text;
};

#endif
