//
//  helper.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#pragma once

#include "structure.h"
#include <nanogui/common.h>
#include <string>
#include <utility>
#include <value.h>

using ColourResult = std::pair<nanogui::Color, std::string>;

ColourResult colourFromString(const std::string &colour);
ColourResult colourFromValue(const Value &colour);
std::string stringFromColour(const nanogui::Color &colour);
nanogui::Color colourFromProperty(Structure *s, const std::string &prop);
nanogui::Color colourFromProperty(Structure *element, const char *prop);
