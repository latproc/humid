
 //
//  PropertyWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/widget.h>
#include <nanogui/vscrollpanel.h>
#include "propertywindow.h"
#include "editor.h"
#include "editorwidget.h"

class Proxy {
public:

};

class ItemProxy {
public:
	ItemProxy(PropertyFormHelper *pfh, nanogui::Widget *w) : helper(pfh), item(w) {
		getAll();
	}
	void getAll() {
		if (!item) return;
		x = item->position().x();
		y = item->position().y();
		//tbd
	}
	void setAll() {
		if (!item) return;
		item->setPosition(Eigen::Vector2i(x,y));
		//tbd
	}
	void link(nanogui::Widget *w) {
		item = w;
		getAll();
		helper->clear();
		helper->addVariable("x pos", x);
		helper->addVariable("y pos", y);
	}
	int x;
	int y;
private:
	PropertyFormHelper *helper;
	nanogui::Widget *item;
};

void PropertyWindow::toggleShrunk() {
}

PropertyFormWindow::PropertyFormWindow(nanogui::Widget *parent, const std::string &title) : SkeletonWindow(parent, title), mContent(0) { }

void PropertyFormWindow::setContent(nanogui::Widget *content) { mContent = content; }

bool PropertyFormWindow::focusEvent(bool focused) {
	using namespace nanogui;
	return nanogui::Window::focusEvent(focused);
}

void PropertyFormHelper::clear() {
	using namespace nanogui;
	while (window()->childCount()) {
		window()->removeChild(0);
	}
}

PropertyWindow::PropertyWindow(nanogui::Screen *s, nanogui::Theme *theme) : screen(s) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	//properties->setFixedSize(nanogui::Vector2i(120,28));
	//item_proxy = new ItemProxy(properties, 0);
	window = properties->addWindow(Eigen::Vector2i(30, 50), "Property List");
	window->setTheme(theme);
	window->setFixedSize(nanogui::Vector2i(320,560));

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

nanogui::Window *PropertyFormHelper::addWindow(const Vector2i &pos,
																							 const std::string &title) {
	assert(mScreen);
	if (mWindow) { mWindow->decRef(); mWindow = 0; }
	PropertyFormWindow *pfw = new PropertyFormWindow(mScreen, title);
	mWindow = pfw;
	//mWindow->setSize(nanogui::Vector2i(160, 640));
	//mWindow->setFixedSize(nanogui::Vector2i(160, 640));
	mWindow->setWidth(400);
	nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
	//palette_scroller->setSize(Vector2i(mWindow->width()-60, mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	//palette_scroller->setFixedSize(Vector2i(mWindow->width()-60, mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	//palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
	mContent = new nanogui::Widget(palette_scroller);
	pfw->setContent(mContent);
	//mContent->setFixedSize(Vector2i(400,  200));
	mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
	mLayout->setMargin(1);
	mLayout->setColStretch(1, 1.0);
	mLayout->setColStretch(2, 1.0);
	mContent->setLayout(mLayout);
	mWindow->setPosition(pos);
	mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
	mWindow->setVisible(true);
	return mWindow;
}

void PropertyFormHelper::setWindow(nanogui::Window *wind) {
	assert(mScreen);
	mWindow = wind;
	mWindow->setSize(nanogui::Vector2i(wind->width(), 500));
	mWindow->setFixedSize(nanogui::Vector2i(wind->width(), 500));
	nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
	palette_scroller->setSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
	palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
	mContent = new nanogui::Widget(palette_scroller);
	PropertyFormWindow *pfw = dynamic_cast<PropertyFormWindow*>(wind);
	if (pfw) pfw->setContent(mContent);
	mContent->setFixedSize(Vector2i(palette_scroller->width()-20,  palette_scroller->height()));
	mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
	mLayout->setMargin(1);
	mLayout->setColStretch(1, 1.0);
	mLayout->setColStretch(2, 1.0);
	mContent->setLayout(mLayout);
	mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
	mWindow->setVisible(true);
}

