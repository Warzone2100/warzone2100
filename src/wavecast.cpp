/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "map.h"
#include "wavecast.h"
#include <vector>
#include <algorithm>
#include <map>

// Angles are sorted in this order. Can only be created and compared to each other, nothing else.
// (1, 0) < (0, 1) < (-1, 0) < (0, -1) < (1, -ε) < (0, 0)
// (0, 0) = special case, compares greater than all other angles.
struct RationalAngle
{
	RationalAngle(int x, int y) : myX(x), myY(y) {}
	bool operator <(RationalAngle const &v2) const
	{
		int x1, y1, x2, y2;
		int qa = quadrant(x1, y1), qb = v2.quadrant(x2, y2);
		return qa != qb ? qa < qb
		       : y1 * x2 < y2 * x1; // Same as y1/x1 < y2/x2, using real numbers. (And works even with x1 or x2 = 0.)
	}
	bool operator ==(RationalAngle const &v2) const
	{
		int x1, y1, x2, y2;
		int qa = quadrant(x1, y1), qb = v2.quadrant(x2, y2);
		return qa == qb && y1 * x2 == y2 * x1; // Same as y1/x1 == y2/x2, using real numbers. (And works even with x1 or x2 = 0.)
	}

private:
	int quadrant(int &x, int &y) const
	{
		int q = myY >= 0 ? (myX >= 0 ? (myX != 0 || myY != 0 ? 0 : 4) : 1)   // Quadrant is arbitrarily chosen for myX == 0 || myY == 0, just needs to be consistent. Quadrant is 4 for myX == 0 && myY == 0.
		        : (myX >= 0 ?                         3      : 2);
		switch (q)
		{
		case 0: x =  myX; y =  myY; break;
		case 1: x =  myY; y = -myX; break;
		case 2: x = -myX; y = -myY; break;
		case 3: x = -myY; y =  myX; break;
		case 4: x = 0;    y = 0;    break;  // myX and myY are both 0, set x, y to arbitrary constant so (0, 0) compares equal to itself.
		}
		return q;
	}

	int myX, myY;
};

