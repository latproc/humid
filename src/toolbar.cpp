#include <list>
#include <string>

#include <nanogui/window.h>
#include <nanogui/toolbutton.h>
#include <nanogui/entypo.h>
#include <nanogui/layout.h>
#include <nanogui/formhelper.h>

#include <boost/filesystem.hpp>

#include "editor.h"
#include "editorgui.h"
#include "toolbar.h"
#include "screenswindow.h"
#include "objectwindow.h"
#include "viewswindow.h"

extern std::list<std::string> source_files;

#ifndef ENTYPO_ICON_LAYOUT
#define ENTYPO_ICON_LAYOUT                              0x0000268F
#endif

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

	tb = new ToolButton(toolbar, ENTYPO_ICON_NOTE);
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Open Project");
	tb->setFlags(Button::NormalButton);
	tb->setCallback([this] {
		Editor *editor = EDITOR;
		if (editor) {
			std::string file_path(file_dialog(
				{ {"humid", "Humid layout file"} }, false));
			if (file_path.length()) {
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
					editor->saveAs(file_path);
					editor->gui()->updateProperties();
					Structure *s = editor->gui()->getSettings();
					boost::filesystem::path base(source_files.front());
					if (boost::filesystem::is_regular_file(base))
						base = base.parent_path();
					assert(boost::filesystem::is_directory(base));
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

	tb = new ToolButton(toolbar, ENTYPO_ICON_OPEN_BOOK);
	tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Tags");
	tb->setFixedSize(Vector2i(32,32));
	tb->setCallback([&] {
		std::string tags(file_dialog(
		  { {"csv", "Clockwork TAG file"}, {"txt", "Text file"} }, false));
		gui->getObjectWindow()->loadTagFile(tags);
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_INSTALL);
	tb->setTooltip("Refresh");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([this](bool state) {
	});

	ToolButton *settings_button = new ToolButton(toolbar, ENTYPO_ICON_COG);
	settings_button->setFixedSize(Vector2i(32,32));
	settings_button->setTooltip("Theme properties");
	settings_button->setChangeCallback([this](bool state) {
		this->gui->getThemeWindow()->setVisible(state);
		if (state)
			this->gui->getThemeWindow()->getWindow()->requestFocus();
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_LAYOUT);
	tb->setTooltip("Create");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([](bool state) { });

	tb = new ToolButton(toolbar, ENTYPO_ICON_EYE);
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
