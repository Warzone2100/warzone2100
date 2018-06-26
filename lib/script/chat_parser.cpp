/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#if defined( _MSC_VER )
	#pragma warning( disable : 4702 ) // warning C4702: unreachable code
#endif

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
#define yyparse         chat_parse
#define yylex           chat_lex
#define yyerror         chat_error
#define yylval          chat_lval
#define yychar          chat_char
#define yydebug         chat_debug
#define yynerrs         chat_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 21 "chat_parser.ypp"

/*
 * chat_parser.y
 *
 * yacc grammar for multiplayer chat messages
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"

#include "lib/framework/frameresource.h"
#include "lib/script/chat_processing.h"

#define MAX_CHAT_ARGUMENTS 10

/* Holds information about a processed player chat message */
CHAT_MSG chat_msg;

// Parameter variable
static INTERP_VAL scrParameter;

extern int chat_lex(void);

// Store extracted command for use in scripts
static void chat_store_command(const char *command);

/* Return value of the chat parsing - to be used in scripts */
static bool chat_store_parameter(INTERP_VAL *parameter);

// Store players that were addressed in a command
static void chat_store_player(SDWORD cmdIndex, SDWORD playerIndex);

// Reset a command
static void chat_reset_command(SDWORD cmdIndex);

// Information extracted from message and available for scripts
//static int					numMsgParams=0;		//number of parameters currently extracted/stored
//static INTERP_VAL		msgParams[MAX_CHAT_ARGUMENTS];

/* Store command parameter extracted from the message */
static bool chat_store_parameter(INTERP_VAL *cmdParam)
{
	SDWORD	numCmdParams, numCommands;

	//console("chat_store_parameter: new parameter");

	/* Make sure we have no overflow */
	//if(numMsgParams >= MAX_CHAT_ARGUMENTS)
	if(chat_msg.numCommands >= MAX_CHAT_COMMANDS)
	{
		ASSERT(false, "chat_store_parameter: too many commands in a message");
		return false;
	}

	numCommands = chat_msg.numCommands;
	numCmdParams = chat_msg.cmdData[numCommands].numCmdParams;

	/* Make sure we still have room for more parameters */
	if(numCmdParams >= MAX_CHAT_CMD_PARAMS)
	{
		ASSERT(false, "chat_store_parameter: out of parameters for command %d", numCommands);
		return false;
	}

	/* Store parameter for command we are currently processing */
	//memcpy(&(msgParams[numMsgParams]), parameter, sizeof(INTERP_VAL));
	memcpy(&(chat_msg.cmdData[numCommands].parameter[numCmdParams]), cmdParam, sizeof(INTERP_VAL));

	chat_msg.cmdData[numCommands].numCmdParams++;

	return true;
}

// Store extracted command for use in scripts
static void chat_store_command(const char *command)
{
	SDWORD	numCmdParams, numCommands;

	//console("chat_store_command: new command: %s", command);

	/* Make sure we have no overflow */
	if(chat_msg.numCommands >= MAX_CHAT_COMMANDS)
	{
		ASSERT(false, "chat_store_command: too many commands in a message");
		return;
	}

	numCommands = chat_msg.numCommands;
	numCmdParams = chat_msg.cmdData[numCommands].numCmdParams;

	/* Make sure we still have room for more parameters */
	if(numCmdParams >= MAX_CHAT_CMD_PARAMS)
	{
		ASSERT(false, "chat_store_command: out of parameters for command %d", numCommands);
		return;
	}

	/* Store command */
	chat_msg.cmdData[numCommands].pCmdDescription = command;

	chat_msg.numCommands++;
}

// Store players that were addressed in a command
static void chat_store_player(SDWORD cmdIndex, SDWORD playerIndex)
{
	SDWORD i;

	//console("chat_store_player: player %d addressd in command %d", playerIndex, cmdIndex);

	/* Make sure we have no overflow */
	if(cmdIndex < 0 || cmdIndex >= MAX_CHAT_COMMANDS)
	{
		ASSERT(false, "chat_store_player: command message out of bounds: %d", cmdIndex);
		return;
	}

	if(playerIndex == -1)	//no player specified means all players addressed
	{
		/* Ally players addressed */
		for(i=0; i<MAX_PLAYERS; i++)
		{
			chat_msg.cmdData[cmdIndex].bPlayerAddressed[i] = true;
		}
	}
	else if(playerIndex >= 0 && playerIndex < MAX_PLAYERS)
	{
		chat_msg.cmdData[cmdIndex].bPlayerAddressed[playerIndex] = true;
	}
	else	/* Wrong player index */
	{
		ASSERT(false, "chat_store_player: wrong player index: %d", playerIndex);
		return;
	}
}

static void chat_reset_command(SDWORD cmdIndex)
{
	SDWORD i;

	ASSERT(cmdIndex >= 0 && cmdIndex < MAX_CHAT_COMMANDS,
		"chat_reset_command: command index out of bounds: %d", cmdIndex);

	chat_msg.cmdData[cmdIndex].numCmdParams = 0;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		chat_msg.cmdData[cmdIndex].bPlayerAddressed[i] = false;
	}
}

