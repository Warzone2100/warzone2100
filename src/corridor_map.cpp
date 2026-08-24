// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** \file
 *  Corridor detection. See corridor_map.h.
 */

#include "corridor_map.h"

#include "lib/framework/frame.h"

#include "fpath.h"
#include "map.h"
#include "world_map_state.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace
{

// Chamfer weights for the distance transform. An orthogonal step is 2 and a
// diagonal 3, which approximates Euclidean distance scaled by two, so a tile of
// clearance reads as a value of two.
const int32_t CHAMFER_ORTHO = 2;
const int32_t CHAMFER_DIAG = 3;

// A skeleton tile is part of a corridor while its clearance stays below this, in
// chamfer units. Above it the channel has opened into free space and the
// corridor ends. Provisional, calibrated against the synthetic bench corridors.
const int32_t NARROW_DIST_MAX = 5;

// A traced chain is only a corridor if it is actually narrow somewhere.
// The distance transform that seeds the skeleton measures the nearest wall,
// which is small next to an isolated obstacle even where the passage is
// wide, so the real width is checked here against the chain's narrowest
// measured cross-section.
const int32_t MAX_CORRIDOR_WIDTH_WORLD = 5 * TILE_UNITS;

// Traced chains shorter than this many tiles are pruned as skeleton spurs rather
// than kept as corridors.
const size_t MIN_CORRIDOR_TILES = 3;

// The propulsion whose passability defines the ground corridors. VTOLs fly and
// do not queue, so one ground set covers the units that jam.
const PROPULSION_TYPE GROUND_PROP = PROPULSION_TYPE_WHEELED;

struct Grid
{
	int w = 0;
	int h = 0;
	size_t idx(int x, int y) const { return static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x); }
	bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
};

// Clockwise from north, the eight neighbour offsets as Zhang-Suen orders them
// (P2..P9), so the transition count below matches the classic formulation.
const int NB8_X[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
const int NB8_Y[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

// Two-pass integer chamfer distance from every passable tile to the nearest
// blocked tile or edge. Passable tiles start at a large value, blocked at zero.
std::vector<int32_t> distanceTransform(const Grid &g, const std::vector<uint8_t> &passable)
{
	const int32_t INF = std::numeric_limits<int32_t>::max() / 4;
	std::vector<int32_t> dist(static_cast<size_t>(g.w) * static_cast<size_t>(g.h), 0);
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			dist[g.idx(x, y)] = passable[g.idx(x, y)] ? INF : 0;
		}
	}
	auto relax = [&](int x, int y, int nx, int ny, int32_t weight) {
		if (!g.inside(nx, ny))
		{
			return;
		}
		const int32_t cand = dist[g.idx(nx, ny)] + weight;
		if (cand < dist[g.idx(x, y)])
		{
			dist[g.idx(x, y)] = cand;
		}
	};
	// Forward pass, top-left to bottom-right, looking at already-settled neighbours.
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			relax(x, y, x - 1, y, CHAMFER_ORTHO);
			relax(x, y, x, y - 1, CHAMFER_ORTHO);
			relax(x, y, x - 1, y - 1, CHAMFER_DIAG);
			relax(x, y, x + 1, y - 1, CHAMFER_DIAG);
		}
	}
	// Backward pass, bottom-right to top-left, closing the transform.
	for (int y = g.h - 1; y >= 0; --y)
	{
		for (int x = g.w - 1; x >= 0; --x)
		{
			relax(x, y, x + 1, y, CHAMFER_ORTHO);
			relax(x, y, x, y + 1, CHAMFER_ORTHO);
			relax(x, y, x + 1, y + 1, CHAMFER_DIAG);
			relax(x, y, x - 1, y + 1, CHAMFER_DIAG);
		}
	}
	return dist;
}

int foreground(const Grid &g, const std::vector<uint8_t> &skel, int x, int y)
{
	return (g.inside(x, y) && skel[g.idx(x, y)]) ? 1 : 0;
}

