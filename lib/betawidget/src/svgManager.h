/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Elio Gubser
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#ifndef SVG_MANAGER_H
#define SVG_MANAGER_H

#include "internal-cairo.h"
#include "vector.h"
#include "geom.h"

/*
 * Forward declarations
 */
typedef struct _svgRenderedImage svgRenderedImage;

/**
 * Ways in which the size (dimensions) of an SVG can be specified.
 */
typedef enum
{
	/// Renders the image at its native size
	SVG_SIZE_NATIVE,

	/// Renders the image with a specified width and auto-computed height
	SVG_SIZE_WIDTH,

	/// Renders the image with a specified height and auto-computed width
	SVG_SIZE_HEIGHT,

	/// Renders the image with a specified width and height, may be distorted
	SVG_SIZE_WIDTH_AND_HEIGHT,

	/**
	 * Renders the image with a width and a height as close as possible to that
	 * specified while keeping the image in proportion
	 */
	SVG_SIZE_WIDTH_AND_HEIGHT_FIT
} svgSizingPolicy;

/**
 * Represents a rendered SVG image which is ready to be blitted/composited onto
 * a widget.
 */
struct _svgRenderedImage
{
	/// Bitmap cairo_pattern_t containing the rendered image
	cairo_pattern_t *pattern;

	/// Size of the rendered image
	size patternSize;
};

/**
 * Initialises the SVG manager. This must be called before any SVG images are
 * loaded/used.
 */
void svgManagerInit(void);

/**
 * Uninitialises the SVG manager. Once this method has been called the result of
 * attempting to call any svgManager* methods (without a call to svgManagerInit
 * first) is undefined. Undefined behaviour is also encountered if one attempts
 * to call this method without having first called svgManagerInit.
 */
void svgManagerQuit(void);

/**
 * Composites the rendered SVG image, svg, onto the cairo context, cr, at the
 * current translation position. The state of the cairo context is unchanged by
 * this method.
 *
 * @param cr    The cairo context to composite the image onto.
 * @param svg   The rendered SVG image to composite.
 */
void svgManagerBlit(cairo_t *cr, const svgRenderedImage *svg);

/**
 * Returns the width and height that the SVG image, filename, will be rendered
 * at when using the sizing policy, policy.
 *
 * @param filename  The path to the SVG image to get the size of.
 * @param policy    The sizing policy to use, extra parameters may be required.
 * @return The size that the image will be rendered at.
 */
size svgManagerGetSize(const char *filename, svgSizingPolicy policy, ...);

/**
 * Loads and renders the SVG image, filename, at a size of (width,height). In
 * order to improve performance the SVG manager makes use of an image cache.
 * Hence, repeated calls to this method with the same parameters will not
 * result in any kind of performance penalty.
 *
 * The at which the image is rendered is determined by the sizing policy
 * argument. Many policies require extra arguments. The various available
 * policies are documented in the svgSizingPolicy enum.
 *
 * @param filename  The path to the SVG image to render.
 * @param policy    The sizing policy to use, extra parameters may be required.
 * @return A pointer to the rendered SVG image on success or NULL on failure.
 */
svgRenderedImage *svgManagerGet(const char *filename, svgSizingPolicy policy, ...);

#endif /*SVG_MANAGER_H*/
