/* lex -a -p strres_ -o strres_l.c StrRes.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define QUOTE 4
#define SLCOMMENT 6
#define strres__endst 23
#define strres__nxtmax 1129
#define YY_LA_SIZE 3

static unsigned int strres__la_act[] = {
 0, 14, 1, 14, 5, 14, 5, 14, 14, 6, 11, 0, 9, 9, 10, 7,
 8, 4, 2, 3, 4, 13, 12, 13, 0
};

static unsigned char strres__look[] = {
 0
};

static int strres__final[] = {
 0, 0, 2, 4, 6, 7, 8, 9, 10, 11, 12, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24
};
#ifndef strres__state_t
#define strres__state_t unsigned char
#endif

static strres__state_t strres__begin[] = {
 0, 0, 10, 10, 16, 16, 20, 20, 0
};

static strres__state_t strres__next[] = {
 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 4, 6, 6, 3, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 3, 6, 2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6, 6, 6, 6,
 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 7, 14, 15, 9, 23, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 23, 23, 23, 23, 23, 23, 23, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 23, 23, 23, 23, 9, 23, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 11, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 18, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 17, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
 19, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 0
};

static strres__state_t strres__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 5, 11, 14, 1, 22, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, ~0U, ~0U, ~0U, ~0U, 1, ~0U, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 19, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 19, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0
};

static strres__state_t strres__default[] = {
 23, 23, 23, 23, 23, 23, 23, 23, 23, 1, 23, 23, 23, 23, 23, 23,
 23, 23, 23, 16, 23, 23, 20, 0
};

static int strres__base[] = {
 0, 214, 1130, 1130, 1130, 214, 1130, 1130, 1130, 1130, 337, 210, 1130, 1130, 248, 1130,
 593, 1130, 1130, 839, 874, 1130, 250, 1130
};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/strres_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
 *
 */
#include <stdlib.h>
#include <stdio.h>
#if	__STDC__
#define YY_ARGS(args)	args
#else
#define YY_ARGS(args)	()
#endif

#ifdef LEX_WINDOWS
#include <windows.h>

/*
 * define, if not already defined
 * the flag YYEXIT, which will allow
 * graceful exits from strres_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling strres_lex must supply this!
 */

#ifdef STRICT
extern HINSTANCE hInst;	
#else
extern HANDLE hInst;	
#endif

#endif	/* LEX_WINDOWS */

/*
 * Define m_textmsg() to an appropriate function for internationalized messages
 * or custom processing.
 */
#ifndef I18N
#define	m_textmsg(id, str, cls)	(str)
#else /*I18N*/
extern	char* m_textmsg YY_ARGS((int id, const char* str, char* cls));
#endif/*I18N*/

/*
 * Include string.h to get definition of memmove() and size_t.
 * If you do not have string.h or it does not declare memmove
 * or size_t, you will have to declare them here.
 */
#include <string.h>
/* Uncomment next line if memmove() is not declared in string.h */
/*extern char * memmove();*/
/* Uncomment next line if size_t is not available in stdio.h or string.h */
/*typedef unsigned size_t;*/
/* Drop this when LATTICE provides memmove */
#ifdef LATTICE
#define memmove	memcopy
#endif

/*
 * YY_STATIC determines the scope of variables and functions
 * declared by the lex scanner. It must be set with a -DYY_STATIC
 * option to the compiler (it cannot be defined in the lex program).
 */
#ifdef	YY_STATIC
/* define all variables as static to allow more than one lex scanner */
#define	YY_DECL	static
#else
/* define all variables as global to allow other modules to access them */
#define	YY_DECL	
#endif


/*
 * You can redefine strres_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define strres_getc()	yt_getc()
#else
#define	strres_getc()	getc(strres_in) 	/* strres_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(strres_out, "strres_lex: %s\n", msg); strres_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "strres_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(strres_text, strres_out)

#define	output(c)	putc((c), strres_out) /* strres_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		strres__start =
#define	REJECT		goto strres__reject
#define	NLSTATE		(strres__lastc = YYNEWLINE)
#define	YY_INIT \
	(strres__start = strres_leng = strres__end = 0, strres__lastc = YYNEWLINE)
#define	strres_more()	goto strres__more
#define	strres_less(n)	if ((n) < 0 || (n) > strres__end) ; \
			else { YY_SCANNER; strres_leng = (n); YY_USER; }

YY_DECL	void	strres__reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	strres_wrap	YY_ARGS((void));
extern	void	strres_error	YY_ARGS((char *fmt, ...));
extern	void	strres_comment	YY_ARGS((char *term));
extern	int	strres_mapch	YY_ARGS((int delim, int escape));

/*
 * StrRes.l
 *
 * Lex file for parsing string resource files
 */


