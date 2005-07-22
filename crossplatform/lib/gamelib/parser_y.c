/* d:\mks-ly\mksnt\yacc -p audp_ -o parser_y.c -D parser_y.h .\parser.y */
#ifdef YYTRACE
#define YYDEBUG 1
#else
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#endif
/*
 * Portable way of defining ANSI C prototypes
 */
#ifndef YY_ARGS
#ifdef __STDC__
#define YY_ARGS(x)	x
#else
#define YY_ARGS(x)	()
#endif
#endif

#ifdef YACC_WINDOWS

#include <windows.h>

/*
 * the following is the handle to the current
 * instance of a windows program. The user
 * program calling audp_parse must supply this!
 */

#ifdef STRICT
extern HINSTANCE hInst;	
#else
extern HANDLE hInst;	
#endif

#endif	/* YACC_WINDOWS */

#if YYDEBUG
typedef struct yyNamedType_tag {	/* Tokens */
	char	* name;		/* printable name */
	short	token;		/* token # */
	short	type;		/* token type */
} yyNamedType;
typedef struct yyTypedRules_tag {	/* Typed rule table */
	char	* name;		/* compressed rule string */
	short	type;		/* rule result type */
} yyTypedRules;

#endif

#include <stdio.h>

#include "frame.h"
#include "parser.h"
#include "audio.h"
#include "anim.h"

static int			g_iCurAnimID = 0;
static int			g_iDummy;
static VECTOR3D		vecPos, vecRot, vecScale;

typedef union {
	float		fval;
	long		ival;
	signed char	bval;
	char		sval[100];
} YYSTYPE;
#define FLOAT	257
#define INTEGER	258
#define TEXT	259
#define text	260
#define QTEXT	261
#define LOOP	262
#define ONESHOT	263
#define AUDIO	264
#define ANIM2D	265
#define ANIM3DFRAMES	266
#define ANIM3DTRANS	267
#define ANIM3DFILE	268
#define AUDIO_MODULE	269
#define ANIM_MODULE	270
#define ANIMOBJECT	271
extern int audp_char, yyerrflag;
extern YYSTYPE audp_lval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_fval, YY_t_ival, YY_t_sval
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02",  0},
	{ "%01: %03",  0},
	{ "%02: %02 %04",  0},
	{ "%02: %04",  0},
	{ "%04: %05",  0},
	{ "%04: %06",  0},
	{ "%07: &14 &17",  0},
	{ "%05: %07 %08 &18",  0},
	{ "%05: %07 &18",  0},
	{ "%08: %08 %09",  0},
	{ "%08: %09",  0},
	{ "%09: &09 &06 &07 &03 &03 &03",  0},
	{ "%09: &09 &06 &08 &03 &03 &03",  0},
	{ "%10: &15 &17",  0},
	{ "%06: %10 %11 &18",  0},
	{ "%06: %10 %12 &18",  0},
	{ "%06:",  0},
	{ "%12: %12 %13",  0},
	{ "%12: %13",  0},
	{ "%12:",  0},
	{ "%11: %11 %03",  0},
	{ "%11: %03",  0},
	{ "%11:",  0},
	{ "%03: %14",  0},
	{ "%03: %15",  0},
	{ "%13: &06 &03",  0},
	{ "%16:",  0},
	{ "%14: &12 &06 &03 &03 &03 %16 &17 %17 &18",  0},
	{ "%18:",  0},
	{ "%19:",  0},
	{ "%15: &11 &06 &03 &03 %18 &17 %19 %20 &18",  0},
	{ "%17: %21 %17",  0},
	{ "%17: %21",  0},
	{ "%22:",  0},
	{ "%21: &16 &03 &06 &17 %22 %20 &18",  0},
	{ "%20: %20 %23",  0},
	{ "%20: %23",  0},
	{ "%23: &03 &03 &03 &03 &03 &03 &03 &03 &03 &03",  0},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "FLOAT",  257,  1},
	{ "INTEGER",  258,  2},
	{ "TEXT",  259,  3},
	{ "text",  260,  3},
	{ "QTEXT",  261,  3},
	{ "LOOP",  262,  2},
	{ "ONESHOT",  263,  2},
	{ "AUDIO",  264,  0},
	{ "ANIM2D",  265,  0},
	{ "ANIM3DFRAMES",  266,  0},
	{ "ANIM3DTRANS",  267,  0},
	{ "ANIM3DFILE",  268,  0},
	{ "AUDIO_MODULE",  269,  0},
	{ "ANIM_MODULE",  270,  0},
	{ "ANIMOBJECT",  271,  0},
	{ ""{"",  123,  0},
	{ ""}"",  125,  0}

};
#endif
static short yydef[] = {

	65535, 65531,   40, 65527,   36
};
static short yyex[] = {

	   0,   39, 65535,    1,  125,   38, 65535,    1,    0,    0, 
	65535,    1
};
static short yyact[] = {

	65489, 65490, 65493, 65491,  270,  269,  267,  266, 65494,  261, 
	65495,  261, 65496, 65489, 65490,  267,  266,  261, 65479,  123, 
	65469, 65499,  264,  125, 65471,  123, 65493, 65491,  270,  269, 
	65501,  258, 65502,  258, 65480,  258, 65465, 65496,  261,  125, 
	65466, 65489, 65490,  267,  266,  125, 65503,  261, 65470, 65499, 
	 264,  125, 65483,  258, 65504,  258, 65506, 65505,  263,  262, 
	65481,  258, 65508,  258, 65509,  258, 65484,  123, 65511,  258, 
	65512,  258, 65514,  123, 65478,  258, 65477,  258, 65515,  258, 
	65517,  271, 65519,  258, 65485, 65515,  258,  125, 65520,  258, 
	65482,  125, 65521,  258, 65522,  261, 65523,  258, 65486,  123, 
	65524,  258, 65526,  258, 65528,  258, 65487, 65515,  258,  125, 
	65529,  258, 65530,  258, 65488,  258,   -1
};
static short yypact[] = {

	   4,   15,   28,   30,   81,  115,  113,  111,  108,  105, 
	  79,  103,  101,   99,   97,   95,   93,   91,   89,   86, 
	  83,   81,   79,   77,   75,   73,   71,   69,   67,   65, 
	  63,   61,   58,   55,   53,   50,   47,   43,   38,   35, 
	  33,   31,   25,   22,   19,   11,    9
};
static short yygo[] = {

	65532, 65533, 65461, 65462, 65476,   37,    1, 65475, 65474,    2, 
	65473, 65472, 65492, 65500, 65468, 65467,   35, 65534, 65498, 65497, 
	65464, 65463,   38, 65460, 65459, 65510, 65458, 65518,    4, 65507, 
	65513, 65527, 65516,   10, 65531, 65525, 65457, 65457, 65456,   19, 
	   8,   -1
};
static short yypgo[] = {

	   0,    0,    0,   38,   34,   35,   24,   30,   29,   23, 
	  25,   21,   17,   15,   15,    0,    1,    1,    8,    8, 
	  12,   10,   10,   13,   13,   11,   11,   19,   19,   18, 
	  18,    4,    4,   27,   32,   32,   27,   18,   19,   11, 
	   0,    0
};
static short yyrlen[] = {

	   0,    0,    0,   10,    7,    0,    9,    0,    0,    9, 
	   0,    2,    2,    6,    6,    1,    2,    1,    1,    1, 
	   2,    3,    2,    2,    1,    3,    3,    2,    1,    2, 
	   1,    1,    1,    2,    2,    1,    1,    0,    0,    0, 
	   1,    2
};
#define YYS0	0
#define YYDELTA	44
#define YYNPACT	47
#define YYNDEF	5

