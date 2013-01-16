/*
	This file is part of Warzone 2100.
	Copyright (C) 2009  Elio Gubser
	Copyright (C) 2013  Warzone 2100 Project

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

#ifndef PATTERN_MANAGER_H
#define PATTERN_MANAGER_H

#include "internal-cairo.h"
#include "vector.h"
#include "geom.h"

struct _pattern;
typedef struct _pattern pattern;

/**
 * Represents a pattern
 */
struct _pattern
{
	/// Unique Identifier
	char *id;

	/// The underlying cairo pattern
	cairo_pattern_t *crPattern;
};

/**
 * Initialises the pattern manager. This must be called before any patterns are
 * loaded/used.
 */
void patternManagerInit(void);

/**
 * Uninitialises the pattern manager. Once this method has been called the
 * result of attempting to call any patternManager* methods (without a call to
 * patternManagerInit first) is undefined. Undefined behaviour is also
 * encountered if one attempts to call this method without having first called
 * patternManagerInit.
 */
void patternManagerQuit(void);

/**
 * Creates and Registers an empty linear gradient pattern. The pattern has to
 * be filled with values by using the patternManagerGradientAddColourStop
 * function. The gradient will be drawn along the line from pointStart to
 * pointEnd. Their supposed to be normalized because it will be scaled to the
 * target size afterwards. (e.g. widget size)
 * Old patterns which match this new pattern's id will be overwritten. So
 * pointers to a pattern will be valid.
 *
 * @param id The pattern Identifier
 * @param x0 The X-Coordinate of the start point of the gradient
 * @param y0 The Y-Coordinate of the start point of the gradient
 * @param x1 The X-Coordinate of the end point of the gradient
 * @param y1 The X-Coordinate of the end point of the gradient
 * @see patternManagerAddColourStop Function to fill the pattern with colour stops
 */
pattern *patternManagerGradientCreateLinear(const char *id, float x0, float y0, float x1, float y1);

/**
 * Creates and Registers an empty radial gradient pattern. The pattern has to
 * be filled with values by using the patternManagerGradientAddColourStop
 * function. The gradient will be drawn between two circles defined by
 * (pointStart, radiusStart) and (pointEnd, radiusEnd). The values are supposed
 * to be normalized because it will be scaled to the target size afterwards.
 * (e.g. widget size)
 * Old patterns which match this new pattern's id will be overwritten. So
 * pointers to a pattern will be valid.
 *
 * @param id The pattern Identifier
 * @param x0 The X-Coordinate of the start point of the gradient
 * @param y0 The Y-Coordinate of the start point of the gradient
 * @param r0 The Radius of the circle in the start point
 * @param x1 The X-Coordinate of the end point of the gradient
 * @param y1 The X-Coordinate of the end point of the gradient
 * @param r1 The Radius of the circle in the end point
 * @param radiusEnd The end radius of the gradient
 */
pattern *patternManagerGradientCreateRadial(const char *id,
                                            float x0, float y0, float r0,
                                            float x1, float y1, float r1);

/**
 * Adds a colour stop to a (linear/radial) gradient pattern.
 * @param item The pattern which retrieve the colour stop
 * @param o The offset
 * @param r Red colour component
 * @param g Green colour component
 * @param b Blue colour component
 * @param a Alpha component
 */
void patternManagerGradientAddColourStop(pattern *item, float o,
                                         float r, float g, float b, float a);

/**
 * Removes and deletes a pattern by it's Id. All pointers to that pattern are
 * invalid after calling this function.
 *
 * @param id The id of the pattern which will be removed
 * @return true if pattern was found and removed
 */
bool patternManagerRemove(const char *id);

/**
 * Sets the pattern as the current source of the cairo context
 * and scales it up to x, y so that it matches the widget's size.
 *
 * @param cr The cairo context
 * @param item The used pattern
 * @param x The Size in X-Axis
 * @param y The Size in Y-Axis
 */
void patternManagerSetAsSource(cairo_t *cr, pattern *item, float x, float y);

/**
 * Returns the pattern by the given Id
 * The pattern is valid until it's removed by patternManagerRemove()
 * In fact this function is a wrapper for the internal
 * patternManagerLookForPattern function and adds an assert if the
 * desired pattern exists.
 *
 * @param id The pattern id
 * @return The requested pattern
 * @see patternManagerLookForPattern
 */
pattern *patternManagerGetPattern(const char *id);

#endif /* PATTERN_MANAGER_H */