// One Zhang-Suen sub-iteration. step 0 and step 1 differ only in the last two
// conditions, which is what alternates the erosion between the two diagonals.
// Returns the number of tiles removed.
size_t thinningPass(const Grid &g, std::vector<uint8_t> &skel, int step)
{
	std::vector<size_t> toClear;
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			if (!skel[g.idx(x, y)])
			{
				continue;
			}
			int p[8];
			for (int i = 0; i < 8; ++i)
			{
				p[i] = foreground(g, skel, x + NB8_X[i], y + NB8_Y[i]);
			}
			int neighbours = 0;
			for (int i = 0; i < 8; ++i)
			{
				neighbours += p[i];
			}
			if (neighbours < 2 || neighbours > 6)
			{
				continue;
			}
			int transitions = 0;
			for (int i = 0; i < 8; ++i)
			{
				if (p[i] == 0 && p[(i + 1) % 8] == 1)
				{
					++transitions;
				}
			}
			if (transitions != 1)
			{
				continue;
			}
			// p indices: 0=N(P2) 1=NE 2=E(P4) 3=SE 4=S(P6) 5=SW 6=W(P8) 7=NW.
			if (step == 0)
			{
				if (p[0] * p[2] * p[4] != 0)
				{
					continue;
				}
				if (p[2] * p[4] * p[6] != 0)
				{
					continue;
				}
			}
			else
			{
				if (p[0] * p[2] * p[6] != 0)
				{
					continue;
				}
				if (p[0] * p[4] * p[6] != 0)
				{
					continue;
				}
			}
			toClear.push_back(g.idx(x, y));
		}
	}
	for (size_t idx : toClear)
	{
		skel[idx] = 0;
	}
	return toClear.size();
}

// After Zhang-Suen, a two-wide diagonal can survive as a mesh, because each of
// its tiles has foreground neighbours in two arcs around it (a local transition
// count of two) though those neighbours are connected to one another. Remove a
// tile when its foreground neighbours stay connected without it, judged by all
// their adjacencies rather than only the arcs around the tile. That reduces such
// a mesh to one tile wide, so a wide diagonal traces as a single line instead of
// overlapping rails. Deterministic raster order, repeated until stable.
void thinRedundant(const Grid &g, std::vector<uint8_t> &skel)
{
	for (;;)
	{
		bool changed = false;
		for (int y = 0; y < g.h; ++y)
		{
			for (int x = 0; x < g.w; ++x)
			{
				if (!skel[g.idx(x, y)])
				{
					continue;
				}
				Vector2i nb[8];
				int n = 0;
				for (int k = 0; k < 8; ++k)
				{
					const int nx = x + NB8_X[k];
					const int ny = y + NB8_Y[k];
					if (g.inside(nx, ny) && skel[g.idx(nx, ny)])
					{
						nb[n++] = Vector2i(nx, ny);
					}
				}
				if (n < 2)
				{
					continue;   // an endpoint or a stray tile stays
				}
				// Flood the neighbours through their own adjacencies. If they all
				// join into one group, the centre is not needed to connect them.
				int seen = 1;
				for (bool grew = true; grew; )
				{
					grew = false;
					for (int i = 0; i < n; ++i)
					{
						if (!(seen & (1 << i))) { continue; }
						for (int j = 0; j < n; ++j)
						{
							if (seen & (1 << j)) { continue; }
							const int ddx = nb[i].x - nb[j].x;
							const int ddy = nb[i].y - nb[j].y;
							if (ddx >= -1 && ddx <= 1 && ddy >= -1 && ddy <= 1)
							{
								seen |= (1 << j);
								grew = true;
							}
						}
					}
				}
				if (seen == (1 << n) - 1)
				{
					skel[g.idx(x, y)] = 0;
					changed = true;
				}
			}
		}
		if (!changed)
		{
			break;
		}
	}
}

