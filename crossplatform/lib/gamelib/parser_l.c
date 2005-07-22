/* d:\mks-ly\mksnt\lex -p audp_ -o parser_l.c .\parser.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define QUOTE 4
#define audp__endst 85
#define audp__nxtmax 588
#define YY_LA_SIZE 11

static unsigned int audp__la_act[] = {
 11, 18, 11, 18, 11, 18, 11, 18, 18, 9, 18, 11, 18, 11, 18, 12,
 18, 12, 18, 18, 13, 11, 11, 10, 10, 11, 9, 11, 11, 11, 11, 11,
 11, 11, 11, 11, 8, 11, 11, 11, 11, 11, 11, 11, 7, 11, 11, 11,
 11, 11, 6, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 5,
 11, 11, 11, 11, 11, 3, 11, 11, 11, 2, 11, 11, 11, 11, 11, 11,
 11, 4, 11, 11, 11, 1, 11, 11, 11, 11, 11, 11, 0, 11, 16, 16,
 17, 14, 15, 0
};

static unsigned char audp__look[] = {
 0
};

static int audp__final[] = {
 0, 0, 2, 4, 6, 8, 9, 11, 13, 15, 17, 18, 19, 20, 21, 22,
 22, 23, 24, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 38, 39,
 40, 41, 42, 43, 44, 46, 47, 48, 49, 50, 52, 53, 54, 55, 56, 57,
 58, 59, 60, 61, 62, 63, 65, 66, 67, 68, 69, 71, 72, 73, 75, 76,
 77, 78, 79, 80, 81, 83, 84, 85, 87, 88, 89, 90, 91, 92, 94, 94,
 95, 96, 97, 98, 99, 99
};
#ifndef audp__state_t
#define audp__state_t unsigned char
#endif

static audp__state_t audp__begin[] = {
 0, 0, 78, 78, 84, 84, 0
};

static audp__state_t audp__next[] = {
 12, 12, 12, 12, 12, 12, 12, 12, 12, 9, 10, 12, 12, 9, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 9, 12, 7, 8, 12, 12, 12, 12, 12, 12, 12, 12, 12, 5, 8, 11,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 12, 12, 12, 12, 12, 12,
 12, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12, 12, 12,
 12, 3, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 2, 8, 8, 1,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12, 12, 12,
 13, 14, 14, 20, 21, 22, 25, 26, 27, 28, 29, 30, 14, 14, 31, 14,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 33, 34, 35, 32, 36, 23, 37,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 24, 38, 39, 40, 14, 41,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 17, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 18, 16, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 16, 16, 15, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 15, 15, 15, 16, 15, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 17, 15, 43, 44, 45, 48,
 49, 50, 51, 42, 52, 15, 15, 46, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 53, 54, 55, 56, 57, 58, 59, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 47, 60, 61, 62, 15, 63, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 64, 65, 66,
 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 81, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 82, 83, 0
};

static audp__state_t audp__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 11, 8, 8, 4, 20, 21, 24, 25, 26, 27, 28, 23, 8, 8, 30, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 32, 33, 34, 30, 35, 22, 31,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 22, 37, 38, 39, 8, 40,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 15, 15, 3, 43, 44, 47,
 48, 49, 50, 3, 51, 15, 15, 45, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 52, 46, 54, 55, 56, 57, 42, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 45, 59, 60, 61, 15, 62, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 63, 64, 65,
 66, 67, 2, 69, 70, 1, 72, 73, 74, 75, 76, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78,
 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 78, 79, 82, 0
};

static audp__state_t audp__default[] = {
 85, 8, 8, 8, 8, 6, 85, 85, 85, 85, 85, 85, 85, 85, 8, 7,
 7, 85, 8, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 85, 85,
 85, 85, 85, 85, 85, 0
};

static int audp__base[] = {
 0, 343, 339, 238, 53, 589, 387, 218, 95, 589, 589, 86, 589, 589, 589, 312,
 589, 589, 589, 589, 59, 56, 107, 71, 68, 61, 67, 70, 54, 589, 72, 77,
 71, 89, 77, 74, 589, 122, 111, 120, 108, 589, 276, 244, 241, 308, 271, 242,
 241, 253, 237, 248, 269, 589, 270, 268, 266, 274, 589, 299, 294, 311, 299, 334,
 346, 330, 340, 348, 589, 340, 340, 589, 353, 340, 352, 346, 342, 589, 459, 540,
 589, 589, 578, 589, 589, 589
};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/audp_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
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
 * graceful exits from audp_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling audp_lex must supply this!
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
 * You can redefine audp_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define audp_getc()	yt_getc()
#else
#define	audp_getc()	getc(audp_in) 	/* audp_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(audp_out, "audp_lex: %s\n", msg); audp_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "audp_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(audp_text, audp_out)

#define	output(c)	putc((c), audp_out) /* audp_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		audp__start =
#define	REJECT		goto audp__reject
#define	NLSTATE		(audp__lastc = YYNEWLINE)
#define	YY_INIT \
	(audp__start = audp_leng = audp__end = 0, audp__lastc = YYNEWLINE)
#define	audp_more()	goto audp__more
#define	audp_less(n)	if ((n) < 0 || (n) > audp__end) ; \
			else { YY_SCANNER; audp_leng = (n); YY_USER; }

YY_DECL	void	audp__reset YY_ARGS((void));

/* functions defined in libl.lib */
extern	int	audp_wrap	YY_ARGS((void));
extern	void	audp_error	YY_ARGS((char *fmt, ...));
extern	void	audp_comment	YY_ARGS((char *term));
extern	int	audp_mapch	YY_ARGS((int delim, int escape));

