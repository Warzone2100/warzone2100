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
 *  Corridor coordination. See corridor_gate.h.
 */

#include "corridor_gate.h"

#include "lib/framework/frame.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/trig.h"

#include "lib/gamelib/gtime.h"

#include "corridor_map.h"
#include "droid.h"
#include "game_world.h"
#include "map.h"
#include "move.h"
#include "pathfinding_backend.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace
{

// How far off a droid's tile to look for a corridor centerline, in tiles. Half
// the widest corridor, since a droid can sit that far to the side of the line.
const int SEARCH_RADIUS = 3;

// How many centerline points ahead to aim the steering, so a droid follows its
// lane smoothly rather than snapping to the nearest point.
const int LOOKAHEAD = 3;

// A droid whose nearest centerline point is within this of a mouth counts as
// still at the entrance rather than through it.
const int MOUTH_MARGIN = 2;

// Which way a droid relates to a corridor this tick.
enum Relation
{
	NOT_IN_CORRIDOR,
	INSIDE,       ///< between the mouths, within the width, following a lane
	APPROACHING,  ///< outside a mouth and heading in, subject to the flow gate
};

struct Query
{
	Relation relation = NOT_IN_CORRIDOR;
	int corridorId = -1;
	size_t nearest = 0;         ///< nearest centerline point index
	Vector2i axis = Vector2i(0, 0); ///< local centerline direction, index-increasing
	int dir = 0;                ///< +1 travelling toward the far mouth, -1 toward the near one
};

// Keep a lane centre this far from a wall and from the centreline, so a droid is
// off the wall and clearly on its own side. About a unit radius.
const int32_t LANE_MARGIN = TILE_UNITS / 2;

// Only a corridor too narrow for a lane each way needs turn-taking. A wider one
// runs both directions at once in lanes.
const int32_t METER_MAX_WIDTH = 2 * TILE_UNITS;

// On a turn-taking corridor the waiting direction queues from this far back, so
// the mouth stays clear for the flow coming out of it.
const int32_t HOLD_STANDOFF = 2 * TILE_UNITS;

// Gap between queued droids on top of their bodies, so the file has room to
// stop and start without shoving.
const int32_t QUEUE_GAP = TILE_UNITS / 4;

// The lane is extended out past a mouth as an approach funnel: a droid this far
// out, heading in, is already drawn onto its own side, so it arrives tucked to the
// side and clear of the flow coming out the mouth, rather than crossing at the
// entrance and catching the corner.
const int32_t APPROACH_RADIUS = 8 * TILE_UNITS;

// Aim this far ahead along the lane, toward and into the mouth, so the funnel
// leads a droid in smoothly instead of at one fixed point it would pile onto.
const int32_t APPROACH_LEAD = 2 * TILE_UNITS;

// Ignore a droid further than this to the side of a mouth. It is not lined up to
// enter here, so the funnel leaves it to its own route.
const int32_t APPROACH_SIDE_MAX = 5 * TILE_UNITS;

// While still far out the funnel aims this much wider than the lane, onto open
// ground beside the corridor's line, tapering to nothing by FUNNEL_MERGE before
// the mouth. The two approaches then run well apart, the middle of the field
// stays clear for the flow coming out, and a droid rounds the entrance corner
// with room and is straight for the last stretch in.
const int32_t FUNNEL_FLARE = 2 * TILE_UNITS;
const int32_t FUNNEL_MERGE = 2 * TILE_UNITS;

// Fixed-point length for the unit direction vectors the funnel projects along.
const int32_t DIR_UNIT = 4096;

// Per-corridor flow direction for this tick: +1, -1, or 0 for open. Rebuilt each
// tick from synced droid state, so it is never saved.
std::vector<int8_t> g_activeDir;

// Per-corridor, whether both directions have droids at it this tick. When only
// one direction is present it fills the whole passage, when both it splits.
std::vector<uint8_t> g_contested;

// Per-droid queue slot for this tick: the position along the approach line,
// negative behind the mouth, that the droid should advance to and wait at.
// The approaching droids of each direction are ranked front to back, so they
// file in one behind another instead of arriving abreast at a mouth that fits
// one or two. Rebuilt each tick from synced droid state, so it is never saved.
std::unordered_map<uint32_t, int32_t> g_slotAlong;

// Corridors whose mouths adjoin form a chain, one constrained passage the
// detector split at junctions and openings. Flow is coordinated over the whole
// chain: whether it is contested, which way it runs, and where the waiting side
// queues. Queuing at an inner mouth would park droids inside the neighbouring
// corridor and block it, so only the chain's outermost mouths hold a queue.
// Derived from corridor geometry once per map and cached.
const CorridorMap *g_chainMap = nullptr;
std::vector<int> g_chainId;          ///< per corridor, chain representative
std::vector<int8_t> g_chainSign;     ///< +1 oriented with the chain, -1 flipped
std::vector<uint8_t> g_mouthAOuter;  ///< per corridor, mouthA adjoins no other corridor
std::vector<uint8_t> g_mouthBOuter;
std::vector<int32_t> g_chainMinWidth;   ///< per corridor, narrowest point of its whole chain

// Mouths within this of each other adjoin, chaining their corridors. A queue
// can park up to APPROACH_RADIUS behind a mouth, so any mouth within that span
// could be corked by another mouth's queue, ex. across the small pocket between
// two passes. Chaining them moves all queuing to the outermost mouths instead.
const int32_t CHAIN_LINK_DIST = APPROACH_RADIUS;

int64_t distSqToFwd(const Vector2i &a, const Vector2i &b)
{
	const Vector2i d = a - b;
	return static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
}

void chainEnsure(const CorridorMap *cmap)
{
	if (g_chainMap == cmap)
	{
		return;
	}
	g_chainMap = cmap;
	const size_t n = cmap->corridors.size();
	g_chainId.assign(n, 0);
	g_chainSign.assign(n, 1);
	g_mouthAOuter.assign(n, 1);
	g_mouthBOuter.assign(n, 1);
	g_chainMinWidth.assign(n, 0);
	for (size_t i = 0; i < n; ++i)
	{
		g_chainMinWidth[i] = cmap->corridors[i].minWidth;
	}

	// Collect every mouth adjacency. Adjacency alone decides inner mouths, so
	// no queue ever parks in a junction pocket, whatever the orientation below.
	struct Link
	{
		int64_t distSq;
		int i;
		int j;
		int8_t rel;
	};
	std::vector<Link> links;
	for (size_t i = 0; i < n; ++i)
	{
		for (size_t j = i + 1; j < n; ++j)
		{
			for (int ei = 0; ei < 2; ++ei)
			{
				for (int ej = 0; ej < 2; ++ej)
				{
					const Vector2i mi = ei ? cmap->corridors[i].mouthB : cmap->corridors[i].mouthA;
					const Vector2i mj = ej ? cmap->corridors[j].mouthB : cmap->corridors[j].mouthA;
					const int64_t dSq = distSqToFwd(mi, mj);
					if (dSq > static_cast<int64_t>(CHAIN_LINK_DIST) * CHAIN_LINK_DIST)
					{
						continue;
					}
					(ei ? g_mouthBOuter : g_mouthAOuter)[i] = 0;
					(ej ? g_mouthBOuter : g_mouthAOuter)[j] = 0;
					// Leaving one corridor's end into the other's opposite end keeps
					// the travel direction, same ends adjoining flips it.
					links.push_back({dSq, static_cast<int>(i), static_cast<int>(j),
					                 static_cast<int8_t>(ei != ej ? 1 : -1)});
				}
			}
		}
	}

	// A loop of passages with an odd orientation twist cannot carry one
	// direction convention, and forcing one makes a single stream crossing the
	// odd link read as two opposing flows. So the flow grouping drops links
	// until a consistent orientation exists, splitting such a loop rather than
	// degrading it. Closest adjacencies merge first, which lands the drop on
	// the most distant one, the roomiest junction, where uncoordinated meeting
	// costs least. Deterministic order, so every client splits identically.
	std::sort(links.begin(), links.end(), [](const Link &a, const Link &b)
	{
		if (a.distSq != b.distSq)
		{
			return a.distSq < b.distSq;
		}
		if (a.i != b.i)
		{
			return a.i < b.i;
		}
		if (a.j != b.j)
		{
			return a.j < b.j;
		}
		return a.rel < b.rel;
	});
	std::vector<int> parent(n);
	std::vector<int8_t> sign(n, 1);   // orientation relative to parent chain root
	std::function<int(int)> find = [&](int x)
	{
		while (parent[x] != x)
		{
			// Path halving, folding the orientation down as we go.
			sign[x] = static_cast<int8_t>(sign[x] * sign[parent[x]]);
			parent[x] = parent[parent[x]];
			x = parent[x];
		}
		return x;
	};
	auto signToRoot = [&](int x)
	{
		int8_t s = 1;
		while (parent[x] != x)
		{
			s = static_cast<int8_t>(s * sign[x]);
			x = parent[x];
		}
		return s;
	};
	std::vector<uint8_t> dropped(links.size(), 0);
	for (bool retry = true; retry;)
	{
		retry = false;
		for (size_t i = 0; i < n; ++i)
		{
			parent[i] = static_cast<int>(i);
			sign[i] = 1;
		}
		for (size_t k = 0; k < links.size(); ++k)
		{
			if (dropped[k])
			{
				continue;
			}
			const Link &l = links[k];
			const int ri = find(l.i);
			const int rj = find(l.j);
			const int8_t si = signToRoot(l.i);
			const int8_t sj = signToRoot(l.j);
			if (ri != rj)
			{
				parent[rj] = ri;
				sign[rj] = static_cast<int8_t>(si * l.rel * sj);
			}
			else if (si * l.rel != sj)
			{
				dropped[k] = 1;
				retry = true;
				break;
			}
		}
	}
	for (size_t i = 0; i < n; ++i)
	{
		g_chainId[i] = find(static_cast<int>(i));
		g_chainSign[i] = signToRoot(static_cast<int>(i));
	}
	for (size_t i = 0; i < n; ++i)
	{
		const size_t root = static_cast<size_t>(g_chainId[i]);
		g_chainMinWidth[root] = std::min(g_chainMinWidth[root], cmap->corridors[i].minWidth);
	}
	for (size_t i = 0; i < n; ++i)
	{
		g_chainMinWidth[i] = g_chainMinWidth[static_cast<size_t>(g_chainId[i])];
	}
}

int64_t distSqTo(const Vector2i &a, const Vector2i &b)
{
	const Vector2i d = a - b;
	return static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
}

Query queryCorridor(const Vector2i &pos, const Vector2i &target)
{
	Query q;
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (cmap == nullptr || cmap->corridors.empty())
	{
		return q;
	}
	const int tx = map_coord(pos.x);
	const int ty = map_coord(pos.y);

	int bestId = -1;
	int bestTileDistSq = SEARCH_RADIUS * SEARCH_RADIUS + 1;
	for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; ++dy)
	{
		for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; ++dx)
		{
			const int16_t id = cmap->at(tx + dx, ty + dy);
			if (id < 0)
			{
				continue;
			}
			const int distSq = dx * dx + dy * dy;
			if (distSq < bestTileDistSq)
			{
				bestTileDistSq = distSq;
				bestId = id;
			}
		}
	}
	if (bestId < 0)
	{
		return q;
	}

	const Corridor &c = cmap->corridors[static_cast<size_t>(bestId)];
	if (c.centerline.size() < 3)
	{
		return q;
	}
	size_t nearest = 0;
	int64_t nearestDistSq = INT64_MAX;
	for (size_t i = 0; i < c.centerline.size(); ++i)
	{
		const Vector2i d = c.centerline[i] - pos;
		const int64_t distSq = static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearest = i;
		}
	}
	const size_t lo = nearest > 0 ? nearest - 1 : nearest;
	const size_t hi = nearest + 1 < c.centerline.size() ? nearest + 1 : nearest;
	const Vector2i axis = c.centerline[hi] - c.centerline[lo];
	if (axis.x == 0 && axis.y == 0)
	{
		return q;
	}

	q.corridorId = bestId;
	q.nearest = nearest;
	q.axis = axis;
	const Vector2i toTarget = target - pos;
	q.dir = (static_cast<int64_t>(toTarget.x) * axis.x + static_cast<int64_t>(toTarget.y) * axis.y) >= 0 ? 1 : -1;

	const size_t last = c.centerline.size() - 1;
	const bool endpoint = (nearest == 0 || nearest == last);
	// Inside means on the passable ground of this cross-section. The room is
	// measured per side, since an even-width passage puts its centerline off
	// centre, and half the total width would misplace the boundary on both
	// sides - droids crowded onto the roomy side flapped in and out of the
	// corridor with sub-tile jitter while standing on plain corridor ground.
	const Vector2i fromLine = pos - c.centerline[nearest];
	const int64_t latRight = static_cast<int64_t>(fromLine.y) * axis.x - static_cast<int64_t>(fromLine.x) * axis.y;
	const int32_t sideRoom = latRight >= 0 ? c.rightExtent[nearest] : c.leftExtent[nearest];
	const int64_t lateral = latRight >= 0 ? latRight : -latRight;
	// At a sharp bend the frame is ill conditioned: the nearest point alternates
	// between the two limbs and the lateral against either limb's axis can read
	// outside while the droid stands almost on the centerline itself. Plain
	// closeness to the line also counts as inside, which no frame can dispute.
	const bool withinWidth = lateral <= static_cast<int64_t>(sideRoom) * iHypot(axis)
	                         || nearestDistSq <= static_cast<int64_t>(TILE_UNITS) * TILE_UNITS;

	if (!endpoint && withinWidth)
	{
		q.relation = INSIDE;
	}
	else if ((nearest <= static_cast<size_t>(MOUTH_MARGIN) && q.dir > 0)
	         || (nearest + static_cast<size_t>(MOUTH_MARGIN) >= last && q.dir < 0))
	{
		q.relation = APPROACHING;   // at a mouth, heading into the corridor
	}
	return q;
}

