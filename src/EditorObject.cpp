//
//  EditorObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "LinkableObject.h"
#include "EditorObject.h"

std::ostream &EditorObject::operator<<(std::ostream &out) const  {
    return out;
}

std::ostream &operator<<(std::ostream &out, const EditorObject &m) {
    return m.operator<<(out);
}
