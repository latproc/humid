/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include "structure.h"


std::ostream &Structure::operator<<(std::ostream &out) const {
	return out << name << " " << kind;
}

std::ostream &operator<<(std::ostream &out, const Structure &s) {
	return s.operator<<(out);
}


void StructureClass::addProperty(const char *p) {
	property_names.insert(p);
}

void StructureClass::addProperty(const std::string &p) {
	property_names.insert(p.c_str());
}

void StructureClass::addPrivateProperty(const char *p) {
	property_names.insert(p);
	local_properties.insert(p); //
}

void StructureClass::addPrivateProperty(const std::string &p) {
	property_names.insert(p.c_str());
	local_properties.insert(p); //
}
