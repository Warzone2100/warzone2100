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

#include "svgManager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <svg-cairo.h>

static vector *svgImages = NULL;

/*
 * Internal structures
 */
typedef struct _svgImage svgImage;

struct _svgImage
{
	/// Path to the image
	const char *filename;

	/// libsvg-cairo internal parse tree
	svg_cairo_t *svg;

	/// Array of svgRenderedImages for the currently rendered sizes
	vector *renders;
};

static void svgManagerFreeRender(void *renderedImage)
{
	// Un-reference the pattern
	cairo_pattern_destroy(((svgRenderedImage *) renderedImage)->pattern);

	// Free the memory allocated for the structure
	free(renderedImage);
}

static void svgManagerFreeImage(void *image)
{
	// Free filename buffer
	free(((svgImage *)image)->filename);

	// Free the libcairo-svg parse tree
	svg_cairo_destroy(((svgImage *) image)->svg);

	// Release all renders of the image
	vectorMapAndDestroy(((svgImage *) image)->renders, svgManagerFreeRender);

	// Finally, free the memory allocated for our structure
	free(image);
}

/**
 * Loads the SVG image specified by filename and adds it to the global list
 * of SVG images. The newly loaded image is then returned for rendering.
 *
 * This function should NOT be called directly; instead use svgManagerLoad.
 *
 * @param filename  The path of the image to load.
 * @return A pointer to the newly loaded SVG image on success, NULL otherwise.
 */
static svgImage *svgManagerLoadInternal(const char *filename)
{
	svgImage *svg = malloc(sizeof(svgImage));
	svg_cairo_status_t status;

	// Set the filename of the newly loaded image
	svg->filename = strdup(filename);

	// Initialise the svg_cairo structure
	status = svg_cairo_create(&svg->svg);

	// Ensure that we were able to create the structure
	assert(status == SVG_CAIRO_STATUS_SUCCESS);

	// Parse the SVG
	status = svg_cairo_parse(svg->svg, filename);

	// Ensure that the SVG was successfully parsed
	assert(status == SVG_CAIRO_STATUS_SUCCESS);

	// Create a new vector to store size-specific renders of the image
	svg->renders = vectorCreate();

	// Add the newly loaded image to the images array and return it
	return vectorAdd(svgImages, svg);
}

/**
 * Loads and returns the SVG image specified by filename. This is a higher-level
 * version of svgManagerLoad which checks the image cache before loading an
 * image into memory.
 *
 * @param filename The path to the SVG image to load.
 * @return A pointer to the newly loaded SVG image.
 */
static svgImage *svgManagerLoad(const char *filename)
{
	svgImage *currSvg, *svg = NULL;

	// See if the image exists in the cache at *any* size
	vectorRewind(svgImages);
	while ((currSvg = vectorNext(svgImages)))
	{
		// If the filenames match then we have found the image
		if (strcmp(filename, currSvg->filename) == 0)
		{
			svg = currSvg;
			break;
		}
	}

	// If no image was found then go ahead and load/parse it
	if (svg == NULL)
	{
		svg = svgManagerLoadInternal(filename);
	}

	assert(svg != NULL);

	return svg;
}

/**
 * A low-level version of svgManagerGetSize.
 *
 * @param image The image to get the rendered size for.
 * @param policy    The policy to use for working out the size of the image.
 * @param ap    An additional arguments required by the sizing policy.
 * @return The rendered size of the image.
 * @see svgManagerGetSize
 */
static size svgManagerGetSizeInternal(svgImage *image,
                                      svgSizingPolicy policy,
                                      va_list ap)
{
	size size;
	unsigned int sourceWidth, sourceHeight;

	// Get the native size of the image
	svg_cairo_get_size(image->svg, &sourceWidth, &sourceHeight);

	switch (policy)
	{
		// Just use the source (native) size
		case SVG_SIZE_NATIVE:
		{
			size.x = sourceWidth;
			size.y = sourceHeight;
			break;
		}
		// Scale such that the width is that of the first argument
		case SVG_SIZE_WIDTH:
		{
			unsigned int width = va_arg(ap, unsigned int);

			size.x = width;
			size.y = ((float) width / (float) sourceWidth) * (float) sourceHeight;
			break;
		}
		// Scale such that the height is that of first argument
		case SVG_SIZE_HEIGHT:
		{
			unsigned int height = va_arg(ap, unsigned int);

			size.x = ((float) height / (float) sourceHeight) * (float) sourceWidth;
			size.y = height;
			break;
		}
		// Just use the provided width and height (first and second arguments)
		case SVG_SIZE_WIDTH_AND_HEIGHT:
		{
			unsigned int width = va_arg(ap, unsigned int);
			unsigned int height = va_arg(ap, unsigned int);

			size.x = width;
			size.y = height;
			break;
		}
		// Use the provided width and height as a guideline
		case SVG_SIZE_WIDTH_AND_HEIGHT_FIT:
		{
			unsigned int width = va_arg(ap, unsigned int);
			unsigned int height = va_arg(ap, unsigned int);

			float maxWidth = ((float) height / (float) sourceHeight) * (float) sourceWidth;
			float maxHeight = ((float) width / (float) sourceWidth) * (float) sourceHeight;

			if (maxWidth <= width)
			{
				width = maxWidth;
			}
			else if (maxHeight <= height)
			{
				height = maxHeight;
			}

			size.x = width;
			size.y = height;
			break;
		}
	}

	return size;
}