#define YYr39	0
#define YYr40	1
#define YYr41	2
#define YYr38	3
#define YYr35	4
#define YYr34	5
#define YYr31	6
#define YYr30	7
#define YYr29	8
#define YYr28	9
#define YYr27	10
#define YYr26	11
#define YYr14	12
#define YYr13	13
#define YYr12	14
#define YYrACCEPT	YYr39
#define YYrERROR	YYr40
#define YYrLR2	YYr41
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"data_file",
	"module_file",
	"anim_file",
	"data_list",
	"audio_module",
	"anim_module",
	"audio_header",
	"audio_list",
	"audio_track",
	"anim_module_header",
	"anim_file_list",
	"anim_config_list",
	"anim_config",
	"anim_trans",
	"anim_frames",
	"$27",
	"anim_obj_list",
	"$29",
	"$30",
	"anim_script",
	"anim_obj",
	"$34",
	"anim_state",
	0
};
short yyrmap[] = {

	  39,   40,   41,   38,   35,   34,   31,   30,   29,   28, 
	  27,   26,   14,   13,   12,    2,    3,    4,    5,    6, 
	   7,    8,    9,   10,   11,   15,   16,   18,   19,   21, 
	  22,   24,   25,   32,   36,   37,   33,   23,   20,   17, 
	   1,    0
};
short yysmap[] = {

	   0,    5,   13,   14,   59,   78,   77,   75,   74,   73, 
	  72,   71,   69,   68,   67,   64,   61,   60,   58,   57, 
	  55,   52,   51,   50,   49,   48,   46,   45,   43,   42, 
	  41,   40,   36,   30,   29,   26,   23,   21,   20,   17, 
	  16,   15,    8,    7,    6,    2,    1,   79,   76,   70, 
	  63,   47,   39,   66,   44,   31,   22,   53,   54,   12, 
	  28,   11,   10,    9,   27,   38,   25,   37,   24,   35, 
	  33,   32,   19,   34,   18,    4,    3,   65,   62,   56
};
int yyntoken = 19;
int yynvar = 24;
int yynstate = 80;
int yynrule = 42;
#endif

