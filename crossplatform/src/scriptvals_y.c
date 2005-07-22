/* d:\mks-ly\mksnt\yacc -p scrv_ -o ScriptVals_y.c -D ScriptVals_y.h .\\ScriptVals.y */
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
 * program calling scrv_parse must supply this!
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
 * ScriptVals.y
 *
 * yacc grammar for loading script variable values
 *
 */

#include <stdio.h>

#include "frame.h"
#include "script.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "objects.h"
#include "gtime.h"
#include "droid.h"
#include "structure.h"
#include "message.h"
#include "audio.h"
#include "levels.h"
#include "research.h"

// The current script code
static SCRIPT_CODE		*psCurrScript;

// The current script context
static SCRIPT_CONTEXT	*psCurrContext;

// the current array indexes
static ARRAY_INDEXES	sCurrArrayIndexes;

// check that an array index is valid
BOOL scrvCheckArrayIndex(SDWORD base, ARRAY_INDEXES *psIndexes, UDWORD *pIndex)
{
	SDWORD	i, size;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return FALSE;
	}

	if (base < 0 || base >= psCurrScript->numArrays)
	{
		scrv_error("Array index out of range");
		return FALSE;
	}

	if (psIndexes->dimensions != psCurrScript->psArrayInfo[base].dimensions)
	{
		scrv_error("Invalid number of dimensions for array initialiser");
		return FALSE;
	}

	for(i=0; i<psCurrScript->psArrayInfo[base].dimensions; i++)
	{
		if ((psIndexes->elements[i] < 0) ||
			(psIndexes->elements[i] >= psCurrScript->psArrayInfo[base].elements[i]))
		{
			scrv_error("Invalid index for dimension %d", i);
			return FALSE;
		}
	}

	*pIndex = 0;
	size = 1;
	for(i = psCurrScript->psArrayInfo[base].dimensions-1; i >= 0; i--)
	{
		*pIndex += psIndexes->elements[i] * size;
		size *= psCurrScript->psArrayInfo[base].elements[i];
	}

	*pIndex += psCurrScript->psArrayInfo[base].base;

	return TRUE;
}

typedef union {
	BOOL			bval;
	INTERP_TYPE		tval;
	STRING			*sval;
	UDWORD			vindex;
	SDWORD			ival;
	VAR_INIT		sInit;
	ARRAY_INDEXES	*arrayIndex;
} YYSTYPE;
#define BOOLEAN	257
#define INTEGER	258
#define IDENT	259
#define QTEXT	260
#define TYPE	261
#define VAR	262
#define ARRAY	263
#define SCRIPT	264
#define STORE	265
#define RUN	266
extern int scrv_char, yyerrflag;
extern YYSTYPE scrv_lval;
#if YYDEBUG
enum YY_Types { YY_t_NoneDefined, YY_t_bval, YY_t_ival, YY_t_sval, YY_t_tval, YY_t_vindex, YY_t_sInit, YY_t_arrayIndex
};
#endif
#if YYDEBUG
yyTypedRules yyRules[] = {
	{ "&00: %06 &00",  0},
	{ "%06: %07",  0},
	{ "%06: %06 %07",  0},
	{ "%08:",  0},
	{ "%07: %01 &11 %08 &12 %09 &13",  0},
	{ "%10:",  0},
	{ "%07: %01 &10 &05 %10 &12 %09 &13",  0},
	{ "%01: &09 &05",  3},
	{ "%09:",  0},
	{ "%09: %11",  0},
	{ "%09: %09 %11",  0},
	{ "%11: %03 &06 %02",  0},
	{ "%04: &14 &03 &15",  7},
	{ "%05: %04",  7},
	{ "%05: %05 &14 &03 &15",  7},
	{ "%03: &07",  5},
	{ "%03: &08 %05",  5},
	{ "%02: &02",  6},
	{ "%02: &03",  6},
	{ "%02: &05",  6},
{ "$accept",  0},{ "error",  0}
};
yyNamedType yyTokenTypes[] = {
	{ "$end",  0,  0},
	{ "error",  256,  0},
	{ "BOOLEAN",  257,  1},
	{ "INTEGER",  258,  2},
	{ "IDENT",  259,  3},
	{ "QTEXT",  260,  3},
	{ "TYPE",  261,  4},
	{ "VAR",  262,  5},
	{ "ARRAY",  263,  5},
	{ "SCRIPT",  264,  0},
	{ "STORE",  265,  0},
	{ "RUN",  266,  0},
	{ "'{'",  123,  0},
	{ "'}'",  125,  0},
	{ "'['",  91,  0},
	{ "']'",  93,  0}

};
#endif
static short yydef[] = {

	65535, 65531, 65527,    3
};
static short yyex[] = {

	   0,    0, 65535,    1,  125,   21, 65535,    1,  125,   21, 
	65535,    1
};
static short yyact[] = {

	65518,  264, 65508,  260, 65520, 65505,  266,  265, 65507,  260, 
	65534,  123, 65533,  123, 65513, 65523,  263,  262, 65527,   91, 
	65528,  261, 65506, 65513, 65523,  263,  262,  125, 65502, 65513, 
	65523,  263,  262,  125, 65529,   91, 65530,  258, 65514, 65515, 
	65516,  260,  258,  257, 65531,  258, 65510,   93, 65512,   93,   -1
};
static short yypact[] = {

	   1,   16,   16,   35,   49,   47,   45,   41,   37,   31, 
	  25,   21,   19,   13,   11,    9,    6,    3,    1
};
static short yygo[] = {

	65519, 65509, 65524, 65511, 65532, 65535, 65503, 65504,    0, 65521, 
	65526, 65525,    2, 65522, 65500, 65500, 65501,   10,    9,   -1
};
static short yypgo[] = {

	   0,    0,    0,    2,    1,    1,    1,    2,    4,    4, 
	   3,   16,    0,   13,    7,    9,    5,    5,    7,   11, 
	  11,   11,    0
};
static short yyrlen[] = {

	   0,    0,    0,    2,    1,    1,    1,    1,    4,    1, 
	   3,    3,    2,    0,    6,    0,    1,    2,    7,    1, 
	   2,    0,    2
};
#define YYS0	18
#define YYDELTA	15
#define YYNPACT	19
#define YYNDEF	4

