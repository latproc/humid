//
//  ScreensWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/nanogui.h>
#include "editorgui.h"
#include "skeleton.h"
#include "palette.h"
#include "screenswindow.h"
#include "userwindow.h"
#include "userwindowwin.h"
#include "structure.h"
#include "helper.h"
#include "selectable.h"
#include "selectablebutton.h"
#include "editor.h"

using nanogui::Vector2i;
extern std::list<Structure *>hm_structures;

class ScreenSelectButton : public SelectableButton {
	public:
	ScreenSelectButton(const std::string kind, Palette *pal, nanogui::Widget *parent,
			const std::string &caption, ScreensWindow*sw)
		: SelectableButton(kind, pal, parent, caption),  screens_window(sw) {
			screen = findScreen(caption);
#if 0
			if (screen) {
				std::cout << "screen select button " << caption << " selects structure " << screen->getName() << ":" << screen->getKind() << "\n";
			}
			else
			{
				std::cout << "screen select button " << caption << " has no screen\n";
			}
#endif
		}
	virtual void justDeselected() override {
		UserWindow *uw = screens_window->getUserWindow();
		if (uw) {
			uw->clear();
		}
	}
	virtual void justSelected() override {
		UserWindow *uw = EDITOR->gui()->getUserWindow();
		if (uw)
			uw->clearSelections();
		if (!getScreen())
			setScreen(findScreen(caption()));
		if (uw && getScreen()) {
			// in edit mode the active screen is set by user actions.
			// otherwise it is only changed by a property change from the remote end
			EditorGUI::systemSettings()->getProperties().add("active_screen", Value(getScreen()->getName(), Value::t_string));
			uw->setStructure(getScreen());
			uw->refresh();
		}
	}

	void setScreen( Structure *s) {
		screen = s;
	}
	Structure *getScreen() { return screen; }
	private:
	ScreensWindow *screens_window;
	Structure *screen;
};

ScreensWindow::ScreensWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen),
		Palette(PT_SINGLE_SELECT), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(220,320));
	window->setSize(Vector2i(220,320));
	window->setPosition(Vector2i(screen->width() - 220, 40));
	window->setLayout(new BoxLayout(Orientation::Vertical,Alignment::Fill));
	window->setTitle("Screens");
	window->setVisible(false);

	const int tool_button_size = 32;
	Widget *items = new Widget(window);
	items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight+1));
	items->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill));
	items->setFixedSize(Vector2i(window->width(), tool_button_size+5));
	items->setSize(Vector2i(window->width(), tool_button_size+5));

	ToolButton *tb = new ToolButton(items, ENTYPO_ICON_PLUS);
	tb->setFlags(Button::NormalButton);
	tb->setFixedSize(Vector2i(tool_button_size, tool_button_size));
	tb->setSize(Vector2i(tool_button_size, tool_button_size));
	//tb->setPosition(Vector2i(32, 64));
	UserWindow *uw = gui->getUserWindow();
	tb->setCallback([this, uw]() {
		uw->clearSelections();

		createScreenStructure();
		if (gui->getScreensWindow()) {
			this->update();
			this->selectFirst();
		}
	});

	{
		palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight - tool_button_size));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight + 10 + tool_button_size));
		palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,100));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
	}
	update();
	window->performLayout(gui->nvgContext());
}

void ScreensWindow::clearSelections(Selectable * except) {
	if (except == 0)
		int x = 1;
	Palette::clearSelections(except);
	reset();
}

UserWindow *ScreensWindow::getUserWindow() {
	return gui->getUserWindow();
}

Structure *ScreensWindow::getSelectedStructure() {
	if (!hasSelections()) return 0;
	auto found = selections.begin();
	ScreenSelectButton *btn = dynamic_cast<ScreenSelectButton*>(*found);
	if (btn)
		return btn->getScreen();
	return 0;
}

void ScreensWindow::updateSelectedName() {
	if (!hasSelections()) return;
	auto found = selections.begin();
	ScreenSelectButton *btn = dynamic_cast<ScreenSelectButton*>(*found);
	if (btn) {
		Structure *s = btn->getScreen();
		btn->setCaption(s->getName());
	}
}

void ScreensWindow::update() {
	gui->getUserWindow()->clearSelections();

	if (getWindow()->visible())
		getWindow()->requestFocus();
	Selectable *current = nullptr;
	//if (hasSelections())
		//current = *(selections.begin());
	clearSelections();
	int button_width = window->width() - 20;
	int n = palette_content->childCount();
	while (n--) {
		palette_content->removeChild(0);
	}
	EditorGUI *app = gui;

	// check whether all screens are instantiated
	int created = createScreens();
	if (created)
    {
		// std::cout << "Warning: " << created << " screen instances were missing!\n";
    }

	for (auto item : hm_structures ) {
		Structure *s = item;
		//std::cout << "checking if structure " << item << " (" << s->getKind() << ") is a screen\n";
		StructureClass *sc = s->getStructureDefinition(); //findClass(s->getKind());
		int count = 0;
		if (s->getKind() == "SCREEN" || (sc && sc->getBase() == "SCREEN") ) {
			++count;
			nanogui::Widget *cell = new nanogui::Widget(palette_content);
			cell->setFixedSize(Vector2i(button_width+4,35));
			ScreenSelectButton *b = new ScreenSelectButton("BUTTON", this, cell, s->getName(), this);
			b->setEnabled(true);
			b->setFixedSize(Vector2i(button_width, 30));
			b->setPosition(Vector2i(2,2));
			b->setPassThrough(true);
			b->setCallback( [app,b](){
				app->getUserWindow()->clearSelections();
				app->getScreensWindow()->getWindow()->requestFocus();
				app->getScreensWindow()->clearSelections(b);
				b->select();
			});
		}
		//else
		//	std::cout << s->getName() << ":" << ((sc) ? sc->getName() : "null") << " is not a screen\n";
	}

	if (!palette_content->childCount()) {
			Structure *screen = createScreenStructure();
			nanogui::Widget *cell = new nanogui::Widget(palette_content);
			cell->setFixedSize(Vector2i(button_width+4,35));
			ScreenSelectButton *b = new ScreenSelectButton("BUTTON", this, cell, screen->getName(), this);
			b->setEnabled(true);
			b->setFixedSize(Vector2i(button_width, 30));
			b->setPosition(Vector2i(2,2));
			b->setPassThrough(true);
			b->setCallback( [app,b](){
				app->getScreensWindow()->getWindow()->requestFocus();
				app->getUserWindow()->clearSelections();
				app->getScreensWindow()->clearSelections(b);
				b->select();
			});
	}
	getWindow()->performLayout(gui->nvgContext());
	if (EDITOR->isEditMode()) selectFirst();
	/*if (current) {
		SelectableButton *btn = dynamic_cast<SelectableButton*>(current);
		if (btn)
			btn->callback()();
	}*/

}

void ScreensWindow::selectFirst() {
	if (hasSelections()) return;
	int n = palette_content->childCount();
	if (!n) return;
	nanogui::Widget *cell = palette_content->childAt(0);
	if (!cell->childCount()) return;
	Selectable *s = dynamic_cast<Selectable*>(cell->childAt(0));
	if (s) s->select();
}


