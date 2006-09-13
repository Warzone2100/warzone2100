/*
 * Internal definitions for the level system
 *
 */
#ifndef _levelint_h
#define _levelint_h

// return values from the lexer
enum _token_type
{
	LTK_LEVEL = 0x100,		// level key word
	LTK_PLAYERS,			// players key word
	LTK_TYPE,				// type key word
	LTK_DATA,				// data key word
	LTK_GAME,				// game key word
	LTK_CAMPAIGN,			// campaign key word
	LTK_CAMSTART,			// camstart key word
	LTK_CAMCHANGE,			// camchange key word
	LTK_DATASET,			// dataset key word
	LTK_EXPAND,				// expand key word
	LTK_EXPAND_LIMBO,		// expand Limbo key word
	LTK_BETWEEN,			// between key word
	LTK_MKEEP,				// miss_keep key word
	LTK_MKEEP_LIMBO,		// miss_keep Limbo key word
	LTK_MCLEAR,				// miss_clear key word
	LTK_IDENT,				// an identifier
	LTK_STRING,				// a quoted string
	LTK_INTEGER,			// a number
};

// return values from the lexer
extern STRING *pLevToken;
extern SDWORD levVal;

// error report function for the level parser
extern void levError(const STRING *pError);

// the lexer function
extern int lev_lex(void);

/* Set the current input buffer for the lexer */
extern void levSetInputBuffer(char *pBuffer, UDWORD size);

extern void levGetErrorData(int *pLine, char **ppText);

#endif

