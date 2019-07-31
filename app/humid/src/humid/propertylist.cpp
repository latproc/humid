/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include "propertylist.h"

#if 0
PropertyList::PropertyList(const PropertyList &orig){
    text = orig.text;
}

PropertyList &PropertyList::operator=(const PropertyList &other) {
    text = other.text;
    return *this;
}

std::ostream &PropertyList::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const PropertyList &m) {
    return m.operator<<(out);
}

bool PropertyList::operator==(const PropertyList &other) {
    return text == other.text;
}
#endif