#define YYr20	0
#define YYr21	1
#define YYr22	2
#define YYr16	3
#define YYr19	4
#define YYr18	5
#define YYr17	6
#define YYr15	7
#define YYr14	8
#define YYr13	9
#define YYr12	10
#define YYr11	11
#define YYr7	12
#define YYr5	13
#define YYr4	14
#define YYr3	15
#define YYrACCEPT	YYr20
#define YYrERROR	YYr21
#define YYrLR2	YYr22
#if YYDEBUG
char * yysvar[] = {
	"$accept",
	"script_name",
	"var_value",
	"var_entry",
	"array_index",
	"array_index_list",
	"val_file",
	"script_entry",
	"$3",
	"var_init_list",
	"$5",
	"var_init",
	0
};
short yyrmap[] = {

	  20,   21,   22,   16,   19,   18,   17,   15,   14,   13, 
	  12,   11,    7,    5,    4,    3,    1,    2,    6,    9, 
	  10,    8,    0
};
short yysmap[] = {

	   4,   12,   13,   20,   33,   28,   27,   23,   22,   19, 
	  18,   16,   14,   11,   10,    6,    2,    1,    0,   29, 
	  30,   31,   15,   35,   21,   34,   32,    5,    9,   25, 
	   7,    3,    8,   26,   17,   24
};
int yyntoken = 16;
int yynvar = 12;
int yynstate = 36;
int yynrule = 23;
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
 *	YYREAD - ensure scrv_char contains a lookahead token by reading
 *		one if it does not.  See also YYSYNC.
 *	YYRECOVERING - 1 if syntax error detected and not recovered
 *		yet; otherwise, 0.
 *
 * Preprocessor flags:
 *	YYDEBUG - includes debug code if 1.  The parser will print
 *		 a travelogue of the parse if this is defined as 1
 *		 and scrv_debug is non-zero.
 *		yacc -t sets YYDEBUG to 1, but not scrv_debug.
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
 *		the user explicitly sets scrv_char = -1
 *
 * Copyright (c) 1983, by the University of Waterloo
 */
/*
 * Prototypes
 */

extern int scrv_lex YY_ARGS((void));
extern void scrv_error YY_ARGS((char *, ...));

