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
#ifndef __game_io_h
#define __game_io_h
#include <SDL.h>

#include "wzglobal.h"

/* Simple implementation of singleclick doubleclick holdbutton via SDL */

///use max possible sdl keys
const Uint32 MAX_KEYS = SDLK_LAST;
///use max possible sdl mouse buttons(UNUSED?,LEFT,RIGHT,MIDDLE)
const Uint8 MAX_MOUSE_BUTTONS = 4;


///timer for hold,the interval between 2 key events < 100ms triggers OH_KEY_HOLD
const Uint32 KEY_HOLD_TIMER = 100;
///timer for double down(double click for mouse),the interval between 2 key events < 500ms triggers OH_KEY_DDOWN
const Uint32 KEY_DDOWN_TIMER = 500;

///state for both mouse and keyboard
enum _key_state {
	OH_KEY_UP = 0x0001,		///< Key/Button is up
	OH_KEY_DOWN = 0x0002,	///< Key/Button is down
	OH_KEY_HOLD = 0x0004,	///< Key/Button is hold for some time
	OH_KEY_DDOWN = 0x0008,	///< Key/Button is double-down/double-clickd
	OH_KEY_RELEASE = 0x0010,	///< Key/Button is released(note:this one is the proper clicked event)
};

///input element with hold event duration
typedef struct _input_elem {
	///key state
	Uint32	data;
	///last down timestamp
	Uint32	lastDown;
	///duration of hold
	Uint32	duration;
} INPUT_ELEM;

extern Uint16 MouseX;
extern Uint16 MouseY;
extern Sint16 MouseMoveX;
extern Sint16 MouseMoveY;
extern INPUT_ELEM KeyStates[MAX_KEYS];
extern INPUT_ELEM MouseStates[MAX_MOUSE_BUTTONS];

extern void inputInitialize(void);

extern void inputButtonMouseEvent(SDL_MouseButtonEvent button, Uint8 newState);
extern void inputMotionMouseEvent(SDL_MouseMotionEvent motion);

extern void inputKeyEvent(SDL_KeyboardEvent key, Uint8 newState);

bool isKeyDown(SDLKey key);
bool isKeyDoubleDown(SDLKey key);
bool isKeyHold(SDLKey key);
bool isMouseButtonDown(Uint8 button);
bool isMouseButtonDoubleDown(Uint8 button);
bool isMouseButtonHold(Uint8 button);

extern void inputUpdate(void);


#endif
