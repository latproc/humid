//
//  StructuresWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <string>

#include "skeleton.h"
#include "toolbar.h"

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/formhelper.h>
#include <nanogui/theme.h>
#include <nanogui/tabwidget.h>
#include <nanogui/common.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>

#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <libgen.h>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <lib_clockwork_client.hpp>
#include "includes.hpp"
#include "list_panel.h"
#include "userwindowwin.h"
#include "toolbar.h"
#include "propertywindow.h"
#include "viewswindow.h"

#ifndef ENTYPO_ICON_LAYOUT
#define ENTYPO_ICON_LAYOUT                              0x0000268F
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2f;
using Eigen::Vector2d;
using Eigen::MatrixXd;
using Eigen::Matrix3d;


Toolbar::Toolbar(EditorGUI *screen, nanogui::Theme *theme) : nanogui::Window(screen), gui(screen) {
	using namespace nanogui;
	Window *toolbar = this;
	toolbar->setTheme(theme);
	ToolButton *tb = new ToolButton(toolbar, ENTYPO_ICON_PENCIL);
	tb->setFlags(Button::ToggleButton);
	tb->setFixedSize(Vector2i(32,32));
	tb->setPosition(Vector2i(32, 64));
	tb->setChangeCallback([this](bool state) {
		Editor *editor = EDITOR;
		if (state)
			editor->gui()->setState(EditorGUI::GUIEDITMODE);
		else
			editor->gui()->setState(EditorGUI::GUIWORKING);
		if (editor) {
			editor->setEditMode(state);
		}
	});
	tb = new ToolButton(toolbar, ENTYPO_ICON_NEW);
	tb->setFlags(Button::NormalButton);
	tb->setTooltip("New Project");
	tb->setFixedSize(Vector2i(32,32));

	tb = new ToolButton(toolbar, ENTYPO_ICON_TEXT_DOCUMENT);
	tb->setFixedSize(Vector2i(32,32));
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Open Project");
	tb->setFlags(Button::NormalButton);
	tb->setCallback([this] {
		Editor *editor = EDITOR;
		if (editor) {
			std::string file_path(file_dialog({{"humid_project", "Humid Project File"}}, false));
			// std::string file_path(file_dialog({{"humid_project", "Humid Project File"},  {"humid", "Humid layout file"}}, false));
			if (file_path.length()) {
				//TODO: unload current project
				editor->load(file_path);
				editor->gui()->getScreensWindow()->update();
			}
		}
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_SAVE);
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Save Project");
	tb->setFlags(Button::NormalButton);
	tb->setCallback([this] {
		Editor *editor = EDITOR;
		if (editor) {
			Structure *es = EditorSettings::find("EditorSettings");
			if (!es) es = EditorSettings::create();
			const Value &base_v(es->getProperties().find("project_base"));
			if (base_v == SymbolTable::Null) {
				std::string file_path(file_dialog(
					{ {"humid", "Humid layout file"},
					{"txt", "Text file"} }, true));
				if (file_path.length()) {
					boost::filesystem::path base(file_path);
					base = base.parent_path();
					assert(boost::filesystem::is_directory(base));
					editor->saveAs(base.native());
					editor->gui()->updateProperties();
					Structure *s = editor->gui()->getSettings();
					s->getProperties().add("project_base", Value(base.string(), Value::t_string));
					EditorSettings::setDirty(); // TBD fix this
					EditorSettings::flush();
				}
			}
			else {
				editor->save();
			}
		}
	});
	tb->setFixedSize(Vector2i(32,32));

	tb = new ToolButton(toolbar, ENTYPO_ICON_PRICE_TAG);
	tb->setFlags(Button::NormalButton);
	tb->setTooltip("Tags");
	tb->setFixedSize(Vector2i(32,32));
	tb->setCallback([&] {
		std::string tags(file_dialog(
		  {{"csv", "Clockwork TAG file"}, {"txt", "Text file"} }, false));
		gui->getObjectWindow()->loadTagFile(tags);
	});

//	tb = new ToolButton(toolbar, ENTYPO_ICON_INSTALL);
//	tb->setTooltip("Refresh");
//	tb->setFixedSize(Vector2i(32,32));
//	tb->setChangeCallback([this](bool state) {
//	});
//
	ToolButton *settings_button = new ToolButton(toolbar, ENTYPO_ICON_COG);
	settings_button->setFixedSize(Vector2i(32,32));
	settings_button->setTooltip("Theme properties");
	settings_button->setChangeCallback([this](bool state) {
		this->gui->getThemeWindow()->setVisible(state);
		if (state)
			this->gui->getThemeWindow()->getWindow()->requestFocus();
	});

//	tb = new ToolButton(toolbar, ENTYPO_ICON_LAYOUT);
//	tb->setTooltip("Create");
//	tb->setFixedSize(Vector2i(32,32));
//	tb->setChangeCallback([](bool state) { });
//
	tb = new ToolButton(toolbar, ENTYPO_ICON_EYE);
	tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Views");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([this](bool state) {
		this->gui->getViewsWindow()->setVisible(state);
		this->gui->getViewsWindow()->getWindow()->requestFocus();
 	});

	BoxLayout *bl = new BoxLayout(Orientation::Horizontal);
	toolbar->setLayout(bl);
	toolbar->setTitle("Toolbar");

	toolbar->setVisible(false);
}

