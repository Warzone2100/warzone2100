// SPDX-License-Identifier: GPL-2.0-or-later

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
 *  The built-in on-screen keyboard.
 *
 *  Shown while an edit box is being edited, for typing with a gamepad or touch.
 *  It types into the edit box through the input buffer, and never takes focus away from it.
 *
 *  It only appears when the player asked to type, by pressing the edit box,
 *  or through an action reported with virtualKeyboardRequestForNextTextEntry() (ex. opening the chat).
 *  An edit box that code focuses by itself (ex. the lobby chat regaining focus) doesn't bring it up.
 *  Hiding it, or typing on a physical keyboard, withdraws the request until the edit box is pressed again.
 */

#ifndef __INCLUDED_LIB_WIDGET_VIRTUALKEYBOARD_H__
#define __INCLUDED_LIB_WIDGET_VIRTUALKEYBOARD_H__

#include "lib/framework/wzstring.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

enum class VirtualKeyboardMode : uint8_t
{
	Automatic = 0,  // when a gamepad or touch is in use, unless the platform shows its own keyboard
	BuiltIn,        // whenever text is edited (platform keyboards are suppressed)
	System,         // only the platform's own on-screen keyboard, if it has one
	Disabled,       // no on-screen keyboard at all
};

// Layout preference values besides a layout id
#define VIRTUAL_KEYBOARD_LAYOUT_AUTO "auto"      // the OS layout with a physical keyboard, else the game language's layout
#define VIRTUAL_KEYBOARD_LAYOUT_SYSTEM "system"  // the current OS keyboard layout

void virtualKeyboardInit();
void virtualKeyboardShutdown();

void virtualKeyboardSetMode(VirtualKeyboardMode mode);
VirtualKeyboardMode virtualKeyboardGetMode();

// "auto", "system", or a layout id (see virtualKeyboardLayoutChoices)
void virtualKeyboardSetLayoutPreference(const std::string& preference);
const std::string& virtualKeyboardGetLayoutPreference();

// (preference value, display name) pairs for the layout options
std::vector<std::pair<std::string, WzString>> virtualKeyboardLayoutChoices();

// Call when the player did something to enter text (ex. opened the chat), just before code focuses an edit box.
// The next edit box to start editing within a moment counts as requested, as if the player had pressed it.
void virtualKeyboardRequestForNextTextEntry();

// Call once per frame, after game and widget code have run
void virtualKeyboardUpdate();

// Call once per frame, before game and widget code run (which may swallow the gamepad presses it uses)
void virtualKeyboardProcessGamepadInput();

bool virtualKeyboardIsVisible();

// Whether a point (in logical screen coordinates) is on the keyboard
bool virtualKeyboardContainsScreenPoint(int x, int y);

#endif // __INCLUDED_LIB_WIDGET_VIRTUALKEYBOARD_H__
