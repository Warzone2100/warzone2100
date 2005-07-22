/* lex -p res_ -o resource_l.c Resource.l */
#define YYNEWLINE 10
#define INITIAL 0
#define COMMENT 2
#define QUOTE 4
#define SLCOMMENT 6
#define res__endst 37
#define res__nxtmax 617
#define YY_LA_SIZE 5

static unsigned int res__la_act[] = {
 2, 17, 2, 17, 2, 17, 3, 17, 8, 17, 8, 17, 17, 9, 14, 7,
 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 12, 12,
 13, 10, 11, 6, 4, 5, 6, 16, 15, 16, 0
};

static unsigned char res__look[] = {
 0
};

static int res__final[] = {
 0, 0, 2, 4, 6, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 21, 22, 23, 24, 25, 26, 27, 28, 30, 30, 31, 32, 33, 34, 35, 36,
 37, 38, 39, 40, 41, 42
};
#ifndef res__state_t
#define res__state_t unsigned char
#endif

static res__state_t res__begin[] = {
 0, 0, 24, 24, 30, 30, 34, 34, 0
};

static res__state_t res__next[] = {
 8, 8, 8, 8, 8, 8, 8, 8, 8, 5, 6, 8, 8, 5, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 5, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 8, 8, 8, 8,
 8, 3, 3, 3, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 8, 8, 8, 8,
 9, 11, 13, 12, 14, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 15, 16, 17, 18, 19, 20, 21, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 22, 23, 28, 29, 12, 37, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
 12, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 25, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
 26, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 32, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 31, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 35, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 0
};

static res__state_t res__check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 7, 4, 2, 3, 13, 7, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 14, 1, 16, 17, 18, 19, 20, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 21, 22, 25, 28, 3, 36, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 33, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U,
 ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, ~0U, 33, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34,
 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 0
};

static res__state_t res__default[] = {
 37, 3, 3, 37, 37, 37, 37, 37, 37, 37, 37, 37, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 3, 37, 37, 37, 37, 37, 37, 37, 37,
 37, 30, 37, 37, 34, 0
};

static int res__base[] = {
 0, 40, 25, 86, 95, 618, 618, 86, 618, 618, 618, 618, 618, 24, 43, 618,
 32, 46, 49, 33, 39, 63, 57, 618, 209, 132, 618, 618, 170, 618, 337, 618,
 618, 455, 490, 618, 172, 618
};

/*
 * Copyright 1988, 1992 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 *
 * $Header: /u/rd/src/lex/rcs/res_lex.c 1.57 1995/12/11 22:14:06 fredw Exp $
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
 * graceful exits from res_lex()
 * without resorting to calling exit();
 */

#ifndef YYEXIT
#define YYEXIT	1
#endif

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling res_lex must supply this!
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
 * You can redefine res_getc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define res_getc()	yt_getc()
#else
#define	res_getc()	getc(res_in) 	/* res_lex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#ifdef YYEXIT
#define	YY_FATAL(msg)	{ fprintf(res_out, "res_lex: %s\n", msg); res_LexFatal = 1; }
#else /* YYEXIT */
#define	YY_FATAL(msg)	{ fprintf(stderr, "res_lex: %s\n", msg); exit(1); }
#endif /* YYEXIT */

#undef ECHO
#define	ECHO		fputs(res_text, res_out)

#define	output(c)	putc((c), res_out) /* res_lex sink for unmatched chars */
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */

#define	BEGIN		res__start =
#define	NLSTATE		(res__lastc = YYNEWLINE)
#define	YY_INIT \
	(res__start = res_leng = res__end = 0, res__lastc = YYNEWLINE)
#define	res_less(n)	if ((n) < 0 || (n) > res__end) ; \
			else { YY_SCANNER; res_leng = (n); YY_USER; }

YY_DECL	void	res__reset YY_ARGS((void));

/* functions defined in libl.lib */
extern	int	res_wrap	YY_ARGS((void));
extern	void	res_error	YY_ARGS((char *fmt, ...));
extern	void	res_comment	YY_ARGS((char *term));
extern	int	res_mapch	YY_ARGS((int delim, int escape));

/*
 * resource.l
 *
 * Lex file for parsing res files
 */