// Reduce the passable region to a one-tile-wide skeleton by Zhang-Suen thinning,
// then remove any redundant tiles the thinning left in two-wide diagonal meshes.
std::vector<uint8_t> thinToSkeleton(const Grid &g, const std::vector<uint8_t> &passable)
{
	std::vector<uint8_t> skel = passable;
	for (;;)
	{
		const size_t a = thinningPass(g, skel, 0);
		const size_t b = thinningPass(g, skel, 1);
		if (a == 0 && b == 0)
		{
			break;
		}
	}
	thinRedundant(g, skel);
	return skel;
}

int narrowDegree(const Grid &g, const std::vector<uint8_t> &narrow, int x, int y)
{
	int deg = 0;
	for (int i = 0; i < 8; ++i)
	{
		const int nx = x + NB8_X[i];
		const int ny = y + NB8_Y[i];
		if (g.inside(nx, ny) && narrow[g.idx(nx, ny)])
		{
			++deg;
		}
	}
	return deg;
}

// A dead-end twig shorter than this many tiles, running from a skeleton endpoint
// to a junction, is a thinning artifact and is pruned. Removing it lets the two
// real corridors the junction split rejoin into one.
const int MAX_SPUR_TILES = 5;

// Removes short skeleton spurs so real corridors are not fragmented at every
// twig. Repeats until stable, since pruning one spur can expose another.
void pruneSpurs(const Grid &g, std::vector<uint8_t> &narrow)
{
	for (;;)
	{
		std::vector<size_t> remove;
		for (int y = 0; y < g.h; ++y)
		{
			for (int x = 0; x < g.w; ++x)
			{
				if (!narrow[g.idx(x, y)])
				{
					continue;
				}
				const int deg = narrowDegree(g, narrow, x, y);
				if (deg == 0)
				{
					remove.push_back(g.idx(x, y));   // stray tile, no chain at all
					continue;
				}
				if (deg != 1)
				{
					continue;   // walk only from a free end
				}
				// Walk inward along the twig, collecting it, until a junction or a
				// longer run than a spur.
				std::vector<size_t> branch;
				int px = x, py = y, cx = x, cy = y;
				bool spur = false;
				for (int step = 0; step <= MAX_SPUR_TILES; ++step)
				{
					branch.push_back(g.idx(cx, cy));
					int nx = -1, ny = -1;
					for (int k = 0; k < 8; ++k)
					{
						const int tx = cx + NB8_X[k];
						const int ty = cy + NB8_Y[k];
						if ((tx == px && ty == py) || !g.inside(tx, ty) || !narrow[g.idx(tx, ty)])
						{
							continue;
						}
						nx = tx;
						ny = ty;
						break;
					}
					if (nx < 0)
					{
						break;   // twig with no junction, a free-standing short chain
					}
					if (narrowDegree(g, narrow, nx, ny) >= 3)
					{
						spur = true;   // reached a junction, so the twig fragments a corridor
						break;
					}
					px = cx;
					py = cy;
					cx = nx;
					cy = ny;
				}
				if (spur)
				{
					remove.insert(remove.end(), branch.begin(), branch.end());
				}
			}
		}
		if (remove.empty())
		{
			break;
		}
		for (size_t idx : remove)
		{
			narrow[idx] = 0;
		}
	}
}

Vector2i tileCentre(int x, int y)
{
	return Vector2i(world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2);
}

int clampDir(int d)
{
	return (d > 0) - (d < 0);
}

int countSpan(const Grid &g, const std::vector<uint8_t> &passable, int sx, int sy, int stepX, int stepY)
{
	int count = 0;
	int x = sx + stepX;
	int y = sy + stepY;
	while (g.inside(x, y) && passable[g.idx(x, y)])
	{
		++count;
		x += stepX;
		y += stepY;
	}
	return count;
}

// Physical passable width across the corridor at one centerline tile, in world
// units. Measured perpendicular to the local centerline direction, counting the
// free span each way, so a three-wide and a four-wide passage read apart, which
// the one-sided distance transform cannot. A diagonal cross-section counts each
// step as the longer diagonal spacing.
int32_t measureWidth(const Grid &g, const std::vector<uint8_t> &passable, int tx, int ty, int dirX, int dirY)
{
	const int perpX = -dirY;
	const int perpY = dirX;
	const int steps = countSpan(g, passable, tx, ty, perpX, perpY)
	                + countSpan(g, passable, tx, ty, -perpX, -perpY);
	const bool diagonal = (perpX != 0 && perpY != 0);
	const int32_t stepUnits = diagonal ? (TILE_UNITS * 181 / 128) : TILE_UNITS;
	return TILE_UNITS + steps * stepUnits;
}

