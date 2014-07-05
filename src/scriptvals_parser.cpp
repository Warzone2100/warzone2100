/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2013  Warzone 2100 Project

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         scrv_parse
#define yylex           scrv_lex
#define yyerror         scrv_error
#define yylval          scrv_lval
#define yychar          scrv_char
#define yydebug         scrv_debug
#define yynerrs         scrv_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 20 "scriptvals_parser.ypp"

/*
 * ScriptVals.y
 *
 * yacc grammar for loading script variable values
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"

#include "lib/gamelib/gtime.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"

#include "src/scriptvals.h"
#include "lib/framework/lexer_input.h"

#include "scriptvals_parser.h"

#include "src/scripttabs.h"
#include "src/objects.h"
#include "src/droid.h"
#include "src/structure.h"
#include "src/message.h"
#include "src/levels.h"
#include "src/research.h"
#include "src/text.h"
#include "src/template.h"

// The current script code
static SCRIPT_CODE		*psCurrScript;

extern int scrv_lex(void);
extern void scrv_set_extra(YY_EXTRA_TYPE user_defined);
extern int scrv_lex_destroy(void);
extern int scrv_get_lineno(void);
extern char* scrv_get_text(void);

// The current script context
static SCRIPT_CONTEXT	*psCurrContext;

// the current array indexes
static ARRAY_INDEXES	sCurrArrayIndexes;

// check that an array index is valid
static bool scrvCheckArrayIndex(SDWORD base, ARRAY_INDEXES *psIndexes, UDWORD *pIndex)
{
	SDWORD	i, size;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	if (base < 0 || base >= psCurrScript->numArrays)
	{
		yyerror("Array index out of range");
		return false;
	}

	if (psIndexes->dimensions != psCurrScript->psArrayInfo[base].dimensions)
	{
		yyerror("Invalid number of dimensions for array initialiser");
		return false;
	}

	for(i=0; i<psCurrScript->psArrayInfo[base].dimensions; i++)
	{
		if ((psIndexes->elements[i] < 0) ||
			(psIndexes->elements[i] >= psCurrScript->psArrayInfo[base].elements[i]))
		{
			yyerror("Invalid index for dimension %d", i);
			return false;
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

	return true;
}



/* Line 268 of yacc.c  */
#line 176 "scriptvals_parser.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     BOOLEAN_T = 258,
     INTEGER = 259,
     IDENT = 260,
     QTEXT = 261,
     TYPE = 262,
     VAR = 263,
     ARRAY = 264,
     SCRIPT = 265,
     STORE = 266,
     RUN = 267
   };
#endif
/* Tokens.  */
#define BOOLEAN_T 258
#define INTEGER 259
#define IDENT 260
#define QTEXT 261
#define TYPE 262
#define VAR 263
#define ARRAY 264
#define SCRIPT 265
#define STORE 266
#define RUN 267




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 120 "scriptvals_parser.ypp"

	bool			bval;
	INTERP_TYPE		tval;
	char			*sval;
	UDWORD			vindex;
	SDWORD			ival;
	VAR_INIT		sInit;
	ARRAY_INDEXES	*arrayIndex;