#include <stdio.h>


/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include <string.h>
#include "types.h"
#include "debug.h"
#include "resly.h"

/* Get the Yacc definitions */
#include "resource_y.h"

/* Maximum length for any TEXT value */
#define YYLMAX	255

/* Store for any string values */
STRING aText[TEXT_BUFFERS][YYLMAX];		// No longer static ... lets use this area globally
static UDWORD currText=0;

// Note if we are in a comment
static BOOL inComment = FALSE;

/* Pointer to the input buffer */
static UBYTE *pInputBuffer = NULL;
static UBYTE *pEndBuffer = NULL;

#undef res_getc
#define res_getc() (pInputBuffer != pEndBuffer ? *(pInputBuffer++) : EOF)

#ifndef YYLMAX
#define	YYLMAX		100		/* token and pushback buffer size */
#endif /* YYLMAX */

/*
 * If %array is used (or defaulted), res_text[] contains the token.
 * If %pointer is used, res_text is a pointer to res__tbuf[].
 */
YY_DECL char	res_text[YYLMAX+1];

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
#define	YYLEX res_lex			/* name of lex scanner */
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
YY_DECL	FILE   *res_in = stdin;
YY_DECL	FILE   *res_out = stdout;
#else
YY_DECL	FILE   *res_in = (FILE *)0;
YY_DECL	FILE   *res_out = (FILE *)0;
#endif
YY_DECL	int	res_lineno = 1;		/* line number */

/* res__sbuf[0:res_leng-1] contains the states corresponding to res_text.
 * res_text[0:res_leng-1] contains the current token.
 * res_text[res_leng:res__end-1] contains pushed-back characters.
 * When the user action routine is active,
 * res__save contains res_text[res_leng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */
static	res__state_t res__sbuf [YYLMAX+1];	/* state buffer */
static	int	res__end = 0;		/* end of pushback */
static	int	res__start = 0;		/* start state */
static	int	res__lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	res_leng = 0;		/* res_text token length */
#ifdef YYEXIT
static	int res_LexFatal;
#endif /* YYEXIT */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char res__save;	/* saved res_text[res_leng] */

#define	YY_USER	{ /* set up res_text for user */ \
		res__save = res_text[res_leng]; \
		res_text[res_leng] = 0; \
	}
#define	YY_SCANNER { /* set up res_text for scanner */ \
		res_text[res_leng] = res__save; \
	}

#else		/* not-so efficient push-back for res_text mungers */

static	char res__save [YYLMAX];
static	char *res__push = res__save+YYLMAX;

#define	YY_USER { \
		size_t n = res__end - res_leng; \
		res__push = res__save+YYLMAX - n; \
		if (n > 0) \
			memmove(res__push, res_text+res_leng, n); \
		res_text[res_leng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = res__save+YYLMAX - res__push; \
		if (n > 0) \
			memmove(res_text+res_leng, res__push, n); \
		res__end = res_leng + n; \
	}

#endif


#ifdef LEX_WINDOWS

/*
 * When using the windows features of lex,
 * it is necessary to load in the resources being
 * used, and when done with them, the resources must
 * be freed up, otherwise we have a windows app that
 * is not following the rules. Thus, to make res_lex()
 * behave in a windows environment, create a new
 * res_lex() which will call the original res_lex() as
 * another function call. Observe ...
 */

/*
 * The actual lex scanner (usually res_lex(void)).
 * NOTE: you should invoke res__init() if you are calling res_lex()
 * with new input; otherwise old lookaside will get in your way
 * and res_lex() will die horribly.
 */
static int win_res_lex();			/* prototype for windows res_lex handler */

