/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
	Copyright (C) 2012  Warzone 2100 Project

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

#include "patternManager.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <svg-cairo.h>

static vector *patternList = NULL;

static pattern *patternManagerLookForPattern(const char *id);

/**
 * Deletes a pattern item.
 * This function should NOT be called directly; instead use patternManagerRemove.
 *
 * @param item pattern to destroy.
 * @see patternManagerRemove
 * @see patternManagerQuit
 */
void patternManagerFreePattern(void *item)
{
	free(((pattern *)item)->id);
	cairo_pattern_destroy(((pattern *)item)->crPattern);
}

bool patternManagerRemove(const char *id)
{
	pattern *cur = NULL;
	int index;

	assert(id != NULL);
	assert(patternList != NULL);

	vectorRewind(patternList);
	// Search the pattern list for the corresponding id
	for(index = 0; (cur = vectorNext(patternList)); index++)
	{
		// If the filenames match then we have found the image
		if (strcmp(id, cur->id) == 0)
		{
			patternManagerFreePattern((void *)cur);
			vectorRemoveAt(patternList, index);
			return true;
		}
	}
	return false;
}


/**
 * Copies an existing cairo pattern and adds it to our global list. An existing
 * pattern with the same id will be overwritten while all pointers to the
 * pattern structure will be kept intact.
 *
 * @param id The pattern's id.
 * @param crPattern The cairo pattern.
 * @return A pattern item ready for use.
 */
pattern *patternManagerAddCairoPattern(const char *id, cairo_pattern_t *crPattern)
{
	pattern *item;

	assert(id != NULL);
	assert(crPattern != NULL);

	// check for existing patterns
	item = patternManagerLookForPattern(id);

	if(item != NULL)
	{
		// if there's already a pattern with the desired id then destroy it's cairo pattern.
		cairo_pattern_destroy(item->crPattern);
		item->crPattern = NULL;
	}
	else
	{
		// if there's no occurence of the id we'll create a new item.
		item = malloc(sizeof(pattern));
		assert(item != NULL);
		item->id = strdup(id);
	}

	// copy (or in fact add a reference to) the cairo pattern in our pattern structure
	item->crPattern = cairo_pattern_reference(crPattern);

	// add it to our global list
	vectorAdd(patternList, item);
	return item;
}

pattern *patternManagerGradientCreateLinear(const char *id,
                                            float x0, float y0,
                                            float x1, float y1)
{
	cairo_pattern_t *crPattern;
	pattern *pat;

	assert(id != NULL);

	// create a linear cairo pattern
	crPattern = cairo_pattern_create_linear(x0, y0, x1, y1);

	// and create a pattern for use in betawidget
	pat = patternManagerAddCairoPattern(id, crPattern);

	return pat;
}

pattern *patternManagerGradientCreateRadial(const char *id,
                                            float x0, float y0, float r0,
                                            float x1, float y1, float r1)
{
	cairo_pattern_t *crPattern;
	pattern *pat;

	assert(id != NULL);

	// create a radial cairo pattern
	crPattern = cairo_pattern_create_radial(x0, y0, r0, x1, y1, r1);

	// and create a pattern for use in betawidget
	pat = patternManagerAddCairoPattern(id, crPattern);

	return pat;
}

void patternManagerGradientAddColourStop(pattern *item, float o,
                                         float r, float g, float b, float a)
{
	assert(item != NULL);
	cairo_pattern_add_color_stop_rgba(item->crPattern, o, r, g, b, a);
}

/**
 * Scales the pattern matrix to get the desired size.
 * For this reason, the gradient coordinates should be normalised.
 *
 * @param item The Pattern to set the size of.
 * @param size The desired size.
 * @see patternManagerSetAsSource
 */
void patternManagerSetSize(pattern *item, point size)
{
	cairo_matrix_t matrixScale;
	assert(item != NULL);

	// create a new identity matrix and scale it
	cairo_matrix_init_scale(&matrixScale, 1.0 / size.x, 1.0 / size.y);

	// apply the matrix
	cairo_pattern_set_matrix(item->crPattern, &matrixScale);
}

void patternManagerSetAsSource(cairo_t *cr, pattern *item, float x, float y)
{
	assert(cr != NULL);
	assert(item != NULL);

	point p = {x, y};

	patternManagerSetSize(item, p);
	cairo_set_source(cr, item->crPattern);
}

pattern *patternManagerGetPattern(const char *id)
{
	pattern *item;

	item = patternManagerLookForPattern(id);
	assert(item != NULL);

	return item;
}

/**
 * Returns the pattern by the given Id
 * The pattern is valid until it's removed by patternManagerRemove()
 *
 * Do NOT call this function directly unless you want to know if a
 * pattern exists. Call patternManagerGetPattern instead which adds
 * an assert if the pattern doesn't exist.
 *
 * @param id The pattern id
 * @return The requested pattern
 * @see patternManagerGetPattern
 * @see patternManagerAddCairoPattern
 */
static pattern *patternManagerLookForPattern(const char *id)
{
	pattern *cur, *item;
	assert(id != NULL);
	int i;

	// the function returns NULL when no occurence is found
	cur = NULL;
	item = NULL;

	// Search the pattern for the corresponding id
	vectorRewind(patternList);
	while((cur = vectorNext(patternList)))
	{
		// If the id matches then we have found the pattern
		if (strcmp(id, cur->id) == 0)
		{
			item = cur;
			break;
		}
	}

	return item;
}

void patternManagerInit()
{
	// Make sure that we are not being called twice in a row
	assert(patternList == NULL);

	patternList = vectorCreate();
}

void patternManagerQuit()
{
	// Ensure that we have not already been called
	assert(patternList != NULL);

	// Release all loaded patterns
	vectorMapAndDestroy(patternList, patternManagerFreePattern);

	// Note that we have quit
	patternList = NULL;
}
