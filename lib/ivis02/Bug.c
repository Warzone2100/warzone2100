#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ivi.h"
#include "bug.h"


#ifdef PSX

#define NO_IV_DEBUG_PRINTF	// if defined then NEVER do the printf ... if not defined then it depends on iV_DEBUG

#include "file_psx.h"	// special PSX handling of 'FILE'

#ifdef iV_DEBUG
char dbgbuffer[MAXDBGSIZE];	// global for debugging buffer
#endif

#endif

//*************************************************************************

void _debug_create_log(void)

{
	FILE *fp;

	fp = fopen(iV_DEBUG_FILE,"w");
	if (fp)
		fclose(fp);
}

void iV_Debug(char *string, ...)

{
#ifndef PIEPSX
	va_list argptr;
	FILE *fp;

	fp = fopen(iV_DEBUG_FILE,"a");
	if (fp) {
		va_start(argptr,string);
		vfprintf(fp,string,argptr);
		va_end(argptr);
		fclose(fp);
	}
#else
#ifndef NO_IV_DEBUG_PRINTF	
	DBPRINTF(("iv_debug : %s\n",string));
#endif
#endif
}

void iV_DisplayLogFile(void)

{
#ifdef WIN32
	FILE *fp;
	int c;

	fp = fopen(iV_DEBUG_FILE,"r");

	if (fp) {
		while ((c = getc(fp)) != EOF)
			DBPRINTF(("%c",c));
		fclose(fp);
	}
#else
	DBPRINTF(("iv_debug output\n"));
#endif
}