static void yyerror(const char* msg);



/* Line 268 of yacc.c  */
#line 235 "chat_parser.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
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
     _T_EOF = 0,
     BRACKET_OPEN = 258,
     BRACKET_CLOSE = 259,
     SQ_BRACKET_OPEN = 260,
     SQ_BRACKET_CLOSE = 261,
     PIPE = 262,
     T_WORD = 263,
     R_POSSESSION = 264,
     _T_QM = 265,
     _T_EM = 266,
     _T_FULLSTOP = 267,
     _T_COLON = 268,
     _T_SEMICOLON = 269,
     _T_COMMA = 270,
     _T_A = 271,
     _T_AFFIRMATIVE = 272,
     _T_AFTER = 273,
     _T_ALLY = 274,
     _T_AM = 275,
     _T_AN = 276,
     _T_AND = 277,
     _T_ANY = 278,
     _T_ATTACK = 279,
     _T_ATTACKING = 280,
     _T_ARMY = 281,
     _T_BEACON = 282,
     _T_BUILDING = 283,
     _T_CANT = 284,
     _T_CENTER = 285,
     _T_DEAD = 286,
     _T_DERRICK = 287,
     _T_DO = 288,
     _T_DROP = 289,
     _T_FINE = 290,
     _T_FOR = 291,
     _T_FORCE = 292,
     _T_GET = 293,
     _T_GETTING = 294,
     _T_GO = 295,
     _T_GOING = 296,
     _T_GONNA = 297,
     _T_GOT = 298,
     _T_GREAT = 299,
     _T_HAVE = 300,
     _T_HAS = 301,
     _T_HELP = 302,
     _T_HOLD = 303,
     _T_I = 304,
     _T_IM = 305,
     _T_IS = 306,
     _T_LASSAT = 307,
     _T_LETS = 308,
     _T_ME = 309,
     _T_MORE = 310,
     _T_NEED = 311,
     _T_NO = 312,
     _T_NOW = 313,
     _T_OFCOURSE = 314,
     _T_OK = 315,
     _T_ON = 316,
     _T_PLACE = 317,
     _T_POSSESSION = 318,
     _T_POWER = 319,
     _T_PUMPING = 320,
     _T_PUT = 321,
     _T_READY = 322,
     _T_REQUIRE = 323,
     _T_ROGER = 324,
     _T_RUSH = 325,
     _T_SEC = 326,
     _T_SEE = 327,
     _T_SOME = 328,
     _T_STATUS = 329,
     _T_STOP = 330,
     _T_SURE = 331,
     _T_THANK_YOU = 332,
     _T_THANKS = 333,
     _T_THE = 334,
     _T_U = 335,
     _T_UNITS = 336,
     _T_VTOLS = 337,
     _T_WAIT = 338,
     _T_WHERE = 339,
     _T_YES = 340,
     _T_YOU = 341,
     R_PLAYER = 342,
     R_INTEGER = 343
   };
#endif
/* Tokens.  */
#define _T_EOF 0
#define BRACKET_OPEN 258
#define BRACKET_CLOSE 259
#define SQ_BRACKET_OPEN 260
#define SQ_BRACKET_CLOSE 261
#define PIPE 262
#define T_WORD 263
#define R_POSSESSION 264
#define _T_QM 265
#define _T_EM 266
#define _T_FULLSTOP 267
#define _T_COLON 268
#define _T_SEMICOLON 269
#define _T_COMMA 270
#define _T_A 271
#define _T_AFFIRMATIVE 272
#define _T_AFTER 273
#define _T_ALLY 274
#define _T_AM 275
#define _T_AN 276
#define _T_AND 277
#define _T_ANY 278
#define _T_ATTACK 279
#define _T_ATTACKING 280
#define _T_ARMY 281
#define _T_BEACON 282
#define _T_BUILDING 283
#define _T_CANT 284
#define _T_CENTER 285
#define _T_DEAD 286
#define _T_DERRICK 287
#define _T_DO 288
#define _T_DROP 289
#define _T_FINE 290
#define _T_FOR 291
#define _T_FORCE 292
#define _T_GET 293
#define _T_GETTING 294
#define _T_GO 295
#define _T_GOING 296
#define _T_GONNA 297
#define _T_GOT 298
#define _T_GREAT 299
#define _T_HAVE 300
#define _T_HAS 301
#define _T_HELP 302
#define _T_HOLD 303
#define _T_I 304
#define _T_IM 305
#define _T_IS 306
#define _T_LASSAT 307
#define _T_LETS 308
#define _T_ME 309
#define _T_MORE 310
#define _T_NEED 311
#define _T_NO 312
#define _T_NOW 313
#define _T_OFCOURSE 314
#define _T_OK 315
#define _T_ON 316
#define _T_PLACE 317
#define _T_POSSESSION 318
#define _T_POWER 319
#define _T_PUMPING 320
#define _T_PUT 321
#define _T_READY 322
#define _T_REQUIRE 323
#define _T_ROGER 324
#define _T_RUSH 325
#define _T_SEC 326
#define _T_SEE 327
#define _T_SOME 328
#define _T_STATUS 329
#define _T_STOP 330
#define _T_SURE 331
#define _T_THANK_YOU 332
#define _T_THANKS 333
#define _T_THE 334
#define _T_U 335
#define _T_UNITS 336
#define _T_VTOLS 337
#define _T_WAIT 338
#define _T_WHERE 339
#define _T_YES 340
#define _T_YOU 341
#define R_PLAYER 342
#define R_INTEGER 343




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 178 "chat_parser.ypp"

	int32_t				bval;
	SDWORD			ival;