YYDECL {
	int wReturnValue;
	HANDLE hRes_table;
	unsigned short far *old_res__la_act;	/* remember previous pointer values */
	short far *old_res__final;
	res__state_t far *old_res__begin;
	res__state_t far *old_res__next;
	res__state_t far *old_res__check;
	res__state_t far *old_res__default;
	short far *old_res__base;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst,
		FindResource (hInst, "UD_RES_res_LEX", "res_LEXTBL"));
	
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

	old_res__la_act = res__la_act;
	old_res__final = res__final;
	old_res__begin = res__begin;
	old_res__next = res__next;
	old_res__check = res__check;
	old_res__default = res__default;
	old_res__base = res__base;

	res__la_act = (unsigned short far *)LockResource (hRes_table);
	res__final = (short far *)(res__la_act + Sizeof_res__la_act);
	res__begin = (res__state_t far *)(res__final + Sizeof_res__final);
	res__next = (res__state_t far *)(res__begin + Sizeof_res__begin);
	res__check = (res__state_t far *)(res__next + Sizeof_res__next);
	res__default = (res__state_t far *)(res__check + Sizeof_res__check);
	res__base = (res__state_t far *)(res__default + Sizeof_res__default);


	/*
	 * call the standard res_lex() code
	 */

	wReturnValue = win_res_lex();

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

	res__la_act = old_res__la_act;
	res__final = old_res__final;
	res__begin = old_res__begin;
	res__next = old_res__next;
	res__check = old_res__check;
	res__default = old_res__default;
	res__base = old_res__base;

	return (wReturnValue);
}	/* end function */

