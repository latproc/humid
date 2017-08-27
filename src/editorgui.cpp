/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <nanogui/common.h>
#include "editorproject.h"
#include "editorsettings.h"
#include "editorgui.h"
#include "helper.h"
#include "linkableobject.h"
#include "editorwidget.h"
#include "editorbutton.h"
#include "editortextbox.h"
#include "editorlabel.h"
#include "editorimageview.h"
#include "editorlineplot.h"
#include "editorprogressbar.h"
#include "panelscreen.h"
#include "editor.h"
#include "helper.h"
#include "structureswindow.h"
#include "propertywindow.h"
#include "patternswindow.h"
#include "screenswindow.h"
#include "objectwindow.h"

extern std::map<std::string, Structure *>structures;
extern std::list<Structure *>st_structures;

Structure *EditorGUI::system_settings = 0;


class EditorKeyboard {
public:
	static EditorKeyboard *instance() {
		if (instance_ == nullptr) instance_ = new EditorKeyboard();
		return instance_;
	}
	const std::string& keyName(int key) {
		assert(key_names.size() > 1);
		auto found = key_names.find(key);
		if (found != key_names.end()) 
			return (*found).second;
		return unknown_key;
	}
private:
	EditorKeyboard();
	static EditorKeyboard* instance_;
	std::map<int, std::string>key_names;
	std::string unknown_key;
};
EditorKeyboard *EditorKeyboard::instance_ = nullptr;


EditorGUI::EditorGUI(int width, int height, bool full_screen)
: ClockworkClient(Eigen::Vector2i(width, height), "Humid", !full_screen, full_screen), theme(0),
	state(GUIWELCOME),
	editor(0), w_toolbar(0), w_properties(0), w_theme(0), w_user(0), w_patterns(0),
	w_structures(0), w_connections(0), w_startup(0), w_screens(0), w_views(0), needs_update(false),
	sample_buffer_size(5000), project(0)
{
	old_size = mSize;
	/*
	std::vector<std::pair<int, std::string>> icons = nanogui::loadImageDirectory(mNVGContext, "images");
	// Load all of the images by creating a GLTexture object and saving the pixel data.
	std::string resourcesFolderPath("./");
	for (auto& icon : icons) {
		GLTexture texture(icon.second);
		auto data = texture.load(resourcesFolderPath + icon.second + ".png");
		std::cout << "loaded image " << icon.second << " with id " << texture.texture() << "\n";
		mImagesData.emplace_back(std::move(texture), std::move(data));
	}
	assert(mImagesData.size() > 0);
	*/
}

Structure *EditorGUI::getSettings() {
    Structure *es = EditorSettings::find("EditorSettings");
    if (es) return es;
		else
        return EditorSettings::create();
}

void EditorGUI::addLinkableProperty(const std::string name, LinkableProperty*lp) {
	std::lock_guard<std::recursive_mutex>  lock(linkables_mutex);
	linkables[name] = lp;
}


nanogui::Window *EditorGUI::getNamedWindow(const std::string name) {
	if (name == "Structures") return (getStructuresWindow()) ? getStructuresWindow()->getWindow() : nullptr;
	if (name == "Properties") return (getPropertyWindow()) ? getPropertyWindow()->getWindow() : nullptr;
	if (name == "Objects") return (getObjectWindow()) ? getObjectWindow()->getWindow() : nullptr;
	if (name == "Patterns") return (getPatternsWindow()) ? getPatternsWindow()->getWindow() : nullptr;
	if (name == "Screens") return (getScreensWindow()) ? getScreensWindow()->getWindow() : nullptr;
	return nullptr;
}