/* Line 293 of yacc.c  */
#line 456 "chat_parser.cpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 468 "chat_parser.cpp"

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
#define YYFINAL  121
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   512

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  89
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  48
/* YYNRULES -- Number of rules.  */
#define YYNRULES  135
/* YYNRULES -- Number of states.  */
#define YYNSTATES  207

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   343

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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    13,    15,    19,    23,
      27,    30,    32,    34,    36,    38,    40,    42,    44,    46,
      48,    50,    52,    54,    56,    58,    60,    62,    64,    66,
      68,    70,    72,    75,    79,    83,    86,    87,    89,    91,
      93,    95,    97,   100,   103,   104,   106,   108,   110,   112,
     114,   116,   118,   120,   122,   124,   127,   129,   131,   133,
     135,   137,   139,   141,   143,   145,   147,   149,   152,   154,
     156,   158,   160,   162,   164,   168,   171,   174,   176,   178,
     180,   182,   184,   186,   188,   190,   192,   194,   196,   198,
     200,   203,   207,   211,   214,   217,   220,   224,   227,   231,
     235,   238,   241,   244,   248,   253,   257,   260,   264,   268,
     271,   274,   278,   283,   285,   287,   291,   295,   299,   303,
     308,   312,   317,   321,   324,   328,   332,   336,   340,   344,
     349,   355,   359,   362,   365,   370
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      90,     0,    -1,    91,    -1,    92,    -1,    90,    91,    -1,
      90,    92,    -1,     1,    -1,    93,    13,    92,    -1,    93,
      14,    92,    -1,    93,    15,    92,    -1,    93,    92,    -1,
     116,    -1,   117,    -1,   118,    -1,   119,    -1,   120,    -1,
     121,    -1,   123,    -1,   124,    -1,   125,    -1,   126,    -1,
     128,    -1,   129,    -1,   130,    -1,   131,    -1,   132,    -1,
     133,    -1,   134,    -1,   135,    -1,   136,    -1,   115,    -1,
      87,    -1,    93,    87,    -1,    93,    15,    87,    -1,    93,
      22,    87,    -1,    87,     9,    -1,    -1,    16,    -1,    21,
      -1,    79,    -1,    10,    -1,    11,    -1,    96,    10,    -1,
      96,    11,    -1,    -1,     0,    -1,    12,    -1,    11,    -1,
      97,    -1,    10,    -1,    97,    -1,    98,    -1,    96,    -1,
      25,    -1,    41,    -1,    41,    18,    -1,    24,    -1,    38,
      -1,    66,    -1,    34,    -1,    62,    -1,    65,    -1,    39,
      -1,    28,    -1,    86,    -1,    80,    -1,   105,    -1,    33,
     105,    -1,    45,    -1,    43,    -1,    46,    -1,    43,    -1,
      23,    -1,    73,    -1,   106,   107,   109,    -1,   106,   107,
      -1,   107,   109,    -1,   107,    -1,   109,    -1,    76,    -1,
      59,    -1,    85,    -1,    35,    -1,    60,    -1,   112,    -1,
     111,    -1,    69,    -1,    17,    -1,    56,    -1,    68,    -1,
      70,   100,    -1,    53,    70,   100,    -1,    19,    54,   100,
      -1,    19,   100,    -1,    40,    10,    -1,    67,   100,    -1,
      50,    67,   100,    -1,    40,    98,    -1,   103,    95,    27,
      -1,    40,    30,   100,    -1,    74,   100,    -1,    95,    37,
      -1,    95,    26,    -1,   104,    81,    98,    -1,   104,    55,
      81,    98,    -1,   104,   122,    98,    -1,    75,    98,    -1,
     110,    64,    99,    -1,    47,    54,   100,    -1,    47,   100,
      -1,    49,   126,    -1,   114,    47,   100,    -1,   114,   109,
      47,   100,    -1,    77,    -1,    78,    -1,    50,    60,    98,
      -1,    50,    35,    98,    -1,   128,    58,    98,    -1,   128,
     127,    98,    -1,   128,    15,   127,    98,    -1,   127,   128,
      98,    -1,   127,    15,   128,    98,    -1,   127,   127,    98,
      -1,   113,    98,    -1,   129,   113,    98,    -1,    19,    87,
      98,    -1,   102,    87,    98,    -1,    40,    87,    98,    -1,
     101,    87,    98,    -1,    87,   108,    82,    98,    -1,    42,
     102,    94,    32,    98,    -1,    52,    87,    98,    -1,    83,
      98,    -1,    71,    98,    -1,    83,    36,    54,    98,    -1,
      48,    61,    98,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   288,   288,   289,   293,   294,   298,   309,   311,   313,
     315,   327,   331,   335,   339,   343,   347,   351,   355,   359,
     363,   367,   371,   375,   379,   383,   387,   391,   395,   399,
     403,   411,   415,   419,   423,   429,   437,   438,   439,   440,
     444,   445,   446,   447,   453,   454,   455,   461,   462,   468,
     469,   476,   477,   481,   482,   483,   487,   488,   491,   491,
     491,   493,   494,   495,   498,   499,   503,   504,   508,   509,
     512,   513,   517,   518,   521,   522,   523,   524,   525,   528,
     529,   532,   533,   534,   537,   538,   539,   540,   544,   545,
     548,   549,   558,   559,   566,   567,   568,   572,   575,   578,
     581,   584,   585,   589,   590,   591,   595,   598,   601,   602,
     603,   604,   605,   608,   609,   613,   614,   615,   616,   617,
     618,   619,   620,   625,   626,   631,   644,   654,   667,   680,
     692,   704,   716,   717,   718,   719
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "_T_EOF", "error", "$undefined", "BRACKET_OPEN", "BRACKET_CLOSE",
  "SQ_BRACKET_OPEN", "SQ_BRACKET_CLOSE", "PIPE", "T_WORD", "R_POSSESSION",
  "_T_QM", "_T_EM", "_T_FULLSTOP", "_T_COLON", "_T_SEMICOLON", "_T_COMMA",
  "_T_A", "_T_AFFIRMATIVE", "_T_AFTER", "_T_ALLY", "_T_AM", "_T_AN",
  "_T_AND", "_T_ANY", "_T_ATTACK", "_T_ATTACKING", "_T_ARMY", "_T_BEACON",
  "_T_BUILDING", "_T_CANT", "_T_CENTER", "_T_DEAD", "_T_DERRICK", "_T_DO",
  "_T_DROP", "_T_FINE", "_T_FOR", "_T_FORCE", "_T_GET", "_T_GETTING",
  "_T_GO", "_T_GOING", "_T_GONNA", "_T_GOT", "_T_GREAT", "_T_HAVE",
  "_T_HAS", "_T_HELP", "_T_HOLD", "_T_I", "_T_IM", "_T_IS", "_T_LASSAT",
  "_T_LETS", "_T_ME", "_T_MORE", "_T_NEED", "_T_NO", "_T_NOW",
  "_T_OFCOURSE", "_T_OK", "_T_ON", "_T_PLACE", "_T_POSSESSION", "_T_POWER",
  "_T_PUMPING", "_T_PUT", "_T_READY", "_T_REQUIRE", "_T_ROGER", "_T_RUSH",
  "_T_SEC", "_T_SEE", "_T_SOME", "_T_STATUS", "_T_STOP", "_T_SURE",
  "_T_THANK_YOU", "_T_THANKS", "_T_THE", "_T_U", "_T_UNITS", "_T_VTOLS",
  "_T_WAIT", "_T_WHERE", "_T_YES", "_T_YOU", "R_PLAYER", "R_INTEGER",
  "$accept", "R_PHRASE", "R_ADDRESSED_COMMAND", "R_COMMAND",
  "R_PLAYER_LIST", "R_PLAYER_POSSESSION", "R_A_OR_EMPTY",
  "R_PUNCTUATION_MARK", "R_FULLSTOP_OR_EOF", "R_EOD", "R_EOQ", "R_EOS",
  "R_ATTACKING", "R_INITIATE_ATTACK", "R_PUT_DOWN", "R_CONSTRUCTING",
  "R_YOU", "R_DO_YOU", "R_POSSESSION_Q", "R_POSSESSES", "R_QUANTITY",
  "R_DO_YOU_HAVE_ANY", "R_CONFIDENCE_EXPRESSION", "R_AGREEMENT_EXPRESSION",
  "R_AFFIRMATIVE_FORMS", "R_NEED", "R_RUSH_INITIATION", "R_ALLY_OFFER",
  "R_ASK_READINESS", "R_INITIATE_ACTION", "R_DEMAND_BEACON",
  "R_MEET_CENTER", "R_ASK_STATUS", "R_UNIT_ARMY", "R_BUILDING_UNITS",
  "R_STOP", "R_WONDER_IF_HAVE_POWER", "R_DEMAND_HELP", "R_GRATITUDE",
  "R_REPORT_SAFETY", "R_AFFIRMATIVE", "R_ALLY_PLAYER", "R_ATTACK_PLAYER",
  "R_ATTACKING_PLAYER", "R_PLAYER_HAS_VTOLS", "R_GETTING_ENEMY_DERRICK",
  "R_LASSAT_PLAYER", "R_WAIT_FOR_ME", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    89,    90,    90,    90,    90,    90,    91,    91,    91,
      91,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      92,    93,    93,    93,    93,    94,    95,    95,    95,    95,
      96,    96,    96,    96,    97,    97,    97,    98,    98,    99,
      99,   100,   100,   101,   101,   101,   102,   102,   103,   103,
     103,   104,   104,   104,   105,   105,   106,   106,   107,   107,
     108,   108,   109,   109,   110,   110,   110,   110,   110,   111,
     111,   112,   112,   112,   113,   113,   113,   113,   114,   114,
     115,   115,   116,   116,   117,   117,   117,   118,   119,   120,
     121,   122,   122,   123,   123,   123,   124,   125,   126,   126,
     126,   126,   126,   127,   127,   128,   128,   128,   128,   128,
     128,   128,   128,   129,   129,   130,   131,   131,   132,   133,
     134,   135,   136,   136,   136,   136
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     2,     1,     3,     3,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     3,     3,     2,     0,     1,     1,     1,
       1,     1,     2,     2,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     3,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     3,     3,     2,     2,     2,     3,     2,     3,     3,
       2,     2,     2,     3,     4,     3,     2,     3,     3,     2,
       2,     3,     4,     1,     1,     3,     3,     3,     3,     4,
       3,     4,     3,     2,     3,     3,     3,     3,     3,     4,
       5,     3,     2,     2,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     6,    87,    44,    72,    56,    53,    63,     0,    59,
      82,    57,    62,    44,    54,     0,    69,    68,    44,     0,
       0,     0,     0,     0,    88,    80,    83,    60,    61,    58,
      44,    89,    86,    44,    44,    73,    44,    44,    79,   113,
     114,    65,    44,    81,    64,    31,     0,     2,     3,     0,
       0,     0,    36,    36,    66,     0,    77,    78,     0,    85,
      84,    44,     0,    30,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    45,    40,    41,    46,    44,    44,
      52,    48,    51,    93,    67,    94,    47,    44,    44,    97,
      55,     0,    44,   109,    44,   110,    44,    44,    44,    44,
      44,    95,    90,   133,   100,   106,     0,   132,    71,    70,
       0,     1,     4,     5,     0,     0,     0,     0,    32,    10,
      44,    44,    37,    38,    39,     0,     0,    44,     0,    44,
      75,    76,    44,   123,    44,     0,     0,     0,    44,    44,
       0,    44,    44,    44,    92,   125,    42,    43,    99,   127,
       0,     0,   108,   135,   116,   115,    96,   131,    91,    44,
      44,     0,     7,     8,    33,     9,    34,   128,   126,    98,
      44,   103,   102,   101,   105,    74,    49,    50,   107,   111,
      44,    44,   122,   120,    44,   117,   118,   124,    35,    44,
     134,   129,   104,   112,   121,   119,   130
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    46,    47,    48,    49,   161,   135,    90,    91,    92,
     188,    93,    50,    51,    52,    53,    54,    55,    56,   120,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,   139,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -70
