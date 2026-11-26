// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "researchdetail.h"
#include "researcheffecttext.h"

#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/scrollablelist.h"

#include "../../../research.h"
#include "../../../multiplay.h"
#include "../../../mapdisplay.h"
#include "../../../structure.h"
#include "../../../game_world.h"
#include "../../../intimage.h"
#include "../../../messagedef.h"
#include "../../../researchtree/researchtracks.h"

#include <algorithm>

static constexpr int PADDING = DetailPanelContents::PADDING;
static constexpr int MODEL_SIZE = DetailPanelContents::MODEL_SIZE;
static constexpr int COMPACT_MODEL_SIZE = 48;
static constexpr int BADGE_GAP = 5;

ResearchDetailSubject researchDetailSubjectForPlayer(uint16_t researchIndex, uint32_t player)
{
	ResearchDetailSubject topic;
	topic.researchIndex = researchIndex;
	if (player >= MAX_PLAYERS || researchIndex >= asResearch.size() || researchIndex >= asPlayerResList[player].size())
	{
		return topic;
	}
	const PLAYER_RESEARCH *state = &asPlayerResList[player][researchIndex];
	if (IsResearchCompleted(state))
	{
		topic.state = NodeState::Researched;
	}
	else if (IsResearchDisabled(state))
	{
		topic.state = NodeState::Disabled;
	}
	else if (IsResearchStartedPending(state) && !IsResearchCancelledPending(state))
	{
		topic.state = NodeState::InProgress;
	}
	else if (researchAvailable(researchIndex, player, ModeQueue))
	{
		topic.state = NodeState::Available;
	}
	const RESEARCH &research = asResearch[researchIndex];
	if (!research.category.isEmpty() && research.categoryMax > 1)
	{
		topic.stepText = WzString::format(_("step %u of %u"), static_cast<unsigned>(research.categoryProgress), static_cast<unsigned>(research.categoryMax));
	}
	topic.prerequisiteDone = [player](uint16_t prereqIndex) {
		return prereqIndex < asPlayerResList[player].size() && IsResearchCompleted(&asPlayerResList[player][prereqIndex]);
	};
	return topic;
}

// "+N more" after the first few of a list that would not fit
template <typename Entries, typename Name>
static void addListEntries(const std::shared_ptr<Paragraph>& paragraph, const Entries& entries, size_t limit, Name&& name)
{
	size_t shown = 0;
	size_t total = 0;
	for (const auto& entry : entries)
	{
		WzString text = name(entry);
		if (text.isEmpty())
		{
			continue;
		}
		++total;
		if (shown < limit)
		{
			paragraph->addText(text + "\n");
			++shown;
		}
	}
	if (total > shown)
	{
		paragraph->addText(WzString::fromUtf8(astringf(_("+%u more"), static_cast<unsigned>(total - shown))) + "\n");
	}
}

// Combined output of every research facility the player has running
static int playerResearchRate(uint32_t player)
{
	if (player >= MAX_PLAYERS)
	{
		return 0;
	}
	int rate = 0;
	for (const STRUCTURE *psStruct : gameWorld.objects.structures[player])
	{
		if (psStruct->pStructureType->type == REF_RESEARCH && psStruct->status == SS_BUILT)
		{
			rate += getBuildingResearchPoints(psStruct);
		}
	}
	return rate;
}

static WzString formatDuration(uint32_t seconds)
{
	return WzString::format("%u:%02u", seconds / 60, seconds % 60);
}

// A panel with nothing to offer returns nullptr.
static std::shared_ptr<WIDGET> makeActionRow(const ResearchDetailActions& actions)
{
	if (!actions.onTracePath && !actions.onResearch)
	{
		return nullptr;
	}
	auto row = makeDetailPanelActionRow();

	if (actions.onResearch)
	{
		auto research = makeDetailPanelActionButton(_("Research"), _("Start this in a research facility"));
		row->attach(research);
		const auto onResearch = actions.onResearch;
		research->addOnClickHandler([onResearch](W_BUTTON& button) {
			onResearch(button.shared_from_this());
		});
	}

	if (actions.onTracePath)
	{
		auto path = makeDetailPanelQuietButton(actions.tracingPath ? _("Whole tree") : _("Trace path"),
		                                       actions.tracingPath ? _("Show the whole tree again") : _("Show only what this topic needs"));
		row->attach(path);
		const auto onTracePath = actions.onTracePath;
		path->addOnClickHandler([onTracePath](W_BUTTON&) {
			onTracePath();
		});
	}
	return row;
}

