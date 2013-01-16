/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
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