bool EditorGUI::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (EDITOR->isEditMode()) {
		return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
	}
	else {
		if (key == GLFW_KEY_ESCAPE) {
			w_user->getWindow()->requestFocus();
			return true;
		}
		if (action == GLFW_PRESS ) {
			bool function_key = false;
			std::string key_name("");
			if ((key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)) {
				char buf[10];
				snprintf(buf, 10, "KEY_F%d", key - GLFW_KEY_F1+1);
				key_name = buf;
				function_key = true;
			}
			if (!function_key && nanogui::Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

			key_name = EditorKeyboard::instance()->keyName(key);
			if (key_name.empty()) {
				return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
			}


			{
				std::cout << "detected key: " << key_name << "\n";
				std::string conn;
				StructureClass *sc = w_user->structure()->getStructureDefinition();
				if (sc) {
					std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
					if (found != sc->getOptions().end())
						conn = (*found).second.asString();
				}
				if (conn.empty() && EditorGUI::systemSettings()) {
					sc = EditorGUI::systemSettings()->getStructureDefinition();
					if (!sc) {
						std::cout << "no class\n";
						return false;
					}
					std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
					if (found != sc->getOptions().end())
						conn = (*found).second.asString();
				}
				if (conn.length()) {				
					std::cout << "found key mapping: " << conn << "\n";
					size_t dpos = conn.find(':');
					if (dpos != std::string::npos) {
						std::string remote = conn.substr(dpos+1);
						conn = conn.erase(dpos);

						LinkableProperty *lp = findLinkableProperty(remote);
						if (lp) {
							std::string msgon = getIODSyncCommand(conn, 0, lp->address(), 1);
							queueMessage(conn, msgon, [](std::string s){std::cout << ": " << s << "\n"; });
							std::string msgoff = getIODSyncCommand(conn, 0, lp->address(), 0);
							queueMessage(conn, msgoff, [](std::string s){std::cout << ": " << s << "\n"; });
							return true;
						}
					}
					else {
						//search the current screen for a button with this name and click it
						for (auto w : w_user->getWindow()->children()) {
							EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
							EditorTextBox *et = dynamic_cast<EditorTextBox*>(w);
							EditorButton *eb = dynamic_cast<EditorButton*>(w);
							if (et && et->getName() == conn) {
								w->requestFocus();
								et->selectAll();
								return true;
							}
							else if (eb && eb->getName() == conn) {
								if (eb->callback()) eb->callback()();
								if (eb->changeCallback()) {
									eb->changeCallback()(eb->pushed());
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
}


EditorKeyboard::EditorKeyboard() {
	key_names.insert(std::make_pair(GLFW_KEY_SPACE, "KEY_SPACE"));
	key_names.insert(std::make_pair(GLFW_KEY_APOSTROPHE, "KEY_APOSTROPHE"));
	key_names.insert(std::make_pair(GLFW_KEY_COMMA, "KEY_COMMA"));
	key_names.insert(std::make_pair(GLFW_KEY_MINUS, "KEY_MINUS"));
	key_names.insert(std::make_pair(GLFW_KEY_PERIOD, "KEY_PERIOD"));
	key_names.insert(std::make_pair(GLFW_KEY_SLASH, "KEY_SLASH"));
	key_names.insert(std::make_pair(GLFW_KEY_0, "KEY_0"));
	key_names.insert(std::make_pair(GLFW_KEY_1, "KEY_1"));
	key_names.insert(std::make_pair(GLFW_KEY_2, "KEY_2"));
	key_names.insert(std::make_pair(GLFW_KEY_3, "KEY_3"));
	key_names.insert(std::make_pair(GLFW_KEY_4, "KEY_4"));
	key_names.insert(std::make_pair(GLFW_KEY_5, "KEY_5"));
	key_names.insert(std::make_pair(GLFW_KEY_6, "KEY_6"));
	key_names.insert(std::make_pair(GLFW_KEY_7, "KEY_7"));
	key_names.insert(std::make_pair(GLFW_KEY_8, "KEY_8"));
	key_names.insert(std::make_pair(GLFW_KEY_9, "KEY_9"));
	key_names.insert(std::make_pair(GLFW_KEY_SEMICOLON, "KEY_SEMICOLON"));
	key_names.insert(std::make_pair(GLFW_KEY_EQUAL, "KEY_EQUAL"));
	key_names.insert(std::make_pair(GLFW_KEY_A, "KEY_A"));
	key_names.insert(std::make_pair(GLFW_KEY_B, "KEY_B"));
	key_names.insert(std::make_pair(GLFW_KEY_C, "KEY_C"));
	key_names.insert(std::make_pair(GLFW_KEY_D, "KEY_D"));
	key_names.insert(std::make_pair(GLFW_KEY_E, "KEY_E"));
	key_names.insert(std::make_pair(GLFW_KEY_F, "KEY_F"));
	key_names.insert(std::make_pair(GLFW_KEY_G, "KEY_G"));
	key_names.insert(std::make_pair(GLFW_KEY_H, "KEY_H"));
	key_names.insert(std::make_pair(GLFW_KEY_I, "KEY_I"));
	key_names.insert(std::make_pair(GLFW_KEY_J, "KEY_J"));
	key_names.insert(std::make_pair(GLFW_KEY_K, "KEY_K"));
	key_names.insert(std::make_pair(GLFW_KEY_L, "KEY_L"));
	key_names.insert(std::make_pair(GLFW_KEY_M, "KEY_M"));
	key_names.insert(std::make_pair(GLFW_KEY_N, "KEY_N"));
	key_names.insert(std::make_pair(GLFW_KEY_O, "KEY_O"));
	key_names.insert(std::make_pair(GLFW_KEY_P, "KEY_P"));
	key_names.insert(std::make_pair(GLFW_KEY_Q, "KEY_Q"));
	key_names.insert(std::make_pair(GLFW_KEY_R, "KEY_R"));
	key_names.insert(std::make_pair(GLFW_KEY_S, "KEY_S"));
	key_names.insert(std::make_pair(GLFW_KEY_T, "KEY_T"));
	key_names.insert(std::make_pair(GLFW_KEY_U, "KEY_U"));
	key_names.insert(std::make_pair(GLFW_KEY_V, "KEY_V"));
	key_names.insert(std::make_pair(GLFW_KEY_W, "KEY_W"));
	key_names.insert(std::make_pair(GLFW_KEY_X, "KEY_X"));
	key_names.insert(std::make_pair(GLFW_KEY_Y, "KEY_Y"));
	key_names.insert(std::make_pair(GLFW_KEY_Z, "KEY_Z"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT_BRACKET, "KEY_LEFT_BRACKET"));
	key_names.insert(std::make_pair(GLFW_KEY_BACKSLASH, "KEY_BACKSLASH"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT_BRACKET, "KEY_RIGHT_BRACKET"));
	key_names.insert(std::make_pair(GLFW_KEY_GRAVE_ACCENT, "KEY_GRAVE_ACCENT"));
	key_names.insert(std::make_pair(GLFW_KEY_WORLD_1, "KEY_WORLD_1"));
	key_names.insert(std::make_pair(GLFW_KEY_WORLD_2, "KEY_WORLD_2"));
	key_names.insert(std::make_pair(GLFW_KEY_ESCAPE, "KEY_ESCAPE"));
	key_names.insert(std::make_pair(GLFW_KEY_ENTER, "KEY_ENTER"));
	key_names.insert(std::make_pair(GLFW_KEY_TAB, "KEY_TAB"));
	key_names.insert(std::make_pair(GLFW_KEY_BACKSPACE, "KEY_BACKSPACE"));
	key_names.insert(std::make_pair(GLFW_KEY_INSERT, "KEY_INSERT"));
	key_names.insert(std::make_pair(GLFW_KEY_DELETE, "KEY_DELETE"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT, "KEY_RIGHT"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT, "KEY_LEFT"));
	key_names.insert(std::make_pair(GLFW_KEY_DOWN, "KEY_DOWN"));
	key_names.insert(std::make_pair(GLFW_KEY_UP, "KEY_UP"));
	key_names.insert(std::make_pair(GLFW_KEY_PAGE_UP, "KEY_PAGE_UP"));
	key_names.insert(std::make_pair(GLFW_KEY_PAGE_DOWN, "KEY_PAGE_DOWN"));
	key_names.insert(std::make_pair(GLFW_KEY_HOME, "KEY_HOME"));
	key_names.insert(std::make_pair(GLFW_KEY_END, "KEY_END"));
	key_names.insert(std::make_pair(GLFW_KEY_CAPS_LOCK, "KEY_CAPS_LOCK"));
	key_names.insert(std::make_pair(GLFW_KEY_SCROLL_LOCK, "KEY_SCROLL_LOCK"));
	key_names.insert(std::make_pair(GLFW_KEY_NUM_LOCK, "KEY_NUM_LOCK"));
	key_names.insert(std::make_pair(GLFW_KEY_PRINT_SCREEN, "KEY_PRINT_SCREEN"));
	key_names.insert(std::make_pair(GLFW_KEY_PAUSE, "KEY_PAUSE"));
	key_names.insert(std::make_pair(GLFW_KEY_F1, "KEY_F1"));
	key_names.insert(std::make_pair(GLFW_KEY_F2, "KEY_F2"));
	key_names.insert(std::make_pair(GLFW_KEY_F3, "KEY_F3"));
	key_names.insert(std::make_pair(GLFW_KEY_F4, "KEY_F4"));
	key_names.insert(std::make_pair(GLFW_KEY_F5, "KEY_F5"));
	key_names.insert(std::make_pair(GLFW_KEY_F6, "KEY_F6"));
	key_names.insert(std::make_pair(GLFW_KEY_F7, "KEY_F7"));
	key_names.insert(std::make_pair(GLFW_KEY_F8, "KEY_F8"));
	key_names.insert(std::make_pair(GLFW_KEY_F9, "KEY_F9"));
	key_names.insert(std::make_pair(GLFW_KEY_F10, "KEY_F10"));
	key_names.insert(std::make_pair(GLFW_KEY_F11, "KEY_F11"));
	key_names.insert(std::make_pair(GLFW_KEY_F12, "KEY_F12"));
	key_names.insert(std::make_pair(GLFW_KEY_F13, "KEY_F13"));
	key_names.insert(std::make_pair(GLFW_KEY_F14, "KEY_F14"));
	key_names.insert(std::make_pair(GLFW_KEY_F15, "KEY_F15"));
	key_names.insert(std::make_pair(GLFW_KEY_F16, "KEY_F16"));
	key_names.insert(std::make_pair(GLFW_KEY_F17, "KEY_F17"));
	key_names.insert(std::make_pair(GLFW_KEY_F18, "KEY_F18"));
	key_names.insert(std::make_pair(GLFW_KEY_F19, "KEY_F19"));
	key_names.insert(std::make_pair(GLFW_KEY_F20, "KEY_F20"));
	key_names.insert(std::make_pair(GLFW_KEY_F21, "KEY_F21"));
	key_names.insert(std::make_pair(GLFW_KEY_F22, "KEY_F22"));
	key_names.insert(std::make_pair(GLFW_KEY_F23, "KEY_F23"));
	key_names.insert(std::make_pair(GLFW_KEY_F24, "KEY_F24"));
	key_names.insert(std::make_pair(GLFW_KEY_F25, "KEY_F25"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_0, "KEYPAD_0"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_1, "KEYPAD_1"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_2, "KEYPAD_2"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_3, "KEYPAD_3"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_4, "KEYPAD_4"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_5, "KEYPAD_5"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_6, "KEYPAD_6"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_7, "KEYPAD_7"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_8, "KEYPAD_8"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_9, "KEYPAD_9"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_DECIMAL, "KEYPAD_DECIMAL"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_DIVIDE, "KEYPAD_DIVIDE"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_MULTIPLY, "KEYPAD_MULTIPLY"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_SUBTRACT, "KEYPAD_SUBTRACT"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_ADD, "KEYPAD_ADD"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_ENTER, "KEYPAD_ENTER"));
	key_names.insert(std::make_pair(GLFW_KEY_KP_EQUAL, "KEYPAD_EQUAL"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT_SHIFT, "KEY_LEFT_SHIFT"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT_CONTROL, "KEY_LEFT_CONTROL"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT_ALT, "KEY_LEFT_ALT"));
	key_names.insert(std::make_pair(GLFW_KEY_LEFT_SUPER, "KEY_LEFT_SUPER"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT_SHIFT, "KEY_RIGHT_SHIFT"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT_CONTROL, "KEY_RIGHT_CONTROL"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT_ALT, "KEY_RIGHT_ALT"));
	key_names.insert(std::make_pair(GLFW_KEY_RIGHT_SUPER, "KEY_RIGHT_SUPER"));
	key_names.insert(std::make_pair(GLFW_KEY_MENU, "KEY_MENU"));
}

