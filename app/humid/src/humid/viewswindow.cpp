//
//  StructuresWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/nanogui.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/theme.h>

#include "skeleton.h"
#include "structureswindow.h"
#include "editorgui.h"
#include "factorybuttons.h"
#include "viewswindow.h"
#include "objectwindow.h"
#include "patternswindow.h"
#include "structureswindow.h"
#include "screenswindow.h"

ViewsWindow::ViewsWindow(EditorGUI *screen, nanogui::Theme *theme) : gui(screen) {
	using namespace nanogui;
	properties = new FormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(200, 50), "Views");
	window->setTheme(theme);
	window->setVisible(false);
}

void ViewsWindow::addWindows() {
	add("Structures", gui->getStructuresWindow()->getWindow());
	add("Properties", gui->getPropertyWindow()->getWindow());
	add("Objects", gui->getObjectWindow()->getWindow());
	add("Patterns", gui->getPatternsWindow()->getWindow());
	add("Screens", gui->getScreensWindow()->getWindow());
}

void ViewsWindow::add(const std::string name, nanogui::Widget *w) {
	assert(w);
	properties->addVariable<bool> (
		name,
		  [&,name,this](bool value) mutable{
				nanogui::Widget *w = gui->getNamedWindow(name);
			  if (w) w->setVisible(value);
				gui->getViewManager().set(name, value);
			  EditorSettings::flush();
		  },
		  [&,name,this]()->bool{
			  return this->gui->getViewManager().get(name).visible;
		  });
}