#if YYDEBUG

#include <stdlib.h>		/* common prototypes */
#include <string.h>

extern char *	yyValue YY_ARGS((YYSTYPE, int));	/* print scrv_lval */
extern void yyShowState YY_ARGS((yyTraceItems *));
extern void yyShowReduce YY_ARGS((yyTraceItems *));
extern void yyShowGoto YY_ARGS((yyTraceItems *));
extern void yyShowShift YY_ARGS((yyTraceItems *));
extern void yyShowErrRecovery YY_ARGS((yyTraceItems *));
extern void yyShowErrDiscard YY_ARGS((yyTraceItems *));

extern void yyShowRead YY_ARGS((int));
#endif

/*
 * If YYDEBUG defined and scrv_debug set,
 * tracing functions will be called at appropriate times in scrv_parse()
 * Pass state of YACC parse, as filled into yyTraceItems yyx
 * If yyx.done is set by the tracing function, scrv_parse() will terminate
 * with a return value of -1
 */
#define YY_TRACE(fn) { \
	yyx.state = yystate; yyx.lookahead = scrv_char; yyx.errflag =yyerrflag; \
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
#define yyclearin	{ if (scrv_debug) yyShowRead(-1); scrv_char = -1; }
#else
#define yyclearin	scrv_char = -1
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
# define YYREAD	if (scrv_char < 0) {					\
			if ((scrv_char = scrv_lex()) < 0)	{		\
				if (scrv_char == -2) YYABORT; \
				scrv_char = 0;				\
			}	/* endif */			\
			if (scrv_debug)					\
				yyShowRead(scrv_char);			\
		} else
#else
# define YYREAD	if (scrv_char < 0) {					\
			if ((scrv_char = scrv_lex()) < 0) {			\
				if (scrv_char == -2) YYABORT; \
				scrv_char = 0;				\
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
YYSTYPE	scrv_lval;				/* scrv_lex() sets this */

int	scrv_char,				/* current token */
	yyerrflag,			/* error flag */
	yynerrs;			/* error count */

#if YYDEBUG
int scrv_debug = 0;		/* debug if this flag is set */
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

// Lookup a type
BOOL scrvLookUpType(STRING *pIdent, INTERP_TYPE *pType)
{
	TYPE_SYMBOL		*psCurr;

	for(psCurr = asTypeTable; psCurr->typeID != 0; psCurr++)
	{
		if (strcmp(psCurr->pIdent, pIdent) == 0)
		{
			*pType = psCurr->typeID;
			return TRUE;
		}
	}

	return FALSE;
}

// Lookup a variable identifier
BOOL scrvLookUpVar(STRING *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return FALSE;
	}

	for(i=0; i<psCurrScript->numGlobals; i++)
	{
		if (psCurrScript->psVarDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psVarDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return TRUE;
		}
	}

	return FALSE;
}

// Lookup an array identifier
BOOL scrvLookUpArray(STRING *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return FALSE;
	}

	for(i=0; i<psCurrScript->numArrays; i++)
	{
		if (psCurrScript->psArrayDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psArrayDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return TRUE;
		}
	}

	return FALSE;
}

// Load a script value file
BOOL scrvLoad(UBYTE *pData, UDWORD size)
{
	scrvSetInputBuffer(pData, size);

	if (scrv_parse() != 0)
	{
		return FALSE;
	}

	return TRUE;
}

#ifndef FINALBUILD
/* A simple error reporting routine */
void scrv_error(char *pMessage,...)
{
	int		line;
	char	*pText;
	char	aTxtBuf[1024];
	va_list	args;

	va_start(args, pMessage);

	vsprintf(aTxtBuf, pMessage, args);
	scrvGetErrorData(&line, &pText);
	DBERROR(("VLO file parse error:\n%s at line %d\nToken: %d, Text: '%s'\n",
			  aTxtBuf, line, scrv_char, pText));

	va_end(args);
}
#endif

#ifdef YACC_WINDOWS

/*
 * the following is the scrv_parse() function that will be
 * callable by a windows type program. It in turn will
 * load all needed resources, obtain pointers to these
 * resources, and call a statically defined function
 * win_yyparse(), which is the original scrv_parse() fn
 * When win_yyparse() is complete, it will return a
 * value to the new scrv_parse(), where it will be stored
 * away temporarily, all resources will be freed, and
 * that return value will be given back to the caller
 * scrv_parse(), as expected.
 */

