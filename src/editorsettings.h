//
//  EditorSettings.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorSettings_h__
#define __EditorSettings_h__

#include "structure.h"
#include <map>
#include <nanogui/widget.h>
#include <ostream>
#include <string>

bool updateSettingsStructure(const std::string name, nanogui::Widget *widget);
bool applyWindowSettings(Structure *item, nanogui::Widget *widget);

class EditorSettings : public Structure {
  public:
    EditorSettings(const std::string sname, const std::string skind);
    static void applySettings(const std::string object_name, nanogui::Widget *widget);
    static Structure *find(const std::string object_name);
    static Structure *create();
    static void flush();

    static void add(const std::string &name, nanogui::Widget *w);
    static void setDirty() { dirty = true; }
    std::ostream &operator<<(std::ostream &out) const;

  private:
    static std::map<std::string, nanogui::Widget *> widgets;
    static bool dirty;
};

std::ostream &operator<<(std::ostream &out, const EditorSettings &m);

//
#endif
