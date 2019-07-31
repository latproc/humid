/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef hmilang_h
#define hmilang_h

#include <list>
#include <lib_clockwork_client.hpp>
#include "parameter.h"
#include "structure.h"
#include "anchor.h"
#include "linkableproperty.h"

void yyerror(const char *str);

#ifndef __MAIN__
class LinkedProperty;

extern int line_num;
extern SymbolTable globals;
extern const char *yyfilename;
extern std::list<Structure *>hm_structures;
extern std::list<StructureClass *> hm_classes;
extern std::map<std::string, LinkedProperty *> remotes;
#endif

#endif
