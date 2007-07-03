/***************************************************************************************
 *
 *	File : Debug.h
 *
 *  John Elliot		11/96
 *
 *	Function:	Provides a large number of macros for debugging output.
 *
 *	Comments:	The basic macros : DBPRINTF, DBMB, DBMONOPRINTF, etc are turned on if
 *				either DEBUG is defined or a MSVC Debug build is specified.  They are
 *				turned off by default and if NODEBUG is defined or a MSVC Release build
 *				is specified.  Debugging can be forced to be included on a Release build
 *				by defining FORCEDEBUG.
 *
 *				The conditional versions of these macros : DBP0 - DBP9, DBMB0 - DBMB9,
 *				DBMONOP0 - DBMONOP9, etc. are turned on by defining the appropriate
 *				macro from DEBUG_GROUP0 - DEBUG_GROUP9.
 *
 *	Global Macros:
 *			DBPRINTF(x)
 *			DBOUTPUTFILE(x)
 *			DBNOOUTPUTFILE()
 * 			DBSETOUTPUTSTRING()
 *			DBNOOUTPUTSTRING()
 *			DBMB(x)
 *			ASSERT(x)
 * 
 *			DBMONOPRINTF(x)
 * 			DBMONOCLEAR()
 *			DBMONOCLEARRECT(x,y,width,height)
 * 
 *			DBP0 - DBP9
 *			DBMB0 - DBMB9
 *			DBMONOP0 - DBMONOP9
 *			DBMONOC0 - DBMONOC9
 *			DBMONOCR0 - DBMONOCR9
 *
 ***************************************************************************************
 */
#ifndef _debug_h
#define _debug_h

//#define ALWAYS_ASSERT	// Define this to always process ASSERT even on release builds.

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */

#include <stdio.h>

#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

/* Turn on basic debugging if a MSVC debug build and NODEBUG has not been defined */
#ifdef _DEBUG
#ifndef NODEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif
#endif

/* Turn off debugging if a MSVC release build and FORCEDEBUG has not been defined.
   Turn on debugging if FORCEDEBUG has been defined. */
#ifdef _NDEBUG
#ifndef FORCEDEBUG
#undef DEBUG
#else
#ifndef DEBUG
#define DEBUG
#endif
#endif
#endif

/* Allow mono debugging to be turned on and off seperately */
#undef MONODEBUG
#ifdef FORCEMONODEBUG
#define MONODEBUG
#elif !defined(NOMONODEBUG)
/* turn on mono debugging by default */
#define MONODEBUG
#endif

#include <stdarg.h>
#include "types.h"

#ifdef WIN32
/* Include the mono printing stuff */
#include "mono.h"
#endif

/****************************************************************************************
 *
 * Function prototypes from debug.c
 *
 */
extern void dbg_printf(SBYTE *pFormat, ...);
extern void dbg_SetOutputFile(SBYTE *pFilename);
extern void dbg_NoOutputFile(void);
extern void dbg_SetOutputString(void);
extern void dbg_NoOutputString(void);
extern void dbg_MessageBox(SBYTE *pFormat, ...);
extern void dbg_ErrorPosition(SBYTE *pFile, UDWORD Line);
extern void dbg_ErrorBox(SBYTE *pFormat, ...);
extern void dbg_AssertPosition(SBYTE *pFile, UDWORD Line);
extern void dbg_Assert(BOOL Expression, SBYTE *pFormat, ...);

/*****************************************************************************************
 *
 * Definitions for message box callback functions
 *
 */

typedef enum _db_mbretval
{
	DBR_USE_WINDOWS_MB,		// display the windows MB after the callback
	DBR_YES,				// yes button pressed
	DBR_NO,					// no button pressed
	DBR_OK,					// ok button pressed
	DBR_CANCEL,				// cancel button pressed
} DB_MBRETVAL;

// message box callback function
typedef DB_MBRETVAL (*DB_MBCALLBACK)(SBYTE *pBuffer);

