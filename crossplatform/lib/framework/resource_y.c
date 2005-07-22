#include <string.h>

/* yacc -v -D Resource_y.h -p res_ -o Resource_y.c Resource.y */
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
 * program calling res_parse must supply this!
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

/*
 * resource.y
 *
 * Yacc file for parsing RES files
 */

#ifdef PSX
/* A few definitions so the yacc generated code will compile on the PSX.
 * These shouldn't actually be used by any code that is run on the PSX, it
 * just keeps the compiler happy.
 */
static int printf(char* c, ...)
{
	return 0;
}
#endif

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

// directory printfs
#define DEBUG_GROUP0
// file printfs
//#define DEBUG_GROUP1
#include "types.h"
#include "debug.h"
#include "resly.h"
#include "frameresource.h"

typedef union {
	STRING  *sval;
} YYSTYPE;
#define TEXT	257
#define QTEXT	258
#define DIRECTORY	259
#define FILETOKEN	260
extern int res_char, yyerrflag;
extern YYSTYPE res_lval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_sval
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %01 &00",  0},
	{ "%01: %02",  0},
	{ "%01: %01 %02",  0},
	{ "%02: %03",  0},
	{ "%02: %04",  0},
	{ "%03: &04 &03",  0},
	{ "%04: &05 &02 &03",  0},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "TEXT",  257,  1},
	{ "QTEXT",  258,  1},
	{ "DIRECTORY",  259,  0},
	{ "FILETOKEN",  260,  0}

};
#endif
static short yydef[] = {
  -1
};
static short yyex[] = {

	   0,    0, 65535,    1
};
static short yyact[] = {

	65533, 65532,  260,  259, 65534,  257, 65529,  258, 65530,  258,   -1
};
static short yypact[] = {

	   2,    9,    7,    5,    2
};
static short yygo[] = {

	65535, 65527, 65528,    0, 65526, 65525,   -1
};
static short yypgo[] = {

	   0,    0,    0,    5,    4,    0,    0,    2,    2,    0
};
static short yyrlen[] = {

	   0,    0,    0,    3,    2,    1,    2,    1,    1,    2
};
#define YYS0	4
#define YYDELTA	2
#define YYNPACT	5
#define YYNDEF	1

#define YYr7	0
#define YYr8	1
#define YYr9	2
#define YYr6	3
#define YYr5	4
#define YYrACCEPT	YYr7
#define YYrERROR	YYr8
#define YYrLR2	YYr9
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"res_file",
	"res_line",
	"dir_line",
	"file_line",
	0
};
short yyrmap[] = {

	   7,    8,    9,    6,    5,    1,    2,    3,    4,    0
};
short yysmap[] = {

	   6,    7,    2,    1,    0,   10,    8,    5,    9,    4,    3
};
int yyntoken = 6;
int yynvar = 5;
int yynstate = 11;
int yynrule = 10;
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
 *	YYREAD - ensure res_char contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and res_debug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not res_debug.
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
 *		the user explicitly sets res_char = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int res_lex YY_ARGS((void));
extern void res_error YY_ARGS((char *, ...));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print res_lval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and res_debug set,
 * tracing functions will be called at appropriate times in res_parse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, res_parse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = res_char; yyx.errflag =yyerrflag; \
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
#define yyclearin	{ if (res_debug) yyShowRead(-1); res_char = -1; }
#else
#define yyclearin	res_char = -1
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
# define YYREAD	if (res_char < 0) {					\
			if ((res_char = res_lex()) < 0)	{		\
				if (res_char == -2) YYABORT; \
				res_char = 0;				\
			}	/* endif */			\
			if (res_debug)					\
				yyShowRead(res_char);			\
		} else
