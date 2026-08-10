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
 *  Research search.
 */

#include "researchsearch.h"

#include "../research.h"
#include "../researchdef.h"
#include "../statsdef.h"
#include "../structure.h"

#include <algorithm>

namespace
{

// Where a searchable string came from.
// A name wins ties against the same match found elsewhere.
enum SourceKind : uint8_t
{
	SOURCE_NAME = 0,
	SOURCE_UNLOCK,		// a component or structure the topic grants
	SOURCE_CATEGORY,
	SOURCE_ID,
};

// How well a query matched in a string, lower being better
enum MatchQuality : int
{
	MATCH_EXACT = 0,
	MATCH_PREFIX,
	MATCH_WORD_PREFIX,
	MATCH_SUBSTRING,
	MATCH_SUBSEQUENCE,
	MATCH_NONE,
};

// Neither case nor accent are allowed to matter.
std::string foldForSearch(const WzString& text)
{
	// Decomposing puts an accent in its own codepoint, which then gets dropped.
	const WzString decomposed = text.normalized(WzString::NormalizationForm_KD).toLower();
	std::string folded;
	for (const auto codepoint : decomposed.toUtf32())
	{
		if (codepoint >= 0x0300 && codepoint <= 0x036F)
		{
			continue; // a combining mark, ex. what is left of an acute accent
		}
		folded += WzString::fromUtf32({codepoint}).toUtf8();
	}
	return folded;
}

bool isWordBoundary(char c)
{
	return c == ' ' || c == '-' || c == '_' || c == '/' || c == '.';
}

MatchQuality qualityOf(const std::string& text, const std::string& query)
{
	if (text == query)
	{
		return MATCH_EXACT;
	}
	const size_t at = text.find(query);
	if (at == 0)
	{
		return MATCH_PREFIX;
	}
	if (at != std::string::npos)
	{
		return isWordBoundary(text[at - 1]) ? MATCH_WORD_PREFIX : MATCH_SUBSTRING;
	}

	// Letters in order but not together, which catches an abbreviation or a half-remembered name
	size_t q = 0;
	for (size_t t = 0; t < text.size() && q < query.size(); ++t)
	{
		if (text[t] == query[q])
		{
			++q;
		}
	}
	return (q == query.size()) ? MATCH_SUBSEQUENCE : MATCH_NONE;
}

} // anonymous namespace

void ResearchSearchIndex::build(const std::vector<bool> *visible)
{
	m_entries.clear();
	m_entries.reserve(asResearch.size() * 3);

	const auto add = [this](uint16_t researchIndex, uint8_t kind, const WzString& text) {
		if (text.isEmpty())
		{
			return;
		}
		Entry entry;
		entry.folded = foldForSearch(text);
		entry.original = text;
		entry.researchIndex = researchIndex;
		entry.kind = kind;
		if (!entry.folded.empty())
		{
			m_entries.push_back(std::move(entry));
		}
	};

	for (size_t i = 0; i < asResearch.size(); ++i)
	{
		if (visible != nullptr && (i >= visible->size() || !(*visible)[i]))
		{
			continue;
		}
		const RESEARCH& research = asResearch[i];
		const uint16_t index = static_cast<uint16_t>(i);

		add(index, SOURCE_NAME, WzString::fromUtf8(getLocalizedStatsName(&research)));
		add(index, SOURCE_ID, research.id);
		add(index, SOURCE_CATEGORY, research.category);

		for (const auto *component : research.componentResults)
		{
			if (component != nullptr)
			{
				add(index, SOURCE_UNLOCK, WzString::fromUtf8(getLocalizedStatsName(component)));
			}
		}
		for (const auto structIndex : research.pStructureResults)
		{
			if (structIndex < numStructureStats)
			{
				add(index, SOURCE_UNLOCK, WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])));
			}
		}
	}
}

std::vector<ResearchSearchHit> ResearchSearchIndex::find(const WzString& query, size_t maxResults) const
{
	std::vector<ResearchSearchHit> results;
	const std::string folded = foldForSearch(query);
	if (folded.size() < 2)
	{
		return results;
	}

	struct Scored
	{
		uint16_t researchIndex;
		int score;
		size_t length;		// a shorter string containing the query is the closer fit
		const Entry *entry;
	};
	std::vector<Scored> scored;

	for (const auto& entry : m_entries)
	{
		const MatchQuality quality = qualityOf(entry.folded, folded);
		if (quality == MATCH_NONE)
		{
			continue;
		}
		scored.push_back({entry.researchIndex, static_cast<int>(quality) * 8 + entry.kind, entry.folded.size(), &entry});
	}

	// Best first, and stable enough that the same query always gives the same list
	std::sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
		if (a.score != b.score) { return a.score < b.score; }
		if (a.length != b.length) { return a.length < b.length; }
		return a.researchIndex < b.researchIndex;
	});

	// A topic appears once, under whichever of its strings matched best
	std::vector<bool> taken(asResearch.size(), false);
	for (const auto& hit : scored)
	{
		if (results.size() >= maxResults)
		{
			break;
		}
		if (hit.researchIndex >= taken.size() || taken[hit.researchIndex])
		{
			continue;
		}
		taken[hit.researchIndex] = true;
		ResearchSearchHit result;
		result.researchIndex = hit.researchIndex;
		if (hit.entry->kind != SOURCE_NAME)
		{
			result.via = hit.entry->original;
		}
		results.push_back(std::move(result));
	}
	return results;
}
