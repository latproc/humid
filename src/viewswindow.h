#pragma once

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/object.h>
#include <nanogui/formhelper.h>

#include "editorgui.h"

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
