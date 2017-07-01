/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef hmilang_h
#define hmilang_h

#include <list>
#include <symboltable.h>
#include "Parameter.h"
#include "structure.h"

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
