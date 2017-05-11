/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef hmilang_h
#define hmilang_h

#include <list>
#include <symboltable.h>
#include "parameter.h"
#include "structure.h"

void yyerror(const char *str);

#ifndef __MAIN__
extern int line_num;
extern SymbolTable globals;
extern const char *yyfilename;
#endif

#endif
