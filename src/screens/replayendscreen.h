/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  The overlay screen displayed when a replay ends (currently: a "Player Stats" graph panel)
 */

#ifndef __INCLUDED_SRC_SCREENS_REPLAYENDSCREEN_H__
#define __INCLUDED_SRC_SCREENS_REPLAYENDSCREEN_H__

void showReplayEndScreen();
void closeReplayEndScreen();

// Temporarily hide / re-show the replay end screen (if it's up) - e.g. while the in-game options menu is open
void setReplayEndScreenVisible(bool visible);

#endif // __INCLUDED_SRC_SCREENS_REPLAYENDSCREEN_H__
