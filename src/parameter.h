/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __hmi_parameter_h__
#define __hmi_parameter_h__

#include <value.h>
#include <symboltable.h>

class Structure;
class HmiParameter {
public:
	Value val;
	SymbolTable properties;
	Structure *machine;
	std::string real_name;
	HmiParameter(Value v);
	HmiParameter(const char *name, const SymbolTable &st);
	std::ostream &operator<< (std::ostream &out)const;
	HmiParameter(const HmiParameter &orig);
};
std::ostream &operator<<(std::ostream &out, const HmiParameter &p);

#endif