static const yytype_int16 yypact[] =
{
     220,   -70,   -70,     1,   -70,   -70,   -70,   -70,   -29,   -70,
     -70,   -70,   -70,    22,    -1,   -22,   -70,   -70,    35,   -24,
     103,    -4,   -43,    20,   -70,   -70,   -70,   -70,   -70,   -70,
     151,   -70,   -70,   151,    99,   -70,   151,    99,   -70,   -70,
     -70,   -70,    95,   -70,   -70,    28,   149,   -70,   -70,   295,
      10,    33,    -7,    -6,   -70,   -15,   -20,   -70,    57,   -70,
     -70,    99,    13,   -70,   -70,   -70,   -70,   -70,   -70,   -70,
     -70,   -70,   -70,   -70,    52,    55,    84,   -70,   -70,   -70,
     -70,   -70,   -70,   -70,   -70,   -70,   -70,   -70,   151,    99,
      48,   -70,   -70,   -70,   -70,   -70,   -70,   151,    99,   -70,
     -70,    36,   151,   -70,    99,   -70,    99,    99,   151,    99,
     151,   -70,   -70,   -70,   -70,   -70,    71,   -70,   -70,   -70,
      59,   -70,   -70,   -70,   360,   360,   425,    58,    28,   -70,
      99,    99,   -70,   -70,   -70,   115,    65,    99,   111,    99,
     -20,   -70,    54,   -70,   151,   100,    -9,   -17,    27,    50,
      38,    99,    99,    99,   -70,   -70,   -70,   -70,   -70,   -70,
     145,   123,   -70,   -70,   -70,   -70,   -70,   -70,   -70,    99,
      99,    28,   -70,   -70,    28,   -70,   -70,   -70,   -70,   -70,
      99,   -70,   -70,   -70,   -70,   -70,   -70,   -70,   -70,   -70,
     151,    50,   -70,   -70,    99,   -70,   -70,   -70,   -70,    99,
     -70,   -70,   -70,   -70,   -70,   -70,   -70
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -70,   -70,   112,   -42,   -70,   -70,   117,   -70,    23,   -13,
     -70,   -10,   -70,   160,   -70,   -70,   156,   -70,   121,   -70,
     -37,   -70,   -70,   -70,   109,   -70,   -70,   -70,   -70,   -70,
     -70,   -70,   -70,   -70,   -70,   -70,   -70,   159,   -69,   -34,
     -70,   -70,   -70,   -70,   -70,   -70,   -70,   -70
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      99,    84,     5,     4,   123,   148,   152,   129,   103,   132,
     132,    85,    86,    87,   133,   133,    11,   100,   106,   141,
     111,   113,    84,   112,   115,   145,   114,    84,    16,   117,
      17,   106,    95,    96,    87,    84,     4,   104,    96,    87,
     149,   147,   146,   107,   109,    85,    86,    87,   143,   136,
      84,    41,    97,    35,    84,    88,   107,    44,   156,   157,
     144,    96,    87,   108,   186,   150,    87,   146,    39,    40,
     150,   118,   134,   134,   119,   137,   155,   147,   154,   148,
     152,   194,   172,   173,   175,   159,    35,   158,    89,   102,
     110,   163,   162,   164,   165,    84,   167,   130,   166,    84,
     168,     2,   147,   185,    39,    40,    96,    87,   151,    98,
      96,    87,   191,   151,   149,    39,    40,   177,   178,    10,
     131,   142,   152,   160,   181,   169,   184,    39,    40,    39,
      40,   116,    39,    40,   189,   192,   193,   182,   195,   196,
     197,   170,   179,    25,    26,   176,   180,   190,   183,   121,
      18,    84,    20,    32,   198,   199,   200,   201,   122,    24,
      38,    85,    86,    87,    94,   187,     2,   202,     3,    43,
     138,    31,     4,     5,     6,   101,   140,     7,   204,   105,
     203,   205,     8,     9,    10,   153,   206,    11,    12,    13,
      14,    15,    16,     0,    17,     0,    18,    19,    20,    21,
       0,    22,    23,     0,     0,    24,     0,     0,    25,    26,
       0,    27,     0,     0,    28,    29,    30,    31,    32,    33,
      34,     1,    35,    36,    37,    38,    39,    40,     0,    41,
       0,     0,    42,     0,    43,    44,    45,     2,     0,     3,
       0,     0,     0,     4,     5,     6,     0,     0,     7,     0,
       0,     0,     0,     8,     9,    10,     0,     0,    11,    12,
      13,    14,    15,    16,     0,    17,     0,    18,    19,    20,
      21,     0,    22,    23,     0,     0,    24,     0,     0,    25,
      26,     0,    27,     0,     0,    28,    29,    30,    31,    32,
      33,    34,     0,    35,    36,    37,    38,    39,    40,     0,
      41,     0,     0,    42,     0,    43,    44,    45,   124,   125,
     126,     0,     2,     0,     3,     0,     0,   127,     4,     5,
       6,     0,     0,     7,     0,     0,     0,     0,     8,     9,
      10,     0,     0,    11,    12,    13,    14,    15,    16,     0,
      17,     0,    18,    19,    20,    21,     0,    22,    23,     0,
       0,    24,     0,     0,    25,    26,     0,    27,     0,     0,
      28,    29,    30,    31,    32,    33,    34,     0,    35,    36,
      37,    38,    39,    40,     0,    41,     0,     2,    42,     3,
      43,    44,   128,     4,     5,     6,     0,     0,     7,     0,
       0,     0,     0,     8,     9,    10,     0,     0,    11,    12,
      13,    14,    15,    16,     0,    17,     0,    18,    19,    20,
      21,     0,    22,    23,     0,     0,    24,     0,     0,    25,
      26,     0,    27,     0,     0,    28,    29,    30,    31,    32,
      33,    34,     0,    35,    36,    37,    38,    39,    40,     0,
      41,     0,     2,    42,     3,    43,    44,   171,     4,     5,
       6,     0,     0,     7,     0,     0,     0,     0,     8,     9,
      10,     0,     0,    11,    12,    13,    14,    15,    16,     0,
      17,     0,    18,    19,    20,    21,     0,    22,    23,     0,
       0,    24,     0,     0,    25,    26,     0,    27,     0,     0,
      28,    29,    30,    31,    32,    33,    34,     0,    35,    36,
      37,    38,    39,    40,     0,    41,     0,     0,    42,     0,
      43,    44,   174
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-70))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      13,     0,    24,    23,    46,    74,    75,    49,    18,    16,
      16,    10,    11,    12,    21,    21,    38,    18,    35,    56,
      30,    34,     0,    33,    37,    62,    36,     0,    43,    42,
      45,    35,    10,    11,    12,     0,    23,    61,    11,    12,
      74,    50,    15,    60,    87,    10,    11,    12,    61,    55,
       0,    80,    30,    73,     0,    54,    60,    86,    10,    11,
      47,    11,    12,    67,    10,    15,    12,    15,    77,    78,
      15,    43,    79,    79,    46,    81,    89,    50,    88,   148,
     149,   150,   124,   125,   126,    98,    73,    97,    87,    54,
      70,   104,   102,   106,   107,     0,   109,    87,   108,     0,
     110,    17,    50,   140,    77,    78,    11,    12,    58,    87,
      11,    12,   146,    58,   148,    77,    78,   130,   131,    35,
      87,    64,   191,    87,   137,    54,   139,    77,    78,    77,
      78,    36,    77,    78,   144,   148,   149,    26,   151,   152,
     153,    82,    27,    59,    60,    87,    81,    47,    37,     0,
      47,     0,    49,    69,     9,    32,   169,   170,    46,    56,
      76,    10,    11,    12,     8,   142,    17,   180,    19,    85,
      53,    68,    23,    24,    25,    15,    55,    28,   191,    20,
     190,   194,    33,    34,    35,    76,   199,    38,    39,    40,
      41,    42,    43,    -1,    45,    -1,    47,    48,    49,    50,
      -1,    52,    53,    -1,    -1,    56,    -1,    -1,    59,    60,
      -1,    62,    -1,    -1,    65,    66,    67,    68,    69,    70,
      71,     1,    73,    74,    75,    76,    77,    78,    -1,    80,
      -1,    -1,    83,    -1,    85,    86,    87,    17,    -1,    19,
      -1,    -1,    -1,    23,    24,    25,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    33,    34,    35,    -1,    -1,    38,    39,
      40,    41,    42,    43,    -1,    45,    -1,    47,    48,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    62,    -1,    -1,    65,    66,    67,    68,    69,
      70,    71,    -1,    73,    74,    75,    76,    77,    78,    -1,
      80,    -1,    -1,    83,    -1,    85,    86,    87,    13,    14,
      15,    -1,    17,    -1,    19,    -1,    -1,    22,    23,    24,
      25,    -1,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    42,    43,    -1,
      45,    -1,    47,    48,    49,    50,    -1,    52,    53,    -1,
      -1,    56,    -1,    -1,    59,    60,    -1,    62,    -1,    -1,
      65,    66,    67,    68,    69,    70,    71,    -1,    73,    74,
      75,    76,    77,    78,    -1,    80,    -1,    17,    83,    19,
      85,    86,    87,    23,    24,    25,    -1,    -1,    28,    -1,
      -1,    -1,    -1,    33,    34,    35,    -1,    -1,    38,    39,
      40,    41,    42,    43,    -1,    45,    -1,    47,    48,    49,
      50,    -1,    52,    53,    -1,    -1,    56,    -1,    -1,    59,
      60,    -1,    62,    -1,    -1,    65,    66,    67,    68,    69,
      70,    71,    -1,    73,    74,    75,    76,    77,    78,    -1,
      80,    -1,    17,    83,    19,    85,    86,    87,    23,    24,
      25,    -1,    -1,    28,    -1,    -1,    -1,    -1,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    42,    43,    -1,
      45,    -1,    47,    48,    49,    50,    -1,    52,    53,    -1,
      -1,    56,    -1,    -1,    59,    60,    -1,    62,    -1,    -1,
      65,    66,    67,    68,    69,    70,    71,    -1,    73,    74,
      75,    76,    77,    78,    -1,    80,    -1,    -1,    83,    -1,
      85,    86,    87
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,    17,    19,    23,    24,    25,    28,    33,    34,
      35,    38,    39,    40,    41,    42,    43,    45,    47,    48,
      49,    50,    52,    53,    56,    59,    60,    62,    65,    66,
      67,    68,    69,    70,    71,    73,    74,    75,    76,    77,
      78,    80,    83,    85,    86,    87,    90,    91,    92,    93,
     101,   102,   103,   104,   105,   106,   107,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,     0,    10,    11,    12,    54,    87,
      96,    97,    98,   100,   105,    10,    11,    30,    87,    98,
      18,   102,    54,   100,    61,   126,    35,    60,    67,    87,
      70,   100,   100,    98,   100,    98,    36,    98,    43,    46,
     108,     0,    91,    92,    13,    14,    15,    22,    87,    92,
      87,    87,    16,    21,    79,    95,    55,    81,    95,   122,
     107,   109,    64,    98,    47,   109,    15,    50,   127,   128,
      15,    58,   127,   113,   100,    98,    10,    11,   100,    98,
      87,    94,   100,    98,    98,    98,   100,    98,   100,    54,
      82,    87,    92,    92,    87,    92,    87,    98,    98,    27,
      81,    98,    26,    37,    98,   109,    10,    97,    99,   100,
      47,   128,    98,    98,   127,    98,    98,    98,     9,    32,
      98,    98,    98,   100,    98,    98,    98
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
        case 3:

