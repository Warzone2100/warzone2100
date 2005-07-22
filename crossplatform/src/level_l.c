/* d:\usr\mks-ly\mksnt\lex -p lev_ -o Level_l.c Level.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define SLCOMMENT 4
#define QUOTE 6
#define lev__endst 111
#define lev__nxtmax 692
#define YY_LA_SIZE 14

static unsigned int lev__la_act[] = {
 15, 30, 15, 30, 15, 30, 15, 30, 15, 30, 15, 30, 15, 30, 15, 30,
 15, 30, 15, 30, 16, 30, 30, 20, 30, 21, 30, 21, 30, 30, 22, 27,
 20, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 15, 15, 15, 12,
 15, 15, 15, 15, 15, 15, 13, 15, 15, 15, 15, 15, 15, 11, 15, 15,
 15, 15, 15, 9, 15, 15, 15, 15, 15, 15, 10, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 7, 15, 15, 15, 15, 6, 15, 15, 15, 15, 5,
 15, 15, 15, 4, 15, 15, 15, 3, 15, 15, 15, 8, 15, 15, 15, 2,
 15, 15, 15, 15, 15, 15, 1, 15, 15, 15, 15, 0, 15, 25, 25, 26,
 23, 24, 29, 28, 29, 19, 17, 18, 19, 19, 0
};

static unsigned char lev__look[] = {
 0
};

static int lev__final[] = {
 0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23, 25, 27,
 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
 45, 46, 47, 49, 50, 51, 52, 53, 54, 56, 57, 58, 59, 60, 61, 63,
 64, 65, 66, 67, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79, 80, 81,
 82, 83, 84, 85, 87, 88, 89, 90, 92, 93, 94, 95, 97, 98, 99, 101,
 102, 103, 105, 106, 107, 109, 110, 111, 113, 114, 115, 116, 117, 118, 120, 121,
 122, 123, 125, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 137, 138

};
#ifndef lev__state_t
#define lev__state_t unsigned char
#endif

static lev__state_t lev__begin[] = {
 0, 0, 98, 98, 104, 104, 107, 107, 0
};

static lev__state_t lev__next[] = {
 17, 17, 17, 17, 17, 17, 17, 17, 17, 14, 15, 17, 17, 14, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 14, 17, 11, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 12, 17, 16,
 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 17, 17, 17, 17, 17, 17,
 17, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 17, 17, 17, 17, 17,
 17, 10, 8, 6, 4, 7, 10, 5, 10, 10, 10, 10, 1, 9, 10, 10,
 2, 10, 10, 10, 3, 10, 10, 10, 10, 10, 10, 17, 17, 17, 17, 17,
 18, 22, 23, 24, 25, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
 21, 28, 29, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 30, 31, 32,
 33, 34, 35, 36, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 37, 38,
 39, 40, 21, 41, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 27, 42,
 43, 44, 45, 46, 47, 48, 26, 49, 50, 51, 52, 53, 54, 55, 56, 57,
 58, 59, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 60,
 75, 76, 61, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 101, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
 100, 100, 100, 100, 100, 100, 100, 100, 102, 103, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 105, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 106,
 106, 106, 106, 106, 106, 106, 106, 106, 106, 106, 111, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 109, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 108, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110,
 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 111, 111, 111,
 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111, 111,
 111, 111, 111, 111, 111, 0
};

static lev__state_t lev__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 16, 9, 22, 23, 24, 16, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
 10, 27, 28, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 29, 30, 26,
 32, 33, 34, 35, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 36, 37,
 38, 39, 10, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 25, 41,
 42, 43, 44, 45, 7, 47, 25, 48, 49, 50, 51, 52, 53, 54, 55, 56,
 6, 58, 59, 62, 63, 64, 65, 66, 61, 68, 69, 70, 60, 72, 73, 59,
 74, 5, 59, 76, 77, 4, 79, 80, 81, 82, 83, 3, 85, 86, 2, 88,
 89, 90, 91, 92, 1, 94, 95, 96, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98, 98,
 98, 98, 98, 98, 98, 98, 98, 98, 99, 102, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104,
 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 106, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107,
 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 107, 110, 109, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, 110, 109, 0
};

static lev__state_t lev__default[] = {
 111, 10, 10, 10, 10, 10, 10, 10, 10, 10, 111, 111, 13, 111, 111, 111,
 111, 111, 111, 111, 13, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 111, 111, 111, 111, 111, 111, 111, 111, 104, 111, 111, 107, 107, 0

};

static int lev__base[] = {
 0, 175, 162, 146, 164, 160, 143, 108, 94, 24, 99, 693, 693, 86, 693, 693,
 86, 693, 693, 693, 693, 693, 15, 16, 37, 123, 58, 37, 45, 60, 44, 693,
 59, 49, 67, 55, 85, 82, 94, 82, 693, 107, 105, 124, 125, 117, 693, 117,
 134, 122, 133, 139, 127, 131, 128, 140, 128, 693, 132, 143, 155, 132, 139, 147,
 135, 143, 146, 693, 152, 136, 135, 693, 148, 151, 146, 693, 150, 159, 693, 146,
 166, 149, 164, 150, 693, 156, 168, 693, 174, 151, 172, 160, 160, 693, 159, 177,
 171, 693, 280, 361, 693, 693, 399, 693, 410, 693, 528, 539, 693, 658, 657, 693

};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/lev_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
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
 * graceful exits from lev_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling lev_lex must supply this!
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
 * You can redefine lev_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define lev_getc()	yt_getc()
#else
#define	lev_getc()	getc(lev_in) 	/* lev_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(lev_out, "lev_lex: %s\n", msg); lev_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "lev_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(lev_text, lev_out)

#define	output(c)	putc((c), lev_out) /* lev_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		lev__start =
#define	REJECT		goto lev__reject
#define	NLSTATE		(lev__lastc = YYNEWLINE)
#define	YY_INIT \
	(lev__start = lev_leng = lev__end = 0, lev__lastc = YYNEWLINE)
#define	lev_more()	goto lev__more
#define	lev_less(n)	if ((n) < 0 || (n) > lev__end) ; \
			else { YY_SCANNER; lev_leng = (n); YY_USER; }

YY_DECL	void	lev__reset YY_ARGS((void));

/* functions defined in libl.lib */
extern	int	lev_wrap	YY_ARGS((void));
extern	void	lev_error	YY_ARGS((char *fmt, ...));
extern	void	lev_comment	YY_ARGS((char *term));
extern	int	lev_mapch	YY_ARGS((int delim, int escape));

