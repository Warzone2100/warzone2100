/*! \file strres.h
 *  * \brief String resource interface functions
 */
#ifndef _strres_h
#define _strres_h

/* A string block */
typedef struct _str_block
{
	STRING	**apStrings;
	UDWORD	idStart, idEnd;

#ifdef DEBUG
	UDWORD	*aUsage;
#endif

	struct _str_block *psNext;
} STR_BLOCK;

/* An ID entry */
typedef struct _str_id
{
	UDWORD	id;
	STRING	*pIDStr;
} STR_ID;


/* A String Resource */
typedef struct _str_res
{
	TREAP			*psIDTreap;		// The treap to store string identifiers
	STR_BLOCK		*psStrings;		// The store for the strings themselves
	UDWORD			init,ext;		// Sizes for the string blocks
	UDWORD			nextID;			// The next free ID
} STR_RES;

/* Create a string resource object */
extern BOOL strresCreate(STR_RES **ppsRes, UDWORD init, UDWORD ext);

/* Release a string resource object */
extern void strresDestroy(STR_RES *psRes);

/* Release the id strings, but not the strings themselves,
 * (they can be accessed only by id number).
 */
extern void strresReleaseIDStrings(STR_RES *psRes);

/* Load a list of string ID's from a memory buffer
 * id == 0 should be a default string which is returned if the
 * requested string is not found.
 */
extern BOOL strresLoadFixedID(STR_RES *psRes, STR_ID *psID, UDWORD numID);

/* Return the ID number for an ID string */
extern BOOL strresGetIDNum(STR_RES *psRes, STRING *pIDStr, UDWORD *pIDNum);

/* Return the stored ID string that matches the string passed in */
extern BOOL strresGetIDString(STR_RES *psRes, STRING *pIDStr, STRING **ppStoredID);

/* Get the string from an ID number */
extern STRING *strresGetString(STR_RES *psRes, UDWORD id);

/* Load a string resource file */
extern BOOL strresLoad(STR_RES *psRes, char *pData, UDWORD size);

/* Return the the length of a STRING */
extern UDWORD stringLen(STRING *pStr);

/* Copy a STRING */
extern void stringCpy(STRING *pDest, STRING *pSrc);

/* Get the ID number for a string*/
extern UDWORD strresGetIDfromString(STR_RES *psRes, STRING *pString);

#endif

