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
/** @file
 *  Warzone Global configuration functions.
 */

#ifndef __INCLUDED_SRC_WARZONECONFIG_H__
#define __INCLUDED_SRC_WARZONECONFIG_H__

#include "lib/framework/frame.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
typedef	enum	SEQ_MODE
				{
					SEQ_FULL,
					SEQ_SMALL,
					SEQ_SKIP
				}
				SEQ_MODE;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void	war_SetDefaultStates(void);
extern void war_SetFog(BOOL val);
extern BOOL war_GetFog(void);
extern void war_SetSeqMode(SEQ_MODE mode);
extern SEQ_MODE war_GetSeqMode(void);
extern void war_SetPlayAudioCDs(BOOL b);
extern BOOL war_GetPlayAudioCDs(void);
extern void war_SetAllowSubtitles(BOOL);
extern BOOL war_GetAllowSubtitles(void);
extern void war_setFullscreen(BOOL);
extern BOOL war_getFullscreen(void);
extern void war_SetTrapCursor(BOOL b);
extern BOOL war_GetTrapCursor(void);
extern void war_SetWidth(UDWORD width);
extern UDWORD war_GetWidth(void);
extern void war_SetHeight(UDWORD height);
extern UDWORD war_GetHeight(void);

/**
 * Enable or disable sound initialization
 * Has no effect after systemInitialize()!
 *
 * \param	soundEnabled	enable sound (or not)
 */
void war_setSoundEnabled( BOOL soundEnabled );

/**
 * Whether we should initialize sound or not
 *
 * \return	Enable sound (or not)
 */
BOOL war_getSoundEnabled( void );

#endif // __INCLUDED_SRC_WARZONECONFIG_H__