#include <stdio.h>

/* include framework */
#include "frame.h"

#include "parser.h"

/* Get the Yacc definitions */
#include "parser_y.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* extern funcs */
extern int	audp_parse YY_ARGS( (void) );

/* global variables */
static	BOOL	g_bParsingSubFile;
static	FILE	*g_fpOld;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef audp_getc
#define audp_getc() ( pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF )

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), audp_text[] contains the token.
 * If %pointer is used, audp_text is a pointer to audp__tbuf[].
 */
YY_DECL char	audp_text[YYLMAX+1];

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
#define	YYLEX audp_lex			/* name of lex scanner */
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
YY_DECL	FILE   *audp_in = stdin;
YY_DECL	FILE   *audp_out = stdout;
#else
YY_DECL	FILE   *audp_in = (FILE *)0;
YY_DECL	FILE   *audp_out = (FILE *)0;
#endif
YY_DECL	int	audp_lineno = 1;		/* line number */

/* audp__sbuf[0:audp_leng-1] contains the states corresponding to audp_text.
 * audp_text[0:audp_leng-1] contains the current token.
 * audp_text[audp_leng:audp__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * audp__save contains audp_text[audp_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	audp__state_t audp__sbuf [YYLMAX+1];	/* state buffer */
static	int	audp__end = 0;		/* end of pushback */
static	int	audp__start = 0;		/* start state */
static	int	audp__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	audp_leng = 0;		/* audp_text token length */
#ifdef YYEXIT
static	int audp_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char audp__save;	/* saved audp_text[audp_leng] */

#define	YY_USER	{ /* set up audp_text for user */ \
		audp__save = audp_text[audp_leng]; \
		audp_text[audp_leng] = 0; \
	}
#define	YY_SCANNER { /* set up audp_text for scanner */ \
		audp_text[audp_leng] = audp__save; \
	}

#else		/* not-so efficient push-back for audp_text mungers */

static	char audp__save [YYLMAX];
static	char *audp__push = audp__save+YYLMAX;

#define	YY_USER { \
		size_t n = audp__end - audp_leng; \
		audp__push = audp__save+YYLMAX - n; \
		if (n > 0) \
			memmove(audp__push, audp_text+audp_leng, n); \
		audp_text[audp_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = audp__save+YYLMAX - audp__push; \
		if (n > 0) \
			memmove(audp_text+audp_leng, audp__push, n); \
		audp__end = audp_leng + n; \
	}

#endif

#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make audp_lex()
 * behave in a windows environment, create a new
 * audp_lex() which will call the original audp_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually audp_lex(void)).
 * NOTE: you should invoke audp__init() if you are calling audp_lex()
 * with new input; otherwise old lookaside will get in your way
 * and audp_lex() will die horribly.
 */
