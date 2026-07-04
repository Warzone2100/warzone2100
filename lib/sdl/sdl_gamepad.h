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

// Applies the current gamepad mode setting, initializing or shutting down the
// subsystem as needed. Call after changing the mode at runtime
void wzGamepadApplyMode();

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

// Called from the SDL mouse/keyboard handlers when physical input arrives.
// If the gamepad was the active input source, this hands control back to the
// mouse/keyboard - warping the system pointer to the current cursor position -
// and returns true, meaning the caller should discard the event's stale position.
bool wzGamepadNotifyNonGamepadInput();

#endif // __INCLUDED_LIB_SDL_SDL_GAMEPAD_H__
