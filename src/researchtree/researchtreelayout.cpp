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
 *  Research graph model
 */

#include "researchtreelayout.h"

#include "../activity.h"

#include "../research.h"
#include "../researchdef.h"
#include "../gamehistorylogger.h"
#include "../campaigninfo.h"
#include "../ai.h"
#include "../multiplay.h"
#include "../component.h"
#include "researchassign.h"
#include "../modinfo.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

// ---------------------------------------------------------------------------
// MARK: - State sourcing
// ---------------------------------------------------------------------------

static uint32_t g_displayedPlayers = 0;
static_assert(MAX_PLAYERS <= 32, "one bit per player");

static inline void noteResearchDisplayedPlayer(uint32_t player)
{
	if (player < MAX_PLAYERS)
	{
		g_displayedPlayers |= (static_cast<uint32_t>(1) << player);
	}
}

uint32_t researchDisplayedPlayersMask()
{
	return g_displayedPlayers;
}

void clearResearchDisplayedPlayers()
{
	g_displayedPlayers = 0;
}

static std::unordered_set<uint16_t> completedFromGameLog(uint32_t player)
{
	std::unordered_set<uint16_t> completed;
	for (const auto& event : GameStoryLogger::instance().getResearchLog())
	{
		if (event.player < 0 || static_cast<uint32_t>(event.player) != player)
		{
			continue;
		}
		const RESEARCH *psResearch = getResearch(event.researchId.toUtf8().c_str());
		if (psResearch != nullptr)
		{
			completed.insert(static_cast<uint16_t>(psResearch->index));
		}
	}
	return completed;
}

static std::vector<bool> researchDiscoveredBy(const ResearchTreeContext& context)
{
	if (context.source != ResearchTreeContext::Source::LivePlayerState
	    || ActivityManager::instance().getCurrentGameMode() != ActivitySink::GameMode::CAMPAIGN
	    || context.player >= MAX_PLAYERS)
	{
		return std::vector<bool>();
	}
	std::vector<bool> discovered(asResearch.size(), false);
	for (size_t i = 0; i < asResearch.size(); ++i)
	{
		discovered[i] = researchDiscovered(i, context.player);
	}
	return discovered;
}

std::vector<bool> researchVisibleTo(const ResearchTreeContext& context)
{
	// A campaign declaring "greyed" wants its shape to be part of the game,
	// so everything is placed (even if details are not shown).
	if (getCampaignResearchTreeVisibility() != WzResearchTreeVisibility::Hidden)
	{
		return std::vector<bool>();
	}
	return researchDiscoveredBy(context);
}

std::vector<bool> researchKnownTo(const ResearchTreeContext& context)
{
	return researchDiscoveredBy(context);
}

std::vector<uint32_t> alliesResearching(const ResearchTreeContext& context, uint16_t researchIndex)
{
	std::vector<uint32_t> allies;
	if (context.source != ResearchTreeContext::Source::LivePlayerState
	    || context.player >= MAX_PLAYERS || context.player != selectedPlayer
	    || researchIndex >= asResearch.size() || !alliancesSharedResearch(game.alliance))
	{
		return allies;
	}
	for (const auto& ally : listAllyResearch(asResearch[researchIndex].ref))
	{
		if (ally.active)
		{
			allies.push_back(static_cast<uint32_t>(ally.player));
		}
	}
	return allies;
}

std::vector<uint16_t> duplicatedResearch(const ResearchTreeContext& context)
{
	std::vector<uint16_t> duplicated;
	if (context.source != ResearchTreeContext::Source::LivePlayerState
	    || context.player >= MAX_PLAYERS || context.player != selectedPlayer
	    || !alliancesSharedResearch(game.alliance))
	{
		return duplicated;
	}
	// The viewer's own labs, which the ally list doesn't include
	std::unordered_map<uint16_t, size_t> labsOn;
	for (const auto& lab : researchLabsFor(context.player))
	{
		if (!lab.idle && !lab.onHold)
		{
			labsOn[lab.subjectIndex]++;
		}
	}
	for (size_t i = 0; i < asResearch.size(); ++i)
	{
		const uint16_t researchIndex = static_cast<uint16_t>(i);
		size_t labs = alliesResearching(context, researchIndex).size();
		const auto own = labsOn.find(researchIndex);
		labs += (own != labsOn.end()) ? own->second : 0;
		if (labs > 1)
		{
			duplicated.push_back(researchIndex);
		}
	}
	return duplicated;
}

TeamViewScope teamViewScopeFor(uint32_t viewer, uint32_t target)
{
	if (target >= MAX_PLAYERS)
	{
		return TeamViewScope::None;
	}
	if (viewer >= MAX_PLAYERS || (bMultiPlayer && NetPlay.players[viewer].isSpectator))
	{
		return TeamViewScope::Full;
	}
	if (viewer == target)
	{
		return TeamViewScope::Full;
	}
	if (!bMultiPlayer || game.alliance == NO_ALLIANCES || !aiCheckAlliances(viewer, target))
	{
		return TeamViewScope::None;
	}
	return alliancesFixed(game.alliance) ? TeamViewScope::Full : TeamViewScope::CurrentSubjectOnly;
}

uint32_t researchDisplayAllowedMask(uint32_t viewer)
{
	uint32_t allowed = 0;
	for (uint32_t target = 0; target < MAX_PLAYERS; ++target)
	{
		if (teamViewScopeFor(viewer, target) == TeamViewScope::Full)
		{
			allowed |= (static_cast<uint32_t>(1) << target);
		}
	}
	return allowed;
}

struct VisibleSeat
{
	uint32_t player;
	int32_t team;
};

static std::vector<VisibleSeat> visibleSeatsFor(uint32_t viewer)
{
	std::vector<VisibleSeat> seats;
	for (uint32_t player = 0; player < MAX_PLAYERS; ++player)
	{
		if (player >= game.maxPlayers || (!isHumanPlayer(player) && NetPlay.players[player].ai < 0))
		{
			continue;
		}
		if (NetPlay.players[player].isSpectator || teamViewScopeFor(viewer, player) != TeamViewScope::Full)
		{
			continue;
		}
		seats.push_back({player, NetPlay.players[player].team});
	}
	return seats;
}

static ResearchPerspective seatPerspective(uint32_t player, uint8_t depth)
{
	ResearchPerspective seat;
	seat.player = player;
	seat.title = WzString::fromUtf8(getPlayerName(player));
	seat.color = pal_GetTeamColour(getPlayerColour(player));
	seat.depth = depth;
	return seat;
}

static ResearchPerspective teamPerspective(const std::vector<uint32_t>& members, uint32_t anchor,
                                           optional<size_t> letterIndex)
{
	ResearchPerspective team;
	team.kind = ResearchPerspective::Kind::Team;
	team.player = anchor;
	team.members = members;
	if (letterIndex.has_value())
	{
		const std::string letter(1, static_cast<char>('A' + (*letterIndex % 26)));
		team.title = WzString::fromUtf8(astringf(_("Team %s"), letter.c_str()));
	}
	else
	{
		team.title = WzString::fromUtf8(_("Team"));
	}
	team.color = WZCOL_TEXT_BRIGHT;
	return team;
}

