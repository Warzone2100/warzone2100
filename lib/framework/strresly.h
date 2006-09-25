/*! \file strresly.h
 *  * \brief Interface to the string resource lex and yacc functions.
 */
#ifndef _strresly_h
#define _strresly_h

/* Maximum number of characters in a directory entry */
#define FILE_MAXCHAR		255

/* Maximum number of TEXT items in any one Yacc rule */
#define TEXT_BUFFERS	10

/* The string resource currently being loaded */
extern STR_RES	*psCurrRes;

/* Set the current input buffer for the lexer - used by strresLoad */
extern void strresSetInputBuffer(char *pBuffer, UDWORD size);

/* Give access to the line number and current text for error messages */
extern void strresGetErrorData(int *pLine, char **ppText);

/* Call the yacc parser */
extern int strres_parse(void);

/* Store a string */
extern BOOL strresStoreString(STR_RES *psRes, STRING *pID, STRING *pString);

#endif