// Passable distance from the centerline tile to the wall on each side,
// perpendicular to the local direction, in world units. +perp is the direction's
// right (verified against the movement convention), so it is the right extent.
// Half the centerline tile counts toward each side.
void measureExtents(const Grid &g, const std::vector<uint8_t> &passable, int tx, int ty, int dirX, int dirY,
                    int32_t &rightOut, int32_t &leftOut)
{
	const int perpX = -dirY;
	const int perpY = dirX;
	const bool diagonal = (perpX != 0 && perpY != 0);
	const int32_t stepUnits = diagonal ? (TILE_UNITS * 181 / 128) : TILE_UNITS;
	rightOut = TILE_UNITS / 2 + countSpan(g, passable, tx, ty, perpX, perpY) * stepUnits;
	leftOut = TILE_UNITS / 2 + countSpan(g, passable, tx, ty, -perpX, -perpY) * stepUnits;
}

// True when the full skeleton branches off the narrow set at this tile, meaning
// the passage opens into a wider area here. This is a skeleton tile the narrow
// width threshold excluded, so the medial axis is running out into free space.
// It marks an opening whether the tile is a corridor end or a point along it,
// which is where a unit can enter from the open ground beside a corridor.
bool opensToSpace(const Grid &g, const std::vector<uint8_t> &skel, const std::vector<uint8_t> &narrow, int tx, int ty)
{
	for (int k = 0; k < 8; ++k)
	{
		const int nx = tx + NB8_X[k];
		const int ny = ty + NB8_Y[k];
		if (g.inside(nx, ny) && skel[g.idx(nx, ny)] && !narrow[g.idx(nx, ny)])
		{
			return true;
		}
	}
	return false;
}

// As opensToSpace, but also true when a narrow tile just off the centerline opens
// into space. The tile carrying an opening branch can sit beside the traced line
// (a throat the merge routed around), so the corridor point next to it still has
// to become a mouth. The neighbours along the centerline (prev and next) are
// excluded, since reaching through them would fire beside every ordinary end.
bool opensNearby(const Grid &g, const std::vector<uint8_t> &skel, const std::vector<uint8_t> &narrow,
                 int tx, int ty, const Vector2i &prev, const Vector2i &next)
{
	if (opensToSpace(g, skel, narrow, tx, ty))
	{
		return true;
	}
	for (int k = 0; k < 8; ++k)
	{
		const int nx = tx + NB8_X[k];
		const int ny = ty + NB8_Y[k];
		if ((nx == prev.x && ny == prev.y) || (nx == next.x && ny == next.y))
		{
			continue;   // along the corridor, not off to the side
		}
		if (g.inside(nx, ny) && narrow[g.idx(nx, ny)] && opensToSpace(g, skel, narrow, nx, ny))
		{
			return true;
		}
	}
	return false;
}

// True when a corridor end opens into free space rather than dead-ending.
// It does so if passages meet there (a junction in the narrow skeleton) or the
// passage opens into a wider area. A dead-end against a wall satisfies neither.
bool mouthOpens(const Grid &g, const std::vector<uint8_t> &skel, const std::vector<uint8_t> &narrow, int tx, int ty)
{
	return narrowDegree(g, narrow, tx, ty) >= 3 || opensToSpace(g, skel, narrow, tx, ty);
}

bool tilesAdjacent(const Vector2i &a, const Vector2i &b)
{
	const int dx = a.x - b.x;
	const int dy = a.y - b.y;
	return dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1;
}

// Number of tiles a chain must run back alongside an earlier stretch of itself,
// mirrored point for point, before it counts as a hairpin rather than a passage
// that merely winds close to itself.
const int MIN_FOLD_RUN = 3;