/*
 * Level.l
 *
 * lexer for loading level description files
 *
 */

#include <stdio.h>

#include "frame.h"

#include "levels.h"
#include "levelint.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* Store for any string values */
static STRING aText[YYLMAX];

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef lev_getc
#define lev_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), lev_text[] contains the token.
 * If %pointer is used, lev_text is a pointer to lev__tbuf[].
 */
YY_DECL char	lev_text[YYLMAX+1];

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
#define	YYLEX lev_lex			/* name of lex scanner */
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
YY_DECL	FILE   *lev_in = stdin;
YY_DECL	FILE   *lev_out = stdout;
#else
YY_DECL	FILE   *lev_in = (FILE *)0;
YY_DECL	FILE   *lev_out = (FILE *)0;
#endif
YY_DECL	int	lev_lineno = 1;		/* line number */

/* lev__sbuf[0:lev_leng-1] contains the states corresponding to lev_text.
 * lev_text[0:lev_leng-1] contains the current token.
 * lev_text[lev_leng:lev__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * lev__save contains lev_text[lev_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	lev__state_t lev__sbuf [YYLMAX+1];	/* state buffer */
static	int	lev__end = 0;		/* end of pushback */
static	int	lev__start = 0;		/* start state */
static	int	lev__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	lev_leng = 0;		/* lev_text token length */
#ifdef YYEXIT
static	int lev_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char lev__save;	/* saved lev_text[lev_leng] */

#define	YY_USER	{ /* set up lev_text for user */ \
		lev__save = lev_text[lev_leng]; \
		lev_text[lev_leng] = 0; \
	}
#define	YY_SCANNER { /* set up lev_text for scanner */ \
		lev_text[lev_leng] = lev__save; \
	}

#else		/* not-so efficient push-back for lev_text mungers */

static	char lev__save [YYLMAX];
static	char *lev__push = lev__save+YYLMAX;

#define	YY_USER { \
		size_t n = lev__end - lev_leng; \
		lev__push = lev__save+YYLMAX - n; \
		if (n > 0) \
			memmove(lev__push, lev_text+lev_leng, n); \
		lev_text[lev_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = lev__save+YYLMAX - lev__push; \
		if (n > 0) \
			memmove(lev_text+lev_leng, lev__push, n); \
		lev__end = lev_leng + n; \
	}