static int win_audp_lex();			/* prototype for windows audp_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_audp__la_act;	/* remember previous pointer values */
	short far *old_audp__final;
	audp__state_t far *old_audp__begin;
	audp__state_t far *old_audp__next;
	audp__state_t far *old_audp__check;
	audp__state_t far *old_audp__default;
	short far *old_audp__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_audp_LEX", "audp_LEXTBL"));
	
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

	old_audp__la_act = audp__la_act;
	old_audp__final = audp__final;
	old_audp__begin = audp__begin;
	old_audp__next = audp__next;
	old_audp__check = audp__check;
	old_audp__default = audp__default;
	old_audp__base = audp__base;

	audp__la_act = (unsigned short far *)LockResource (hRes_table);
	audp__final = (short far *)(audp__la_act + Sizeof_audp__la_act);
	audp__begin = (audp__state_t far *)(audp__final + Sizeof_audp__final);
	audp__next = (audp__state_t far *)(audp__begin + Sizeof_audp__begin);
	audp__check = (audp__state_t far *)(audp__next + Sizeof_audp__next);
	audp__default = (audp__state_t far *)(audp__check + Sizeof_audp__check);
	audp__base = (audp__state_t far *)(audp__default + Sizeof_audp__default);


	/*
	 * call the standard audp_lex() code
	 */

	wReturnValue = win_audp_lex();

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

	audp__la_act = old_audp__la_act;
	audp__final = old_audp__final;
	audp__begin = old_audp__begin;
	audp__next = old_audp__next;
	audp__check = old_audp__check;
	audp__default = old_audp__default;
	audp__base = old_audp__base;

	return (wReturnValue);
}	/* end function */

static int win_audp_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually audp_lex(void)).
 * NOTE: you should invoke audp__init() if you are calling audp_lex()
 * with new input; otherwise old lookaside will get in your way
 * and audp_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, audp_base;
	unsigned	audp_st;	/* state */
	int audp_fmin, audp_fmax;	/* audp__la_act indices of final states */
	int audp_oldi, audp_oleng;	/* base i, audp_leng before look-ahead */
	int audp_eof;		/* 1 if eof has already been read */

#if !YY_STATIC_STDIO
	if (audp_in == (FILE *)0)
		audp_in = stdin;
	if (audp_out == (FILE *)0)
		audp_out = stdout;
#endif

#ifdef YYEXIT
	audp_LexFatal = 0;