size_t nearestCenterlineIdx(const Corridor &c, const Vector2i &pos)
{
	size_t nearest = 0;
	int64_t nearestDistSq = INT64_MAX;
	for (size_t i = 0; i < c.centerline.size(); ++i)
	{
		const int64_t distSq = distSqTo(c.centerline[i], pos);
		if (distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearest = i;
		}
	}
	return nearest;
}

// The geometry of an approach to one mouth: the mouth point, unit vectors along
// the travel direction into it and to the axis right, and the droid's projection
// onto the approach line, negative behind the mouth. One shared frame, so the
// slot a droid is ranked at, the point it steers for, and the hold that stops it
// at its slot all measure the same line the same way.
struct ApproachFrame
{
	Vector2i mouth = Vector2i(0, 0);
	Vector2i gVec = Vector2i(0, 0);   ///< into the corridor, length DIR_UNIT
	Vector2i pVec = Vector2i(0, 0);   ///< axis right, length DIR_UNIT
	int32_t along = 0;
};

ApproachFrame approachFrame(const Corridor &c, int dir, const Vector2i &pos)
{
	ApproachFrame f;
	const int last = static_cast<int>(c.centerline.size()) - 1;
	const int inIdx = dir > 0 ? std::min(last, LOOKAHEAD) : std::max(0, last - LOOKAHEAD);
	f.mouth = dir > 0 ? c.centerline.front() : c.centerline.back();
	const Vector2i axis = dir > 0 ? c.centerline[inIdx] - c.centerline[0]
	                              : c.centerline[last] - c.centerline[inIdx];
	const uint16_t axisDir = iAtan2(axis);
	f.gVec = iSinCosR(static_cast<uint16_t>(dir > 0 ? axisDir : axisDir + DEG(180)), DIR_UNIT);
	f.pVec = iSinCosR(static_cast<uint16_t>(axisDir - DEG(90)), DIR_UNIT);
	const Vector2i rel = pos - f.mouth;
	const int64_t along = (static_cast<int64_t>(rel.x) * f.gVec.x + static_cast<int64_t>(rel.y) * f.gVec.y) / DIR_UNIT;
	f.along = std::clamp(static_cast<int32_t>(along), -APPROACH_RADIUS, 0);
	return f;
}

