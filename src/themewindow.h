//
//  ThemeWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __ThemeWindow_h__
#define __ThemeWindow_h__

#include <ostream>
#include <string>

#include <nanogui/object.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>

#include "editorgui.h"

class PropertyFormHelper;

class ThemeWindow : public nanogui::Object {
public:
	ThemeWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void loadTheme(nanogui::Theme*);
private:
	EditorGUI *gui;
	nanogui::Window *window;
	PropertyFormHelper *properties;
};

#endif
