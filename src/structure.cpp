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