#if YYDEBUG
/*
 * Package up YACC context for tracing
 */
typedef struct yyTraceItems_tag {
	int	state, lookahead, errflag, done;
	int	rule, npop;
	short	* states;
	int	nstates;
	YYSTYPE * values;
	int	nvalues;
	short	* types;
} yyTraceItems;
#endif

/*
 * Copyright 1985, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * 
 * Automaton to interpret LALR(1) tables.
 *
 * Macros:
 *	yyclearin - clear the lookahead token.
 *	yyerrok - forgive a pending error
 *	YYERROR - simulate an error
 *	YYACCEPT - halt and return 0
 *	YYABORT - halt and return 1
 *	YYRETURN(value) - halt and return value.  You should use this
 *		instead of return(value).
 *	YYREAD - ensure audp_char contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and audp_debug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not audp_debug.
 *	YYTRACE - turn on YYDEBUG, and undefine default trace functions
 *		so that the interactive functions in 'ytrack.c' will
 *		be used.
 *	YYSSIZE - size of state and value stacks (default 150).
 *	YYSTATIC - By default, the state stack is an automatic array.
 *		If this is defined, the stack will be static.
 *		In either case, the value stack is static.
 *	YYALLOC - Dynamically allocate both the state and value stacks
 *		by calling malloc() and free().
 *	YYDYNAMIC - Dynamically allocate (and reallocate, if necessary)
 *		both the state and value stacks by calling malloc(),
 *		realloc(), and free().
 *	YYSYNC - if defined, yacc guarantees to fetch a lookahead token
 *		before any action, even if it doesnt need it for a decision.
 *		If YYSYNC is defined, YYREAD will never be necessary unless
 *		the user explicitly sets audp_char = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int audp_lex YY_ARGS((void));
extern void audp_error YY_ARGS((char *, ...));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print audp_lval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and audp_debug set,
 * tracing functions will be called at appropriate times in audp_parse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, audp_parse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = audp_char; yyx.errflag =yyerrflag; \
	yyx.states = yys+1; yyx.nstates = yyps-yys; \
	yyx.values = yyv+1; yyx.nvalues = yypv-yyv; \
	yyx.types = yytypev+1; yyx.done = 0; \
	yyx.rule = yyi; yyx.npop = yyj; \
	fn(&yyx); \
	if (yyx.done) YYRETURN(-1); }

#ifndef I18N
#define m_textmsg(id, str, cls)	(str)
#else /*I18N*/
#include <m_nls.h>
#endif/*I18N*/

#ifndef YYSSIZE
# define YYSSIZE	150
#endif

#ifdef YYDYNAMIC
#define YYALLOC
char *getenv();
int atoi();
int yysinc = -1; /* stack size increment, <0 = double, 0 = none, >0 = fixed */
#endif

#ifdef YYALLOC
int yyssize = YYSSIZE;
#endif

#define yyerrok		yyerrflag = 0
#if YYDEBUG
#define yyclearin	{ if (audp_debug) yyShowRead(-1); audp_char = -1; }
#else
#define yyclearin	audp_char = -1
#endif
#define YYACCEPT	YYRETURN(0)
#define YYABORT		YYRETURN(1)
#define YYRECOVERING()	(yyerrflag != 0)
#ifdef YYALLOC
#define YYRETURN(val)	{ retval = (val); goto yyReturn; }
#else
#define YYRETURN(val)	return(val);
#endif
#if YYDEBUG
/* The if..else makes this macro behave exactly like a statement */
# define YYREAD	if (audp_char < 0) {					\
			if ((audp_char = audp_lex()) < 0)	{		\
				if (audp_char == -2) YYABORT; \
				audp_char = 0;				\
			}	/* endif */			\
			if (audp_debug)					\
				yyShowRead(audp_char);			\
		} else
#else
# define YYREAD	if (audp_char < 0) {					\
			if ((audp_char = audp_lex()) < 0) {			\
				if (audp_char == -2) YYABORT; \
				audp_char = 0;				\
			}	/* endif */			\
		} else
#endif

