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
 *  Controller layout overlay screen.
 */

#ifndef __INCLUDED_SRC_SCREENS_GAMEPADLAYOUTSCREEN_H__
#define __INCLUDED_SRC_SCREENS_GAMEPADLAYOUTSCREEN_H__

// Shows a dismissable overlay listing the connected controller's core
// controls and current gamepad bindings, each with its button glyph
void showGamepadLayoutScreen();

void closeGamepadLayoutScreen();

bool isGamepadLayoutScreenUp();

// Shows the layout once per controller model when one connects, called each
// frame from the main loop. Gated on the show-on-connect setting
void gamepadLayoutMaybeAutoShow();

#endif // __INCLUDED_SRC_SCREENS_GAMEPADLAYOUTSCREEN_H__
