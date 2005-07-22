/* lex -p scrv_ -o ScriptVals_l.c ScriptVals.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define SLCOMMENT 4
#define QUOTE 6
#define scrv__endst 71
#define scrv__nxtmax 650
#define YY_LA_SIZE 9

static unsigned int scrv__la_act[] = {
 11, 26, 11, 26, 11, 26, 11, 26, 11, 26, 11, 26, 11, 26, 11, 26,
 11, 26, 11, 26, 11, 26, 26, 12, 26, 13, 26, 17, 26, 17, 26, 26,
 18, 23, 12, 11, 11, 11, 11, 10, 11, 11, 11, 11, 9, 11, 11, 11,
 8, 11, 11, 11, 7, 11, 11, 6, 11, 11, 11, 11, 11, 5, 11, 11,
 11, 11, 4, 11, 11, 11, 3, 11, 11, 11, 2, 11, 11, 1, 11, 11,
 0, 11, 21, 21, 22, 19, 20, 25, 24, 25, 16, 14, 15, 16, 0
};

static unsigned char scrv__look[] = {
 0
};

static int scrv__final[] = {
 0, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 23, 25, 27,
 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 43, 44, 46,
 47, 48, 50, 51, 52, 54, 55, 57, 58, 59, 60, 61, 63, 64, 65, 66,
 68, 69, 70, 72, 73, 74, 76, 77, 79, 80, 82, 82, 83, 84, 85, 86,
 87, 88, 89, 90, 91, 92, 93, 94
};
#ifndef scrv__state_t
#define scrv__state_t unsigned char
#endif

static scrv__state_t scrv__begin[] = {
 0, 0, 58, 58, 64, 64, 67, 67, 0
};

static scrv__state_t scrv__next[] = {
 18, 18, 18, 18, 18, 18, 18, 18, 18, 15, 16, 18, 18, 15, 18, 18,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
 15, 18, 14, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 12, 18, 17,
 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 18, 18, 18, 18, 18, 18,
 18, 11, 4, 11, 11, 11, 10, 11, 11, 2, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 8, 11, 11, 11, 11, 11, 11, 18, 18, 18, 18, 18,
 18, 11, 3, 11, 11, 11, 9, 11, 11, 1, 11, 11, 11, 11, 11, 11,
 11, 11, 6, 5, 7, 11, 11, 11, 11, 11, 11, 18, 18, 18, 18, 18,
 19, 23, 24, 25, 26, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
 22, 27, 28, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 29, 30, 31,
 32, 33, 34, 35, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 36, 37,
 38, 41, 22, 42, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 39, 43,
 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 62, 40,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 61, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 59, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
 63, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 65, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
 66, 71, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 69, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 68, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
 70, 70, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 71, 0
};

static scrv__state_t scrv__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 17, 10, 23, 24, 25, 17, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
 11, 9, 27, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 28, 29, 8,
 31, 32, 7, 34, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 35, 6,
 37, 40, 11, 41, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 5, 42,
 39, 44, 45, 46, 4, 48, 49, 3, 51, 52, 2, 54, 1, 56, 59, 5,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
 62, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
 64, 66, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67, 67,
 67, 67, 70, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 70, 0
};

static scrv__state_t scrv__default[] = {
 71, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 71, 13, 71, 71, 71,
 71, 71, 71, 71, 71, 13, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 71, 71, 71, 71, 71, 71,
 71, 71, 64, 71, 71, 71, 67, 0
};

static int scrv__base[] = {
 0, 126, 156, 120, 149, 123, 74, 48, 77, 48, 64, 99, 651, 86, 651, 651,
 651, 86, 651, 651, 651, 651, 651, 54, 48, 63, 651, 38, 42, 57, 651, 75,
 92, 651, 46, 89, 651, 82, 651, 110, 82, 81, 122, 651, 120, 114, 111, 651,
 150, 154, 651, 121, 125, 651, 151, 651, 121, 651, 240, 191, 651, 651, 358, 651,
 369, 651, 487, 498, 651, 651, 616, 651
};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/scrv_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
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
 * graceful exits from scrv_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling scrv_lex must supply this!
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
 * You can redefine scrv_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define scrv_getc()	yt_getc()
#else
#define	scrv_getc()	getc(scrv_in) 	/* scrv_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(scrv_out, "scrv_lex: %s\n", msg); scrv_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "scrv_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(scrv_text, scrv_out)

#define	output(c)	putc((c), scrv_out) /* scrv_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		scrv__start =
#define	REJECT		goto scrv__reject
#define	NLSTATE		(scrv__lastc = YYNEWLINE)
#define	YY_INIT \
	(scrv__start = scrv_leng = scrv__end = 0, scrv__lastc = YYNEWLINE)
#define	scrv_more()	goto scrv__more
#define	scrv_less(n)	if ((n) < 0 || (n) > scrv__end) ; \
			else { YY_SCANNER; scrv_leng = (n); YY_USER; }

YY_DECL	void	scrv__reset YY_ARGS((void));

/* functions defined in libl.lib */
extern	int	scrv_wrap	YY_ARGS((void));
extern	void	scrv_error	YY_ARGS((char *fmt, ...));
extern	void	scrv_comment	YY_ARGS((char *term));
extern	int	scrv_mapch	YY_ARGS((int delim, int escape));

