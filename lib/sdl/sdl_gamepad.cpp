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
#include "lib/framework/wzapp.h"
#include "lib/framework/file.h"
#include "lib/framework/input.h"
#include "sdl_gamepad.h"
#include "sdl_backend_private.h"
#include "src/warzoneconfig.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"

#include "controllerimage.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <vector>
#include <utility>
#include <cmath>
#include <cstring>
#include <algorithm>

// Widget scroll speed at full stick deflection, in wheel units per second
static const float GAMEPAD_SCROLL_SPEED = 18.f;
// Cursor speed multiplier while the left stick is clicked in
static const float GAMEPAD_CURSOR_PRECISION_FACTOR = 0.5f;
// Cursor magnetism tuning. The assist reaches full strength below the first
// deflection and fades to nothing at the second, so full-tilt travel across
// other objects is never slowed or pulled
static const float GAMEPAD_MAGNET_FULL_ASSIST_DEFLECTION = 0.4f;
static const float GAMEPAD_MAGNET_NO_ASSIST_DEFLECTION = 0.85f;
static const float GAMEPAD_MAGNET_MAX_FRICTION = 0.45f;
static const float GAMEPAD_MAGNET_MAX_PULL_FRACTION = 0.5f;
static const float GAMEPAD_MAGNET_CAPTURE_SLACK = 24.f;
static const float GAMEPAD_MAGNET_SETTLE_RADIUS = 20.f;
static const float GAMEPAD_MAGNET_SETTLE_SPEED = 280.f;
static const uint32_t GAMEPAD_MAGNET_SETTLE_WINDOW_MS = 250;
// A quick press-and-release of the cursor stick counts as a flick, which
// requests a hop to the next widget in the flicked direction. Once deflection
// falls below this fraction of the gesture's peak the stick counts as
// releasing, and the remaining travel is banked
static const float GAMEPAD_FLICK_MIN_DEFLECTION = 0.6f;
static const uint32_t GAMEPAD_FLICK_MAX_MS = 180;
static const float GAMEPAD_FLICK_RELEASE_FRACTION = 0.7f;
static const float GAMEPAD_HOP_SPEED = 2200.f;
// Press threshold for the synthesized stick-direction buttons - triggers use
// the configured threshold instead
static const float GAMEPAD_AXIS_BUTTON_PRESS_THRESHOLD = 0.5f;
// Release thresholds sit this far below the press threshold
static const float GAMEPAD_AXIS_BUTTON_HYSTERESIS = 0.05f;

static bool subsystemInitialized = false;
struct OpenGamepad
{
	SDL_JoystickID id;
	SDL_Gamepad* handle;
	// queried once at open - a device's rumble capability never changes
	bool hasRumble;
};
static std::vector<OpenGamepad> openGamepads;
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
static bool rightStickScrollingWidget = false;
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

bool gamepadRightStickConsumedByUI()
{
	return rightStickScrollingWidget;
}

static bool captureModeActive = false;

void gamepadSetCaptureMode(bool enabled)
{
	captureModeActive = enabled;
}

bool gamepadInCaptureMode()
{
	return captureModeActive;
}

static bool magnetTargetFresh = false;
static float magnetTargetX = 0.f;
static float magnetTargetY = 0.f;
static float magnetTargetRadius = 0.f;

// A detected flick awaiting consumption, and a hop glide in progress
static bool flickPending = false;
static float flickDirX = 0.f;
static float flickDirY = 0.f;
static int flickStartPosX = 0;
static int flickStartPosY = 0;

// The movement gesture being watched for a flick
static uint32_t flickStartTick = 0;
static float flickPeakDeflection = 0.f;
static float flickPeakDirX = 0.f;
static float flickPeakDirY = 0.f;
static float flickStartCursorX = 0.f;
static float flickStartCursorY = 0.f;

// This frame's prospective hop target - the cursor parks on it while the
// gesture may still become a flick, so an actual flick lands with no jump
static bool flickAnchorFresh = false;
static float flickAnchorX = 0.f;
static float flickAnchorY = 0.f;
static bool hopActive = false;
static float hopTargetX = 0.f;
static float hopTargetY = 0.f;
// Travel banked while the stick snaps back from a potential flick - dropped
// when a hop claims it, drained back into the cursor otherwise
static float bankedTravelX = 0.f;
static float bankedTravelY = 0.f;
static bool bankAwaitingHop = false;

void gamepadSetCursorMagnetTarget(int screenX, int screenY, int screenRadius)
{
	magnetTargetFresh = true;
	magnetTargetX = (float)screenX;
	magnetTargetY = (float)screenY;
	magnetTargetRadius = (float)std::max(screenRadius, 1);
}

