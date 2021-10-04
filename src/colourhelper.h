//
//  helper.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#pragma once

#include <string>
#include <nanogui/common.h>
#include "structure.h"

nanogui::Color colourFromString(const std::string &colour);
nanogui::Color colourFromProperty(Structure *s, const std::string &prop);
nanogui::Color colourFromProperty(Structure *element, const char *prop);
