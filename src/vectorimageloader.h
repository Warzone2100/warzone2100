// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef __INCLUDED_SRC_VECTORIMAGELOADER_H__
#define __INCLUDED_SRC_VECTORIMAGELOADER_H__

#include "lib/ivis_opengl/piepalette.h"

#include <string>

/// Loads the vector image at `path` (lib/ivis_opengl/vectorimage.h) the first time, from the game's own data only: a
/// file that a mod or a map overrides is refused. Failures are remembered. A missing optional file isn't logged.
bool loadGameVectorImage(const std::string &path, bool optional = false);
/// Draws it (iV_DrawVectorImage), loading it the first time
bool drawGameVectorImage(const std::string &path, float x, float y, float width, float height, PIELIGHT colour, float haloWidth = 0.f, bool optional = false);

#endif // __INCLUDED_SRC_VECTORIMAGELOADER_H__
