/*
 *  PieToaster is an OpenGL application to edit 3D models in
 *  Warzone 2100's (an RTS game) PIE 3D model format, which is heavily
 *  inspired by PieSlicer created by stratadrake.
 *  Copyright (C) 2007  Carl Hee
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "game_io.h"

//sets the MouseX,MouseY to invalid 0xFFFF
///mouse x coord on screen
Uint16 MouseX = 0xFFFF;
///mouse y coord on screen
Uint16 MouseY = 0xFFFF;
///mouse x move last frame
Sint16 MouseMoveX = 0;
///mouse y move last frame
Sint16 MouseMoveY = 0;

INPUT_ELEM KeyStates[MAX_KEYS];
INPUT_ELEM MouseStates[MAX_MOUSE_BUTTONS];

///initialize and set Mouse/KeyStates to zero
void inputInitialize(void) {
	memset(&KeyStates[0], 0, sizeof(INPUT_ELEM) * MAX_KEYS);
	memset(&MouseStates[0], 0, sizeof(INPUT_ELEM) * MAX_MOUSE_BUTTONS);
}

///handles  mouse input
void inputButtonMouseEvent(SDL_MouseButtonEvent button, Uint8 newState) {
	const Uint16 buttonId = button.button;

	if (newState == SDL_MOUSEBUTTONUP) {
		MouseStates[buttonId].data |= OH_KEY_UP;
		MouseStates[buttonId].duration = 0;
	}
	else if (newState == SDL_MOUSEBUTTONDOWN) {
		MouseStates[buttonId].data |= OH_KEY_DOWN;
	}
}

//update the mouse screen x,y
void inputMotionMouseEvent(SDL_MouseMotionEvent motion) {
	MouseMoveX = (Sint16)MouseX - (Sint16)motion.x;
	MouseMoveY = (Sint16)MouseY - (Sint16)motion.y;
	MouseX = motion.x;
	MouseY = motion.y;
}

///handles key input
void inputKeyEvent(SDL_KeyboardEvent key, Uint8 newState) {
	const Uint16 eventKey = key.keysym.sym;

	if (newState == SDL_KEYUP) {
		KeyStates[eventKey].data |= OH_KEY_UP;
		KeyStates[eventKey].duration = 0;
	}
	else if (newState == SDL_KEYDOWN) {
		KeyStates[eventKey].data |= OH_KEY_DOWN;
	}
}

bool isKeyDown(SDLKey key) {
	return (KeyStates[key].data & OH_KEY_RELEASE);
}

bool isKeyDoubleDown(SDLKey key) {
	return (KeyStates[key].data & OH_KEY_DDOWN && KeyStates[key].data & OH_KEY_RELEASE );
}

bool isKeyHold(SDLKey key) {
	return (KeyStates[key].duration);
}

bool isMouseButtonDown(Uint8 button) {
	return (MouseStates[button].data & OH_KEY_RELEASE);
}

bool isMouseButtonDoubleDown(Uint8 button) {
	return (MouseStates[button].data & OH_KEY_DDOWN && MouseStates[button].data & OH_KEY_RELEASE);
}

bool isMouseButtonHold(Uint8 button) {
	return (MouseStates[button].duration);
}

///per-frame input update
void inputUpdate(void) {
	Uint32	timeDiff, i;
	const Uint32 currTicks = SDL_GetTicks();

	for (i = 0;i < MAX_KEYS;i++) {
		if (KeyStates[i].data) {
			timeDiff = currTicks - KeyStates[i].lastDown;
			if (KeyStates[i].data & OH_KEY_RELEASE)
			{
				KeyStates[i].data = 0;
			}

			if (KeyStates[i].data & OH_KEY_UP)
			{
				KeyStates[i].data |= OH_KEY_RELEASE;
				KeyStates[i].lastDown = 0;
			}

			if (KeyStates[i].data & OH_KEY_DOWN) {
				if (timeDiff)
				{
					if (!(KeyStates[i].data & OH_KEY_RELEASE)) {
						KeyStates[i].data |= OH_KEY_HOLD;
						KeyStates[i].duration += timeDiff;
					}
					else if (timeDiff <= KEY_DDOWN_TIMER) {
						KeyStates[i].data |= OH_KEY_DDOWN;
						KeyStates[i].lastDown = currTicks;
					}
					else {
						KeyStates[i].duration = 0;
						KeyStates[i].lastDown = currTicks;
					}
				}
			}
		}
	}
	for (i = 0;i < MAX_MOUSE_BUTTONS;i++) {
		if (MouseStates[i].data) {
			timeDiff = currTicks - MouseStates[i].lastDown;
			if (MouseStates[i].data & OH_KEY_RELEASE)
			{
				MouseStates[i].data = 0;
			}

			if (MouseStates[i].data & OH_KEY_UP)
			{
				MouseStates[i].data |= OH_KEY_RELEASE;
				MouseStates[i].lastDown = 0;
			}

			if (MouseStates[i].data & OH_KEY_DOWN) {
				if (timeDiff)
				{
					if (!(MouseStates[i].data & OH_KEY_RELEASE)) {
						MouseStates[i].data |= OH_KEY_HOLD;
						MouseStates[i].duration += timeDiff;
					}
					else if (timeDiff <= KEY_DDOWN_TIMER) {
						MouseStates[i].data |= OH_KEY_DDOWN;
						MouseStates[i].data |= OH_KEY_RELEASE;
						MouseStates[i].lastDown = currTicks;
					}
					else {
						MouseStates[i].duration = 0;
						MouseStates[i].data |= OH_KEY_RELEASE;
						MouseStates[i].lastDown = currTicks;
					}
				}
			}
		}
	}
}
