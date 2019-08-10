/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __UserWindow_h__
#define __UserWindow_h__

#include <ostream>
#include <string>
#include <map>
#include <list>
#include <nanogui/theme.h>
#include <humid/common/includes.hpp>
#include "skeleton.h"
#include "palette.h"
#include "linkableobject.h"

class EditorGUI;
class UserWindowWin;
class PanelScreen;
class Structure;
class PropertyFormHelper;

// the UserWindow displays user interface widgets for a Screen structure
// the widgets are linked to remote objects and update automatically from remote changes
class UserWindow : public Skeleton, public Palette, public LinkableObject {
public:
	//UserWindow(EditorGUI *screen, nanogui::Theme *theme);
	UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww);
	void setVisible(bool which) { window->setVisible(which); }
	void save(const std::string &path);
	void load(const std::string &path);

	virtual void update(const Value &value) override;

	CircularBuffer *getValues(const std::string name);
	CircularBuffer * addDataBuffer(const std::string name, Humid::DataType dt, size_t len);
	std::map<std::string, CircularBuffer *> &getData() { return data; }
	void update();
	void refresh();
	void fixLinks(LinkableProperty *lp); // relink gui objects to remote
	nanogui::Widget *current() { return current_layer; }
	void deleteSelections();
	EditorGUI *app() { return gui; }
	CircularBuffer* getDataBuffer(const std::string name);
	NVGcontext* getNVGContext();
	PanelScreen *getActivePanel() { if (active_screens.empty()) return nullptr; else return active_screens.back(); }
	void push(PanelScreen *nxt) { active_screens.push_back(nxt); }
	PanelScreen *pop(PanelScreen *nxt) {
		PanelScreen *last = getActivePanel();
		if (!active_screens.empty()) active_screens.pop_back();
		return last;
	}
	nanogui::Vector2i defaultSize() { return mDefaultSize; }
	CircularBuffer *createBuffer(const std::string name);

	Structure *structure() { return current_structure; }

	// syncs the current widget state to their owning structure and loads the given structure 
	void setStructure( Structure *s);

	// create and initialise widgets in the user window from the given structure
	void loadStructure( Structure *s);

	// remove all widgets from the user window
	void clear();

	virtual void select(Selectable * w) override;
	virtual void deselect(Selectable *w) override;

	void startEditMode();
	void endEditMode();

	virtual void getPropertyNames(std::list<std::string> &names);
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map);
	virtual void loadProperties(PropertyFormHelper *pfh);
	virtual Value getPropertyValue(const std::string &prop);


private:
	EditorGUI *gui;
	std::list<PanelScreen*>active_screens;
	nanogui::Widget *current_layer;
	nanogui::Vector2i mDefaultSize;
	std::list<nanogui::Widget *> layers;
	std::map<std::string, CircularBuffer *>data; // tbd need more lines

	UserWindow(const UserWindow &orig);
	UserWindow &operator=(const UserWindow &other);
	std::ostream &operator<<(std::ostream &out) const;
	bool operator==(const UserWindow &other);
	Structure *current_structure;
};

std::ostream &operator<<(std::ostream &out, const UserWindow &m);

#endif
