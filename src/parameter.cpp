/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <value.h>
#include "parameter.h"

HmiParameter::HmiParameter(Value v) : val(v), machine(0) {
	;
}
HmiParameter::HmiParameter(const char *name, const SymbolTable &st) : val(name), properties(st), machine(0) { }
std::ostream &HmiParameter::operator<< (std::ostream &out)const {
	return out << val << "(" << properties << ")";
}
HmiParameter::HmiParameter(const HmiParameter &orig) {
	val = orig.val; machine = orig.machine; properties = orig.properties;
	real_name = orig.real_name;

}