// Splice out any loop where the chain returns to a tile it already visited, so a
// small skeleton loop merged onto the path does not leave the centerline crossing
// itself. The two occurrences are the same tile, so dropping the span between them
// keeps the path continuous.
void removeLoops(std::vector<Vector2i> &chain)
{
	for (size_t k = 1; k < chain.size(); ++k)
	{
		for (size_t i = 0; i < k; ++i)
		{
			if (chain[i].x == chain[k].x && chain[i].y == chain[k].y)
			{
				chain.erase(chain.begin() + static_cast<long>(i) + 1, chain.begin() + static_cast<long>(k) + 1);
				k = i;
				break;
			}
		}
	}
}

// Cut a chain where it doubles back over itself. A wide diagonal passage can thin
// to a hairpin, two parallel lines joined at one end, and tracing it walks down
// one and back up the other. That shows as a run where each point sits beside an
// earlier point in reverse order. A path that only brushes itself at a bend does
// not sustain that mirrored run, so it is left whole.
void trimSelfFold(std::vector<Vector2i> &chain)
{
	removeLoops(chain);
	for (size_t k = 2; k < chain.size(); ++k)
	{
		for (size_t i = 0; i + 1 < k; ++i)
		{
			if (!tilesAdjacent(chain[k], chain[i]))
			{
				continue;
			}
			int run = 1;
			while (run < MIN_FOLD_RUN
			       && k + static_cast<size_t>(run) < chain.size()
			       && i >= static_cast<size_t>(run)
			       && tilesAdjacent(chain[k + run], chain[i - run]))
			{
				++run;
			}
			if (run >= MIN_FOLD_RUN)
			{
				chain.resize(k);
				return;
			}
		}
	}
}

} // anonymous namespace

