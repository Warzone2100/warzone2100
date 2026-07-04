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
#include "src/warzoneconfig.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>

#include <vector>
#include <utility>

static bool subsystemInitialized = false;
static std::vector<std::pair<SDL_JoystickID, SDL_Gamepad*>> openGamepads;
// All connected gamepads feed the same input state. The most recent device to
// send input is the one polled for axis values.
static SDL_JoystickID activeGamepadId = 0;
// Logical button state, read by the public accessors (and thus game input
// handling). Cleared when input is swallowed via inputLoseFocus
static INPUT_STATE aGamepadState[GPAD_BTN_MAX] = {};
// Raw device button state, feeding click/key synthesis. Only reset on focus
// loss or device removal, so held synthetic input always completes
static INPUT_STATE actualGamepadState[GPAD_BTN_MAX] = {};

bool gamepadIsConnected()
{
	return !openGamepads.empty();
}

bool gamepadButtonDown(GAMEPAD_INPUT button)
{
	ASSERT_OR_RETURN(false, button < GPAD_BTN_MAX, "Invalid gamepad button: %d", (int)button);
	return (aGamepadState[button].state != KEY_UP);
}

bool gamepadButtonPressed(GAMEPAD_INPUT button)
{
	ASSERT_OR_RETURN(false, button < GPAD_BTN_MAX, "Invalid gamepad button: %d", (int)button);
	return ((aGamepadState[button].state == KEY_PRESSED) || (aGamepadState[button].state == KEY_PRESSRELEASE));
}

bool gamepadButtonReleased(GAMEPAD_INPUT button)
{
	ASSERT_OR_RETURN(false, button < GPAD_BTN_MAX, "Invalid gamepad button: %d", (int)button);
	return ((aGamepadState[button].state == KEY_RELEASED) || (aGamepadState[button].state == KEY_PRESSRELEASE));
}

bool gamepadButtonPhysicallyDown(GAMEPAD_INPUT button)
{
	ASSERT_OR_RETURN(false, button < GPAD_BTN_MAX, "Invalid gamepad button: %d", (int)button);
	return (actualGamepadState[button].state != KEY_UP);
}

float gamepadAxis(GAMEPAD_AXIS axis)
{
	return 0.f;
}

static optional<GAMEPAD_INPUT> gamepadInputFromSDLButton(Uint8 sdlButton)
{
	switch (sdlButton)
	{
	case SDL_GAMEPAD_BUTTON_SOUTH: return GPAD_BTN_SOUTH;
	case SDL_GAMEPAD_BUTTON_EAST: return GPAD_BTN_EAST;
	case SDL_GAMEPAD_BUTTON_WEST: return GPAD_BTN_WEST;
	case SDL_GAMEPAD_BUTTON_NORTH: return GPAD_BTN_NORTH;
	case SDL_GAMEPAD_BUTTON_BACK: return GPAD_BTN_BACK;
	case SDL_GAMEPAD_BUTTON_GUIDE: return GPAD_BTN_GUIDE;
	case SDL_GAMEPAD_BUTTON_START: return GPAD_BTN_START;
	case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GPAD_BTN_LEFT_STICK;
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GPAD_BTN_RIGHT_STICK;
	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GPAD_BTN_LEFT_SHOULDER;
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GPAD_BTN_RIGHT_SHOULDER;
	case SDL_GAMEPAD_BUTTON_DPAD_UP: return GPAD_BTN_DPAD_UP;
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GPAD_BTN_DPAD_DOWN;
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GPAD_BTN_DPAD_LEFT;
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GPAD_BTN_DPAD_RIGHT;
	default: return nullopt;
	}
}

static void applyButtonTransition(INPUT_STATE& state, bool pressed)
{
	if (pressed)
	{
		if (state.state == KEY_UP ||
		    state.state == KEY_RELEASED ||
		    state.state == KEY_PRESSRELEASE)
		{
			state.state = KEY_PRESSED;
			state.lastdown = 0;
		}
	}
	else
	{
		if (state.state == KEY_PRESSED)
		{
			state.state = KEY_PRESSRELEASE;
		}
		else if (state.state == KEY_DOWN)
		{
			state.state = KEY_RELEASED;
		}
	}
}