// Which way the droid's route runs along the corridor, +1 toward the far mouth,
// -1 toward the near one, 0 when the route does not say. Judged by how the
// route's waypoints progress along the centerline. The local axis flips on a
// curved corridor, so a group all travelling the same way through a bend would
// read as two opposing flows and stand off against phantom traffic. The route
// is the ground truth for which way a droid is going.
//
// A waypoint only counts as evidence when it is actually near this corridor, or
// past one of its ends along the corridor's own extension. Nearest-point
// projection alone would let a leg of the route that runs through some other
// passage nearby project onto this corridor's far end and reverse the answer,
// flapping the direction as the droid moves.
int routeDir(const DROID *psDroid, const Corridor &c)
{
	const std::vector<Vector2i> &path = psDroid->sMove.asPath;
	const int last = static_cast<int>(c.centerline.size()) - 1;
	const int startIdx = static_cast<int>(nearestCenterlineIdx(c, psDroid->pos.xy()));
	const int start = std::max(psDroid->sMove.pathIndex, 0);
	const int stop = std::min(static_cast<int>(path.size()), start + 6);
	int weak = 0;
	for (int i = start; i < stop; ++i)
	{
		const int idx = static_cast<int>(nearestCenterlineIdx(c, path[i]));
		const int wIdx = std::clamp(idx, std::min(MOUTH_MARGIN, last), std::max(last - MOUTH_MARGIN, 0));
		const int32_t nearRadius = c.widthProfile[wIdx] / 2 + TILE_UNITS;
		bool counts = distSqTo(path[i], c.centerline[static_cast<size_t>(idx)]) <= static_cast<int64_t>(nearRadius) * nearRadius;
		if (!counts && (idx == 0 || idx == last))
		{
			// Past this end, on the corridor's own line out of the mouth.
			const ApproachFrame f = approachFrame(c, idx == 0 ? 1 : -1, path[i]);
			const Vector2i rel = path[i] - f.mouth;
			const int32_t lat = static_cast<int32_t>((static_cast<int64_t>(rel.x) * f.pVec.x
			                                          + static_cast<int64_t>(rel.y) * f.pVec.y) / DIR_UNIT);
			counts = f.along < 0 && lat > -nearRadius && lat < nearRadius;
		}
		if (!counts)
		{
			continue;
		}
		const int diff = idx - startIdx;
		if (diff >= 2)
		{
			return 1;
		}
		if (diff <= -2)
		{
			return -1;
		}
		if ((diff == 1 || diff == -1) && (idx == 0 || idx == last))
		{
			// A single step of progression toward a terminal point, the mouth
			// itself, settles a droid one point from an end that the strong
			// test skips. A single step elsewhere is position noise, ex. a
			// fresh path's first waypoint at the droid's own tile projecting
			// one point back, which would manufacture a reverse direction.
			weak = diff;
		}
	}
	return weak;
}

