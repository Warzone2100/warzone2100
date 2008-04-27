#ifndef GEOM_H_
#define GEOM_H_

// TODO: Make this cross platform (MSVC)
#include <stdbool.h>

/*
 * A point
 */
typedef struct _point point;

struct _point
{
	int x;
	int y;
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
 * Method signatures
 */

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


#endif /*GEOM_H_*/

