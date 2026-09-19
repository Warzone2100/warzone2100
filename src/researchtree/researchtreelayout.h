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
/** @file
 *  Research graph model and layout
 *
 *  Nothing here should depend on lib/widget - one model serves any number of views
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREELAYOUT_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREELAYOUT_H__

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/ivis_opengl/piepalette.h"
#include "researchtracks.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// Notes:
// - Node topology is identical under every source and only NodeState differs, so one model can serve a live game and a finished one.

struct ResearchTreeContext
{
	enum class Source : uint8_t
	{
		LivePlayerState,	// asPlayerResList (for a game in progress)
		GameLog,		// GameStoryLogger's research log (for a finished game)
	};

	Source source = Source::LivePlayerState;
	uint32_t player = 0;	// whose tree is drawn
	uint32_t viewer = 0;	// who is viewing it
	// Seats whose research is read together (as one set).
	// `player` is the seat the reading speaks for, which is the viewer's own where they hold
	// one, so a topic the team holds and the viewer does not can still be identified. For a
	// spectator, it is the first member instead.
	std::vector<uint32_t> aggregate;
	bool allowAssignment = false;	// starting research from the view needs a live game
};

// Which topics a context may show (campaign may lock / hide topics until discovered or revealed by script).
// Returns: Empty vector for no restrictions, otherwise a vector<bool> indexed by research index
std::vector<bool> researchVisibleTo(const ResearchTreeContext& context);

// Which topics a context may say anything about: their name, cost and effect.
// A campaign may show the *shape* of what the player has not found but may still not name it.
// Returns: Empty vector for no restrictions, otherwise a vector<bool> indexed by research index
std::vector<bool> researchKnownTo(const ResearchTreeContext& context);

enum class TeamViewScope : uint8_t
{
	None,				// nothing at all
	CurrentSubjectOnly,		// what they are researching right now
	Full				// the whole tree from their seat
};
TeamViewScope teamViewScopeFor(uint32_t viewer, uint32_t target);

struct ResearchPerspective
{
	// A seat reads one player's research. A team reads every team-member's at once, which only
	// says anything where the mode does not share research - where it does, a team's set is
	// any member's set.
	enum class Kind : uint8_t { Seat, Team };

	Kind kind = Kind::Seat;
	uint32_t player = 0; // the player, or the viewer when on the same team (used to identify what "you" have vs the rest of the team)
	std::vector<uint32_t> members; // team only
	WzString title;
	PIELIGHT color;
	uint8_t depth = 0; // a member of a team listed under it, for the switcher
};
std::vector<ResearchPerspective> researchPerspectivesFor(uint32_t viewer);

// How many of an aggregate context's seats have finished this topic
uint32_t researchHeldCount(const ResearchTreeContext& context, uint16_t researchIndex);

std::vector<uint32_t> researchHeldBy(const ResearchTreeContext& context, uint16_t researchIndex);

// Teammates with a lab working on this topic right now, in the order the research list reports them.
// In "shared research" modes only. (In unshared research, everyone needs their own copy of everything.)
std::vector<uint32_t> alliesResearching(const ResearchTreeContext& context, uint16_t researchIndex);

// Topics more than one lab on the team has loaded at once (counting the viewer's own).
// Shared research only, for the same reason as alliesResearching().
std::vector<uint16_t> duplicatedResearch(const ResearchTreeContext& context);

enum class NodeKind : uint8_t
{
	Topic,		// a single research topic
	Track,		// a collapsed progression of topics
};

// Color bands, one per tech category
static constexpr uint8_t LANE_COUNT = 9;
uint8_t laneOfResearchIcon(UDWORD iconID);
WzString researchLaneName(uint8_t lane);
// The bands for anything displayed, in band order. A campaign part way through may
// put nothing in most of them, and a mod need not use them all.
// Empty means everything is displayed.
std::vector<uint8_t> researchLanesInUse(const std::vector<bool>& visible);

enum class NodeState : uint8_t
{
	Researched,
	InProgress,	// a facility is working on it (counting pending netcode state)
	Available,	// can be started right now
	Reachable,	// every prerequisite is itself researched or reachable
	Locked,		// prerequisites exist but the chain is blocked
	Disabled,	// switched off for this game (ex. VTOL topics in a no-VTOL match)
};

struct GraphNode
{
	NodeKind kind = NodeKind::Topic;
	uint16_t primaryResearchIndex = 0;	// index into asResearch
	std::vector<uint16_t> members;		// Track: the chain in progression order. Topic: {primary}
	uint16_t completedMembers = 0;		// how much of a Track is done, for the progress pips
	uint16_t iconID = 0;
	uint16_t subGroupIconID = 0;
	uint8_t laneIndex = 0;			// tech-category band, 0 .. LANE_COUNT - 1
	NodeState state = NodeState::Locked;
};

struct GraphEdge
{
	uint32_t from = 0;		// GraphNode indices
	uint32_t to = 0;
	uint8_t colorLane = 0;		// the source node's laneIndex
};

// Node/edge model over asResearch, plus the adjacency
class ResearchGraph
{
public:
	static ResearchGraph build(const ResearchTreeContext& context);

	// Recompute node states without touching topology or layout
	void refreshState(const ResearchTreeContext& context);

	const std::vector<GraphNode>& nodes() const { return m_nodes; }
	const std::vector<GraphEdge>& edges() const { return m_edges; }

	// Node indices, not research indices
	const std::vector<uint32_t>& dependentsOf(uint32_t node) const { return m_forward[node]; }
	const std::vector<uint32_t>& prerequisitesOf(uint32_t node) const { return m_reverse[node]; }

	optional<uint32_t> nodeForResearchIndex(uint16_t researchIndex) const;

	// Distinct tech-category bands present in this dataset
	size_t laneCount() const { return m_laneCount; }

	// Topics with no prerequisites, and topics nothing depends on
	std::vector<uint32_t> roots() const;
	std::vector<uint32_t> leaves() const;

private:
	std::vector<GraphNode> m_nodes;
	std::vector<GraphEdge> m_edges;
	std::vector<std::vector<uint32_t>> m_forward;	// node -> nodes that depend on it
	std::vector<std::vector<uint32_t>> m_reverse;	// node -> nodes it depends on
	std::unordered_map<uint16_t, uint32_t> m_researchToNode;
	size_t m_laneCount = 0;
};

// One object in the drawn tree, either a single topic or a whole progression
//
// Every step keeps the tier it reached, so a progression occupies the range its steps
// span rather than sitting at one tier, and an edge pointing forwards over topics
// still points forwards over units.
//
// Two steps can land in one tier (ex. a progression ordered by what obsoletes what,
// being built from prerequisite siblings) - so a unit is as many rows tall as its most
// crowded tier, and each step keeps a row of its own.
struct LayoutUnit
{
	std::vector<uint16_t> members;		// research indices, by tier, which is the order drawn
	std::vector<int32_t> memberTiers;	// each member's own tier, parallel to members
	std::vector<int32_t> memberRows;	// each member's row within the unit, from 0
	WzString name;				// the progression's, empty when it has none
	uint8_t laneIndex = 0;
	int32_t firstTier = 0;			// lowest and highest member tier
	int32_t lastTier = 0;
	int32_t row = 0;			// the topmost row it occupies
	int32_t rowSpan = 1;
	// Dependencies between two of this unit's own steps, as member indices.
	// The layout itself has nothing to place for these, both ends being inside one object,
	// but they are most of what a progression means (and should be drawn or otherwise indicated).
	std::vector<std::pair<uint16_t, uint16_t>> internalEdges;
	// The last tier of the nearest unit sharing a row and ending before this one, or -1
	// where nothing does. What is written beside a unit has this much room and no more.
	int32_t leftNeighborTier = -1;
	// Several progressions that need each other, merged into one object.
	// Their steps are in no order, so nothing here is a step count.
	bool merged = false;
};

// A dependency between two units, leaving and arriving at the tier of the member at
// each end rather than the unit as a whole. `toTier` is always greater than `fromTier`.
// Those crossing more than one tier are drawn through dummies - see segments().
struct LayoutEdge
{
	uint32_t from = 0;		// unit indices
	uint32_t to = 0;
	int32_t fromTier = 0;
	int32_t toTier = 0;
	int32_t fromRow = 0;		// the row within each unit that the step sits in
	int32_t toRow = 0;
	uint8_t colorLane = 0;
	// Whether both ends survived the layout's filter.
	// A unit is kept whole when any one of its steps is wanted, so under a focus
	// an edge can hang off a step the target does not rest on.
	bool bothWanted = true;
};

// A single-tier hop between layout points. Points below unitCount() are units, the
// rest are dummies standing in for a long edge crossing a tier.
// `tier` is the one the hop leaves, so it arrives at `tier + 1`.
struct LayoutSegment
{
	uint32_t from = 0;
	uint32_t to = 0;
	int32_t tier = 0;
	uint32_t edge = 0;	// which LayoutEdge this is part of
};

// Sizes the drawn tree - in logical pixels before any zoom
struct LayoutMetrics
{
	int32_t unitWidth = 168;	// of the part of a unit sitting in one tier
	int32_t unitHeight = 30;
	int32_t tierSpacing = 240;	// between the left edges of neighboring tiers
	int32_t rowPadding = 14;	// clear space between neighboring rows
	// Room before the first tier, so that a unit starting there still has somewhere to put what names it
	int32_t leftGutter = 0;
};

// Wide enough for an icon and a name beside it
static constexpr int32_t LABELED_LEFT_GUTTER = 176;

// Tiers every topic, then packs what spans those tiers into rows
class ResearchTreeLayout
{
public:
	enum class TrackMode : uint8_t
	{
		None,		// one unit per topic
		Collapse,	// one unit per progression, plus one per topic outside any
	};

	enum class Ordering : uint8_t
	{
		None,			// pack in the order things were created
		Crossings,		// fewest edge crossings (wherever that puts things)
		LaneGrouped,		// keep a tech category together, then fewest crossings
	};

	// `includedResearch`, when given, is indexed by research index and excludes every topic it does not name.
	// A progression is kept if any of its steps is wanted.
	static ResearchTreeLayout build(const ResearchGraph& graph, const std::vector<ResearchTrack>& tracks, TrackMode mode, Ordering ordering = Ordering::LaneGrouped, const LayoutMetrics& metrics = LayoutMetrics(), const std::vector<bool> *includedResearch = nullptr, const std::vector<bool> *visibleResearch = nullptr);

	// The ends of one hop, in logical pixels. Both are needed together, since a
	// unit end sits at the tier and row of the step the edge belongs to, rather
	// than anywhere on the unit.
	Vector2i segmentFrom(uint32_t segment) const;
	Vector2i segmentTo(uint32_t segment) const;

	// The box a unit occupies, in logical pixels
	Vector2i unitTopLeft(uint32_t unit) const;
	Vector2i unitSize(uint32_t unit) const;
	Vector2i unitCenter(uint32_t unit) const;

	const LayoutMetrics& metrics() const { return m_metrics; }

	// Re-space the same tiers and rows. Which tier and which row anything is in is
	// an integer grid, and the metrics are only its pitch, so density is a *drawing*
	// decision rather than a layout one.
	void setMetrics(const LayoutMetrics& metrics);

	// Smallest box holding every unit
	Vector2i canvasSize() const { return m_canvasSize; }

	// Clear space to the left of a unit before whatever shares its row, or before
	// the canvas where nothing does, in logical pixels
	int32_t clearLeftOf(uint32_t unit) const;

	// The middle of one of a unit's rows, as an offset down from its top. The rows
	// of a unit are spread evenly through it rather than left on the layout's grid
	// pitch, since that pitch carries the clearance between neighboring units and
	// has no business being the gap between one unit's own steps.
	int32_t unitRowOffset(uint32_t unit, int32_t rowWithinUnit) const;

	uint8_t laneOf(uint32_t point) const;
	int32_t rowOf(uint32_t point) const;

	// How many times two edges cross
	size_t crossingCount() const;

	// How far, in rows, every edge has to run up or down between its ends
	int64_t totalEdgeTravel() const;

	const std::vector<LayoutUnit>& units() const { return m_units; }
	const std::vector<LayoutEdge>& edges() const { return m_edges; }
	const std::vector<LayoutSegment>& segments() const { return m_segments; }

	size_t unitCount() const { return m_units.size(); }
	size_t dummyCount() const { return m_dummyTiers.size(); }

	size_t tierCount() const { return m_tierOccupancy.size(); }
	size_t rowCount() const { return m_rowTop.size(); }
	// Units covering each tier, and those plus the dummies passing through it
	const std::vector<size_t>& tierOccupancy() const { return m_tierOccupancy; }
	const std::vector<size_t>& tierSlots() const { return m_tierSlots; }

private:
	// A long edge crosses the tiers between its ends as one straight run, so every dummy standing in for it shares a row
	struct DummyChain
	{
		uint32_t edge = 0;
		uint32_t firstDummy = 0;
		uint32_t dummyCount = 0;
		int32_t firstTier = 0;
		int32_t lastTier = 0;
	};

	std::vector<LayoutUnit> m_units;
	std::vector<LayoutEdge> m_edges;
	std::vector<LayoutSegment> m_segments;
	std::vector<DummyChain> m_chains;
	std::vector<int32_t> m_dummyTiers;
	std::vector<uint8_t> m_dummyLanes;
	std::vector<size_t> m_tierOccupancy;
	std::vector<size_t> m_tierSlots;
	std::vector<int32_t> m_pointRow;
	std::vector<int32_t> m_rowTop;
	std::vector<bool> m_rowHoldsUnit;
	LayoutMetrics m_metrics;
	Vector2i m_canvasSize = Vector2i(0, 0);

	int32_t rowMiddle(int32_t row) const;
	// The row an edge meets a point in, counted from the top of the layout - and where that comes out in logical pixels
	int32_t attachRow(uint32_t point, uint32_t edge, bool isStart) const;
	int32_t attachHeight(uint32_t point, uint32_t edge, bool isStart) const;
	void packIntoRows(Ordering ordering);
	void assignCoordinates();
};

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREELAYOUT_H__
