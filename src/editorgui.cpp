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


EditorGUI::EditorGUI(int width, int height, bool full_screen)
: ClockworkClient(Eigen::Vector2i(width, height), "Humid", !full_screen, full_screen), theme(0),
	startup(sINIT), state(GUIWELCOME),
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
