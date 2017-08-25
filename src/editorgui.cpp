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
			std::string key_name = "";
			if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9)) {
				char buf[10];
				snprintf(buf, 10, "KEY_%d", key - GLFW_KEY_0);
				key_name = buf;
			}
			else if ((key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)) {
				char buf[10];
				snprintf(buf, 10, "KEY_F%d", key - GLFW_KEY_F1+1);
				key_name = buf;
				function_key = true;
			}
			else if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z)) {
				char buf[10];
				snprintf(buf, 10, "KEY_%c", key - GLFW_KEY_A + 'A');
				key_name = buf;
			}
			if (!function_key && nanogui::Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

			if (key_name.length()) {
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
								else if (eb->changeCallback()) {
									eb->changeCallback()(!eb->pushed());
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
