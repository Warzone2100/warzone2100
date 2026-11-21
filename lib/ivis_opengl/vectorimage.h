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

/** @file
 *  Vector images: one-color icons from a small subset of SVG, rasterized (with ThorVG) at exactly the size they
 *  cover on screen, and again when the display scale changes.
 *
 *  The data is checked before ThorVG parses it (iV_CheckVectorImage): a size cap, an element cap, and allowlists of
 *  elements and attributes that leave out everything that refers to other data (images, text, styles, use, links,
 *  url() references, entities).
 */

#ifndef __INCLUDED_LIB_IVIS_OPENGL_VECTORIMAGE_H__
#define __INCLUDED_LIB_IVIS_OPENGL_VECTORIMAGE_H__

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/piepalette.h"

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

#include <nonstd/optional.hpp>

constexpr size_t VECTOR_IMAGE_MAX_BYTES = 16 * 1024;
constexpr size_t VECTOR_IMAGE_MAX_ELEMENTS = 256;
/// The largest rasterization, in pixels on either side (larger draws are scaled up from it)
constexpr int VECTOR_IMAGE_MAX_PIXELS = 512;

/// Why the data can't be used as a vector image, or nothing if it can
nonstd::optional<std::string> iV_CheckVectorImage(const char *data, size_t size);

/// Keeps a copy of the data as the image `name` if it passes iV_CheckVectorImage. Otherwise logs why and returns false
bool iV_AddVectorImage(const std::string &name, const char *data, size_t size);
bool iV_HasVectorImage(const std::string &name);

/// An image's coverage (0-255, row by row), for callers that build their own textures (ex. an atlas)
struct VectorImageCoverage
{
	int width = 0;   ///< (padding included)
	int height = 0;
	int padding = 0; ///< (around the shape, for its halo)
	std::vector<uint8_t> coverage;
};

/// The image rasterized at width × height physical pixels, grown by haloPixels if > 0 (as iV_DrawVectorImage's halo)
nonstd::optional<VectorImageCoverage> iV_VectorImageCoverage(const std::string &name, int width, int height, float haloPixels = 0.f);
/// The game-to-renderer scale the images are rasterized for (iV_VectorImagesUpdateScaleFactor)
float iV_VectorImagesScale();

/// Its own width over its height (from its viewBox), or nothing if there is no such image or ThorVG can't load it
nonstd::optional<float> iV_VectorImageAspect(const std::string &name);

/// Draws the image into the logical rectangle in one color (only its shape counts, not its own colors).
/// With haloWidth > 0, draws the shape grown by that many logical pixels instead: a halo to draw under it.
/// False if there is no such image or ThorVG can't draw it.
bool iV_DrawVectorImage(const std::string &name, float x, float y, float width, float height, PIELIGHT colour, float haloWidth = 0.f);
/// The same, with the rectangle in its own space, drawn through modelViewProjection (ex. rotated with the radar). That
/// space should keep the screen's scale
bool iV_DrawVectorImageTransformed(const std::string &name, const glm::mat4 &modelViewProjection, float x, float y, float width, float height, PIELIGHT colour, float haloWidth = 0.f);

/// The game-to-renderer scale (as for iV_TextUpdateScaleFactor): images are rasterized again when next drawn
void iV_VectorImagesUpdateScaleFactor(unsigned int horizScalePercentage, unsigned int vertScalePercentage);
/// Frees every rasterization and shuts ThorVG down (the renderer is shutting down). The images stay
void iV_VectorImagesShutdown();

#endif // __INCLUDED_LIB_IVIS_OPENGL_VECTORIMAGE_H__