#endif

#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make lev_lex()
 * behave in a windows environment, create a new
 * lev_lex() which will call the original lev_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually lev_lex(void)).
 * NOTE: you should invoke lev__init() if you are calling lev_lex()
 * with new input; otherwise old lookaside will get in your way
 * and lev_lex() will die horribly.
 */
static int win_lev_lex();			/* prototype for windows lev_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_lev__la_act;	/* remember previous pointer values */
	short far *old_lev__final;
	lev__state_t far *old_lev__begin;
	lev__state_t far *old_lev__next;
	lev__state_t far *old_lev__check;
	lev__state_t far *old_lev__default;
	short far *old_lev__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_lev_LEX", "lev_LEXTBL"));
	
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

	old_lev__la_act = lev__la_act;
	old_lev__final = lev__final;
	old_lev__begin = lev__begin;
	old_lev__next = lev__next;
	old_lev__check = lev__check;
	old_lev__default = lev__default;
	old_lev__base = lev__base;

	lev__la_act = (unsigned short far *)LockResource (hRes_table);
	lev__final = (short far *)(lev__la_act + Sizeof_lev__la_act);
	lev__begin = (lev__state_t far *)(lev__final + Sizeof_lev__final);
	lev__next = (lev__state_t far *)(lev__begin + Sizeof_lev__begin);
	lev__check = (lev__state_t far *)(lev__next + Sizeof_lev__next);
	lev__default = (lev__state_t far *)(lev__check + Sizeof_lev__check);
	lev__base = (lev__state_t far *)(lev__default + Sizeof_lev__default);


	/*
	 * call the standard lev_lex() code
	 */

	wReturnValue = win_lev_lex();

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

	lev__la_act = old_lev__la_act;
	lev__final = old_lev__final;
	lev__begin = old_lev__begin;
	lev__next = old_lev__next;
	lev__check = old_lev__check;
	lev__default = old_lev__default;
	lev__base = old_lev__base;

	return (wReturnValue);
}	/* end function */

static int win_lev_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually lev_lex(void)).
 * NOTE: you should invoke lev__init() if you are calling lev_lex()
 * with new input; otherwise old lookaside will get in your way
 * and lev_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, lev_base;
	unsigned	lev_st;	/* state */
	int lev_fmin, lev_fmax;	/* lev__la_act indices of final states */
	int lev_oldi, lev_oleng;	/* base i, lev_leng before look-ahead */
	int lev_eof;		/* 1 if eof has already been read */

#if !YY_STATIC_STDIO
	if (lev_in == (FILE *)0)
		lev_in = stdin;
	if (lev_out == (FILE *)0)
		lev_out = stdout;
#endif

#ifdef YYEXIT
	lev_LexFatal = 0;
