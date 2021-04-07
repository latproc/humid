#pragma once

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/object.h>
#include <nanogui/formhelper.h>

#include "editorgui.h"

class DialogWindow : public nanogui::Window {
public:
	DialogWindow(EditorGUI *screen, nanogui::Theme *theme);
	nanogui::Window *getWindow() { return this; }

	Structure *structure() { return current_structure; }
	void setStructure( Structure *s);
	void loadStructure( Structure *s);
	void clear();
private:
	EditorGUI *gui;
	Structure *current_structure;
};