// Set the message box callback
extern void dbg_SetMessageBoxCallback(DB_MBCALLBACK callback);

// set the error box callback
extern void dbg_SetErrorBoxCallback(DB_MBCALLBACK callback);

// Set the assert box callback
extern void dbg_SetAssertCallback(DB_MBCALLBACK callback);

/* Get PCLint to check the printf args of these functions */
/*lint -printf(1,dbg_printf,dbg_MessageBox,dbg_ErrorBox) */
/*lint -printf(2,dbg_Assert) */

#ifdef PSX
extern char DBGstring[256];
#endif

#ifdef DEBUG
/* Debugging output required */

/****************************************************************************************
 *
 * Basic debugging macro's
 *
 */

/*
 *
 * DBPRINTF
 *
 * Output debugging strings - arguments as printf except two sets of brackets have
 * to be used :
 *		DBPRINTF(("Example output string with a variable: %d\n", Variable));
 */
#ifdef WIN32
#define DBPRINTF(x)				dbg_printf x
#else
/*#define DBPRINTF(x) \
	printf("DBPRINTF @ %s,%d:\n",__FILE__,__LINE__);\
	printf x;\
	printf("\n") */
#define DBPRINTF(x) printf x;
#endif

/*
 *
 * DBSETOUTPUTFILE
 *
 * Sets the name of a text file to send all debugging output to
 */
#define DBOUTPUTFILE(x)		dbg_SetOutputFile(x)

/*
 *
 * DBNOOUTPUTFILE
 *
 * Stops sending debugging output to a text file
 */
#define DBNOOUTPUTFILE			dbg_NoOutputFile

/*
 *
 * DBSETOUTPUTSTRING
 *
 * Turns on sending debugging output to OutputDebugString
 */
#define DBSETOUTPUTSTRING		dbg_SetOutputString

/*
 *
 * DBSETOUTPUTSTRING
 *
 * Turns off sending debugging output to OutputDebugString
 */
#define DBNOOUTPUTSTRING		dbg_NoOutputString

/*
 *
 * DBMB
 *
 * Displays a message box containing a string and waits until OK is clicked on the
 * message box.
 * Arguments are as for DBPRINTF.
 */
#define DBMB(x)					dbg_MessageBox x

/*
 *
 * ASSERT
 *
 * Rewritten version of assert that allows a printf format text string to be passed
 * to ASSERT along with the condition.
 *
 * Arguments:	ASSERT((condition, "Format string with variables: %d, %d", var1, var2));
 */
#define ASSERT(x) \
	dbg_AssertPosition(__FILE__, __LINE__), \
	dbg_Assert x

/*
 *
 * DBERROR
 *
 * Error message macro - use this if the error should be reported even in
 * production code (i.e. out of memory errors, file not found etc.)
 *
 * Arguments as for printf
 */
#ifdef PSX
#define DBERROR(x) printf x; printf("\n...DBERROR in line %d of %s\n",__LINE__,__FILE__);
#else				   
#define DBERROR(x) \
	dbg_ErrorPosition(__FILE__, __LINE__), \
	dbg_ErrorBox x
#endif

/****************************************************************************************
 *
 * Mono monitor output macros
 *
 */
#if defined(WIN32) && defined(MONODEBUG)

/*
 *
 * DBMONOPRINTF
 *
 * Version of printf that outputs the string to a specified location on the mono screen
 *
 * Arguments :  DBMONOPRINTF((x,y, "String : %d, %d", var1, var2));
 */
#define DBMONOPRINTF(x)			dbg_MONO_PrintString x

/*
 *
 * DBMONOCLEAR
 *
 * Clear the mono monitor
 */
#define DBMONOCLEAR				dbg_MONO_ClearScreen

/*
 *
 * DBMONOCLEARRECT
 *
 * Clear a rectangle on the mono screen
 * Arguments :  DBMONOCLEARRECT(x,y, width, height)
 */
