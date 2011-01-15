/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#include "pieclip.h"
#include "ivi.h"

static UDWORD videoBufferDepth = 32, videoBufferWidth = 0, videoBufferHeight = 0;

BOOL pie_SetVideoBufferDepth(UDWORD depth)
{
	videoBufferDepth = depth;
	return(true);
}

BOOL pie_SetVideoBufferWidth(UDWORD width)
{
	videoBufferWidth = width;
	return(true);
}

BOOL pie_SetVideoBufferHeight(UDWORD height)
{
	videoBufferHeight = height;
	return(true);
}

UDWORD pie_GetVideoBufferDepth(void)
{
	return(videoBufferDepth);
}

UDWORD pie_GetVideoBufferWidth(void)
{
	return(videoBufferWidth);
}

UDWORD pie_GetVideoBufferHeight(void)
{
	return(videoBufferHeight);
}
