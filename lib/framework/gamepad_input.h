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
 *  Gamepad input state access functions.
 */

#ifndef __INCLUDED_LIB_FRAMEWORK_GAMEPAD_INPUT_H__
#define __INCLUDED_LIB_FRAMEWORK_GAMEPAD_INPUT_H__

enum GAMEPAD_INPUT
{
	// physical buttons, following the SDL gamepad layout
	GPAD_BTN_SOUTH,           // A on an Xbox-style pad
	GPAD_BTN_EAST,            // B
	GPAD_BTN_WEST,            // X
	GPAD_BTN_NORTH,           // Y
	GPAD_BTN_BACK,
	GPAD_BTN_GUIDE,
	GPAD_BTN_START,
	GPAD_BTN_LEFT_STICK,
	GPAD_BTN_RIGHT_STICK,
	GPAD_BTN_LEFT_SHOULDER,
	GPAD_BTN_RIGHT_SHOULDER,
	GPAD_BTN_DPAD_UP,
	GPAD_BTN_DPAD_DOWN,
	GPAD_BTN_DPAD_LEFT,
	GPAD_BTN_DPAD_RIGHT,

	// synthesized from a trigger crossing its press threshold
	GPAD_BTN_LEFT_TRIGGER,
	GPAD_BTN_RIGHT_TRIGGER,

	// synthesized from stick deflection crossing its press threshold
	GPAD_BTN_LSTICK_UP,
	GPAD_BTN_LSTICK_DOWN,
	GPAD_BTN_LSTICK_LEFT,
	GPAD_BTN_LSTICK_RIGHT,
	GPAD_BTN_RSTICK_UP,
	GPAD_BTN_RSTICK_DOWN,
	GPAD_BTN_RSTICK_LEFT,
	GPAD_BTN_RSTICK_RIGHT,

	GPAD_BTN_MAX
};

enum GAMEPAD_AXIS
{
	GPAD_AXIS_LEFT_X,
	GPAD_AXIS_LEFT_Y,
	GPAD_AXIS_RIGHT_X,
	GPAD_AXIS_RIGHT_Y,
	GPAD_AXIS_LEFT_TRIGGER,
	GPAD_AXIS_RIGHT_TRIGGER,

	GPAD_AXIS_MAX
};

// State queries. All return inert values (false or 0) when gamepad support is
// disabled or no controller is connected.
bool gamepadIsConnected();

// Whether the gamepad is the most recent input source (vs the mouse/keyboard)
bool isGamepadActiveInput();
bool gamepadButtonDown(GAMEPAD_INPUT button);      // held this frame
bool gamepadButtonPressed(GAMEPAD_INPUT button);   // went down this frame
bool gamepadButtonReleased(GAMEPAD_INPUT button);  // went up this frame

// Raw device state, unaffected by overlays swallowing the logical input state
bool gamepadButtonPhysicallyDown(GAMEPAD_INPUT button);

// Deadzone-adjusted axis value. Sticks are -1..1 and triggers are 0..1
float gamepadAxis(GAMEPAD_AXIS axis);

#endif // __INCLUDED_LIB_FRAMEWORK_GAMEPAD_INPUT_H__
