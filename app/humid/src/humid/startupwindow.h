/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __StartupWindow_h__
#define __StartupWindow_h__

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
class PanelScreen;
class ListPanel;

class StartupWindow : public Skeleton {
public:
	StartupWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }

private:
	std::string message;
	EditorGUI *gui;
	ListPanel *project_box;
};

#endif