static int win_yyparse();			/* prototype */

scrv_parse() 
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
	 * call the official scrv_parse() function
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
}	/* end scrv_parse */

static int win_yyparse() 

#else /* YACC_WINDOWS */

/*
 * we are not compiling a windows resource
 * based parser, so call scrv_parse() the old
 * standard way.
 */

int scrv_parse() 

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
		scrv_error(m_textmsg(4967, "Not enough space for parser stacks",
				  "E"));
		return 1;
	}
	save_yylval = scrv_lval;
	save_yyval = yyval;
	save_yypvt = yypvt;
	save_yychar = scrv_char;
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
			scrv_error(m_textmsg(4968, "Parser stack overflow", "E"));
			YYABORT;
		} else if (yysinc < 0)		/* binary-exponential */
			yynewsize = yyssize * 2;
		else				/* fixed increment */
			yynewsize = yyssize + yysinc;
		if (yynewsize < yyssize) {
			scrv_error(m_textmsg(4967,
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
			scrv_error(m_textmsg(4967, 
					  "Not enough space for parser stacks",
					  "E"));
			YYABORT;
		}
	}
#else
	if (++yyps > &yys[YYSSIZE]) {
		scrv_error(m_textmsg(4968, "Parser stack overflow", "E"));
		YYABORT;
	}
#endif /* !YYDYNAMIC */
	*yyps = yystate;	/* stack current state */
	*++yypv = yyval;	/* ... and value */
#if YYDEBUG
	*++yytp = yyruletype;	/* ... and type */

	if (scrv_debug)
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
			/* Look for a shift on scrv_char */
#ifndef YYSYNC
			YYREAD;
#endif
			yyq = yyp;
			yyi = scrv_char;
			while (yyi < *yyp++)
				;
			if (yyi == yyp[-1]) {
				yystate = yyneg(YYQYYP);
#if YYDEBUG
				if (scrv_debug) {
					yyruletype = yyGetType(scrv_char);
					YY_TRACE(yyShowShift)
				}
#endif
				yyval = scrv_lval;	/* stack what scrv_lex() set */
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
			while((yyi = *yyp) >= 0 && yyi != scrv_char)
				yyp += 2;
			yyi = yyp[1];
			yyassert(yyi >= 0,
				 m_textmsg(2826, "Ex table not reduce %d\n", "I num"), yyi);
		}
	}

	yyassert((unsigned)yyi < yynrule, m_textmsg(2827, "reduce %d\n", "I num"), yyi);
	yyj = yyrlen[yyi];
#if YYDEBUG
	if (scrv_debug)
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
		
