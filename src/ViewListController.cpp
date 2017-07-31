//
//  ViewListController.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "ViewListController.h"

std::ostream &ViewListController::operator<<(std::ostream &out) const  {
    out << "View List";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ViewListController &m) {
    return m.operator<<(out);
}