#define YYERRCODE	256		/* value of `error' */
#define YYTOKEN_BASE	256
#define	YYQYYP	yyq[yyq-yyp]

/*
 * Simulate bitwise negation as if was done on a two's complement machine.
 * This makes the generated code portable to machines with different
 * representations of integers (ie. signed magnitude).
 */
#define	yyneg(s)	(-((s)+1))

YYSTYPE	yyval;				/* $ */
YYSTYPE	*yypvt;				/* $n */
YYSTYPE	audp_lval;				/* audp_lex() sets this */

int	audp_char,				/* current token */
	yyerrflag,			/* error flag */
	yynerrs;			/* error count */

#if YYDEBUG
int audp_debug = 0;		/* debug if this flag is set */
extern char	*yysvar[];	/* table of non-terminals (aka 'variables') */
extern yyNamedType yyTokenTypes[];	/* table of terminals & their types */
extern short	yyrmap[], yysmap[];	/* map internal rule/states */
extern int	yynstate, yynvar, yyntoken, yynrule;

extern int	yyGetType YY_ARGS((int));	/* token type */
extern char	*yyptok YY_ARGS((int));	/* printable token string */
extern int	yyExpandName YY_ARGS((int, int, char *, int));
				  /* expand yyRules[] or yyStates[] */
static char *	yygetState YY_ARGS((int));

#define yyassert(condition, msg, arg) \
	if (!(condition)) { \
		printf(m_textmsg(2824, "\nyacc bug: ", "E")); \
		printf(msg, arg); \
		YYABORT; }
#else /* !YYDEBUG */
#define yyassert(condition, msg, arg)
#endif

/***************************************************************************/
/* A simple error reporting routine */

