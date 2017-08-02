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

class EditorButton : public nanogui::Button, public EditorWidget {

public:

	EditorButton(Widget *parent, const std::string &btn_name, LinkableProperty *lp,
            const std::string &caption = "Untitled", bool toggle = false, int icon = 0);

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;
	void setCommand(std::string cmd);
	const std::string &command() const;

	virtual void draw(NVGcontext *ctx) override;
protected:
	bool is_toggle;
	std::string command_str;
};

#endif
