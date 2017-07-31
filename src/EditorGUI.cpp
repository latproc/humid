/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <nanogui/common.h>
#include "EditorProject.h"
#include "EditorSettings.h"
#include "EditorGUI.h"
#include "helper.h"
#include "LinkableObject.h"
#include "EditorWidget.h"
#include "EditorButton.h"
#include "EditorTextBox.h"
#include "EditorLabel.h"
#include "EditorImageView.h"
#include "EditorLinePlot.h"
#include "EditorProgressBar.h"
#include "PanelScreen.h"
#include "Editor.h"
#include "helper.h"

extern std::map<std::string, Structure *>structures;
extern std::list<Structure *>st_structures;


EditorGUI::EditorGUI() : ClockworkClient(Eigen::Vector2i(1024, 768), "Humid"), theme(0),
	startup(sINIT), state(GUIWELCOME), 
	editor(0), w_toolbar(0), w_properties(0), w_theme(0), w_user(0), w_patterns(0),
	w_structures(0), w_connections(0), w_startup(0), w_screens(0), w_views(0), needs_update(false),
	sample_buffer_size(5000), project(0)
{
	old_size = mSize;
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

}

Structure &EditorGUI::getSettings() {
    auto found = structures.find("EditorSettings");
    if (found == structures.end()) { 
        EditorSettings *es = new EditorSettings("EditorSettings", "EDITORSETTINGS");
        st_structures.push_back(es);
        structures["EditorSettings"] = es;
        return *es;
    }
    Structure *s = (*found).second;
    return *s;
}


bool buttonToScript(std::ostream &out, EditorButton *b) {
	if (!b) return false;
	out << b->getName() << " " << b->baseName() << " ("
		<< "pos_x: " << b->position().x() << ", pos_y: " << b->position().y()
		<< ", width: " << b->width() << ", height: " << b->height()
		<< ", caption: \"" << b->caption() << '"'
		<< ", font_size: " << b->fontSize()
		<< ", tab_pos: " << b->tabPosition();
	const nanogui::Color color = b->backgroundColor();
	out << ", bg_color: \"" << color.r() <<"," << color.g()<<","<<color.b()<<","<<color.w()<<"\"";
	if (b->command().length()) out << ", command: " << escapeQuotes(b->command());
	if (b->getRemote()) {
		out << ", remote: \"" << b->getRemote()->tagName() << "\"";
	}
	out << ");\n";
	return true;
}

bool labelToScript(std::ostream &out, EditorLabel *el) {
	if (!el) return false;
	out << el->getName() << " " << el->baseName() << " ("
		<< "pos_x: " << el->position().x() 
		<< ", pos_y: " << el->position().y()
		<< ", width: " << el->width() << ", height: " << el->height()
		<< ", caption: \"" << el->caption() << '"'
		<< ", font_size: " << el->fontSize()
		<< ", tab_pos: " << el->tabPosition();
	if (el->getRemote()) {
		out << ", remote: \"" << el->getRemote()->tagName() << "\"";
	}
	out << ");\n";
	return true;
}

bool progressbarToScript(std::ostream &out, EditorProgressBar *eb) {
	if (!eb) return false;
	out << eb->getName() << " " << eb->baseName() << " ("
		<< "pos_x: " << eb->position().x() 
		<< ", pos_y: " << eb->position().y()
		<< ", width: " << eb->width() << ", height: " << eb->height()
		<< ", tab_pos: " << eb->tabPosition();
	if (eb->getRemote()) {
		out << ", remote:\"" << eb->getRemote()->tagName() << "\"";
	}
	out << ");\n";
	return true;
}

bool textboxToScript(std::ostream &out, EditorTextBox *t) {
	if (!t) return false;
	out << t->getName() << " " << t->baseName() << " ("
	<< "pos_x: " << t->position().x() << ", pos_y: " << t->position().y()
	<< ", width: " << t->width() << ", height: " << t->height()
	<< ", font_size: " << t->fontSize()
	<< ", tab_pos: " << t->tabPosition();
	if (!t->getRemote())
		out << ", value: " << escapeQuotes(t->value());
	/*else if (t->getRemote() && t->getRemote()->dataType() == CircularBuffer::STR)
		out << ", value: " << escapeQuotes(t->value());
	else
		out << ", value: " << t->value();*/
	if (t->getRemote()) {
		out << ", remote: \"" << t->getRemote()->tagName() << "\"";
	}
	out << ");\n";
	return true;
}

