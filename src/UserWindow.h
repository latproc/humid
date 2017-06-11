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
#include "skeleton.h"
#include "Palette.h"
#include "circularbuffer.h"

class EditorGUI;
class UserWindowWin;
class PanelScreen;
class Structure;

class UserWindow : public Skeleton, public Palette {
public:
	//UserWindow(EditorGUI *screen, nanogui::Theme *theme);
	UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww);
	void setVisible(bool which) { window->setVisible(which); }
	void save(const std::string &path);
	void load(const std::string &path);

	CircularBuffer *getValues(const std::string name);
	CircularBuffer * addDataBuffer(const std::string name, CircularBuffer::DataType dt, size_t len);
	std::map<std::string, CircularBuffer *> &getData() { return data; }
	void update();
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
	void setStructure( Structure *s);
	void loadStructure( Structure *s);
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
