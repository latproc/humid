/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __hmi_parameter_h__
#define __hmi_parameter_h__

#include <lib_clockwork_client.hpp>

class Structure;
class Parameter {
public:
	Value val;
	SymbolTable properties;
	Structure *machine;
	std::string real_name;
	Parameter(Value v);
	Parameter(const char *name, const SymbolTable &st);
	std::ostream &operator<< (std::ostream &out)const;
	Parameter(const Parameter &orig);
};
std::ostream &operator<<(std::ostream &out, const Parameter &p);

#endif
