#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/bug.h"
#include "lib/ivis_common/piepalette.h"
#include "piematrix.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/ivispatch.h"

//*************************************************************************

iError	_iVERROR;

//*************************************************************************
// pass in true to reset the palette too.
void iV_Reset(int bPalReset)
{
	_TEX_INDEX = 0;
	iV_ClearFonts();		// Initialise the IVIS font module.
}


void iV_ShutDown(void)

{

	iV_DEBUG0("1\n");

#ifdef iV_DEBUG
	iV_HeapLog();
#endif

	iV_DEBUG0("4\n");
	pie_ShutDown();
	iV_DEBUG0("5\n");

	iV_VideoMemoryUnlock();

	iV_DEBUG0("6\n");

	pie_TexShutDown();

	iV_DEBUG0("7\n");

//	_iv_heap_tidy();

	iV_DEBUG0("8\n");

	iV_DEBUG0("9\n");

	iV_DEBUG0("iVi[ShutDown] = successful\n");
}

//*************************************************************************

void iV_Stop(char *string, ...)

{
	va_list argptr;

	iV_ShutDown();
	va_start(argptr,string);
	vprintf(string,argptr);
	va_end(argptr);
	exit(0);
}

//*************************************************************************

void iV_Abort(char *string, ...)

{
	va_list argptr;

	va_start(argptr,string);
	vprintf(string,argptr);
	va_end(argptr);
}

//*************************************************************************

#ifndef FINALBUILD
void iV_Error(long errorn, char *msge, ...)
{
	va_list argptr;

	va_start(argptr,msge);
	vsprintf(&_iVERROR.msge[0],msge,argptr);
	va_end(argptr);
	_iVERROR.n = errorn;
}
#endif