std::vector<ResearchPerspective> researchPerspectivesFor(uint32_t viewer)
{
	const bool watching = viewer >= MAX_PLAYERS || (bMultiPlayer && NetPlay.players[viewer].isSpectator);
	const bool shared = bMultiPlayer && alliancesSharedResearch(game.alliance);
	const bool teamsAreFixed = bMultiPlayer && game.alliance != NO_ALLIANCES && alliancesFixed(game.alliance);

	const std::vector<VisibleSeat> visible = visibleSeatsFor(viewer);
	std::map<int32_t, std::vector<uint32_t>> byTeam;
	for (const auto& seat : visible)
	{
		byTeam[seat.team].push_back(seat.player);
	}

	std::vector<ResearchPerspective> offered;
	if (watching)
	{
		const bool offerTeams = teamsAreFixed && byTeam.size() > 1;
		const bool offerSeats = !teamsAreFixed || !shared;
		size_t letter = 0;
		for (const auto& team : byTeam)
		{
			if (offerTeams)
			{
				offered.push_back(teamPerspective(team.second, team.second.front(), letter));
			}
			if (offerSeats)
			{
				for (const auto player : team.second)
				{
					offered.push_back(seatPerspective(player, offerTeams ? 1 : 0));
				}
			}
			++letter;
		}
		return (offered.size() < 2) ? std::vector<ResearchPerspective>() : offered;
	}

	// A player reads their own team (where research is shared, there's only one perspective)
	std::vector<uint32_t> own;
	for (const auto& seat : visible)
	{
		if (seat.player == viewer || !shared)
		{
			own.push_back(seat.player);
		}
	}
	if (own.size() < 2)
	{
		return std::vector<ResearchPerspective>();
	}
	// The viewer leads their teammates
	const auto self = std::find(own.begin(), own.end(), viewer);
	if (self != own.end())
	{
		std::rotate(own.begin(), self, self + 1);
	}
	offered.push_back(teamPerspective(own, viewer, nullopt));
	for (const auto player : own)
	{
		offered.push_back(seatPerspective(player, 1));
	}
	return offered;
}

uint32_t researchHeldCount(const ResearchTreeContext& context, uint16_t researchIndex)
{
	uint32_t held = 0;
	for (const auto member : context.aggregate)
	{
		if (member >= MAX_PLAYERS || researchIndex >= asPlayerResList[member].size())
		{
			continue;
		}
		if (IsResearchCompleted(&asPlayerResList[member][researchIndex]))
		{
			++held;
		}
	}
	return held;
}

std::vector<uint32_t> researchHeldBy(const ResearchTreeContext& context, uint16_t researchIndex)
{
	std::vector<uint32_t> held;
	for (const auto member : context.aggregate)
	{
		if (member >= MAX_PLAYERS || researchIndex >= asPlayerResList[member].size())
		{
			continue;
		}
		if (IsResearchCompleted(&asPlayerResList[member][researchIndex]))
		{
			held.push_back(member);
		}
	}
	return held;
}

// The state of one topic over a whole team: held by somebody, held by nobody (but reachable), or not in this match at all
static NodeState aggregateStateOf(uint16_t researchIndex, const ResearchTreeContext& context)
{
	bool anyEnabled = false;
	size_t counted = 0;
	for (const auto member : context.aggregate)
	{
		if (member >= MAX_PLAYERS || researchIndex >= asPlayerResList[member].size())
		{
			continue;
		}
		noteResearchDisplayedPlayer(member);
		++counted;
		const PLAYER_RESEARCH *psPlayerRes = &asPlayerResList[member][researchIndex];
		if (IsResearchCompleted(psPlayerRes))
		{
			return NodeState::Researched;
		}
		anyEnabled = anyEnabled || !IsResearchDisabled(psPlayerRes);
	}
	if (counted > 0 && !anyEnabled)
	{
		return NodeState::Disabled;
	}
	// Nobody holds it - reachable rather than locked, so the topic still shows its category
	return NodeState::Reachable;
}

// The state of one topic, before Reachable is propagated over the graph
static NodeState directStateOf(uint16_t researchIndex, const ResearchTreeContext& context, const std::unordered_set<uint16_t>& logCompleted)
{
	if (context.source == ResearchTreeContext::Source::GameLog)
	{
		if (logCompleted.count(researchIndex) > 0)
		{
			return NodeState::Researched;
		}
		// Start-of-game research isn't in the log, so check the live list as well
		if (context.player < MAX_PLAYERS && researchIndex < asPlayerResList[context.player].size()
		    && IsResearchCompleted(&asPlayerResList[context.player][researchIndex]))
		{
			return NodeState::Researched;
		}
		// Nothing is in flight and nothing is left to start, so the rest is simply not researched
		return NodeState::Locked;
	}

	if (!context.aggregate.empty())
	{
		return aggregateStateOf(researchIndex, context);
	}

	ASSERT_OR_RETURN(NodeState::Locked, context.player < MAX_PLAYERS, "Invalid player %" PRIu32, context.player);
	noteResearchDisplayedPlayer(context.player);
	const auto& playerList = asPlayerResList[context.player];
	ASSERT_OR_RETURN(NodeState::Locked, researchIndex < playerList.size(), "Research index %u out of range", researchIndex);
	const PLAYER_RESEARCH *psPlayerRes = &playerList[researchIndex];

	if (IsResearchCompleted(psPlayerRes))
	{
		return NodeState::Researched;
	}
	if (IsResearchDisabled(psPlayerRes))
	{
		return NodeState::Disabled;
	}
	// Pending states - so the view reacts to a click before the netcode round-trip
	if (IsResearchStartedPending(psPlayerRes) && !IsResearchCancelledPending(psPlayerRes))
	{
		return NodeState::InProgress;
	}
	if (researchAvailable(researchIndex, context.player, ModeQueue))
	{
		return NodeState::Available;
	}
	return NodeState::Locked;
}

// ---------------------------------------------------------------------------
// MARK: - Lanes
// ---------------------------------------------------------------------------

// Fixed band per tech category
uint8_t laneOfResearchIcon(UDWORD iconID)
{
	switch (mapIconToRID(iconID))
	{
	case RID_DROIDTECH:
	case RID_HOVERCRAFT:
	case RID_TRACKS:		return 0;
	case RID_WEAPONTECH:
	case RID_ROCKET:
	case RID_CANNON:		return 1;
	case RID_COMPUTERTECH:		return 2;
	case RID_POWERTECH:		return 3;
	case RID_SYSTEMTECH:
	case RID_ECM:			return 4;
	case RID_STRUCTURETECH:		return 5;
	case RID_CYBORGTECH:		return 6;
	case RID_DEFENCE:
	case RID_PLASCRETE:		return 7;
	default:			return 8;
	}
}

WzString researchLaneName(uint8_t lane)
{
	switch (lane)
	{
	case 0:		return WzString::fromUtf8(_("Droids"));
	case 1:		return WzString::fromUtf8(_("Weapons"));
	case 2:		return WzString::fromUtf8(_("Computing"));
	case 3:		return WzString::fromUtf8(_("Power"));
	case 4:		return WzString::fromUtf8(_("Systems"));
	case 5:		return WzString::fromUtf8(_("Structures"));
	case 6:		return WzString::fromUtf8(_("Cyborgs"));
	case 7:		return WzString::fromUtf8(_("Defenses"));
	default:	return WzString::fromUtf8(_("Other"));
	}
}

