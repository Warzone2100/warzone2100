/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include <stdio.h>
#include "pointtree.h"
#include <algorithm>
#include <vector>

/*
How this works:

Points are sorted by their Morton numbers, which are interleaved x and y coordinate bits.
See: http://en.wikipedia.org/wiki/Z-order_(curve)
See also: http://en.wikipedia.org/wiki/Morton_number_(number_theory)

When looking for points in a particular area, a search square is split up into 4 rectangles
of varying sizes, and a quick binary search for point in those ranges is performed. The ranges
may overlap, in which case they are combined.

For illustrations, run "make check" and look at tests/pointtree.ppm. (Different image each time.)
The coloured areas are the points in the ranges (note that each range also contains some points
outside the rectangles). The orange points are the search results.
*/

// Expands bit pattern abcd efgh to 0a0b 0c0d 0e0f 0g0h
static uint64_t expand(uint32_t x)
{
	uint64_t r = x;
	r = (r | r<<16) & 0x0000FFFF0000FFFFULL;
	r = (r | r<<8)  & 0x00FF00FF00FF00FFULL;
	r = (r | r<<4)  & 0x0F0F0F0F0F0F0F0FULL;
	r = (r | r<<2)  & 0x3333333333333333ULL;
	r = (r | r<<1)  & 0x5555555555555555ULL;
	return r;
}

// Returns v with highest set bit and all higher bits set, and all following bits 0. Example: 0000 0110 1001 1100 -> 1111 1100 0000 0000.
static uint32_t findSplit(uint32_t v)
{
	v |= v>>1;
	v |= v>>2;
	v |= v>>4;
	v |= v>>8;
	v |= v>>16;
	return ~(v>>1);
}

static uint64_t expandX(int32_t x)
{
	return expand(x + 0x80000000u)<<1;
}

static uint64_t expandY(int32_t y)
{
	return expand(y + 0x80000000u);
}

// Interleaves x and y, but after adding 0x80000000u to both, to make their ranges unsigned.
static uint64_t interleave(int32_t x, int32_t y)
{
	return expandX(x) | expandY(y);
}

void PointTree::insert(void *pointData, int32_t x, int32_t y)
{
	points.push_back(Point(interleave(x, y), pointData));
}

void PointTree::clear()
{
	points.clear();
}

static bool pointTreeSortFunction(std::pair<uint64_t, void *> const &a, std::pair<uint64_t, void *> const &b)
{
	return a.first < b.first;  // Sort only by position, not by pointer address, even if two units are in the same place.
}

void PointTree::sort()
{
	std::stable_sort(points.begin(), points.end(), pointTreeSortFunction);  // Stable sort to avoid unspecified behaviour when two objects are in exactly the same place.
}

//#define DUMP_IMAGE  // All x and y coordinates must be in range -500 to 499, if dumping an image.
#ifdef DUMP_IMAGE
#include <math.h>
uint8_t ppm[1000][1000][3];
int doDump = false;
#endif //DUMP_IMAGE

struct PointTreeRange
{
	uint64_t a, z;
};

// If !IsFiltered, function is trivially optimised to "return i;".
template<bool IsFiltered>
static unsigned current(std::vector<unsigned> &filterData, unsigned i)
{
	unsigned ret = i;
	while (IsFiltered && filterData[ret])
	{
		ret += filterData[ret];
	}
	while (IsFiltered && filterData[i])
	{
		unsigned next = i + filterData[i];
		filterData[i] = ret - i;
		i = next;
	}

	return ret;
}