#endif /* YYEXIT */

	audp_eof = 0;
	i = audp_leng;
	YY_SCANNER;

  audp__again:
	audp_leng = i;
	/* determine previous char. */
	if (i > 0)
		audp__lastc = audp_text[i-1];
	/* scan previously accepted token adjusting audp_lineno */
	while (i > 0)
		if (audp_text[--i] == YYNEWLINE)
			audp_lineno++;
	/* adjust pushback */
	audp__end -= audp_leng;
	if (audp__end > 0)
		memmove(audp_text, audp_text+audp_leng, (size_t) audp__end);
	i = 0;

	audp_oldi = i;

	/* run the state machine until it jams */
	audp_st = audp__begin[audp__start + ((audp__lastc == YYNEWLINE) ? 1 : 0)];
	audp__sbuf[i] = (audp__state_t) audp_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), audp_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (audp_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < audp__end)
			c = audp_text[i];		/* get pushback char */
		else if (!audp_eof && (c = audp_getc()) != EOF) {
			audp__end = i+1;
			audp_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == audp_oldi) {	/* no token */
				audp_eof = 0;
				if (audp_wrap())
					return 0;
				else
					goto audp__again;
			} else {
				audp_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((audp_base = audp__base[audp_st]+(unsigned char)c) > audp__nxtmax
		    || audp__check[audp_base] != (audp__state_t) audp_st) {
			if (audp_st == audp__endst)
				goto audp__jammed;
			audp_st = audp__default[audp_st];
		}
		audp_st = audp__next[audp_base];
	  audp__jammed: ;
	  audp__sbuf[++i] = (audp__state_t) audp_st;
	} while (!(audp_st == audp__endst || (YY_INTERACTIVE && audp__base[audp_st] > audp__nxtmax && audp__default[audp_st] == audp__endst)));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), audp_st, i);
	if (audp_st != audp__endst)
		++i;

	/* search backward for a final state */
	while (--i > audp_oldi) {
		audp_st = audp__sbuf[i];
		if ((audp_fmin = audp__final[audp_st]) < (audp_fmax = audp__final[audp_st+1]))
			goto audp__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = audp_oldi + 1;
	output(audp_text[audp_oldi]);
	goto audp__again;

  audp__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), audp_st, i);
	audp_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(audp__la_act[audp_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = audp__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < audp_oldi) {	/* no / */
				i = audp_oleng;
				break;
			}
			audp_st = audp__sbuf[i];
			if (bv[(unsigned)audp_st/8] & bits[(unsigned)audp_st%8])
				break;
		}
	}

	/* perform action */
	audp_leng = i;
	YY_USER;
	switch (audp__la_act[audp_fmin] & 0777) {
	case 0:
	{	return ONESHOT;			}
	break;
	case 1:
	{	return LOOP;			}
	break;
	case 2:
	{	return AUDIO;			}
	break;
	case 3:
	{	return ANIM3DFILE;		}
	break;
	case 4:
	{	return AUDIO_MODULE;	}
	break;
	case 5:
	{	return ANIM_MODULE;		}
	break;
	case 6:
	{	return ANIM3DFRAMES;	}
	break;
	case 7:
	{	return ANIM3DTRANS;		}
	break;
	case 8:
	{	return ANIMOBJECT;		}
	break;
	case 9:
	{	audp_lval.ival = atoi(audp_text);
									return INTEGER;
								}
	break;
	case 10:
	{
									/* skip opening quote */
									strcpy( audp_lval.sval, audp_text+1 );

									/* check for unterminated string */
									if ( audp_text[audp_leng-1] != '"' )
									{
										sprintf( audp_lval.sval, "Unterminated string %s\n", audp_lval.sval );
										audp_error( audp_lval.sval );
										return (1);
									}

									/* set final quote in string to blank */
									audp_lval.sval[audp_leng-2] = (char) NULL;

									return QTEXT;
								}
	break;
	case 11:
	{	strcpy( audp_lval.sval, audp_text );
									return TEXT;
								}
	break;
	case 12:
	;
	break;
	case 13:
	{ BEGIN COMMENT; }
	break;
	case 14:
	case 15:
	{ BEGIN 0; }
	break;
	case 16:
	case 17:
	;
	break;
	case 18:
	return audp_text[0];
	break;

	}
	YY_SCANNER;
	i = audp_leng;
	goto audp__again;			/* action fell though */
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct audp__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	audp__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
audp_SaveScan(FILE * fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = audp_in;
	p->oldline = audp_lineno;
	p->oldend = audp__end;
	p->oldstart = audp__start;
	p->oldlastc = audp__lastc;
	p->oldleng = audp_leng;
	(void) memcpy(p->savetext, audp_text, sizeof audp_text);
	(void) memcpy((char *) p->savestate, (char *) audp__sbuf,
		sizeof audp__sbuf);

	audp_in = fp;
	audp_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
audp_RestoreScan(YY_SAVED * p)
{
	if (p == NULL)
		return;
	audp_in = p->oldfp;
	audp_lineno = p->oldline;
	audp__end = p->oldend;
	audp__start = p->oldstart;
	audp__lastc = p->oldlastc;
	audp_leng = p->oldleng;

	(void) memcpy(audp_text, p->savetext, sizeof audp_text);
	(void) memcpy((char *) audp__sbuf, (char *) p->savestate,
		sizeof audp__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of audp_lex()
 */
void
audp__reset()
{
	YY_INIT;
	audp_lineno = 1;		/* line number */
}

/***************************************************************************/

BOOL
ParseFile( char szFileName[] )
{
	FILE	*fp;

	/* open input file */
	if ( (fp = fopen( szFileName, "rt" )) == NULL )
	{
		DBERROR( ("ParseFile: file not found\n") );
		return FALSE;
	}

	/* point input to input file */
	audp_in = fp;

	audp_parse();

	fclose( fp );

	return TRUE;
}

/***************************************************************************/

void
IncludeFile( char szFileName[] )
{
	FILE		*fpNew;

	/* open module file */
	if ( (fpNew = fopen( szFileName, "rt" )) != NULL )
	{
		/* save current file pointer and switch to new */
		g_fpOld = audp_in;
		audp_in = fpNew;

		g_bParsingSubFile = TRUE;
	}
	else
	{
		DBERROR( ("Included file %s not found\n", szFileName) );
	}
}

/***************************************************************************/

int
audp_wrap( void )
{
	if ( g_bParsingSubFile == TRUE )
	{
		/* close current file and restore old file pointer */
		fclose( audp_in );
		audp_in = g_fpOld;

		g_bParsingSubFile = FALSE;

		return 0;
	}
	else
	{
		return 1;
	}
}

/***************************************************************************/
/* Set the current input buffer for the lexer */
/***************************************************************************/

void
parserSetInputBuffer( UBYTE *pBuffer, UDWORD size )
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer in case it's been used before */
	audp__reset();
}

/***************************************************************************/

void
parseGetErrorData(int *pLine, char **ppText)
{
	*pLine  = audp_lineno;
	*ppText = audp_text;
}

/***************************************************************************/
