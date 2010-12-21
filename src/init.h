/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/** @file
 *  Interface to the initialisation routines.
 */

#ifndef __INCLUDED_SRC_INIT_H__
#define __INCLUDED_SRC_INIT_H__

#include "lib/ivis_common/ivisdef.h"

// the size of the file loading buffer
// FIXME Totally inappropriate place for this.
#define FILE_LOAD_BUFFER_SIZE (1024*1024*4)
extern char fileLoadBuffer[];

extern BOOL systemInitialise(void);
extern void systemShutdown(void);
extern BOOL frontendInitialise(const char *ResourceFile);
extern BOOL frontendShutdown(void);
extern BOOL stageOneInitialise(void);
extern BOOL stageOneShutDown(void);
extern BOOL stageTwoInitialise(void);
extern BOOL stageTwoShutDown(void);
extern BOOL stageThreeInitialise(void);
extern BOOL stageThreeShutDown(void);

// Reset the game between campaigns
extern BOOL campaignReset(void);
// Reset the game when loading a save game
extern BOOL saveGameReset(void);

typedef struct _wzSearchPath
{
	char path[PATH_MAX];
	unsigned int priority;
	struct _wzSearchPath * higherPriority, * lowerPriority;
} wzSearchPath;

typedef enum { mod_clean=0, mod_campaign=1, mod_multiplay=2, mod_override=3 } searchPathMode;

void cleanSearchPath( void );
void registerSearchPath( const char path[], unsigned int priority );
BOOL rebuildSearchPath( searchPathMode mode, BOOL force );

BOOL buildMapList(void);

extern IMAGEFILE	*FrontImages;

#endif // __INCLUDED_SRC_INIT_H__
