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
 *  Deriving research progressions.
 */

#include "researchtracks.h"

#include "researcheffecttext.h"
#include "../research.h"
#include "../researchdef.h"
#include "../statsdef.h"
#include "../structuredef.h"
#include "../structure.h"
#include "../stats.h"

#include <set>
#include <string>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>

// ---------------------------------------------------------------------------
// MARK: - Chain extraction, shared by every source
// ---------------------------------------------------------------------------

// Positions into `group`, ordered so nothing comes before something it is above.
// Counting how many members are below each gives a linear extension of the partial order,
// which is all that any pass here needs.
static std::vector<size_t> byDepthIn(const std::vector<size_t>& group, const ResearchPrecedes& precedes)
{
	const size_t n = group.size();
	std::vector<size_t> byDepth(n);
	for (size_t i = 0; i < n; ++i)
	{
		byDepth[i] = i;
	}
	std::vector<size_t> below(n, 0);
	for (size_t a = 0; a < n; ++a)
	{
		for (size_t b = 0; b < n; ++b)
		{
			if (a != b && precedes(group[b], group[a]))
			{
				below[a]++;
			}
		}
	}
	std::stable_sort(byDepth.begin(), byDepth.end(), [&below](size_t l, size_t r) { return below[l] < below[r]; });
	return byDepth;
}

// Turn a candidate group into one track. A chain keeps its order, and one that branches
// keeps every member anyway, ordered so that nothing comes before what it is above.
//
// Not split into separate chains, which reads as several strips repeating each other -
// (on the campaign set the machinegun line breaks into four). A unit can be several rows
// tall, so two members that neither precedes sit in the same column.
static void emitTracksFromGroup(const std::vector<size_t>& group, const ResearchPrecedes& precedes, TrackSource source, const WzString& name, std::vector<ResearchTrack>& tracksOut, std::unordered_set<size_t>& claimed)
{
	std::vector<size_t> remaining;
	for (const auto member : group)
	{
		if (claimed.count(member) == 0)
		{
			remaining.push_back(member);
		}
	}
	if (remaining.size() < 2)
	{
		return; // nothing that reads as a progression
	}

	std::vector<size_t> ordered;
	if (!computeResearchChainOrder(remaining, precedes, ordered))
	{
		for (const auto at : byDepthIn(remaining, precedes))
		{
			ordered.push_back(remaining[at]);
		}
	}
	if (ordered.size() > MAX_RESEARCH_TRACK_MEMBERS)
	{
		// Whatever is left over stays unclaimed and is drawn as topics of its own, which is
		// truer than a second strip pretending to be a progression
		debug(LOG_INFO, "Research progression of %zu topics starting at %s exceeds the %zu member cap, dropping the rest",
		      ordered.size(), asResearch[ordered.front()].id.toUtf8().c_str(), MAX_RESEARCH_TRACK_MEMBERS);
		ordered.resize(MAX_RESEARCH_TRACK_MEMBERS);
	}

	ResearchTrack track;
	track.source = source;
	track.name = name;
	for (const auto member : ordered)
	{
		track.members.push_back(static_cast<uint16_t>(member));
		claimed.insert(member);
	}
	tracksOut.push_back(std::move(track));
}

// ---------------------------------------------------------------------------
// MARK: - Upgrade signature
// ---------------------------------------------------------------------------

// Every step of an upgrade progression changes the same set of things, differing only
// in how much, so ignoring the magnitude leaves a key that groups them.
// iconID joins the key as a guard: it changes nothing on the shipped data, where the signature
// implies it, but stops unrelated topics merging in unknown / mod data.
static WzString upgradeSignatureOf(const RESEARCH& research)
{
	if (!research.results.is_array() || research.results.empty())
	{
		return WzString();
	}

	std::vector<std::string> parts;
	parts.reserve(research.results.size());
	for (const auto& result : research.results)
	{
		if (!result.is_object())
		{
			continue;
		}
		const auto field = [&result](const char *key) -> std::string {
			const auto it = result.find(key);
			return (it != result.end() && it->is_string()) ? it->get<std::string>() : std::string();
		};
		parts.push_back(field("class") + "|" + field("parameter") + "|" + field("filterParameter") + "|" + field("filterValue"));
	}
	if (parts.empty())
	{
		return WzString();
	}

	std::sort(parts.begin(), parts.end());
	std::string signature = std::to_string(research.iconID);
	for (const auto& part : parts)
	{
		signature += "/" + part;
	}
	return WzString::fromUtf8(signature);
}