#define DBMONOCLEARRECT(x,y,width,height) \
								dbg_MONO_ClearRectangle(x,y,width,height);
#else
/* No mono monitor on a playstation so undefine all the macros */
#define DBMONOPRINTF(x)
#define DBMONOCLEAR()
#define DBMONOCLEARRECT(x,y,width,height)
#endif

/****************************************************************************************
 *
 * Conditional debugging macro's that can be selectively turned on or off on a file
 * by file basis.
 *
 */

#ifdef DEBUG_GROUP0
#define DBP0(x)							DBPRINTF(x)
#define DBMB0(x)						DBMB(x)
#define DBMONOP0(x)						DBMONOPRINTF(x)
#define DBMONOC0()						DBMONOCLEAR()
#define DBMONOCR0(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP0(x)
#define DBMB0(x)
#define DBMONOP0(x)
#define DBMONOC0()
#define DBMONOCR0(x,y,width,height)
#endif

#ifdef DEBUG_GROUP1
#define DBP1(x)							DBPRINTF(x)
#define DBMB1(x)						DBMB(x)
#define DBMONOP1(x)						DBMONOPRINTF(x)
#define DBMONOC1()						DBMONOCLEAR()
#define DBMONOCR1(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP1(x)
#define DBMB1(x)
#define DBMONOP1(x)
#define DBMONOC1()
#define DBMONOCR1(x,y,width,height)
#endif

#ifdef DEBUG_GROUP2
#define DBP2(x)							DBPRINTF(x)
#define DBMB2(x)						DBMB(x)
#define DBMONOP2(x)						DBMONOPRINTF(x)
#define DBMONOC2()						DBMONOCLEAR()
#define DBMONOCR2(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP2(x)
#define DBMB2(x)
#define DBMONOP2(x)
#define DBMONOC2()
#define DBMONOCR2(x,y,width,height)
#endif

#ifdef DEBUG_GROUP3
#define DBP3(x)							DBPRINTF(x)
#define DBMB3(x)						DBMB(x)
#define DBMONOP3(x)						DBMONOPRINTF(x)
#define DBMONOC3()						DBMONOCLEAR()
#define DBMONOCR3(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP3(x)
#define DBMB3(x)
#define DBMONOP3(x)
#define DBMONOC3()
#define DBMONOCR3(x,y,width,height)
#endif

#ifdef DEBUG_GROUP4
#define DBP4(x)							DBPRINTF(x)
#define DBMB4(x)						DBMB(x)
#define DBMONOP4(x)						DBMONOPRINTF(x)
#define DBMONOC4()						DBMONOCLEAR()
#define DBMONOCR4(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP4(x)
#define DBMB4(x)
#define DBMONOP4(x)
#define DBMONOC4()
#define DBMONOCR4(x,y,width,height)
#endif

#ifdef DEBUG_GROUP5
#define DBP5(x)							DBPRINTF(x)
#define DBMB5(x)						DBMB(x)
#define DBMONOP5(x)						DBMONOPRINTF(x)
#define DBMONOC5()						DBMONOCLEAR()
#define DBMONOCR5(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP5(x)
#define DBMB5(x)
#define DBMONOP5(x)
#define DBMONOC5()
#define DBMONOCR5(x,y,width,height)
#endif

#ifdef DEBUG_GROUP6
#define DBP6(x)							DBPRINTF(x)
#define DBMB6(x)						DBMB(x)
#define DBMONOP6(x)						DBMONOPRINTF(x)
#define DBMONOC6()						DBMONOCLEAR()
#define DBMONOCR6(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP6(x)
#define DBMB6(x)
#define DBMONOP6(x)
#define DBMONOC6()
#define DBMONOCR6(x,y,width,height)
#endif

