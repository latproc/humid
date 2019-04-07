//
//  PropertyWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "propertywindow.h"
#include "editor.h"
#include "editorwidget.h"

void PropertyWindow::toggleShrunk() {
}


PropertyWindow::PropertyWindow(nanogui::Screen *s, nanogui::Theme *theme) : screen(s) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	//properties->setFixedSize(nanogui::Vector2i(120,28));
	//item_proxy = new ItemProxy(properties, 0);
	window = properties->addWindow(Eigen::Vector2i(30, 50), "Property List");
	window->setTheme(theme);
	window->setFixedSize(nanogui::Vector2i(260,560));

	window->setVisible(false);
}

void PropertyWindow::setVisible(bool which) { window->setVisible(which); }

void PropertyWindow::update() {
	EditorGUI *gui = dynamic_cast<EditorGUI*>(screen);
	UserWindow *uw = 0;
	if (gui) {
		uw = gui->getUserWindow();
		if (!uw) return;
		nanogui::Window *pw =getWindow();
		if (!pw) return;

		properties->clear();
		properties->setWindow(pw); // reset the grid layout
		pw->setVisible(EDITOR->isEditMode() && gui->getViewManager().get("Properties").visible);
		int n = pw->children().size();

		if (uw->getSelected().size()) {
			// collect a map of all properties to their objects and then load the
			// properties that are common to all selected objects
			int num_sel = uw->getSelected().size();
			if (num_sel>1) {
				std::multimap<std::string, EditorWidget*> items;
				std::set<std::string>non_shared_properties;
				non_shared_properties.insert("Name");
				non_shared_properties.insert("Structure");
				for (auto sel : uw->getSelected()) {
					EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
					if (ew) {
						std::list<std::string> names;
						ew->getPropertyNames(names);
						for (auto pn : names) {
							if (non_shared_properties.count(pn) == 0)
								items.insert( std::make_pair(pn, ew) );
						}
					}
				}
				std::set<std::string> common;
				std::string last;
				unsigned int count = 0;
				for (auto pmap : items) {
					if (pmap.first == last) ++count;
					else {
						if (count == num_sel) common.insert(last);
						last = pmap.first;
						count = 1;
					}
				}
				if (count == num_sel) common.insert(last);
				if (common.size()) {
					std::list<EditorWidget *>widgets;
					for (auto sel : uw->getSelected()){
						EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
						assert(ew);
						widgets.push_back(ew);
					}
					for (auto prop : common) {
						std::string label(prop);
						properties->addVariable<std::string>(label,
																								 [widgets, label](std::string value) {
																									 for (auto sel : widgets) {
																										 assert(sel);
																										 sel->setProperty(label, value);
																									 }
																								 },
																								 [widgets, label]()->std::string{
																									 EditorWidget *ew = widgets.front();
																									 assert(ew);
																									 return ew->getProperty(label);
																								 });
					}
				}
				else {
					// dummy
					std::string label("dummy");
					int val;
					properties->addVariable<int>(label,
						 [val](int value) mutable { val = value; },
						 [val]()->int{ return val; });
				}

			}
			else {
				for (auto sel : uw->getSelected()) {
					nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
					EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
					if (ew) {
						ew->loadProperties(properties);
					}
					else if (w && sel->isSelected()) {
						std::string label("Width");
						properties->addVariable<int>(label,
																				 [w](int value) { w->setWidth(value); },
																				 [w]()->int{ return w->width(); });
					}
					break;
				}
			}
		}
		else {
			uw->loadProperties(properties);
		}
		gui->performLayout();
	}
}