case YYr3: {	/* script_entry :  script_name RUN */

					if (!eventNewContext(psCurrScript, CR_RELEASE, &psCurrContext))
					{
						scrv_error("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext(yypvt[-1].sval, psCurrContext, SCRV_EXEC))
					{
						scrv_error("Couldn't store context");
						YYABORT;
					}
				
} break;

case YYr4: {	/* script_entry :  script_name RUN $3 '{' var_init_list '}' */

					if (!eventRunContext(psCurrContext, gameTime/SCR_TICKRATE))
					{
						YYABORT;
					}
				
} break;

case YYr5: {	/* script_entry :  script_name STORE QTEXT */

					if (!eventNewContext(psCurrScript, CR_NORELEASE, &psCurrContext))
					{
						scrv_error("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext(yypvt[0].sval, psCurrContext, SCRV_NOEXEC))
					{
						scrv_error("Couldn't store context");
						YYABORT;
					}
				
} break;

case YYr7: {	/* script_name :  SCRIPT QTEXT */

					int namelen,extpos;
					char *stringname;

					stringname=yypvt[0].sval;

					namelen=strlen( stringname);
					extpos=namelen-3;
					if (strncmp(&stringname[extpos],"blo",3)==0)
					{
						if (resPresent("BLO",stringname)==TRUE)
						{
							psCurrScript=resGetData("BLO",stringname);				
						}
						else
						{
							// change extension to "slo"
							stringname[extpos]='s';
							psCurrScript=resGetData("SCRIPT",stringname);				
						}
					}
					else if (strncmp(&stringname[extpos],"slo",3)==0)
					{
						if (resPresent("SCRIPT",stringname)==TRUE)
						{
							psCurrScript=resGetData("SCRIPT",stringname);				
						}

					}

					if (!psCurrScript)
					{
						scrv_error("Script file not found");
						YYABORT;
					}

					yyval.sval = yypvt[0].sval;
				
} break;

case YYr11: {	/* var_init :  var_entry TYPE var_value */

					BASE_OBJECT		*psObj;
					SDWORD			compIndex;
					DROID_TEMPLATE	*psTemplate;
					VIEWDATA		*psViewData;
					STRING			*pString;
					RESEARCH		*psResearch;

					switch (yypvt[-1].tval)
					{
					case VAL_INT:
						if (yypvt[0].sInit.type != IT_INDEX ||
							!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)yypvt[0].sInit.index))
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_DROID:
						if (yypvt[0].sInit.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetBaseObj((UDWORD)yypvt[0].sInit.index, &psObj))
						{
							scrv_error("Droid id %d not found", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						if (psObj->type != OBJ_DROID)
						{
							scrv_error("Object id %d is not a droid", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psObj))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;

					case ST_STRUCTURE:
						if (yypvt[0].sInit.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetBaseObj((UDWORD)yypvt[0].sInit.index, &psObj))
						{
							scrv_error("Structure id %d not found", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						if (psObj->type != OBJ_STRUCTURE)
						{
							scrv_error("Object id %d is not a structure", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psObj))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;
					case ST_FEATURE:
						if (yypvt[0].sInit.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetBaseObj((UDWORD)yypvt[0].sInit.index, &psObj))
						{
							scrv_error("Feature id %d not found", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						if (psObj->type != OBJ_FEATURE)
						{
							scrv_error("Object id %d is not a feature", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psObj))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;
					case ST_FEATURESTAT:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getFeatureStatFromName(yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Feature Stat %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case VAL_BOOL:
						if (yypvt[0].sInit.type != IT_BOOL ||
							!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)yypvt[0].sInit.index))
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_BODY:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_BODY, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("body component %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_PROPULSION:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_PROPULSION, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Propulsion component %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_ECM:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_ECM, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("ECM component %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_SENSOR:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_SENSOR, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Sensor component %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_CONSTRUCT:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_CONSTRUCT, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Construct component %s not found",	yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_REPAIR:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_REPAIRUNIT, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Repair component %s not found",	yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_BRAIN:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_BRAIN, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Brain component %s not found",	yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_WEAPON:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getCompFromResName(COMP_WEAPON, yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Weapon component %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_TEMPLATE:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						psTemplate = getTemplateFromName(yypvt[0].sInit.pString);
						if (psTemplate == NULL)
						{
							scrv_error("Template %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psTemplate))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_STRUCTURESTAT:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						compIndex = getStructStatFromName(yypvt[0].sInit.pString);
						if (compIndex == -1)
						{
							scrv_error("Structure Stat %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_STRUCTUREID:
						if (yypvt[0].sInit.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetBaseObj((UDWORD)yypvt[0].sInit.index, &psObj))
						{
							scrv_error("Structure id %d not found", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						if (psObj->type != OBJ_STRUCTURE)
						{
							scrv_error("Object id %d is not a structure", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)yypvt[0].sInit.index))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;
					case ST_DROIDID:
						if (yypvt[0].sInit.type != IT_INDEX)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetBaseObj((UDWORD)yypvt[0].sInit.index, &psObj))
						{
							scrv_error("Droid id %d not found", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						if (psObj->type != OBJ_DROID)
						{
							scrv_error("Object id %d is not a droid", (UDWORD)yypvt[0].sInit.index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)yypvt[0].sInit.index))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;
					case ST_INTMESSAGE:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						psViewData = getViewData(yypvt[0].sInit.pString);
						if (psViewData == NULL)
						{
							scrv_error("Message %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psViewData))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_TEXTSTRING:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						if (!scrvGetString(yypvt[0].sInit.pString, &pString))
						{
							scrv_error("String %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)pString))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					case ST_LEVEL:
						{
							LEVEL_DATASET	*psLevel;

							if (yypvt[0].sInit.type != IT_STRING)
							{
								scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
								YYABORT;
							}
							// just check the level exists
							if (!levFindDataSet(yypvt[0].sInit.pString, &psLevel))
							{
								scrv_error("Level %s not found", yypvt[0].sInit.pString);
								YYABORT;
							}
							if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psLevel->pName))
							{
								scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
								YYABORT;
							}
						}
						break;
					case ST_SOUND:

						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						
						compIndex = audio_GetTrackID( yypvt[0].sInit.pString );
						if (compIndex == SAMPLE_NOT_FOUND)
						{
							
							audio_SetTrackVals( yypvt[0].sInit.pString, FALSE, &compIndex, 100, 1, 1800, 0 );
						}
						
						if (!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)compIndex))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}

						break;
					case ST_RESEARCH:
						if (yypvt[0].sInit.type != IT_STRING)
						{
							scrv_error("Typemismatch for variable %d", yypvt[-2].vindex);
							YYABORT;
						}
						psResearch = getResearch(yypvt[0].sInit.pString, TRUE);
						if (psResearch == NULL)
						{
							scrv_error("Research %s not found", yypvt[0].sInit.pString);
							YYABORT;
						}
						if(!eventSetContextVar(psCurrContext, yypvt[-2].vindex, yypvt[-1].tval, (UDWORD)psResearch))
						{
							scrv_error("Set Value Failed for %s", yypvt[-2].vindex);
							YYABORT;
						}
						break;
					default:
						scrv_error("Unknown type: %s", asTypeTable[yypvt[-1].tval].pIdent);
						YYABORT;
						break;
					}
				
} break;

case YYr12: {	/* array_index :  '[' INTEGER ']' */

					sCurrArrayIndexes.dimensions = 1;
					sCurrArrayIndexes.elements[0] = yypvt[-1].ival;

					yyval.arrayIndex = &sCurrArrayIndexes;
				
} break;

case YYr13: {	/* array_index_list :  array_index */

					yyval.arrayIndex = yypvt[0].arrayIndex;
				
} break;

case YYr14: {	/* array_index_list :  array_index_list '[' INTEGER ']' */

					if (yypvt[-3].arrayIndex->dimensions >= VAR_MAX_DIMENSIONS)
					{
						scrv_error("Too many dimensions for array");
						YYABORT;
					}
					yypvt[-3].arrayIndex->elements[yypvt[-3].arrayIndex->dimensions] = yypvt[-1].ival;
					yypvt[-3].arrayIndex->dimensions += 1;
				
} break;

case YYr15: {	/* var_entry :  VAR */

					yyval.vindex = yypvt[0].vindex;
				
} break;

case YYr16: {	/* var_entry :  ARRAY array_index_list */

					UDWORD	index;

					if (!scrvCheckArrayIndex(yypvt[-1].vindex, yypvt[0].arrayIndex, &index))
					{
						YYABORT;
					}

					yyval.vindex = index;
				
} break;

case YYr17: {	/* var_value :  BOOLEAN */

					yyval.sInit.type = IT_BOOL;
					yyval.sInit.index = yypvt[0].bval;
				
} break;

case YYr18: {	/* var_value :  INTEGER */

					yyval.sInit.type = IT_INDEX;
					yyval.sInit.index = yypvt[0].ival;
				
} break;

case YYr19: {	/* var_value :  QTEXT */

					yyval.sInit.type = IT_STRING;
					yyval.sInit.pString = yypvt[0].sval;
				
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
	if (scrv_debug)
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
		yyi = scrv_char;
		scrv_error(m_textmsg(4969, "Syntax error", "E"));
		if (yyi != scrv_char) {
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
			if (scrv_debug && yyps > yys+1)
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

		if (scrv_char == 0)  /* but not EOF */
			break;
#if YYDEBUG
		if (scrv_debug)
			YY_TRACE(yyShowErrDiscard)
#endif
		yyclearin;
		goto yyEncore;	/* try again in same state */
	}
	YYABORT;

#ifdef YYALLOC
yyReturn:
	scrv_lval = save_yylval;
	yyval = save_yyval;
	yypvt = save_yypvt;
	scrv_char = save_yychar;
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
 * Show current state of scrv_parse
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