static int win_res_lex() {

#else /* LEX_WINDOWS */

/*
 * The actual lex scanner (usually res_lex(void)).
 * NOTE: you should invoke res__init() if you are calling res_lex()
 * with new input; otherwise old lookaside will get in your way
 * and res_lex() will die horribly.
 */
YYDECL {

#endif /* LEX_WINDOWS */

	register int c, i, res_base;
	unsigned	res_st;	/* state */
	int res_fmin, res_fmax;	/* res__la_act indices of final states */
	int res_oldi, res_oleng;	/* base i, res_leng before look-ahead */
	int res_eof;		/* 1 if eof has already been read */


#if !YY_STATIC_STDIO
	if (res_in == (FILE *)0)
		res_in = stdin;
	if (res_out == (FILE *)0)
		res_out = stdout;
#endif

#ifdef YYEXIT
	res_LexFatal = 0;
#endif /* YYEXIT */

	res_eof = 0;
	i = res_leng;
	YY_SCANNER;

  res__again:
	res_leng = i;
	/* determine previous char. */
	if (i > 0)
		res__lastc = res_text[i-1];
	/* scan previously accepted token adjusting res_lineno */
	while (i > 0)
		if (res_text[--i] == YYNEWLINE)
			res_lineno++;
	/* adjust pushback */
	res__end -= res_leng;
	if (res__end > 0)
		memmove(res_text, res_text+res_leng, (size_t) res__end);
	i = 0;

	res_oldi = i;

	/* run the state machine until it jams */
	res_st = res__begin[res__start + ((res__lastc == YYNEWLINE) ? 1 : 0)];
	res__sbuf[i] = (res__state_t) res_st;
	do {
		YY_DEBUG(m_textmsg(1547, "<state %d, i = %d>\n", "I num1 num2"), res_st, i);
		if (i >= YYLMAX) {
			YY_FATAL(m_textmsg(1548, "Token buffer overflow", "E"));
#ifdef YYEXIT
			if (res_LexFatal)
				return -2;
#endif /* YYEXIT */
		}	/* endif */

		/* get input char */
		if (i < res__end)
			c = res_text[i];		/* get pushback char */
		else if (!res_eof && (c = res_getc()) != EOF) {
			res__end = i+1;
			res_text[i] = (char) c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == res_oldi) {	/* no token */
				res_eof = 0;
				if (res_wrap())
					return 0;
				else
					goto res__again;
			} else {
				res_eof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(m_textmsg(1549, "<input %d = 0x%02x>\n", "I num hexnum"), c, c);

		/* look up next state */
		while ((res_base = res__base[res_st]+(unsigned char)c) > res__nxtmax
		    || res__check[res_base] != (res__state_t) res_st) {
			if (res_st == res__endst)
				goto res__jammed;
			res_st = res__default[res_st];
		}
		res_st = res__next[res_base];
	  res__jammed: ;
	  res__sbuf[++i] = (res__state_t) res_st;
	} while (!(res_st == res__endst || (YY_INTERACTIVE && res__base[res_st] > res__nxtmax && res__default[res_st] == res__endst)));
	YY_DEBUG(m_textmsg(1550, "<stopped %d, i = %d>\n", "I num1 num2"), res_st, i);
	if (res_st != res__endst)
		++i;

	/* search backward for a final state */
	while (--i > res_oldi) {
		res_st = res__sbuf[i];
		if ((res_fmin = res__final[res_st]) < (res_fmax = res__final[res_st+1]))
			goto res__found;	/* found final state(s) */
	}
	/* no match, default action */
	i = res_oldi + 1;
	output(res_text[res_oldi]);
	goto res__again;

  res__found:
	YY_DEBUG(m_textmsg(1551, "<final state %d, i = %d>\n", "I num1 num2"), res_st, i);
	res_oleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(res__la_act[res_fmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = res__look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < res_oldi) {	/* no / */
				i = res_oleng;
				break;
			}
			res_st = res__sbuf[i];
			if (bv[(unsigned)res_st/8] & bits[(unsigned)res_st%8])
				break;
		}
	}

	/* perform action */
	res_leng = i;
	YY_USER;
	switch (res__la_act[res_fmin] & 0777) {
	case 0:
	{ return DIRECTORY; }
	break;
	case 1:
	{ return FILETOKEN; }
	break;
	case 2:
	{
								strcpy(aText[currText], res_text);
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return TEXT;
							}
	break;
	case 3:
	{ BEGIN QUOTE; }
	break;
	case 4:
	{ BEGIN 0; }
	break;
	case 5:
	{ res_error("Unexpected end of line in string"); }
	break;
	case 6:
	{
								strcpy(aText[currText], res_text);
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}
	break;
	case 7:
	{
								aText[currText][0] = '\0';
								aText[currText][1] = '\0';
								res_lval.sval = aText[currText];
								currText = (currText + 1) % TEXT_BUFFERS;
								return QTEXT;
							}
	break;
	case 8:
	;
	break;
	case 9:
	{ inComment=TRUE; BEGIN COMMENT; }
	break;
	case 10:
	case 11:
	{ inComment=FALSE; BEGIN 0; }
	break;
	case 12:
	case 13:
	;
	break;
	case 14:
	{ BEGIN SLCOMMENT; }
	break;
	case 15:
	{ BEGIN 0; }
	break;
	case 16:
	;
	break;
	case 17:
	return res_text[0];
	break;

	}
	YY_SCANNER;
	i = res_leng;
	goto res__again;			/* action fell though */
}

/*
 * Safely switch input stream underneath LEX
 */
typedef struct res__save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	res__state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
res_SaveScan(FILE *fp)
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = res_in;
	p->oldline = res_lineno;
	p->oldend = res__end;
	p->oldstart = res__start;
	p->oldlastc = res__lastc;
	p->oldleng = res_leng;
	(void) memcpy(p->savetext, res_text, sizeof res_text);
	(void) memcpy((char *) p->savestate, (char *) res__sbuf,
		sizeof res__sbuf);

	res_in = fp;
	res_lineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
res_RestoreScan(YY_SAVED *p)
{
	if (p == NULL)
		return;
	res_in = p->oldfp;
	res_lineno = p->oldline;
	res__end = p->oldend;
	res__start = p->oldstart;
	res__lastc = p->oldlastc;
	res_leng = p->oldleng;

	(void) memcpy(res_text, p->savetext, sizeof res_text);
	(void) memcpy((char *) res__sbuf, (char *) p->savestate,
		sizeof res__sbuf);
	free(p);
}
/*
 * User-callable re-initialization of res_lex()
 */
void
res__reset()
{
	YY_INIT;
	res_lineno = 1;		/* line number */
}

/* Set the current input buffer for the lexer */
void resSetInputBuffer(UBYTE *pBuffer, UDWORD size)
{
	pInputBuffer = pBuffer;
	pEndBuffer = pBuffer + size;

	/* Reset the lexer incase it's been used before */
	res__reset();
	inComment = FALSE;
}

void resGetErrorData(int *pLine, char **ppText)
{
	*pLine = res_lineno;
	*ppText = res_text;
}

int res_wrap(void)
{
	if (inComment)
	{
		DBERROR(("Warning: reched end of file in a comment"));
	}
	return 1;
}