bool imageviewToScript(std::ostream &out, EditorImageView *ip) {
	if (!ip) return false;
	out << ip->getName() << " " << ip->baseName() << " ("
	<< "pos_x: " << ip->position().x() << ", pos_y: " << ip->position().y()
	<< ", width: " << ip->width() << ", height: " << ip->height()
	<< ", location: \"" << ip->imageName() << '"'
	<< ", tab_pos: " << ip->tabPosition();
	if (ip->getRemote()) {
		out << ", remote: \"" << ip->getRemote()->tagName() << "\"";
	}
	out <<  ");\n";
	return true;
}

bool linePlotToScript(std::ostream &out, EditorLinePlot *lp) {
	if (!lp) return false;
	out << lp->getName() << " " << lp->baseName()<< "_" << lp->getName() 
		<< " ("
		<< "pos_x: " << lp->position().x() << ", pos_y: " << lp->position().y()
		<< ", width: " << lp->width() << ", height: " << lp->height()
		<< ", tab_pos: " << lp->tabPosition()
		<< ", x_scale: " << lp->xScale() 
		<< ", grid_intensity: " << lp->gridIntensity()
		<< ", display_grid: " << lp->displayGrid()
		<< ", monitors: \"" << lp->monitors() << '"'
		<< ", overlay: " << lp->overlaid()
		<< ");\n";
	return true;
}

bool linePlotDefinitionToScript(std::ostream &out, EditorLinePlot *lp, std::set<LinkableProperty*> &used_properties) {
	if (!lp) return false;
	out <<  lp->baseName() << "_" << lp->getName() << " STRUCTURE EXTENDS " << lp->baseName() << " {\n"
		<< "\tOPTION pos_x 50;\n"
		<< "\tOPTION pos_y 100;\n"
		<< "\tOPTION width 200;\n"
		<< "\tOPTION height 100;\n"
		<< "\tOPTION x_scale 1.0;\n"
		<< "\tOPTION grid_intensity 0.05;\n"
		<< "\tOPTION display_grid TRUE;\n\n";
	for (auto series : lp->getSeries()) {
		LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(series->getName());
		if (lp) used_properties.insert(lp);
		out << "\t" << series->getName() << " SERIES (remote:" <<  series->getName() << ");\n";
	}
	out << "}\n";
	return true;
}

void windowToScript(std::ostream &out, std::stringstream &pending_definitions, PanelScreen *screen) {
	using namespace nanogui;
	std::set<LinkableProperty*> used_properties;
	nanogui::Window *window = 0;
	if (!window) return;	
	std::string screen_type(screen->getName());
	boost::to_upper(screen_type);
	if (screen_type == screen->getName())
		screen_type += '_';
	out << screen_type << " STRUCTURE EXTENDS SCREEN {\n";
	for (auto it = window->children().rbegin(); it != window->children().rend(); ++it) {
		Widget *child = *it;
		EditorWidget *ew = dynamic_cast<EditorWidget*>(child);
		if (ew && ew->getRemote()) {
			EditorLinePlot *lp = dynamic_cast<EditorLinePlot*>(ew);
			if (lp) for (auto series : lp->getSeries()) {
				LinkableProperty *prop = EDITOR->gui()->findLinkableProperty(series->getName());
				if (prop) used_properties.insert(prop);
			}
		}
		if (buttonToScript(out, dynamic_cast<EditorButton*>(child))) continue;
		if (labelToScript(out, dynamic_cast<EditorLabel*>(child))) continue;
		if (progressbarToScript(out, dynamic_cast<EditorProgressBar*>(child))) continue;
		if (textboxToScript(out, dynamic_cast<EditorTextBox*>(child))) continue;
		if (imageviewToScript(out, dynamic_cast<EditorImageView*>(child))) continue;
		{
			EditorLinePlot *lp = dynamic_cast<EditorLinePlot*>(child);
			if (linePlotToScript(out, lp)) { 
				linePlotDefinitionToScript(pending_definitions, lp, used_properties); continue;
			}
		}
	}
	out << "}\n" << screen->getName() << " " << screen_type 
		<< " (caption: \"" << escapeQuotes(screen->getName()) << "\");\n\n";
	out << pending_definitions.str() << "\n";

	for (auto link : used_properties) {
		out << link->tagName() << " REMOTE (";
		link->save(out);
		out << ");\n";	
	}
}

