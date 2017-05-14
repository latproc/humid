//
//  PanelScreen.cpp
//  Project: Humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "PanelScreen.h"

PanelScreen::PanelScreen(const std::string sname) :name(sname) {
    
}

#if 0
PanelScreen::PanelScreen(const PanelScreen &orig){
    text = orig.text;
}

PanelScreen &PanelScreen::operator=(const PanelScreen &other) {
    text = other.text;
    return *this;
}

std::ostream &PanelScreen::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const PanelScreen &m) {
    return m.operator<<(out);
}

bool PanelScreen::operator==(const PanelScreen &other) {
    return text == other.text;
}
#endif

