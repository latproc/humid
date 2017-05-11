/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __hmi_structure_h__
#define __hmi_structure_h__

#include "Parameter.h"
#include <list>
#include <ostream>

class Structure {
public:
	Structure(const std::string sname, const std::string skind) : name(sname), kind(skind) {}
	std::list<Parameter> parameters;
	void setDefinitionLocation(const std::string fnam, int lineno) { }
	void setProperties(const SymbolTable &props) { properties.add(props); }
	SymbolTable &getProperties() { return properties; }

	const std::string &getName() { return name; }
	const std::string &getKind() { return kind; }

	std::ostream &operator<<(std::ostream &out) const;
private:
	SymbolTable properties;
	std::string name;
	std::string kind;
};
std::ostream &operator<<(std::ostream &out, const Structure &s);

#endif
