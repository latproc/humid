//
//  EditorClock.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorClock_h__
#define __EditorClock_h__

#include <ostream>
#include <string>
#include <nanogui/label.h>
#include "editorwidget.h"

// EditorClock displays the local panel system clock. Unlike labels and
// buttons it is not driven by Clockwork updates: the text it shows is derived
// from the local wall-clock time whenever the widget is drawn, so the running
// main loop keeps it current automatically. The displayed content is
// controlled by a strftime() format string supplied through the "format"
// property, and its size, font, colours, alignment, border and visibility are
// all configured through the same setup-file properties as other widgets.

class EditorClock : public nanogui::Label, public EditorWidget {

public:

	EditorClock(NamedObject *owner, Widget *parent, const std::string nam,
				LinkableProperty *lp);

	virtual nanogui::Widget *asWidget() override { return this; }

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
    virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	const std::map<std::string, std::string> & property_map() const override;
	const std::map<std::string, std::string> & reverse_property_map() const override;
	virtual Value getPropertyValue(const std::string &prop) override;
	virtual void setProperty(const std::string &prop, const std::string value) override;
	virtual void draw(NVGcontext *ctx) override;

    const nanogui::Color &backgroundColor() { return mBackgroundColor; }
    const nanogui::Color &textColor() { return mTextColor; }

    /// Sets the background color of this Clock.
    void setBackgroundColor(const nanogui::Color &backgroundColor) { mBackgroundColor = backgroundColor; }
    void setTextColor(const nanogui::Color &textColor) { mTextColor = textColor; }

    /// strftime() format used to render the local time.
    void setFormat(const std::string &format) { time_format = format; }
    const std::string &format() const { return time_format; }

protected:
    nanogui::Color mBackgroundColor;
    nanogui::Color mTextColor;
	int alignment;
	int valign;
	std::string time_format;
};

#endif
