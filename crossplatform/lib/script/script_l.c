/* lex -p scr_ -o Script_l.c Script.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define SLCOMMENT 4
#define QUOTE 6
#define scr__endst 139
#define scr__nxtmax 680
#define YY_LA_SIZE 18

static unsigned int scr__la_act[] = {
 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 36, 49,
 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 49, 49, 27, 49, 28, 49,
 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 36, 49, 49, 35, 49, 36,
 49, 37, 49, 40, 49, 40, 49, 49, 41, 46, 36, 35, 36, 34, 36, 36,
 33, 36, 32, 36, 31, 36, 36, 30, 36, 36, 29, 36, 26, 25, 24, 23,
 36, 36, 36, 22, 36, 36, 36, 36, 21, 36, 36, 36, 19, 36, 36, 18,
 36, 36, 36, 16, 36, 36, 36, 15, 36, 36, 36, 36, 36, 36, 14, 36,
 36, 36, 36, 36, 9, 36, 36, 36, 36, 8, 36, 36, 7, 36, 36, 36,
 6, 36, 36, 11, 36, 36, 36, 17, 36, 5, 36, 36, 36, 36, 36, 4,
 36, 36, 36, 36, 20, 36, 36, 36, 36, 2, 36, 36, 36, 36, 36, 13,
 36, 36, 12, 36, 36, 36, 36, 3, 36, 1, 36, 36, 36, 36, 36, 10,
 36, 36, 0, 36, 44, 44, 45, 42, 43, 48, 47, 48, 39, 38, 39, 0

};

static unsigned char scr__look[] = {
 0
};

static int scr__final[] = {
 0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 27,
 28, 30, 32, 34, 36, 38, 40, 42, 44, 45, 47, 49, 51, 53, 54, 55,
 56, 57, 58, 59, 60, 61, 63, 64, 66, 68, 70, 71, 73, 74, 76, 77,
 78, 79, 80, 81, 82, 83, 85, 86, 87, 88, 90, 91, 92, 94, 95, 97,
 98, 99, 101, 102, 103, 105, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116,
 118, 119, 120, 121, 123, 124, 126, 127, 128, 130, 131, 133, 134, 135, 137, 139,
 140, 141, 142, 143, 145, 146, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158,
 159, 161, 162, 164, 165, 166, 167, 169, 171, 172, 173, 174, 175, 177, 178, 180,
 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191
};
#ifndef scr__state_t
#define scr__state_t unsigned char
#endif

static scr__state_t scr__begin[] = {
 0, 0, 127, 127, 133, 133, 136, 136, 0
};

static scr__state_t scr__next[] = {
 31, 31, 31, 31, 31, 31, 31, 31, 31, 28, 29, 31, 31, 28, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 28, 15, 27, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 24, 31, 30,
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 31, 31, 17, 14, 16, 31,
 31, 19, 9, 26, 26, 26, 12, 26, 26, 10, 26, 26, 26, 26, 23, 21,
 26, 26, 26, 26, 11, 26, 26, 26, 26, 26, 26, 31, 31, 31, 31, 31,
 31, 18, 8, 26, 26, 2, 13, 26, 26, 4, 26, 26, 5, 26, 22, 20,
 7, 26, 6, 26, 3, 26, 26, 1, 26, 26, 26, 31, 31, 31, 31, 31,
 32, 36, 37, 38, 39, 33, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 40, 41, 42, 43, 44, 45, 46, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 47, 48, 49, 50, 34, 51, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 52, 53, 54, 55, 56,
 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 71, 72, 73, 74,
 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 70, 88, 90,
 69, 91, 94, 95, 96, 97, 98, 89, 99, 92, 100, 101, 103, 104, 105, 106,
 107, 109, 111, 112, 93, 113, 114, 102, 115, 118, 117, 108, 119, 110, 116, 120,
 122, 123, 124, 125, 126, 131, 121, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 130, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 128, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129,
 129, 129, 129, 129, 129, 129, 129, 132, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 134, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135, 135,
 135, 135, 135, 135, 135, 135, 135, 135, 139, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 139, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 137, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138, 138,
 138, 138, 138, 138, 138, 138, 138, 138, 138, 0
};

static scr__state_t scr__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 30, 23, 36, 22, 38, 30, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 21, 20, 19, 42, 18, 44, 17, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 16, 15, 14, 13, 26, 50, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 51, 52, 12, 54, 55,
 56, 11, 58, 59, 10, 61, 9, 63, 64, 8, 66, 67, 7, 71, 72, 73,
 70, 75, 76, 77, 78, 69, 80, 81, 82, 6, 84, 5, 86, 7, 87, 4,
 7, 89, 92, 91, 95, 96, 97, 4, 98, 89, 3, 100, 102, 101, 104, 105,
 106, 2, 110, 111, 89, 109, 113, 100, 108, 117, 115, 2, 116, 2, 115, 1,
 121, 122, 123, 120, 125, 128, 1, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
 127, 127, 127, 127, 127, 127, 127, 131, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133, 133,
 133, 133, 133, 133, 133, 133, 133, 133, 135, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 138, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 0
};

static scr__state_t scr__default[] = {
 139, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 139, 139,
 139, 139, 26, 26, 26, 26, 26, 26, 25, 139, 139, 139, 139, 139, 139, 139,
 139, 139, 26, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 139, 139,
 139, 139, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 139,
 139, 139, 139, 139, 139, 139, 139, 133, 139, 139, 136, 0
};

static int scr__base[] = {
 0, 190, 165, 152, 153, 146, 148, 139, 122, 151, 150, 143, 156, 83, 118, 117,
 116, 89, 38, 68, 31, 62, 20, 50, 681, 161, 86, 681, 681, 681, 86, 681,
 681, 681, 681, 681, 46, 681, 16, 681, 681, 681, 79, 681, 49, 681, 681, 681,
 681, 681, 74, 104, 119, 681, 146, 140, 155, 681, 141, 158, 681, 145, 681, 152,
 156, 681, 123, 127, 681, 147, 135, 120, 123, 138, 681, 123, 145, 127, 143, 681,
 138, 142, 149, 681, 148, 681, 142, 147, 681, 160, 681, 160, 142, 681, 681, 144,
 156, 144, 163, 681, 162, 166, 167, 681, 167, 170, 158, 681, 179, 162, 169, 159,
 681, 177, 681, 172, 163, 165, 681, 681, 186, 183, 181, 189, 681, 176, 681, 295,
 246, 681, 681, 413, 681, 424, 681, 542, 553, 681, 529, 681
};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/scr_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
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
 * graceful exits from scr_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling scr_lex must supply this!
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
 * You can redefine scr_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define scr_getc()	yt_getc()
#else
#define	scr_getc()	getc(scr_in) 	/* scr_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(scr_out, "scr_lex: %s\n", msg); scr_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "scr_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(scr_text, scr_out)

#define	output(c)	putc((c), scr_out) /* scr_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		scr__start =
#define	REJECT		goto scr__reject
#define	NLSTATE		(scr__lastc = YYNEWLINE)
#define	YY_INIT \
	(scr__start = scr_leng = scr__end = 0, scr__lastc = YYNEWLINE)
#define	scr_more()	goto scr__more
#define	scr_less(n)	if ((n) < 0 || (n) > scr__end) ; \
			else { YY_SCANNER; scr_leng = (n); YY_USER; }

YY_DECL	void	scr__reset YY_ARGS((void));

/* functions defined in libl.lib */
extern	int	scr_wrap	YY_ARGS((void));
extern	void	scr_error	YY_ARGS((char *fmt, ...));
extern	void	scr_comment	YY_ARGS((char *term));
extern	int	scr_mapch	YY_ARGS((int delim, int escape));

