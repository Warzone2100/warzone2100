#include "pointtree.h"
#include <algorithm>
#include <vector>

/*
How this works:

Points are sorted by their Morton numbers, which are interleaved x and y coordinate bits.
See: http://en.wikipedia.org/wiki/Morton_number_(number_theory)

When looking for points in a particular area, a search square is split up into 4 rectangles
of varying sizes, and a quick binary search for point in those ranges is performed. The ranges
may overlap, in which case they are combined.

For illustrations, run "make check" and look at tests/pointtree.ppm. (Different image each time.)
The coloured areas are the points in the ranges (note that each range also contains some points
outside the rectangles). The orange points are the search results.
*/

typedef std::pair<uint64_t, void *> POINT;

struct _pointTree
{
	std::vector<POINT> points;

	std::vector<void *> lastQueryResults;
};


// Expands bit pattern abcd efgh to 0a0b 0c0d 0e0f 0g0h
static uint64_t expand(uint32_t x)
{
	uint64_t r = x;
	r = (r | r<<16) & 0x0000FFFF0000FFFF;
	r = (r | r<<8)  & 0x00FF00FF00FF00FF;
	r = (r | r<<4)  & 0x0F0F0F0F0F0F0F0F;
	r = (r | r<<2)  & 0x3333333333333333;
	r = (r | r<<1)  & 0x5555555555555555;
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

POINT_TREE *pointTreeCreate(void)
{
	return new POINT_TREE;
}

void pointTreeDestroy(POINT_TREE *pointTree)
{
	delete pointTree;
}

void pointTreeInsert(POINT_TREE *pointTree, void *point, int32_t x, int32_t y)
{
	pointTree->points.push_back(POINT(interleave(x, y), point));
}

void pointTreeClear(POINT_TREE *pointTree)
{
	pointTree->points.clear();
}

void pointTreeSort(POINT_TREE *pointTree)
{
	std::sort(pointTree->points.begin(), pointTree->points.end());
}

//#define DUMP_IMAGE  // All x and y coordinates must be in range -500 to 499, if dumping an image.
#ifdef DUMP_IMAGE
#include <math.h>
uint8_t ppm[1000][1000][3];
int doDump = false;
#endif //DUMP_IMAGE

void **pointTreeQuery(POINT_TREE *pointTree, int32_t x, int32_t y, int32_t radius)
{
	int32_t minXo = x - radius;
	int32_t maxXo = x + radius;
	int32_t minYo = y - radius;
	int32_t maxYo = y + radius;
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

	uint64_t ranges[4][2] = {{minX    | minY,    splitX1 | splitY1},
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
		int px, py;
		for (py = 0; py != 1000; ++py) for (px = 0; px != 1000; ++px)
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
			if (ranges[0][0] <= nn && nn <= ranges[0][1])
			{
				ppm[py][px][0] /= 2;
			}
			if (ranges[1][0] <= nn && nn <= ranges[1][1])
			{
				ppm[py][px][1] /= 2;
			}
			if (ranges[2][0] <= nn && nn <= ranges[2][1])
			{
				ppm[py][px][2] /= 2;
			}
			if (ranges[3][0] <= nn && nn <= ranges[3][1] && ((ax ^ ay) & 2))
			{
				ppm[py][px][0] /= 2;
				ppm[py][px][1] /= 2;
				ppm[py][px][2] /= 2;
			}
		}
	}
#endif //DUMP_IMAGE

	if (ranges[1][0] > ranges[2][0])
	{
		std::swap(ranges[1][0], ranges[2][0]);
		std::swap(ranges[1][1], ranges[2][1]);
	}
	if (ranges[2][1] + 1 >= ranges[3][0])
	{
		ranges[2][1] = ranges[3][1];
		--numRanges;
	}
	if (ranges[1][1] + 1 >= ranges[2][0])
	{
		ranges[1][1] = ranges[2][1];
		ranges[2][0] = ranges[3][0];
		ranges[2][1] = ranges[3][1];
		--numRanges;
	}
	if (ranges[0][1] + 1 >= ranges[1][0])
	{
		ranges[0][1] = ranges[1][1];
		ranges[1][0] = ranges[2][0];
		ranges[1][1] = ranges[2][1];
		ranges[2][0] = ranges[3][0];
		ranges[2][1] = ranges[3][1];
		--numRanges;
	}

	pointTree->lastQueryResults.clear();
	for (unsigned r = 0; r != numRanges; ++r)
	{
		std::vector<POINT>::iterator i1 = std::lower_bound(pointTree->points.begin(), pointTree->points.end(), POINT(ranges[r][0],     NULL));
		std::vector<POINT>::iterator i2 = std::lower_bound(i1,                        pointTree->points.end(), POINT(ranges[r][1] + 1, NULL));
		for (std::vector<POINT>::const_iterator i = i1; i != i2; ++i)
		{
			uint64_t px = i->first & 0xAAAAAAAAAAAAAAAA;
			uint64_t py = i->first & 0x5555555555555555;
			if (px >= minX && px <= maxX && py >= minY && py <= maxY)  // Only add point if it's at least in the desired square.
			{
				pointTree->lastQueryResults.push_back(i->second);
#ifdef DUMP_IMAGE
				if (doDump)
				{
					ppm[((int32_t *)i->second)[1] + 500][((int32_t *)i->second)[0] + 500][0] = 192;
					ppm[((int32_t *)i->second)[1] + 500][((int32_t *)i->second)[0] + 500][1] = 128;
					ppm[((int32_t *)i->second)[1] + 500][((int32_t *)i->second)[0] + 500][2] = 0;
				}
#endif //DUMP_IMAGE
			}
		}
	}

	pointTree->lastQueryResults.push_back(NULL);  // So it's possible to know when the list ends.

#ifdef DUMP_IMAGE
	if (doDump)
	{
		fwrite(ppm[0][0], 3000000, 1, f);
		fclose(f);
	}
#endif //DUMP_IMAGE

	return &pointTree->lastQueryResults[0];
}
