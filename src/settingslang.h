/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef settingslang_h
#define settingslang_h

#include <list>
#include <symboltable.h>
#include "parameter.h"
#include "structure.h"

void yyerror(const char *str);

#ifndef __MAIN__
extern int line_num;
extern SymbolTable st_globals;
extern const char *st_yyfilename; 
extern std::list<Structure *>st_structures;
#endif

#endif