static void markGamepadActive()
{
	if (!gamepadIsActiveInput)
	{
		gamepadIsActiveInput = true;
		virtualCursorX = (float)mouseX();
		virtualCursorY = (float)mouseY();
		// the cursor is drawn as part of the frame while the gamepad is active
		wzShowMouse(false);
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
	wzShowMouse(true);
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

static SDL_Gamepad* displayGamepadHandle();
static void shutdownGlyphRenderer();

const char* gamepadButtonName(GAMEPAD_INPUT button)
{
	// face buttons take the connected controller's labels, so an Xbox pad
	// reads A/B/X/Y and a PlayStation pad reads Cross/Circle/Square/Triangle
	if (button == GPAD_BTN_SOUTH || button == GPAD_BTN_EAST || button == GPAD_BTN_WEST || button == GPAD_BTN_NORTH)
	{
		SDL_Gamepad* pad = displayGamepadHandle();
		if (pad)
		{
			SDL_GamepadButton sdlButton = SDL_GAMEPAD_BUTTON_SOUTH;
			switch (button)
			{
			case GPAD_BTN_EAST: sdlButton = SDL_GAMEPAD_BUTTON_EAST; break;
			case GPAD_BTN_WEST: sdlButton = SDL_GAMEPAD_BUTTON_WEST; break;
			case GPAD_BTN_NORTH: sdlButton = SDL_GAMEPAD_BUTTON_NORTH; break;
			default: break;
			}
			switch (SDL_GetGamepadButtonLabel(pad, sdlButton))
			{
			case SDL_GAMEPAD_BUTTON_LABEL_A: return "A";
			case SDL_GAMEPAD_BUTTON_LABEL_B: return "B";
			case SDL_GAMEPAD_BUTTON_LABEL_X: return "X";
			case SDL_GAMEPAD_BUTTON_LABEL_Y: return "Y";
			case SDL_GAMEPAD_BUTTON_LABEL_CROSS: return "Cross";
			case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE: return "Circle";
			case SDL_GAMEPAD_BUTTON_LABEL_SQUARE: return "Square";
			case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE: return "Triangle";
			default: break;
			}
		}
	}

	switch (button)
	{
	case GPAD_BTN_SOUTH: return "A";
	case GPAD_BTN_EAST: return "B";
	case GPAD_BTN_WEST: return "X";
	case GPAD_BTN_NORTH: return "Y";
	case GPAD_BTN_BACK: return "Back";
	case GPAD_BTN_GUIDE: return "Guide";
	case GPAD_BTN_START: return "Start";
	case GPAD_BTN_LEFT_STICK: return "L3";
	case GPAD_BTN_RIGHT_STICK: return "R3";
	case GPAD_BTN_LEFT_SHOULDER: return "LB";
	case GPAD_BTN_RIGHT_SHOULDER: return "RB";
	case GPAD_BTN_DPAD_UP: return "D-Pad Up";
	case GPAD_BTN_DPAD_DOWN: return "D-Pad Down";
	case GPAD_BTN_DPAD_LEFT: return "D-Pad Left";
	case GPAD_BTN_DPAD_RIGHT: return "D-Pad Right";
	case GPAD_BTN_LEFT_TRIGGER: return "LT";
	case GPAD_BTN_RIGHT_TRIGGER: return "RT";
	case GPAD_BTN_LSTICK_UP: return "LS Up";
	case GPAD_BTN_LSTICK_DOWN: return "LS Down";
	case GPAD_BTN_LSTICK_LEFT: return "LS Left";
	case GPAD_BTN_LSTICK_RIGHT: return "LS Right";
	case GPAD_BTN_RSTICK_UP: return "RS Up";
	case GPAD_BTN_RSTICK_DOWN: return "RS Down";
	case GPAD_BTN_RSTICK_LEFT: return "RS Left";
	case GPAD_BTN_RSTICK_RIGHT: return "RS Right";
	case GPAD_BTN_MAX: break;
	}
	return "?";
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
	// stick clicks follow the swap-sticks setting so the cursor stick's click
	// always maps to the logical left stick button
	case SDL_GAMEPAD_BUTTON_LEFT_STICK: return war_GetGamepadSwapSticks() ? GPAD_BTN_RIGHT_STICK : GPAD_BTN_LEFT_STICK;
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return war_GetGamepadSwapSticks() ? GPAD_BTN_LEFT_STICK : GPAD_BTN_RIGHT_STICK;
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

// Raw device state queries for the synthesis layer, unaffected by input swallowing
static bool actualButtonDown(GAMEPAD_INPUT button)
{
	return (actualGamepadState[button].state != KEY_UP);
}

static bool actualButtonPressed(GAMEPAD_INPUT button)
{
	return ((actualGamepadState[button].state == KEY_PRESSED) || (actualGamepadState[button].state == KEY_PRESSRELEASE));
}

static bool actualButtonReleased(GAMEPAD_INPUT button)
{
	return ((actualGamepadState[button].state == KEY_RELEASED) || (actualGamepadState[button].state == KEY_PRESSRELEASE));
}

static bool padHasRumble(SDL_Gamepad* pad)
{
	return SDL_GetBooleanProperty(SDL_GetGamepadProperties(pad), SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
}

static void openGamepadDevice(SDL_JoystickID id)
{
	for (const auto& pad : openGamepads)
	{
		if (pad.id == id)
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
	const bool hasRumble = padHasRumble(pad);
	openGamepads.push_back({id, pad, hasRumble});
	if (activeGamepadId == 0)
	{
		activeGamepadId = id;
	}
	debug(LOG_INFO, "Gamepad connected: %s (id %u)%s", SDL_GetGamepadName(pad), (unsigned)id, hasRumble ? "" : ", no rumble support");
}

static void closeGamepadDevice(SDL_JoystickID id)
{
	for (auto it = openGamepads.begin(); it != openGamepads.end(); ++it)
	{
		if (it->id == id)
		{
			SDL_CloseGamepad(it->handle);
			openGamepads.erase(it);
			debug(LOG_INFO, "Gamepad disconnected (id %u)", (unsigned)id);
			break;
		}
	}
	if (activeGamepadId == id)
	{
		activeGamepadId = openGamepads.empty() ? 0 : openGamepads.front().id;
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
		SDL_CloseGamepad(pad.handle);
	}
	openGamepads.clear();
	activeGamepadId = 0;
	wzGamepadResetInputState();
	shutdownGlyphRenderer();
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

static const OpenGamepad* activeGamepadEntry()
{
	for (const auto& pad : openGamepads)
	{
		if (pad.id == activeGamepadId)
		{
			return &pad;
		}
	}
	return nullptr;
}

// The device that labels and glyphs describe - the active pad, or the first
// connected one before any gamepad input arrives
static const OpenGamepad* displayGamepadEntry()
{
	if (const OpenGamepad* pad = activeGamepadEntry())
	{
		return pad;
	}
	return openGamepads.empty() ? nullptr : &openGamepads.front();
}

static SDL_Gamepad* displayGamepadHandle()
{
	const OpenGamepad* pad = displayGamepadEntry();
	return pad ? pad->handle : nullptr;
}

// ControllerImage glyph rendering, initialized on first glyph request so the
// data file loads after the game data is mounted
static bool glyphRendererInitialized = false;
static bool glyphDataLoadAttempted = false;
static bool glyphDataLoaded = false;
static ControllerImage_Device* glyphDevice = nullptr;
static SDL_JoystickID glyphDeviceId = 0;

static bool ensureGlyphData()
{
	if (!subsystemInitialized)
	{
		return false;
	}
	if (!glyphRendererInitialized)
	{
		if (!ControllerImage_Init())
		{
			debug(LOG_WARNING, "ControllerImage_Init failed: %s", SDL_GetError());
			return false;
		}
		glyphRendererInitialized = true;
	}
	if (!glyphDataLoadAttempted)
	{
		glyphDataLoadAttempted = true;
		std::vector<char> data;
		if (loadFileToBufferVector("images/controllerimage-standard.bin", data, false, false))
		{
			glyphDataLoaded = ControllerImage_AddData(data.data(), data.size());
			if (!glyphDataLoaded)
			{
				debug(LOG_WARNING, "Loading controller glyph data failed: %s", SDL_GetError());
			}
		}
		else
		{
			debug(LOG_WARNING, "Controller glyph data file not found: images/controllerimage-standard.bin");
		}
	}
	return glyphDataLoaded;
}

static ControllerImage_Device* glyphDeviceForCurrentPad()
{
	SDL_Gamepad* pad = displayGamepadHandle();
	if (!pad)
	{
		return nullptr;
	}
	const SDL_JoystickID id = SDL_GetGamepadID(pad);
	if (glyphDevice && glyphDeviceId == id)
	{
		return glyphDevice;
	}
	if (glyphDevice)
	{
		ControllerImage_DestroyDevice(glyphDevice);
		glyphDevice = nullptr;
	}
	glyphDevice = ControllerImage_CreateGamepadDevice(pad);
	glyphDeviceId = id;
	if (!glyphDevice)
	{
		debug(LOG_INFO, "No controller glyphs for %s: %s", SDL_GetGamepadName(pad), SDL_GetError());
	}
	return glyphDevice;
}

static void shutdownGlyphRenderer()
{
	if (glyphDevice)
	{
		ControllerImage_DestroyDevice(glyphDevice);
		glyphDevice = nullptr;
	}
	glyphDeviceId = 0;
	ControllerImage_Quit();
	glyphRendererInitialized = false;
	glyphDataLoadAttempted = false;
	glyphDataLoaded = false;
}

static optional<SDL_GamepadButton> sdlButtonForGamepadInput(GAMEPAD_INPUT button)
{
	switch (button)
	{
	case GPAD_BTN_SOUTH: return SDL_GAMEPAD_BUTTON_SOUTH;
	case GPAD_BTN_EAST: return SDL_GAMEPAD_BUTTON_EAST;
	case GPAD_BTN_WEST: return SDL_GAMEPAD_BUTTON_WEST;
	case GPAD_BTN_NORTH: return SDL_GAMEPAD_BUTTON_NORTH;
	case GPAD_BTN_BACK: return SDL_GAMEPAD_BUTTON_BACK;
	case GPAD_BTN_GUIDE: return SDL_GAMEPAD_BUTTON_GUIDE;
	case GPAD_BTN_START: return SDL_GAMEPAD_BUTTON_START;
	// stick clicks resolve to the physical stick they live on under the
	// swap-sticks setting, mirroring the event translation
	case GPAD_BTN_LEFT_STICK: return war_GetGamepadSwapSticks() ? SDL_GAMEPAD_BUTTON_RIGHT_STICK : SDL_GAMEPAD_BUTTON_LEFT_STICK;
	case GPAD_BTN_RIGHT_STICK: return war_GetGamepadSwapSticks() ? SDL_GAMEPAD_BUTTON_LEFT_STICK : SDL_GAMEPAD_BUTTON_RIGHT_STICK;
	case GPAD_BTN_LEFT_SHOULDER: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
	case GPAD_BTN_RIGHT_SHOULDER: return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
	case GPAD_BTN_DPAD_UP: return SDL_GAMEPAD_BUTTON_DPAD_UP;
	case GPAD_BTN_DPAD_DOWN: return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
	case GPAD_BTN_DPAD_LEFT: return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
	case GPAD_BTN_DPAD_RIGHT: return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
	default: return nullopt;
	}
}

const char* gamepadDeviceName()
{
	SDL_Gamepad* pad = displayGamepadHandle();
	const char* name = pad ? SDL_GetGamepadName(pad) : nullptr;
	return name ? name : "";
}

const char* gamepadDeviceGUID()
{
	static char guidString[33] = {0};
	SDL_Gamepad* pad = displayGamepadHandle();
	if (!pad)
	{
		return "";
	}
	const SDL_GUID guid = SDL_GetJoystickGUIDForID(SDL_GetGamepadID(pad));
	SDL_GUIDToString(guid, guidString, sizeof(guidString));
	return guidString;
}

bool gamepadHasRumble()
{
	const OpenGamepad* pad = displayGamepadEntry();
	return pad != nullptr && pad->hasRumble;
}

void gamepadRumble(float lowFrequencyStrength, float highFrequencyStrength, unsigned int durationMs)
{
	if (!war_GetGamepadRumble() || !gamepadIsActiveInput)
	{
		return;
	}
	const OpenGamepad* pad = displayGamepadEntry();
	if (!pad || !pad->hasRumble)
	{
		return;
	}
	const Uint16 low = (Uint16)(std::clamp(lowFrequencyStrength, 0.f, 1.f) * 65535.f);
	const Uint16 high = (Uint16)(std::clamp(highFrequencyStrength, 0.f, 1.f) * 65535.f);
	if (!SDL_RumbleGamepad(pad->handle, low, high, durationMs))
	{
		// a pad advertising rumble but rejecting it is worth diagnosing, but
		// only log the first failure per device to avoid spam
		static SDL_JoystickID lastFailedRumbleId = 0;
		if (lastFailedRumbleId != pad->id)
		{
			lastFailedRumbleId = pad->id;
			debug(LOG_INFO, "SDL_RumbleGamepad failed for %s: %s", SDL_GetGamepadName(pad->handle), SDL_GetError());
		}
	}
}

bool gamepadGetButtonGlyph(GAMEPAD_INPUT button, unsigned int size, std::vector<unsigned char>& outRGBA, unsigned int& outWidth, unsigned int& outHeight)
{
	if (!ensureGlyphData())
	{
		return false;
	}
	ControllerImage_Device* device = glyphDeviceForCurrentPad();
	if (!device)
	{
		return false;
	}

	// stick glyphs show the physical stick the logical input lives on, so
	// they stay accurate under the swap-sticks setting
	const bool swapSticks = war_GetGamepadSwapSticks();
	SDL_Surface* surface = nullptr;
	switch (button)
	{
	case GPAD_BTN_LEFT_TRIGGER:
		surface = ControllerImage_CreateSurfaceForAxis(device, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, (int)size);
		break;
	case GPAD_BTN_RIGHT_TRIGGER:
		surface = ControllerImage_CreateSurfaceForAxis(device, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, (int)size);
		break;
	case GPAD_BTN_LSTICK_UP:
	case GPAD_BTN_LSTICK_DOWN:
	case GPAD_BTN_LSTICK_LEFT:
	case GPAD_BTN_LSTICK_RIGHT:
		surface = ControllerImage_CreateSurfaceForAxis(device, swapSticks ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_LEFTX, (int)size);
		break;
	case GPAD_BTN_RSTICK_UP:
	case GPAD_BTN_RSTICK_DOWN:
	case GPAD_BTN_RSTICK_LEFT:
	case GPAD_BTN_RSTICK_RIGHT:
		surface = ControllerImage_CreateSurfaceForAxis(device, swapSticks ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX, (int)size);
		break;
	default:
	{
		const auto sdlButton = sdlButtonForGamepadInput(button);
		if (!sdlButton.has_value())
		{
			return false;
		}
		surface = ControllerImage_CreateSurfaceForButton(device, sdlButton.value(), (int)size);
		break;
	}
	}
	if (!surface)
	{
		return false;
	}

	SDL_Surface* converted = surface;
	if (surface->format != SDL_PIXELFORMAT_RGBA32)
	{
		converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		if (!converted)
		{
			return false;
		}
	}
	outWidth = (unsigned int)converted->w;
	outHeight = (unsigned int)converted->h;
	outRGBA.resize((size_t)outWidth * outHeight * 4);
	const unsigned char* srcPixels = (const unsigned char*)converted->pixels;
	for (unsigned int row = 0; row < outHeight; ++row)
	{
		memcpy(&outRGBA[(size_t)row * outWidth * 4], srcPixels + (size_t)row * converted->pitch, (size_t)outWidth * 4);
	}
	SDL_DestroySurface(converted);
	return true;
}

// Applies a radial deadzone to a stick, rescaling the remaining range to 0..1
static void applyStickDeadzone(float rawX, float rawY, GAMEPAD_AXIS axisX, GAMEPAD_AXIS axisY)
{
	const float deadzone = (float)war_GetGamepadStickDeadzone() / 100.f;
	const float magnitude = std::sqrt(rawX * rawX + rawY * rawY);
	if (magnitude < deadzone)
	{
		aGamepadAxisValues[axisX] = 0.f;
		aGamepadAxisValues[axisY] = 0.f;
		return;
	}
	const float scale = ((magnitude - deadzone) / (1.f - deadzone)) / magnitude;
	aGamepadAxisValues[axisX] = std::clamp(rawX * scale, -1.f, 1.f);
	aGamepadAxisValues[axisY] = std::clamp(rawY * scale, -1.f, 1.f);
}

static void updateAxisButton(GAMEPAD_INPUT button, float deflection)
{
	const bool isTrigger = (button == GPAD_BTN_LEFT_TRIGGER || button == GPAD_BTN_RIGHT_TRIGGER);
	const float pressThreshold = isTrigger ? (float)war_GetGamepadTriggerThreshold() / 100.f : GAMEPAD_AXIS_BUTTON_PRESS_THRESHOLD;
	if (!axisButtonHeld[button] && deflection >= pressThreshold)
	{
		axisButtonHeld[button] = true;
		setGamepadButtonState(button, true);
		debug(LOG_INPUT, "Gamepad axis button %d pressed", (int)button);
	}
	else if (axisButtonHeld[button] && deflection < pressThreshold - GAMEPAD_AXIS_BUTTON_HYSTERESIS)
	{
		axisButtonHeld[button] = false;
		setGamepadButtonState(button, false);
		debug(LOG_INPUT, "Gamepad axis button %d released", (int)button);
	}
}

// Returns banked flick travel to the cursor over roughly a tenth of a second
static bool drainBankedTravel(float dt)
{
	if (bankedTravelX == 0.f && bankedTravelY == 0.f)
	{
		return false;
	}
	const float drain = std::min(1.f, dt * 10.f);
	virtualCursorX += bankedTravelX * drain;
	virtualCursorY += bankedTravelY * drain;
	bankedTravelX *= (1.f - drain);
	bankedTravelY *= (1.f - drain);
	if (std::abs(bankedTravelX) < 0.5f && std::abs(bankedTravelY) < 0.5f)
	{
		bankedTravelX = 0.f;
		bankedTravelY = 0.f;
	}
	return true;
}

// Moves the cursor from left-stick deflection while the gamepad is the active input source
static void updateVirtualCursor(float dt)
{
	if (!gamepadIsActiveInput)
	{
		return;
	}

	const float deflectionX = aGamepadAxisValues[GPAD_AXIS_LEFT_X];
	const float deflectionY = aGamepadAxisValues[GPAD_AXIS_LEFT_Y];
	const float deflection = std::sqrt(deflectionX * deflectionX + deflectionY * deflectionY);
	static uint32_t lastMovementTick = 0;

	if (hopActive)
	{
		// new stick input or a button press cancels the hop and returns
		// control to the stick
		if (deflection > 0.f || mouseDown(MOUSE_LMB) || mouseDown(MOUSE_RMB) || gamepadSyntheticClickHeld())
		{
			hopActive = false;
		}
		else
		{
			const float hopDX = hopTargetX - virtualCursorX;
			const float hopDY = hopTargetY - virtualCursorY;
			const float hopDistance = std::sqrt(hopDX * hopDX + hopDY * hopDY);
			const float step = std::min(hopDistance, GAMEPAD_HOP_SPEED * dt);
			if (hopDistance > 0.f)
			{
				virtualCursorX += (hopDX / hopDistance) * step;
				virtualCursorY += (hopDY / hopDistance) * step;
			}
			if (step >= hopDistance)
			{
				hopActive = false;
			}
			inputSetMousePos((int)std::lround(virtualCursorX), (int)std::lround(virtualCursorY));
			return;
		}
	}

	const float magnetStrength = (float)war_GetGamepadCursorMagnetism() / 100.f;
	const float targetDX = magnetTargetX - virtualCursorX;
	const float targetDY = magnetTargetY - virtualCursorY;
	const float targetDistance = std::sqrt(targetDX * targetDX + targetDY * targetDY);
	const bool magnetUsable = magnetTargetFresh && magnetStrength > 0.f && targetDistance > 0.5f;

	if (deflection == 0.f)
	{
		// a short sharp press-and-release is a flick, requesting a hop to the
		// next widget in the flicked direction
		if (flickStartTick != 0)
		{
			if (flickPeakDeflection >= GAMEPAD_FLICK_MIN_DEFLECTION && SDL_GetTicks() - flickStartTick <= GAMEPAD_FLICK_MAX_MS
				&& !mouseDown(MOUSE_LMB) && !mouseDown(MOUSE_RMB) && !gamepadSyntheticClickHeld())
			{
				flickPending = true;
				flickDirX = flickPeakDirX;
				flickDirY = flickPeakDirY;
				// hops search from where the gesture began, so the manual
				// travel during the flick does not compound with the hop
				flickStartPosX = (int)std::lround(flickStartCursorX);
				flickStartPosY = (int)std::lround(flickStartCursorY);
				// hold the banked release tail until a hop can claim it
				bankAwaitingHop = true;
			}
			flickStartTick = 0;
			flickPeakDeflection = 0.f;
		}
		else if (bankAwaitingHop)
		{
			// no hop claimed the flick, so the banked tail returns
			bankAwaitingHop = false;
		}
		bool cursorMoved = false;
		if (!bankAwaitingHop)
		{
			cursorMoved = drainBankedTravel(dt);
		}
		// glide onto a near target right after the stick releases, so slow
		// acquisitions finish centered without the cursor ever being held
		if (magnetUsable && targetDistance <= GAMEPAD_MAGNET_SETTLE_RADIUS
			&& lastMovementTick != 0 && SDL_GetTicks() - lastMovementTick <= GAMEPAD_MAGNET_SETTLE_WINDOW_MS)
		{
			const float step = std::min(targetDistance, GAMEPAD_MAGNET_SETTLE_SPEED * dt);
			virtualCursorX += (targetDX / targetDistance) * step;
			virtualCursorY += (targetDY / targetDistance) * step;
			cursorMoved = true;
		}
		if (cursorMoved)
		{
			inputSetMousePos((int)std::lround(virtualCursorX), (int)std::lround(virtualCursorY));
		}
		return;
	}
	lastMovementTick = (uint32_t)SDL_GetTicks();
	if (flickStartTick == 0)
	{
		flickStartTick = lastMovementTick;
		flickPeakDeflection = 0.f;
		flickStartCursorX = virtualCursorX;
		flickStartCursorY = virtualCursorY;
	}
	if (deflection > flickPeakDeflection)
	{
		flickPeakDeflection = deflection;
		flickPeakDirX = deflectionX / deflection;
		flickPeakDirY = deflectionY / deflection;
	}

	float speed = (float)war_GetGamepadCursorSpeed();
	if (actualButtonDown(GPAD_BTN_LEFT_STICK))
	{
		speed *= GAMEPAD_CURSOR_PRECISION_FACTOR;
	}

	// quadratic response gives finer control near the center
	float velX = deflectionX * std::abs(deflectionX) * speed;
	float velY = deflectionY * std::abs(deflectionY) * speed;

	if (magnetUsable)
	{
		const float assistWeight = magnetStrength * std::clamp((GAMEPAD_MAGNET_NO_ASSIST_DEFLECTION - deflection) / (GAMEPAD_MAGNET_NO_ASSIST_DEFLECTION - GAMEPAD_MAGNET_FULL_ASSIST_DEFLECTION), 0.f, 1.f);
		const float captureRange = magnetTargetRadius + GAMEPAD_MAGNET_CAPTURE_SLACK;
		// motion away from the target releases the assist entirely, so small
		// nudges escape without fighting the friction
		const bool movingToward = (velX * targetDX + velY * targetDY) > 0.f;
		if (assistWeight > 0.f && movingToward && targetDistance < captureRange)
		{
			// slow over the target and bend the path toward it - the pull is
			// capped well below the commanded speed, so it can never stop or
			// reverse the stick's motion
			const float proximity = 1.f - (targetDistance / captureRange);
			const float frictionScale = 1.f - GAMEPAD_MAGNET_MAX_FRICTION * assistWeight * proximity;
			velX *= frictionScale;
			velY *= frictionScale;
			const float commandedSpeed = std::sqrt(velX * velX + velY * velY);
			const float pullSpeed = commandedSpeed * GAMEPAD_MAGNET_MAX_PULL_FRACTION * assistWeight * proximity;
			velX += (targetDX / targetDistance) * pullSpeed;
			velY += (targetDY / targetDistance) * pullSpeed;
		}
	}

	bankAwaitingHop = false;
	// a held button means a drag is in progress, so no flick handling may
	// park or bank the cursor
	const bool inFlickWindow = lastMovementTick - flickStartTick <= GAMEPAD_FLICK_MAX_MS;
	const bool flickQualifies = inFlickWindow && flickPeakDeflection >= GAMEPAD_FLICK_MIN_DEFLECTION
		&& !mouseDown(MOUSE_LMB) && !mouseDown(MOUSE_RMB) && !gamepadSyntheticClickHeld();
	if (flickQualifies && flickAnchorFresh)
	{
		// ride the movement onto the prospective hop target and park there
		// until the flick window passes - an actual flick then releases with
		// the cursor already in place
		const float oldDX = flickAnchorX - virtualCursorX;
		const float oldDY = flickAnchorY - virtualCursorY;
		float newX = virtualCursorX + velX * dt;
		float newY = virtualCursorY + velY * dt;
		const float newDX = flickAnchorX - newX;
		const float newDY = flickAnchorY - newY;
		const bool crossedAnchor = (oldDX * newDX + oldDY * newDY) <= 0.f;
		const bool nearAnchor = (newDX * newDX + newDY * newDY) <= 16.f;
		if (crossedAnchor || nearAnchor)
		{
			newX = flickAnchorX;
			newY = flickAnchorY;
		}
		virtualCursorX = newX;
		virtualCursorY = newY;
		virtualCursorX = std::clamp(virtualCursorX, 0.f, (float)(screenWidth > 0 ? screenWidth - 1 : 0));
		virtualCursorY = std::clamp(virtualCursorY, 0.f, (float)(screenHeight > 0 ? screenHeight - 1 : 0));
		inputSetMousePos((int)std::lround(virtualCursorX), (int)std::lround(virtualCursorY));
		return;
	}
	// with no target to park on, once the stick starts snapping back bank the
	// tail of the travel - a resulting hop drops it, trimming the visible
	// overshoot, and it drains back if the gesture is not a flick after all
	if (flickQualifies && deflection < flickPeakDeflection * GAMEPAD_FLICK_RELEASE_FRACTION)
	{
		bankedTravelX += velX * dt;
		bankedTravelY += velY * dt;
		return;
	}
	drainBankedTravel(dt);

	virtualCursorX += velX * dt;
	virtualCursorY += velY * dt;
	virtualCursorX = std::clamp(virtualCursorX, 0.f, (float)(screenWidth > 0 ? screenWidth - 1 : 0));
	virtualCursorY = std::clamp(virtualCursorY, 0.f, (float)(screenHeight > 0 ? screenHeight - 1 : 0));

	inputSetMousePos((int)std::lround(virtualCursorX), (int)std::lround(virtualCursorY));
}

// Scrolls the widget under the cursor with the right stick. The camera skips
// consuming the stick while this captures it
static void updateRightStickScroll(float dt)
{
	rightStickScrollingWidget = false;

	if (!gamepadIsActiveInput)
	{
		return;
	}
	// holding the right shoulder routes the stick to camera rotation regardless of hover
	if (actualButtonDown(GPAD_BTN_RIGHT_SHOULDER))
	{
		return;
	}
	const float deflectionX = aGamepadAxisValues[GPAD_AXIS_RIGHT_X];
	const float deflectionY = aGamepadAxisValues[GPAD_AXIS_RIGHT_Y];
	static float wheelAccumulatorX = 0.f;
	static float wheelAccumulatorY = 0.f;
	if ((deflectionX == 0.f && deflectionY == 0.f) || !isMouseOverWheelScrollConsumingWidget())
	{
		wheelAccumulatorX = 0.f;
		wheelAccumulatorY = 0.f;
		return;
	}
	rightStickScrollingWidget = true;

	// stick up scrolls up and stick right scrolls right - accumulate
	// fractional wheel units across frames
	wheelAccumulatorX += deflectionX * GAMEPAD_SCROLL_SPEED * dt;
	wheelAccumulatorY += -deflectionY * GAMEPAD_SCROLL_SPEED * dt;
	const int wholeUnitsX = (int)wheelAccumulatorX;
	const int wholeUnitsY = (int)wheelAccumulatorY;
	if (wholeUnitsX != 0 || wholeUnitsY != 0)
	{
		wheelAccumulatorX -= (float)wholeUnitsX;
		wheelAccumulatorY -= (float)wholeUnitsY;
		inputAddMouseWheelScroll(Vector2i(wholeUnitsX, wholeUnitsY));
	}
}

// Synthetic mouse click driven by a gamepad face button
struct SyntheticClickState
{
	bool held = false;       // the synthetic mouse button is currently pressed
	bool ownsShift = false;  // this interaction asserted the shift key
};
static SyntheticClickState syntheticLeftClick;
static SyntheticClickState syntheticRightClick;

bool gamepadSyntheticClickHeld()
{
	return syntheticLeftClick.held || syntheticRightClick.held;
}
// Count of interactions holding a synthetic shift, and releases deferred by a
// frame so the release-frame click processing still sees the modifier
static int shiftAssertCount = 0;
static int shiftReleaseQueue = 0;

static void handleClickButton(GAMEPAD_INPUT gamepadButton, MOUSE_KEY_CODE mouseButton, SyntheticClickState& state)
{
	const Vector2i pos((int)mouseX(), (int)mouseY());

	// holding the right shoulder reserves the face buttons for chorded actions
	if (actualButtonPressed(gamepadButton) && !state.held && !actualButtonDown(GPAD_BTN_RIGHT_SHOULDER) && !captureModeActive)
	{
		// holding the left shoulder makes this an additive/queueing click - assert
		// shift so the existing modifier checks in the click handling apply, unless
		// the physical shift key is already doing the job
		if (actualButtonDown(GPAD_BTN_LEFT_SHOULDER))
		{
			if (shiftAssertCount > 0 || !keyDown(KEY_LSHIFT))
			{
				if (shiftAssertCount == 0)
				{
					inputSetKey(KEY_LSHIFT, true);
				}
				++shiftAssertCount;
				state.ownsShift = true;
			}
		}
		state.held = true;
		inputSetMouseButton(mouseButton, true, pos);
	}
	if (actualButtonReleased(gamepadButton) && state.held)
	{
		state.held = false;
		inputSetMouseButton(mouseButton, false, pos);
		if (state.ownsShift)
		{
			state.ownsShift = false;
			++shiftReleaseQueue;
		}
	}
}

static void handleKeyButton(GAMEPAD_INPUT gamepadButton, KEY_CODE key, bool suppressPress = false)
{
	if (actualButtonPressed(gamepadButton) && !suppressPress)
	{
		inputSetKey(key, true);
		inputAddEditingKey(key);
	}
	if (actualButtonReleased(gamepadButton))
	{
		inputSetKey(key, false);
	}
}

static void updateSyntheticInput()
{
	// apply shift releases deferred from the previous frame
	while (shiftReleaseQueue > 0)
	{
		--shiftReleaseQueue;
		--shiftAssertCount;
		if (shiftAssertCount == 0)
		{
			inputSetKey(KEY_LSHIFT, false);
		}
	}

	handleClickButton(GPAD_BTN_SOUTH, MOUSE_LMB, syntheticLeftClick);
	handleClickButton(GPAD_BTN_EAST, MOUSE_RMB, syntheticRightClick);

	handleKeyButton(GPAD_BTN_START, KEY_ESC, captureModeActive);
	// the right shoulder reserves west for its chorded action
	handleKeyButton(GPAD_BTN_WEST, KEY_RETURN, actualButtonDown(GPAD_BTN_RIGHT_SHOULDER) || captureModeActive);
}

void wzGamepadUpdate()
{
	const OpenGamepad* activePad = activeGamepadEntry();
	if (!activePad)
	{
		return;
	}
	SDL_Gamepad* pad = activePad->handle;

	const auto rawAxis = [pad](SDL_GamepadAxis axis) -> float {
		return std::clamp((float)SDL_GetGamepadAxis(pad, axis) / 32767.f, -1.f, 1.f);
	};

	// swapping exchanges the sticks at the device level, so the logical left
	// stick is always the one that moves the cursor
	const bool swapSticks = war_GetGamepadSwapSticks();
	applyStickDeadzone(rawAxis(swapSticks ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_LEFTX),
	                   rawAxis(swapSticks ? SDL_GAMEPAD_AXIS_RIGHTY : SDL_GAMEPAD_AXIS_LEFTY),
	                   GPAD_AXIS_LEFT_X, GPAD_AXIS_LEFT_Y);
	applyStickDeadzone(rawAxis(swapSticks ? SDL_GAMEPAD_AXIS_LEFTX : SDL_GAMEPAD_AXIS_RIGHTX),
	                   rawAxis(swapSticks ? SDL_GAMEPAD_AXIS_LEFTY : SDL_GAMEPAD_AXIS_RIGHTY),
	                   GPAD_AXIS_RIGHT_X, GPAD_AXIS_RIGHT_Y);
	if (war_GetGamepadInvertRightStick())
	{
		aGamepadAxisValues[GPAD_AXIS_RIGHT_Y] = -aGamepadAxisValues[GPAD_AXIS_RIGHT_Y];
	}
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

	static Uint64 lastTick = 0;
	const Uint64 now = SDL_GetTicks();
	// clamp dt so a long stall cannot fling the cursor or scroll
	const float dt = (lastTick != 0) ? std::min((float)(now - lastTick) / 1000.f, 0.1f) : 0.f;
	lastTick = now;

	// a flick not consumed in the frame it was detected in expires
	flickPending = false;

	updateVirtualCursor(dt);
	updateRightStickScroll(dt);
	updateSyntheticInput();

	// the magnet target and flick anchor are only valid for the frame they
	// were set in
	magnetTargetFresh = false;
	flickAnchorFresh = false;
}

bool gamepadConsumePendingCursorFlick(float& outDirX, float& outDirY, int& outStartX, int& outStartY)
{
	if (!flickPending)
	{
		return false;
	}
	flickPending = false;
	outDirX = flickDirX;
	outDirY = flickDirY;
	outStartX = flickStartPosX;
	outStartY = flickStartPosY;
	return true;
}

bool gamepadGetActiveCursorFlick(float& outDirX, float& outDirY, int& outStartX, int& outStartY)
{
	if (flickStartTick == 0 || flickPeakDeflection < GAMEPAD_FLICK_MIN_DEFLECTION)
	{
		return false;
	}
	if (SDL_GetTicks() - flickStartTick > GAMEPAD_FLICK_MAX_MS)
	{
		return false;
	}
	outDirX = flickPeakDirX;
	outDirY = flickPeakDirY;
	outStartX = (int)std::lround(flickStartCursorX);
	outStartY = (int)std::lround(flickStartCursorY);
	return true;
}

void gamepadSetCursorFlickAnchor(int screenX, int screenY)
{
	flickAnchorFresh = true;
	flickAnchorX = (float)screenX;
	flickAnchorY = (float)screenY;
}

void gamepadCursorHopTo(int screenX, int screenY)
{
	hopActive = true;
	hopTargetX = (float)screenX;
	hopTargetY = (float)screenY;
	// the hop claims any release travel banked during the flick
	bankedTravelX = 0.f;
	bankedTravelY = 0.f;
	bankAwaitingHop = false;
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

void wzGamepadRestoreMetaButtonState()
{
	for (GAMEPAD_INPUT button : { GPAD_BTN_LEFT_SHOULDER, GPAD_BTN_RIGHT_SHOULDER })
	{
		if (actualGamepadState[button].state != KEY_UP)
		{
			aGamepadState[button] = actualGamepadState[button];
		}
	}
}

void wzGamepadResetInputState()
{
	// release any synthetic input still held
	const Vector2i pos((int)mouseX(), (int)mouseY());
	if (syntheticLeftClick.held)
	{
		inputSetMouseButton(MOUSE_LMB, false, pos);
	}
	if (syntheticRightClick.held)
	{
		inputSetMouseButton(MOUSE_RMB, false, pos);
	}
	syntheticLeftClick = SyntheticClickState();
	syntheticRightClick = SyntheticClickState();
	if (shiftAssertCount > 0)
	{
		inputSetKey(KEY_LSHIFT, false);
	}
	shiftAssertCount = 0;
	shiftReleaseQueue = 0;

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
	magnetTargetFresh = false;
	flickPending = false;
	flickStartTick = 0;
	flickPeakDeflection = 0.f;
	flickAnchorFresh = false;
	hopActive = false;
	bankedTravelX = 0.f;
	bankedTravelY = 0.f;
	bankAwaitingHop = false;
}