template<bool IsFiltered>
PointTree::ResultVector &PointTree::queryMaybeFilter(Filter &filter, int32_t minXo, int32_t minYo, int32_t maxXo, int32_t maxYo)
{
	uint64_t minX = expandX(minXo);
	uint64_t maxX = expandX(maxXo);
	uint64_t minY = expandY(minYo);
	uint64_t maxY = expandY(maxYo);

	uint32_t splitXo = maxXo & findSplit(minXo ^ maxXo);
	uint32_t splitYo = maxYo & findSplit(minYo ^ maxYo);

	uint64_t splitX1 = expandX(splitXo - 1);
	uint64_t splitX2 = expandX(splitXo);
	uint64_t splitY1 = expandY(splitYo - 1);
	uint64_t splitY2 = expandY(splitYo);

	PointTreeRange ranges[4] = {{minX    | minY,    splitX1 | splitY1},
	                            {splitX2 | minY,    maxX    | splitY1},
	                            {minX    | splitY2, splitX1 | maxY},
	                            {splitX2 | splitY2, maxX    | maxY}
	                           };
	int numRanges = 4;

#ifdef DUMP_IMAGE
	FILE *f;
	if (doDump)
	{
		f = fopen("pointtree.ppm", "wb");
		fprintf(f, "P6\n1000 1000\n255\n");
		for (int py = 0; py != 1000; ++py) for (int px = 0; px != 1000; ++px)
		{
			int ax = px - 500, ay = py-500;
			double dx = ax - x, dy = ay - y;
			double dist = sqrt(dx*dx + dy*dy);
			ppm[py][px][0] = 255;
			ppm[py][px][1] = 255;
			ppm[py][px][2] = 255;
			if (dist <= radius)
			{
				ppm[py][px][0] = 128;
				ppm[py][px][1] = 128;
				ppm[py][px][2] = 128;
			}
			if (ax == minXo || ax == splitXo || ax == maxXo || ay == minYo || ay == splitYo || ay == maxYo)
			{
				ppm[py][px][0] /= 2;
				ppm[py][px][1] /= 2;
				ppm[py][px][2] /= 2;
			}
			uint64_t nn = expandX(ax) | expandY(ay);
			if (ranges[0].a <= nn && nn <= ranges[0].z)
			{
				ppm[py][px][0] /= 2;
			}
			if (ranges[1].a <= nn && nn <= ranges[1].z)
			{
				ppm[py][px][1] /= 2;
			}
			if (ranges[2].a <= nn && nn <= ranges[2].z)
			{
				ppm[py][px][2] /= 2;
			}
			if (ranges[3].a <= nn && nn <= ranges[3].z && ((ax ^ ay) & 2))
			{
				ppm[py][px][0] /= 2;
				ppm[py][px][1] /= 2;
				ppm[py][px][2] /= 2;
			}
		}
	}
#endif //DUMP_IMAGE

	// Sort ranges ready to be merged.
	if (ranges[1].a > ranges[2].a)
	{
		std::swap(ranges[1], ranges[2]);
	}
	// Merge ranges if needed.
	if (ranges[2].z + 1 >= ranges[3].a)
	{
		ranges[2].z = ranges[3].z;
		--numRanges;
	}
	if (ranges[1].z + 1 >= ranges[2].a)
	{
		ranges[1].z = ranges[2].z;
		ranges[2] = ranges[3];
		--numRanges;
	}
	if (ranges[0].z + 1 >= ranges[1].a)
	{
		ranges[0].z = ranges[1].z;
		ranges[1] = ranges[2];
		ranges[2] = ranges[3];
		--numRanges;
	}

	lastQueryResults.clear();
	if (IsFiltered)
	{
		lastFilteredQueryIndices.clear();
	}
	for (int r = 0; r != numRanges; ++r)
	{
		// Find range of points which may be close enough. Range is [i1 ... i2 - 1]. The pointers are ignored when searching.
		unsigned i1 = std::lower_bound(points.begin(),      points.end(), Point(ranges[r].a, (void *)NULL), pointTreeSortFunction) - points.begin();
		unsigned i2 = std::upper_bound(points.begin() + i1, points.end(), Point(ranges[r].z, (void *)NULL), pointTreeSortFunction) - points.begin();

		for (unsigned i = current<IsFiltered>(filter.data, i1); i < i2; i = current<IsFiltered>(filter.data, i + 1))
		{
			uint64_t px = points[i].first & 0xAAAAAAAAAAAAAAAAULL;
			uint64_t py = points[i].first & 0x5555555555555555ULL;
			if (px >= minX && px <= maxX && py >= minY && py <= maxY)  // Only add point if it's at least in the desired square.
			{
				lastQueryResults.push_back(points[i].second);
				if (IsFiltered)
				{
					lastFilteredQueryIndices.push_back(i);
				}
#ifdef DUMP_IMAGE
				if (doDump)
				{
					ppm[((int32_t *)points[i].second)[1] + 500][((int32_t *)points[i].second)[0] + 500][0] = 192;
					ppm[((int32_t *)points[i].second)[1] + 500][((int32_t *)points[i].second)[0] + 500][1] = 128;
					ppm[((int32_t *)points[i].second)[1] + 500][((int32_t *)points[i].second)[0] + 500][2] = 0;
				}
#endif //DUMP_IMAGE
			}
		}
	}

#ifdef DUMP_IMAGE
	if (doDump)
	{
		fwrite(ppm[0][0], 3000000, 1, f);
		fclose(f);
	}
#endif //DUMP_IMAGE

	return lastQueryResults;
}

PointTree::ResultVector &PointTree::query(int32_t x, int32_t y, uint32_t x2, uint32_t y2)
{
	Filter unused;
	return queryMaybeFilter<false>(unused, x, y, x2, y2);
}

PointTree::ResultVector &PointTree::query(int32_t x, int32_t y, uint32_t radius)
{
	Filter unused;
	int32_t minXo = x - radius;
	int32_t maxXo = x + radius;
	int32_t minYo = y - radius;
	int32_t maxYo = y + radius;
	return queryMaybeFilter<false>(unused, minXo, minYo, maxXo, maxYo);
}

PointTree::ResultVector &PointTree::query(Filter &filter, int32_t x, int32_t y, uint32_t radius)
{
	int32_t minXo = x - radius;
	int32_t maxXo = x + radius;
	int32_t minYo = y - radius;
	int32_t maxYo = y + radius;
	return queryMaybeFilter<true>(filter, minXo, minYo, maxXo, maxYo);
}