Query queryDroid(const DROID *psDroid)
{
	Query q = queryCorridor(psDroid->pos.xy(), psDroid->sMove.target);
	if (q.corridorId < 0)
	{
		return q;
	}
	const Corridor &c = gameWorld.map.corridors->corridors[static_cast<size_t>(q.corridorId)];
	const int rd = routeDir(psDroid, c);
	if (rd != 0 && rd != q.dir)
	{
		q.dir = rd;
		// At a mouth, heading in is judged by direction, so re-derive it.
		if (q.relation == APPROACHING)
		{
			const size_t last = c.centerline.size() - 1;
			const bool headingIn = (q.nearest <= static_cast<size_t>(MOUTH_MARGIN) && rd > 0)
			                       || (q.nearest + static_cast<size_t>(MOUTH_MARGIN) >= last && rd < 0);
			if (!headingIn)
			{
				q.relation = NOT_IN_CORRIDOR;
			}
		}
	}
	else if (rd == 0 && q.relation == INSIDE)
	{
		// No route evidence through this corridor at all, ex. a route that
		// leaves through a nearby mouth into another passage, or one just
		// issued that ends close by. Only the local axis is left and its sign
		// flaps with sub-tile position, flipping the lane and the slide clamp
		// each tick, shoving the droid in circles and feeding phantom opposing
		// flow into the counts. A distance condition here just moves the flap
		// to its boundary. With nothing reliable to say, say nothing, and the
		// droid drives its own route.
		return Query();
	}
	return q;
}

Query approachQuery(const Vector2i &pos, const Vector2i &target);

// True if the droid's upcoming route passes through the entry cross-section of
// the corridor. The approach capture is geometric, and where several mouths sit
// a few tiles apart it can catch a droid whose route enters a neighbouring
// passage, steering it away from its own entrance and into a loop between the
// two. The route is the ground truth for where the droid is going, so a mouth
// its route does not pass is not its mouth.
bool routeEntersMouth(const DROID *psDroid, const Corridor &c, int dir)
{
	const std::vector<Vector2i> &path = psDroid->sMove.asPath;
	const int last = static_cast<int>(c.centerline.size()) - 1;
	Vector2i prev = psDroid->pos.xy();
	const int start = std::max(psDroid->sMove.pathIndex, 0);
	const int stop = std::min(static_cast<int>(path.size()), start + 6);
	for (int i = start; i < stop; ++i)
	{
		const Vector2i next = path[i];
		const Vector2i seg = next - prev;
		const int64_t segLenSq = static_cast<int64_t>(seg.x) * seg.x + static_cast<int64_t>(seg.y) * seg.y;
		for (int k = 0; k <= MOUTH_MARGIN + 1; ++k)
		{
			const int idx = dir > 0 ? std::min(k, last) : std::max(last - k, 0);
			const Vector2i p = c.centerline[idx];
			// The passable width here, read a couple in from the end where the
			// measured width balloons into open space.
			const int wIdx = std::clamp(idx, std::min(MOUTH_MARGIN, last), std::max(last - MOUTH_MARGIN, 0));
			const int32_t nearRadius = c.widthProfile[wIdx] / 2 + TILE_UNITS / 2;
			const Vector2i toP = p - prev;
			int64_t distSq;
			const int64_t t = static_cast<int64_t>(toP.x) * seg.x + static_cast<int64_t>(toP.y) * seg.y;
			if (segLenSq == 0 || t <= 0)
			{
				distSq = distSqTo(p, prev);
			}
			else if (t >= segLenSq)
			{
				distSq = distSqTo(p, next);
			}
			else
			{
				const int64_t cross = static_cast<int64_t>(toP.x) * seg.y - static_cast<int64_t>(toP.y) * seg.x;
				distSq = cross * cross / segLenSq;
			}
			if (distSq <= static_cast<int64_t>(nearRadius) * nearRadius)
			{
				return true;
			}
		}
		prev = next;
	}
	return false;
}

// A droid's relation to a corridor: inside it, or heading in from close by, or
// from further out in the open. Used for both the flow count and the lane steer,
// so a corridor is known to be contested as soon as opposing flows head into it,
// not only once they reach the mouth. An approach only counts when the droid's
// route really enters through that mouth.
Query classifyDroid(const DROID *psDroid)
{
	// A droid going nowhere creates no flow, imposes no lanes and joins no
	// queue, however close to a corridor it sits.
	if (psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.asPath.empty())
	{
		return Query();
	}
	Query q = queryDroid(psDroid);
	if (q.corridorId < 0)
	{
		q = approachQuery(psDroid->pos.xy(), psDroid->sMove.target);
	}
	if (q.relation == APPROACHING)
	{
		// Passing the entry cross-section is not enough. A droid that just left
		// through this mouth, or skirts past it, has the mouth on its route too.
		// Only a route that demonstrably progresses inward, in this direction, is
		// an approach.
		const Corridor &c = gameWorld.map.corridors->corridors[static_cast<size_t>(q.corridorId)];
		if (!routeEntersMouth(psDroid, c, q.dir) || routeDir(psDroid, c) != q.dir)
		{
			return Query();
		}
	}
	return q;
}