void audp_error(char *pMessage,...)
{
	int		line;
	char	*pText;
	char	aTxtBuf[1024];
	va_list	args;

	va_start(args, pMessage);

	vsprintf(aTxtBuf, pMessage, args);
	parseGetErrorData( &line, &pText );
	DBERROR(("RES file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  aTxtBuf, line, audp_char, pText));

	va_end(args);
}

/***************************************************************************/

#ifdef YACC_WINDOWS

/*
 * the following is the audp_parse() function that will be
 * callable by a windows type program. It in turn will
 * load all needed resources, obtain pointers to these
 * resources, and call a statically defined function
 * win_yyparse(), which is the original audp_parse() fn
 * When win_yyparse() is complete, it will return a
 * value to the new audp_parse(), where it will be stored
 * away temporarily, all resources will be freed, and
 * that return value will be given back to the caller
 * audp_parse(), as expected.
 */

static int win_yyparse();			/* prototype */

int audp_parse() 
{
	int wReturnValue;
	HANDLE hRes_table;		/* handle of resource after loading */
	short far *old_yydef;		/* the following are used for saving */
	short far *old_yyex;		/* the current pointers */
	short far *old_yyact;
	short far *old_yypact;
	short far *old_yygo;
	short far *old_yypgo;
	short far *old_yyrlen;

	/*
	 * the following code will load the required
	 * resources for a Windows based parser.
	 */

	hRes_table = LoadResource (hInst, 
		FindResource (hInst, "UD_RES_yyYACC", "yyYACCTBL"));
	
	/*
	 * return an error code if any
	 * of the resources did not load
	 */

	if (hRes_table == NULL)
		return (1);
	
	/*
	 * the following code will lock the resources
	 * into fixed memory locations for the parser
	 * (also, save the current pointer values first)
	 */

	old_yydef = yydef;
	old_yyex = yyex;
	old_yyact = yyact;
	old_yypact = yypact;
	old_yygo = yygo;
	old_yypgo = yypgo;
	old_yyrlen = yyrlen;

	yydef = (short far *)LockResource (hRes_table);
	yyex = (short far *)(yydef + Sizeof_yydef);
	yyact = (short far *)(yyex + Sizeof_yyex);
	yypact = (short far *)(yyact + Sizeof_yyact);
	yygo = (short far *)(yypact + Sizeof_yypact);
	yypgo = (short far *)(yygo + Sizeof_yygo);
	yyrlen = (short far *)(yypgo + Sizeof_yypgo);

	/*
	 * call the official audp_parse() function
	 */

	wReturnValue = win_yyparse();

	/*
	 * unlock the resources
	 */

	UnlockResource (hRes_table);

	/*
	 * and now free the resource
	 */

	FreeResource (hRes_table);

	/*
	 * restore previous pointer values
	 */

	yydef = old_yydef;
	yyex = old_yyex;
	yyact = old_yyact;
	yypact = old_yypact;
	yygo = old_yygo;
	yypgo = old_yypgo;
	yyrlen = old_yyrlen;

	return (wReturnValue);
}	/* end audp_parse */

static int win_yyparse() 

#else /* YACC_WINDOWS */

/*
 * we are not compiling a windows resource
 * based parser, so call audp_parse() the old
 * standard way.
 */

int audp_parse() 

#endif /* YACC_WINDOWS */

{
#ifdef YACC_WINDOWS
	register short far	*yyp;	/* for table lookup */
	register short far	*yyq;
#else
	register short		*yyp;	/* for table lookup */
	register short		*yyq;
#endif	/* YACC_WINDOWS */
	register short		yyi;
	register short		*yyps;		/* top of state stack */
	register short		yystate;	/* current state */
	register YYSTYPE	*yypv;		/* top of value stack */
	register int		yyj;
#if YYDEBUG
	yyTraceItems	yyx;			/* trace block */
	short	* yytp;
	int	yyruletype = 0;
#endif
#ifdef YYSTATIC
	static short	yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];
#if YYDEBUG
	static short	yytypev[YYSSIZE+1];	/* type assignments */
#endif
#else /* ! YYSTATIC */
#ifdef YYALLOC
	YYSTYPE *yyv;
	short	*yys;
#if YYDEBUG
	short	*yytypev;
#endif
	YYSTYPE save_yylval;
	YYSTYPE save_yyval;
	YYSTYPE *save_yypvt;
	int save_yychar, save_yyerrflag, save_yynerrs;
	int retval; 			/* return value holder */
#else
	short		yys[YYSSIZE + 1];
	static YYSTYPE	yyv[YYSSIZE + 1];	/* historically static */
#if YYDEBUG
	short	yytypev[YYSSIZE+1];		/* mirror type table */
#endif
#endif /* ! YYALLOC */
#endif /* ! YYSTATIC */
#ifdef YYDYNAMIC
	char *envp;
#endif


#ifdef YYDYNAMIC
	if ((envp = getenv("YYSTACKSIZE")) != (char *)0) {
		yyssize = atoi(envp);
		if (yyssize <= 0)
			yyssize = YYSSIZE;
	}
	if ((envp = getenv("YYSTACKINC")) != (char *)0)
		yysinc = atoi(envp);
#endif
#ifdef YYALLOC
	yys = (short *) malloc((yyssize + 1) * sizeof(short));
	yyv = (YYSTYPE *) malloc((yyssize + 1) * sizeof(YYSTYPE));
#if YYDEBUG
	yytypev = (short *) malloc((yyssize + 1) * sizeof(short));
#endif
	if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
		|| yytypev == (short *) 0
#endif
	) {
		audp_error(m_textmsg(4967, "Not enough space for parser stacks",
				  "E"));
		return 1;
	}
	save_yylval = audp_lval;
	save_yyval = yyval;
	save_yypvt = yypvt;
	save_yychar = audp_char;
	save_yyerrflag = yyerrflag;
	save_yynerrs = yynerrs;
#endif

	yynerrs = 0;
	yyerrflag = 0;
	yyclearin;
	yyps = yys;
	yypv = yyv;
	*yyps = yystate = YYS0;		/* start state */
#if YYDEBUG
	yytp = yytypev;
	yyi = yyj = 0;			/* silence compiler warnings */
#endif

yyStack:
	yyassert((unsigned)yystate < yynstate, m_textmsg(587, "state %d\n", ""), yystate);
#ifdef YYDYNAMIC
	if (++yyps > &yys[yyssize]) {
		int yynewsize;
		int yysindex = yyps - yys;
		int yyvindex = yypv - yyv;
#if YYDEBUG
		int yytindex = yytp - yytypev;
#endif
		if (yysinc == 0) {		/* no increment */
			audp_error(m_textmsg(4968, "Parser stack overflow", "E"));
			YYABORT;
		} else if (yysinc < 0)		/* binary-exponential */
			yynewsize = yyssize * 2;
		else				/* fixed increment */
			yynewsize = yyssize + yysinc;
		if (yynewsize < yyssize) {
			audp_error(m_textmsg(4967,
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
		yyssize = yynewsize;
		yys = (short *) realloc(yys, (yyssize + 1) * sizeof(short));
		yyps = yys + yysindex;
		yyv = (YYSTYPE *) realloc(yyv, (yyssize + 1) * sizeof(YYSTYPE));
		yypv = yyv + yyvindex;
#if YYDEBUG
		yytypev = (short *)realloc(yytypev,(yyssize + 1)*sizeof(short));
		yytp = yytypev + yytindex;
#endif
		if (yys == (short *)0 || yyv == (YYSTYPE *)0
#if YYDEBUG
			|| yytypev == (short *) 0
#endif
		) {
			audp_error(m_textmsg(4967, 
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
	}
#else
	if (++yyps > &yys[YYSSIZE]) {
		audp_error(m_textmsg(4968, "Parser stack overflow", "E"));
		YYABORT;
	}
#endif /* !YYDYNAMIC */
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (audp_debug)
		YY_TRACE(yyShowState)
#endif

	/*
	 *	Look up next action in action table.
	 */
yyEncore:
#ifdef YYSYNC
	YYREAD;
#endif

#ifdef YACC_WINDOWS
	if (yystate >= Sizeof_yypact) 	/* simple state */
#else /* YACC_WINDOWS */
	if (yystate >= sizeof yypact/sizeof yypact[0]) 	/* simple state */
#endif /* YACC_WINDOWS */
		yyi = yystate - YYDELTA;	/* reduce in any case */
	else {
		if(*(yyp = &yyact[yypact[yystate]]) >= 0) {
			/* Look for a shift on audp_char */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = audp_char;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = yyneg(YYQYYP);
#if YYDEBUG
				if (audp_debug) {
					yyruletype = yyGetType(audp_char);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = audp_lval;	/* stack what audp_lex() set */
				yyclearin;		/* clear token */
				if (yyerrflag)
					yyerrflag--;	/* successful shift */
				goto yyStack;
			}
		}

		/*
	 	 *	Fell through - take default action
	 	 */

#ifdef YACC_WINDOWS
		if (yystate >= Sizeof_yydef)
#else /* YACC_WINDOWS */
		if (yystate >= sizeof yydef /sizeof yydef[0])
#endif /* YACC_WINDOWS */
			goto yyError;
		if ((yyi = yydef[yystate]) < 0)	 { /* default == reduce? */
			/* Search exception table */
#ifdef YACC_WINDOWS
			yyassert((unsigned)yyneg(yyi) < Sizeof_yyex,
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#else /* YACC_WINDOWS */
			yyassert((unsigned)yyneg(yyi) < sizeof yyex/sizeof yyex[0],
				m_textmsg(2825, "exception %d\n", "I num"), yystate);
#endif /* YACC_WINDOWS */
			yyp = &yyex[yyneg(yyi)];
#ifndef YYSYNC
			YYREAD;
#endif
			while((yyi = *yyp) >= 0 && yyi != audp_char)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 m_textmsg(2826, "Ex table not reduce %d\n", "I num"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, m_textmsg(2827, "reduce %d\n", "I num"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (audp_debug)
		YY_TRACE(yyShowReduce)
	yytp -= yyj;
#endif
	yyps -= yyj;		/* pop stacks */
	yypvt = yypv;		/* save top */
	yypv -= yyj;
	yyval = yypv[1];	/* default action $ = $1 */
#if YYDEBUG
	yyruletype = yyRules[yyrmap[yyi]].type;
#endif

	switch (yyi) {		/* perform semantic action */
		
case YYr12: {	/* audio_track :  AUDIO QTEXT LOOP INTEGER INTEGER INTEGER */
							audio_SetTrackVals( yypvt[-4].sval, TRUE, &g_iDummy, yypvt[-2].ival, yypvt[-1].ival, yypvt[0].ival, 0 );
} break;

case YYr13: {	/* audio_track :  AUDIO QTEXT ONESHOT INTEGER INTEGER INTEGER */
							audio_SetTrackVals( yypvt[-4].sval, FALSE, &g_iDummy, yypvt[-2].ival, yypvt[-1].ival, yypvt[0].ival, 0 );
} break;

case YYr14: {	/* anim_module_header :  ANIM_MODULE "{" */
} break;

case YYr26: {	/* anim_config :  QTEXT INTEGER */
							g_iCurAnimID = yypvt[0].ival;
							anim_SetVals( yypvt[-1].sval, yypvt[0].ival );
} break;

case YYr27: {	/* anim_trans :  ANIM3DTRANS QTEXT INTEGER INTEGER INTEGER */
							anim_Create3D( yypvt[-3].sval, yypvt[-2].ival, yypvt[-1].ival, yypvt[0].ival, ANIM_3D_TRANS, g_iCurAnimID );
} break;

case YYr28: {	/* anim_trans :  ANIM3DTRANS QTEXT INTEGER INTEGER INTEGER $27 "{" anim_obj_list "}" */
							g_iCurAnimID++;
} break;

case YYr29: {	/* anim_frames :  ANIM3DFRAMES QTEXT INTEGER INTEGER */
							anim_Create3D( yypvt[-2].sval, yypvt[-1].ival, yypvt[0].ival, 1, ANIM_3D_FRAMES, g_iCurAnimID );
} break;

case YYr30: {	/* anim_frames :  ANIM3DFRAMES QTEXT INTEGER INTEGER $29 "{" */
							anim_BeginScript();
} break;

case YYr31: {	/* anim_frames :  ANIM3DFRAMES QTEXT INTEGER INTEGER $29 "{" $30 anim_script "}" */
							anim_EndScript();
							g_iCurAnimID++;
} break;

case YYr34: {	/* anim_obj :  ANIMOBJECT INTEGER QTEXT "{" */
							anim_BeginScript();
} break;

case YYr35: {	/* anim_obj :  ANIMOBJECT INTEGER QTEXT "{" $34 anim_script "}" */
							anim_EndScript();
} break;

case YYr38: {	/* anim_state :  INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER INTEGER */
							vecPos.x   = yypvt[-8].ival;
							vecPos.y   = yypvt[-7].ival;
							vecPos.z   = yypvt[-6].ival;
							vecRot.x   = yypvt[-5].ival;
							vecRot.y   = yypvt[-4].ival;
							vecRot.z   = yypvt[-3].ival;
							vecScale.x = yypvt[-2].ival;
							vecScale.y = yypvt[-1].ival;
							vecScale.z = yypvt[0].ival;
							anim_AddFrameToAnim( yypvt[-9].ival, vecPos, vecRot, vecScale );
} break;
	case YYrACCEPT:
		YYACCEPT;
	case YYrERROR:
		goto yyError;
	}

	/*
	 *	Look up next state in goto table.
	 */

	yyp = &yygo[yypgo[yyi]];
	yyq = yyp++;
	yyi = *yyps;
	while (yyi < *yyp++)
		;

	yystate = yyneg(yyi == *--yyp? YYQYYP: *yyq);
#if YYDEBUG
	if (audp_debug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

/*
#pragma used yyerrlabel
 */
	yyerrflag = 1;
	if (yyi == YYrERROR) {
		yyps--;
		yypv--;
#if YYDEBUG
		yytp--;
#endif
	}

yyError:
	switch (yyerrflag) {

	case 0:		/* new error */
		yynerrs++;
		yyi = audp_char;
		audp_error(m_textmsg(4969, "Syntax error", "E"));
		if (yyi != audp_char) {
			/* user has changed the current token */
			/* try again */
			yyerrflag++;	/* avoid loops */
			goto yyEncore;
		}

	case 1:		/* partially recovered */
	case 2:
		yyerrflag = 3;	/* need 3 valid shifts to recover */
			
		/*
		 *	Pop states, looking for a
		 *	shift on `error'.
		 */

		for ( ; yyps > yys; yyps--, yypv--
#if YYDEBUG
					, yytp--
#endif
		) {
#ifdef YACC_WINDOWS
			if (*yyps >= Sizeof_yypact)
#else /* YACC_WINDOWS */
			if (*yyps >= sizeof yypact/sizeof yypact[0])
#endif /* YACC_WINDOWS */
				continue;
			yyp = &yyact[yypact[*yyps]];
			yyq = yyp;
			do {
				if (YYERRCODE == *yyp) {
					yyp++;
					yystate = yyneg(YYQYYP);
					goto yyStack;
				}
			} while (*yyp++ > YYTOKEN_BASE);
		
			/* no shift in this state */
#if YYDEBUG
			if (audp_debug && yyps > yys+1)
				YY_TRACE(yyShowErrRecovery)
#endif
			/* pop stacks; try again */
		}
		/* no shift on error - abort */
		break;

	case 3:
		/*
		 *	Erroneous token after
		 *	an error - discard it.
		 */

		if (audp_char == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (audp_debug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	audp_lval = save_yylval;
	yyval = save_yyval;
	yypvt = save_yypvt;
	audp_char = save_yychar;
	yyerrflag = save_yyerrflag;
	yynerrs = save_yynerrs;
	free((char *)yys);
	free((char *)yyv);
#if YYDEBUG
	free((char *)yytypev);
#endif
	return(retval);
#endif
}

/***************************************************************************/
/* Read a resource file */

BOOL ParseResourceFile( UBYTE *pData, UDWORD fileSize )
{
	// Tell lex about the input buffer
	parserSetInputBuffer( pData, fileSize );

	audp_parse();

	return TRUE;
}

		
#if YYDEBUG
/*
 * Return type of token
 */
int
yyGetType(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->type;
	return 0;
}
/*
 * Print a token legibly.
 */
char *
yyptok(tok)
int tok;
{
	yyNamedType * tp;
	for (tp = &yyTokenTypes[yyntoken-1]; tp > yyTokenTypes; tp--)
		if (tp->token == tok)
			return tp->name;
	return "";
}

/*
 * Read state 'num' from YYStatesFile
 */
#ifdef YYTRACE

static char *
yygetState(num)
int num;
{
	int	size;
	static FILE *yyStatesFile = (FILE *) 0;
	static char yyReadBuf[YYMAX_READ+1];

	if (yyStatesFile == (FILE *) 0
	 && (yyStatesFile = fopen(YYStatesFile, "r")) == (FILE *) 0)
		return "yyExpandName: cannot open states file";

	if (num < yynstate - 1)
		size = (int)(yyStates[num+1] - yyStates[num]);
	else {
		/* length of last item is length of file - ptr(last-1) */
		if (fseek(yyStatesFile, 0L, 2) < 0)
			goto cannot_seek;
		size = (int) (ftell(yyStatesFile) - yyStates[num]);
	}
	if (size < 0 || size > YYMAX_READ)
		return "yyExpandName: bad read size";
	if (fseek(yyStatesFile, yyStates[num], 0) < 0) {
	cannot_seek:
		return "yyExpandName: cannot seek in states file";
	}

	(void) fread(yyReadBuf, 1, size, yyStatesFile);
	yyReadBuf[size] = '\0';
	return yyReadBuf;
}
#endif /* YYTRACE */
/*
 * Expand encoded string into printable representation
 * Used to decode yyStates and yyRules strings.
 * If the expansion of 's' fits in 'buf', return 1; otherwise, 0.
 */
int
yyExpandName(num, isrule, buf, len)
int num, isrule;
char * buf;
int len;
{
	int	i, n, cnt, type;
	char	* endp, * cp;
	char	*s;

	if (isrule)
		s = yyRules[num].name;
	else
#ifdef YYTRACE
		s = yygetState(num);
#else
		s = "*no states*";
#endif

	for (endp = buf + len - 8; *s; s++) {
		if (buf >= endp) {		/* too large: return 0 */
		full:	(void) strcpy(buf, " ...\n");
			return 0;
		} else if (*s == '%') {		/* nonterminal */
			type = 0;
			cnt = yynvar;
			goto getN;
		} else if (*s == '&') {		/* terminal */
			type = 1;
			cnt = yyntoken;
		getN:
			if (cnt < 100)
				i = 2;
			else if (cnt < 1000)
				i = 3;
			else
				i = 4;
			for (n = 0; i-- > 0; )
				n = (n * 10) + *++s - '0';
			if (type == 0) {
				if (n >= yynvar)
					goto too_big;
				cp = yysvar[n];
			} else if (n >= yyntoken) {
			    too_big:
				cp = "<range err>";
			} else
				cp = yyTokenTypes[n].name;

			if ((i = strlen(cp)) + buf > endp)
				goto full;
			(void) strcpy(buf, cp);
			buf += i;
		} else
			*buf++ = *s;
	}
	*buf = '\0';
	return 1;
}
#ifndef YYTRACE
/*
 * Show current state of audp_parse
 */
void
yyShowState(tp)
yyTraceItems * tp;
{
	short * p;
	YYSTYPE * q;

	printf(
	    m_textmsg(2828, "state %d (%d), char %s (%d)\n", "I num1 num2 char num3"),
	      yysmap[tp->state], tp->state,
	      yyptok(tp->lookahead), tp->lookahead);
}
/*
 * show results of reduction
 */
void
yyShowReduce(tp)
yyTraceItems * tp;
{
	printf("reduce %d (%d), pops %d (%d)\n",
		yyrmap[tp->rule], tp->rule,
		tp->states[tp->nstates - tp->npop],
		yysmap[tp->states[tp->nstates - tp->npop]]);
}
void
yyShowRead(val)
int val;
{
	printf(m_textmsg(2829, "read %s (%d)\n", "I token num"), yyptok(val), val);
}
void
yyShowGoto(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2830, "goto %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowShift(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2831, "shift %d (%d)\n", "I num1 num2"), yysmap[tp->state], tp->state);
}
void
yyShowErrRecovery(tp)
yyTraceItems * tp;
{
	short	* top = tp->states + tp->nstates - 1;

	printf(
	m_textmsg(2832, "Error recovery pops state %d (%d), uncovers %d (%d)\n", "I num1 num2 num3 num4"),
		yysmap[*top], *top, yysmap[*(top-1)], *(top-1));
}
void
yyShowErrDiscard(tp)
yyTraceItems * tp;
{
	printf(m_textmsg(2833, "Error recovery discards %s (%d), ", "I token num"),
		yyptok(tp->lookahead), tp->lookahead);
}
#endif	/* ! YYTRACE */
#endif	/* YYDEBUG */