// ---------------------------------------------------------------------------
// MARK: - Obsolescence
// ---------------------------------------------------------------------------

// True when researching `newer` makes something `older` granted redundant.
// Weapon and structure tier lines are built this way, their members usually being prerequisite
// siblings rather than ancestors, so prerequisites do not see the progression.
static bool supersedes(const RESEARCH& newer, const RESEARCH& older)
{
	for (const auto *redundant : newer.pRedArtefacts)
	{
		if (std::find(older.componentResults.begin(), older.componentResults.end(), redundant) != older.componentResults.end())
		{
			return true;
		}
	}
	for (const auto& replacement : newer.componentReplacement)
	{
		if (std::find(older.componentResults.begin(), older.componentResults.end(), replacement.pOldComponent) != older.componentResults.end())
		{
			return true;
		}
	}
	for (const auto redundant : newer.pRedStructs)
	{
		if (std::find(older.pStructureResults.begin(), older.pStructureResults.end(), redundant) != older.pStructureResults.end())
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// MARK: - Naming a group that nobody named
// ---------------------------------------------------------------------------

static std::vector<std::string> wordsOf(const std::string& text)
{
	std::vector<std::string> words;
	size_t at = 0;
	while (at < text.size())
	{
		const size_t next = text.find(' ', at);
		if (next == std::string::npos)
		{
			if (next != at) { words.push_back(text.substr(at)); }
			break;
		}
		if (next != at) { words.push_back(text.substr(at, next - at)); }
		at = next + 1;
	}
	return words;
}

// The longest run of whole words every name shares, in order but not necessarily touching,
// so "Cyclone AA Flak Site" and "Whirlwind AA Site" give "AA Site"
static std::vector<std::string> commonWords(const std::vector<std::string>& a, const std::vector<std::string>& b)
{
	const size_t m = a.size();
	const size_t n = b.size();
	std::vector<size_t> length((m + 1) * (n + 1), 0);
	for (size_t i = m; i-- > 0; )
	{
		for (size_t j = n; j-- > 0; )
		{
			length[i * (n + 1) + j] = (a[i] == b[j])
				? length[(i + 1) * (n + 1) + (j + 1)] + 1
				: std::max(length[(i + 1) * (n + 1) + j], length[i * (n + 1) + (j + 1)]);
		}
	}
	std::vector<std::string> shared;
	size_t i = 0;
	size_t j = 0;
	while (i < m && j < n)
	{
		if (a[i] == b[j]) { shared.push_back(a[i]); ++i; ++j; }
		else if (length[(i + 1) * (n + 1) + j] >= length[i * (n + 1) + (j + 1)]) { ++i; }
		else { ++j; }
	}
	return shared;
}

// A group derived from obsolescence has no authored name, and taking a member's name
// calls a line of four cannon hardpoints after whichever the player has reached.
// What the members have in common is often a better answer, and being a run of words out of names
// the translators wrote it needs no string of its own. (Obviously this works better for some
// languages than others, but we're trying to intelligently best-effort name things here.)
void nameUnnamedTracks(std::vector<ResearchTrack>& tracks)
{
	// A name shared by two groups is worse than no name, so every candidate is counted
	// before any is taken, and authored names are counted too
	std::map<WzString, int> taken;
	for (const auto& track : tracks)
	{
		if (!track.name.isEmpty()) { taken[track.name]++; }
	}

	std::vector<WzString> candidate(tracks.size());
	for (size_t t = 0; t < tracks.size(); ++t)
	{
		const auto& track = tracks[t];
		if (!track.name.isEmpty() || track.members.size() < 2) { continue; }

		// What the names have in common, where that is more than one word.
		// One word is as likely to be the mounting as the weapon.
		std::vector<std::string> shared = wordsOf(getLocalizedStatsName(&asResearch[track.members.front()]));
		for (size_t m = 1; m < track.members.size() && !shared.empty(); ++m)
		{
			shared = commonWords(shared, wordsOf(getLocalizedStatsName(&asResearch[track.members[m]])));
		}
		if (shared.size() > 1)
		{
			std::string joined = shared.front();
			for (size_t w = 1; w < shared.size(); ++w) { joined += " " + shared[w]; }
			candidate[t] = WzString::fromUtf8(joined);
			continue;
		}

		// Naming the rest by weapon subclass is probably worse than saying nothing.
		// getWeaponSubClassDisplayName() returns the engine's research classes, so the laser
		// line comes out "Energy" and a turret line "A-A" beside a matching "AA Site".
		// Anything left keeps no name (and is ultimately described after one of its members).
	}

	for (const auto& name : candidate)
	{
		if (!name.isEmpty()) { taken[name]++; }
	}
	for (size_t t = 0; t < tracks.size(); ++t)
	{
		// Anything still ambiguous keeps no name, and whoever draws it should name the group
		// after the step that the player has reached
		if (!candidate[t].isEmpty() && taken[candidate[t]] == 1)
		{
			tracks[t].name = candidate[t];
		}
	}
}

// ---------------------------------------------------------------------------
// MARK: - Derivation
// ---------------------------------------------------------------------------

std::vector<ResearchTrack> deriveResearchTracks(const ResearchPrereqClosure& closure)
{
	std::vector<ResearchTrack> tracks;
	std::unordered_set<size_t> claimed;
	const size_t topicCount = asResearch.size();

	const ResearchPrecedes byPrereq = [&closure](size_t below, size_t above) {
		return closure.isPrereq(below, above);
	};

	// Categories first - being authored rather than inferred.
	// The stats loader has already checked each is a chain and put it in order.
	std::map<WzString, std::vector<size_t>> categoriesInOrder(getResearchCategories().begin(), getResearchCategories().end());
	for (const auto& category : categoriesInOrder)
	{
		if (category.second.size() < 2)
		{
			continue;
		}
		emitTracksFromGroup(category.second, byPrereq, TrackSource::Category, category.first, tracks, claimed);
	}

	// Then topics whose results blocks match, which is what an authored category would have probably covered (had it been written).
	std::map<WzString, std::vector<size_t>> bySignature;
	for (size_t i = 0; i < topicCount; ++i)
	{
		if (claimed.count(i) > 0)
		{
			continue;
		}
		const WzString signature = upgradeSignatureOf(asResearch[i]);
		if (!signature.isEmpty())
		{
			bySignature[signature].push_back(i);
		}
	}
	for (const auto& group : bySignature)
	{
		if (group.second.size() < 2)
		{
			continue;
		}
		// The signature is what the group has in common, so a name read out of it is the same for every member
		const WzString name = nameResearchProgression(asResearch[group.second.front()]);
		emitTracksFromGroup(group.second, byPrereq, TrackSource::UpgradeSignature, name, tracks, claimed);
	}

	// Finally the tier lines, which no prerequisite edge describes.
	// Grouped and then ordered by the obsolescence relation.
	std::vector<size_t> parent(topicCount);
	for (size_t i = 0; i < topicCount; ++i)
	{
		parent[i] = i;
	}
	std::function<size_t(size_t)> findRoot = [&parent, &findRoot](size_t i) {
		return parent[i] == i ? i : (parent[i] = findRoot(parent[i]));
	};
	// A topic joins only the closest thing it supersedes, not everything. Grouping by
	// every obsolescence link welds unrelated lines together: one campaign topic makes
	// the assault gun, the heavy flamer, two cyborg weapons and the flashlight laser
	// redundant at once, which would put all of those lines into one progression.
	//
	// Closest is by shared identifier prefix, which is what tells R-Wpn-Laser02 that
	// R-Wpn-Laser01 is its line and the assault gun is not. Not by weapon subclass: a
	// line is meant to change class as it improves, so that severs cannon into gauss
	// and rocket into missile (which are real progressions).
	const auto sharedPrefix = [](const WzString& lhs, const WzString& rhs) {
		const std::string& a = lhs.toStdString();
		const std::string& b = rhs.toStdString();
		size_t n = 0;
		while (n < a.size() && n < b.size() && a[n] == b[n]) { ++n; }
		return n;
	};
	bool anyObsolescence = false;
	for (size_t a = 0; a < topicCount; ++a)
	{
		size_t closest = topicCount;
		size_t bestShared = 0;
		for (size_t b = 0; b < topicCount; ++b)
		{
			if (a == b || !supersedes(asResearch[a], asResearch[b]))
			{
				continue;
			}
			const size_t shared = sharedPrefix(asResearch[a].id, asResearch[b].id);
			// The identifier breaks a tie, so the same data always groups the same way
			// (instead of by whatever order the list is in)
			if (closest == topicCount || shared > bestShared
			    || (shared == bestShared && asResearch[b].id.toStdString() > asResearch[closest].id.toStdString()))
			{
				closest = b;
				bestShared = shared;
			}
		}
		if (closest != topicCount)
		{
			anyObsolescence = true;
			parent[findRoot(a)] = findRoot(closest);
		}
	}

	if (anyObsolescence)
	{
		std::map<size_t, std::vector<size_t>> families;
		for (size_t i = 0; i < topicCount; ++i)
		{
			if (claimed.count(i) == 0)
			{
				families[findRoot(i)].push_back(i);
			}
		}

		// supersedes() is a direct relation, so close it over each family before down-counting.
		// (Families are tiny, so this is a local scan.)
		for (const auto& family : families)
		{
			const auto& members = family.second;
			if (members.size() < 2)
			{
				continue;
			}
			std::unordered_map<size_t, size_t> slotOf;
			for (size_t s = 0; s < members.size(); ++s)
			{
				slotOf[members[s]] = s;
			}
			const size_t k = members.size();
			std::vector<bool> below(k * k, false);
			for (size_t a = 0; a < k; ++a)
			{
				for (size_t b = 0; b < k; ++b)
				{
					if (a != b && supersedes(asResearch[members[a]], asResearch[members[b]]))
					{
						below[a * k + b] = true;	// b is below a
					}
				}
			}
			for (size_t via = 0; via < k; ++via)
			{
				for (size_t a = 0; a < k; ++a)
				{
					if (!below[a * k + via])
					{
						continue;
					}
					for (size_t b = 0; b < k; ++b)
					{
						if (below[via * k + b])
						{
							below[a * k + b] = true;
						}
					}
				}
			}

			const ResearchPrecedes byObsolescence = [&below, &slotOf, k](size_t lower, size_t upper) -> bool {
				const auto lowerSlot = slotOf.find(lower);
				const auto upperSlot = slotOf.find(upper);
				if (lowerSlot == slotOf.end() || upperSlot == slotOf.end())
				{
					return false;
				}
				return below[upperSlot->second * k + lowerSlot->second];
			};
			emitTracksFromGroup(members, byObsolescence, TrackSource::Obsolescence, WzString(), tracks, claimed);
		}
	}

	nameUnnamedTracks(tracks);
	return tracks;
}

WzString researchTechCategoryName(const RESEARCH& research)
{
	switch (mapIconToRID(research.iconID))
	{
	case RID_ROCKET:	return WzString::fromUtf8(_("Rockets"));
	case RID_CANNON:	return WzString::fromUtf8(_("Cannons"));
	case RID_HOVERCRAFT:	return WzString::fromUtf8(_("Hover"));
	case RID_ECM:		return WzString::fromUtf8(_("ECM"));
	case RID_PLASCRETE:	return WzString::fromUtf8(_("Plascrete"));
	case RID_TRACKS:	return WzString::fromUtf8(_("Tracks"));
	case RID_DROIDTECH:	return WzString::fromUtf8(_("Droids"));
	case RID_WEAPONTECH:	return WzString::fromUtf8(_("Weapons"));
	case RID_COMPUTERTECH:	return WzString::fromUtf8(_("Computing"));
	case RID_POWERTECH:	return WzString::fromUtf8(_("Power"));
	case RID_SYSTEMTECH:	return WzString::fromUtf8(_("Systems"));
	case RID_STRUCTURETECH:	return WzString::fromUtf8(_("Structures"));
	case RID_CYBORGTECH:	return WzString::fromUtf8(_("Cyborgs"));
	case RID_DEFENCE:	return WzString::fromUtf8(_("Defenses"));
	default:		return WzString();
	}
}
