/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
 *  Cursor drawing for gamepad input.
 */

#ifndef __INCLUDED_SRC_INPUT_GAMEPADCURSOR_H__
#define __INCLUDED_SRC_INPUT_GAMEPADCURSOR_H__

// Registers the overlay screen that draws the cursor while the gamepad is the
// active input source. Call once the widget system is initialized
bool gamepadCursorInit();
void gamepadCursorShutdown();

// Feeds the cursor's attraction target from clickable widgets near the
// cursor, called each frame from the main loop
void gamepadCursorUpdateWidgetMagnet();

#endif // __INCLUDED_SRC_INPUT_GAMEPADCURSOR_H__