std::shared_ptr<DetailPanelContents> makeUndiscoveredResearchDetailContents(int32_t width)
{
	auto unknown = std::make_shared<W_LABEL>();
	unknown->setFont(font_regular, WZCOL_TEXT_MEDIUM);
	unknown->setString(_("Undiscovered"));
	unknown->setGeometry(0, 0, width, iV_GetTextLineSize(font_regular));
	return DetailPanelContents::make(nullptr, unknown, nullptr, nullptr, nullptr, {}, nullptr, nullptr, width);
}

std::shared_ptr<DetailPanelContents> makeResearchTopicDetailContents(const ResearchDetailSubject& topic, const ResearchTreeContext& context, int32_t width, int32_t maxWidth, const ResearchDetailActions& actions, const ResearchDetailOptions& options)
{
	const uint16_t subject = topic.researchIndex;
	if (subject >= asResearch.size())
	{
		return nullptr;
	}
	const RESEARCH& research = asResearch[subject];
	const NodeState state = topic.state;

	auto title = makeDetailPanelTitle(WzString::fromUtf8(getLocalizedStatsName(&research)));

	auto subtitle = std::make_shared<W_LABEL>();
	subtitle->setFont(font_small, pal_RGBA(150, 190, 235, 255));

	WzString groupName = topic.groupName;
	if (groupName.isEmpty())
	{
		groupName = research.category;
	}

	WzString subtitleText = groupName.isEmpty() ? researchTechCategoryName(research) : WzString();
	if (!topic.stepText.isEmpty())
	{
		subtitleText = subtitleText.isEmpty() ? topic.stepText
			: (subtitleText + WzString::fromUtf8(" - ") + topic.stepText);
	}
	subtitle->setString(subtitleText);
	subtitle->setCanTruncate(true);

	WzString standing;
	PIELIGHT standingFill = pal_RGBA(40, 40, 40, 255);
	PIELIGHT standingTextColor = pal_Colour(190, 200, 215);
	switch (state)
	{
	case NodeState::Researched:
		standing = WzString::fromUtf8(_("Researched"));
		standingFill = pal_RGBA(26, 56, 32, 255);
		standingTextColor = pal_Colour(150, 220, 165);
		break;
	case NodeState::InProgress:
		standing = WzString::fromUtf8(_("Being researched"));
		standingFill = pal_RGBA(70, 58, 18, 255);
		standingTextColor = pal_Colour(245, 205, 120);
		break;
	case NodeState::Available:
		standing = WzString::fromUtf8(_("Available now"));
		standingFill = pal_RGBA(20, 58, 84, 255);
		standingTextColor = pal_Colour(110, 210, 255);
		break;
	case NodeState::Disabled:
		standing = WzString::fromUtf8(_("Not in this game"));
		standingFill = pal_RGBA(52, 28, 28, 255);
		standingTextColor = pal_Colour(225, 150, 150);
		break;
	default:
		standing = WzString::fromUtf8(_("Not yet available"));
		break;
	}

	if (!context.aggregate.empty() && state != NodeState::Disabled)
	{
		const uint32_t held = researchHeldCount(context, subject);
		const uint32_t members = static_cast<uint32_t>(context.aggregate.size());
		if (held == 0)
		{
			standing = WzString::fromUtf8(astringf(_("Held by none of %u"), static_cast<unsigned>(members)));
			standingFill = pal_RGBA(40, 40, 40, 255);
			standingTextColor = pal_Colour(190, 200, 215);
		}
		else if (held >= members)
		{
			standing = WzString::fromUtf8(astringf(_("Held by all %u"), static_cast<unsigned>(members)));
		}
		else
		{
			standing = WzString::fromUtf8(astringf(_("Held by %u of %u"), static_cast<unsigned>(held), static_cast<unsigned>(members)));
			standingFill = pal_RGBA(33, 50, 43, 255);
			standingTextColor = pal_Colour(120, 180, 150);
		}
	}

	if (!options.routineStatus && (state == NodeState::Available || state == NodeState::InProgress))
	{
		standing = WzString();
	}

	auto onIt = alliesResearching(context, subject);
	if (state == NodeState::Researched)
	{
		onIt.clear();
	}
	bool viewerHasIt = (state == NodeState::Researched);
	if (!context.aggregate.empty())
	{
		viewerHasIt = context.player < MAX_PLAYERS && subject < asPlayerResList[context.player].size()
			&& IsResearchCompleted(&asPlayerResList[context.player][subject]);
	}

	// Stats (cost, time, etc)
	std::vector<std::shared_ptr<WIDGET>> stats;
	if (!viewerHasIt && state != NodeState::Disabled)
	{
		stats.push_back(makeDetailPanelStat(WzString::fromUtf8(_("Cost")),
		                                    WzString::format("%u", research.researchPower),
		                                    IMAGE_DES_POWER, _("Power to start it")));
		stats.push_back(makeDetailPanelStat(WzString::fromUtf8(_("Research")),
		                                    WzString::format(_("%u pts"), static_cast<unsigned>(research.researchPoints)),
		                                    nullopt, _("Research points to finish it")));
		const int rate = playerResearchRate(context.player);
		if (rate > 0 && context.source == ResearchTreeContext::Source::LivePlayerState)
		{
			uint32_t remaining = research.researchPoints;
			if (context.player < MAX_PLAYERS && subject < asPlayerResList[context.player].size())
			{
				const uint32_t already = asPlayerResList[context.player][subject].currentPoints;
				remaining = (already < remaining) ? remaining - already : 0;
			}
			stats.push_back(makeDetailPanelStat(WzString::fromUtf8(_("Time")),
			                                    WzString::fromUtf8("≈ ") + formatDuration(remaining / static_cast<uint32_t>(rate)),
			                                    nullopt,
			                                    astringf(_("At the %d points a second the labs make now"), rate)));
		}
	}

	std::shared_ptr<WIDGET> status;
	if (!onIt.empty())
	{
		status = makeDetailPanelStatusLine(standing, standingTextColor,
		                                   WzString::fromUtf8(_("Researching this now:")), onIt);
	}
	else if (!standing.isEmpty() && stats.empty())
	{
		status = makeDetailPanelBadge(standing, standingFill, standingTextColor);
	}
	else if (!standing.isEmpty())
	{
		auto line = std::make_shared<W_LABEL>();
		line->setFont(font_small, standingTextColor);
		line->setString(standing);
		line->setCanTruncate(true);
		status = std::move(line);
	}

	// Which progression a topic sits in (if it has one)
	std::shared_ptr<WIDGET> group;
	if (!groupName.isEmpty())
	{
		group = makeDetailPanelBadge(groupName, pal_RGBA(40, 40, 40, 255), WZCOL_FORM_TEXT);
	}

	const int modelSize = options.small ? 0 : (options.compact ? COMPACT_MODEL_SIZE : MODEL_SIZE);
	// The title and the badges beside the model do not wrap, and a long progression name next to a
	// long status runs off the end of the header without this.
	{
		const int32_t asked = width;
		const int textX = PADDING + (modelSize > 0 ? modelSize + PADDING : 0);
		// A status shown as a bar spans the panel rather than sharing the header row, so
		// what it wants bounds the panel and not that row
		const bool statusIsBar = !stats.empty();
		int badges = (statusIsBar || !status) ? 0 : status->idealWidth();
		if (group)
		{
			badges += (badges > 0 ? BADGE_GAP : 0) + group->idealWidth();
		}
		width = std::max(width, textX + std::max(title->idealWidth(), badges) + PADDING);
		if (statusIsBar && status)
		{
			width = std::max(width, PADDING + status->idealWidth() + PADDING);
		}
		width = std::min(width, std::max(maxWidth, asked));
	}

	auto paragraph = makeDetailPanelParagraph(width - PADDING * 2);

	const auto held = researchHeldBy(context, subject);
	if (!held.empty())
	{
		WzString names;
		bool viewerAmong = false;
		for (const auto member : held)
		{
			if (!names.isEmpty())
			{
				names += WzString::fromUtf8(", ");
			}
			names += WzString::fromUtf8(getPlayerName(member));
			viewerAmong = viewerAmong || member == context.player;
		}
		const bool aboutTheViewer = context.player == selectedPlayer;
		const bool leftOut = aboutTheViewer && !viewerAmong;
		paragraph->setFontColour(leftOut ? pal_Colour(110, 210, 255) : WZCOL_TEXT_MEDIUM);
		paragraph->addText(leftOut
			? WzString::format(_("Held by %s, not by you"), names.toUtf8().c_str()) + "\n"
			: WzString::format(_("Held by: %s"), names.toUtf8().c_str()) + "\n");
		paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	}

	// Flag duplicated research (in shared research mode)
	const auto allies = alliesResearching(context, subject);
	if (!allies.empty())
	{
		const auto duplicated = duplicatedResearch(context);
		if (std::find(duplicated.begin(), duplicated.end(), subject) != duplicated.end())
		{
			paragraph->setFontColour(pal_Colour(240, 120, 70));
			paragraph->addText(WzString::fromUtf8(_("Doubled up - a second lab on this researches it no faster")) + "\n");
			paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
		}
	}

	// The authored description that the intelligence screen already shows for this topic
	if (!options.compact && research.pViewData != nullptr && !research.pViewData->textMsg.empty())
	{
		paragraph->addText("\n");
		for (const auto& line : research.pViewData->textMsg)
		{
			paragraph->addText(line + "\n");
		}
	}

	// What it actually changes
	const auto effects = describeResearchEffects(research);
	if (!effects.empty())
	{
		detailPanelAddSection(paragraph, WzString::fromUtf8(_("Effects")));
		for (const auto& effect : effects)
		{
			paragraph->setFontColour(effect.isBuff ? detailPanelBuffColor() : detailPanelNerfColor());
			paragraph->addText(effect.text + "\n");
		}
		paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
	}

	// Prereqs, as a checklist
	std::vector<std::pair<WzString, bool>> prereqs;
	for (const auto prereqIndex : research.pPRList)
	{
		if (prereqIndex >= asResearch.size())
		{
			continue;
		}
		const bool done = topic.prerequisiteDone && topic.prerequisiteDone(prereqIndex);
		prereqs.push_back({WzString::fromUtf8(getLocalizedStatsName(&asResearch[prereqIndex])), done});
	}
	std::shared_ptr<WIDGET> checklist;
	if (options.requirements && !prereqs.empty())
	{
		checklist = makeDetailPanelChecklist(WzString::fromUtf8(_("Requires")), std::move(prereqs));
	}

	// Resume the running text after the checklist widget
	auto after = makeDetailPanelParagraph(width - PADDING * 2);

	if (options.compact)
	{
		std::vector<WzString> unlocks;
		for (const auto *component : research.componentResults)
		{
			unlocks.push_back(component != nullptr ? WzString::fromUtf8(getLocalizedStatsName(component)) : WzString());
		}
		for (const auto structIndex : research.pStructureResults)
		{
			unlocks.push_back(structIndex < numStructureStats ? WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])) : WzString());
		}
		if (!unlocks.empty())
		{
			detailPanelAddSection(after, WzString::fromUtf8(_("Unlocks")));
			addListEntries(after, unlocks, options.maxListEntries, [](const WzString& name) { return name; });
		}
	}
	else if (!research.componentResults.empty() || !research.pStructureResults.empty())
	{
		detailPanelAddSection(after, WzString::fromUtf8(_("Unlocks")));
		for (const auto *component : research.componentResults)
		{
			if (component != nullptr)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(component)) + "\n");
			}
		}
		for (const auto structIndex : research.pStructureResults)
		{
			if (structIndex < numStructureStats)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])) + "\n");
			}
		}
	}

	if (!options.compact && (!research.pRedArtefacts.empty() || !research.pRedStructs.empty()))
	{
		detailPanelAddSection(after, WzString::fromUtf8(_("Replaces")));
		for (const auto *component : research.pRedArtefacts)
		{
			if (component != nullptr)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(component)) + "\n");
			}
		}
		for (const auto structIndex : research.pRedStructs)
		{
			if (structIndex < numStructureStats)
			{
				after->addText(WzString::fromUtf8(getLocalizedStatsName(&asStructureStats[structIndex])) + "\n");
			}
		}
	}

	if (!checklist)
	{
		// (a paragraph only lays its text out when its width changes, and the list reads its height. The checklist
		// otherwise brings that about)
		for (const auto &text : {paragraph, after})
		{
			text->setGeometry(0, 0, text->width() - 1, text->height());
			text->setGeometry(0, 0, text->width() + 1, text->height());
		}
	}
	auto body = ScrollableListWidget::make();
	body->addItem(paragraph);
	if (checklist)
	{
		body->addItem(checklist);
	}
	body->addItem(after);

	std::shared_ptr<WIDGET> model;
	if (modelSize > 0)
	{
		model = makeDetailPanelModel([subject](int x0, int y0, int modelWidth, int modelHeight) {
			if (subject < asResearch.size())
			{
				renderResearchFittedToBuffer(&asResearch[subject], x0 + modelWidth / 2, y0 + modelHeight / 2,
				                             static_cast<UDWORD>(std::min(modelWidth, modelHeight)));
			}
		});
	}
	return DetailPanelContents::make(std::move(model), std::move(title), std::move(subtitle), std::move(group), std::move(status), std::move(stats), std::move(body), makeActionRow(actions), width, options.style, DetailStatusPlacement::Automatic, modelSize);
}
