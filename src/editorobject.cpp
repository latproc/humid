//
//  EditorObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "editorobject.h"
#include "linkableobject.h"
#include <iostream>

std::ostream &EditorObject::operator<<(std::ostream &out) const { return out; }

std::ostream &operator<<(std::ostream &out, const EditorObject &m) { return m.operator<<(out); }
