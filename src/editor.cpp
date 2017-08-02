//
//  Editor.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/widget.h>
#include <nanogui/theme.h>
#include <nanogui/screen.h>
#include <value.h>
#include <symboltable.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <nanogui/messagedialog.h>

#include "structure.h"
#include "draghandle.h"
#include "userwindow.h"
#include "editor.h"
#include "propertymonitor.h"
#include "helper.h"

extern std::list<Structure *>hm_structures;
extern std::list<StructureClass *> hm_classes;

Editor *Editor::_instance = 0;

Editor::Editor(EditorGUI *gui) :mEditMode(false), mCreateMode(false), screen(gui)  {
	_instance = this;
	drag_handle = new nanogui::DragHandle(screen, new PositionMonitor);
}

Editor *Editor::instance() {
	return _instance;
}

void Editor::setEditMode(bool which) {
	mEditMode = which;
	if (!mEditMode) {
		if (drag_handle) drag_handle->setVisible(false);
		screen->getUserWindow()->clearSelections();
	}
}

bool Editor::isEditMode() {
	return mEditMode;
}

bool Editor::isCreateMode() {
	return mCreateMode;
}

void Editor::setCreateMode(bool which) {
	mCreateMode = which;
}

void Editor::refresh(bool ) {
	if (screen) {
		if (screen->getUserWindow() ) {
			nanogui::Window *mainWindow = screen->getUserWindow()->getWindow();
			nanogui::Theme *theme = mainWindow->theme();
			theme->incRef();
			mainWindow->setTheme(new nanogui::Theme(screen->nvgContext()));
			mainWindow->setTheme(theme);
			theme->mWindowHeaderHeight = 0;
			theme->decRef();
		}

		screen->performLayout();

	}
}

nanogui::DragHandle *Editor::getDragHandle() { return drag_handle; }

void Editor::load(const std::string &path) {
	using namespace nanogui;
	UserWindow *uw = screen->getUserWindow();
	if (uw) uw->load(path);
}

void Editor::saveAs(const std::string &path) {

}
void Editor::save() {
	using namespace nanogui;
	using namespace boost::filesystem;
	UserWindow *uw = screen->getUserWindow();
	//if (uw) uw->save(path);
	Structure *settings = EditorSettings::find("EditorSettings");
	assert(settings);
	Value base_v = settings->getProperties().find("project_base");
	assert(base_v != SymbolTable::Null);

	path project_base_path(base_v.asString());
  backup_humid_files(project_base_path);

	// collect a list of files to save to
	std::map<NamedObject*, std::string> structure_files;
	for (auto s : hm_classes) {

		Value &filename = s->getProperties().find("file_name");
		std::string fname(s->getName());
		fname += ".humid";
		if (filename != SymbolTable::Null) {
			fname = filename.asString();
		}

		std::cout << "writing class " << s->getName() << " into fname\n";

		std::string file_path = base_v.asString() + "/" + fname;
		structure_files[s] = file_path;
	}
	for (auto s : hm_structures) {
		if (s->getOwner()) {
			std::cout << s->getName() << " is owened by " << s->getOwner()->getName() << "\n";
			continue;
		}
		Value &filename = s->getProperties().find("file_name");
		std::string fname(s->getName());
		fname += ".humid";
		if (filename != SymbolTable::Null) {
			fname = filename.asString();
		}

		std::cout << "filing structure " << s->getName() << " into fname\n";

		std::string file_path = base_v.asString() + "/" + fname;
		structure_files[s] = file_path;
	}
	// save to the files
	std::set<StructureClass*>saved_classes;
	for (auto item : structure_files) {
		Structure *s = dynamic_cast<Structure *>(item.first);
		StructureClass *sc = dynamic_cast<StructureClass *>(item.first);
		std::string fn = item.second;
		if (!s && sc && sc->isBuiltIn()) {
			std::cout << "skipping save of builtin: " << sc->getName() << "\n";
			continue; // skip saving builtin objects
		}
		if (s && s->getOwner()) {
			std::cout << "skipping save of owned structure " << s->getName() << "\n";
			continue; // skip items that are local to another structure
		}
		/*if (sc && findStructureFromClass(sc->getName()) == 0
				&& sc->getLocals().empty()
				&& sc->getProperties().begin() == sc->getProperties().end()) {
			std::cout << "skipping save of empty structure class " << sc->getName() << "\n";
			continue;
		}
		*/
		std::ofstream out;
		out.open(fn, std::ofstream::out | std::ofstream::app);
		if (out.fail()) {
			char buf[200];
			snprintf(buf, 200, "Could not open %s for write", fn.c_str());
			std::cerr << buf << "\n";
			continue;
		}
		if (sc) std::cout << "attempting to write structure class " << sc->getName() << "\n";
		if (sc && !sc->isBuiltIn() && !sc->save(out)) {
			char buf[200];
			snprintf(buf, 200, "error when writing structure class to %s", fn.c_str());
		}
		if (s) std::cout << "attempting to write structure instance " << s->getName() << "\n";
		if (s && !s->save(out)) {
			char buf[200];
			snprintf(buf, 200, "Error writing %s to %s", s->getName().c_str(), fn.c_str());
			std::cerr << buf << "\n";
			std::cerr << buf << "\n";
		}
			//auto dlg = new MessageDialog(EDITOR->gui()->getUserWindow()->getWindow(),
			//		MessageDialog::Type::Warning, "Error saving file", buf);
            //dlg->setCallback([](int result) {
			//	std::cout << "Dialog result: " << result << std::endl;
			//});
		out.close();
	}
}
