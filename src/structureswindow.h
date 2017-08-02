//
//  StructuresWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __StructuresWindow_h__
#define __StructuresWindow_h__

#include <ostream>
#include <string>
#include <map>

#include "palette.h"
#include "skeleton.h"
#include "editorgui.h"

class StructuresWindow : public Skeleton, public Palette {
public:
    static StructuresWindow *instance();
	void setVisible(bool which) { window->setVisible(which); }
	Structure *createStructure(const std::string kind);
protected:
    static StructuresWindow *create(EditorGUI *screen, nanogui::Theme *theme);
private:
	StructuresWindow(EditorGUI *screen, nanogui::Theme *theme);
    static StructuresWindow *instance_;
	EditorGUI *gui;
	std::map<std::string, Structure *>starters;

    friend class EditorGUI;
};

#endif