std::vector<uint8_t> researchLanesInUse(const std::vector<bool>& visible)
{
	std::vector<bool> seen(LANE_COUNT, false);
	for (size_t index = 0; index < asResearch.size(); ++index)
	{
		if (!visible.empty() && (index >= visible.size() || !visible[index]))
		{
			continue;
		}
		const uint8_t lane = laneOfResearchIcon(asResearch[index].iconID);
		if (lane < LANE_COUNT)
		{
			seen[lane] = true;
		}
	}
	std::vector<uint8_t> lanes;
	for (uint8_t lane = 0; lane < LANE_COUNT; ++lane)
	{
		if (seen[lane]) { lanes.push_back(lane); }
	}
	return lanes;
}

// ---------------------------------------------------------------------------
// MARK: - Build
// ---------------------------------------------------------------------------

ResearchGraph ResearchGraph::build(const ResearchTreeContext& context)
{
	ResearchGraph graph;
	const size_t topicCount = asResearch.size();

	graph.m_nodes.reserve(topicCount);
	graph.m_researchToNode.reserve(topicCount);

	graph.m_laneCount = LANE_COUNT;

	// One node per topic.
	// The member list holds just that topic, a Track node covering a progression using the same field.
	for (size_t i = 0; i < topicCount; ++i)
	{
		const RESEARCH& research = asResearch[i];
		GraphNode node;
		node.kind = NodeKind::Topic;
		node.primaryResearchIndex = static_cast<uint16_t>(i);
		node.members.push_back(static_cast<uint16_t>(i));
		node.iconID = research.iconID;
		node.subGroupIconID = research.subGroup;
		node.laneIndex = laneOfResearchIcon(research.iconID);
		graph.m_researchToNode[static_cast<uint16_t>(i)] = static_cast<uint32_t>(graph.m_nodes.size());
		graph.m_nodes.push_back(std::move(node));
	}

	graph.m_forward.resize(graph.m_nodes.size());
	graph.m_reverse.resize(graph.m_nodes.size());

	// Both directions - the engine stores prerequisites only
	std::set<std::pair<uint32_t, uint32_t>> seen;
	for (size_t i = 0; i < topicCount; ++i)
	{
		const uint32_t to = graph.m_researchToNode[static_cast<uint16_t>(i)];
		for (const auto prereqIndex : asResearch[i].pPRList)
		{
			const auto it = graph.m_researchToNode.find(prereqIndex);
			if (it == graph.m_researchToNode.end())
			{
				continue;
			}
			const uint32_t from = it->second;
			if (from == to || !seen.insert({from, to}).second)
			{
				continue;	// a self-edge or a duplicate would corrupt the layering pass
			}

			GraphEdge edge;
			edge.from = from;
			edge.to = to;
			edge.colorLane = graph.m_nodes[from].laneIndex;
			graph.m_edges.push_back(edge);

			graph.m_forward[from].push_back(to);
			graph.m_reverse[to].push_back(from);
		}
	}

	graph.refreshState(context);
	return graph;
}

void ResearchGraph::refreshState(const ResearchTreeContext& context)
{
	std::unordered_set<uint16_t> logCompleted;
	if (context.source == ResearchTreeContext::Source::GameLog)
	{
		logCompleted = completedFromGameLog(context.player);
	}

	for (auto& node : m_nodes)
	{
		node.state = directStateOf(node.primaryResearchIndex, context, logCompleted);
		node.completedMembers = (node.state == NodeState::Researched) ? 1 : 0;
	}

	// Promote Locked to Reachable where every prerequisite is researched or reachable,
	// so the view separates "further along this line" from "dead end".
	// Iterated to a fixed point, since node order is not guaranteed to be topological.
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (size_t n = 0; n < m_nodes.size(); ++n)
		{
			if (m_nodes[n].state != NodeState::Locked)
			{
				continue;
			}
			bool allSatisfied = true;
			for (const auto prereq : m_reverse[n])
			{
				const NodeState prereqState = m_nodes[prereq].state;
				if (prereqState != NodeState::Researched && prereqState != NodeState::Reachable
				    && prereqState != NodeState::Available && prereqState != NodeState::InProgress)
				{
					allSatisfied = false;
					break;
				}
			}
			if (allSatisfied)
			{
				m_nodes[n].state = NodeState::Reachable;
				changed = true;
			}
		}
	}
}

optional<uint32_t> ResearchGraph::nodeForResearchIndex(uint16_t researchIndex) const
{
	const auto it = m_researchToNode.find(researchIndex);
	if (it == m_researchToNode.end())
	{
		return nullopt;
	}
	return it->second;
}

std::vector<uint32_t> ResearchGraph::roots() const
{
	std::vector<uint32_t> result;
	for (uint32_t n = 0; n < m_nodes.size(); ++n)
	{
		if (m_reverse[n].empty())
		{
			result.push_back(n);
		}
	}
	return result;
}

std::vector<uint32_t> ResearchGraph::leaves() const
{
	std::vector<uint32_t> result;
	for (uint32_t n = 0; n < m_nodes.size(); ++n)
	{
		if (m_forward[n].empty())
		{
			result.push_back(n);
		}
	}
	return result;
}

// ---------------------------------------------------------------------------
// MARK: - Layering
// ---------------------------------------------------------------------------

// Longest-path layering over a directed graph in topological-sortable form.
// Returns false and leaves tiers undefined on a cycle (which the caller must have ruled out).
static bool layerLongestPath(size_t pointCount, const std::vector<std::vector<uint32_t>>& outgoing, const std::vector<size_t>& inDegreeIn, std::vector<int32_t>& tierOut)
{
	tierOut.clear();
	if (pointCount == 0)
	{
		return true; // nothing to layer
	}

	std::vector<size_t> inDegree = inDegreeIn;
	std::vector<uint32_t> ready;
	tierOut.assign(pointCount, 0);
	for (uint32_t i = 0; i < pointCount; ++i)
	{
		if (inDegree[i] == 0)
		{
			ready.push_back(i);
		}
	}

	size_t settled = 0;
	while (!ready.empty())
	{
		const uint32_t at = ready.back();
		ready.pop_back();
		settled++;
		for (const auto next : outgoing[at])
		{
			tierOut[next] = std::max(tierOut[next], tierOut[at] + 1);
			if (--inDegree[next] == 0)
			{
				ready.push_back(next);
			}
		}
	}
	return settled == pointCount;
}

// ---------------------------------------------------------------------------
// MARK: - Geometry
// ---------------------------------------------------------------------------

int32_t ResearchTreeLayout::rowOf(uint32_t point) const
{
	return (point < m_pointRow.size()) ? m_pointRow[point] : 0;
}

uint8_t ResearchTreeLayout::laneOf(uint32_t point) const
{
	if (point < m_units.size())
	{
		return m_units[point].laneIndex;
	}
	const size_t dummy = point - m_units.size();
	return (dummy < m_dummyLanes.size()) ? m_dummyLanes[dummy] : 0;
}

// The vertical middle of a row.
// NOTE: A row holding no unit carries only edges passing through, so it has no height and its middle is its top.
int32_t ResearchTreeLayout::rowMiddle(int32_t row) const
{
	if (row < 0 || row >= static_cast<int32_t>(m_rowTop.size()))
	{
		return 0;
	}
	return m_rowTop[row] + (m_rowHoldsUnit[row] ? m_metrics.unitHeight / 2 : 0);
}

int32_t ResearchTreeLayout::attachRow(uint32_t point, uint32_t edge, bool isStart) const
{
	if (point >= m_units.size() || edge >= m_edges.size())
	{
		return rowOf(point);
	}
	return m_units[point].row + (isStart ? m_edges[edge].fromRow : m_edges[edge].toRow);
}