/*
 * script.l
 *
 * Script file lexer.
 */
#ifdef PSX
/* A few definitions so the lex generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
#ifndef _FILE_
#define _FILE_
typedef int FILE;
#endif
#define stderr 0
#define stdin  0
#define stdout 0
static int fprintf(FILE* f,char* c,...)
{
	return 0;
}
#endif

#include "frame.h"
#include "interp.h"
#include "parse.h"
#include "script.h"

/* Get the Yacc definitions */
#include "script_y.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* Store for any string values */
static STRING aText[TEXT_BUFFERS][YYLMAX];
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef scr_getc
#define scr_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

/* Get the token type for a variable symbol */
SDWORD scriptGetVarToken(VAR_SYMBOL *psVar)
{
	BOOL	object;

	// See if this is an object pointer
	if (!asScrTypeTab || psVar->type < VAL_USERTYPESTART)
	{
		object = FALSE;
	}
	else
	{
		object = asScrTypeTab[psVar->type - VAL_USERTYPESTART].accessType == AT_OBJECT;
	}

	if (psVar->storage == ST_OBJECT)
	{
		/* This is an object member variable */
		if (object)
		{
			return OBJ_OBJVAR;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_OBJVAR;
				break;
			case VAL_INT:
//			case VAL_FLOAT:
				return NUM_OBJVAR;
				break;
			default:
				return USER_OBJVAR;
				break;
			}
		}
	}
	else if (psVar->dimensions > 0)
	{
		/* This is an array variable */
		if (object)
		{
			return OBJ_ARRAY;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_ARRAY;
				break;
			case VAL_INT:
//			case VAL_FLOAT:
				return NUM_ARRAY;
				break;
			default:
				return VAR_ARRAY;
				break;
			}
		}
	}
	else
	{
		/* This is a standard variable */
		if (object)
		{
			return OBJ_VAR;
		}
		else
		{
			switch (psVar->type)
			{
			case VAL_BOOL:
				return BOOL_VAR;
				break;
			case VAL_INT:
//			case VAL_FLOAT:
				return NUM_VAR;
				break;
			default:
				return VAR;
				break;
			}
		}
	}
}

