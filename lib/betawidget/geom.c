#include "geom.h"

rect rectFromPointAndSize(point p, size s)
{
	rect r;
	r.topLeft = p;
	r.bottomRight.x = p.x + s.x;
	r.bottomRight.y = p.y + s.y;

	return r;
}

bool pointInRect(point p, rect r)
{
	return (r.topLeft.x < p.x
	     && r.bottomRight.x > p.x
	     && r.topLeft.y < p.y
	     && r.bottomRight.y > p.y);
}

point pointAdd(point p, point q)
{
	point r;
	r.x = p.x + q.x;
	r.y = p.y + q.y;

	return r;
}

point pointSub(point p, point q)
{
	point r;
	r.x = p.x - q.x;
	r.y = p.y - q.y;
	
	return r;
}