int32_t ResearchTreeLayout::clearLeftOf(uint32_t unit) const
{
	const auto& placed = m_units[unit];
	const int32_t left = m_metrics.leftGutter + placed.firstTier * m_metrics.tierSpacing;
	if (placed.leftNeighborTier < 0)
	{
		return left;
	}
	const int32_t neighborRight = m_metrics.leftGutter + placed.leftNeighborTier * m_metrics.tierSpacing + m_metrics.unitWidth;
	return std::max(0, left - neighborRight);
}

int32_t ResearchTreeLayout::unitRowOffset(uint32_t unit, int32_t rowWithinUnit) const
{
	const auto& placed = m_units[unit];
	const int32_t height = unitSize(unit).y;
	return height * (2 * rowWithinUnit + 1) / (2 * std::max(1, placed.rowSpan));
}

// Where an edge meets a point, in logical pixels.
// A unit end sits on its own step's pip rather than on the layout's row grid.
int32_t ResearchTreeLayout::attachHeight(uint32_t point, uint32_t edge, bool isStart) const
{
	if (point >= m_units.size() || edge >= m_edges.size())
	{
		return rowMiddle(rowOf(point));
	}
	const int32_t within = isStart ? m_edges[edge].fromRow : m_edges[edge].toRow;
	const int32_t top = (m_units[point].row < static_cast<int32_t>(m_rowTop.size())) ? m_rowTop[m_units[point].row] : 0;
	return top + unitRowOffset(point, within);
}

Vector2i ResearchTreeLayout::segmentFrom(uint32_t segment) const
{
	const auto& hop = m_segments[segment];
	return Vector2i(m_metrics.leftGutter + hop.tier * m_metrics.tierSpacing + m_metrics.unitWidth / 2,
	                attachHeight(hop.from, hop.edge, true));
}

Vector2i ResearchTreeLayout::segmentTo(uint32_t segment) const
{
	const auto& hop = m_segments[segment];
	return Vector2i(m_metrics.leftGutter + (hop.tier + 1) * m_metrics.tierSpacing + m_metrics.unitWidth / 2,
	                attachHeight(hop.to, hop.edge, false));
}

Vector2i ResearchTreeLayout::unitTopLeft(uint32_t unit) const
{
	const auto& placed = m_units[unit];
	const int32_t top = (placed.row < static_cast<int32_t>(m_rowTop.size())) ? m_rowTop[placed.row] : 0;
	return Vector2i(m_metrics.leftGutter + placed.firstTier * m_metrics.tierSpacing, top);
}

Vector2i ResearchTreeLayout::unitSize(uint32_t unit) const
{
	const auto& placed = m_units[unit];
	// Every row a unit covers holds a unit, so the pitch inside is uniform
	return Vector2i((placed.lastTier - placed.firstTier) * m_metrics.tierSpacing + m_metrics.unitWidth,
	                placed.rowSpan * m_metrics.unitHeight + (placed.rowSpan - 1) * m_metrics.rowPadding);
}

Vector2i ResearchTreeLayout::unitCenter(uint32_t unit) const
{
	const Vector2i topLeft = unitTopLeft(unit);
	const Vector2i size = unitSize(unit);
	return Vector2i(topLeft.x + size.x / 2, topLeft.y + size.y / 2);
}

int64_t ResearchTreeLayout::totalEdgeTravel() const
{
	int64_t total = 0;
	for (uint32_t e = 0; e < m_edges.size(); ++e)
	{
		total += std::abs(attachRow(m_edges[e].from, e, true) - attachRow(m_edges[e].to, e, false));
	}
	return total;
}

size_t ResearchTreeLayout::crossingCount() const
{
	// Two hops over the same gap cross when their ends are in opposite row order - count inverted pairs one gap at a time
	std::vector<std::vector<std::pair<int32_t, int32_t>>> byGap(m_tierOccupancy.empty() ? 0 : m_tierOccupancy.size() - 1);
	for (const auto& segment : m_segments)
	{
		if (segment.tier >= 0 && segment.tier < static_cast<int32_t>(byGap.size()))
		{
			byGap[segment.tier].push_back({attachRow(segment.from, segment.edge, true),
			                               attachRow(segment.to, segment.edge, false)});
		}
	}

	size_t crossings = 0;
	for (auto& gap : byGap)
	{
		std::sort(gap.begin(), gap.end());
		for (size_t a = 0; a < gap.size(); ++a)
		{
			for (size_t b = a + 1; b < gap.size(); ++b)
			{
				if (gap[a].first != gap[b].first && gap[a].second > gap[b].second)
				{
					crossings++;
				}
			}
		}
	}
	return crossings;
}

// ---------------------------------------------------------------------------
// MARK: - Rows
// ---------------------------------------------------------------------------