std::unique_ptr<CorridorMap> corridorMapBuild(const WorldMapState& mapState)
{
	auto result = std::make_unique<CorridorMap>();
	if (!mapState.tiles || mapState.width <= 0 || mapState.height <= 0)
	{
		return result;
	}

	Grid g;
	g.w = mapState.width;
	g.h = mapState.height;
	result->width = g.w;
	result->height = g.h;
	const size_t cells = static_cast<size_t>(g.w) * static_cast<size_t>(g.h);
	result->tileCorridor.assign(cells, -1);

	// Passability for one ground propulsion. A blocked tile is a wall the
	// distance transform measures clearance against.
	std::vector<uint8_t> passable(cells, 0);
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			passable[g.idx(x, y)] = fpathBlockingTile(mapState, x, y, GROUND_PROP) ? 0 : 1;
		}
	}

	const std::vector<int32_t> dist = distanceTransform(g, passable);
	const std::vector<uint8_t> skel = thinToSkeleton(g, passable);

	// Keep only the skeleton where the channel is still narrow. This is the
	// corridor centerline set, open-area skeleton dropped.
	std::vector<uint8_t> narrow(cells, 0);
	for (size_t i = 0; i < cells; ++i)
	{
		narrow[i] = (skel[i] && dist[i] <= NARROW_DIST_MAX) ? 1 : 0;
	}
	pruneSpurs(g, narrow);
	result->debugSkel = skel;
	result->debugNarrow = narrow;

	// Extract skeleton edges, the chains between nodes, where a node is an endpoint
	// (degree <= 1) or a junction (degree >= 3). Each edge records the direction it
	// leaves either end, so junctions can pair edges that continue straight.
	struct SkelEdge
	{
		std::vector<Vector2i> tiles;   // ordered, front and back are the node tiles
		Vector2i awayFront = Vector2i(0, 0);
		Vector2i awayBack = Vector2i(0, 0);
		bool emitted = false;
	};
	std::vector<SkelEdge> edges;
	std::vector<uint8_t> pathUsed(cells, 0);
	std::set<uint64_t> nodePairSeen;   // one edge kept per pair of directly adjacent nodes

	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			if (!narrow[g.idx(x, y)] || narrowDegree(g, narrow, x, y) == 2)
			{
				continue;   // edges begin at nodes only
			}
			for (int dir = 0; dir < 8; ++dir)
			{
				int cx = x + NB8_X[dir];
				int cy = y + NB8_Y[dir];
				if (!g.inside(cx, cy) || !narrow[g.idx(cx, cy)])
				{
					continue;
				}
				if (narrowDegree(g, narrow, cx, cy) != 2)
				{
					const uint64_t lo = std::min<uint64_t>(g.idx(x, y), g.idx(cx, cy));
					const uint64_t hi = std::max<uint64_t>(g.idx(x, y), g.idx(cx, cy));
					if (!nodePairSeen.insert((lo << 32) | hi).second)
					{
						continue;   // already recorded from the other node
					}
				}
				else if (pathUsed[g.idx(cx, cy)])
				{
					continue;   // this chain was already traced from its other end
				}
				SkelEdge e;
				e.tiles.push_back(Vector2i(x, y));
				int px = x, py = y;
				while (narrowDegree(g, narrow, cx, cy) == 2 && !pathUsed[g.idx(cx, cy)])
				{
					pathUsed[g.idx(cx, cy)] = 1;
					e.tiles.push_back(Vector2i(cx, cy));
					int nx = -1, ny = -1;
					for (int k = 0; k < 8; ++k)
					{
						const int tx = cx + NB8_X[k];
						const int ty = cy + NB8_Y[k];
						if ((tx == px && ty == py) || !g.inside(tx, ty) || !narrow[g.idx(tx, ty)])
						{
							continue;
						}
						nx = tx;
						ny = ty;
						break;
					}
					if (nx < 0)
					{
						break;
					}
					px = cx;
					py = cy;
					cx = nx;
					cy = ny;
				}
				e.tiles.push_back(Vector2i(cx, cy));
				const size_t n = e.tiles.size();
				e.awayFront = Vector2i(clampDir(e.tiles[1].x - e.tiles[0].x), clampDir(e.tiles[1].y - e.tiles[0].y));
				e.awayBack = Vector2i(clampDir(e.tiles[n - 2].x - e.tiles[n - 1].x), clampDir(e.tiles[n - 2].y - e.tiles[n - 1].y));
				edges.push_back(std::move(e));
			}
		}
	}

	// At each junction pair the edge ends that continue straight through, so a
	// passage crossing a thinning junction stays one corridor and only a genuine
	// fork splits. An end left unpaired becomes a mouth. pairing is by half-edge,
	// 2*edge + (0 front, 1 back).
	std::vector<int> pairing(edges.size() * 2, -1);
	std::map<size_t, std::vector<int>> incident;
	for (size_t e = 0; e < edges.size(); ++e)
	{
		incident[g.idx(edges[e].tiles.front().x, edges[e].tiles.front().y)].push_back(static_cast<int>(2 * e));
		incident[g.idx(edges[e].tiles.back().x, edges[e].tiles.back().y)].push_back(static_cast<int>(2 * e + 1));
	}
	auto awayOf = [&](int he) -> Vector2i { return (he & 1) ? edges[he >> 1].awayBack : edges[he >> 1].awayFront; };
	for (const auto &kv : incident)
	{
		const std::vector<int> &ends = kv.second;
		if (ends.size() < 3)
		{
			continue;   // an endpoint has nothing to pair
		}
		std::vector<char> paired(ends.size(), 0);
		for (;;)
		{
			int bi = -1, bj = -1, bestDot = 0;   // negative dot means the two branches oppose, so the path runs straight through
			for (size_t i = 0; i < ends.size(); ++i)
			{
				if (paired[i]) { continue; }
				for (size_t j = i + 1; j < ends.size(); ++j)
				{
					if (paired[j]) { continue; }
					const Vector2i da = awayOf(ends[i]);
					const Vector2i db = awayOf(ends[j]);
					const int dot = da.x * db.x + da.y * db.y;
					if (dot < bestDot)
					{
						bestDot = dot;
						bi = static_cast<int>(i);
						bj = static_cast<int>(j);
					}
				}
			}
			if (bi < 0)
			{
				break;   // no straight-through pair remains
			}
			paired[bi] = paired[bj] = 1;
			pairing[ends[bi]] = ends[bj];
			pairing[ends[bj]] = ends[bi];
		}
	}

	// Walk each merged chain from one unpaired end through its pairings to the
	// other, then filter and emit. Starting only at unpaired ends traces each
	// chain once, and a loop with no unpaired end is dropped.
	int nextId = 0;
	auto emitCorridor = [&](const std::vector<Vector2i> &chain)
	{
		if (chain.size() < MIN_CORRIDOR_TILES)
		{
			return;
		}
		const Vector2i a = chain.front();
		const Vector2i b = chain.back();
		if (iHypot(b - a) < 2)
		{
			return;   // mouths coincide or are adjacent, not a spanning passage
		}
		if (!mouthOpens(g, skel, narrow, a.x, a.y) || !mouthOpens(g, skel, narrow, b.x, b.y))
		{
			return;   // a dead-end pocket, not a through-corridor
		}
		std::vector<int32_t> widths(chain.size());
		std::vector<int32_t> rightExt(chain.size());
		std::vector<int32_t> leftExt(chain.size());
		for (size_t i = 0; i < chain.size(); ++i)
		{
			const Vector2i prev = chain[i > 0 ? i - 1 : i];
			const Vector2i next = chain[i + 1 < chain.size() ? i + 1 : i];
			int dirX = clampDir(next.x - prev.x);
			int dirY = clampDir(next.y - prev.y);
			if (dirX == 0 && dirY == 0)
			{
				dirX = 1;
			}
			widths[i] = measureWidth(g, passable, chain[i].x, chain[i].y, dirX, dirY);
			measureExtents(g, passable, chain[i].x, chain[i].y, dirX, dirY, rightExt[i], leftExt[i]);
		}
		const int32_t minW = *std::min_element(widths.begin(), widths.end());
		if (minW > MAX_CORRIDOR_WIDTH_WORLD)
		{
			return;   // wide everywhere, not a corridor
		}
		Corridor c;
		c.id = nextId++;
		c.centerline.reserve(chain.size());
		for (const Vector2i &t : chain)
		{
			c.centerline.push_back(tileCentre(t.x, t.y));
		}
		c.widthProfile = widths;
		c.rightExtent = rightExt;
		c.leftExtent = leftExt;
		c.mouthA = c.centerline.front();
		c.mouthB = c.centerline.back();
		c.minWidth = minW;
		for (size_t i = 1; i + 1 < chain.size(); ++i)
		{
			result->tileCorridor[g.idx(chain[i].x, chain[i].y)] = static_cast<int16_t>(c.id);
		}
		result->corridors.push_back(std::move(c));
	};

	for (size_t e = 0; e < edges.size(); ++e)
	{
		for (int end = 0; end < 2; ++end)
		{
			if (edges[e].emitted)
			{
				break;
			}
			const int startHe = static_cast<int>(2 * e + end);
			if (pairing[startHe] != -1)
			{
				continue;   // begin only at an unpaired end
			}
			std::vector<Vector2i> chain;
			int curHe = startHe;
			for (;;)
			{
				SkelEdge &ce = edges[curHe >> 1];
				ce.emitted = true;
				if ((curHe & 1) == 0)
				{
					for (const Vector2i &t : ce.tiles)
					{
						if (!chain.empty() && chain.back().x == t.x && chain.back().y == t.y) { continue; }
						chain.push_back(t);
					}
				}
				else
				{
					for (size_t k = ce.tiles.size(); k-- > 0; )
					{
						const Vector2i &t = ce.tiles[k];
						if (!chain.empty() && chain.back().x == t.x && chain.back().y == t.y) { continue; }
						chain.push_back(t);
					}
				}
				const int otherHe = (curHe & 1) ? (curHe - 1) : (curHe + 1);
				const int nextHe = pairing[otherHe];
				if (nextHe == -1 || edges[nextHe >> 1].emitted)
				{
					break;
				}
				curHe = nextHe;
			}
			trimSelfFold(chain);

			// Split the chain wherever it passes an opening into free space, so
			// each segment is bounded by places a unit can enter, and every such
			// opening becomes a mouth rather than a point buried mid-corridor.
			std::vector<Vector2i> seg;
			for (size_t i = 0; i < chain.size(); ++i)
			{
				seg.push_back(chain[i]);
				if (i > 0 && i + 1 < chain.size()
				    && opensNearby(g, skel, narrow, chain[i].x, chain[i].y, chain[i - 1], chain[i + 1]))
				{
					emitCorridor(seg);
					seg.assign(1, chain[i]);   // the opening is a shared mouth of both sides
				}
			}
			emitCorridor(seg);
		}
	}

	// The claim mask: ground the corridor layer owns or influences, one bit
	// per tile so other mechanisms exclude it with a single lookup. Claimed
	// means within 3 tiles of any centerline or within the approach reach
	// (8 tiles, matching APPROACH_RADIUS) of any mouth.
	result->tileClaimed.assign(cells, 0);
	result->tileInterior.assign(cells, 0);
	result->tileInteriorCorridor.assign(cells, -1);
	auto stamp = [&](std::vector<uint8_t> &mask, Vector2i world, int radius)
	{
		const int cx = map_coord(world.x);
		const int cy = map_coord(world.y);
		for (int y = std::max(0, cy - radius); y <= std::min(g.h - 1, cy + radius); ++y)
		{
			for (int x = std::max(0, cx - radius); x <= std::min(g.w - 1, cx + radius); ++x)
			{
				mask[g.idx(x, y)] = 1;
			}
		}
	};
	for (const Corridor &c : result->corridors)
	{
		for (const Vector2i &p : c.centerline)
		{
			stamp(result->tileClaimed, p, 3);
			stamp(result->tileInterior, p, 3);
			const int cx = map_coord(p.x);
			const int cy = map_coord(p.y);
			for (int y = std::max(0, cy - 3); y <= std::min(g.h - 1, cy + 3); ++y)
			{
				for (int x = std::max(0, cx - 3); x <= std::min(g.w - 1, cx + 3); ++x)
				{
					result->tileInteriorCorridor[g.idx(x, y)] = static_cast<int16_t>(c.id);
				}
			}
		}
		stamp(result->tileClaimed, c.mouthA, 8);
		stamp(result->tileClaimed, c.mouthB, 8);
	}

	// A fold over the detected geometry.
	// Every movement decision the corridor layer makes is against this map, so clients must agree about it.
	// (The tile masks are stamped from the geometry by fixed rules, so folding the corridors covers them.)
	uint32_t checksum = 0;
	uint32_t factor = 0;
	auto fold = [&checksum, &factor](int32_t v)
	{
		checksum ^= static_cast<uint32_t>(v) * (factor = 3 * factor + 1);
	};
	fold(g.w);
	fold(g.h);
	fold(static_cast<int32_t>(result->corridors.size()));
	for (const Corridor &c : result->corridors)
	{
		fold(c.id);
		fold(c.mouthA.x);
		fold(c.mouthA.y);
		fold(c.mouthB.x);
		fold(c.mouthB.y);
		fold(c.minWidth);
		fold(static_cast<int32_t>(c.centerline.size()));
		for (const Vector2i &p : c.centerline)
		{
			fold(p.x);
			fold(p.y);
		}
		fold(static_cast<int32_t>(c.widthProfile.size()));
		for (int32_t w : c.widthProfile)
		{
			fold(w);
		}
		fold(static_cast<int32_t>(c.leftExtent.size()));
		for (int32_t e : c.leftExtent)
		{
			fold(e);
		}
		fold(static_cast<int32_t>(c.rightExtent.size()));
		for (int32_t e : c.rightExtent)
		{
			fold(e);
		}
	}
	result->checksum = checksum;

	return result;
}
