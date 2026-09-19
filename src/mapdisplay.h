/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_MAPDISPLAY_H__
#define __INCLUDED_SRC_MAPDISPLAY_H__

// Draw a research topic's model centered on the given point, at the fixed scales
// the intelligence screen's 238x168 view is built around.
void renderResearchToBuffer(RESEARCH *psResearch, UDWORD OriginX, UDWORD OriginY, bool rotate = true);

// The same, fitted to a square of the given size by normalizing on the model's
// own radius. Those fixed scales do not survive being shrunk uniformly, since
// they are per type rather than per model: a propulsion model overflows a small
// box while an engine upgrade nearly disappears in it. A model is drawn in its
// own projection and cannot be clipped, so overflow spills out of the caller.
void renderResearchFittedToBuffer(RESEARCH *psResearch, UDWORD OriginX, UDWORD OriginY, UDWORD boxSize, bool rotate = true);

#endif // __INCLUDED_SRC_MAPDISPLAY_H__