/* Get the token type for a constant symbol */
SDWORD scriptGetConstToken(CONST_SYMBOL *psConst)
{
	BOOL	object;

	// See if this is an object constant
	if (!asScrTypeTab || psConst->type < VAL_USERTYPESTART)
	{
		object = FALSE;
	}
	else
	{
		object = asScrTypeTab[psConst->type - VAL_USERTYPESTART].accessType == AT_OBJECT;
	}

	switch (psConst->type)
	{
	case VAL_BOOL:
		return BOOL_CONSTANT;
		break;
	case VAL_INT:
//	case VAL_FLOAT:
		return NUM_CONSTANT;
		break;
	default:
		if (object)
		{
			return OBJ_CONSTANT;
		}
		else
		{
			return USER_CONSTANT;
		}
		break;
	}
}

/* Get the token type for a function symbol */
SDWORD scriptGetFuncToken(FUNC_SYMBOL *psFunc)
{
	BOOL	object;

	// See if this is an object pointer
	object = asScrTypeTab[psFunc->type - VAL_USERTYPESTART].accessType == AT_OBJECT;

	if (object)
	{
		return OBJ_FUNC;
	}
	else
	{
		switch (psFunc->type)
		{
		case VAL_BOOL:
			return BOOL_FUNC;
			break;
		case VAL_INT:
//		case VAL_FLOAT:
			return NUM_FUNC;
			break;
		case VAL_VOID:
			return FUNC;
			break;
		default:
			return USER_FUNC;
			break;
		}
	}
}

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), scr_text[] contains the token.
 * If %pointer is used, scr_text is a pointer to scr__tbuf[].
 */
YY_DECL char	scr_text[YYLMAX+1];

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
#define	YYLEX scr_lex			/* name of lex scanner */
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
YY_DECL	FILE   *scr_in = stdin;
YY_DECL	FILE   *scr_out = stdout;
#else
YY_DECL	FILE   *scr_in = (FILE *)0;
YY_DECL	FILE   *scr_out = (FILE *)0;
#endif
YY_DECL	int	scr_lineno = 1;		/* line number */

/* scr__sbuf[0:scr_leng-1] contains the states corresponding to scr_text.
 * scr_text[0:scr_leng-1] contains the current token.
 * scr_text[scr_leng:scr__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * scr__save contains scr_text[scr_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	scr__state_t scr__sbuf [YYLMAX+1];	/* state buffer */
static	int	scr__end = 0;		/* end of pushback */
static	int	scr__start = 0;		/* start state */
static	int	scr__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	scr_leng = 0;		/* scr_text token length */
#ifdef YYEXIT
static	int scr_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char scr__save;	/* saved scr_text[scr_leng] */