/* Line 1806 of yacc.c  */
#line 290 "chat_parser.ypp"
    {
											chat_store_player(chat_msg.numCommands - 1, -1);	/* No player list means current command is addressed to everyone */
										}
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 295 "chat_parser.ypp"
    {
											chat_store_player(chat_msg.numCommands - 1, -1);
										}
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 299 "chat_parser.ypp"
    {
											//console("-unrecognized phrase");

											/* Reset current command */
											chat_reset_command(chat_msg.numCommands);
										}
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 328 "chat_parser.ypp"
    {
										chat_store_command("ally me");		/* Store current command */
									}
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 332 "chat_parser.ypp"
    {
										chat_store_command("go?");
									}
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 336 "chat_parser.ypp"
    {
										chat_store_command("go!");
									}
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 340 "chat_parser.ypp"
    {
										chat_store_command("drop a beacon");
									}
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 344 "chat_parser.ypp"
    {
										chat_store_command("go center");
									}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 348 "chat_parser.ypp"
    {
										chat_store_command("status?");
									}
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 352 "chat_parser.ypp"
    {
										chat_store_command("pumping units");
									}
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 356 "chat_parser.ypp"
    {
										chat_store_command("stop");
									}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 360 "chat_parser.ypp"
    {
										chat_store_command("got power?");
									}
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 364 "chat_parser.ypp"
    {
										chat_store_command("help me");
									}
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 368 "chat_parser.ypp"
    {
										chat_store_command("i'm ok");
									}
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 372 "chat_parser.ypp"
    {
										chat_store_command("roger");
									}
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 376 "chat_parser.ypp"
    {
										chat_store_command("ally player");
									}
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 380 "chat_parser.ypp"
    {
										chat_store_command("attack player");
									}
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 384 "chat_parser.ypp"
    {
										chat_store_command("attacking player?");
									}
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 388 "chat_parser.ypp"
    {
										chat_store_command("player has vtols");
									}
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 392 "chat_parser.ypp"
    {
										chat_store_command("getting player oil");
									}
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 396 "chat_parser.ypp"
    {
										chat_store_command("lassat player");
									}
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 400 "chat_parser.ypp"
    {
										chat_store_command("wait for me");
									}
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 404 "chat_parser.ypp"
    {
										chat_store_command("rush");
									}
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 412 "chat_parser.ypp"
    {
										chat_store_player(chat_msg.numCommands, (yyvsp[(1) - (1)].ival));		/* Remember this player was addressed in current command */
									}
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 416 "chat_parser.ypp"
    {
										chat_store_player(chat_msg.numCommands, (yyvsp[(2) - (2)].ival));
									}
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 420 "chat_parser.ypp"
    {
										chat_store_player(chat_msg.numCommands, (yyvsp[(3) - (3)].ival));
									}
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 424 "chat_parser.ypp"
    {
										chat_store_player(chat_msg.numCommands, (yyvsp[(3) - (3)].ival));
									}
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 430 "chat_parser.ypp"
    {
									(yyval.ival) = (yyvsp[(1) - (2)].ival);	// just pass player index
								}
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 632 "chat_parser.ypp"
    {
												/* Store for scripts */
												scrParameter.type = VAL_INT;
												scrParameter.v.ival = (yyvsp[(2) - (3)].ival);
												if(!chat_store_parameter(&scrParameter))
												{
													yyerror("chat command: Too many parameters in the message");
												}
											}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 645 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(2) - (3)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 655 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(2) - (3)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 668 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(2) - (3)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 681 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(1) - (4)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 693 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(3) - (5)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 705 "chat_parser.ypp"
    {
													/* Store for scripts */
													scrParameter.type = VAL_INT;
													scrParameter.v.ival = (yyvsp[(2) - (3)].ival);
													if(!chat_store_parameter(&scrParameter))
													{
														yyerror("chat command: Too many parameters in the message");
													}
												}
    break;



