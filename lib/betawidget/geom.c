#include "geom.h"

bool pointInRect(point p, rect r)
{
	return (r.topLeft.x < p.x
		 && r.bottomRight.x > p.x
		 && r.topLeft.y < p.y
		 && r.bottomRight.y > p.y);
}

