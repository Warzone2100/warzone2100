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
#include "ivi.h"
#include "bug.h"

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
	va_list argptr;
	FILE *fp;

	fp = fopen(iV_DEBUG_FILE,"a");
	if (fp) {
		va_start(argptr,string);
		vfprintf(fp,string,argptr);
		va_end(argptr);
		fclose(fp);
	}
}

void iV_DisplayLogFile(void)

{
	FILE *fp;
	int c;

	fp = fopen(iV_DEBUG_FILE,"r");

	if (fp) {
		while ((c = getc(fp)) != EOF)
			debug( LOG_NEVER, "%c", c );
		fclose(fp);
	}
}