#define	YY_USER	{ /* set up scr_text for user */ \
		scr__save = scr_text[scr_leng]; \
		scr_text[scr_leng] = 0; \
	}
#define	YY_SCANNER { /* set up scr_text for scanner */ \
		scr_text[scr_leng] = scr__save; \
	}

#else		/* not-so efficient push-back for scr_text mungers */

static	char scr__save [YYLMAX];
static	char *scr__push = scr__save+YYLMAX;

#define	YY_USER { \
		size_t n = scr__end - scr_leng; \
		scr__push = scr__save+YYLMAX - n; \
		if (n > 0) \
			memmove(scr__push, scr_text+scr_leng, n); \
		scr_text[scr_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = scr__save+YYLMAX - scr__push; \
		if (n > 0) \
			memmove(scr_text+scr_leng, scr__push, n); \
		scr__end = scr_leng + n; \
	}

#endif

#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make scr_lex()
 * behave in a windows environment, create a new
 * scr_lex() which will call the original scr_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually scr_lex(void)).
 * NOTE: you should invoke scr__init() if you are calling scr_lex()
 * with new input; otherwise old lookaside will get in your way
 * and scr_lex() will die horribly.
 */
static int win_scr_lex();			/* prototype for windows scr_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_scr__la_act;	/* remember previous pointer values */
	short far *old_scr__final;
	scr__state_t far *old_scr__begin;
	scr__state_t far *old_scr__next;
	scr__state_t far *old_scr__check;
	scr__state_t far *old_scr__default;
	short far *old_scr__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_scr_LEX", "scr_LEXTBL"));
	
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

	old_scr__la_act = scr__la_act;
	old_scr__final = scr__final;
	old_scr__begin = scr__begin;
	old_scr__next = scr__next;
	old_scr__check = scr__check;
	old_scr__default = scr__default;
	old_scr__base = scr__base;

	scr__la_act = (unsigned short far *)LockResource (hRes_table);
	scr__final = (short far *)(scr__la_act + Sizeof_scr__la_act);
	scr__begin = (scr__state_t far *)(scr__final + Sizeof_scr__final);
	scr__next = (scr__state_t far *)(scr__begin + Sizeof_scr__begin);
	scr__check = (scr__state_t far *)(scr__next + Sizeof_scr__next);
	scr__default = (scr__state_t far *)(scr__check + Sizeof_scr__check);
	scr__base = (scr__state_t far *)(scr__default + Sizeof_scr__default);


	/*
	 * call the standard scr_lex() code
	 */

	wReturnValue = win_scr_lex();

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

	scr__la_act = old_scr__la_act;
	scr__final = old_scr__final;
	scr__begin = old_scr__begin;
	scr__next = old_scr__next;
	scr__check = old_scr__check;
	scr__default = old_scr__default;
	scr__base = old_scr__base;

	return (wReturnValue);
}	/* end function */

static int win_scr_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually scr_lex(void)).
 * NOTE: you should invoke scr__init() if you are calling scr_lex()
 * with new input; otherwise old lookaside will get in your way
 * and scr_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, scr_base;
	unsigned	scr_st;	/* state */
	int scr_fmin, scr_fmax;	/* scr__la_act indices of final states */
	int scr_oldi, scr_oleng;	/* base i, scr_leng before look-ahead */
	int scr_eof;		/* 1 if eof has already been read */

#if !YY_STATIC_STDIO
	if (scr_in == (FILE *)0)
		scr_in = stdin;
	if (scr_out == (FILE *)0)
		scr_out = stdout;
#endif

#ifdef YYEXIT
	scr_LexFatal = 0;
