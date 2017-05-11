/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include "UIItem.h"

#if 0
UIItem::UIItem(const UIItem &orig){
    text = orig.text;
}

UIItem &UIItem::operator=(const UIItem &other) {
    text = other.text;
    return *this;
}

std::ostream &UIItem::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const UIItem &m) {
    return m.operator<<(out);
}

bool UIItem::operator==(const UIItem &other) {
    return text == other.text;
}
#endif

