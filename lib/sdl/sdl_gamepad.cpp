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
#include "lib/ivis_opengl/screen.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>

// Radial deadzone applied to stick deflection before values are exposed
static const float GAMEPAD_STICK_DEADZONE = 0.15f;
// Cursor speed at full stick deflection, in logical pixels per second
static const float GAMEPAD_CURSOR_SPEED = 800.f;
// Cursor speed multiplier while the left stick is clicked in
static const float GAMEPAD_CURSOR_PRECISION_FACTOR = 0.5f;
// Axis thresholds for the synthesized trigger and stick-direction buttons, with hysteresis
static const float GAMEPAD_AXIS_BUTTON_PRESS_THRESHOLD = 0.5f;
static const float GAMEPAD_AXIS_BUTTON_RELEASE_THRESHOLD = 0.45f;

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
static bool gamepadIsActiveInput = false;
// Cursor position with subpixel precision, valid while the gamepad is the active input source
static float virtualCursorX = 0.f;
static float virtualCursorY = 0.f;
static float aGamepadAxisValues[GPAD_AXIS_MAX] = {};
// Hysteresis latches for the buttons synthesized from axis thresholds
static bool axisButtonHeld[GPAD_BTN_MAX] = {};
static float lastLoggedAxisValues[GPAD_AXIS_MAX] = {};

bool gamepadIsConnected()
{
	return !openGamepads.empty();
}

bool isGamepadActiveInput()
{
	return gamepadIsActiveInput;
}

static void markGamepadActive()
{
	if (!gamepadIsActiveInput)
	{
		gamepadIsActiveInput = true;
		virtualCursorX = (float)mouseX();
		virtualCursorY = (float)mouseY();
		debug(LOG_INPUT, "Gamepad is now the active input source");
	}
}

bool wzGamepadNotifyNonGamepadInput()
{
	if (!gamepadIsActiveInput)
	{
		return false;
	}
	gamepadIsActiveInput = false;
	// hand the pointer back where the cursor currently is, so the switch is seamless
	wzWarpMouseToLogicalPos(mouseX(), mouseY());
	debug(LOG_INPUT, "Mouse/keyboard is now the active input source");
	return true;
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
	ASSERT_OR_RETURN(0.f, axis < GPAD_AXIS_MAX, "Invalid gamepad axis: %d", (int)axis);
	return aGamepadAxisValues[axis];
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
		wzGamepadNotifyNonGamepadInput();
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
		markGamepadActive();
		debug(LOG_INPUT, "Gamepad button %d %s", (int)button.value(), pressed ? "pressed" : "released");
		break;
	}
	default:
		break;
	}
}

static SDL_Gamepad* activeGamepadHandle()
{
	for (const auto& pad : openGamepads)
	{
		if (pad.first == activeGamepadId)
		{
			return pad.second;
		}
	}
	return nullptr;
}

// Applies a radial deadzone to a stick, rescaling the remaining range to 0..1
static void applyStickDeadzone(float rawX, float rawY, GAMEPAD_AXIS axisX, GAMEPAD_AXIS axisY)
{
	const float magnitude = std::sqrt(rawX * rawX + rawY * rawY);
	if (magnitude < GAMEPAD_STICK_DEADZONE)
	{
		aGamepadAxisValues[axisX] = 0.f;
		aGamepadAxisValues[axisY] = 0.f;
		return;
	}
	const float scale = ((magnitude - GAMEPAD_STICK_DEADZONE) / (1.f - GAMEPAD_STICK_DEADZONE)) / magnitude;
	aGamepadAxisValues[axisX] = std::clamp(rawX * scale, -1.f, 1.f);
	aGamepadAxisValues[axisY] = std::clamp(rawY * scale, -1.f, 1.f);
}

static void updateAxisButton(GAMEPAD_INPUT button, float deflection)
{
	if (!axisButtonHeld[button] && deflection >= GAMEPAD_AXIS_BUTTON_PRESS_THRESHOLD)
	{
		axisButtonHeld[button] = true;
		setGamepadButtonState(button, true);
		debug(LOG_INPUT, "Gamepad axis button %d pressed", (int)button);
	}
	else if (axisButtonHeld[button] && deflection < GAMEPAD_AXIS_BUTTON_RELEASE_THRESHOLD)
	{
		axisButtonHeld[button] = false;
		setGamepadButtonState(button, false);
		debug(LOG_INPUT, "Gamepad axis button %d released", (int)button);
	}
}

// Moves the cursor from left-stick deflection while the gamepad is the active input source
static void updateVirtualCursor()
{
	static Uint64 lastTick = 0;
	const Uint64 now = SDL_GetTicks();
	// clamp dt so a long stall cannot fling the cursor
	const float dt = (lastTick != 0) ? std::min((float)(now - lastTick) / 1000.f, 0.1f) : 0.f;
	lastTick = now;

	if (!gamepadIsActiveInput)
	{
		return;
	}

	const float deflectionX = aGamepadAxisValues[GPAD_AXIS_LEFT_X];
	const float deflectionY = aGamepadAxisValues[GPAD_AXIS_LEFT_Y];
	if (deflectionX == 0.f && deflectionY == 0.f)
	{
		return;
	}

	float speed = GAMEPAD_CURSOR_SPEED;
	if (gamepadButtonDown(GPAD_BTN_LEFT_STICK))
	{
		speed *= GAMEPAD_CURSOR_PRECISION_FACTOR;
	}

	// quadratic response gives finer control near the center
	virtualCursorX += deflectionX * std::abs(deflectionX) * speed * dt;
	virtualCursorY += deflectionY * std::abs(deflectionY) * speed * dt;
	virtualCursorX = std::clamp(virtualCursorX, 0.f, (float)(screenWidth > 0 ? screenWidth - 1 : 0));
	virtualCursorY = std::clamp(virtualCursorY, 0.f, (float)(screenHeight > 0 ? screenHeight - 1 : 0));

	inputSetMousePos((int)std::lround(virtualCursorX), (int)std::lround(virtualCursorY));
}

