/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2017  Warzone 2100 Project

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
     lexFUNCTION = 258,
     TRIGGER = 259,
     EVENT = 260,
     WAIT = 261,
     EVERY = 262,
     INACTIVE = 263,
     INITIALISE = 264,
     LINK = 265,
     REF = 266,
     RET = 267,
     _VOID = 268,
     WHILE = 269,
     IF = 270,
     ELSE = 271,
     EXIT = 272,
     PAUSE = 273,
     BOOLEQUAL = 274,
     NOTEQUAL = 275,
     GREATEQUAL = 276,
     LESSEQUAL = 277,
     GREATER = 278,
     LESS = 279,
     _AND = 280,
     _OR = 281,
     _NOT = 282,
     _INC = 283,
     _DEC = 284,
     TO_INT_CAST = 285,
     TO_FLOAT_CAST = 286,
     UMINUS = 287,
     BOOLEAN_T = 288,
     FLOAT_T = 289,
     INTEGER = 290,
     QTEXT = 291,
     TYPE = 292,
     STORAGE = 293,
     IDENT = 294,
     VAR = 295,
     BOOL_VAR = 296,
     NUM_VAR = 297,
     FLOAT_VAR = 298,
     OBJ_VAR = 299,
     STRING_VAR = 300,
     VAR_ARRAY = 301,
     BOOL_ARRAY = 302,
     NUM_ARRAY = 303,
     FLOAT_ARRAY = 304,
     OBJ_ARRAY = 305,
     BOOL_OBJVAR = 306,
     NUM_OBJVAR = 307,
     USER_OBJVAR = 308,
     OBJ_OBJVAR = 309,
     BOOL_CONSTANT = 310,
     NUM_CONSTANT = 311,
     USER_CONSTANT = 312,
     OBJ_CONSTANT = 313,
     STRING_CONSTANT = 314,
     FUNC = 315,
     BOOL_FUNC = 316,
     NUM_FUNC = 317,
     FLOAT_FUNC = 318,
     USER_FUNC = 319,
     OBJ_FUNC = 320,
     STRING_FUNC = 321,
     VOID_FUNC_CUST = 322,
     BOOL_FUNC_CUST = 323,
     NUM_FUNC_CUST = 324,
     FLOAT_FUNC_CUST = 325,
     USER_FUNC_CUST = 326,
     OBJ_FUNC_CUST = 327,
     STRING_FUNC_CUST = 328,
     TRIG_SYM = 329,
     EVENT_SYM = 330,
     CALLBACK_SYM = 331
   };
#endif
/* Tokens.  */
#define lexFUNCTION 258
#define TRIGGER 259
#define EVENT 260
#define WAIT 261
#define EVERY 262
#define INACTIVE 263
#define INITIALISE 264
#define LINK 265
#define REF 266
#define RET 267
#define _VOID 268
#define WHILE 269
#define IF 270
#define ELSE 271
#define EXIT 272
#define PAUSE 273
#define BOOLEQUAL 274
#define NOTEQUAL 275
#define GREATEQUAL 276
#define LESSEQUAL 277
#define GREATER 278
#define LESS 279
#define _AND 280
#define _OR 281
#define _NOT 282
#define _INC 283
#define _DEC 284
#define TO_INT_CAST 285
#define TO_FLOAT_CAST 286
#define UMINUS 287
#define BOOLEAN_T 288
#define FLOAT_T 289
#define INTEGER 290
#define QTEXT 291
#define TYPE 292
#define STORAGE 293
#define IDENT 294
#define VAR 295
#define BOOL_VAR 296
#define NUM_VAR 297
#define FLOAT_VAR 298
#define OBJ_VAR 299
#define STRING_VAR 300
#define VAR_ARRAY 301
#define BOOL_ARRAY 302
#define NUM_ARRAY 303
#define FLOAT_ARRAY 304
#define OBJ_ARRAY 305
#define BOOL_OBJVAR 306
#define NUM_OBJVAR 307
#define USER_OBJVAR 308
#define OBJ_OBJVAR 309
#define BOOL_CONSTANT 310
#define NUM_CONSTANT 311
#define USER_CONSTANT 312
#define OBJ_CONSTANT 313
#define STRING_CONSTANT 314
#define FUNC 315
#define BOOL_FUNC 316
#define NUM_FUNC 317
#define FLOAT_FUNC 318
#define USER_FUNC 319
#define OBJ_FUNC 320
#define STRING_FUNC 321
#define VOID_FUNC_CUST 322
#define BOOL_FUNC_CUST 323
#define NUM_FUNC_CUST 324
#define FLOAT_FUNC_CUST 325
#define USER_FUNC_CUST 326
#define OBJ_FUNC_CUST 327
#define STRING_FUNC_CUST 328
#define TRIG_SYM 329
#define EVENT_SYM 330
#define CALLBACK_SYM 331




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 1559 "script_parser.ypp"

	/* Types returned by the lexer */
	int32_t			bval;
	float			fval;
	SDWORD			ival;
	char			*sval;
	INTERP_TYPE		tval;
	STORAGE_TYPE	stype;
	VAR_SYMBOL		*vSymbol;
	CONST_SYMBOL	*cSymbol;
	FUNC_SYMBOL		*fSymbol;
	TRIGGER_SYMBOL	*tSymbol;
	EVENT_SYMBOL	*eSymbol;
	CALLBACK_SYMBOL	*cbSymbol;

	/* Types only returned by rules */
	CODE_BLOCK		*cblock;
	COND_BLOCK		*condBlock;
	OBJVAR_BLOCK	*objVarBlock;
	ARRAY_BLOCK		*arrayBlock;
	PARAM_BLOCK		*pblock;
	PARAM_DECL		*pdecl;
	TRIGGER_DECL	*tdecl;
	UDWORD			integer_val;
	VAR_DECL		*vdecl;
	VAR_IDENT_DECL	*videcl;



/* Line 2068 of yacc.c  */
#line 232 "script_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE scr_lval;


