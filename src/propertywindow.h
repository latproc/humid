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

#include "editorgui.h"

class PropertyFormHelper;

class PropertyWindow : public nanogui::Object {
public:
	PropertyWindow(nanogui::Screen *screen, nanogui::Theme *theme);
	void setVisible(bool which);
	nanogui::Window *getWindow()  { return window; }
	PropertyFormHelper *getFormHelper() { return properties; }
	void update();
	nanogui::Screen *getScreen() { return screen; }
	void show(nanogui::Widget &w);

protected:
	nanogui::Screen *screen;
	nanogui::Window *window;
	PropertyFormHelper *properties;
	nanogui::Widget *items;
};

#endif
