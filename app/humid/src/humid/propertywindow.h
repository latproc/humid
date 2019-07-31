//
//  PropertyWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __PropertyWindow_h__
#define __PropertyWindow_h__

#include <ostream>
#include <string>

#include <nanogui/common.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/screen.h>
#include <nanogui/widget.h>
#include <nanogui/formhelper.h>
#include "shrinkable.h"
#include "skeleton.h"

#include "editorgui.h"


class PropertyFormWindow : public SkeletonWindow {
public:
	PropertyFormWindow(nanogui::Widget *parent, const std::string &title = "Untitled");
	virtual ~PropertyFormWindow() {}

	void setContent(nanogui::Widget *content);

	virtual bool focusEvent(bool focused) override;
private:
	nanogui::Widget *mContent;
};

class PropertyFormHelper : public nanogui::FormHelper {
public:
	PropertyFormHelper(nanogui::Screen *screen) : nanogui::FormHelper(screen), mContent(0) { }
	void clear();

	nanogui::Window *addWindow(const nanogui::Vector2i &pos,
														 const std::string &title = "Untitled") override;

	void setWindow(nanogui::Window *wind) override;

	nanogui::Widget *content() override;
private:
	nanogui::Widget *mContent;
};

class PropertyWindow : public nanogui::Object, public Shrinkable {
public:
	PropertyWindow(nanogui::Screen *screen, nanogui::Theme *theme);
	void setVisible(bool which);
	nanogui::Window *getWindow()  { return window; }
	PropertyFormHelper *getFormHelper() { return properties; }
	void update();
	nanogui::Screen *getScreen() { return screen; }
	void show(nanogui::Widget &w);
	void toggleShrunk();

protected:
	nanogui::Screen *screen;
	nanogui::Window *window;
	PropertyFormHelper *properties;
	nanogui::Widget *items;
};

#endif