/*
 * ScriptVals.l
 *
 * lexer for loading script variable values
 *
 */

#include <stdio.h>

#include "frame.h"
#include "script.h"
#include "scriptvals.h"

/* Get the Yacc definitions */
#include "scriptvals_y.h"

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

#undef scrv_getc
#define scrv_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), scrv_text[] contains the token.
 * If %pointer is used, scrv_text is a pointer to scrv__tbuf[].
 */
YY_DECL char	scrv_text[YYLMAX+1];

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
#define	YYLEX scrv_lex			/* name of lex scanner */
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
YY_DECL	FILE   *scrv_in = stdin;
YY_DECL	FILE   *scrv_out = stdout;
#else
YY_DECL	FILE   *scrv_in = (FILE *)0;
YY_DECL	FILE   *scrv_out = (FILE *)0;
#endif
YY_DECL	int	scrv_lineno = 1;		/* line number */

/* scrv__sbuf[0:scrv_leng-1] contains the states corresponding to scrv_text.
 * scrv_text[0:scrv_leng-1] contains the current token.
 * scrv_text[scrv_leng:scrv__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * scrv__save contains scrv_text[scrv_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	scrv__state_t scrv__sbuf [YYLMAX+1];	/* state buffer */
static	int	scrv__end = 0;		/* end of pushback */
static	int	scrv__start = 0;		/* start state */
static	int	scrv__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	scrv_leng = 0;		/* scrv_text token length */
#ifdef YYEXIT
static	int scrv_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char scrv__save;	/* saved scrv_text[scrv_leng] */

#define	YY_USER	{ /* set up scrv_text for user */ \
		scrv__save = scrv_text[scrv_leng]; \
		scrv_text[scrv_leng] = 0; \
	}
#define	YY_SCANNER { /* set up scrv_text for scanner */ \
		scrv_text[scrv_leng] = scrv__save; \
	}

#else		/* not-so efficient push-back for scrv_text mungers */

static	char scrv__save [YYLMAX];
static	char *scrv__push = scrv__save+YYLMAX;

#define	YY_USER { \
		size_t n = scrv__end - scrv_leng; \
		scrv__push = scrv__save+YYLMAX - n; \
		if (n > 0) \
			memmove(scrv__push, scrv_text+scrv_leng, n); \
		scrv_text[scrv_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = scrv__save+YYLMAX - scrv__push; \
		if (n > 0) \
			memmove(scrv_text+scrv_leng, scrv__push, n); \
		scrv__end = scrv_leng + n; \
	}

#endif

#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make scrv_lex()
 * behave in a windows environment, create a new
 * scrv_lex() which will call the original scrv_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually scrv_lex(void)).
 * NOTE: you should invoke scrv__init() if you are calling scrv_lex()
 * with new input; otherwise old lookaside will get in your way
 * and scrv_lex() will die horribly.
 */
