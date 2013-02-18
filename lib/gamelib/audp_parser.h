/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2013  Warzone 2100 Project

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
     FLOAT_T = 258,
     INTEGER = 259,
     QTEXT = 260,
     ONESHOT = 261,
     LOOP = 262,
     AUDIO = 263,
     ANIM3DFRAMES = 264,
     ANIM3DTRANS = 265,
     ANIM3DFILE = 266,
     AUDIO_MODULE = 267,
     ANIM_MODULE = 268,
     ANIMOBJECT = 269
   };
#endif
/* Tokens.  */
#define FLOAT_T 258
#define INTEGER 259
#define QTEXT 260
#define ONESHOT 261
#define LOOP 262
#define AUDIO 263
#define ANIM3DFRAMES 264
#define ANIM3DTRANS 265
#define ANIM3DFILE 266
#define AUDIO_MODULE 267
#define ANIM_MODULE 268
#define ANIMOBJECT 269




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 44 "audp_parser.ypp"

	float		fval;
	long		ival;
	bool            bval;
	char*		sval;



/* Line 2068 of yacc.c  */
#line 87 "audp_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE audp_lval;


