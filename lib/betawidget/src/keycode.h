/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#ifndef KEYCODE_H_
#define KEYCODE_H_

/**
 * Contains the list of known keycodes for keyboard input.
 */
typedef enum
{
	// `Control' keys
	EVT_KEYCODE_BACKSPACE,
	EVT_KEYCODE_TAB,
	EVT_KEYCODE_RETURN,
	EVT_KEYCODE_ESCAPE,
	EVT_KEYCODE_ENTER,
	
	// Arrow keys
	EVT_KEYCODE_LEFT,
	EVT_KEYCODE_RIGHT,
	EVT_KEYCODE_UP,
	EVT_KEYCODE_DOWN,
	
	// Home/end pad
	EVT_KEYCODE_INSERT,
	EVT_KEYCODE_DELETE,
	EVT_KEYCODE_HOME,
	EVT_KEYCODE_END,
	EVT_KEYCODE_PAGEUP,
	EVT_KEYCODE_PAGEDOWN,
	
	// Unknown
	EVT_KEYCODE_UNKNOWN
} eventKeycode;

#endif /*KEYCODE_H_*/
