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
 * @param point p
 * @param rect r
 * @return bool
 */
bool pointInRect(point p, rect r);


#endif /*GEOM_H_*/
