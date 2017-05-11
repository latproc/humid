/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include "EditorGUI.h"

EditorGUI::EditorGUI(const char *msg) :text(msg) {
    
}

#if 0
EditorGUI::EditorGUI(const EditorGUI &orig){
    text = orig.text;
}

EditorGUI &EditorGUI::operator=(const EditorGUI &other) {
    text = other.text;
    return *this;
}

std::ostream &EditorGUI::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const EditorGUI &m) {
    return m.operator<<(out);
}

bool EditorGUI::operator==(const EditorGUI &other) {
    return text == other.text;
}
#endif

