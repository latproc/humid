/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <lib_clockwork_client.hpp>
#include "parameter.h"

Parameter::Parameter(Value v) : val(v), machine(0) {
	;
}
Parameter::Parameter(const char *name, const SymbolTable &st) : val(name), properties(st), machine(0) { }
std::ostream &Parameter::operator<< (std::ostream &out)const {
	return out << val << "(" << properties << ")";
}
Parameter::Parameter(const Parameter &orig) {
	val = orig.val; machine = orig.machine; properties = orig.properties;
	real_name = orig.real_name;

}
