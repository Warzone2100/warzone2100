/*
 * Debug.c
 *
 * Various debugging output functions.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif

#include "frame.h"
#include "frameint.h"

#define DEBUG_STR_MAX 1000  // Maximum length of a debugging output string

static FILE *pDebugFile = NULL;
static BOOL StringOut = TRUE;

#ifdef WIN321	//Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly 
// message and assert callbacks
static DB_MBCALLBACK	mbCallback = NULL;
static DB_MBCALLBACK	errorCallback = NULL;
static DB_MBCALLBACK	assertCallback = NULL;

// Set the message box callback
void dbg_SetMessageBoxCallback(DB_MBCALLBACK callback)
{
	mbCallback = callback;
}

// set the error box callback
void dbg_SetErrorBoxCallback(DB_MBCALLBACK callback)
{
	errorCallback = callback;
}

// Set the assert box callback
void dbg_SetAssertCallback(DB_MBCALLBACK callback)
{
	assertCallback = callback;
}
#endif // WIN32

/*
 * dbg_printf
 *
 * Replacement for printf for debugging output.
 * Sends strings to OutputDebugString and/or a file.
 * Output is controlled by the functions :
 *
 * dbg_SetOutputFile, dbg_NoOutputFile
 * dbg_SetOutputString, dbg_NoOutputString
 */
void dbg_printf(SBYTE *pFormat, ...)
{
	SBYTE		aBuffer[DEBUG_STR_MAX];   // Output string buffer
    va_list		pArgs;					  // Format arguments
	
	/* Initialise the argument list */
	va_start(pArgs, pFormat);

	/* Print out the string */
	(void)vsprintf(aBuffer, pFormat, pArgs);

	/* Output it */
	if (StringOut)
	{
#ifdef WIN321	//Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
		OutputDebugString(aBuffer);
#else
		fprintf(stderr, "%s", aBuffer);
	    fflush(stderr);
#endif
	}

	/* If there is a debugging file open, send text to that too */
	if (pDebugFile)
	{
		fprintf(pDebugFile, "%s", aBuffer);
		fflush(pDebugFile);
	}

}

/*
 * dbg_SetOutputFile
 *
 * Send debugging output to a file
 */
void dbg_SetOutputFile(SBYTE *pFilename)
{
	ASSERT((pFilename != NULL, "dbg_SetOutputFile passed NULL filename"));

	if (pDebugFile)
	{
		fclose(pDebugFile);
	}
	pDebugFile = fopen(pFilename, "w");

	if (!pDebugFile)
	{
		dbg_MessageBox("Couldn't open debugging output file: %s", pFilename);
	}
}

/*
 * dbg_NoOutputFile
 *
 * Turn off debugging to a file
 */
void dbg_NoOutputFile(void)
{
	if (pDebugFile)
	{
		fclose(pDebugFile);
	}
}

/*
 * dbg_SetOutputString
 *
 * Send debugging to OutputDebugString
 */
void dbg_SetOutputString(void)
{
	StringOut = TRUE;
}

/*
 * dbg_NoOutputString
 *
 * Turn off debugging to OutputDebugString
 */
void dbg_NoOutputString(void)
{
	StringOut = FALSE;
}

/*
 * dbg_MessageBox
 *
 * Display a message box, arguments as printf
 */
void dbg_MessageBox(SBYTE *pFormat, ...)
{
	SBYTE		aBuffer[DEBUG_STR_MAX];   // Output string buffer
    va_list		pArgs;					  // Format arguments
#ifdef WIN321  //Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
	DB_MBRETVAL	retVal;
#endif
	
	/* Initialise the argument list */
	va_start(pArgs, pFormat);

	/* Print out the string */
	(void)vsprintf(aBuffer, pFormat, pArgs);

	/* Output it */
	dbg_printf("MB: %s\n", aBuffer);

#ifdef WIN321  //Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
	/* Ensure the box can be seen */
	screenFlipToGDI();

	retVal = DBR_USE_WINDOWS_MB;
	if (mbCallback)
	{
		retVal = mbCallback(aBuffer);
	}
	if (retVal == DBR_USE_WINDOWS_MB)
	{
		(void)MessageBox(frameGetWinHandle(), aBuffer, "Debugging Message", MB_OK);
	}
#endif
}

/*
 *
 * dbg_ErrorPosition
 *
 * Set the position for an error message.
 */
#define ERROR_DEFAULT_FILE "No valid Error file name"
static SBYTE aErrorFile[DEBUG_STR_MAX]=ERROR_DEFAULT_FILE;
static UDWORD ErrorLine;
void dbg_ErrorPosition(SBYTE *pFile, UDWORD Line)
{
#ifdef WIN321  //Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
	if (pFile == NULL)
	{
		/* Ensure the box can be seen */
		screenFlipToGDI();

		(void)MessageBox(frameGetWinHandle(), "Invalid error arguments\n", "Error",
			       MB_OK | MB_ICONWARNING);
		strcpy(aErrorFile, ERROR_DEFAULT_FILE);
		ErrorLine = 0;
		return;
	}
	
	if (strlen(pFile) >= DEBUG_STR_MAX)
	{
		memcpy(aErrorFile, pFile, DEBUG_STR_MAX);
		aErrorFile[DEBUG_STR_MAX-1] = '\0';
		ErrorLine = Line;
	}
	else
	{
		strcpy(aErrorFile, pFile);
		ErrorLine = Line;
	}
#endif // WIN32
}

