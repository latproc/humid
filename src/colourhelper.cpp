//
//  helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <sstream>
#include "colourhelper.h"
#include "valuehelper.h"
#include "structure.h"


static int val(char hex) {
	int res = 0;
	if (hex >= 'a') hex -= 32;
	if (hex >= 'A') {
		res = 10 + hex - 'A';
		if (res >= 16) res = 0;
	}
	else if (hex >= '0') {
		res = hex - '0';
		if (res >= 10) res = 0;
	}
	return res;
}

nanogui::Color colourFromString(const std::string &colour) {
	struct Colour { int r, g, b, a; };
	Colour c{0, 0, 0, 0};
	auto len = colour.length();
	auto dbl = [](int val) -> int { return val * 16 + val; };
	auto parse = [](const char * &p) -> int { int res = val(*p++); return res * 16 + val(*p++); };
	const char *p = colour.c_str() + 1;
	if (colour[0] == '#') {
		if (len == 4 || len ==5) {
			// #rgb format, with or without alpha
			auto dbl = [](int val) -> int { return val * 16 + val; };
			c = {
				.r = dbl(val(*p++)),
				.g = dbl(val(*p++)),
				.b = dbl(val(*p++)),
				.a = (len == 5) ? dbl(val(*p)) : 255
			};
		}
		else if (len == 7 || len == 9) {
			// #rrggbb format with or without alpha
			c = {
				.r = parse(p),
				.g = parse(p),
				.b = parse(p),
				.a = len == 9 ? parse(p) : 255
			};
		}
	}
	else if (colour[0] == '&') {
		auto len = colour.length();
		if (len == 3) {
			// #ga where c is a grey intensity level and a is an alpha level
			auto grey = dbl(val(*p++));
			c = {
				.r = grey,
				.g = grey,
				.b = grey,
				.a = dbl(val(*p))
			};
		}
		else if (len == 5) {
			auto grey = parse(p);
			c = {
				.r = grey,
				.g = grey,
				.b = grey,
				.a = parse(p)
			};
		}
	}
	return nanogui::Color(nanogui::Vector4i{c.r, c.g, c.b, c.a});
}

std::string stringFromColour(const nanogui::Color &colour) {

	std::stringstream ss;
	ss << '#' << std::hex << std::setfill('0') 
		<< std::setw(2) << static_cast<int>(colour.r()*255) 
		<< std::setw(2) << static_cast<int>(colour.g()*255)  
		<< std::setw(2) << static_cast<int>(colour.b()*255);
	if (colour.w() != 1.0)
		ss << std::setw(2) <<static_cast<int>(colour.w()*255);
	return ss.str();
}

nanogui::Color colourFromProperty(Structure *element, const std::string &prop) {
	return colourFromProperty(element, prop.c_str());
}

nanogui::Color colourFromProperty(Structure *element, const char *prop) {
	Value colour(element->getProperties().find(prop));
	if (colour == SymbolTable::Null) {
		std::cerr << "---------- unable to find colour " << prop << " on structure " << element->getName() << "\n";
		colour = defaultForProperty(prop);
	}
	if (colour != SymbolTable::Null) {
		std::string colour_str = colour.asString();
		if (colour_str[0] == '#' || colour_str[0] == '&') {
			return colourFromString(colour_str);
		}
		std::vector<std::string> tokens;
		boost::algorithm::split(tokens, colour_str, boost::is_any_of(","));
		if (tokens.size() == 4) {
			std::vector<float>fields(4);
			for (int i=0; i<4; ++i) fields[i] = std::atof(tokens[i].c_str());
			return nanogui::Color(fields[0], fields[1], fields[2], fields[3]);
		}
		else if (tokens.size() == 3) {
			std::vector<float>fields(3);
			for (int i=0; i<4; ++i) fields[i] = std::atof(tokens[i].c_str());
			return nanogui::Color(fields[0], fields[1], fields[2], 1.0f);
		}
		else {
			std::cerr << "unrecognised colour: " << colour << "\n";
		}
	}
	else {
		std::cerr << "No property " << prop << " on element " << element->getName() << "\n";
	}
	return nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f);
}

