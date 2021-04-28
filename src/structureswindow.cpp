//
//  StructuresWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>

#include <nanogui/theme.h>
#include <nanogui/widget.h>
#include <nanogui/layout.h>
#include <nanogui/vscrollpanel.h>

#include "skeleton.h"
#include "editorgui.h"
#include "structureswindow.h"
#include "factorybuttons.h"
#include "selectable.h"

extern std::list<Structure *>hm_structures;
extern std::list<StructureClass *> hm_classes;

StructuresWindow *StructuresWindow::instance_;

StructuresWindow *StructuresWindow::create(EditorGUI *screen, nanogui::Theme *theme) {
    if (!instance_) instance_ = new StructuresWindow(screen, theme);
    return instance_;
}

StructuresWindow *StructuresWindow::instance() {
    return instance_;
}


StructuresWindow::StructuresWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(180,340));
	window->setSize(Vector2i(180,320));
	window->setPosition(Vector2i(screen->width() - 200, 40));
	window->setLayout(new GridLayout(Orientation::Vertical,1));
	window->setTitle("Structures");
	window->setVisible(false);


	{
		VScrollPanel *palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight+1));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,10));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));

		SelectableButton *b = new StructureFactoryButton(gui, "BUTTON", this, cell, 0, "BUTTON", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		Structure *s = new Structure(nullptr, "Start_Button", "BUTTON");
		starters["BUTTON"] = s;
		StructureClass *sc = new StructureClass("BUTTON", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);

		s->getProperties().add("width",120);
		s->getProperties().add("height",60);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "INDICATOR", this, cell, 0, "INDICATOR", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Indicator", "INDICATOR");
		starters["INDICATOR"] = s;
		sc = new StructureClass("INDICATOR", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);

		s->getProperties().add("width",120);
		s->getProperties().add("height",60);


		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "IMAGE", this, cell, 0, "IMAGE", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Image", "IMAGE");
		starters["IMAGE"] = s;
		sc = new StructureClass("IMAGE", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",128);
		s->getProperties().add("height",128);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "LABEL", this, cell, 0, "LABEL", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Label", "LABEL");
		starters["LABEL"] = s;
		sc = new StructureClass("LABEL", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",80);
		s->getProperties().add("height",40);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "TEXT", this, cell, 0, "TEXT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Text", "TEXT");
		starters["TEXT"] = s;
		sc = new StructureClass("TEXT", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",80);
		s->getProperties().add("height",40);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "PLOT", this, cell, 0, "PLOT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Plot", "PLOT");
		starters["PLOT"] = s;
		sc = new StructureClass("PLOT", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",256);
		s->getProperties().add("height",128);


		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "PROGRESS", this, cell, 0, "PROGRESS", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Progress", "PROGRESS");
		starters["PROGRESS"] = s;
		sc = new StructureClass("PROGRESS", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",256);
		s->getProperties().add("height",32);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "FRAME", this, cell, 0, "FRAME", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Frame", "FRAME");
		starters["FRAME"] = s;
		sc = new StructureClass("FRAME", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",128);
		s->getProperties().add("height",128);
	}

}

Structure *StructuresWindow::createStructure(const std::string kind) {
	StructureClass *sc = findClass(kind);
	if (sc) return sc->instantiate(nullptr);
	return 0;
}