/* Line 293 of yacc.c  */
#line 248 "scriptvals_parser.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 260 "scriptvals_parser.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   33

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  17
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  12
/* YYNRULES -- Number of rules.  */
#define YYNRULES  20
/* YYNRULES -- Number of states.  */
#define YYNSTATES  37

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   267

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    15,     2,    16,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    13,     2,    14,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     8,     9,    16,    17,    25,    28,
      29,    31,    34,    38,    42,    44,    49,    51,    54,    56,
      58
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      18,     0,    -1,    19,    -1,    18,    19,    -1,    -1,    22,
      12,    20,    13,    23,    14,    -1,    -1,    22,    11,     6,
      21,    13,    23,    14,    -1,    10,     6,    -1,    -1,    24,
      -1,    23,    24,    -1,    27,     7,    28,    -1,    15,     4,
      16,    -1,    25,    -1,    26,    15,     4,    16,    -1,     8,
      -1,     9,    26,    -1,     3,    -1,     4,    -1,     6,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   153,   153,   154,   158,   157,   178,   177,   193,   234,
     235,   236,   239,   730,   739,   743,   756,   760,   773,   778,
     783
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "BOOLEAN_T", "INTEGER", "IDENT", "QTEXT",
  "TYPE", "VAR", "ARRAY", "SCRIPT", "STORE", "RUN", "'{'", "'}'", "'['",
  "']'", "$accept", "val_file", "script_entry", "$@1", "$@2",
  "script_name", "var_init_list", "var_init", "array_index",
  "array_index_list", "var_entry", "var_value", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   123,   125,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    17,    18,    18,    20,    19,    21,    19,    22,    23,
      23,    23,    24,    25,    26,    26,    27,    27,    28,    28,
      28
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     0,     6,     0,     7,     2,     0,
       1,     2,     3,     3,     1,     4,     1,     2,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     2,     0,     8,     1,     3,     0,     4,
       6,     0,     0,     9,     9,    16,     0,     0,    10,     0,
       0,     0,    14,    17,     5,    11,     0,     7,     0,     0,
      18,    19,    20,    12,    13,     0,    15
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,     3,    11,    12,     4,    17,    18,    22,    23,
      19,    33
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -8
static const yytype_int8 yypact[] =
{
       8,    11,     0,    -8,     1,    -8,    -8,    -8,    13,    -8,
      -8,     9,    10,     7,     7,    -8,     5,    -7,    -8,    14,
      -5,    20,    -8,    12,    -8,    -8,     2,    -8,    15,    21,
      -8,    -8,    -8,    -8,    -8,    16,    -8
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
      -8,    -8,    24,    -8,    -8,    -8,    19,    -6,    -8,    -8,
      -8,    -8
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
       6,    15,    16,    15,    16,    30,    31,    24,    32,    27,
       1,    25,     8,     9,    25,    15,    16,     5,     1,    10,
      21,    26,    13,    14,    28,    35,     7,    29,     0,     0,
       0,    34,    36,    20
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-8))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int8 yycheck[] =
{
       0,     8,     9,     8,     9,     3,     4,    14,     6,    14,
      10,    17,    11,    12,    20,     8,     9,     6,    10,     6,
      15,     7,    13,    13,     4,     4,     2,    15,    -1,    -1,
      -1,    16,    16,    14
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    10,    18,    19,    22,     6,     0,    19,    11,    12,
       6,    20,    21,    13,    13,     8,     9,    23,    24,    27,
      23,    15,    25,    26,    14,    24,     7,    14,     4,    15,
       3,     4,     6,    28,    16,     4,    16
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

/* Line 1806 of yacc.c  */
#line 158 "scriptvals_parser.ypp"
    {
					if (!eventNewContext(psCurrScript, CR_RELEASE, &psCurrContext))
					{
						yyerror("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext((yyvsp[(1) - (2)].sval), psCurrContext, SCRV_EXEC))
					{
						yyerror("Couldn't store context");
						YYABORT;
					}
				}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 171 "scriptvals_parser.ypp"
    {
					if (!eventRunContext(psCurrContext, gameTime/SCR_TICKRATE))
					{
						YYABORT;
					}
				}
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 178 "scriptvals_parser.ypp"
    {
					if (!eventNewContext(psCurrScript, CR_NORELEASE, &psCurrContext))
					{
						yyerror("Couldn't create context");
						YYABORT;
					}
					if (!scrvAddContext((yyvsp[(3) - (3)].sval), psCurrContext, SCRV_NOEXEC))
					{
						yyerror("Couldn't store context");
						YYABORT;
					}
				}
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 194 "scriptvals_parser.ypp"
    {

					int namelen,extpos;
					char *stringname;

					stringname=(yyvsp[(2) - (2)].sval);

					namelen=strlen( stringname);
					extpos=namelen-3;
					if (strncmp(&stringname[extpos],"blo",3)==0)
					{
						if (resPresent("BLO",stringname)==true)
						{
							psCurrScript = (SCRIPT_CODE*)resGetData("BLO",stringname);
						}
						else
						{
							// change extension to "slo"
							stringname[extpos]='s';
							psCurrScript = (SCRIPT_CODE*)resGetData("SCRIPT",stringname);
						}
					}
					else if (strncmp(&stringname[extpos],"slo",3)==0)
					{
						if (resPresent("SCRIPT",stringname)==true)
						{
							psCurrScript = (SCRIPT_CODE*)resGetData("SCRIPT",stringname);
						}
					}

					if (!psCurrScript)
					{
						yyerror("Script file %s not found", stringname);
						YYABORT;
					}

					(yyval.sval) = (yyvsp[(2) - (2)].sval);
				}
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 240 "scriptvals_parser.ypp"
    {
					INTERP_VAL		data;	/* structure to to hold all types */
					BASE_OBJECT *psObj;
					SDWORD   compIndex;

					/* set type */
					data.type = (yyvsp[(2) - (3)].tval);

					switch ((unsigned)(yyvsp[(2) - (3)].tval))  // Unsigned cast to suppress compiler warnings due to enum abuse.
					{
					case VAL_INT:
						data.v.ival = (yyvsp[(3) - (3)].sInit).index;	//index = integer value of the variable, not var index
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX ||
							!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_DROID:
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)(yyvsp[(3) - (3)].sInit).index);
						if (!psObj)
						{
							yyerror("Droid id %d not found", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}

						data.v.oval = psObj;	/* store as pointer */

						if (psObj->type != OBJ_DROID)
						{
							yyerror("Object id %d is not a droid", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;

					case ST_STRUCTURE:
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)(yyvsp[(3) - (3)].sInit).index);
						if (!psObj)
						{
							yyerror("Structure id %d not found", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}

						data.v.oval = psObj;

						if (psObj->type != OBJ_STRUCTURE)
						{
							yyerror("Object id %d is not a structure", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;
					case ST_FEATURE:
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)(yyvsp[(3) - (3)].sInit).index);
						if (!psObj)
						{
							yyerror("Feature id %d not found", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}

						data.v.oval = psObj;

						if (psObj->type != OBJ_FEATURE)
						{
							yyerror("Object id %d is not a feature", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;
					case ST_FEATURESTAT:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}

						data.v.ival = getFeatureStatFromName((yyvsp[(3) - (3)].sInit).pString);

						if (data.v.ival == -1)
						{
							yyerror("Feature Stat %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case VAL_BOOL:
						data.v.bval = (yyvsp[(3) - (3)].sInit).index;	//index = boolean value, not var index
						if ((yyvsp[(3) - (3)].sInit).type != IT_BOOL ||
							!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_BODY:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_BODY, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("body component %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_PROPULSION:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_PROPULSION, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Propulsion component %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_ECM:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_ECM, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("ECM component %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_SENSOR:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_SENSOR, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Sensor component %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_CONSTRUCT:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_CONSTRUCT, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Construct component %s not found",	(yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_REPAIR:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_REPAIRUNIT, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Repair component %s not found",	(yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_BRAIN:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_BRAIN, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Brain component %s not found",	(yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_WEAPON:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getCompFromName(COMP_WEAPON, (yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Weapon component %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_TEMPLATE:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.oval = getTemplateFromTranslatedNameNoPlayer((yyvsp[(3) - (3)].sInit).pString);	/* store pointer to the template */
						if (data.v.oval == NULL)
						{
							yyerror("Template %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_STRUCTURESTAT:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.ival = getStructStatFromName((yyvsp[(3) - (3)].sInit).pString);
						if (data.v.ival == -1)
						{
							yyerror("Structure Stat %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_STRUCTUREID:
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)(yyvsp[(3) - (3)].sInit).index);
						if (!psObj)
						{
							yyerror("Structure id %d not found", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						data.v.ival = (yyvsp[(3) - (3)].sInit).index;	/* store structure id */
						if (psObj->type != OBJ_STRUCTURE)
						{
							yyerror("Object id %d is not a structure", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;
					case ST_DROIDID:
						if ((yyvsp[(3) - (3)].sInit).type != IT_INDEX)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						psObj = getBaseObjFromId((UDWORD)(yyvsp[(3) - (3)].sInit).index);
						if (!psObj)
						{
							yyerror("Droid id %d not found", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						data.v.ival = (yyvsp[(3) - (3)].sInit).index;	/* store id*/
						if (psObj->type != OBJ_DROID)
						{
							yyerror("Object id %d is not a droid", (UDWORD)(yyvsp[(3) - (3)].sInit).index);
							YYABORT;
						}
						else
						{
							if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;
					case ST_INTMESSAGE:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.oval = getViewData((yyvsp[(3) - (3)].sInit).pString);	/* store pointer to the intelligence message */
						if (data.v.oval == NULL)
						{
							yyerror("Message %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_TEXTSTRING:
					{
						const char* pString;

						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						pString = strresGetString(psStringRes, (yyvsp[(3) - (3)].sInit).pString);
						if (!pString)
						{
							yyerror("Cannot find the string for id \"%s\"", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
						data.v.sval = strdup(pString);
						if (!data.v.sval)
						{
							debug(LOG_ERROR, "Out of memory");
							abort();
							YYABORT;
						}
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					}
					case ST_LEVEL:
						{
							LEVEL_DATASET	*psLevel;

							if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
							{
								yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
							// just check the level exists
							psLevel = levFindDataSet((yyvsp[(3) - (3)].sInit).pString);
							if (psLevel == NULL)
							{
								yyerror("Level %s not found", (yyvsp[(3) - (3)].sInit).pString);
								YYABORT;
							}
							data.v.sval = psLevel->pName; /* store string pointer */
							if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
							{
								yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
								YYABORT;
							}
						}
						break;
					case ST_SOUND:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						/* find audio id */
						compIndex = audio_GetTrackID( (yyvsp[(3) - (3)].sInit).pString );
						if (compIndex == SAMPLE_NOT_FOUND)
						{
							/* set track vals */
							compIndex = audio_SetTrackVals((yyvsp[(3) - (3)].sInit).pString, false, 100, 1800);
						}
						/* save track ID */
						data.v.ival = compIndex;
						if (!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					case ST_RESEARCH:
						if ((yyvsp[(3) - (3)].sInit).type != IT_STRING)
						{
							yyerror("Typemismatch for variable %d", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						data.v.oval = getResearch((yyvsp[(3) - (3)].sInit).pString);	/* store pointer */
#if 0
						if (data.v.oval == NULL)
						{
							yyerror("Research %s not found", (yyvsp[(3) - (3)].sInit).pString);
							YYABORT;
						}
#endif
						if(!eventSetContextVar(psCurrContext, (yyvsp[(1) - (3)].vindex), &data))
						{
							yyerror("Set Value Failed for %u", (yyvsp[(1) - (3)].vindex));
							YYABORT;
						}
						break;
					default:
						yyerror("Unknown type: %s", asTypeTable[(yyvsp[(2) - (3)].tval)].pIdent);
						YYABORT;
						break;
					}
				}
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 731 "scriptvals_parser.ypp"
    {
					sCurrArrayIndexes.dimensions = 1;
					sCurrArrayIndexes.elements[0] = (yyvsp[(2) - (3)].ival);

					(yyval.arrayIndex) = &sCurrArrayIndexes;
				}
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 740 "scriptvals_parser.ypp"
    {
					(yyval.arrayIndex) = (yyvsp[(1) - (1)].arrayIndex);
				}
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 744 "scriptvals_parser.ypp"
    {
					if ((yyvsp[(1) - (4)].arrayIndex)->dimensions >= VAR_MAX_DIMENSIONS)
					{
						yyerror("Too many dimensions for array");
						YYABORT;
					}
					(yyvsp[(1) - (4)].arrayIndex)->elements[(yyvsp[(1) - (4)].arrayIndex)->dimensions] = (yyvsp[(3) - (4)].ival);
					(yyvsp[(1) - (4)].arrayIndex)->dimensions += 1;
				}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 757 "scriptvals_parser.ypp"
    {
					(yyval.vindex) = (yyvsp[(1) - (1)].vindex);
				}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 761 "scriptvals_parser.ypp"
    {
					UDWORD	index;

					if (!scrvCheckArrayIndex((yyvsp[(1) - (2)].vindex), (yyvsp[(2) - (2)].arrayIndex), &index))
					{
						YYABORT;
					}

					(yyval.vindex) = index;
				}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 774 "scriptvals_parser.ypp"
    {
					(yyval.sInit).type = IT_BOOL;
					(yyval.sInit).index = (yyvsp[(1) - (1)].bval);
				}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 779 "scriptvals_parser.ypp"
    {
					(yyval.sInit).type = IT_INDEX;
					(yyval.sInit).index = (yyvsp[(1) - (1)].ival);
				}
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 784 "scriptvals_parser.ypp"
    {
					(yyval.sInit).type = IT_STRING;
					(yyval.sInit).pString = (yyvsp[(1) - (1)].sval);
				}
    break;



/* Line 1806 of yacc.c  */
#line 2181 "scriptvals_parser.cpp"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 790 "scriptvals_parser.ypp"


// Lookup a type
bool scrvLookUpType(const char *pIdent, INTERP_TYPE *pType)
{
	TYPE_SYMBOL		*psCurr;

	for(psCurr = asTypeTable; psCurr->typeID != 0; psCurr++)
	{
		if (strcmp(psCurr->pIdent, pIdent) == 0)
		{
			*pType = psCurr->typeID;
			return true;
		}
	}

	return false;
}


// Lookup a variable identifier
bool scrvLookUpVar(const char *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	for(i=0; i<psCurrScript->numGlobals; i++)
	{
		if (psCurrScript->psVarDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psVarDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return true;
		}
	}

	return false;
}


// Lookup an array identifier
bool scrvLookUpArray(const char *pIdent, UDWORD *pIndex)
{
	UDWORD	i;

	if (!psCurrScript || psCurrScript->psDebug == NULL)
	{
		return false;
	}

	for(i=0; i<psCurrScript->numArrays; i++)
	{
		if (psCurrScript->psArrayDebug[i].pIdent != NULL &&
			strcmp(psCurrScript->psArrayDebug[i].pIdent, pIdent) == 0)
		{
			*pIndex = i;
			return true;
		}
	}

	return false;
}


// Load a script value file
bool scrvLoad(PHYSFS_file* fileHandle)
{
	bool retval;
	lexerinput_t input;

	input.type = LEXINPUT_PHYSFS;
	input.input.physfsfile = fileHandle;

	scrv_set_extra(&input);

	retval = (scrv_parse() == 0);
	scrv_lex_destroy();

	return retval;
}

/* A simple error reporting routine */
void yyerror(const char* fmt, ...)
{
	char* txtBuf;
	size_t size;
	va_list	args;

	va_start(args, fmt);
	size = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	txtBuf = (char *)alloca(size + 1);

	va_start(args, fmt);
	vsprintf(txtBuf, fmt, args);
	va_end(args);

	ASSERT(false, "VLO parse error: %s at line %d, text: '%s'", txtBuf, scrv_get_lineno(), scrv_get_text());
}