// Create a list of all tiles within the given radius, and the leftmost and rightmost angles of the tiles, for field of vision.
// Radius in tiles*TILE_UNITS.
// The droid is assumed to be at (dx = 0.5, dy = 0.5)*TILE_UNITS, not at the origin, since tile data is for the top left of the tile.
static std::vector<WavecastTile> generateWavecastTable(unsigned radius)
{
	std::vector<WavecastTile> tiles;

	std::vector<RationalAngle> unsortedAngles;

	for (unsigned diamond = 1; diamond * TILE_UNITS < radius * 2; ++diamond) // Factor is a bit more than needed to surround the circle with diamonds.
	{
		for (unsigned quadrant = 0; quadrant < 4; ++quadrant)
		{
			for (unsigned s = 0; s < diamond; ++s)
			{
				WavecastTile tile;
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif
				switch (quadrant)
				{
				case 0: tile.dx =  diamond - s;     tile.dy =            s + 1; break;
				case 1: tile.dx =          - s;     tile.dy =  diamond - s;     break;
				case 2: tile.dx = -diamond + s + 1; tile.dy =          - s;     break;
				case 3: tile.dx =            s + 1; tile.dy = -diamond + s + 1; break;
				default:
					debug(LOG_FATAL, "quadrant is unexpected value: %u", quadrant); // Silence later MSVC warning C4701: potentially uninitialized local variable 'tile' used
				}

#if defined( _MSC_VER )
	#pragma warning( pop )
#endif

				const int sdx = tile.dx * 2 - 1, sdy = tile.dy * 2 - 1; // 2*distance from sensor located at (0.5, 0.5)
				const unsigned tileRadiusSq = sdx * sdx + sdy * sdy;

				if (tileRadiusSq * (TILE_UNITS * TILE_UNITS / 4) > radius * radius)
				{
					continue;
				}
				tile.invRadius = i64Sqrt((uint64_t)2 * 65536 * 65536 / tileRadiusSq); // Result is at most 65536, inversely proportional to the radius.

				const int minCorner[4][2] = {{1, 0}, {1, 1}, {0, 1}, {0, 0}};
				const int mcdx = minCorner[quadrant][0], mcdy = minCorner[quadrant][1];  // Corner of the tile which the minimum angle.
				RationalAngle minAngle = RationalAngle(tile.dx - 1 + mcdx, tile.dy - 1 + mcdy), maxAngle = RationalAngle(tile.dx - mcdx, tile.dy - mcdy);
				if (maxAngle < minAngle)
				{
					maxAngle = RationalAngle(0, 0);  // Special case, like RationalAngle(1, 0), except that it compares greater than all other angles instead of less than all other angles.
				}
				tile.angBegin = unsortedAngles.size();
				unsortedAngles.push_back(minAngle);
				tile.angEnd = unsortedAngles.size();
				unsortedAngles.push_back(maxAngle);

				tiles.push_back(tile);
			}
		}
	}

	// Sort the angles and remove duplicates.
	std::vector<RationalAngle> sortedAngles = unsortedAngles;
	std::sort(sortedAngles.begin(), sortedAngles.end());
	sortedAngles.erase(std::unique(sortedAngles.begin(), sortedAngles.end()), sortedAngles.end());

	// Subtitute the angle values angBegin and angEnd with ones that can be compared to each other, so that
	// the angles can be compared without using the unsortedAngles lookup table. (And without using the
	// sortedAngles lookup table either.)
	for (auto &i : tiles)
	{
		i.angBegin = std::lower_bound(sortedAngles.begin(), sortedAngles.end(), unsortedAngles[i.angBegin]) - sortedAngles.begin();
		i.angEnd   = std::lower_bound(sortedAngles.begin(), sortedAngles.end(), unsortedAngles[i.angEnd  ]) - sortedAngles.begin();
	}

#if 0  // Prints wavecast table.
	if (radius == 8 * TILE_UNITS)
	{
		printf("Table for radius %f:\n", radius / (float)TILE_UNITS);
		for (std::vector<WavecastTile>::iterator i = tiles.begin(); i != tiles.end(); ++i)
		{
			printf("Tile%5d: (%3d,%3d): angle %3d-%3d, invRadius %5d\n", (int)(i -/*>*/tiles.begin()), i->dx, i->dy, i->angBegin, i->angEnd, i->invRadius);
		}
		printf("Unsorted angles for radius %f:\n", radius / (float)TILE_UNITS);
		for (std::vector<RationalAngle>::iterator i = unsortedAngles.begin(); i != unsortedAngles.end(); ++i)
		{
			printf("Unsorted angle%5d: (%3d,%3d) = %11.6lf°\n", (int)(i -/*>*/unsortedAngles.begin()), ((int *)&*i)[0], ((int *)&*i)[1], atan2(((int *)&*i)[1], ((int *)&*i)[0]) * 180 / M_PI);    // ((int *)&*i)[0] = very hacky bypass of "private:".
		}
		printf("Sorted angles for radius %f:\n", radius / (float)TILE_UNITS);
		for (std::vector<RationalAngle>::iterator i = sortedAngles.begin(); i != sortedAngles.end(); ++i)
		{
			printf("Sorted angle%5d: (%3d,%3d) = %11.6lf°\n", (int)(i -/*>*/sortedAngles.begin()), ((int *)&*i)[0], ((int *)&*i)[1], atan2(((int *)&*i)[1], ((int *)&*i)[0]) * 180 / M_PI);    // ((int *)&*i)[0] = very hacky bypass of "private:".
		}
	}
	printf("Radius %11.6f summary: %5d tiles, %5d angles (%5d with duplicates)\n", radius / (float)TILE_UNITS, (int)tiles.size(), (int)sortedAngles.size(), (int)unsortedAngles.size());
#endif

	return tiles;
}

// Not thread safe if someone calls with a new radius. Thread safe, otherwise.
const WavecastTile *getWavecastTable(unsigned radius, size_t *size)
{
	static std::map<unsigned, std::vector<WavecastTile> > tables;

	std::vector<WavecastTile> &table = tables[radius];
	if (table.empty())
	{
		// Table not generated yet.
		generateWavecastTable(radius).swap(table);
	}

	*size = table.size();
	return &table[0];
}