#endif /* YYEXIT */

	lev_eof = 0;
	i = lev_leng;
	YY_SCANNER;

  lev__again:
	lev_leng = i;
	/* determine previous char. */
	if (i > 0)
		lev__lastc = lev_text[i-1];
	/* scan previously accepted token adjusting lev_lineno */
	while (i > 0)
		if (lev_text[--i] == YYNEWLINE)
			lev_lineno++;
	/* adjust pushback */
	lev__end -= lev_leng;
	if (lev__end > 0)
		memmove(lev_text, lev_text+lev_leng, (size_t) lev__end);
	i = 0;

	lev_oldi = i;

	/* run the state machine until it jams */
	lev_st = lev__begin[lev__start + ((lev__lastc == YYNEWLINE) ? 1 : 0)];
	lev__sbuf[i] = (lev__state_t) lev_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), lev_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (lev_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < lev__end)
			c = lev_text[i];		/* get pushback char */
		else if (!lev_eof && (c = lev_getc()) != EOF) {
			lev__end = i+1;
			lev_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == lev_oldi) {	/* no token */
				lev_eof = 0;
				if (lev_wrap())
					return 0;
				else
					goto lev__again;
			} else {
				lev_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((lev_base = lev__base[lev_st]+(unsigned char)c) > lev__nxtmax
		    || lev__check[lev_base] != (lev__state_t) lev_st) {
			if (lev_st == lev__endst)
				goto lev__jammed;
			lev_st = lev__default[lev_st];
		}
		lev_st = lev__next[lev_base];
	  lev__jammed: ;
	  lev__sbuf[++i] = (lev__state_t) lev_st;
	} while (!(lev_st == lev__endst || (YY_INTERACTIVE && lev__base[lev_st] > lev__nxtmax && lev__default[lev_st] == lev__endst)));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), lev_st, i);
	if (lev_st != lev__endst)
		++i;

	/* search backward for a final state */
	while (--i > lev_oldi) {
		lev_st = lev__sbuf[i];
		if ((lev_fmin = lev__final[lev_st]) < (lev_fmax = lev__final[lev_st+1]))
			goto lev__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = lev_oldi + 1;
	output(lev_text[lev_oldi]);
	goto lev__again;

  lev__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), lev_st, i);
	lev_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(lev__la_act[lev_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = lev__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < lev_oldi) {	/* no / */
				i = lev_oleng;
				break;
			}
			lev_st = lev__sbuf[i];
			if (bv[(unsigned)lev_st/8] & bits[(unsigned)lev_st%8])
				break;
		}
	}

	/* perform action */
	lev_leng = i;
	YY_USER;
	switch (lev__la_act[lev_fmin] & 0777) {
	case 0:
	return LTK_LEVEL;
	break;
	case 1:
	return LTK_PLAYERS;
	break;
	case 2:
	return LTK_TYPE;
	break;
	case 3:
	return LTK_DATA;
	break;
	case 4:
	return LTK_GAME;
	break;
	case 5:
	return LTK_CAMPAIGN;
	break;
	case 6:
	return LTK_CAMSTART;
	break;
	case 7:
	return LTK_CAMCHANGE;
	break;
	case 8:
	return LTK_DATASET;
	break;
	case 9:
	return LTK_EXPAND;
	break;
	case 10:
	return LTK_EXPAND_LIMBO;
	break;
	case 11:
	return LTK_BETWEEN;
	break;
	case 12:
	return LTK_MKEEP;
	break;
	case 13:
	return LTK_MKEEP_LIMBO;
	break;
	case 14:
	return LTK_MCLEAR;
	break;
	case 15:
	{
								strcpy(aText, lev_text);
								pLevToken = aText;
								return LTK_IDENT;
							}
	break;
	case 16:
	{ BEGIN QUOTE; }
	break;
	case 17:
	{ BEGIN 0; }
	break;
	case 18:
	{ levError("Unexpected end of line in string"); }
	break;
	case 19:
	{
								strcpy(aText, lev_text);
								pLevToken = aText;
								return LTK_STRING;
							}
	break;
	case 20:
	{ levVal = atol(lev_text); return LTK_INTEGER; }
	break;
	case 21:
	;
	break;
	case 22:
	{ inComment=TRUE; BEGIN COMMENT; }
	break;
	case 23:
	case 24:
	{ inComment=FALSE; BEGIN 0; }
	break;
	case 25:
	case 26:
	;
	break;
	case 27:
	{ BEGIN SLCOMMENT; }
	break;
	case 28:
	{ BEGIN 0; }
	break;
	case 29:
	;
	break;
	case 30:
	return lev_text[0];
	break;

	}
	YY_SCANNER;
	i = lev_leng;
	goto lev__again;			/* action fell though */
}

/*
 * Safely switch input stream underneath LEX
 */
typedef struct lev__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	lev__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
lev_SaveScan(FILE * fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = lev_in;
	p->oldline = lev_lineno;
	p->oldend = lev__end;
	p->oldstart = lev__start;
	p->oldlastc = lev__lastc;
	p->oldleng = lev_leng;
	(void) memcpy(p->savetext, lev_text, sizeof lev_text);
	(void) memcpy((char *) p->savestate, (char *) lev__sbuf,
		sizeof lev__sbuf);

	lev_in = fp;
	lev_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
lev_RestoreScan(YY_SAVED * p)
{
	if (p == NULL)
		return;
	lev_in = p->oldfp;
	lev_lineno = p->oldline;
	lev__end = p->oldend;
	lev__start = p->oldstart;
	lev__lastc = p->oldlastc;
	lev_leng = p->oldleng;

	(void) memcpy(lev_text, p->savetext, sizeof lev_text);
	(void) memcpy((char *) lev__sbuf, (char *) p->savestate,
		sizeof lev__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of lev_lex()
 */
void
lev__reset()
{
	YY_INIT;
	lev_lineno = 1;		/* line number */
}

/* Set the current input buffer for the lexer */
void levSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	lev__reset();
}

void levGetErrorData(int *pLine, char **ppText)
{
	*pLine = lev_lineno;
	*ppText = lev_text;
}

int lev_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}

