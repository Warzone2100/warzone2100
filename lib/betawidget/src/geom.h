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

#ifndef GEOM_H_
#define GEOM_H_

// TODO: Make this cross platform (MSVC)
#include <stdbool.h>
#include <math.h>

/*
 * Alignment schemes
 */
typedef enum
{
	LEFT,
	CENTRE,
	RIGHT
} hAlign;

typedef enum
{
	TOP,
	MIDDLE,
	BOTTOM
} vAlign;

/*
 * A point
 */
typedef struct _point point;
typedef point size;

struct _point
{
	float x;
	float y;
};

/*
 * A rectangle
 */
typedef struct _rect rect;

struct _rect
{
	point topLeft;
	point bottomRight;
};

/*
 * Helper macros
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
 * Method signatures
 */

/**
 * Creates a rect from an absolute point and a size.
 *
 * @param p
 * @param s
 * @return A rect.
 */
rect rectFromPointAndSize(point p, size s);

/**
 * Checks to see if the point p is inside of the rectangle r.
 *
 * @param p
 * @param r
 * @return If the point is in the rectangle.
 */
bool pointInRect(point p, rect r);

/**
 * Adds point p to point q.
 *
 * @param p
 * @param q
 * @return p + q.
 */
point pointAdd(point p, point q);

/**
 * Subtracts point q from point p.
 * 
 * @param p
 * @param q
 * @return p - q.
 */
point pointSub(point p, point q);


#endif /*GEOM_H_*/
