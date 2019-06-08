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
extern std::map<std::string, Structure *>structures;

std::ostream &EditorSettings::operator<<(std::ostream &out) const  {
    return out;
}

std::ostream &operator<<(std::ostream &out, const EditorSettings &m) {
    return m.operator<<(out);
}

EditorSettings::EditorSettings(const std::string sname, const std::string skind) : Structure(nullptr, sname, skind) {
  getProperties().add("full_screen", false);
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
  auto found = structures.find(object_name);
  if (found != structures.end()) return (*found).second;
  for (auto item : st_structures) {
      if (item->getName() == object_name) {
          return item;
      }
  }
  return nullptr;
}

Structure *EditorSettings::create() {
  Structure *s = find("EditorSettings");
  if (!s) s = new EditorSettings("EditorSettings", "EDITORSETTINGS");
  st_structures.push_back(s);
  structures["EditorSettings"] = s;
  flush();
  return s;
}

void EditorSettings::flush() {
    if (!dirty) return;
    dirty = false;
    if (settings_files.size() == 0) return;
    std::string fname(settings_files.front());
    std::ofstream settings_file(fname);
    Structure *s = EditorSettings::find("EditorSettings");
    assert(s);
    s->save(settings_file);
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
