/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 * 
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
  c_alike scripting written by Alexander Rawass <alexannika@users.sourceforge.net>
*/


#include <stdio.h>
#include <malloc.h>

#include <vector>
#include <string>

#define YYERROR_VERBOSE

#define q(x)	("\""+x+"\"")

extern string parseCalike(char const *filename);
extern int yyerror(char *);
extern int yywrap();
extern int yylex();

#define YYDEBUG 0

#define YYSTYPE string
#define YY_SKIP_YYWRAP

extern string module_string;

extern	int yylineno;
extern char *yytext;

#include "c_alike.tab.cpp.h"