// Diagnostic logging on --debug=movement. Emits one line per droid whenever its
// corridor classification or queue state changes, and one per chain whenever
// its flow state flips, so stuck traffic can be traced from a live session.
// Reads synced state only, never writes it.
std::unordered_map<uint32_t, uint32_t> g_logClass;
std::unordered_map<uint32_t, int> g_logSpeed;

const char *relName(Relation r)
{
	switch (r)
	{
	case INSIDE: return "inside";
	case APPROACHING: return "approach";
	default: return "none";
	}
}

void logClassification(const DROID *psDroid, const Query &q, int rdir)
{
	const uint32_t packed = static_cast<uint32_t>(q.relation)
	                        | (static_cast<uint32_t>(q.dir + 1) << 2)
	                        | (static_cast<uint32_t>(q.corridorId + 1) << 4);
	auto it = g_logClass.find(psDroid->id);
	if (it != g_logClass.end() && it->second == packed)
	{
		return;
	}
	g_logClass[psDroid->id] = packed;
	debug(LOG_MOVEMENT, "corridor t=%u droid %u p%u tile %d,%d rel=%s c=%d dir=%d routeDir=%d",
	      gameTime, psDroid->id, psDroid->player,
	      map_coord(psDroid->pos.x), map_coord(psDroid->pos.y),
	      relName(q.relation), q.corridorId, q.dir, rdir);
}

// How many body-wide sub-lanes this droid's direction can run abreast, set by
// the narrowest the side band gets over the whole corridor.
int laneCount(const Corridor &c, int dir, const DROID *psDroid)
{
	const std::vector<int32_t> &sideExt = dir > 0 ? c.rightExtent : c.leftExtent;
	const int32_t globalMin = *std::min_element(sideExt.begin(), sideExt.end()) - 2 * LANE_MARGIN;
	const int32_t laneWidth = std::max(2 * moveObjRadius(psDroid), TILE_UNITS / 2);
	return std::max(1, (globalMin + laneWidth / 2) / laneWidth);
}

// A droid out in the open heading into a mouth, past where the centerline search
// reaches. Picks the nearest such mouth so the lane can bias it onto its side well
// before the entrance. Fills the same Query as an inside droid: relation
// APPROACHING, nearest at the mouth end, axis index-increasing, dir toward the far
// mouth (+1 at the front) or the near one (-1 at the back).
Query approachQuery(const Vector2i &pos, const Vector2i &target)
{
	Query q;
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (cmap == nullptr)
	{
		return q;
	}
	int64_t bestDistSq = static_cast<int64_t>(APPROACH_RADIUS) * APPROACH_RADIUS;
	const Vector2i toTarget = target - pos;
	for (size_t ci = 0; ci < cmap->corridors.size(); ++ci)
	{
		const Corridor &c = cmap->corridors[ci];
		if (c.centerline.size() < 3)
		{
			continue;
		}
		const int last = static_cast<int>(c.centerline.size()) - 1;
		for (int end = 0; end < 2; ++end)
		{
			const int mouthIdx = end ? last : 0;
			const int inIdx = end ? std::max(0, last - LOOKAHEAD) : std::min(last, LOOKAHEAD);
			const Vector2i mouth = c.centerline[mouthIdx];
			const Vector2i inward = c.centerline[inIdx] - mouth;   // into the corridor
			const int64_t inwardLen = iHypot(inward);
			if (inwardLen == 0)
			{
				continue;
			}
			const Vector2i toDroid = pos - mouth;
			const int64_t distSq = static_cast<int64_t>(toDroid.x) * toDroid.x + static_cast<int64_t>(toDroid.y) * toDroid.y;
			if (distSq >= bestDistSq)
			{
				continue;
			}
			// Must be outside the mouth (behind it) and heading in.
			if (static_cast<int64_t>(toDroid.x) * inward.x + static_cast<int64_t>(toDroid.y) * inward.y > 0)
			{
				continue;
			}
			if (static_cast<int64_t>(toTarget.x) * inward.x + static_cast<int64_t>(toTarget.y) * inward.y <= 0)
			{
				continue;
			}
			// Lined up in front of the mouth, not off to one side of it.
			const int64_t side = static_cast<int64_t>(toDroid.x) * inward.y - static_cast<int64_t>(toDroid.y) * inward.x;
			if ((side < 0 ? -side : side) / inwardLen > APPROACH_SIDE_MAX)
			{
				continue;
			}
			bestDistSq = distSq;
			q.corridorId = static_cast<int>(ci);
			q.nearest = static_cast<size_t>(mouthIdx);
			q.axis = end ? c.centerline[last] - c.centerline[std::max(0, last - LOOKAHEAD)]
			             : c.centerline[std::min(last, LOOKAHEAD)] - c.centerline[0];
			q.dir = end ? -1 : 1;
			q.relation = APPROACHING;
		}
	}
	return q;
}

} // anonymous namespace

