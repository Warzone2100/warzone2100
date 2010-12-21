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

#ifndef __INCLUDED_SRC_KEYMAP_H__
#define __INCLUDED_SRC_KEYMAP_H__

#include "lib/framework/input.h"

#define NO_META_KEY				9999
#define KEYFUNC_TOGGLE_RADAR	20

typedef	enum
{
KEYMAP_DOWN,
KEYMAP_PRESSED,
KEYMAP_RELEASED
}KEY_ACTION;

typedef enum
{
KEYMAP__DEBUG,
KEYMAP_ALWAYS,
KEYMAP_ASSIGNABLE,
KEYMAP_ALWAYS_PROCESS,
KEYMAP___HIDE
}KEY_STATUS;

// moved to lib/framework/input.h
//#define KEY_IGNORE	5190

typedef struct _keyMapping
{
void (*function)(void);
BOOL		active;
KEY_STATUS	status;
UDWORD		lastCalled;
KEY_CODE	metaKeyCode;
KEY_CODE	altMetaKeyCode;
KEY_CODE	subKeyCode;
KEY_ACTION	action;
char		*pName;
struct _keyMapping	*psNext;
} KEY_MAPPING;

extern KEY_MAPPING	*keyAddMapping			( KEY_STATUS status, KEY_CODE metaCode, KEY_CODE subcode,
									 KEY_ACTION action, void (*pKeyMapFunc)(void), const char *name );
extern BOOL	keyRemoveMapping		( KEY_CODE metaCode, KEY_CODE subCode );
extern	KEY_MAPPING	*keyGetMappingFromFunction(void	*function);
extern BOOL	keyRemoveMappingPt		( KEY_MAPPING *psToRemove );
extern KEY_MAPPING *keyFindMapping	( KEY_CODE metaCode, KEY_CODE subCode );
extern void	keyClearMappings		( void );
extern void	keyProcessMappings		( BOOL bExclude );
extern void	keyInitMappings			( BOOL bForceDefaults );
extern UDWORD	getNumMappings		( void );
extern KEY_CODE	getLastSubKey		( void );
extern KEY_CODE	getLastMetaKey		( void );
extern KEY_MAPPING	*getLastMapping	( void );
extern void	keyEnableProcessing		( BOOL val );
extern void keyStatusAllInactive	( void );
extern void keyAllMappingsInactive(void);
extern void	keyAllMappingsActive	( void );
extern void	keySetMappingStatus		( KEY_MAPPING *psMapping, BOOL state );
extern void	processDebugMappings	( BOOL val );
extern BOOL	getDebugMappingStatus	( void );
extern	BOOL	keyReAssignMappingName(char *pName, KEY_CODE newMetaCode, KEY_CODE newSubCode);

extern	BOOL	keyReAssignMapping( KEY_CODE origMetaCode, KEY_CODE origSubCode,
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