#include <stdio.h>


/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <string.h>
#include "types.h"
#include "debug.h"
#include "mem.h"
#include "heap.h"
#include "treap.h"
#include "strres.h"
#include "strresly.h"

/* Get the Yacc definitions */
#include "strres_y.h"

/* Turn off a couple of warnings that the lex generated code gives */
#pragma warning ( disable : 4102 4305 )

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* Store for any string values */
extern STRING aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if in a comment
static BOOL inComment;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef strres_getc
#define strres_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), strres_text[] contains the token.
 * If %pointer is used, strres_text is a pointer to strres__tbuf[].
 */
YY_DECL char	strres_text[YYLMAX+1];



#ifdef	YY_DEBUG
#undef	YY_DEBUG
#define	YY_DEBUG(fmt, a1, a2)	fprintf(stderr, fmt, a1, a2)
#else
#define	YY_DEBUG(fmt, a1, a2)
#endif

/*
 * The declaration for the lex scanner can be changed by
 * redefining YYLEX or YYDECL. This must be done if you have
 * more than one scanner in a program.
 */
#ifndef	YYLEX
#define	YYLEX strres_lex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/*
 * stdin and stdout may not neccessarily be constants.
 * If stdin and stdout are constant, and you want to save a few cycles, then
 * #define YY_STATIC_STDIO 1 in this file or on the commandline when
 * compiling this file
 */
#ifndef YY_STATIC_STDIO
#define YY_STATIC_STDIO	0
#endif

#if YY_STATIC_STDIO
YY_DECL	FILE   *strres_in = stdin;
YY_DECL	FILE   *strres_out = stdout;
#else
YY_DECL	FILE   *strres_in = (FILE *)0;
YY_DECL	FILE   *strres_out = (FILE *)0;
#endif
YY_DECL	int	strres_lineno = 1;		/* line number */

/* strres__sbuf[0:strres_leng-1] contains the states corresponding to strres_text.
 * strres_text[0:strres_leng-1] contains the current token.
 * strres_text[strres_leng:strres__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * strres__save contains strres_text[strres_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	strres__state_t strres__sbuf [YYLMAX+1];	/* state buffer */
static	int	strres__end = 0;		/* end of pushback */
static	int	strres__start = 0;		/* start state */
static	int	strres__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	strres_leng = 0;		/* strres_text token length */
#ifdef YYEXIT
static	int strres_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char strres__save;	/* saved strres_text[strres_leng] */

#define	YY_USER	{ /* set up strres_text for user */ \
		strres__save = strres_text[strres_leng]; \
		strres_text[strres_leng] = 0; \
	}
#define	YY_SCANNER { /* set up strres_text for scanner */ \
		strres_text[strres_leng] = strres__save; \
	}

#else		/* not-so efficient push-back for strres_text mungers */

static	char strres__save [YYLMAX];
static	char *strres__push = strres__save+YYLMAX;

#define	YY_USER { \
		size_t n = strres__end - strres_leng; \
		strres__push = strres__save+YYLMAX - n; \
		if (n > 0) \
			memmove(strres__push, strres_text+strres_leng, n); \
		strres_text[strres_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = strres__save+YYLMAX - strres__push; \
		if (n > 0) \
			memmove(strres_text+strres_leng, strres__push, n); \
		strres__end = strres_leng + n; \
	}

#endif


#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make strres_lex()
 * behave in a windows environment, create a new
 * strres_lex() which will call the original strres_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually strres_lex(void)).
 * NOTE: you should invoke strres__init() if you are calling strres_lex()
 * with new input; otherwise old lookaside will get in your way
 * and strres_lex() will die horribly.
 */