// Everything drawn covers a range of tiers - a unit the range its steps span, a long
// edge the tiers it crosses - and two things share a row whenever those don't meet
void ResearchTreeLayout::packIntoRows(Ordering ordering)
{
	const size_t unitCount = m_units.size();
	const size_t itemCount = unitCount + m_chains.size();
	const size_t tiers = m_tierOccupancy.size();
	m_pointRow.assign(unitCount + m_dummyTiers.size(), 0);
	if (itemCount == 0 || tiers == 0)
	{
		return;
	}

	std::vector<int32_t> spanFrom(itemCount, 0);
	std::vector<int32_t> spanTo(itemCount, 0);
	std::vector<int32_t> itemRows(itemCount, 1);
	std::vector<uint8_t> itemLane(itemCount, 0);
	for (uint32_t u = 0; u < unitCount; ++u)
	{
		spanFrom[u] = m_units[u].firstTier;
		spanTo[u] = m_units[u].lastTier;
		itemRows[u] = m_units[u].rowSpan;
		itemLane[u] = m_units[u].laneIndex;
	}
	for (size_t c = 0; c < m_chains.size(); ++c)
	{
		spanFrom[unitCount + c] = m_chains[c].firstTier;
		spanTo[unitCount + c] = m_chains[c].lastTier;
		itemLane[unitCount + c] = m_edges[m_chains[c].edge].colorLane;
	}

	// Which items sit either side, so a round can ask where it would rather be.
	// A long edge stands between its two units rather than beside them.
	std::vector<std::vector<uint32_t>> neighbors(itemCount);
	std::vector<uint32_t> chainOfEdge(m_edges.size(), UINT32_MAX);
	for (size_t c = 0; c < m_chains.size(); ++c)
	{
		chainOfEdge[m_chains[c].edge] = static_cast<uint32_t>(unitCount + c);
	}
	for (uint32_t e = 0; e < m_edges.size(); ++e)
	{
		const uint32_t middle = chainOfEdge[e];
		const uint32_t from = m_edges[e].from;
		const uint32_t to = m_edges[e].to;
		if (middle == UINT32_MAX)
		{
			neighbors[from].push_back(to);
			neighbors[to].push_back(from);
			continue;
		}
		neighbors[from].push_back(middle);
		neighbors[to].push_back(middle);
		neighbors[middle].push_back(from);
		neighbors[middle].push_back(to);
	}

	// First, fit down the rows, preferring one that already holds this item's tech category.
	// (It costs 0 height, any row it settles on being one the item fits in anyway, and keeps a category readable as a band.)
	// Once there is somewhere each item would rather be it takes the nearest free row to that, the lowest one ignoring the
	// preference entirely.
	const bool byLane = (ordering == Ordering::LaneGrouped);
	std::vector<int32_t> assigned(itemCount, 0);
	const std::vector<float> *wanted = nullptr;
	const auto place = [&](const std::vector<uint32_t>& order) {
		std::vector<std::vector<bool>> occupied;
		std::vector<uint32_t> lanesInRow;
		for (const auto item : order)
		{
			const int32_t from = spanFrom[item];
			const int32_t to = spanTo[item];
			const uint32_t laneBit = 1u << (itemLane[item] % 32);
			const size_t height = static_cast<size_t>(std::max(1, itemRows[item]));
			// Two things in one row need no empty tier between them - a tier is wider than the part of a unit sitting in it
			const int32_t low = std::max(0, from);
			const int32_t high = std::min(static_cast<int32_t>(tiers) - 1, to);

			size_t lowest = SIZE_MAX;
			size_t matching = SIZE_MAX;
			size_t nearest = SIZE_MAX;
			float shortest = 0.f;
			for (size_t row = 0; row + height <= occupied.size(); ++row)
			{
				bool free = true;
				for (size_t at = row; at < row + height && free; ++at)
				{
					for (int32_t tier = low; tier <= high && free; ++tier)
					{
						free = !occupied[at][tier];
					}
				}
				if (!free)
				{
					continue;
				}
				if (wanted != nullptr)
				{
					const float away = std::abs(static_cast<float>(row) - (*wanted)[item]);
					if (nearest == SIZE_MAX || away < shortest)
					{
						nearest = row;
						shortest = away;
					}
					continue;
				}
				if (lowest == SIZE_MAX)
				{
					lowest = row;
					if (!byLane)
					{
						break;
					}
				}
				if ((lanesInRow[row] & laneBit) != 0)
				{
					matching = row;
					break;
				}
			}

			size_t chosen = (wanted != nullptr) ? nearest : ((matching != SIZE_MAX) ? matching : lowest);
			if (chosen == SIZE_MAX)
			{
				chosen = occupied.size();
			}
			while (occupied.size() < chosen + height)
			{
				occupied.emplace_back(tiers, false);
				lanesInRow.push_back(0);
			}
			for (size_t at = chosen; at < chosen + height; ++at)
			{
				for (int32_t tier = from; tier <= to; ++tier)
				{
					occupied[at][tier] = true;
				}
			}
			lanesInRow[chosen] |= laneBit;
			assigned[item] = static_cast<int32_t>(chosen);
		}
	};

	const auto applyRows = [&](const std::vector<int32_t>& rows) {
		for (uint32_t u = 0; u < unitCount; ++u)
		{
			m_units[u].row = rows[u];
			m_pointRow[u] = rows[u];
		}
		for (size_t c = 0; c < m_chains.size(); ++c)
		{
			const int32_t row = rows[unitCount + c];
			for (uint32_t d = 0; d < m_chains[c].dummyCount; ++d)
			{
				m_pointRow[unitCount + m_chains[c].firstDummy + d] = row;
			}
		}
	};

	// Start in tier order - where the rounds begin does not depend on the order units happened to be created in
	std::vector<uint32_t> order(itemCount);
	for (uint32_t item = 0; item < itemCount; ++item)
	{
		order[item] = item;
	}
	std::stable_sort(order.begin(), order.end(), [&](uint32_t l, uint32_t r) {
		return spanFrom[l] < spanFrom[r];
	});
	place(order);
	applyRows(assigned);
	if (ordering == Ordering::None)
	{
		return;
	}

	std::vector<int32_t> best = assigned;
	int64_t bestTravel = totalEdgeTravel();

	// Repack vs where each item's neighbors ended up, keeping the best arrangement seen rather than the
	// last, since a round can undo a good one.
	// Measured: how far edges run up and down in total (rather than by crossings), since this view draws
	// almost no edges at once and what matters is that those few are short.
	constexpr size_t PACK_ROUNDS = 20;
	for (size_t round = 0; round < PACK_ROUNDS; ++round)
	{
		std::vector<float> preference(itemCount, 0.f);
		for (uint32_t item = 0; item < itemCount; ++item)
		{
			preference[item] = static_cast<float>(assigned[item]);
			if (neighbors[item].empty())
			{
				continue;
			}
			float total = 0.f;
			for (const auto neighbor : neighbors[item])
			{
				total += static_cast<float>(assigned[neighbor]);
			}
			preference[item] = total / static_cast<float>(neighbors[item].size());
		}
		std::stable_sort(order.begin(), order.end(), [&](uint32_t l, uint32_t r) {
			if (byLane && itemLane[l] != itemLane[r])
			{
				return itemLane[l] < itemLane[r];
			}
			return preference[l] < preference[r];
		});

		wanted = &preference;
		place(order);
		wanted = nullptr;
		applyRows(assigned);
		const int64_t travel = totalEdgeTravel();
		if (travel < bestTravel)
		{
			bestTravel = travel;
			best = assigned;
		}
	}

	applyRows(best);
}

void ResearchTreeLayout::setMetrics(const LayoutMetrics& metrics)
{
	m_metrics = metrics;
	assignCoordinates();
}

void ResearchTreeLayout::assignCoordinates()
{
	int32_t rows = 0;
	for (const auto row : m_pointRow)
	{
		rows = std::max(rows, row + 1);
	}
	for (const auto& unit : m_units)
	{
		rows = std::max(rows, unit.row + unit.rowSpan);
	}
	m_rowHoldsUnit.assign(rows, false);
	for (const auto& unit : m_units)
	{
		for (int32_t at = unit.row; at < unit.row + unit.rowSpan; ++at)
		{
			m_rowHoldsUnit[at] = true;
		}
	}

	// A row carrying only edges takes 0 height of its own
	// (this lets a long edge cross a crowded part of the tree without pushing anything apart)
	m_rowTop.assign(rows, 0);
	int32_t y = 0;
	for (int32_t row = 0; row < rows; ++row)
	{
		m_rowTop[row] = y;
		y += (m_rowHoldsUnit[row] ? m_metrics.unitHeight : 0) + m_metrics.rowPadding;
	}

	if (m_units.empty() || m_tierOccupancy.empty())
	{
		m_canvasSize = Vector2i(0, 0);
		return;
	}
	m_canvasSize = Vector2i(m_metrics.leftGutter + static_cast<int32_t>(m_tierOccupancy.size() - 1) * m_metrics.tierSpacing + m_metrics.unitWidth, y - m_metrics.rowPadding);
}

// ---------------------------------------------------------------------------
// MARK: - Units
// ---------------------------------------------------------------------------