#ifdef DEBUG_GROUP7
#define DBP7(x)							DBPRINTF(x)
#define DBMB7(x)						DBMB(x)
#define DBMONOP7(x)						DBMONOPRINTF(x)
#define DBMONOC7()						DBMONOCLEAR()
#define DBMONOCR7(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP7(x)
#define DBMB7(x)
#define DBMONOP7(x)
#define DBMONOC7()
#define DBMONOCR7(x,y,width,height)
#endif

#ifdef DEBUG_GROUP8
#define DBP8(x)							DBPRINTF(x)
#define DBMB8(x)						DBMB(x)
#define DBMONOP8(x)						DBMONOPRINTF(x)
#define DBMONOC8()						DBMONOCLEAR()
#define DBMONOCR8(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP8(x)
#define DBMB8(x)
#define DBMONOP8(x)
#define DBMONOC8()
#define DBMONOCR8(x,y,width,height)
#endif

#ifdef DEBUG_GROUP9
#define DBP9(x)							DBPRINTF(x)
#define DBMB9(x)						DBMB(x)
#define DBMONOP9(x)						DBMONOPRINTF(x)
#define DBMONOC9()						DBMONOCLEAR()
#define DBMONOCR9(x,y,width,height)		DBMONOCLEARRECT(x,y,width,height)
#else
#define DBP9(x)
#define DBMB9(x)
#define DBMONOP9(x)
#define DBMONOC9()
#define DBMONOCR9(x,y,width,height)
#endif

#else

/* No Debugging output required */
#ifdef WIN32
#define DBPRINTF(x)
#else	// currently we want DBPRINTF to work on the PSX even on release build
#define DBPRINTF(x) printf x;
#endif

#define DBOUTPUTFILE(x)
#define DBNOOUTPUTFILE()
#define DBSETOUTPUTSTRING()
#define DBNOOUTPUTSTRING()
#define DBMB(x)

#ifdef ALWAYS_ASSERT
#define ASSERT(x) \
	dbg_AssertPosition(__FILE__, __LINE__), \
	dbg_Assert x
#else
#define ASSERT(x)
#endif



#ifdef PSX
#define DBERROR(x) printf x; printf("\n...DBERROR in line %d of %s\n",__LINE__,__FILE__);
#else				   
#define DBERROR(x)	dbg_ErrorBox x
#endif


#define DBMONOPRINTF(x)
#define DBMONOCLEAR()
#define DBMONOCLEARRECT(x,y,width,height)

#define DBP0(x)
#define DBP1(x)
#define DBP2(x)
#define DBP3(x)
#define DBP4(x)
#define DBP5(x)
#define DBP6(x)
#define DBP7(x)
#define DBP8(x)
#define DBP9(x)

#define DBMB0(x)
#define DBMB1(x)
#define DBMB2(x)
#define DBMB3(x)
#define DBMB4(x)
#define DBMB5(x)
#define DBMB6(x)
#define DBMB7(x)
#define DBMB8(x)
#define DBMB9(x)

#define DBMONOP0(x)
#define DBMONOP1(x)
#define DBMONOP2(x)
#define DBMONOP3(x)
#define DBMONOP4(x)
#define DBMONOP5(x)
#define DBMONOP6(x)
#define DBMONOP7(x)
#define DBMONOP8(x)
#define DBMONOP9(x)

#define DBMONOC0()
#define DBMONOC1()
#define DBMONOC2()
#define DBMONOC3()
#define DBMONOC4()
#define DBMONOC5()
#define DBMONOC6()
#define DBMONOC7()
#define DBMONOC8()
#define DBMONOC9()

#define DBMONOCR0(x,y,w,h)
#define DBMONOCR1(x,y,w,h)
#define DBMONOCR2(x,y,w,h)
#define DBMONOCR3(x,y,w,h)
#define DBMONOCR4(x,y,w,h)
#define DBMONOCR5(x,y,w,h)
#define DBMONOCR6(x,y,w,h)
#define DBMONOCR7(x,y,w,h)
#define DBMONOCR8(x,y,w,h)
#define DBMONOCR9(x,y,w,h)

#endif

#endif