void corridorGateUpdate()
{
	const CorridorMap *cmap = gameWorld.map.corridors.get();
	if (!pathfindingCorridorLanesEnabled() || cmap == nullptr)
	{
		g_activeDir.clear();
		return;
	}
	chainEnsure(cmap);
	const size_t n = cmap->corridors.size();
	// Flow is aggregated over each chain, with member directions mapped into
	// chain orientation, so a passage split at junctions still reads as the one
	// stream of traffic it physically carries.
	std::vector<int> insideFwd(n, 0);
	std::vector<int> insideBwd(n, 0);
	std::vector<uint32_t> lowestId(n, UINT32_MAX);
	std::vector<int8_t> lowestIdDir(n, 0);
	std::vector<uint32_t> lowestInsideId(n, UINT32_MAX);
	std::vector<int8_t> lowestInsideDir(n, 0);
	std::vector<uint8_t> hasFwd(n, 0);
	std::vector<uint8_t> hasBwd(n, 0);

	struct Approacher
	{
		const DROID *droid;
		int corridor;
		int dir;
		int32_t along;
	};
	std::vector<Approacher> approachers;

	for (unsigned player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const DROID *psDroid : gameWorld.objects.droids[player])
		{
			const Query q = classifyDroid(psDroid);
			if (enabled_debug[LOG_MOVEMENT])
			{
				const int rdir = q.corridorId >= 0
				                 ? routeDir(psDroid, cmap->corridors[static_cast<size_t>(q.corridorId)]) : 0;
				logClassification(psDroid, q, rdir);
			}
			if (q.relation == NOT_IN_CORRIDOR)
			{
				continue;
			}
			const size_t c = static_cast<size_t>(q.corridorId);
			const size_t chain = static_cast<size_t>(g_chainId[c]);
			const int chainDir = q.dir * g_chainSign[c];
			(chainDir > 0 ? hasFwd[chain] : hasBwd[chain]) = 1;
			if (q.relation == INSIDE)
			{
				(chainDir > 0 ? insideFwd[chain] : insideBwd[chain]) += 1;
			}
			if (psDroid->id < lowestId[chain])
			{
				lowestId[chain] = psDroid->id;
				lowestIdDir[chain] = static_cast<int8_t>(chainDir);
			}
			if (q.relation == INSIDE && psDroid->id < lowestInsideId[chain])
			{
				lowestInsideId[chain] = psDroid->id;
				lowestInsideDir[chain] = static_cast<int8_t>(chainDir);
			}
			if (q.relation == APPROACHING)
			{
				const Corridor &cor = cmap->corridors[c];
				const ApproachFrame f = approachFrame(cor, q.dir, psDroid->pos.xy());
				approachers.push_back({psDroid, q.corridorId, q.dir, f.along});
			}
		}
	}

	// The flow direction anchors to the oldest droid inside the chain. Later
	// entrants have higher ids, so a running stream keeps refreshing its anchor
	// and holds the chain until it fully drains, then the other side's oldest
	// takes over. An instantaneous majority would instead see-saw when orders
	// change mid-transit, holding and releasing both sides several times a
	// second. A pure function of this tick's synced droids, so saves and late
	// joiners compute it identically.
	g_activeDir.assign(n, 0);
	g_contested.assign(n, 0);
	for (size_t c = 0; c < n; ++c)
	{
		const size_t chain = static_cast<size_t>(g_chainId[c]);
		g_contested[c] = (hasFwd[chain] && hasBwd[chain]) ? 1 : 0;
		const int8_t chainActive = lowestInsideId[chain] != UINT32_MAX
		                           ? lowestInsideDir[chain] : lowestIdDir[chain];
		g_activeDir[c] = static_cast<int8_t>(chainActive * g_chainSign[c]);
	}
	if (enabled_debug[LOG_MOVEMENT])
	{
		static std::vector<int16_t> prevFlow;
		prevFlow.resize(n, INT16_MIN);
		for (size_t c = 0; c < n; ++c)
		{
			if (g_chainId[c] != static_cast<int>(c))
			{
				continue;   // one line per chain, at its root
			}
			const int16_t flow = static_cast<int16_t>(g_contested[c] * 100 + (g_activeDir[c] + 1) * 10
			                                          + (insideFwd[c] > 9 ? 9 : insideFwd[c]));
			if (prevFlow[c] == flow)
			{
				continue;
			}
			prevFlow[c] = flow;
			debug(LOG_MOVEMENT, "corridor t=%u chain %zu contested=%d active=%d insideFwd=%d insideBwd=%d",
			      gameTime, c, g_contested[c], g_activeDir[c], insideFwd[c], insideBwd[c]);
		}
	}

	// Queue the approaching droids of each contested corridor and direction,
	// front to back, one slot each along the approach line, spaced a body plus a
	// gap apart per sub-lane. Each droid advances to its slot and waits, so a
	// blob arrives as files that fit the passage instead of abreast. On a
	// turn-taking chain the waiting direction's queue starts a standoff back,
	// keeping the mouth clear for the flow coming out. Only a chain's outermost
	// mouths queue, a queue at an inner mouth would stand inside the adjoining
	// corridor and block it, so inner junctions always flow through.
	g_slotAlong.clear();
	std::sort(approachers.begin(), approachers.end(), [](const Approacher &a, const Approacher &b)
	{
		if (a.corridor != b.corridor)
		{
			return a.corridor < b.corridor;
		}
		if (a.dir != b.dir)
		{
			return a.dir < b.dir;
		}
		if (a.along != b.along)
		{
			return a.along > b.along;   // closest to the mouth first
		}
		return a.droid->id < b.droid->id;
	});
	size_t i = 0;
	while (i < approachers.size())
	{
		const int corridor = approachers[i].corridor;
		const int dir = approachers[i].dir;
		const size_t c = static_cast<size_t>(corridor);
		const Corridor &cor = cmap->corridors[c];
		const bool entryOuter = dir > 0 ? g_mouthAOuter[c] : g_mouthBOuter[c];
		const bool waiting = g_chainMinWidth[c] < METER_MAX_WIDTH
		                     && g_activeDir[c] != 0 && g_activeDir[c] != dir;
		const int32_t base = waiting ? -HOLD_STANDOFF : 0;
		std::unordered_map<int, int32_t> laneDepth;
		for (; i < approachers.size()
		     && approachers[i].corridor == corridor && approachers[i].dir == dir; ++i)
		{
			if (!g_contested[c])
			{
				continue;   // unopposed flows freely, no queue
			}
			if (!entryOuter)
			{
				// Entering at an inner junction, so already inside the chain's
				// outer mouths and committed. Any wait here stands in a junction
				// pocket and corks it for the through traffic, so it proceeds,
				// lanes keep the encounter sorted, and metering at the outer
				// mouths keeps the conflicting population bounded.
				continue;
			}
			if (approachers[i].along >= 0)
			{
				// At the mouth and committed. Not queued behind it, and never
				// stopped there where it would block the flow coming out.
				continue;
			}
			const DROID *psDroid = approachers[i].droid;
			const int lanes = laneCount(cor, dir, psDroid);
			const int subLane = static_cast<int>(psDroid->id % static_cast<uint32_t>(lanes));
			int32_t &depth = laneDepth[subLane];
			g_slotAlong[psDroid->id] = base - depth;
			depth += 2 * moveObjRadius(psDroid) + QUEUE_GAP;
		}
	}
}

