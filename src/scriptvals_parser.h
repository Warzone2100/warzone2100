/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     BOOLEAN_T = 258,
     INTEGER = 259,
     IDENT = 260,
     QTEXT = 261,
     TYPE = 262,
     VAR = 263,
     ARRAY = 264,
     SCRIPT = 265,
     STORE = 266,
     RUN = 267
   };
#endif
/* Tokens.  */
#define BOOLEAN_T 258
#define INTEGER 259
#define IDENT 260
#define QTEXT 261
#define TYPE 262
#define VAR 263
#define ARRAY 264
#define SCRIPT 265
#define STORE 266
#define RUN 267




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 120 "scriptvals_parser.ypp"

	bool			bval;
	INTERP_TYPE		tval;
	char			*sval;
	UDWORD			vindex;
	SDWORD			ival;
	VAR_INIT		sInit;
	ARRAY_INDEXES	*arrayIndex;



/* Line 2068 of yacc.c  */
#line 86 "scriptvals_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE scrv_lval;


