// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file
 *  SDL gamepad backend.
 */

#ifndef __INCLUDED_LIB_SDL_SDL_GAMEPAD_H__
#define __INCLUDED_LIB_SDL_SDL_GAMEPAD_H__

#include <SDL3/SDL_events.h>
#include "lib/framework/gamepad_input.h"

// Initializes the SDL gamepad subsystem if gamepad support is enabled
void wzGamepadInit();

// Closes any open devices and shuts down the SDL gamepad subsystem
void wzGamepadShutdown();

// Handles gamepad device and button events from the SDL event loop
void wzGamepadHandleSDLEvent(const SDL_Event& event);

// Per-frame axis polling and threshold button synthesis, called before the game loop runs
void wzGamepadUpdate();

// Ages pressed/released button states, called from inputNewFrame after the game loop runs
void wzGamepadNewFrame();

// Releases all buttons and zeroes all axes (e.g. on window focus loss)
void wzGamepadResetInputState();

// Clears the logical gamepad button states seen by game input handling, so
// remaining input processing this frame sees no gamepad buttons. The raw
// device state that drives click/key synthesis is unaffected
void wzGamepadClearButtonStates();

// Restores the logical state of held shoulder modifiers from the raw device
// state, the gamepad counterpart of restoring held keyboard meta keys
void wzGamepadRestoreMetaButtonState();

// Called from the SDL mouse/keyboard handlers when physical input arrives.
// If the gamepad was the active input source, this hands control back to the
// mouse/keyboard - warping the system pointer to the current cursor position -
// and returns true, meaning the caller should discard the event's stale position.
bool wzGamepadNotifyNonGamepadInput();

#endif // __INCLUDED_LIB_SDL_SDL_GAMEPAD_H__