/**
 * Renders the SVG image, svg, at (width,height) and adds it to the cache. The
 * rendered image is then returned.
 *
 * @param svg   The SVG image to render.
 * @param width The width to render the image at.
 * @param height    The height to render the image at.
 * @return A pointer to the newly rendered image on success, false othwewise.
 */
static svgRenderedImage *svgManagerRender(svgImage *svg, int width, int height)
{
	svgRenderedImage *render = malloc(sizeof(svgRenderedImage));
	unsigned int sourceWidth, sourceHeight;

	cairo_t *cr;
	cairo_surface_t *surface;
	cairo_pattern_t *pattern;

	// Create the surface
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

	// Ensure the surface was created
	assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);

	// Create a context to draw onto the surface with
	cr = cairo_create(surface);

	// Ensure the context was created
	assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);

	// Get the native width/height of the SVG
	svg_cairo_get_size(svg->svg, &sourceWidth, &sourceHeight);

	// Scale the cairo context such that the SVG is rendered at the desired size
	cairo_scale(cr, (double) width / (double) sourceWidth,
	                (double) height / (double) sourceHeight);

	// Render the SVG to the context
	svg_cairo_render(svg->svg, cr);

	// Create a pattern out of the surface
	pattern = cairo_pattern_create_for_surface(surface);

	// Release the cairo context and surface as they are no longer needed
	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	// Save the rendering information in the vector
	render->pattern = pattern;
	render->patternSize.x = width;
	render->patternSize.y = height;

	// Add the newly rendered image to the renders array and return it
	return vectorAdd(svg->renders, render);
}

void svgManagerInit()
{
	// Make sure that we are not being called twice in a row
	assert(svgImages == NULL);

	svgImages = vectorCreate();
}

void svgManagerQuit()
{
	// Ensure that we have not already been called
	assert(svgImages != NULL);

	// Release all rendered SVG images
	vectorMapAndDestroy(svgImages, svgManagerFreeImage);

	// Note that we have quit
	svgImages = NULL;
}

void svgManagerBlit(cairo_t *cr, const svgRenderedImage *svg)
{
	// Save the state of the current cairo context
	cairo_save(cr);

	// Set the current painting source to be the rendered SVG image
	cairo_set_source(cr, svg->pattern);

	// Draw a rectangle the size of the image
	cairo_rectangle(cr, 0.0, 0.0, svg->patternSize.x, svg->patternSize.y);

	// Fill the rectangle with the pattern
	cairo_fill(cr);

	// Finally, paint the rectangle to the surface
	cairo_paint(cr);

	// Restore the cairo context
	cairo_restore(cr);
}

size svgManagerGetSize(const char *filename, svgSizingPolicy policy, ...)
{
	size size;
	svgImage *image = svgManagerLoad(filename);
	va_list ap;

	va_start(ap, policy);
		size = svgManagerGetSizeInternal(image, policy, ap);
	va_end(ap);

	return size;
}

svgRenderedImage *svgManagerGet(const char *filename, svgSizingPolicy policy, ...)
{
	svgImage *svg;
	svgRenderedImage *currRender, *render = NULL;
	size size;
	va_list ap;

	// Load the SVG image (or fetch it from the cache if it is already loaded)
	svg = svgManagerLoad(filename);

	// Determine the size to render the image at
	va_start(ap, policy);
		size = svgManagerGetSizeInternal(svg, policy, ap);
	va_end(ap);

	// See if the image exists at the desired size
	while ((currRender = vectorNext(svg->renders)))
	{
		// If the sizes match then we have found the render
		if (currRender->patternSize.x == size.x
		 && currRender->patternSize.y == size.y)
		{
			render = currRender;
		}
	}

	// If no render was found then render the SVG at the requested size
	if (render == NULL)
	{
		render = svgManagerRender(svg, size.x, size.y);
	}

	// Return the final rendering
	return render;
}