// Every topic in a unit (either its progression's or one of its own) before any merging.
// The two filters differ:
// - a topic the player may not see leaves its progression, since a group keeping it would name and count it
// - one merely off a traced path stays, as a progression drawn with holes reads as not being a progression
static void buildResearchUnits(const ResearchGraph& graph, const std::vector<ResearchTrack>& tracks, ResearchTreeLayout::TrackMode mode, const std::vector<bool> *includedResearch, const std::vector<bool> *visibleResearch, std::vector<LayoutUnit>& units, std::unordered_map<uint16_t, uint32_t>& unitOfResearch)
{
	const auto& nodes = graph.nodes();
	const auto visible = [visibleResearch](uint16_t researchIndex) {
		return visibleResearch == nullptr
			|| (researchIndex < visibleResearch->size() && (*visibleResearch)[researchIndex]);
	};
	const auto wanted = [includedResearch, &visible](uint16_t researchIndex) {
		return visible(researchIndex)
			&& (includedResearch == nullptr
			    || (researchIndex < includedResearch->size() && (*includedResearch)[researchIndex]));
	};

	if (mode == ResearchTreeLayout::TrackMode::Collapse)
	{
		for (const auto& track : tracks)
		{
			if (std::none_of(track.members.begin(), track.members.end(), wanted))
			{
				continue;
			}
			LayoutUnit unit;
			for (const auto member : track.members)
			{
				if (visible(member))
				{
					unit.members.push_back(member);
				}
			}
			if (unit.members.empty())
			{
				continue;
			}
			unit.name = track.name;
			for (const auto member : unit.members)
			{
				unitOfResearch[member] = static_cast<uint32_t>(units.size());
			}
			// Off the first member still standing, so a progression impacted by what the
			// player has seen is not named or colored by a topic no longer in it
			const auto first = graph.nodeForResearchIndex(unit.members.front());
			unit.laneIndex = first ? nodes[*first].laneIndex : 0;
			units.push_back(std::move(unit));
		}
	}
	for (const auto& node : nodes)
	{
		if (unitOfResearch.count(node.primaryResearchIndex) > 0 || !wanted(node.primaryResearchIndex))
		{
			continue;
		}
		LayoutUnit unit;
		unit.members.push_back(node.primaryResearchIndex);
		unit.laneIndex = node.laneIndex;
		unitOfResearch[node.primaryResearchIndex] = static_cast<uint32_t>(units.size());
		units.push_back(std::move(unit));
	}
}

// ---------------------------------------------------------------------------
// MARK: - Knots
// ---------------------------------------------------------------------------

// A container this tall stops reading as one object and starts being the tree.
// Every knot on the built-in campaign and mp datasets is usually two or three rows (except one on campaign that is far deeper).
static constexpr int32_t MAX_KNOT_ROWS = 4;

// Progressions that need each other in both directions (ex. engines <-> bodies <-> propulsion).
// Drawing them apart asserts an order that the data does not have, so each group becomes one object,
// unless it is deep enough that the result holds most of the tree and says less than the separate
// progressions would.
//
// Calculated over every topic there is, not just over what is on show, so a group is consistent
// even in campaign mode where topics are revealed / discovered as play progresses.
//
// Returns the knot each topic belongs to, holding only topics in a group that merges.
static std::unordered_map<uint16_t, uint32_t> researchKnots(const std::vector<LayoutUnit>& units, const std::unordered_map<uint16_t, uint32_t>& unitOfResearch, const ResearchGraph& graph, const std::vector<int32_t>& topicTier)
{
	std::unordered_map<uint16_t, uint32_t> knotOfResearch;
	const size_t count = units.size();
	if (count < 2)
	{
		return knotOfResearch;
	}

	const auto& nodes = graph.nodes();
	std::vector<std::vector<uint32_t>> outgoing(count);
	std::vector<std::set<uint32_t>> pointsAt(count);
	for (const auto& edge : graph.edges())
	{
		const auto from = unitOfResearch.find(nodes[edge.from].primaryResearchIndex);
		const auto to = unitOfResearch.find(nodes[edge.to].primaryResearchIndex);
		if (from == unitOfResearch.end() || to == unitOfResearch.end() || from->second == to->second)
		{
			continue;
		}
		outgoing[from->second].push_back(to->second);
		pointsAt[from->second].insert(to->second);
	}

	// Strongly connected components
	std::vector<int32_t> order(count, -1), low(count, 0), component(count, -1);
	std::vector<bool> onStack(count, false);
	std::vector<uint32_t> stack;
	int32_t counter = 0, components = 0;
	for (uint32_t root = 0; root < count; ++root)
	{
		if (order[root] != -1)
		{
			continue;
		}
		std::vector<std::pair<uint32_t, size_t>> walk{{root, 0}};
		order[root] = low[root] = counter++;
		stack.push_back(root);
		onStack[root] = true;
		while (!walk.empty())
		{
			auto& frame = walk.back();
			if (frame.second < outgoing[frame.first].size())
			{
				const uint32_t next = outgoing[frame.first][frame.second++];
				if (order[next] == -1)
				{
					order[next] = low[next] = counter++;
					stack.push_back(next);
					onStack[next] = true;
					walk.push_back({next, 0});
				}
				else if (onStack[next])
				{
					low[frame.first] = std::min(low[frame.first], order[next]);
				}
				continue;
			}
			const uint32_t at = frame.first;
			walk.pop_back();
			if (!walk.empty())
			{
				low[walk.back().first] = std::min(low[walk.back().first], low[at]);
			}
			if (low[at] == order[at])
			{
				uint32_t member = 0;
				do
				{
					member = stack.back();
					stack.pop_back();
					onStack[member] = false;
					component[member] = components;
				} while (member != at);
				components++;
			}
		}
	}

	std::vector<std::vector<uint32_t>> byComponent(static_cast<size_t>(components));
	for (uint32_t u = 0; u < count; ++u)
	{
		byComponent[static_cast<size_t>(component[u])].push_back(u);
	}

	const auto tierOf = [&graph, &topicTier](uint16_t researchIndex) {
		const auto node = graph.nodeForResearchIndex(researchIndex);
		return node ? topicTier[*node] : 0;
	};

	uint32_t knots = 0;
	for (auto& group : byComponent)
	{
		if (group.size() < 2)
		{
			continue;
		}
		std::map<int32_t, int32_t> perTier;
		int32_t tallest = 1;
		for (const auto u : group)
		{
			for (const auto member : units[u].members)
			{
				tallest = std::max(tallest, ++perTier[tierOf(member)]);
			}
		}
		if (tallest > MAX_KNOT_ROWS)
		{
			continue;
		}

		// Two progressions that point at each other are the intended target.
		//
		// A group can also come out of the pass above held together by one long chain that happens to close,
		// with nothing in it needing anything both ways, because a progression spanning twelve tiers is one
		// node here. Merging that would encompass too much of the tree, so check.
		bool needEachOther = false;
		for (const auto u : group)
		{
			for (const auto other : pointsAt[u])
			{
				needEachOther = needEachOther || pointsAt[other].count(u) > 0;
			}
		}
		if (!needEachOther)
		{
			continue;
		}
		for (const auto u : group)
		{
			for (const auto member : units[u].members)
			{
				knotOfResearch[member] = knots;
			}
		}
		knots++;
	}
	return knotOfResearch;
}

