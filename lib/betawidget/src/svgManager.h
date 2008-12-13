/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008  Elio Gubser
	Copyright (C) 2008  Warzone Resurrection Project

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
 * Loads and renders the SVG image, filename, at a size of (width,height). In
 * order to improve performance the SVG manager makes use of an image cache.
 * Hence, repeated calls to this method with the same parameters will not
 * result in any kind of performance penalty.
 *
 * If both width and height are 0 then the image will be rendered at its native
 * size. If just width is 0 then the rendered width is the native width
 * multiplied by the height scale factor. The same applies when the height is 0
 * except that the width scale factor is used instead.
 * If fit is true, then both width and height mustn't be 0. Then it tries to fit
 * the image in the desired height and width without loosing proportion.
 *
 * @param filename  The path to the SVG image to render.
 * @param width The width to render the image at, 0 for auto-select.
 * @param height    The height to render the image at, 0 for auto-select.
 * @param fit	Wetheter to fit the image, width and height mustn't be 0
 * @return A pointer to the rendered SVG image on success or NULL on failure.
 */
svgRenderedImage *svgManagerGet(const char *filename, int width, int height, bool fit);

/**
 * A convenience wrapper around svgManagerGet which automatically computes the
 * desired image-height based off the provided width.
 *
 * @param filename  The path to the SVG image to render.
 * @param width The width to render the image.
 * @param height    The resulting height of the image, may be NULL.
 * @return A pointer to the rendered SVG image on success or NULL on failure.
 * @see svgManagerGet
 */
svgRenderedImage *svgManagerGetWithWidth(const char *filename, int width,
                                         int *height);

/**
 * A convenience wrapper around svgManagerGet which automatically computes the
 * desired image-width based off the provided height.
 *
 * @param filename  The path to the SVG image to render.
 * @param height    The height to render the image.
 * @param width The resulting width of the image, may be NULL.
 * @return A pointer to the rendered SVG image on success or NULL on failure.
 * @see svgManagerGet
 */
svgRenderedImage *svgManagerGetWithHeight(const char *filename, int height,
                                          int *width);
/**
 * A convenience wrapper around svgManagerGet which computes the size of the image
 * so it fits in height and width without loosing proportions.
 *
 * @param filename  The path to the SVG image to render.
 * @param height    The maximum height to render the image.
 * @param width     The maximum width to render the image
 * @param ptrWidth  The resulting width of the image, may be NULL
 * @param ptrHeight The resulting height of the image, may be NULL
 * @return A pointer to the rendered SVG image on success or NULL on failure.
 * @see svgManagerGet
 */
svgRenderedImage *svgManagerGetFit(const char *filename, int width, int height, int *ptrWidth, int *ptrHeight);

#endif /*SVG_MANAGER_H*/
