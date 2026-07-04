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
 *  SDL gamepad backend implementation.
 */

#include "lib/framework/frame.h"
#include "sdl_gamepad.h"
#include "sdl_backend_private.h"

bool gamepadIsConnected()
{
	return false;
}

bool gamepadButtonDown(GAMEPAD_INPUT button)
{
	return false;
}

bool gamepadButtonPressed(GAMEPAD_INPUT button)
{
	return false;
}

bool gamepadButtonReleased(GAMEPAD_INPUT button)
{
	return false;
}

float gamepadAxis(GAMEPAD_AXIS axis)
{
	return 0.f;
}

void wzGamepadInit()
{
}

void wzGamepadShutdown()
{
}

void wzGamepadHandleSDLEvent(const SDL_Event& event)
{
}

void wzGamepadUpdate()
{
}

void wzGamepadNewFrame()
{
}

void wzGamepadResetInputState()
{
}

void wzGamepadFlushPendingEvents()
{
}
