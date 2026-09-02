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
#include "lib/netplay/sync_debug.h"

#include "ai.h"          // aiCheckAlliances, alliances
#include "corridor_map.h"
#include "droid.h"
#include "game_world.h"
#include "map.h"
#include "move.h"
#include "pathfinding_backend.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace
{

// How many centerline points ahead to aim the steering, so a droid follows its
// lane smoothly rather than snapping to the nearest point. The mouth inward
// lengths the detector precomputes span this same axis.
const int LOOKAHEAD = CORRIDOR_LOOKAHEAD;

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

// Within this many centerline points of a mouth an entering droid may be
// pre-sorted across the opposing lane onto its own side, provided the opposing
// flow's routes stay clear of the zone. Inner mouths have no funnel, so a
// droid arrives on whatever side it came from, and one entering on the wrong
// side of a contested neck fights the oncoming stream for the crossing it
// could have made in the open pocket outside.
const int MOUTH_CROSS_ZONE = 3;

// A zone only reads clear when the opposing flow's covered stretch stays this
// many points short of it, so traffic bound for the zone re-occupies it a few
// tiles ahead of arriving and crossers are out of the way before it comes.
const int SPAN_BUFFER = 3;

// Two opposing bodies thread a pinch when it can separate their centres by the
// sum of their radii plus this control slack, terrain collision being centre
// based. Calibrated between the observed bounds: a cyborg and a medium tank do
// pass each other in a one tile passage, two medium tanks wedge.
const int32_t PASS_MARGIN = 40;

// On a turn-taking corridor the waiting direction queues from this far back, so
// the mouth stays clear for the flow coming out of it.
const int32_t HOLD_STANDOFF = 2 * TILE_UNITS;

// An inside droid bumped against something for longer than this is stalled,
// not just nudging past neighbours in a moving stream. Well under the give-up
// time in moveBlocked, so entrants stop being fed in before the stuck droid
// abandons its move.
const uint32_t STALL_TIME = 1000;

// A column head stalled beyond this is wedged, not draining. Holding entrants
// for it assumes the stall clears, and when what wedged it is the waiting flow
// itself the hold sustains the wedge, so a stall this old stops metering and
// the entrants shuffle through instead.
const uint32_t STALL_WEDGED = 20000;

// Gap between queued droids on top of their bodies, so the file has room to
// stop and start without shoving.
const int32_t QUEUE_GAP = TILE_UNITS / 4;

// The lane is extended out past a mouth as an approach funnel: a droid this far
// out, heading in, is already drawn onto its own side, so it arrives tucked to the
// side and clear of the flow coming out the mouth, rather than crossing at the
// entrance and catching the corner. Exactly as far as mouths chain and as far as
// the claim mask is stamped around every mouth, so capture, claim and chaining
// share one reach.
const int32_t APPROACH_RADIUS = CHAIN_LINK_TILES * TILE_UNITS;

// Aim this far ahead along the lane, toward and into the mouth, so the funnel
// leads a droid in smoothly instead of at one fixed point it would pile onto.
const int32_t APPROACH_LEAD = 2 * TILE_UNITS;

// How far ahead along a passage a droid crossing into it is aimed. Shorter than
// the approach lead on purpose: see the use site.
const int32_t JUNCTION_LEAD = TILE_UNITS;

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

// The flow state itself lives on the GameWorld (CorridorFlowWorld in corridor_gate.h), one state
// per player, each derived every update from the droids of that player and its allies against the
// world's corridor map, so it travels through world swaps with the droids and map it describes and
// coordination never reaches across enemy lines. Chains and mouth adjacency come derived with the
// map (CorridorMap::chains). Flow is coordinated over a whole chain - whether it is contested,
// which way it runs, and where the waiting side queues - and queuing at an inner mouth would park
// droids inside the neighbouring corridor and block it, so only a chain's outermost mouths hold a
// queue.

// Mouths closer than this butt together: a droid crossing is on one passage or
// the other throughout, and handing it the next one's lane while it is still
// departing fights the turn it is making. Only a wider gap has ground to span.
const int32_t JUNCTION_GAP_MIN = 4 * TILE_UNITS;

int64_t distSqTo(const Vector2i &a, const Vector2i &b)
{
	const Vector2i d = a - b;
	return static_cast<int64_t>(d.x) * d.x + static_cast<int64_t>(d.y) * d.y;
}

/// The adjoining passage whose mouth lies nearest the droid. Decide which joint
/// it is first and test that one - searching past a butt joint for a gapped
/// passage further off would hand the droid a corridor it is not entering.
const MouthAdj *mouthNearestJoint(const CorridorChains &chains, size_t cid, bool endB, const Vector2i &pos)
{
	const auto &adj = endB ? chains.mouthBAdj[cid] : chains.mouthAAdj[cid];
	int64_t best = INT64_MAX;
	const MouthAdj *pick = nullptr;
	for (const MouthAdj &a : adj)
	{
		const int64_t d = distSqTo(a.other, pos);
		if (d < best) { best = d; pick = &a; }
	}
	return pick;
}

/// A joint worth holding a lane across: real ground between the two passages,
/// and both of them carrying flow into it this tick. The second half is what
/// separates a corner the two directions share from a junction one flow merely
/// departs across - only the other flow's behavior tells them apart. cid and
/// endB name the mouth whose adjacency list the joint came from.
bool jointHolds(const CorridorFlowState &flow, size_t cid, bool endB, const MouthAdj *a)
{
	if (a == nullptr || a->corridor < 0
	    || a->gapSq < static_cast<int64_t>(JUNCTION_GAP_MIN) * JUNCTION_GAP_MIN)
	{
		return false;
	}
	const size_t other = static_cast<size_t>(a->corridor);
	if (cid >= flow.prevFlow.size() || other >= flow.prevFlow.size())
	{
		return false;   // no update against these corridors yet, so no flow holds the joint
	}
	// Flow heads into a mouth B joint travelling forward (bit 0) and into a
	// mouth A joint travelling backward (bit 1), on each side of the joint.
	const bool mine = (flow.prevFlow[cid] & (endB ? 1 : 2)) != 0;
	const bool theirs = (flow.prevFlow[other] & (a->endB ? 1 : 2)) != 0;
	return mine && theirs;
}

/// True if either mouth of this corridor opens onto a joint that both flows are
/// using. Read by the handedness rule, which has to settle a convention the two
/// directions share, so it cares about the corridor rather than one droid.
bool corridorAtSharedJoint(const CorridorFlowState &flow, const CorridorChains &chains, size_t cid)
{
	for (int ei = 0; ei < 2; ++ei)
	{
		const auto &adj = ei ? chains.mouthBAdj[cid] : chains.mouthAAdj[cid];
		for (const MouthAdj &a : adj)
		{
			if (jointHolds(flow, cid, ei != 0, &a)) { return true; }
		}
	}
	return false;
}

// Bumped whenever any world's joint flow changes (the only cross-tick state the corridor
// classification reads) so that any cached answer taken before a bump is not reused after it.
// One counter across worlds: only the active world is ever queried, and a shared monotonic
// stamp stays correct through world swaps without the memo having to record which world.
uint32_t g_queryStamp = 1;

Query queryCorridor(const GameWorld &world, const CorridorFlowState &flow, const Vector2i &pos, const Vector2i &target)
{
	Query q;
	const CorridorMap *cmap = world.map.corridors.get();
	if (cmap == nullptr || cmap->corridors.empty())
	{
		return q;
	}
	const int tx = map_coord(pos.x);
	const int ty = map_coord(pos.y);
	const int bestId = cmap->nearestCorridor(tx, ty);
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
	const bool withinWidth = lateral <= static_cast<int64_t>(sideRoom) * cmap->chains.axisLen[static_cast<size_t>(bestId)][nearest]
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

	// A droid that has run out of this passage's far end on its way into the
	// next one belongs to neither: past the end so not inside, heading away so
	// not approaching. Hand it to the passage it is entering, so it keeps its
	// side across the junction - only the query's answer changes.
	const bool ranOut = q.relation == NOT_IN_CORRIDOR && endpoint
	                    && ((nearest == last && q.dir > 0) || (nearest == 0 && q.dir < 0));
	if (ranOut && pathfindingBendHoldEnabled())
	{
		const size_t self = static_cast<size_t>(bestId);
		const bool selfEndB = nearest == last;
		const auto &adj = selfEndB ? cmap->chains.mouthBAdj[self] : cmap->chains.mouthAAdj[self];
		const MouthAdj *pick = nullptr;
		int64_t best = INT64_MAX;
		for (const MouthAdj &a : adj)
		{
			const int64_t dSq = distSqTo(a.other, pos);
			// Ties broken by corridor id so every client picks the same one.
			if (dSq < best || (dSq == best && pick != nullptr && a.corridor < pick->corridor))
			{
				best = dSq;
				pick = &a;
			}
		}
		// Only span it if the nearest joint is a real gap. If it is not, say
		// nothing rather than reaching past it to some further passage.
		if (jointHolds(flow, self, selfEndB, pick))
		{
			const Corridor &nc = cmap->corridors[static_cast<size_t>(pick->corridor)];
			if (nc.centerline.size() >= 3)
			{
				const size_t nlast = nc.centerline.size() - 1;
				const size_t nnear = pick->endB ? nlast : 0;
				const size_t nlo = nnear > 0 ? nnear - 1 : nnear;
				const size_t nhi = nnear + 1 < nc.centerline.size() ? nnear + 1 : nnear;
				const Vector2i naxis = nc.centerline[nhi] - nc.centerline[nlo];
				if (naxis.x != 0 || naxis.y != 0)
				{
					q.corridorId = pick->corridor;
					q.nearest = nnear;
					q.axis = naxis;
					q.dir = (static_cast<int64_t>(toTarget.x) * naxis.x
					         + static_cast<int64_t>(toTarget.y) * naxis.y) >= 0 ? 1 : -1;
					q.relation = APPROACHING;
				}
			}
		}
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
int routeDir(const DROID *psDroid, const Corridor &c, bool *strongOut = nullptr,
             int *coverLoOut = nullptr, int *coverHiOut = nullptr)
{
	const std::vector<Vector2i> &path = psDroid->sMove.asPath;
	const int last = static_cast<int>(c.centerline.size()) - 1;
	const int startIdx = static_cast<int>(nearestCenterlineIdx(c, psDroid->pos.xy()));
	const int start = std::max(psDroid->sMove.pathIndex, 0);
	const int stop = std::min(static_cast<int>(path.size()), start + 6);
	if (strongOut != nullptr)
	{
		*strongOut = false;
	}
	// The stretch of the corridor this droid's route actually covers, from its
	// position to where the accepted waypoints run out, ex. where the route
	// turns off. Reads only as far as the scan window, so a distant droid's
	// coverage grows as it closes, a few tiles of anticipation.
	int coverLo = startIdx;
	int coverHi = startIdx;
	int weak = 0;
	int strong = 0;
	for (int i = start; i < stop; ++i)
	{
		const int idx = static_cast<int>(nearestCenterlineIdx(c, path[i]));
		const int wIdx = std::clamp(idx, std::min(MOUTH_MARGIN, last), std::max(last - MOUTH_MARGIN, 0));
		// Capped where the profile balloons, ex. a bowl opening onto open
		// ground mid-line. There the footprint overlaps other passages and
		// their legs of the route would read as this corridor's evidence,
		// and a lane claim in a section that wide says nothing anyway.
		const int32_t nearRadius = std::min(c.widthProfile[wIdx] / 2, TILE_UNITS) + TILE_UNITS;
		bool counts = distSqTo(path[i], c.centerline[static_cast<size_t>(idx)]) <= static_cast<int64_t>(nearRadius) * nearRadius;
		if (!counts && (idx == 0 || idx == last))
		{
			// Past this end, on the corridor's own line out of the mouth, and
			// within the approach zone. Unbounded, the extension of a curved
			// corridor's end can run across distant legs of the route that
			// have nothing to do with this passage, and whether they fall in
			// the scan window flips the answer repath by repath.
			const ApproachFrame f = approachFrame(c, idx == 0 ? 1 : -1, path[i]);
			const Vector2i rel = path[i] - f.mouth;
			const int32_t lat = static_cast<int32_t>((static_cast<int64_t>(rel.x) * f.pVec.x
			                                          + static_cast<int64_t>(rel.y) * f.pVec.y) / DIR_UNIT);
			counts = f.along < 0 && f.along > -APPROACH_RADIUS && lat > -nearRadius && lat < nearRadius;
		}
		if (!counts)
		{
			continue;
		}
		coverLo = std::min(coverLo, idx);
		coverHi = std::max(coverHi, idx);
		const int diff = idx - startIdx;
		if ((diff >= 2 || diff <= -2) && strong == 0)
		{
			if (strongOut != nullptr)
			{
				*strongOut = true;
			}
			strong = diff > 0 ? 1 : -1;
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
	if (coverLoOut != nullptr)
	{
		*coverLoOut = coverLo;
	}
	if (coverHiOut != nullptr)
	{
		*coverHiOut = coverHi;
	}
	return strong != 0 ? strong : weak;
}

// Which side the droid's route next turns toward: -1 left, +1 right, 0 none.
// The first bend past 45 degrees among its upcoming waypoints that is not just
// the corridor winding. Winding means the route stays in the same corridor
// across the bend, where the lane follows the centerline anyway and the bend
// says nothing about sides - counting those made the vote alternate switchback
// by switchback through a winding chain, mirroring the lanes mid-transit.
// A bend that changes context, into another corridor, a junction or open
// ground, is where the route actually departs and casts the vote - judged by
// the route's own before and after, not by where the bend vertex happens to
// sit, since a corner-cutting path can place the vertex of a genuine departure
// a tile inside the corridor it is leaving. A flow's lane preference is a
// property of its route: the side of its next departing turn is the side it
// wants to already be on, and members sharing the same flows then agree on
// handedness by construction.
/// The side a droid's route turns, read from the first sharp bend ahead of it.
/// With a passage given, only a bend at that passage's own mouths answers, and
/// bends elsewhere are stepped over rather than ending the scan.
int routeTurnSide(const GameWorld &world, const CorridorFlowState &flow, const DROID *psDroid,
                  const Corridor *passage = nullptr)
{
	const std::vector<Vector2i> &path = psDroid->sMove.asPath;
	const int start = std::max(psDroid->sMove.pathIndex, 0);
	const int stop = std::min(static_cast<int>(path.size()), start + 8);
	Vector2i prev = psDroid->pos.xy();
	Vector2i seg(0, 0);
	for (int i = start; i < stop; ++i)
	{
		const Vector2i next = path[i] - prev;
		if (iHypot(next) < TILE_UNITS / 2)
		{
			continue;   // too short to carry a direction
		}
		if (seg.x != 0 || seg.y != 0)
		{
			const int64_t cross = static_cast<int64_t>(seg.x) * next.y - static_cast<int64_t>(seg.y) * next.x;
			const int64_t dot = static_cast<int64_t>(seg.x) * next.x + static_cast<int64_t>(seg.y) * next.y;
			if ((cross < 0 ? -cross : cross) > (dot < 0 ? -dot : dot))
			{
				const Query at = queryCorridor(world, flow, prev, prev);
				const Query after = queryCorridor(world, flow, path[i], path[i]);
				const bool winding = at.relation == INSIDE && after.relation == INSIDE
				                     && at.corridorId == after.corridorId;
				bool ours = true;
				if (passage != nullptr && pathfindingTurnVoteEnabled())
				{
					const int64_t reach = static_cast<int64_t>(APPROACH_RADIUS) * APPROACH_RADIUS;
					ours = distSqTo(path[i], passage->mouthA) <= reach
					       || distSqTo(path[i], passage->mouthB) <= reach;
				}
				if (!winding && ours)
				{
					return cross > 0 ? 1 : -1;
				}
			}
		}
		seg = next;
		prev = path[i];
	}
	return 0;
}

Query queryDroid(const GameWorld &world, const CorridorFlowState &flow, const DROID *psDroid,
                 int *coverLoOut = nullptr, int *coverHiOut = nullptr)
{
	Query q = queryCorridor(world, flow, psDroid->pos.xy(), psDroid->sMove.target);
	if (q.corridorId < 0)
	{
		return q;
	}
	const Corridor &c = world.map.corridors->corridors[static_cast<size_t>(q.corridorId)];
	const int rd = routeDir(psDroid, c, nullptr, coverLoOut, coverHiOut);
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
		//
		// Unless the chain settles it. A route that cuts the corner never
		// touches this passage's line, but if it runs through a passage
		// adjoining one of the mouths, the direction is not in doubt: toward
		// that mouth. Only when exactly one mouth answers - a droid whose
		// route uses both keeps the silence rather than a tie-break.
		if (pathfindingBendHoldEnabled())
		{
			const CorridorChains &chains = world.map.corridors->chains;
			const size_t ci = static_cast<size_t>(q.corridorId);
			int hits = 0;
			int chainDir = 0;
			const MouthAdj *chainPick = nullptr;
			bool chainPickEndB = false;
			for (int end = 0; end < 2; ++end)
			{
				const auto &adj = end ? chains.mouthBAdj[ci] : chains.mouthAAdj[ci];
				for (const MouthAdj &a : adj)
				{
					if (a.corridor < 0) { continue; }
					const Corridor &nc = world.map.corridors->corridors[static_cast<size_t>(a.corridor)];
					if (nc.centerline.size() >= 3 && routeDir(psDroid, nc) != 0)
					{
						++hits;
						chainDir = end ? 1 : -1;
						chainPick = &a;
						chainPickEndB = end != 0;
						break;
					}
				}
			}
			// One unambiguous answer, and the joint it names has ground to span.
			if (hits == 1 && jointHolds(flow, ci, chainPickEndB, chainPick))
			{
				q.dir = chainDir;
				return q;
			}
		}
		return Query();
	}
	return q;
}

Query approachQuery(const GameWorld &world, const Vector2i &pos, const Vector2i &target);

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
//
// coverLoOut and coverHiOut report how far along the corridor the droid's route
// reaches, filled from the scan this already runs, so a caller wanting it does
// not have to call routeDir again. Only written when a corridor was found.
Query classifyDroid(const GameWorld &world, const CorridorFlowState &flow, const DROID *psDroid,
                    int *coverLoOut = nullptr, int *coverHiOut = nullptr)
{
	// A droid going nowhere creates no flow, imposes no lanes and joins no
	// queue, however close to a corridor it sits. Neither does one fighting on
	// the move, whose movement is combat micro, not route following - its
	// direction through a corridor flaps with every retarget, and one such
	// droid steering by lanes circles in place and pollutes the flow votes.
	if (psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.asPath.empty()
	    || psDroid->action == DACTION_MOVEFIRE)
	{
		return Query();
	}
	Query q = queryDroid(world, flow, psDroid, coverLoOut, coverHiOut);
	if (q.corridorId < 0)
	{
		q = approachQuery(world, psDroid->pos.xy(), psDroid->sMove.target);
	}
	if (q.relation == APPROACHING)
	{
		// Passing the entry cross-section is not enough. A droid that just left
		// through this mouth, or skirts past it, has the mouth on its route too,
		// and one whose route merely ENDS beside the mouth leaves only terminal
		// evidence, which slotted droids into an intake they never meant to use.
		// Only a route that demonstrably penetrates inward, in this
		// direction, is an approach.
		const Corridor &c = world.map.corridors->corridors[static_cast<size_t>(q.corridorId)];
		bool strong = false;
		if (!routeEntersMouth(psDroid, c, q.dir) || routeDir(psDroid, c, &strong, coverLoOut, coverHiOut) != q.dir || !strong)
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

// How many body-wide sub-lanes fit in a band this wide.
int lanesForBand(int32_t bandWidth, const DROID *psDroid)
{
	const int32_t laneWidth = std::max(2 * moveObjRadius(psDroid), TILE_UNITS / 2);
	return std::max(1, (bandWidth + laneWidth / 2) / laneWidth);
}

// How many body-wide sub-lanes this droid's direction can run abreast, set by
// the narrowest the side band gets over the whole corridor. side is positive
// for the band right of the axis.
int laneCount(const Corridor &c, int side, const DROID *psDroid)
{
	const int32_t narrowest = side > 0 ? c.minRightExtent : c.minLeftExtent;
	return lanesForBand(narrowest - 2 * LANE_MARGIN, psDroid);
}

// True when the queue outside this mouth has the approach to itself. Only outer
// mouths queue, so a corridor whose far mouth is an inner junction forms exactly
// one queue. Where both mouths are outer the two queues share a short passage,
// and the depth of a single file is what holds them apart, not the standoff:
// widening such a queue puts both crowds on the mouth together.
bool queueOwnsApproach(const CorridorChains &chains, size_t cid, int dir)
{
	const std::vector<uint8_t> &farOuter = dir > 0 ? chains.mouthBOuter : chains.mouthAOuter;
	return farOuter[cid] == 0;
}

// How many files the queue outside a mouth runs abreast. The queue stands on
// the open ground the funnel flares across, not in the corridor, so the side
// band does not bound it: size it by the width the funnel reaches, and let the
// taper converge the files into the corridor's lane at the mouth. Only used
// where queueOwnsApproach.
int approachLaneCount(const Corridor &c, int side, const DROID *psDroid)
{
	const int32_t narrowest = side > 0 ? c.minRightExtent : c.minLeftExtent;
	return lanesForBand(narrowest - 2 * LANE_MARGIN + FUNNEL_FLARE, psDroid);
}

// A droid out in the open heading into a mouth, past where the centerline search
// reaches. Picks the nearest such mouth so the lane can bias it onto its side well
// before the entrance. Fills the same Query as an inside droid: relation
// APPROACHING, nearest at the mouth end, axis index-increasing, dir toward the far
// mouth (+1 at the front) or the near one (-1 at the back).
Query approachQuery(const GameWorld &world, const Vector2i &pos, const Vector2i &target)
{
	Query q;
	const CorridorMap *cmap = world.map.corridors.get();
	if (cmap == nullptr)
	{
		return q;
	}
	// The claim mask reaches APPROACH_RADIUS around every mouth, so a droid off it is beyond what any
	// funnel here reaches (regardless of which mouth is nearest).
	if (!cmap->claimed(map_coord(pos.x), map_coord(pos.y)))
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
			const int64_t inwardLen = (end ? cmap->chains.inwardLenB : cmap->chains.inwardLenA)[ci];
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

/// One cohort's flow for this tick, measured over the droids of cohortPlayer and its allies.
/// canonical names the player whose pass carries each owner's droids, so the classification log
/// still emits one line per droid however the cohorts share their computation.
static void corridorFlowCompute(const GameWorld &world, CorridorFlowState &flow, unsigned cohortPlayer,
                                const std::array<int, MAX_PLAYERS> &canonical)
{
	const CorridorMap *cmap = world.map.corridors.get();
	const size_t n = cmap->corridors.size();
	const CorridorChains &chains = cmap->chains;
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
	std::vector<int32_t> occWidthFwd(n, INT32_MAX);   ///< per chain, narrowest member this direction occupies
	std::vector<int32_t> occWidthBwd(n, INT32_MAX);
	std::vector<int32_t> maxRadFwd(n, 0);             ///< per chain, largest body of this direction's traffic
	std::vector<int32_t> maxRadBwd(n, 0);
	std::vector<uint8_t> stalledFwd(n, 0);            ///< per chain, this direction's flow is stuck, not draining
	std::vector<uint8_t> stalledBwd(n, 0);
	// Corridor-local claims for the route-shaping configuration: contest and
	// direction scoped to the one passage a droid actually transits, so a
	// queue at one corridor of a chain does not hold entrants of another
	// across open ground. Presence propagates physically instead, opposing
	// units near a shared pocket sit in both corridors' capture zones, so
	// pocket chains still meter, while disjoint corridors flow concurrently.
	std::vector<uint8_t> cHasFwd(n, 0);
	std::vector<uint8_t> cHasBwd(n, 0);
	std::vector<int32_t> cMaxRadFwd(n, 0);
	std::vector<int32_t> cMaxRadBwd(n, 0);
	std::vector<uint32_t> cLowestId(n, UINT32_MAX);
	std::vector<int8_t>  cLowestIdDir(n, 0);
	std::vector<uint32_t> cLowestInsideId(n, UINT32_MAX);
	std::vector<int8_t>  cLowestInsideDir(n, 0);
	flow.useLoFwd.assign(n, INT32_MAX);
	flow.useHiFwd.assign(n, INT32_MIN);
	flow.useLoBwd.assign(n, INT32_MAX);
	flow.useHiBwd.assign(n, INT32_MIN);
	flow.cInsideFwd.assign(n, 0);
	flow.cInsideBwd.assign(n, 0);
	flow.cFlowFwd.assign(n, 0);
	flow.cFlowBwd.assign(n, 0);

	// Only the head of a direction's column can stall a corridor. Followers bump
	// the file ahead of them constantly in a healthy compressing stream, so their
	// bumps say nothing, while the front droid stuck against something it cannot
	// pass means the flow is not draining.
	struct ColumnHead
	{
		int32_t prog = INT32_MIN;   ///< centerline index signed by direction, larger is further along
		uint8_t stalled = 0;
	};
	std::vector<ColumnHead> headFwd(n), headBwd(n);   ///< per corridor and direction

	struct Approacher
	{
		const DROID *droid;
		int corridor;
		int dir;
		int32_t along;
	};
	std::vector<Approacher> approachers;

	// Turn-side preference per corridor and direction, anchored to the oldest
	// member that expresses one. The vote is route-intrinsic, so the same droid
	// votes the same in every member it touches and adjacent members carrying
	// the same flows agree on handedness by construction, lanes running
	// continuous across their junctions. Members with no preference cannot hold
	// the anchor: same-flow voters agree with each other, so the vote stays
	// stable however the anchor identity churns among them, where anchoring on
	// the oldest member outright let one droid with a flapping direction steal
	// the anchor each tick and erase its own flow's vote.
	struct SidePref
	{
		uint32_t id = UINT32_MAX;
		int8_t side = 0;
		uint8_t firm = 0;   ///< a member is inside or committed to the approach
		int left = 0;       ///< members whose route turns that way at this passage
		int right = 0;

		/// The side this direction asks for, or none while its members disagree
		/// evenly. Counted rather than held on one member, so no single early
		/// voter sets the convention alone.
		int8_t settled() const
		{
			return static_cast<int8_t>(left > right ? -1 : (right > left ? 1 : 0));
		}
	};
	std::vector<SidePref> prefFwd(n), prefBwd(n);

	for (unsigned owner = 0; owner < MAX_PLAYERS; ++owner)
	{
		if (owner != cohortPlayer && !aiCheckAlliances(cohortPlayer, owner))
		{
			continue;   // enemy units contribute nothing, coordination stays within the alliance
		}
		for (const DROID *psDroid : world.objects.droids[owner])
		{
			int coverLo = 0;
			int coverHi = 0;
			const Query q = classifyDroid(world, flow, psDroid, &coverLo, &coverHi);
			if (enabled_debug[LOG_MOVEMENT] && canonical[owner] == static_cast<int>(cohortPlayer))
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
			const size_t chain = static_cast<size_t>(chains.chainId[c]);
			const int chainDir = q.dir * chains.chainSign[c];
			(chainDir > 0 ? hasFwd[chain] : hasBwd[chain]) = 1;
			(q.dir > 0 ? cHasFwd : cHasBwd)[c] = 1;
			{
				int32_t &crad = (q.dir > 0 ? cMaxRadFwd : cMaxRadBwd)[c];
				crad = std::max(crad, moveObjRadius(psDroid));
				if (psDroid->id < cLowestId[c])
				{
					cLowestId[c] = psDroid->id;
					cLowestIdDir[c] = static_cast<int8_t>(q.dir);
				}
				if (q.relation == INSIDE && psDroid->id < cLowestInsideId[c])
				{
					cLowestInsideId[c] = psDroid->id;
					cLowestInsideDir[c] = static_cast<int8_t>(q.dir);
				}
			}
			// The claim also reaches chain siblings the droid physically
			// stands near, so opposing traffic just across a shared pocket
			// still meters the next member, while members separated by real
			// ground stay independent until the traffic actually closes in.
			for (int member : chains.chainMembers[chain])
			{
				const size_t c2 = static_cast<size_t>(member);
				if (c2 == c)
				{
					continue;
				}
				const Corridor &sib = cmap->corridors[c2];
				const int64_t reach = static_cast<int64_t>(APPROACH_RADIUS) * APPROACH_RADIUS;
				if (distSqTo(psDroid->pos.xy(), sib.mouthA) > reach
				    && distSqTo(psDroid->pos.xy(), sib.mouthB) > reach)
				{
					continue;
				}
				const int8_t sibDir = static_cast<int8_t>(chainDir * chains.chainSign[c2]);
				(sibDir > 0 ? cHasFwd : cHasBwd)[c2] = 1;
				int32_t &srad = (sibDir > 0 ? cMaxRadFwd : cMaxRadBwd)[c2];
				srad = std::max(srad, moveObjRadius(psDroid));
			}
			{
				int &lo = (q.dir > 0 ? flow.useLoFwd : flow.useLoBwd)[c];
				int &hi = (q.dir > 0 ? flow.useHiFwd : flow.useHiBwd)[c];
				lo = std::min(lo, coverLo);
				hi = std::max(hi, coverHi);
			}
			(q.dir > 0 ? flow.cFlowFwd : flow.cFlowBwd)[c] += 1;
			if (q.relation == INSIDE)
			{
				(chainDir > 0 ? insideFwd[chain] : insideBwd[chain]) += 1;
				(q.dir > 0 ? flow.cInsideFwd : flow.cInsideBwd)[c] += 1;
				int32_t &occ = chainDir > 0 ? occWidthFwd[chain] : occWidthBwd[chain];
				occ = std::min(occ, cmap->corridors[c].minWidth);
				const bool bumped = psDroid->sMove.bumpTime != 0
				                    && psDroid->sMove.bumpTime <= gameTime
				                    && gameTime - psDroid->sMove.bumpTime > STALL_TIME
				                    && gameTime - psDroid->sMove.bumpTime <= STALL_WEDGED;
				ColumnHead &head = q.dir > 0 ? headFwd[c] : headBwd[c];
				const int32_t prog = q.dir * static_cast<int32_t>(q.nearest);
				if (prog > head.prog)
				{
					head.prog = prog;
					head.stalled = bumped ? 1 : 0;
				}
				else if (prog == head.prog && bumped)
				{
					head.stalled = 1;
				}
			}
			int32_t &rad = chainDir > 0 ? maxRadFwd[chain] : maxRadBwd[chain];
			rad = std::max(rad, moveObjRadius(psDroid));
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
			SidePref &pref = q.dir > 0 ? prefFwd[c] : prefBwd[c];
			if (pathfindingTurnVoteEnabled())
			{
				const int side = routeTurnSide(world, flow, psDroid, &cmap->corridors[c]);
				if (side < 0) { pref.left += 1; }
				else if (side > 0) { pref.right += 1; }
			}
			else if (psDroid->id < pref.id)
			{
				const int side = routeTurnSide(world, flow, psDroid);
				if (side != 0)
				{
					pref.id = psDroid->id;
					pref.side = static_cast<int8_t>(side);
				}
			}
			if (q.relation == INSIDE)
			{
				pref.firm = 1;
			}
			if (q.relation == APPROACHING)
			{
				const Corridor &cor = cmap->corridors[c];
				const ApproachFrame f = approachFrame(cor, q.dir, psDroid->pos.xy());
				if (f.along > -APPROACH_RADIUS / 2)
				{
					pref.firm = 1;
				}
				approachers.push_back({psDroid, q.corridorId, q.dir, f.along});
			}
		}
	}

	for (size_t c = 0; c < n; ++c)
	{
		const size_t chain = static_cast<size_t>(chains.chainId[c]);
		if (headFwd[c].stalled)
		{
			(chains.chainSign[c] > 0 ? stalledFwd : stalledBwd)[chain] = 1;
		}
		if (headBwd[c].stalled)
		{
			(chains.chainSign[c] > 0 ? stalledBwd : stalledFwd)[chain] = 1;
		}
	}

	// The flow direction anchors to the oldest droid inside the chain. Later
	// entrants have higher ids, so a running stream keeps refreshing its anchor
	// and holds the chain until it fully drains, then the other side's oldest
	// takes over. An instantaneous majority would instead see-saw when orders
	// change mid-transit, holding and releasing both sides several times a
	// second. A pure function of this tick's synced droids, so saves and late
	// joiners compute it identically.
	flow.activeDir.assign(n, 0);
	flow.contested.assign(n, 0);
	for (size_t c = 0; c < n; ++c)
	{
		const size_t chain = static_cast<size_t>(chains.chainId[c]);
		if (pathfindingDirectionalBiasEnabled())
		{
			flow.contested[c] = (cHasFwd[c] && cHasBwd[c]) ? 1 : 0;
			flow.activeDir[c] = cLowestInsideId[c] != UINT32_MAX
			                 ? cLowestInsideDir[c] : cLowestIdDir[c];
		}
		else
		{
			flow.contested[c] = (hasFwd[chain] && hasBwd[chain]) ? 1 : 0;
			const int8_t chainActive = lowestInsideId[chain] != UINT32_MAX
			                           ? lowestInsideDir[chain] : lowestIdDir[chain];
			flow.activeDir[c] = static_cast<int8_t>(chainActive * chains.chainSign[c]);
		}
	}

	// Lane handedness per corridor: keep left only when at least one direction
	// exits to its left and neither exits to its right, so the turning flow
	// gets the inside of its turn instead of crossing the opposing lane at it.
	// Any conflict or ambiguity keeps the default.
	// Record which directions have flow at each corridor, read from here to the
	// next update to decide whether a joint carries both - by the handedness
	// rule below and by the next tick's classification alike.
	flow.prevFlow.assign(n, 0);
	for (size_t i = 0; i < n; ++i)
	{
		flow.prevFlow[i] = static_cast<uint8_t>((flow.cFlowFwd[i] > 0 ? 1 : 0) | (flow.cFlowBwd[i] > 0 ? 2 : 0));
	}
	flow.prevFlowChecksum = cmap->checksum;

	flow.handed.assign(n, 1);
	for (size_t c = 0; c < n; ++c)
	{
		// A preference only participates once its direction is committed, a
		// member inside or well within the approach. A droid hovering at the
		// capture boundary toggles its whole management status with sub-tile
		// motion, and its preference appearing and vanishing flapped the
		// handedness mid-episode.
		const int8_t fwdSide = pathfindingTurnVoteEnabled() ? prefFwd[c].settled() : prefFwd[c].side;
		const int8_t bwdSide = pathfindingTurnVoteEnabled() ? prefBwd[c].settled() : prefBwd[c].side;
		const int8_t pf = prefFwd[c].firm ? fwdSide : 0;
		const int8_t pb = prefBwd[c].firm ? bwdSide : 0;
		// At a joint both flows use the convention belongs to both, and one
		// arriving first must not set it alone: the other arriving would
		// reverse it under everyone already steering at it. At every other
		// mouth a lone voter is all there will ever be, so it still decides.
		const bool shared = pathfindingHandJointEnabled() && corridorAtSharedJoint(flow, chains, c);
		const bool keepLeft = shared ? (pf == -1 && pb == -1)
		                             : ((pf == -1 || pb == -1) && pf != 1 && pb != 1);
		if (keepLeft)
		{
			flow.handed[c] = -1;
		}
	}
	if (enabled_debug[LOG_MOVEMENT])
	{
		static std::array<std::vector<int8_t>, MAX_PLAYERS> prevHandedPerPlayer;
		std::vector<int8_t> &prevHanded = prevHandedPerPlayer[cohortPlayer];
		prevHanded.resize(n, 0);
		for (size_t c = 0; c < n; ++c)
		{
			if (prevHanded[c] != flow.handed[c])
			{
				prevHanded[c] = flow.handed[c];
				debug(LOG_MOVEMENT, "corridor t=%u corridor %zu handed=%s sharedJoint=%d prefFwd=%d(L%d R%d firm %d) prefBwd=%d(L%d R%d firm %d)",
				      gameTime, c, flow.handed[c] > 0 ? "right" : "left",
				      corridorAtSharedJoint(flow, chains, c) ? 1 : 0,
				      pathfindingTurnVoteEnabled() ? prefFwd[c].settled() : prefFwd[c].side,
				      prefFwd[c].left, prefFwd[c].right, prefFwd[c].firm,
				      pathfindingTurnVoteEnabled() ? prefBwd[c].settled() : prefBwd[c].side,
				      prefBwd[c].left, prefBwd[c].right, prefBwd[c].firm);
			}
		}
	}
	if (enabled_debug[LOG_MOVEMENT])
	{
		static std::array<std::vector<int16_t>, MAX_PLAYERS> prevFlowPerPlayer;
		std::vector<int16_t> &prevFlow = prevFlowPerPlayer[cohortPlayer];
		prevFlow.resize(n, INT16_MIN);
		for (size_t c = 0; c < n; ++c)
		{
			if (chains.chainId[c] != static_cast<int>(c))
			{
				continue;   // one line per chain, at its root
			}
			const int16_t packed = static_cast<int16_t>((stalledFwd[c] * 2 + stalledBwd[c]) * 1000
			                                            + flow.contested[c] * 100 + (flow.activeDir[c] + 1) * 10
			                                            + (insideFwd[c] > 9 ? 9 : insideFwd[c]));
			if (prevFlow[c] == packed)
			{
				continue;
			}
			prevFlow[c] = packed;
			debug(LOG_MOVEMENT, "corridor t=%u p%u chain %zu contested=%d active=%d insideFwd=%d insideBwd=%d stalledFwd=%d stalledBwd=%d",
			      gameTime, cohortPlayer, c, flow.contested[c], flow.activeDir[c], insideFwd[c], insideBwd[c], stalledFwd[c], stalledBwd[c]);
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
	flow.slotAlong.clear();
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
		const bool entryOuter = dir > 0 ? chains.mouthAOuter[c] : chains.mouthBOuter[c];
		// A pinch only meters a pair of flows that cannot thread it: too narrow
		// to separate the two largest bodies' centres with control slack. Wait
		// while the opposing flow actually occupies such a member, rather than
		// because one exists, so a wide entrance streams in lanes whenever the
		// pinches are clear, and a cyborg flow passes an opposing tank flow
		// through a passage that fits the pair. Entering an unthreadable
		// corridor directly still uses the flow direction, or two flows racing
		// into an empty one would meet head-on inside it. A flow that fits on
		// paper but has a member stuck against something is not draining, so
		// entrants also wait while the opposing flow has a stalled droid
		// inside, rather than feeding a column against it.
		const size_t chain = static_cast<size_t>(chains.chainId[c]);
		const bool fwdSide = dir * chains.chainSign[c] > 0;
		int32_t fitNeed = maxRadFwd[chain] + maxRadBwd[chain] + PASS_MARGIN;
		int32_t oppOccWidth = (fwdSide ? occWidthBwd : occWidthFwd)[chain];
		bool oppStalled = (fwdSide ? stalledBwd : stalledFwd)[chain] != 0;
		if (pathfindingDirectionalBiasEnabled())
		{
			fitNeed = cMaxRadFwd[c] + cMaxRadBwd[c] + PASS_MARGIN;
			const bool oppInside = (dir > 0 ? flow.cInsideBwd : flow.cInsideFwd)[c] > 0;
			oppOccWidth = oppInside ? cor.minWidth : INT32_MAX;
			oppStalled = (dir > 0 ? headBwd : headFwd)[c].stalled != 0;
		}
		const bool waiting = oppOccWidth < fitNeed
		                     || oppStalled
		                     || (cor.minWidth < fitNeed
		                         && flow.activeDir[c] != 0 && flow.activeDir[c] != dir);
		const int32_t base = waiting ? -HOLD_STANDOFF : 0;
		std::unordered_map<int, int32_t> laneDepth;
		for (; i < approachers.size()
		     && approachers[i].corridor == corridor && approachers[i].dir == dir; ++i)
		{
			if (!flow.contested[c])
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
			const int lanes = (pathfindingWideQueueEnabled() && queueOwnsApproach(chains, c, dir))
			                  ? approachLaneCount(cor, dir * flow.handed[c], psDroid)
			                  : laneCount(cor, dir * flow.handed[c], psDroid);
			const int subLane = static_cast<int>(psDroid->id % static_cast<uint32_t>(lanes));
			int32_t &depth = laneDepth[subLane];
			flow.slotAlong[psDroid->id] = base - depth;
			depth += 2 * moveObjRadius(psDroid) + QUEUE_GAP;
		}
	}

	ASSERT(flow.contested.size() == n && flow.activeDir.size() == n && flow.handed.size() == n
	       && flow.prevFlow.size() == n,
	       "corridor flow sized for %zu corridors on a map with %zu", flow.contested.size(), n);
}

void corridorGateUpdate(GameWorld &world)
{
	CorridorFlowWorld &wf = world.corridorFlow;
	const CorridorMap *cmap = world.map.corridors.get();
	if (!pathfindingCorridorLanesEnabled() || cmap == nullptr)
	{
		for (CorridorFlowState &flow : wf.perPlayer)
		{
			flow.activeDir.clear();
		}
		return;
	}
	if (wf.seenMap != cmap || wf.seenChecksum != cmap->checksum)
	{
		wf.seenMap = cmap;
		wf.seenChecksum = cmap->checksum;
		// The map every corridor decision is made against, into the sync stream once per map.
		// Emitted here so it lands on a tick that every client shares.
		syncDebug("corridorMap = %08X, %zu corridors", cmap->checksum, cmap->corridors.size());
		// Joint flow measured against another corridor map does not apply to this one, so it is
		// reset - but flow the map's own checksum vouches for stays, which is what lets a carry a
		// load restored survive to the first update, classifying droids at a joint exactly as the
		// saved run's next tick would have.
		for (CorridorFlowState &flow : wf.perPlayer)
		{
			if (flow.prevFlowChecksum != cmap->checksum || flow.prevFlow.size() != cmap->corridors.size())
			{
				flow.prevFlow.assign(cmap->corridors.size(), 0);
				flow.prevFlowChecksum = cmap->checksum;
			}
		}
		++g_queryStamp;   // the joints changed, so no earlier answer holds
		// droid ids recycle across games, so the transition log dedup restarts with the map
		g_logClass.clear();
		g_logSpeed.clear();
	}

	// Each player's flow is measured over its own and allied droids, the cohort the congestion
	// overlay stamps by, so the two mechanisms never disagree about who counts as us and
	// coordination never reaches across enemy lines. Players whose allied sets match see the
	// same droids, so their census is computed once, on the lowest such player, and shared -
	// an n-player team would otherwise classify every droid n times over.
	std::array<int, MAX_PLAYERS> canonical;
	std::array<uint32_t, MAX_PLAYERS> allyMask;
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		canonical[p] = -1;
		uint32_t mask = 0;
		for (unsigned q = 0; q < MAX_PLAYERS; ++q)
		{
			if (q == p || aiCheckAlliances(p, q))
			{
				mask |= 1u << q;
			}
		}
		allyMask[p] = mask;
		if (world.objects.droids[p].empty())
		{
			continue;   // judgment is per owner, so a droidless player's state has no reader
		}
		canonical[p] = static_cast<int>(p);
		for (unsigned q = 0; q < p; ++q)
		{
			if (canonical[q] == static_cast<int>(q) && allyMask[q] == mask)
			{
				canonical[p] = static_cast<int>(q);
				break;
			}
		}
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		CorridorFlowState &flow = wf.perPlayer[p];
		if (canonical[p] == static_cast<int>(p))
		{
			corridorFlowCompute(world, flow, p, canonical);
		}
		else if (canonical[p] >= 0)
		{
			flow = wf.perPlayer[static_cast<size_t>(canonical[p])];
		}
		else
		{
			// No droids means no flow. Clear the per-tick state and quiet the carried
			// flow, as a computed empty census would.
			flow.activeDir.clear();
			flow.contested.clear();
			flow.slotAlong.clear();
			if (flow.prevFlow.size() == cmap->corridors.size())
			{
				std::fill(flow.prevFlow.begin(), flow.prevFlow.end(), 0);
			}
		}
	}
	++g_queryStamp;   // the joint flow changed, so no earlier answer holds
}

/// The flow state a droid is judged against: its owner's.
static const CorridorFlowState &flowFor(const GameWorld &world, const DROID *psDroid)
{
	return world.corridorFlow.perPlayer[psDroid->player];
}

bool corridorLaneTarget(const GameWorld &world, const DROID *psDroid, Vector2i &laneTarget)
{
	const Vector2i pos = psDroid->pos.xy();
	const CorridorFlowState &flow = flowFor(world, psDroid);
	// A droid too far out for the centerline search still gets funnelled in from its
	// approach so it is drawn onto its own lane side before the mouth, leaving the
	// entrance clear rather than blobbing across it. The ones that must wait are held
	// by the flow gate, so steering them costs nothing.
	const Query q = classifyDroid(world, flow, psDroid);
	if (q.relation != INSIDE && q.relation != APPROACHING)
	{
		return false;
	}
	const size_t cid = static_cast<size_t>(q.corridorId);
	const Corridor &c = world.map.corridors->corridors[cid];
	const CorridorChains &chains = world.map.corridors->chains;
	const int last = static_cast<int>(c.centerline.size()) - 1;

	// With no opposing flow the passage is not split, so a droid is left to steer
	// through it naturally and the whole width fills on its own. Lanes are only
	// imposed to keep two opposing flows apart.
	if (cid >= flow.contested.size() || !flow.contested[cid])
	{
		return false;
	}

	// Near the exit the lane would pull a droid onto the mouth point as a blob when
	// its real path continues out into open space. Release it there so it heads for
	// its own target and leaves cleanly. Only an inside droid can be at its exit.
	if (q.relation == INSIDE)
	{
		const int distToExit = q.dir > 0 ? last - static_cast<int>(q.nearest) : static_cast<int>(q.nearest);
		// The release assumes the droid is about to head out into open space.
		// At an inner mouth the route continues into the next passage of the
		// chain, and releasing would lose the side it was sorted onto upstream
		// exactly where the two directions meet.
		const bool exitOuter = q.dir > 0 ? chains.mouthBOuter[cid] != 0 : chains.mouthAOuter[cid] != 0;
		if (distToExit <= MOUTH_MARGIN && (exitOuter || !pathfindingBendHandEnabled()))
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
	// the axis right. Each direction keeps to its own side of the centerline,
	// which side being the corridor's handedness for this episode.
	const int sideSign = q.dir * (cid < flow.handed.size() ? flow.handed[cid] : 1);
	int32_t bandMin, bandMax;
	if (sideSign > 0)
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
	const int lanes = laneCount(c, sideSign, psDroid);
	const int subLane = static_cast<int>(psDroid->id % static_cast<uint32_t>(lanes));
	const int32_t bandWidth = bandMax - bandMin;
	const int32_t lateral = bandMin + (2 * subLane + 1) * bandWidth / (2 * lanes);

	if (q.relation == APPROACHING)
	{
		// The funnel is an open-field mechanism. At an inner junction there is
		// no room for its flared aim, which fights the previous corridor's lane
		// and the droid's own route in the handoff, arcing the droid in circles
		// as the nearest-corridor classification alternates. The junction is
		// mostly driven on the route alone, with one exception: a droid whose
		// arrival side is the opposing lane would otherwise enter unsorted and
		// fight the oncoming stream inside a neck for a crossing it can make
		// here in the pocket, where there is room. When the opposing flow's
		// route coverage stays clear of the entry zone, aim it through the
		// mouth on its own lane side, a single sorted entry point, none of the
		// funnel's flare or slot machinery.
		const bool entryOuter = q.dir > 0 ? chains.mouthAOuter[cid] != 0 : chains.mouthBOuter[cid] != 0;
		if (!entryOuter)
		{
			// Crossing an inner junction a droid has no centerline under it,
			// but it still has a side to keep: the one it is about to occupy.
			// Steer at that lane line extended back out of the mouth. No flare
			// or queue slots - there is no queue to form here, only a side to
			// hold - and only where the joint has ground to span: at a butt
			// joint aiming a departing droid at the next lane fights its turn.
			if (pathfindingBendHoldEnabled()
			    && jointHolds(flow, cid, q.dir <= 0, mouthNearestJoint(chains, cid, q.dir <= 0, pos)))
			{
				const ApproachFrame fr = approachFrame(c, q.dir, pos);
				// Lead only a tile along, not the funnel's two: a long lead
				// points across the junction and drags the droid over the
				// ground the opposing stream is leaving by. Never behind the
				// droid, or it doubles back instead of advancing.
				const int32_t holdAlong = std::min(fr.along + JUNCTION_LEAD, APPROACH_LEAD);
				laneTarget.x = fr.mouth.x + static_cast<int32_t>((static_cast<int64_t>(fr.pVec.x) * lateral
				                                                  + static_cast<int64_t>(fr.gVec.x) * holdAlong) / DIR_UNIT);
				laneTarget.y = fr.mouth.y + static_cast<int32_t>((static_cast<int64_t>(fr.pVec.y) * lateral
				                                                  + static_cast<int64_t>(fr.gVec.y) * holdAlong) / DIR_UNIT);
				return true;
			}
			// Sorting is only worth the steering when there is a stream to be
			// sorted against: opposing droids physically inside this passage
			// whose route coverage stays clear of the entry zone. With no
			// oncoming traffic in the passage there is nothing here to avoid,
			// and reshaping a whole entering column's natural surge for
			// traffic that is still far away is the same perturbation-for-
			// nothing the lane spans kept paying.
			const int oppInside = (q.dir > 0 ? flow.cInsideBwd : flow.cInsideFwd)[cid];
			const int oppLo = (q.dir > 0 ? flow.useLoBwd : flow.useLoFwd)[cid];
			const int oppHi = (q.dir > 0 ? flow.useHiBwd : flow.useHiFwd)[cid];
			const bool zoneClear = q.dir > 0 ? oppLo > MOUTH_CROSS_ZONE + SPAN_BUFFER
			                                 : oppHi < last - MOUTH_CROSS_ZONE - SPAN_BUFFER;
			if (oppInside == 0 || !zoneClear)
			{
				return false;
			}
			const ApproachFrame f = approachFrame(c, q.dir, pos);
			// Only a droid arriving clearly on the opposing side needs the
			// sorted entry. One already on its own side keeps its route-driven
			// entry, steering it anyway would perturb for no benefit.
			const Vector2i rel = pos - f.mouth;
			const int32_t arriveLat = static_cast<int32_t>((static_cast<int64_t>(rel.x) * f.pVec.x
			                                                + static_cast<int64_t>(rel.y) * f.pVec.y) / DIR_UNIT);
			if (sideSign > 0 ? arriveLat >= -LANE_MARGIN : arriveLat <= LANE_MARGIN)
			{
				return false;
			}
			laneTarget.x = f.mouth.x + static_cast<int32_t>((static_cast<int64_t>(f.pVec.x) * lateral
			                                                 + static_cast<int64_t>(f.gVec.x) * APPROACH_LEAD) / DIR_UNIT);
			laneTarget.y = f.mouth.y + static_cast<int32_t>((static_cast<int64_t>(f.pVec.y) * lateral
			                                                 + static_cast<int64_t>(f.gVec.y) * APPROACH_LEAD) / DIR_UNIT);
			return true;
		}
		// Drive to this droid's queue slot on the lane extended out past the
		// mouth, so a blob approaches as files that fit the passage, each droid
		// falling in behind the one ahead. Far out the aim is flared wider onto
		// its own side, tapering away by FUNNEL_MERGE before the mouth, so the
		// approaches sort onto their sides while the middle stays clear for the
		// flow coming out.
		const ApproachFrame f = approachFrame(c, q.dir, pos);
		const auto slot = flow.slotAlong.find(psDroid->id);
		const int32_t slotAlong = slot != flow.slotAlong.end() ? slot->second : f.along;
		// Lead the aim past the slot so the droid keeps driving, through the mouth
		// itself for the front of the queue. Never aim behind the droid, so it
		// advances or waits, not doubles back. The hold stops it at its slot.
		const int32_t aimAlong = std::clamp(std::max(slotAlong, f.along) + APPROACH_LEAD,
		                                    -APPROACH_RADIUS, APPROACH_LEAD);
		const int32_t flare = aimAlong < -FUNNEL_MERGE
		                      ? FUNNEL_FLARE * (-aimAlong - FUNNEL_MERGE) / (APPROACH_RADIUS - FUNNEL_MERGE)
		                      : 0;
		int32_t aimLat;
		if (pathfindingWideQueueEnabled() && queueOwnsApproach(chains, cid, q.dir))
		{
			// Spread the files across the flared width instead of sliding one
			// file sideways, so a crowd queues abreast and converges as the
			// flare tapers. Same lane count the queue slots were built from, or
			// a droid would aim at one file and be spaced for another.
			const int32_t wideLo = sideSign > 0 ? bandMin : bandMin - flare;
			const int32_t wideHi = sideSign > 0 ? bandMax + flare : bandMax;
			const int aLanes = approachLaneCount(c, sideSign, psDroid);
			const int aSub = static_cast<int>(psDroid->id % static_cast<uint32_t>(aLanes));
			aimLat = wideLo + (2 * aSub + 1) * (wideHi - wideLo) / (2 * aLanes);
		}
		else
		{
			aimLat = lateral + sideSign * flare;
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
int queueSpeedInner(const GameWorld &world, const DROID *psDroid, int moveSpeed, const Query &q)
{
	const CorridorFlowState &flow = flowFor(world, psDroid);
	if (q.relation != APPROACHING || q.corridorId < 0)
	{
		return moveSpeed;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= flow.contested.size() || !flow.contested[c])
	{
		return moveSpeed;   // no opposing flow, so nothing to wait for
	}
	const auto slot = flow.slotAlong.find(psDroid->id);
	if (slot == flow.slotAlong.end())
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
	const Corridor &cor = world.map.corridors->corridors[c];
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

int corridorQueueSpeed(const GameWorld &world, const DROID *psDroid, int moveSpeed)
{
	const CorridorFlowState &flow = flowFor(world, psDroid);
	// The same classification the lane steering uses, so the whole approach
	// queues along the funnel line rather than scrumming near the mouth.
	const Query q = classifyDroid(world, flow, psDroid);
	const int out = queueSpeedInner(world, psDroid, moveSpeed, q);
	if (enabled_debug[LOG_MOVEMENT] && moveSpeed > 0)
	{
		// 0 held, 1 ramping, 2 free, logged on change
		const int state = out == 0 ? 0 : (out < moveSpeed ? 1 : 2);
		auto it = g_logSpeed.find(psDroid->id);
		if (it == g_logSpeed.end() || it->second != state)
		{
			g_logSpeed[psDroid->id] = state;
			const auto slot = flow.slotAlong.find(psDroid->id);
			const size_t c = q.corridorId >= 0 ? static_cast<size_t>(q.corridorId) : SIZE_MAX;
			debug(LOG_MOVEMENT, "corridor t=%u droid %u tile %d,%d speed %s c=%d slot=%d active=%d",
			      gameTime, psDroid->id,
			      map_coord(psDroid->pos.x), map_coord(psDroid->pos.y),
			      state == 0 ? "held" : (state == 1 ? "ramp" : "free"),
			      q.corridorId,
			      slot != flow.slotAlong.end() ? slot->second : INT32_MIN,
			      c < flow.activeDir.size() ? flow.activeDir[c] : 0);
		}
	}
	return out;
}

void corridorClampSlide(const GameWorld &world, const DROID *psDroid, int32_t *pdx, int32_t *pdy)
{
	const CorridorFlowState &flow = flowFor(world, psDroid);
	const Query q = queryDroid(world, flow, psDroid);
	if (q.relation != INSIDE)
	{
		return;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= flow.contested.size() || !flow.contested[c])
	{
		return;
	}
	// Exiting through an inner junction a droid legitimately crosses the
	// centerline, swinging into the next passage of its chain, so release the
	// clamp there or it cuts exactly the crossing motion the droid needs and
	// pins it at the junction. At an outer mouth the clamp stays, it is what
	// keeps the opposing flows apart right where they meet.
	const Corridor &cor = world.map.corridors->corridors[c];
	const CorridorChains &chains = world.map.corridors->chains;
	const int last = static_cast<int>(cor.centerline.size()) - 1;
	const int distToExit = q.dir > 0 ? last - static_cast<int>(q.nearest) : static_cast<int>(q.nearest);
	const bool exitOuter = q.dir > 0 ? chains.mouthBOuter[c] != 0 : chains.mouthAOuter[c] != 0;
	if (distToExit <= MOUTH_MARGIN && !exitOuter)
	{
		return;
	}
	// The droid's lateral position, positive into its own half of the corridor,
	// which half being the corridor's handedness for this episode. Well onto its
	// own side, slides resolve collisions within the band as usual.
	const int sideSign = q.dir * (c < flow.handed.size() ? flow.handed[c] : 1);
	const Vector2i rel = psDroid->pos.xy() - cor.centerline[q.nearest];
	const int64_t latPos = sideSign * (static_cast<int64_t>(rel.y) * q.axis.x - static_cast<int64_t>(rel.x) * q.axis.y);
	if (latPos > 0)
	{
		return;
	}
	// At or past the centerline, so cut any further push toward the opposing
	// side and keep only the motion along the corridor. Movement back onto its
	// own side stays free, so a droid bumped across works its way back.
	const int64_t latMove = sideSign * (static_cast<int64_t>(*pdy) * q.axis.x - static_cast<int64_t>(*pdx) * q.axis.y);
	if (latMove >= 0)
	{
		return;
	}
	const int64_t axisSq = static_cast<int64_t>(q.axis.x) * q.axis.x + static_cast<int64_t>(q.axis.y) * q.axis.y;
	*pdx -= static_cast<int32_t>(static_cast<int64_t>(sideSign) * -q.axis.y * latMove / axisSq);
	*pdy -= static_cast<int32_t>(static_cast<int64_t>(sideSign) * q.axis.x * latMove / axisSq);
}

// An approacher waiting into an inner junction is left in peace this long, then
// the watchdog resumes. Waiting there is usually right, the pocket is briefly
// full of merging or exiting traffic and churning through reroutes only adds
// shoving, but nothing of ours is holding the droid and nothing guarantees its
// spot ever clears, ex. pinned against a corner by a crossing exit stream. Long
// enough to sit out a normal merge or drain, short enough to recover a pinned
// droid while the traffic that pinned it still flows.
const uint32_t JUNCTION_GRACE = 15000;

// A queued droid waits far longer before the watchdog resumes. Queues are the
// layer's own doing and a draining one deserves patience, but a queue can also
// stop draining for good, ex. the queued column standing across an opposing
// flow's only crossing, each side holding the other forever. An advancing
// queue never ages the clock, the slot-by-slot creep resets it through the
// moved-on check, so only a stagnant wait runs out the grace and reroutes.
const uint32_t QUEUE_GRACE = 40000;

// With flow costs in the planner a reroute is productive rather than churn:
// a long-queued droid's replan sees the corridor full of opposing traffic
// and either diverts or confirms the wait, so the queue re-evaluates on a
// much shorter clock than the plain grace.
const uint32_t QUEUE_REEVAL = 15000;

std::optional<CorridorGateCarry> corridorGateCarry(const GameWorld &world, unsigned player)
{
	if (player >= MAX_PLAYERS)
	{
		return std::nullopt;
	}
	// prevFlow IS the carry: written by this world's updates and by a restored save alike, so an
	// inactive world saved again before its next active tick still round-trips the flow it held.
	const CorridorFlowState &flow = world.corridorFlow.perPlayer[player];
	const CorridorMap *cmap = world.map.corridors.get();
	if (cmap == nullptr || flow.prevFlowChecksum != cmap->checksum
	    || flow.prevFlow.size() != cmap->corridors.size())
	{
		return std::nullopt;   // no map, or nothing measured against this map, so nothing is carried
	}
	CorridorGateCarry carry;
	carry.mapChecksum = cmap->checksum;
	carry.flow = flow.prevFlow;
	return carry;
}

void corridorGateRestoreCarry(GameWorld &world, unsigned player, const CorridorGateCarry &carry)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "corridor carry for player %u", player);
	const CorridorMap *cmap = world.map.corridors.get();
	if (cmap == nullptr || carry.mapChecksum != cmap->checksum
	    || carry.flow.size() != cmap->corridors.size())
	{
		// The flow is indexed by corridor id, so a carry from a different map would apply to the
		// wrong passages. Dropping it degrades to rebuilding the flow on the next update.
		debug(LOG_MOVEMENT, "corridor carry discarded: p%u saved map %08X over %zu corridors, loaded %08X over %zu",
		      player, carry.mapChecksum, carry.flow.size(),
		      cmap != nullptr ? cmap->checksum : 0, cmap != nullptr ? cmap->corridors.size() : 0);
		return;
	}
	world.corridorFlow.perPlayer[player].prevFlow = carry.flow;
	world.corridorFlow.perPlayer[player].prevFlowChecksum = carry.mapChecksum;
	++g_queryStamp;   // the joint flow changed, so no earlier answer holds
}

bool corridorContested(const GameWorld &world, unsigned player, int corridorId)
{
	if (player >= MAX_PLAYERS)
	{
		return false;
	}
	const CorridorFlowState &flow = world.corridorFlow.perPlayer[player];
	return corridorId >= 0 && static_cast<size_t>(corridorId) < flow.contested.size()
	       && flow.contested[static_cast<size_t>(corridorId)] != 0;
}

static CorridorHold corridorHoldUncached(const GameWorld &world, const DROID *psDroid)
{
	// Only a droid approaching a queue-forming mouth is stopped by the layer
	// itself, the speed hold walks it slot by slot, so the watchdog must not
	// read that stop as being wedged while the queue drains. At an inner
	// junction no queue forms, so the watch merely pauses, and a droid still
	// pinned when the grace runs out gets the normal reroute. A droid inside
	// that stops long enough to trip the watchdog is genuinely wedged, and the
	// watchdog's reroute is what recovers it, so it keeps its normal watch.
	const CorridorFlowState &flow = flowFor(world, psDroid);
	const Query q = classifyDroid(world, flow, psDroid);
	if (q.relation != APPROACHING)
	{
		return CORRIDOR_HOLD_NONE;
	}
	const size_t c = static_cast<size_t>(q.corridorId);
	if (c >= flow.contested.size() || !flow.contested[c])
	{
		return CORRIDOR_HOLD_NONE;
	}
	const CorridorChains &chains = world.map.corridors->chains;
	const bool entryOuter = q.dir > 0 ? chains.mouthAOuter[c] != 0 : chains.mouthBOuter[c] != 0;
	const uint32_t queueGrace = pathfindingFlowCostEnabled() ? QUEUE_REEVAL : QUEUE_GRACE;
	const uint32_t grace = entryOuter ? queueGrace : JUNCTION_GRACE;
	if (psDroid->sMove.bumpTime != 0 && psDroid->sMove.bumpTime <= gameTime
	    && gameTime - psDroid->sMove.bumpTime > grace)
	{
		return CORRIDOR_HOLD_NONE;
	}
	return entryOuter ? CORRIDOR_HOLD_QUEUED : CORRIDOR_HOLD_JUNCTION;
}

CorridorHold corridorHold(const GameWorld &world, const DROID *psDroid)
{
	// Everything it depends on is in the key, so a hit returns exactly what a fresh call would
	// (since there may be many duplicate calls of this per-droid, per-tick).
	MOVE_CONTROL::HoldMemo &memo = const_cast<DROID *>(psDroid)->sMove.holdMemo;
	const uint8_t flags = static_cast<uint8_t>((psDroid->sMove.Status == MOVEINACTIVE ? 1 : 0)
	                                           | (psDroid->action == DACTION_MOVEFIRE ? 2 : 0));
	if (memo.stamp == g_queryStamp && memo.gameTime == gameTime
	    && memo.routeGeneration == psDroid->sMove.routeGeneration
	    && memo.bumpTime == psDroid->sMove.bumpTime && memo.pos == psDroid->pos.xy()
	    && memo.target == psDroid->sMove.target && memo.pathIndex == psDroid->sMove.pathIndex
	    && memo.flags == flags)
	{
		return static_cast<CorridorHold>(memo.value);
	}
	const CorridorHold answer = corridorHoldUncached(world, psDroid);
	memo.stamp = g_queryStamp;
	memo.gameTime = gameTime;
	memo.routeGeneration = psDroid->sMove.routeGeneration;
	memo.bumpTime = psDroid->sMove.bumpTime;
	memo.pos = psDroid->pos.xy();
	memo.target = psDroid->sMove.target;
	memo.pathIndex = psDroid->sMove.pathIndex;
	memo.flags = flags;
	memo.value = static_cast<uint8_t>(answer);
	return answer;
}
