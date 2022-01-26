//
//  EditorTextBox.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorTextBox_h__
#define __EditorTextBox_h__

#include <ostream>
#include <string>
#include <nanogui/common.h>
#include <nanogui/textbox.h>
#include "editorwidget.h"

class EditorTextBox : public nanogui::TextBox, public EditorWidget {

public:

	EditorTextBox(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp, int icon = 0);
	virtual ~EditorTextBox() { }

	virtual nanogui::Widget *asWidget() override { return this; }

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	const std::map<std::string, std::string> & property_map() const override;
	virtual const std::map<std::string, std::string> & reverse_property_map() const override;
	virtual Value getPropertyValue(const std::string &prop) override;
	virtual void setProperty(const std::string &prop, const std::string value) override;
	std::string getScaledValue(bool scaleUp);
	float getScaledFloat(bool scaleUp);
	int getScaledInteger(bool scaleUp);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;
	virtual bool keyboardCharacterEvent(unsigned int codepoint) override;

	virtual bool focusEvent(bool focused) override;
	virtual void draw(NVGcontext *ctx) override;
	MatrixXd handle_coordinates;
	int valign = 1;
	bool wrap_text = true;
    bool auto_update = false;
};

#endif