bool corridorLaneTarget(const DROID *psDroid, Vector2i &laneTarget)
{
	const Vector2i pos = psDroid->pos.xy();
	// A droid too far out for the centerline search still gets funnelled in from its
	// approach so it is drawn onto its own lane side before the mouth, leaving the
	// entrance clear rather than blobbing across it. The ones that must wait are held
	// by the flow gate, so steering them costs nothing.
	const Query q = classifyDroid(psDroid);
	if (q.relation != INSIDE && q.relation != APPROACHING)
	{
		return false;
	}
	const size_t cid = static_cast<size_t>(q.corridorId);
	const Corridor &c = gameWorld.map.corridors->corridors[cid];
	const int last = static_cast<int>(c.centerline.size()) - 1;

	// With no opposing flow the passage is not split, so a droid is left to steer
	// through it naturally and the whole width fills on its own. Lanes are only
	// imposed to keep two opposing flows apart.
	if (cid >= g_contested.size() || !g_contested[cid])
	{
		return false;
	}

	// Near the exit the lane would pull a droid onto the mouth point as a blob when
	// its real path continues out into open space. Release it there so it heads for
	// its own target and leaves cleanly. Only an inside droid can be at its exit.
	if (q.relation == INSIDE)
	{
		const int distToExit = q.dir > 0 ? last - static_cast<int>(q.nearest) : static_cast<int>(q.nearest);
		if (distToExit <= MOUTH_MARGIN)
		{
			return false;
		}
	}

	// Place the lane from the real passable room on each side of the centerline.
	// Positive is toward the axis right (the index-increasing side). Where the
	// passage pinches the extents shrink, so the lane pulls in on its own side
	// rather than onto the corner, and it never crosses to the opposing side.
	//
	// A mouth opens into free space, so the extent balloons at the endpoints.
	// Read the room from a point a couple in from either mouth rather than
	// right at it, or a droid at the entrance is flung wide toward that
	// balloon and onto the corner.
	const int loRead = std::min(MOUTH_MARGIN, last);
	const int extIdx = std::clamp(static_cast<int>(q.nearest), loRead, std::max(loRead, last - MOUTH_MARGIN));
	const int32_t rightExt = c.rightExtent[extIdx];
	const int32_t leftExt = c.leftExtent[extIdx];

	// The band of lateral offsets this droid's direction may use, positive toward
	// the axis right. Each direction keeps to its own side of the centerline.
	int32_t bandMin, bandMax;
	if (q.dir > 0)
	{
		bandMin = LANE_MARGIN;
		bandMax = rightExt - LANE_MARGIN;
	}
	else
	{
		bandMin = -(leftExt - LANE_MARGIN);
		bandMax = -LANE_MARGIN;
	}
	if (bandMax < bandMin)
	{
		bandMin = bandMax = (bandMin + bandMax) / 2;   // side too narrow for a full lane
	}

	// Divide the band into sub-lanes a body wide and give this droid a fixed one
	// by id, so same-direction droids run several abreast and fill the passage
	// instead of single filing, a wider body taking a wider slot. The lane count
	// is set by the narrowest point, so it does not change as the droid moves and
	// make it jump sub-lanes, while the positions scale with the local width.
	const int lanes = laneCount(c, q.dir, psDroid);
	const int subLane = static_cast<int>(psDroid->id % static_cast<uint32_t>(lanes));
	const int32_t bandWidth = bandMax - bandMin;
	const int32_t lateral = bandMin + (2 * subLane + 1) * bandWidth / (2 * lanes);

	if (q.relation == APPROACHING)
	{
		// The funnel is an open-field mechanism. At an inner junction there is
		// no room for its flared aim, which fights the previous corridor's lane
		// and the droid's own route in the handoff, arcing the droid in circles
		// as the nearest-corridor classification alternates. The junction is
		// driven on the route alone.
		const bool entryOuter = q.dir > 0 ? (cid < g_mouthAOuter.size() && g_mouthAOuter[cid])
		                                  : (cid < g_mouthBOuter.size() && g_mouthBOuter[cid]);
		if (!entryOuter)
		{
			return false;
		}
		// Drive to this droid's queue slot on the lane extended out past the
		// mouth, so a blob approaches as files that fit the passage, each droid
		// falling in behind the one ahead. Far out the aim is flared wider onto
		// its own side, tapering away by FUNNEL_MERGE before the mouth, so the
		// approaches sort onto their sides while the middle stays clear for the
		// flow coming out.
		const ApproachFrame f = approachFrame(c, q.dir, pos);
		const auto slot = g_slotAlong.find(psDroid->id);
		const int32_t slotAlong = slot != g_slotAlong.end() ? slot->second : f.along;
		// Lead the aim past the slot so the droid keeps driving, through the mouth
		// itself for the front of the queue. Never aim behind the droid, so it
		// advances or waits, not doubles back. The hold stops it at its slot.
		const int32_t aimAlong = std::clamp(std::max(slotAlong, f.along) + APPROACH_LEAD,
		                                    -APPROACH_RADIUS, APPROACH_LEAD);
		int32_t aimLat = lateral;
		if (aimAlong < -FUNNEL_MERGE)
		{
			aimLat += q.dir * FUNNEL_FLARE * (-aimAlong - FUNNEL_MERGE) / (APPROACH_RADIUS - FUNNEL_MERGE);
		}
		laneTarget.x = f.mouth.x + static_cast<int32_t>((static_cast<int64_t>(f.pVec.x) * aimLat + static_cast<int64_t>(f.gVec.x) * aimAlong) / DIR_UNIT);
		laneTarget.y = f.mouth.y + static_cast<int32_t>((static_cast<int64_t>(f.pVec.y) * aimLat + static_cast<int64_t>(f.gVec.y) * aimAlong) / DIR_UNIT);
		return true;
	}

	const uint16_t axisRightDir = static_cast<uint16_t>(iAtan2(q.axis) - DEG(90));
	const int aheadIdx = std::clamp(static_cast<int>(q.nearest) + q.dir * LOOKAHEAD, 0, last);
	laneTarget = c.centerline[aheadIdx] + iSinCosR(axisRightDir, lateral);
	return true;
}