// Put every unit belonging to one knot into a single object.
// What merges was decided over the entire tree, so a knot may arrive with parts missing, and one with a single
// part left is just that progression again.
static void mergeResearchKnots(std::vector<LayoutUnit>& units, std::unordered_map<uint16_t, uint32_t>& unitOfResearch, const std::unordered_map<uint16_t, uint32_t>& knotOfResearch)
{
	if (knotOfResearch.empty())
	{
		return;
	}

	// Which knot each unit is part of. A unit is one whole progression or one lone topic, so its members never disagree.
	std::vector<optional<size_t>> groupOfUnit(units.size());
	std::vector<std::vector<uint32_t>> groups;
	std::unordered_map<uint32_t, size_t> groupOfKnot;
	for (uint32_t u = 0; u < units.size(); ++u)
	{
		optional<uint32_t> knot;
		for (const auto member : units[u].members)
		{
			const auto at = knotOfResearch.find(member);
			if (at != knotOfResearch.end())
			{
				knot = at->second;
				break;
			}
		}
		if (!knot)
		{
			continue;
		}
		auto group = groupOfKnot.find(*knot);
		if (group == groupOfKnot.end())
		{
			group = groupOfKnot.insert({*knot, groups.size()}).first;
			groups.push_back({});
		}
		groups[group->second].push_back(u);
		groupOfUnit[u] = group->second;
	}

	// Rebuild in the order the units were already in, so a dataset with no knot at all comes out exactly as it went in
	std::vector<LayoutUnit> rebuilt;
	rebuilt.reserve(units.size());
	std::vector<bool> emitted(groups.size(), false);
	for (uint32_t u = 0; u < units.size(); ++u)
	{
		if (!groupOfUnit[u] || groups[*groupOfUnit[u]].size() < 2)
		{
			rebuilt.push_back(std::move(units[u]));
			continue;
		}
		const size_t group = *groupOfUnit[u];
		if (emitted[group])
		{
			continue;
		}
		emitted[group] = true;

		LayoutUnit knot;
		knot.merged = true;
		std::map<uint8_t, size_t> lanes;
		size_t longest = 0;
		for (const auto part : groups[group])
		{
			for (const auto member : units[part].members)
			{
				knot.members.push_back(member);
			}
			lanes[units[part].laneIndex] += units[part].members.size();
			// Named after the biggest progression still standing in it.
			// (The tech category would name six of the eight multiplayer knots "Weapons".)
			if (!units[part].name.isEmpty() && units[part].members.size() > longest)
			{
				longest = units[part].members.size();
				knot.name = units[part].name;
			}
		}
		// Colored for whatever most of it is, since a knot has no one progression to take a lane from
		knot.laneIndex = std::max_element(lanes.begin(), lanes.end(),
		                                  [](const std::pair<uint8_t, size_t>& l, const std::pair<uint8_t, size_t>& r) { return l.second < r.second; })->first;
		if (knot.name.isEmpty() && !knot.members.empty() && knot.members.front() < asResearch.size())
		{
			knot.name = researchTechCategoryName(asResearch[knot.members.front()]);
		}
		rebuilt.push_back(std::move(knot));
	}

	units = std::move(rebuilt);
	unitOfResearch.clear();
	for (uint32_t u = 0; u < units.size(); ++u)
	{
		for (const auto member : units[u].members)
		{
			unitOfResearch[member] = u;
		}
	}
}