/* Line 1806 of yacc.c  */
#line 2339 "chat_parser.cpp"
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
#line 721 "chat_parser.ypp"


/* Initialize Bison and start chat processing */
bool chatLoad(const char *pData, UDWORD size)
{
	SDWORD	cmdIndex,parseResult;

	/* Don't parse the same message again for a different player */
	if(strcmp(pData, &(chat_msg.lastMessage[0])) == 0)	//just parsed this message for some other player
	{
		return true;			//keep all the parsed data unmodified
	}

	/* Tell bison what to parse */
	chatSetInputBuffer(pData, size);

	/* Reset previous chat processing */
	chat_msg.numCommands = 0;

	/* Initialize command data */
	for(cmdIndex=0; cmdIndex<MAX_CHAT_COMMANDS; cmdIndex++)
	{
		chat_reset_command(cmdIndex);
	}

	/* Invoke bison and parse */
	parseResult = chat_parse();

	/* Remember last message we parsed */
	sstrcpy(chat_msg.lastMessage, pData);

	/* See if we were successful parsing */
	if (parseResult != 0)
	{
		return false;
	}

	return true;
}

/* A simple error reporting routine */
static void yyerror(const char* msg)
{
	int		line;
	char	*pText;

	chatGetErrorData(&line, &pText);
	//debug(LOG_WARNING, "multiplayer message parse error: %s at line %d, token: %d, text: '%s'",
	//      msg, line, chat_char, pText);
}

