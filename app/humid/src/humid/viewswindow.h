/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __ViewsWindow_h__
#define __ViewsWindow_h__

#include <ostream>
#include <string>
#include <map>
#include <list>
#include <nanogui/theme.h>
#include <humid/common/includes.hpp>
#include "skeleton.h"
#include "palette.h"
#include "linkableobject.h"

class EditorGUI;
class UserWindowWin;
class PanelScreen;
class Structure;
class PropertyFormHelper;

class ViewsWindow : public nanogui::Object {
public:
	ViewsWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void addWindows();
	void add(const std::string name, nanogui::Widget *);
private:
	EditorGUI *gui;
	nanogui::Window *window;
	nanogui::FormHelper *properties;
};

#endif