/*
 * dbg_ErrorBox
 *
 * Display an error message in a dialog box, arguments as printf
 */
void dbg_ErrorBox(SBYTE *pFormat, ...)
{
	SBYTE		aBuffer[DEBUG_STR_MAX] = "";	// Output string buffer
    va_list		pArgs;							// Format arguments
#ifdef WIN321		//Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
	DB_MBRETVAL	retVal;
#endif
	
	/* Initialise the argument list */
	va_start(pArgs, pFormat);

	/* See if there is a Filename to display */
	if (strcmp(aErrorFile, ERROR_DEFAULT_FILE) != 0)
	{
		sprintf(aBuffer, "File: %s\nLine: %d\n\n", aErrorFile, ErrorLine);
	}

	/* Print out the string */
	(void)vsprintf(aBuffer + strlen(aBuffer), pFormat, pArgs);

	/* Output it */
	dbg_printf("ErrorBox: %s\n", aBuffer);

#ifdef WIN321  //Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
	/* Ensure the box can be seen */
	screenFlipToGDI();

	retVal = DBR_USE_WINDOWS_MB;
	if (errorCallback)
	{
		retVal = errorCallback(aBuffer);
	}
	if (retVal == DBR_USE_WINDOWS_MB)
	{
		(void)MessageBox(frameGetWinHandle(), aBuffer, "Error", MB_OK | MB_ICONWARNING);
	}
#endif
}

/*
 * dbg_AssertPosition
 *
 * Set the file and position for an assertion.
 * This should only be used directly before a call to dbg_Assert.
 * (In fact there is no reason for this to be used outside the ASSERT macro)
 */
#define ASSERT_DEFAULT_FILE "No Valid Assert File Name"
//static SBYTE aAssertFile[DEBUG_STR_MAX];
static SBYTE *pAssertFile;
static UDWORD AssertLine;
void dbg_AssertPosition(SBYTE *pFile, UDWORD Line)
{
	if (pFile == NULL)
	{
#ifdef WIN321	//Mainly for debugging, but it also breaks some stuff, so disabled to WIN321 for now -Qamly
		/* Ensure the box can be seen */
		screenFlipToGDI();

		(void)MessageBox(frameGetWinHandle(), "Invalid assertion arguments\n", "Error",
			       MB_OK | MB_ICONWARNING);
#else
		DBPRINTF(("Error : Invalid assertion arguments\n"));
#endif
//		strcpy(aAssertFile, ASSERT_DEFAULT_FILE);
		pAssertFile = ASSERT_DEFAULT_FILE;
		AssertLine = 0;
		return;
	}
	
/*	if (strlen(pFile) >= DEBUG_STR_MAX)
	{
		memcpy(aAssertFile, pFile, DEBUG_STR_MAX);
		aAssertFile[DEBUG_STR_MAX-1] = '\0';
		AssertLine = Line;
	}
	else
	{
		strcpy(aAssertFile, pFile);
		AssertLine = Line;
	}*/
	pAssertFile = pFile;
	AssertLine = Line;
}

/*
 * dbg_Assert
 *
 * Rewritten assert macro.
 * If Expression is false it uses the Format string and va_list to
 * generate a string which it displays in a message box.
 * 
 * DebugBreak is used to jump into the debugger.
 *
 */
#ifdef WIN32_DEBUG //WIN32
void dbg_Assert(BOOL Expression, SBYTE *pFormat, ...)
{
	va_list		pArgs;
	SBYTE		aBuffer[DEBUG_STR_MAX];
	int			Result=0;
	DB_MBRETVAL	retVal;

	if (!Expression)
	{
		va_start(pArgs, pFormat);
		sprintf(aBuffer, "File: %s\nLine: %d\n\n", pAssertFile, AssertLine);
		(void)vsprintf(aBuffer + strlen(aBuffer), pFormat, pArgs);
		(void)strcat(aBuffer, "\n\nTERMINATE PROGRAM ?");
		
		/* Ensure the box can be seen */
		screenFlipToGDI();

		dbg_printf("ASSERT: %s\n", aBuffer);
		retVal = DBR_USE_WINDOWS_MB;
		if (assertCallback)
		{
			retVal = assertCallback(aBuffer);
		}
		if (retVal == DBR_USE_WINDOWS_MB)
		{
//			if (!bRunningUnderGlide)
			{
				Result = MessageBox(frameGetWinHandle(), aBuffer, "Assertion Failure",
									MB_YESNOCANCEL | MB_ICONWARNING);
			}
/*			else
			{
				Result = MessageBox(NULL, aBuffer, "Assertion Failure",
									MB_YESNOCANCEL | MB_ICONWARNING);
			}*/
		}

		if (retVal == DBR_YES ||
			Result == IDYES)
		{
//			abort();
//			frameShutDown();
			ExitProcess(3);
		}
		else if (retVal == DBR_NO || Result == IDNO)
		{
			/* Can only do a DebugBreak in window mode -
			 * Do it in full screen and GDI can't get in so the machine hangs.
			 */
			screenSetMode(SCREEN_WINDOWED);
			DebugBreak();
		}
		//  Result could == IDCANCEL - do nothing in this case.

	}
}
#else
void dbg_Assert(BOOL Expression, SBYTE *pFormat, ...)
{
	if (!Expression)
	{
		DBPRINTF(("\n\nAssertion failed , File: %s\nLine: %d\n\n", pAssertFile, AssertLine));
		abort();
	}
}
#endif