static int win_scrv_lex();			/* prototype for windows scrv_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_scrv__la_act;	/* remember previous pointer values */
	short far *old_scrv__final;
	scrv__state_t far *old_scrv__begin;
	scrv__state_t far *old_scrv__next;
	scrv__state_t far *old_scrv__check;
	scrv__state_t far *old_scrv__default;
	short far *old_scrv__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_scrv_LEX", "scrv_LEXTBL"));
	
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

	old_scrv__la_act = scrv__la_act;
	old_scrv__final = scrv__final;
	old_scrv__begin = scrv__begin;
	old_scrv__next = scrv__next;
	old_scrv__check = scrv__check;
	old_scrv__default = scrv__default;
	old_scrv__base = scrv__base;

	scrv__la_act = (unsigned short far *)LockResource (hRes_table);
	scrv__final = (short far *)(scrv__la_act + Sizeof_scrv__la_act);
	scrv__begin = (scrv__state_t far *)(scrv__final + Sizeof_scrv__final);
	scrv__next = (scrv__state_t far *)(scrv__begin + Sizeof_scrv__begin);
	scrv__check = (scrv__state_t far *)(scrv__next + Sizeof_scrv__next);
	scrv__default = (scrv__state_t far *)(scrv__check + Sizeof_scrv__check);
	scrv__base = (scrv__state_t far *)(scrv__default + Sizeof_scrv__default);


	/*
	 * call the standard scrv_lex() code
	 */

	wReturnValue = win_scrv_lex();

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

	scrv__la_act = old_scrv__la_act;
	scrv__final = old_scrv__final;
	scrv__begin = old_scrv__begin;
	scrv__next = old_scrv__next;
	scrv__check = old_scrv__check;
	scrv__default = old_scrv__default;
	scrv__base = old_scrv__base;

	return (wReturnValue);
}	/* end function */

static int win_scrv_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually scrv_lex(void)).
 * NOTE: you should invoke scrv__init() if you are calling scrv_lex()
 * with new input; otherwise old lookaside will get in your way
 * and scrv_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, scrv_base;
	unsigned	scrv_st;	/* state */
	int scrv_fmin, scrv_fmax;	/* scrv__la_act indices of final states */
	int scrv_oldi, scrv_oleng;	/* base i, scrv_leng before look-ahead */
	int scrv_eof;		/* 1 if eof has already been read */

#if !YY_STATIC_STDIO
	if (scrv_in == (FILE *)0)
		scrv_in = stdin;
	if (scrv_out == (FILE *)0)
		scrv_out = stdout;
#endif

#ifdef YYEXIT
	scrv_LexFatal = 0;
