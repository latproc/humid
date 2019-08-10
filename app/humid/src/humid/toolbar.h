/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __Toolbar_h__
#define __Toolbar_h__

#include <ostream>
#include <string>
#include <map>
#include <list>
#include <nanogui/theme.h>
#include <humid/common/includes.hpp>
#include "skeleton.h"
#include "palette.h"
#include "linkableobject.h"
#include "editorsettings.h"

class EditorGUI;
class UserWindowWin;
class PanelScreen;
class Structure;
class PropertyFormHelper;

class Toolbar : public nanogui::Window {
public:
	Toolbar(EditorGUI *screen, nanogui::Theme *);
	nanogui::Window *getWindow() { return this; }
	bool mouseDragEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
		bool res = nanogui::Window::mouseDragEvent(p, rel, button, modifiers);
		updateSettingsStructure("ToolBar", this);
		return res;
	}
private:
	EditorGUI *gui;
};


#endif