static int win_strres_lex();			/* prototype for windows strres_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_strres__la_act;	/* remember previous pointer values */
	short far *old_strres__final;
	strres__state_t far *old_strres__begin;
	strres__state_t far *old_strres__next;
	strres__state_t far *old_strres__check;
	strres__state_t far *old_strres__default;
	short far *old_strres__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_strres_LEX", "strres_LEXTBL"));
	
	/*
	 * return an error code if any
	 * of the resources did not load
	 */

	if (hRes_table == NULL)
		return (0);
	
	/*
	 * the following code will lock the resources
	 * into fixed memory locations for the scanner
	 * (and remember previous pointer locations)
	 */

	old_strres__la_act = strres__la_act;
	old_strres__final = strres__final;
	old_strres__begin = strres__begin;
	old_strres__next = strres__next;
	old_strres__check = strres__check;
	old_strres__default = strres__default;
	old_strres__base = strres__base;

	strres__la_act = (unsigned short far *)LockResource (hRes_table);
	strres__final = (short far *)(strres__la_act + Sizeof_strres__la_act);
	strres__begin = (strres__state_t far *)(strres__final + Sizeof_strres__final);
	strres__next = (strres__state_t far *)(strres__begin + Sizeof_strres__begin);
	strres__check = (strres__state_t far *)(strres__next + Sizeof_strres__next);
	strres__default = (strres__state_t far *)(strres__check + Sizeof_strres__check);
	strres__base = (strres__state_t far *)(strres__default + Sizeof_strres__default);


	/*
	 * call the standard strres_lex() code
	 */

	wReturnValue = win_strres_lex();

	/*
	 * unlock the resources
	 */

	UnlockResource (hRes_table);

	/*
	 * and now free the resource
	 */

	FreeResource (hRes_table);

	/*
	 * restore previously saved pointers
	 */

	strres__la_act = old_strres__la_act;
	strres__final = old_strres__final;
	strres__begin = old_strres__begin;
	strres__next = old_strres__next;
	strres__check = old_strres__check;
	strres__default = old_strres__default;
	strres__base = old_strres__base;

	return (wReturnValue);
}	/* end function */

static int win_strres_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually strres_lex(void)).
 * NOTE: you should invoke strres__init() if you are calling strres_lex()
 * with new input; otherwise old lookaside will get in your way
 * and strres_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, strres_base;
	unsigned	strres_st;	/* state */
	int strres_fmin, strres_fmax;	/* strres__la_act indices of final states */
	int strres_oldi, strres_oleng;	/* base i, strres_leng before look-ahead */
	int strres_eof;		/* 1 if eof has already been read */

#if !YY_STATIC_STDIO
	if (strres_in == (FILE *)0)
		strres_in = stdin;
	if (strres_out == (FILE *)0)
		strres_out = stdout;
#endif

#ifdef YYEXIT
	strres_LexFatal = 0;
#endif /* YYEXIT */

	strres_eof = 0;
	i = strres_leng;
	YY_SCANNER;

  strres__again:
	strres_leng = i;
	/* determine previous char. */
	if (i > 0)
		strres__lastc = strres_text[i-1];
	/* scan previously accepted token adjusting strres_lineno */
	while (i > 0)
		if (strres_text[--i] == YYNEWLINE)
			strres_lineno++;
	/* adjust pushback */
	strres__end -= strres_leng;
	if (strres__end > 0)
		memmove(strres_text, strres_text+strres_leng, (size_t) strres__end);
	i = 0;

  strres__contin:
	strres_oldi = i;

	/* run the state machine until it jams */
	strres_st = strres__begin[strres__start + ((strres__lastc == YYNEWLINE) ? 1 : 0)];
	strres__sbuf[i] = (strres__state_t) strres_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), strres_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (strres_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < strres__end)
			c = strres_text[i];		/* get pushback char */
		else if (!strres_eof && (c = strres_getc()) != EOF) {
			strres__end = i+1;
			strres_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == strres_oldi) {	/* no token */
				strres_eof = 0;
				if (strres_wrap())
					return 0;
				else
					goto strres__again;
			} else {
				strres_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((strres_base = strres__base[strres_st]+(unsigned char)c) > strres__nxtmax
		    || strres__check[strres_base] != (strres__state_t) strres_st) {
			if (strres_st == strres__endst)
				goto strres__jammed;
			strres_st = strres__default[strres_st];
		}
		strres_st = strres__next[strres_base];
	  strres__jammed: ;
	  strres__sbuf[++i] = (strres__state_t) strres_st;
	} while (!(strres_st == strres__endst || YY_INTERACTIVE && strres__base[strres_st] > strres__nxtmax && strres__default[strres_st] == strres__endst));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), strres_st, i);
	if (strres_st != strres__endst)
		++i;

  strres__search:
	/* search backward for a final state */
	while (--i > strres_oldi) {
		strres_st = strres__sbuf[i];
		if ((strres_fmin = strres__final[strres_st]) < (strres_fmax = strres__final[strres_st+1]))
			goto strres__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = strres_oldi + 1;
	output(strres_text[strres_oldi]);
	goto strres__again;

  strres__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), strres_st, i);
	strres_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(strres__la_act[strres_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = strres__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < strres_oldi) {	/* no / */
				i = strres_oleng;
				break;
			}
			strres_st = strres__sbuf[i];
			if (bv[(unsigned)strres_st/8] & bits[(unsigned)strres_st%8])
				break;
		}
	}

	/* perform action */
	strres_leng = i;
	YY_USER;
	switch (strres__la_act[strres_fmin] & 0777) {
	case 0:
	{
								strcpy(aText[currText], strres_text);
								strres_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return TEXT;
							}
	break;
	case 1:
	{ BEGIN QUOTE; }
	break;
	case 2:
	{ BEGIN 0; }
	break;
	case 3:
	{ strres_error("Unexpected end of line in string"); }
	break;
	case 4:
	{
								strcpy(aText[currText], strres_text);
								strres_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}
	break;
	case 5:
	;
	break;
	case 6:
	{ inComment=TRUE; BEGIN COMMENT; }
	break;
	case 7:
	case 8:
	{ inComment=FALSE; BEGIN 0; }
	break;
	case 9:
	case 10:
	;
	break;
	case 11:
	{ BEGIN SLCOMMENT; }
	break;
	case 12:
	{ BEGIN 0; }
	break;
	case 13:
	;
	break;
	case 14:
	return strres_text[0];
	break;
	}
	YY_SCANNER;
	i = strres_leng;
	goto strres__again;			/* action fell though */

  strres__reject:
	YY_SCANNER;
	i = strres_oleng;			/* restore original strres_text */
	if (++strres_fmin < strres_fmax)
		goto strres__found;		/* another final state, same length */
	else
		goto strres__search;		/* try shorter strres_text */

  strres__more:
	YY_SCANNER;
	i = strres_leng;
	if (i > 0)
		strres__lastc = strres_text[i-1];
	goto strres__contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct strres__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	strres__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
