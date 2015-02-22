/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_KEYMAP_H__
#define __INCLUDED_SRC_KEYMAP_H__

#include "lib/framework/input.h"


enum KEY_ACTION
{
KEYMAP_DOWN,
KEYMAP_PRESSED,
KEYMAP_RELEASED
};

enum KEY_STATUS
{
KEYMAP__DEBUG,
KEYMAP_ALWAYS,
KEYMAP_ASSIGNABLE,
KEYMAP_ALWAYS_PROCESS,
KEYMAP___HIDE
};

struct KEY_MAPPING
{
void (*function)(void);
bool		active;
KEY_STATUS	status;
UDWORD		lastCalled;
KEY_CODE	metaKeyCode;
KEY_CODE	altMetaKeyCode;
KEY_CODE	subKeyCode;
KEY_ACTION	action;
char		*pName;
KEY_MAPPING *   psNext;
};

extern KEY_MAPPING	*keyAddMapping			( KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subcode,
									 KEY_ACTION action, void (*pKeyMapFunc)(void), const char *name );
extern bool	keyRemoveMapping		( KEY_CODE metaCode, KEY_CODE subCode );
extern	KEY_MAPPING	*keyGetMappingFromFunction(void	*function);
extern bool	keyRemoveMappingPt		( KEY_MAPPING *psToRemove );
extern KEY_MAPPING *keyFindMapping	( KEY_CODE metaCode, KEY_CODE subCode );
extern void	keyClearMappings		( void );
extern void	keyProcessMappings		( bool bExclude );
extern void	keyInitMappings			( bool bForceDefaults );
extern UDWORD	getNumMappings		( void );
extern KEY_CODE	getLastSubKey		( void );
extern KEY_CODE	getLastMetaKey		( void );
extern KEY_MAPPING	*getLastMapping	( void );
extern void	keyEnableProcessing		( bool val );
extern void keyStatusAllInactive	( void );
extern void keyAllMappingsInactive(void);
extern void	keyAllMappingsActive	( void );
extern void	keySetMappingStatus		( KEY_MAPPING *psMapping, bool state );
void processDebugMappings(unsigned player, bool val);
bool getDebugMappingStatus();
bool getWantedDebugMappingStatus(unsigned player);
std::string getWantedDebugMappingStatuses(bool val);
extern	bool	keyReAssignMappingName(char *pName, KEY_CODE newMetaCode, KEY_CODE newSubCode);

extern	bool	keyReAssignMapping( KEY_CODE origMetaCode, KEY_CODE origSubCode,
							KEY_CODE newMetaCode, KEY_CODE newSubCode );
extern	KEY_MAPPING	*getKeyMapFromName(char *pName);


extern KEY_CODE	getQwertyKey		( void );

extern UDWORD	getMarkerX				( KEY_CODE code );
extern UDWORD	getMarkerY				( KEY_CODE code );
extern SDWORD	getMarkerSpin			( KEY_CODE code );

// for keymap editor.
typedef void (*_keymapsave)(void);
extern _keymapsave	keyMapSaveTable[];
extern KEY_MAPPING	*keyMappings;

//remove this one below
extern void	keyShowMappings				( void );

#endif // __INCLUDED_SRC_KEYMAP_H__
