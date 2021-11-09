#pragma once

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/theme.h>

class EditorGUI;
class Structure;

class DialogWindow : public nanogui::Window {
public:
	DialogWindow(EditorGUI *screen, nanogui::Theme *theme);
	nanogui::Window *getWindow() { return this; }

	Structure *structure() { return current_structure; }
	void setStructure( Structure *s);
	void loadStructure( Structure *s);
	void clear();
private:
	EditorGUI *gui = nullptr;
	Structure *current_structure = nullptr;
};