ResearchTreeLayout ResearchTreeLayout::build(const ResearchGraph& graph, const std::vector<ResearchTrack>& tracks, TrackMode mode, Ordering ordering, const LayoutMetrics& metrics, const std::vector<bool> *includedResearch, const std::vector<bool> *visibleResearch)
{
	ResearchTreeLayout layout;
	const auto& nodes = graph.nodes();

	// What the two filters mean is written where the units are built - now the same test, for the edges between them.
	const auto wanted = [includedResearch, visibleResearch](uint16_t researchIndex) {
		return (visibleResearch == nullptr
		        || (researchIndex < visibleResearch->size() && (*visibleResearch)[researchIndex]))
			&& (includedResearch == nullptr
			    || (researchIndex < includedResearch->size() && (*includedResearch)[researchIndex]));
	};

	std::unordered_map<uint16_t, uint32_t> unitOfResearch;
	buildResearchUnits(graph, tracks, mode, includedResearch, visibleResearch, layout.m_units, unitOfResearch);

	// Tier every topic. The research list is acyclic, so this always succeeds and no topic ever leaves the tier it is given.
	std::vector<std::vector<uint32_t>> topicOut(nodes.size());
	std::vector<size_t> topicInDegree(nodes.size(), 0);
	for (const auto& edge : graph.edges())
	{
		topicOut[edge.from].push_back(edge.to);
		topicInDegree[edge.to]++;
	}
	std::vector<int32_t> topicTier;
	const bool layered = layerLongestPath(nodes.size(), topicOut, topicInDegree, topicTier);
	ASSERT(layered, "Research topics do not form an acyclic graph");

	// Which progressions need each other (calculated over every topic there is, not just discovered / visible).
	// Off the tiers as laid out, before the squeeze below, since the whole tree leaves no tier empty to squeeze.
	std::unordered_map<uint16_t, uint32_t> knotOfResearch;
	if (mode == TrackMode::Collapse)
	{
		std::vector<LayoutUnit> everyUnit;
		std::unordered_map<uint16_t, uint32_t> unitOfEveryResearch;
		buildResearchUnits(graph, tracks, mode, nullptr, nullptr, everyUnit, unitOfEveryResearch);
		knotOfResearch = researchKnots(everyUnit, unitOfEveryResearch, graph, topicTier);
	}

	// Tiers nothing is in only exist when part of the tree is left out (ex. under a focus).
	// Squeezing them out keeps every tier strictly after the one before it, so every dependency still points forwards.
	int32_t topTier = 0;
	for (const auto tier : topicTier)
	{
		topTier = std::max(topTier, tier);
	}
	std::vector<bool> tierUsed(topTier + 1, false);
	for (const auto& entry : unitOfResearch)
	{
		const auto node = graph.nodeForResearchIndex(entry.first);
		if (node)
		{
			tierUsed[topicTier[*node]] = true;
		}
	}
	std::vector<int32_t> tierRemap(topTier + 1, 0);
	int32_t dense = 0;
	for (int32_t tier = 0; tier <= topTier; ++tier)
	{
		tierRemap[tier] = dense;
		dense += tierUsed[tier] ? 1 : 0;
	}
	for (auto& tier : topicTier)
	{
		tier = tierRemap[tier];
	}
	const auto tierOfTopic = [&graph, &topicTier](uint16_t researchIndex) {
		const auto node = graph.nodeForResearchIndex(researchIndex);
		return node ? topicTier[*node] : 0;
	};

	mergeResearchKnots(layout.m_units, unitOfResearch, knotOfResearch);
	const size_t unitCount = layout.m_units.size();

	// A unit covers the range its steps span.
	// Every step keeps its own tier, so nothing is pulled out of order by being drawn as one object.
	for (auto& unit : layout.m_units)
	{
		unit.memberTiers.clear();
		unit.memberTiers.reserve(unit.members.size());
		unit.firstTier = INT32_MAX;
		unit.lastTier = 0;
		for (const auto member : unit.members)
		{
			const int32_t tier = tierOfTopic(member);
			unit.memberTiers.push_back(tier);
			unit.firstTier = std::min(unit.firstTier, tier);
			unit.lastTier = std::max(unit.lastTier, tier);
		}
		if (unit.members.empty())
		{
			unit.firstTier = 0;
		}

		// Left to right, the order the steps are drawn in and so the only order anything counting them can use.
		// A progression arrives ordered by what supersedes what, which is not always where its steps end up -
		// a later step may need a prerequisite two tiers on.
		// Stable, so steps sharing a tier keep their order.
		std::vector<size_t> order(unit.members.size());
		for (size_t m = 0; m < order.size(); ++m)
		{
			order[m] = m;
		}
		std::stable_sort(order.begin(), order.end(),
		                 [&unit](size_t l, size_t r) { return unit.memberTiers[l] < unit.memberTiers[r]; });
		std::vector<uint16_t> byTier;
		std::vector<int32_t> tiersByTier;
		byTier.reserve(order.size());
		tiersByTier.reserve(order.size());
		for (const auto at : order)
		{
			byTier.push_back(unit.members[at]);
			tiersByTier.push_back(unit.memberTiers[at]);
		}
		unit.members = std::move(byTier);
		unit.memberTiers = std::move(tiersByTier);

		// A step shares a tier with another when the two are prereq siblings, so give each its own row.
		// A unit is as tall as its most crowded tier, so center what sparser ones hold - since piling
		// every tier against the top leaves a tall unit's steps looking like they belong to whatever
		// is above it.
		const size_t tiers = static_cast<size_t>(unit.lastTier - unit.firstTier + 1);
		std::vector<int32_t> atTier(tiers, 0);
		for (const auto tier : unit.memberTiers)
		{
			atTier[static_cast<size_t>(tier - unit.firstTier)]++;
		}
		unit.rowSpan = 1;
		for (const auto count : atTier)
		{
			unit.rowSpan = std::max(unit.rowSpan, count);
		}

		std::vector<int32_t> takenAtTier(tiers, 0);
		unit.memberRows.clear();
		unit.memberRows.reserve(unit.members.size());
		for (const auto tier : unit.memberTiers)
		{
			const size_t column = static_cast<size_t>(tier - unit.firstTier);
			const int32_t top = (unit.rowSpan - atTier[column]) / 2;
			unit.memberRows.push_back(top + takenAtTier[column]++);
		}
	}

	// Which row of its unit each topic sits in, so an edge leaves and arrives at the step it belongs to rather than the unit as a whole
	const auto rowOfTopic = [&layout, &unitOfResearch](uint16_t researchIndex) {
		const auto unit = unitOfResearch.find(researchIndex);
		if (unit == unitOfResearch.end())
		{
			return 0;
		}
		const auto& members = layout.m_units[unit->second].members;
		const auto at = std::find(members.begin(), members.end(), researchIndex);
		return (at == members.end()) ? 0 : layout.m_units[unit->second].memberRows[at - members.begin()];
	};

	// Unit edges, attached at the tier and row of the step at each end.
	// Two steps of one progression needing the same thing are one line,
	// but a dependency on an early step + one on a late step are not.
	std::set<std::tuple<uint32_t, int32_t, int32_t, uint32_t, int32_t, int32_t>> seen;
	for (const auto& edge : graph.edges())
	{
		const auto fromUnit = unitOfResearch.find(nodes[edge.from].primaryResearchIndex);
		const auto toUnit = unitOfResearch.find(nodes[edge.to].primaryResearchIndex);
		if (fromUnit == unitOfResearch.end() || toUnit == unitOfResearch.end())
		{
			continue; // one end was left out
		}
		if (fromUnit->second == toUnit->second)
		{
			// Both ends are in one object, so there's nothing for the layout to place.
			// Still a dependency, so it is kept for whoever draws the unit.
			auto& inside = layout.m_units[fromUnit->second];
			const auto from = std::find(inside.members.begin(), inside.members.end(), nodes[edge.from].primaryResearchIndex);
			const auto to = std::find(inside.members.begin(), inside.members.end(), nodes[edge.to].primaryResearchIndex);
			if (from != inside.members.end() && to != inside.members.end())
			{
				inside.internalEdges.push_back({static_cast<uint16_t>(from - inside.members.begin()),
				                                static_cast<uint16_t>(to - inside.members.begin())});
			}
			continue;
		}

		LayoutEdge unitEdge;
		unitEdge.from = fromUnit->second;
		unitEdge.to = toUnit->second;
		unitEdge.fromTier = topicTier[edge.from];
		unitEdge.toTier = topicTier[edge.to];
		unitEdge.bothWanted = wanted(nodes[edge.from].primaryResearchIndex)
			&& wanted(nodes[edge.to].primaryResearchIndex);
		unitEdge.fromRow = rowOfTopic(nodes[edge.from].primaryResearchIndex);
		unitEdge.toRow = rowOfTopic(nodes[edge.to].primaryResearchIndex);
		unitEdge.colorLane = layout.m_units[unitEdge.from].laneIndex;
		if (!seen.insert({unitEdge.from, unitEdge.fromTier, unitEdge.fromRow,
		                  unitEdge.to, unitEdge.toTier, unitEdge.toRow}).second)
		{
			continue;
		}
		ASSERT(unitEdge.toTier > unitEdge.fromTier, "Research tree edge %" PRIu32 " -> %" PRIu32 " runs backwards",
		       unitEdge.from, unitEdge.to);
		layout.m_edges.push_back(unitEdge);
	}

	// Split every edge crossing more than one tier into single-tier hops, so packing and drawing only deal with neighboring tiers.
	// (The whole run shares a row, which is what makes a long edge draw straight.)
	for (uint32_t e = 0; e < layout.m_edges.size(); ++e)
	{
		const auto& edge = layout.m_edges[e];
		const int32_t span = edge.toTier - edge.fromTier;
		if (span > 1)
		{
			DummyChain chain;
			chain.edge = e;
			chain.firstDummy = static_cast<uint32_t>(layout.m_dummyTiers.size());
			chain.dummyCount = static_cast<uint32_t>(span - 1);
			chain.firstTier = edge.fromTier + 1;
			chain.lastTier = edge.toTier - 1;
			layout.m_chains.push_back(chain);
		}

		uint32_t previous = edge.from;
		for (int32_t step = 1; step < span; ++step)
		{
			const uint32_t dummy = static_cast<uint32_t>(unitCount + layout.m_dummyTiers.size());
			layout.m_dummyTiers.push_back(edge.fromTier + step);
			layout.m_dummyLanes.push_back(edge.colorLane);
			layout.m_segments.push_back({previous, dummy, edge.fromTier + step - 1, e});
			previous = dummy;
		}
		layout.m_segments.push_back({previous, edge.to, edge.toTier - 1, e});
	}

	int32_t highestTier = 0;
	for (const auto& unit : layout.m_units)
	{
		highestTier = std::max(highestTier, unit.lastTier);
	}
	layout.m_tierOccupancy.assign(highestTier + 1, 0);
	layout.m_tierSlots.assign(highestTier + 1, 0);
	for (const auto& unit : layout.m_units)
	{
		for (int32_t tier = unit.firstTier; tier <= unit.lastTier; ++tier)
		{
			layout.m_tierOccupancy[tier] += static_cast<size_t>(unit.rowSpan);
			layout.m_tierSlots[tier] += static_cast<size_t>(unit.rowSpan);
		}
	}
	for (const auto tier : layout.m_dummyTiers)
	{
		if (tier >= 0 && tier < static_cast<int32_t>(layout.m_tierSlots.size()))
		{
			layout.m_tierSlots[tier]++;
		}
	}

	layout.m_metrics = metrics;
	layout.packIntoRows(ordering);

	// What sits to the left of each unit in the rows it occupies (which decides how much room anything written beside it has)
	for (uint32_t u = 0; u < unitCount; ++u)
	{
		auto& placed = layout.m_units[u];
		placed.leftNeighborTier = -1;
		for (uint32_t other = 0; other < unitCount; ++other)
		{
			const auto& against = layout.m_units[other];
			if (other == u || against.lastTier >= placed.firstTier)
			{
				continue;
			}
			const bool sharesRow = (against.row < placed.row + placed.rowSpan)
				&& (placed.row < against.row + against.rowSpan);
			if (sharesRow)
			{
				placed.leftNeighborTier = std::max(placed.leftNeighborTier, against.lastTier);
			}
		}
	}

	layout.assignCoordinates();
	return layout;
}











