//
//  EditorProject.cpp
//  Project: Humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "editorproject.h"
#include <iostream>

EditorProject::EditorProject(const char *basedir) : base_dir(basedir) {}

#if 0
EditorProject::EditorProject(const EditorProject &orig){
    text = orig.text;
}

EditorProject &EditorProject::operator=(const EditorProject &other) {
    text = other.text;
    return *this;
}

std::ostream &EditorProject::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const EditorProject &m) {
    return m.operator<<(out);
}

bool EditorProject::operator==(const EditorProject &other) {
    return text == other.text;
}
#endif
