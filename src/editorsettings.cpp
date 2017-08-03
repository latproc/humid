//
//  EditorSettings.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "settingslang.h"
#include "editorsettings.h"
#include <list>
#include <fstream>

std::map<std::string, nanogui::Widget *> EditorSettings::widgets;
extern std::list<std::string>settings_files;
bool EditorSettings::dirty = false;

std::ostream &EditorSettings::operator<<(std::ostream &out) const  {
    return out;
}

std::ostream &operator<<(std::ostream &out, const EditorSettings &m) {
    return m.operator<<(out);
}

void EditorSettings::applySettings(const std::string object_name, nanogui::Widget *widget) {
    std::list<std::string> errors;
    {
        Structure *s = find(object_name);
        if (s && s->getKind() == "WINDOW") {
            if (!applyWindowSettings(s, widget)) {
                char buf[100];
                snprintf(buf, 100, "Error loading window %s", s->getName().c_str());
                errors.push_back(buf);
            }
        }
    }
    if (!errors.empty()) {
        for (auto err : errors) { std::cout << err << "\n";}
    }
}

Structure *EditorSettings::find(const std::string object_name) {
    for (auto item : st_structures) {
        if (item->getName() == object_name) {
            return item;
        }
    }
    return nullptr;
}

void EditorSettings::flush() {
    if (!dirty) return;
    dirty = false;
    if (settings_files.size() == 0) return;
    std::string fname(settings_files.front());
    std::ofstream settings_file(fname);
    Structure *s = find("EditorSettings");
	  if (!s) s = new EditorSettings("EditorSettings", "EDITORSETTINGS");
    if (s) s->save(settings_file);
    s = find("ProjectSettings");
    if (s) {
        s->save(settings_file);
    }
    for (auto w : widgets) {
        s = find(w.first);
        if (s) s->save(settings_file);
    }
    settings_file.close();
}

void EditorSettings::add(const std::string &name, nanogui::Widget *w) {
    dirty = true;
    updateSettingsStructure(name, w);
    widgets[name] = w;
}