void wzGamepadUpdate()
{
	SDL_Gamepad* pad = activeGamepadHandle();
	if (!pad)
	{
		return;
	}

	const auto rawAxis = [pad](SDL_GamepadAxis axis) -> float {
		return std::clamp((float)SDL_GetGamepadAxis(pad, axis) / 32767.f, -1.f, 1.f);
	};

	applyStickDeadzone(rawAxis(SDL_GAMEPAD_AXIS_LEFTX), rawAxis(SDL_GAMEPAD_AXIS_LEFTY), GPAD_AXIS_LEFT_X, GPAD_AXIS_LEFT_Y);
	applyStickDeadzone(rawAxis(SDL_GAMEPAD_AXIS_RIGHTX), rawAxis(SDL_GAMEPAD_AXIS_RIGHTY), GPAD_AXIS_RIGHT_X, GPAD_AXIS_RIGHT_Y);
	aGamepadAxisValues[GPAD_AXIS_LEFT_TRIGGER] = std::clamp(rawAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER), 0.f, 1.f);
	aGamepadAxisValues[GPAD_AXIS_RIGHT_TRIGGER] = std::clamp(rawAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER), 0.f, 1.f);

	updateAxisButton(GPAD_BTN_LEFT_TRIGGER, aGamepadAxisValues[GPAD_AXIS_LEFT_TRIGGER]);
	updateAxisButton(GPAD_BTN_RIGHT_TRIGGER, aGamepadAxisValues[GPAD_AXIS_RIGHT_TRIGGER]);
	// positive stick Y is downwards
	updateAxisButton(GPAD_BTN_LSTICK_UP, -aGamepadAxisValues[GPAD_AXIS_LEFT_Y]);
	updateAxisButton(GPAD_BTN_LSTICK_DOWN, aGamepadAxisValues[GPAD_AXIS_LEFT_Y]);
	updateAxisButton(GPAD_BTN_LSTICK_LEFT, -aGamepadAxisValues[GPAD_AXIS_LEFT_X]);
	updateAxisButton(GPAD_BTN_LSTICK_RIGHT, aGamepadAxisValues[GPAD_AXIS_LEFT_X]);
	updateAxisButton(GPAD_BTN_RSTICK_UP, -aGamepadAxisValues[GPAD_AXIS_RIGHT_Y]);
	updateAxisButton(GPAD_BTN_RSTICK_DOWN, aGamepadAxisValues[GPAD_AXIS_RIGHT_Y]);
	updateAxisButton(GPAD_BTN_RSTICK_LEFT, -aGamepadAxisValues[GPAD_AXIS_RIGHT_X]);
	updateAxisButton(GPAD_BTN_RSTICK_RIGHT, aGamepadAxisValues[GPAD_AXIS_RIGHT_X]);

	for (unsigned int i = 0; i < GPAD_AXIS_MAX; ++i)
	{
		if (aGamepadAxisValues[i] != 0.f)
		{
			markGamepadActive();
		}
		if (std::abs(aGamepadAxisValues[i] - lastLoggedAxisValues[i]) > 0.05f)
		{
			debug(LOG_INPUT, "Gamepad axis %u: %.2f", i, aGamepadAxisValues[i]);
			lastLoggedAxisValues[i] = aGamepadAxisValues[i];
		}
	}

	updateVirtualCursor();
}

static void ageButtonStates(INPUT_STATE* states)
{
	for (unsigned int i = 0; i < GPAD_BTN_MAX; ++i)
	{
		if (states[i].state == KEY_PRESSED)
		{
			states[i].state = KEY_DOWN;
		}
		else if (states[i].state == KEY_RELEASED ||
		         states[i].state == KEY_PRESSRELEASE)
		{
			states[i].state = KEY_UP;
		}
	}
}

void wzGamepadNewFrame()
{
	ageButtonStates(aGamepadState);
	ageButtonStates(actualGamepadState);
}

void wzGamepadClearButtonStates()
{
	for (unsigned int i = 0; i < GPAD_BTN_MAX; ++i)
	{
		aGamepadState[i].state = KEY_UP;
		aGamepadState[i].lastdown = 0;
	}
}

void wzGamepadResetInputState()
{
	wzGamepadClearButtonStates();
	for (unsigned int i = 0; i < GPAD_BTN_MAX; ++i)
	{
		actualGamepadState[i].state = KEY_UP;
		actualGamepadState[i].lastdown = 0;
		axisButtonHeld[i] = false;
	}
	for (unsigned int i = 0; i < GPAD_AXIS_MAX; ++i)
	{
		aGamepadAxisValues[i] = 0.f;
	}
}

