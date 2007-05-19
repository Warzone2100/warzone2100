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
/*
 * clParse.h
 *
 * All the command line values
 *
 */
#ifndef _clparse_h
#define _clparse_h

#define MAX_MODS 100

// parse the commandline
extern BOOL ParseCommandLine( int argc, char** argv );
extern BOOL ParseCommandLineEarly(int argc, char** argv);

extern BOOL bAllowDebugMode;

// FIXME The following does not belong here:

/*!
 * Set the framerate limit
 *
 * \param fpsLimit Desired framerate
 */
extern void setFramerateLimit(int fpsLimit);

/*!
 * Get the framerate limit
 *
 * \return Desired framerate
 */
extern int getFramerateLimit(void);

#endif


