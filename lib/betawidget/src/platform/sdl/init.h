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

#ifndef PLATFORM_SDL_INIT_H_
#define PLATFORM_SDL_INIT_H_

/**
 * Initialises the widget library for use in an SDL application. This must be
 * called before any widget methods are called but after SDL_SetVideoMode.
 */
void widgetSDLInit(void);

/**
 * Uninitialise the widget library. Once this method has been called the result
 * of calling any widget* methods (without a call to widgetSDLInit beforehand)
 * is undefined.
 *
 * It is also an error to call this method without a corresponding call to
 * widgetSDLInit.
 */
void widgetSDLQuit(void);

#endif /*PLATFORM_SDL_INIT_H_*/

