//
//  helper.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#pragma once

#include <string>
#include <nanogui/common.h>
#include <valuehelper.h>
#include <symboltable.h>

nanogui::Color colourFromString(const std::string &colour);
std::string stringFromColour(const nanogui::Color &colour);

template<typename T>
nanogui::Color colourFromProperty(T *element, const char *prop) {
    Value colour(element->getValue(prop));
    if (colour == SymbolTable::Null) {
        colour = defaultForProperty(prop);
    }
    if (colour != SymbolTable::Null) {
        return colourFromString(colour.asString());
    }
    return nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f);
}

template<typename T>
nanogui::Color colourFromProperty(T *element, const std::string &prop) {
    return colourFromProperty(element, prop.c_str());
}

