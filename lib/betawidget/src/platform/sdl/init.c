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

#include "init.h"

#include "../../widget.h"
#include "../../svgManager.h"
#include "../../patternManager.h"
#include "../../window.h"

#include <SDL.h>

void widgetSDLInit()
{
	// Get information about the current surface
	SDL_Surface *surface = SDL_GetVideoSurface();

	// Set the screen size
	windowSetScreenSize(surface->w, surface->h);
	
	// Initialise the SVG manager
	svgManagerInit();
	
	// Initialise the pattern manager
	patternManagerInit();

	// Create an initial window vector to store windows
	windowSetWindowVector(vectorCreate());
}

void widgetSDLQuit()
{
	// Get the window vector
	vector *windowVector = windowGetWindowVector();

	// Release all active windows
	vectorMapAndDestroy(windowVector, (mapCallback) widgetDestroy);
	
	// Release any cached SVG images
	svgManagerQuit();
	
	// Release all patterns
	patternManagerQuit();
}