static void setGamepadButtonState(GAMEPAD_INPUT button, bool pressed)
{
	applyButtonTransition(aGamepadState[button], pressed);
	applyButtonTransition(actualGamepadState[button], pressed);
}

static void openGamepadDevice(SDL_JoystickID id)
{
	for (const auto& pad : openGamepads)
	{
		if (pad.first == id)
		{
			return;
		}
	}
	SDL_Gamepad* pad = SDL_OpenGamepad(id);
	if (!pad)
	{
		debug(LOG_INFO, "Failed to open gamepad (id %u): %s", (unsigned)id, SDL_GetError());
		return;
	}
	openGamepads.emplace_back(id, pad);
	if (activeGamepadId == 0)
	{
		activeGamepadId = id;
	}
	debug(LOG_INFO, "Gamepad connected: %s (id %u)", SDL_GetGamepadName(pad), (unsigned)id);
}

static void closeGamepadDevice(SDL_JoystickID id)
{
	for (auto it = openGamepads.begin(); it != openGamepads.end(); ++it)
	{
		if (it->first == id)
		{
			SDL_CloseGamepad(it->second);
			openGamepads.erase(it);
			debug(LOG_INFO, "Gamepad disconnected (id %u)", (unsigned)id);
			break;
		}
	}
	if (activeGamepadId == id)
	{
		activeGamepadId = openGamepads.empty() ? 0 : openGamepads.front().first;
	}
	if (openGamepads.empty())
	{
		wzGamepadResetInputState();
	}
}

void wzGamepadInit()
{
	if (war_GetGamepadMode() == GamepadMode::Disabled)
	{
		return;
	}
	if (subsystemInitialized)
	{
		return;
	}
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	if (!wzSDLOneTimeInitSubsystem(SDL_INIT_GAMEPAD))
	{
		debug(LOG_WARNING, "Failed to initialize SDL gamepad subsystem: %s", SDL_GetError());
		return;
	}
	subsystemInitialized = true;
	// devices connected right now arrive as SDL_EVENT_GAMEPAD_ADDED events
	debug(LOG_INPUT, "SDL gamepad subsystem initialized");
}

void wzGamepadShutdown()
{
	for (const auto& pad : openGamepads)
	{
		SDL_CloseGamepad(pad.second);
	}
	openGamepads.clear();
	activeGamepadId = 0;
	wzGamepadResetInputState();
	if (subsystemInitialized)
	{
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
		subsystemInitialized = false;
	}
}

void wzGamepadApplyMode()
{
	if (war_GetGamepadMode() == GamepadMode::Disabled)
	{
		wzGamepadShutdown();
	}
	else
	{
		wzGamepadInit();
	}
}

void wzGamepadHandleSDLEvent(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_EVENT_GAMEPAD_ADDED:
		openGamepadDevice(event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		closeGamepadDevice(event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
	{
		const auto button = gamepadInputFromSDLButton(event.gbutton.button);
		if (!button.has_value())
		{
			break;
		}
		const bool pressed = (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
		setGamepadButtonState(button.value(), pressed);
		activeGamepadId = event.gbutton.which;
		debug(LOG_INPUT, "Gamepad button %d %s", (int)button.value(), pressed ? "pressed" : "released");
		break;
	}
	default:
		break;
	}
}

void wzGamepadUpdate()
{
}

void wzGamepadNewFrame()
{
}

void wzGamepadResetInputState()
{
	for (unsigned int i = 0; i < GPAD_BTN_MAX; ++i)
	{
		aGamepadState[i].state = KEY_UP;
		aGamepadState[i].lastdown = 0;
		actualGamepadState[i].state = KEY_UP;
		actualGamepadState[i].lastdown = 0;
	}
}

void wzGamepadFlushPendingEvents()
{
}