// Over this distance short of its slot a queued droid ramps its speed down, so
// the file compresses and rolls smoothly instead of stopping and starting.
// Shorter than the slot spacing, so a droid runs at full speed for most of each
// advance and only brakes over the last stretch.
const int32_t QUEUE_RAMP = TILE_UNITS / 2;

namespace
{
int queueSpeedInner(const DROID *psDroid, int moveSpeed, const Query &q)
{
	if (q.relation != APPROACHING || q.corridorId < 0)
	{
		return moveSpeed;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= g_contested.size() || !g_contested[c])
	{
		return moveSpeed;   // no opposing flow, so nothing to wait for
	}
	const auto slot = g_slotAlong.find(psDroid->id);
	if (slot == g_slotAlong.end())
	{
		return moveSpeed;
	}
	// Scale speed by the gap to the queue slot: full when the slot is well
	// ahead, slower as it closes, stopped at the slot. The slot advances as the
	// queue ahead enters, so the file rolls forward on its own. A slot at the
	// mouth itself never slows, that droid's turn is to drive in. On a
	// turn-taking corridor the front slot sits a standoff back until this
	// direction has the flow.
	if (slot->second >= 0)
	{
		return moveSpeed;
	}
	const Corridor &cor = gameWorld.map.corridors->corridors[c];
	const int32_t gap = slot->second - approachFrame(cor, q.dir, psDroid->pos.xy()).along;
	if (gap <= 0)
	{
		return 0;
	}
	if (gap >= QUEUE_RAMP)
	{
		return moveSpeed;
	}
	// Floor the ramp so a droid closes its gap briskly rather than crawling the
	// last stretch asymptotically. A slight overrun just packs the file.
	return std::max(static_cast<int>(static_cast<int64_t>(moveSpeed) * gap / QUEUE_RAMP), moveSpeed / 4);
}
} // anonymous namespace

int corridorQueueSpeed(const DROID *psDroid, int moveSpeed)
{
	// The same classification the lane steering uses, so the whole approach
	// queues along the funnel line rather than scrumming near the mouth.
	const Query q = classifyDroid(psDroid);
	const int out = queueSpeedInner(psDroid, moveSpeed, q);
	if (enabled_debug[LOG_MOVEMENT] && moveSpeed > 0)
	{
		// 0 held, 1 ramping, 2 free, logged on change
		const int state = out == 0 ? 0 : (out < moveSpeed ? 1 : 2);
		auto it = g_logSpeed.find(psDroid->id);
		if (it == g_logSpeed.end() || it->second != state)
		{
			g_logSpeed[psDroid->id] = state;
			const auto slot = g_slotAlong.find(psDroid->id);
			const size_t c = q.corridorId >= 0 ? static_cast<size_t>(q.corridorId) : SIZE_MAX;
			debug(LOG_MOVEMENT, "corridor t=%u droid %u tile %d,%d speed %s c=%d slot=%d active=%d",
			      gameTime, psDroid->id,
			      map_coord(psDroid->pos.x), map_coord(psDroid->pos.y),
			      state == 0 ? "held" : (state == 1 ? "ramp" : "free"),
			      q.corridorId,
			      slot != g_slotAlong.end() ? slot->second : INT32_MIN,
			      c < g_activeDir.size() ? g_activeDir[c] : 0);
		}
	}
	return out;
}

void corridorClampSlide(const DROID *psDroid, int32_t *pdx, int32_t *pdy)
{
	const Query q = queryDroid(psDroid);
	if (q.relation != INSIDE)
	{
		return;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= g_contested.size() || !g_contested[c])
	{
		return;
	}
	// Exiting through an inner junction a droid legitimately crosses the
	// centerline, swinging into the next passage of its chain, so release the
	// clamp there or it cuts exactly the crossing motion the droid needs and
	// pins it at the junction. At an outer mouth the clamp stays, it is what
	// keeps the opposing flows apart right where they meet.
	const Corridor &cor = gameWorld.map.corridors->corridors[c];
	const int last = static_cast<int>(cor.centerline.size()) - 1;
	const int distToExit = q.dir > 0 ? last - static_cast<int>(q.nearest) : static_cast<int>(q.nearest);
	const bool exitOuter = q.dir > 0 ? g_mouthBOuter[c] : g_mouthAOuter[c];
	if (distToExit <= MOUTH_MARGIN && !exitOuter)
	{
		return;
	}
	// The droid's lateral position, positive into its own half of the corridor.
	// Well onto its own side, slides resolve collisions within the band as usual.
	const Vector2i rel = psDroid->pos.xy() - cor.centerline[q.nearest];
	const int64_t latPos = q.dir * (static_cast<int64_t>(rel.y) * q.axis.x - static_cast<int64_t>(rel.x) * q.axis.y);
	if (latPos > 0)
	{
		return;
	}
	// At or past the centerline, so cut any further push toward the opposing
	// side and keep only the motion along the corridor. Movement back onto its
	// own side stays free, so a droid bumped across works its way back.
	const int64_t latMove = q.dir * (static_cast<int64_t>(*pdy) * q.axis.x - static_cast<int64_t>(*pdx) * q.axis.y);
	if (latMove >= 0)
	{
		return;
	}
	const int64_t axisSq = static_cast<int64_t>(q.axis.x) * q.axis.x + static_cast<int64_t>(q.axis.y) * q.axis.y;
	*pdx -= static_cast<int32_t>(q.dir * -q.axis.y * latMove / axisSq);
	*pdy -= static_cast<int32_t>(q.dir * q.axis.x * latMove / axisSq);
}

bool corridorManaged(const DROID *psDroid)
{
	// Only a droid queued outside counts as managed. One inside that stops moving
	// long enough to trip the watchdog is genuinely wedged, and the watchdog's
	// reroute is what recovers it, so it keeps its normal watch.
	const Query q = classifyDroid(psDroid);
	if (q.relation != APPROACHING)
	{
		return false;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	return c < g_contested.size() && g_contested[c];
}