strres_SaveScan(FILE *fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = strres_in;
	p->oldline = strres_lineno;
	p->oldend = strres__end;
	p->oldstart = strres__start;
	p->oldlastc = strres__lastc;
	p->oldleng = strres_leng;
	(void) memcpy(p->savetext, strres_text, sizeof strres_text);
	(void) memcpy((char *) p->savestate, (char *) strres__sbuf,
		sizeof strres__sbuf);

	strres_in = fp;
	strres_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
strres_RestoreScan(YY_SAVED *p)
{
	if (p == NULL)
		return;
	strres_in = p->oldfp;
	strres_lineno = p->oldline;
	strres__end = p->oldend;
	strres__start = p->oldstart;
	strres__lastc = p->oldlastc;
	strres_leng = p->oldleng;

	(void) memcpy(strres_text, p->savetext, sizeof strres_text);
	(void) memcpy((char *) strres__sbuf, (char *) p->savestate,
		sizeof strres__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of strres_lex()
 */
void
strres__reset()
{
	YY_INIT;
	strres_lineno = 1;		/* line number */
}
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (strres__end > strres_leng) {
		strres__end--;
		memmove(strres_text+strres_leng, strres_text+strres_leng+1,
			(size_t) (strres__end-strres_leng));
		c = strres__save;
		YY_USER;
#else
	if (strres__push < strres__save+YYLMAX) {
		c = *strres__push++;
#endif
	} else
		c = strres_getc();
	strres__lastc = c;
	if (c == YYNEWLINE)
		strres_lineno++;
	if (c == EOF) /* strres_getc() can set c=EOF vsc4 wants c==EOF to return 0 */
		return 0;
	else
		return c;
}

/*f
 * pushback char
 */
YY_DECL int
unput(int c)
{
#ifndef YY_PRESERVE
	if (strres__end >= YYLMAX) {
		YY_FATAL(m_textmsg(1552, "Push-back buffer overflow", "E"));
	} else {
		if (strres__end > strres_leng) {
			strres_text[strres_leng] = strres__save;
			memmove(strres_text+strres_leng+1, strres_text+strres_leng,
				(size_t) (strres__end-strres_leng));
			strres_text[strres_leng] = 0;
		}
		strres__end++;
		strres__save = (char) c;
#else
	if (strres__push <= strres__save) {
		YY_FATAL(m_textmsg(1552, "Push-back buffer overflow", "E"));
	} else {
		*--strres__push = c;
#endif
		if (c == YYNEWLINE)
			strres_lineno--;
	}	/* endif */
	return c;
}

/* Set the current input buffer for the lexer */
void strresSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	strres__reset();
}

void strresGetErrorData(int *pLine, char **ppText)
{
	*pLine = strres_lineno;
	*ppText = strres_text;
}

int strres_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}

	return 1;
}