#endif /* YYEXIT */

	scrv_eof = 0;
	i = scrv_leng;
	YY_SCANNER;

  scrv__again:
	scrv_leng = i;
	/* determine previous char. */
	if (i > 0)
		scrv__lastc = scrv_text[i-1];
	/* scan previously accepted token adjusting scrv_lineno */
	while (i > 0)
		if (scrv_text[--i] == YYNEWLINE)
			scrv_lineno++;
	/* adjust pushback */
	scrv__end -= scrv_leng;
	if (scrv__end > 0)
		memmove(scrv_text, scrv_text+scrv_leng, (size_t) scrv__end);
	i = 0;

	scrv_oldi = i;

	/* run the state machine until it jams */
	scrv_st = scrv__begin[scrv__start + ((scrv__lastc == YYNEWLINE) ? 1 : 0)];
	scrv__sbuf[i] = (scrv__state_t) scrv_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), scrv_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (scrv_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < scrv__end)
			c = scrv_text[i];		/* get pushback char */
		else if (!scrv_eof && (c = scrv_getc()) != EOF) {
			scrv__end = i+1;
			scrv_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == scrv_oldi) {	/* no token */
				scrv_eof = 0;
				if (scrv_wrap())
					return 0;
				else
					goto scrv__again;
			} else {
				scrv_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((scrv_base = scrv__base[scrv_st]+(unsigned char)c) > scrv__nxtmax
		    || scrv__check[scrv_base] != (scrv__state_t) scrv_st) {
			if (scrv_st == scrv__endst)
				goto scrv__jammed;
			scrv_st = scrv__default[scrv_st];
		}
		scrv_st = scrv__next[scrv_base];
	  scrv__jammed: ;
	  scrv__sbuf[++i] = (scrv__state_t) scrv_st;
	} while (!(scrv_st == scrv__endst || (YY_INTERACTIVE && scrv__base[scrv_st] > scrv__nxtmax && scrv__default[scrv_st] == scrv__endst)));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), scrv_st, i);
	if (scrv_st != scrv__endst)
		++i;

	/* search backward for a final state */
	while (--i > scrv_oldi) {
		scrv_st = scrv__sbuf[i];
		if ((scrv_fmin = scrv__final[scrv_st]) < (scrv_fmax = scrv__final[scrv_st+1]))
			goto scrv__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = scrv_oldi + 1;
	output(scrv_text[scrv_oldi]);
	goto scrv__again;

  scrv__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), scrv_st, i);
	scrv_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(scrv__la_act[scrv_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = scrv__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < scrv_oldi) {	/* no / */
				i = scrv_oleng;
				break;
			}
			scrv_st = scrv__sbuf[i];
			if (bv[(unsigned)scrv_st/8] & bits[(unsigned)scrv_st%8])
				break;
		}
	}

	/* perform action */
	scrv_leng = i;
	YY_USER;
	switch (scrv__la_act[scrv_fmin] & 0777) {
	case 0:
	{ scrv_lval.tval = VAL_INT; return TYPE; }
	break;
	case 1:
	{ scrv_lval.tval = VAL_INT; return TYPE; }
	break;
	case 2:
	{ scrv_lval.tval = VAL_BOOL; return TYPE; }
	break;
	case 3:
	{ scrv_lval.tval = VAL_BOOL; return TYPE; }
	break;
	case 4:
	return SCRIPT;
	break;
	case 5:
	return STORE;
	break;
	case 6:
	return RUN;
	break;
	case 7:
	{ scrv_lval.bval = TRUE;	 return BOOLEAN; }
	break;
	case 8:
	{ scrv_lval.bval = TRUE;	 return BOOLEAN; }
	break;
	case 9:
	{ scrv_lval.bval = FALSE;	 return BOOLEAN; }
	break;
	case 10:
	{ scrv_lval.bval = FALSE;	 return BOOLEAN; }
	break;
	case 11:
	{
								INTERP_TYPE type;
								UDWORD		index;

								/* See if this is a variable id or a type */
								if (scrvLookUpType(scrv_text, &type))
								{
									scrv_lval.tval = type;
									return TYPE;
								}
								else if (scrvLookUpVar(scrv_text, &index))
								{
									scrv_lval.vindex = index;
									return VAR;
								}
								else if (scrvLookUpArray(scrv_text, &index))
								{
									scrv_lval.vindex = index;
									return ARRAY;
								}
								else
								{
									strcpy(aText[currText], scrv_text);
									scrv_lval.sval = aText[currText];
									currText = (currText + 1) % TEXT_BUFFERS;
									return IDENT;
								}
							}
	break;
	case 12:
	{ scrv_lval.ival = atol(scrv_text); return INTEGER; }
	break;
	case 13:
	{ BEGIN QUOTE; }
	break;
	case 14:
	{ BEGIN 0; }
	break;
	case 15:
	{ scrv_error("Unexpected end of line in string"); }
	break;
	case 16:
	{
								strcpy(aText[currText], scrv_text);
								scrv_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}
	break;
	case 17:
	;
	break;
	case 18:
	{ inComment=TRUE; BEGIN COMMENT; }
	break;
	case 19:
	case 20:
	{ inComment=FALSE; BEGIN 0; }
	break;
	case 21:
	case 22:
	;
	break;
	case 23:
	{ BEGIN SLCOMMENT; }
	break;
	case 24:
	{ BEGIN 0; }
	break;
	case 25:
	;
	break;
	case 26:
	return scrv_text[0];
	break;

	}
	YY_SCANNER;
	i = scrv_leng;
	goto scrv__again;			/* action fell though */
}

/*
 * Safely switch input stream underneath LEX
 */
typedef struct scrv__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	scrv__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
scrv_SaveScan(FILE * fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = scrv_in;
	p->oldline = scrv_lineno;
	p->oldend = scrv__end;
	p->oldstart = scrv__start;
	p->oldlastc = scrv__lastc;
	p->oldleng = scrv_leng;
	(void) memcpy(p->savetext, scrv_text, sizeof scrv_text);
	(void) memcpy((char *) p->savestate, (char *) scrv__sbuf,
		sizeof scrv__sbuf);

	scrv_in = fp;
	scrv_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
scrv_RestoreScan(YY_SAVED * p)
{
	if (p == NULL)
		return;
	scrv_in = p->oldfp;
	scrv_lineno = p->oldline;
	scrv__end = p->oldend;
	scrv__start = p->oldstart;
	scrv__lastc = p->oldlastc;
	scrv_leng = p->oldleng;

	(void) memcpy(scrv_text, p->savetext, sizeof scrv_text);
	(void) memcpy((char *) scrv__sbuf, (char *) p->savestate,
		sizeof scrv__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of scrv_lex()
 */
void
scrv__reset()
{
	YY_INIT;
	scrv_lineno = 1;		/* line number */
}

/* Set the current input buffer for the lexer */
void scrvSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	scrv__reset();
}

void scrvGetErrorData(int *pLine, char **ppText)
{
	*pLine = scrv_lineno;
	*ppText = scrv_text;
}

int scrv_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}

