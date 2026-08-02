/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  Prerequisite-order queries over the research graph.
 *
 *  Deliberately narrow, so that callers wanting these do not have to include
 *  research.h and everything it drags in.
 */

#ifndef __INCLUDED_SRC_RESEARCHPREREQ_H__
#define __INCLUDED_SRC_RESEARCHPREREQ_H__

#include "lib/framework/frame.h"
#include "lib/framework/wzstring.h"
#include "researchdef.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

// Transitive prerequisite closure over a research list, as one bit row per topic.
//
// isPrereq(i, j) is true when research i is a direct or indirect prerequisite of
// research j.
//
// Built in O(V*E/64) time and O(V^2/8) memory (about 22 KB for a 400-topic
// dataset), so that repeated prerequisite queries are all just bit tests. The
// research list must be acyclic, which loadResearch() enforces with
// CycleDetection::detectCycle() before anything here runs.
class ResearchPrereqClosure
{
public:
	explicit ResearchPrereqClosure(const std::vector<RESEARCH>& research);

	bool isPrereq(size_t maybePrereq, size_t of) const
	{
		if (maybePrereq >= m_count || of >= m_count || maybePrereq == of)
		{
			return false;
		}
		return ((m_bits[of * m_words + (maybePrereq >> 6)] >> (maybePrereq & 63)) & 1u) != 0;
	}

	size_t size() const { return m_count; }

private:
	void setBit(size_t row, size_t bit);
	void orRowInto(size_t dstRow, size_t srcRow);

	size_t m_count;
	size_t m_words;
	std::vector<uint64_t> m_bits;
};

// Test whether a set of research topics forms a single linear chain under the
// transitive prerequisite order and, if so, produce them in chain order.
//
// For each member m, count how many other members of the set are transitive
// prerequisites of m (its "down-count").
//
//   - For a chain x0 < x1 < ... < x(n-1), downCount(xk) is exactly k.
//   - Conversely, if the down-counts are a permutation of 0..n-1 the members must
//     form a chain: y < x implies down(y) is a proper subset of down(x) which also
//     contains y, hence downCount(y) < downCount(x). So the member with count k
//     has exactly the members with counts 0..k-1 below it, which by induction
//     gives x0 < x1 < ... < x(n-1).
//
// The down-counts therefore decide validity and give each member's position at
// the same time, which is why the order is written out directly rather than
// sorted. Note that the prerequisite relation is only a partial order, so it must
// never be used as a comparator for std::sort or std::stable_sort. Those require
// a strict weak ordering, meaning incomparability has to be transitive, and it is
// not. Ex. given {A < C, B < D, C < D}, A and B are incomparable and B and C are
// incomparable, yet A < C.
//
// Returns false for an empty member set. orderedOut is cleared on failure.
// downCountsOut, when given, receives the per-member counts in the order the
// members were passed in, which is what diagnostics want: a repeated count names
// the branch.
bool computeResearchChainOrder(const std::vector<size_t>& members, const ResearchPrereqClosure& closure, std::vector<size_t>& orderedOut, std::vector<size_t>* downCountsOut = nullptr);

// Read-only view of the research categories built by the last loadResearch(),
// keyed by category name, each holding its members in chain order.
const std::unordered_map<WzString, std::vector<size_t>>& getResearchCategories();

#endif // __INCLUDED_SRC_RESEARCHPREREQ_H__