#else
# define YYREAD	if (res_char < 0) {					\
			if ((res_char = res_lex()) < 0) {			\
				if (res_char == -2) YYABORT; \
				res_char = 0;				\
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
YYSTYPE	res_lval;				/* res_lex() sets this */

int	res_char,				/* current token */
	yyerrflag,			/* error flag */
	yynerrs;			/* error count */

#if YYDEBUG
int res_debug = 0;		/* debug if this flag is set */
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

/*
 * A simple error reporting routine
 */

void res_error(char *pMessage,...)
{
	int		line;
	char	*pText;

	resGetErrorData(&line, &pText);
	DBERROR(("RES file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  pMessage, line, res_char, pText));
}

#ifdef YACC_WINDOWS

/*
 * the following is the res_parse() function that will be
 * callable by a windows type program. It in turn will
 * load all needed resources, obtain pointers to these
 * resources, and call a statically defined function
 * win_yyparse(), which is the original res_parse() fn
 * When win_yyparse() is complete, it will return a
 * value to the new res_parse(), where it will be stored
 * away temporarily, all resources will be freed, and
 * that return value will be given back to the caller
 * res_parse(), as expected.
 */

static int win_yyparse();			/* prototype */

res_parse() 
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
	 * call the official res_parse() function
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
}	/* end res_parse */

static int win_yyparse() 

#else /* YACC_WINDOWS */

/*
 * we are not compiling a windows resource
 * based parser, so call res_parse() the old
 * standard way.
 */

int res_parse()

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
		res_error(m_textmsg(4967, "Not enough space for parser stacks",
				  "E"));
		return 1;
	}
	save_yylval = res_lval;
	save_yyval = yyval;
	save_yypvt = yypvt;
	save_yychar = res_char;
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
			res_error(m_textmsg(4968, "Parser stack overflow", "E"));
			YYABORT;
		} else if (yysinc < 0)		/* binary-exponential */
			yynewsize = yyssize * 2;
		else				/* fixed increment */
			yynewsize = yyssize + yysinc;
		if (yynewsize < yyssize) {
			res_error(m_textmsg(4967,
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
			res_error(m_textmsg(4967, 
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
	}
#else
	if (++yyps > &yys[YYSSIZE]) {
		res_error(m_textmsg(4968, "Parser stack overflow", "E"));
		YYABORT;
	}
#endif /* !YYDYNAMIC */
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (res_debug)
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
			/* Look for a shift on res_char */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = res_char;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = yyneg(YYQYYP);
#if YYDEBUG
				if (res_debug) {
					yyruletype = yyGetType(res_char);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = res_lval;	/* stack what res_lex() set */
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
			while((yyi = *yyp) >= 0 && yyi != res_char)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 m_textmsg(2826, "Ex table not reduce %d\n", "I num"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, m_textmsg(2827, "reduce %d\n", "I num"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (res_debug)
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
		
case YYr5: {	/* dir_line :  DIRECTORY QTEXT */

											UDWORD len;

											// set a new input directory
											DBP0(("directory: %s\n", yypvt[0].sval));
											if (yypvt[0].sval[1] == ':' ||
												yypvt[0].sval[0] == '\\')
											{
												// the new dir is rooted
												strcpy(aCurrResDir, yypvt[0].sval);
											}
											else
											{
												strcpy(aCurrResDir, aResDir);
												strcpy(aCurrResDir + strlen(aResDir),
													   yypvt[0].sval);
											}
											if (strlen(yypvt[0].sval) > 0)
											{
												// Add a trailing '\\'
												len = strlen(aCurrResDir);
												aCurrResDir[len] = '\\';
												aCurrResDir[len+1] = 0;
//												DBP0(("aCurrResDir: %s\n", aCurrResDir));
											}
										
} break;

case YYr6: {	/* file_line :  FILETOKEN TEXT QTEXT */

											
											DBP1(("file: %s %s\n", yypvt[-1].sval, yypvt[0].sval));
											if (!resLoadFile(yypvt[-1].sval, yypvt[0].sval))
											{
												YYABORT;
											}
										
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
	if (res_debug)
		YY_TRACE(yyShowGoto)
#endif
	goto yyStack;

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
		yyi = res_char;
		res_error(m_textmsg(4969, "Syntax error", "E"));
		if (yyi != res_char) {
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
			if (res_debug && yyps > yys+1)
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

		if (res_char == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (res_debug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	res_lval = save_yylval;
	yyval = save_yyval;
	yypvt = save_yypvt;
	res_char = save_yychar;
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
 * Show current state of res_parse
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