#endif /* YYEXIT */

	scr_eof = 0;
	i = scr_leng;
	YY_SCANNER;

  scr__again:
	scr_leng = i;
	/* determine previous char. */
	if (i > 0)
		scr__lastc = scr_text[i-1];
	/* scan previously accepted token adjusting scr_lineno */
	while (i > 0)
		if (scr_text[--i] == YYNEWLINE)
			scr_lineno++;
	/* adjust pushback */
	scr__end -= scr_leng;
	if (scr__end > 0)
		memmove(scr_text, scr_text+scr_leng, (size_t) scr__end);
	i = 0;

	scr_oldi = i;

	/* run the state machine until it jams */
	scr_st = scr__begin[scr__start + ((scr__lastc == YYNEWLINE) ? 1 : 0)];
	scr__sbuf[i] = (scr__state_t) scr_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), scr_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (scr_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < scr__end)
			c = scr_text[i];		/* get pushback char */
		else if (!scr_eof && (c = scr_getc()) != EOF) {
			scr__end = i+1;
			scr_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == scr_oldi) {	/* no token */
				scr_eof = 0;
				if (scr_wrap())
					return 0;
				else
					goto scr__again;
			} else {
				scr_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((scr_base = scr__base[scr_st]+(unsigned char)c) > scr__nxtmax
		    || scr__check[scr_base] != (scr__state_t) scr_st) {
			if (scr_st == scr__endst)
				goto scr__jammed;
			scr_st = scr__default[scr_st];
		}
		scr_st = scr__next[scr_base];
	  scr__jammed: ;
	  scr__sbuf[++i] = (scr__state_t) scr_st;
	} while (!(scr_st == scr__endst || (YY_INTERACTIVE && scr__base[scr_st] > scr__nxtmax && scr__default[scr_st] == scr__endst)));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), scr_st, i);
	if (scr_st != scr__endst)
		++i;

	/* search backward for a final state */
	while (--i > scr_oldi) {
		scr_st = scr__sbuf[i];
		if ((scr_fmin = scr__final[scr_st]) < (scr_fmax = scr__final[scr_st+1]))
			goto scr__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = scr_oldi + 1;
	output(scr_text[scr_oldi]);
	goto scr__again;

  scr__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), scr_st, i);
	scr_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(scr__la_act[scr_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = scr__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < scr_oldi) {	/* no / */
				i = scr_oleng;
				break;
			}
			scr_st = scr__sbuf[i];
			if (bv[(unsigned)scr_st/8] & bits[(unsigned)scr_st%8])
				break;
		}
	}

	/* perform action */
	scr_leng = i;
	YY_USER;
	switch (scr__la_act[scr_fmin] & 0777) {
	case 0:
	return WAIT;
	break;
	case 1:
	return EVERY;
	break;
	case 2:
	return TRIGGER;
	break;
	case 3:
	return EVENT;
	break;
	case 4:
	return INACTIVE;
	break;
	case 5:
	return INITIALISE;
	break;
	case 6:
	return LINK;
	break;
	case 7:
	return REF;
	break;
	case 8:
	{ scr_lval.stype = ST_PUBLIC; return STORAGE; }
	break;
	case 9:
	{ scr_lval.stype = ST_PRIVATE; return STORAGE; }
	break;
	case 10:
	return WHILE;
	break;
	case 11:
	return IF;
	break;
	case 12:
	return ELSE;
	break;
	case 13:
	return EXIT;
	break;
	case 14:
	return PAUSE;
	break;
	case 15:
	{ scr_lval.tval = VAL_BOOL; return TYPE; }
	break;
	case 16:
	{ scr_lval.tval = VAL_BOOL; return TYPE; }
	break;
	case 17:
	{ scr_lval.tval = VAL_INT; return TYPE; }
	break;
	case 18:
	{ scr_lval.tval = VAL_INT; return TYPE; }
	/*float					{ ais_lval.tval = VAL_FLOAT; return TYPE; }*/
	/* string type isn't implemented yet */
	/* string					{ ais_lval.tval = VAL_STRING; return TYPE; } */
	/* object					{ scr_lval.tval = VAL_OBJECT; return TYPE; } */
	break;
	case 19:
	{ scr_lval.bval = TRUE; return BOOLEAN; }
	break;
	case 20:
	{ scr_lval.bval = TRUE; return BOOLEAN; }
	break;
	case 21:
	{ scr_lval.bval = FALSE; return BOOLEAN; }
	break;
	case 22:
	{ scr_lval.bval = FALSE; return BOOLEAN; }
	break;
	case 23:
	return BOOLEQUAL;
	break;
	case 24:
	return NOTEQUAL;
	break;
	case 25:
	return GREATEQUAL;
	break;
	case 26:
	return LESSEQUAL;
	break;
	case 27:
	return GREATER;
	break;
	case 28:
	return LESS;
	break;
	case 29:
	return _AND;
	break;
	case 30:
	return _AND;
	break;
	case 31:
	return _OR;
	break;
	case 32:
	return _OR;
	break;
	case 33:
	return _NOT;
	break;
	case 34:
	return _NOT;
	break;
	case 35:
	{ scr_lval.ival = atol(scr_text); return INTEGER; }
	break;
	case 36:
	{
								/* See if this identifier has been defined as a type */
								if (scriptLookUpType(scr_text, &scr_lval.tval))
								{
									DBP0(("[lex] TYPE\n"));
									return TYPE;
								}
								/* See if this identifier has been defined as a variable */
								else if (scriptLookUpVariable(scr_text, &scr_lval.vSymbol))
								{
									DBP0(("[lex] variable\n"));
									return scriptGetVarToken(scr_lval.vSymbol);
								}
								/* See if this identifier has been defined as a constant */
								else if (scriptLookUpConstant(scr_text, &scr_lval.cSymbol))
								{
									DBP0(("[lex] constant\n"));
									return scriptGetConstToken(scr_lval.cSymbol);
								}
								/* See if this identifier has been defined as a function */
								else if (scriptLookUpFunction(scr_text, &scr_lval.fSymbol))
								{
									DBP0(("[lex] func\n"));
									return scriptGetFuncToken(scr_lval.fSymbol);
								}
								else if (scriptLookUpTrigger(scr_text, &scr_lval.tSymbol))
								{
									DBP0(("[lex] TRIG_SYM\n"));
									return TRIG_SYM;
								}
								else if (scriptLookUpEvent(scr_text, &scr_lval.eSymbol))
								{
									DBP0(("[lex] EVENT_SYM\n"));
									return EVENT_SYM;
								}
								else if (scriptLookUpCallback(scr_text, &scr_lval.cbSymbol))
								{
									DBP0(("[lex] CALLBACK_SYM\n"));
									return CALLBACK_SYM;
								}
								else
								{
									strcpy(aText[currText], scr_text);
									scr_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									DBP0(("[lex] IDENT\n"));
									return IDENT;
								}
							}
	break;
	case 37:
	{ BEGIN QUOTE; }
	break;
	case 38:
	{ BEGIN 0; }
	break;
	case 39:
	{
								strcpy(aText[currText], scr_text);
								scr_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}
	break;
	case 40:
	;
	break;
	case 41:
	{ inComment=TRUE; BEGIN COMMENT; }
	break;
	case 42:
	case 43:
	{ inComment=FALSE; BEGIN 0; }
	break;
	case 44:
	case 45:
	;
	break;
	case 46:
	{ BEGIN SLCOMMENT; }
	break;
	case 47:
	{ BEGIN 0; }
	break;
	case 48:
	;
	break;
	case 49:
	return scr_text[0];
	break;

	}
	YY_SCANNER;
	i = scr_leng;
	goto scr__again;			/* action fell though */
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct scr__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	scr__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
scr_SaveScan(FILE * fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = scr_in;
	p->oldline = scr_lineno;
	p->oldend = scr__end;
	p->oldstart = scr__start;
	p->oldlastc = scr__lastc;
	p->oldleng = scr_leng;
	(void) memcpy(p->savetext, scr_text, sizeof scr_text);
	(void) memcpy((char *) p->savestate, (char *) scr__sbuf,
		sizeof scr__sbuf);

	scr_in = fp;
	scr_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
scr_RestoreScan(YY_SAVED * p)
{
	if (p == NULL)
		return;
	scr_in = p->oldfp;
	scr_lineno = p->oldline;
	scr__end = p->oldend;
	scr__start = p->oldstart;
	scr__lastc = p->oldlastc;
	scr_leng = p->oldleng;

	(void) memcpy(scr_text, p->savetext, sizeof scr_text);
	(void) memcpy((char *) scr__sbuf, (char *) p->savestate,
		sizeof scr__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of scr_lex()
 */
void
scr__reset()
{
	YY_INIT;
	scr_lineno = 1;		/* line number */
}

/* Set the current input buffer for the lexer */
void scriptSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer in case it's been used before */
	scr__reset();
}

void scriptGetErrorData(int *pLine, char **ppText)
{
	*pLine = scr_lineno;
	*ppText = scr_text;
}

int scr_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}
