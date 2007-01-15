/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
void iV_Error(long errorn, const char *msge, ...)
{
	va_list argptr;

	va_start(argptr,msge);
	vsprintf(&_iVERROR.msge[0],msge,argptr);
	va_end(argptr);
	_iVERROR.n = errorn;
}
#endif

