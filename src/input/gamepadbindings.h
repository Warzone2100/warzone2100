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
 *  Gamepad button dispatch for in-game actions.
 */

#ifndef __INCLUDED_SRC_INPUT_GAMEPADBINDINGS_H__
#define __INCLUDED_SRC_INPUT_GAMEPADBINDINGS_H__

// Dispatches in-game actions bound to gamepad buttons. Called once per frame
// from the game input processing
// False while something modal has taken input away from the game. Such a view
// switches off every key context, and the pad has to follow or its buttons go
// on driving the game behind whatever is on screen.
bool gamepadCanDriveGame();

void gamepadProcessBindings();

// The slice of the dispatch that must keep working while the game update is
// paused, when the input processing above does not run
void gamepadProcessPausedBindings();

#endif // __INCLUDED_SRC_INPUT_GAMEPADBINDINGS_H__
