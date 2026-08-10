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
 *  Research focus.
 */

#include "researchfocus.h"

#include "../research.h"

#include <algorithm>

ResearchFocus computeResearchFocus(const ResearchGraph& graph, const std::vector<uint16_t>& targetMembers)
{
	ResearchFocus focus;
	focus.included.assign(asResearch.size(), false);

	// Walk prerequisites backwards from the target.
	// (The list is acyclic, so a visited set is all this needs.)
	std::vector<uint32_t> pending;
	std::vector<bool> visited(graph.nodes().size(), false);
	for (const auto member : targetMembers)
	{
		const auto node = graph.nodeForResearchIndex(member);
		if (node && !visited[*node])
		{
			visited[*node] = true;
			pending.push_back(*node);
		}
	}
	std::vector<uint32_t> reached = pending;
	while (!pending.empty())
	{
		const uint32_t node = pending.back();
		pending.pop_back();
		for (const auto prereq : graph.prerequisitesOf(node))
		{
			if (!visited[prereq])
			{
				visited[prereq] = true;
				pending.push_back(prereq);
				reached.push_back(prereq);
			}
		}
	}

	for (const auto node : reached)
	{
		const uint16_t researchIndex = graph.nodes()[node].primaryResearchIndex;
		if (researchIndex >= focus.included.size())
		{
			continue;
		}
		focus.included[researchIndex] = true;
		focus.topics++;
		if (graph.nodes()[node].state == NodeState::Researched)
		{
			continue;
		}
		focus.remaining++;
		focus.remainingPower += asResearch[researchIndex].researchPower;
		focus.remainingPoints += asResearch[researchIndex].researchPoints;
	}

	// Longest chain of what is left, by longest path over the same edges.
	// Depth is only ever read from a node's prerequisites, so processing in the order
	// they were reached is not enough - work outwards from the roots instead.
	std::vector<int32_t> depth(graph.nodes().size(), 0);
	std::vector<uint32_t> order = reached;
	std::sort(order.begin(), order.end());
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto node : order)
		{
			int32_t best = 0;
			for (const auto prereq : graph.prerequisitesOf(node))
			{
				if (visited[prereq])
				{
					best = std::max(best, depth[prereq]);
				}
			}
			const int32_t own = best + ((graph.nodes()[node].state == NodeState::Researched) ? 0 : 1);
			if (own > depth[node])
			{
				depth[node] = own;
				changed = true;
			}
		}
	}
	for (const auto node : reached)
	{
		focus.criticalPath = std::max(focus.criticalPath, depth[node]);
	}

	return focus;
}
