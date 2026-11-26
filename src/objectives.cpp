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

#include "objectives.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"

#include "console.h"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace
{

std::vector<Objective> objectives;   ///< every player's, in the order they were added
std::string title;
uint32_t revision = 0;
ObjectivesDisplay display;

bool concernsLocalPlayer(int32_t player)
{
	return player == OBJECTIVES_ALL_PLAYERS || (selectedPlayer < MAX_PLAYERS && player == static_cast<int32_t>(selectedPlayer));
}

bool displayed()
{
	return display.shows && display.shows();
}

std::string progressText(const Objective &objective)
{
	const auto &progress = objective.options.progress;
	return progress.has_value() ? astringf("%s: %d/%d", objective.text.c_str(), progress->first, progress->second) : objective.text;
}

/// Prints what changed to the console (when the HUD doesn't show the objectives)
void printChange(const Objective *before, const Objective &after)
{
	if (!concernsLocalPlayer(after.options.player) || displayed())
	{
		return;
	}
	if (before == nullptr || before->text != after.text)
	{
		addConsoleMessage(astringf(_("Objective: %s"), progressText(after).c_str()).c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		return;
	}
	if (before->options.state != after.options.state)
	{
		const char *format = after.options.state == ObjectiveState::Done ? _("Objective complete: %s")
			: (after.options.state == ObjectiveState::Failed ? _("Objective failed: %s") : _("Objective: %s"));
		addConsoleMessage(astringf(format, after.text.c_str()).c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		return;
	}
	if (before->options.progress != after.options.progress && after.options.progress.has_value())
	{
		addConsoleMessage(progressText(after).c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
	}
}

bool displayOrder(const Objective &a, const Objective &b)
{
	if (a.options.primary != b.options.primary)
	{
		return a.options.primary;
	}
	if (a.options.order != b.options.order)
	{
		return a.options.order < b.options.order;
	}
	return a.addedAt < b.addedAt;
}

}

void objectivesSet(const std::string &id, const std::string &text, const ObjectiveOptions &options)
{
	const uint32_t now = gameTime;
	auto it = std::find_if(objectives.begin(), objectives.end(), [&](const Objective &o) {
		return o.id == id && o.options.player == options.player;
	});
	Objective objective;
	objective.id = id;
	objective.text = text;
	objective.options = options;
	if (it == objectives.end())
	{
		objective.addedAt = now;
		objective.changedAt = now;
		objective.stateAt = now;
		printChange(nullptr, objective);
		objectives.push_back(std::move(objective));
		++revision;
		return;
	}
	const Objective &before = *it;
	objective.addedAt = before.addedAt;
	const bool visibleChange = before.text != text || before.options.state != options.state || before.options.progress != options.progress;
	objective.changedAt = visibleChange ? now : before.changedAt;
	objective.stateAt = before.options.state != options.state ? now : before.stateAt;
	const bool anyChange = visibleChange || before.options.primary != options.primary || before.options.marker != options.marker
		|| before.options.order != options.order;
	printChange(&before, objective);
	*it = std::move(objective);
	if (anyChange)
	{
		++revision;
	}
}

bool objectivesRemove(const std::string &id, nonstd::optional<int32_t> player)
{
	const size_t count = objectives.size();
	objectives.erase(std::remove_if(objectives.begin(), objectives.end(), [&](const Objective &o) {
		return o.id == id && (!player.has_value() || o.options.player == *player);
	}), objectives.end());
	if (objectives.size() == count)
	{
		return false;
	}
	++revision;
	return true;
}

void objectivesClear(nonstd::optional<int32_t> player)
{
	const size_t count = objectives.size();
	objectives.erase(std::remove_if(objectives.begin(), objectives.end(), [&](const Objective &o) {
		return !player.has_value() || o.options.player == *player;
	}), objectives.end());
	if (objectives.size() != count)
	{
		++revision;
	}
}

void objectivesShow(int32_t player, bool reminder)
{
	if (!concernsLocalPlayer(player))
	{
		return;
	}
	if (displayed())
	{
		if (reminder)
		{
			return;
		}
		if (display.bringToAttention)
		{
			display.bringToAttention();
		}
		return;
	}
	// (What's left to do, as the campaign's reminders printed it)
	for (const Objective &objective : objectivesFor(selectedPlayer))
	{
		if (objective.options.state == ObjectiveState::Active)
		{
			addConsoleMessage(astringf(_("Objective: %s"), progressText(objective).c_str()).c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		}
	}
}

std::vector<Objective> objectivesFor(uint32_t player)
{
	std::vector<Objective> result;
	for (const Objective &objective : objectives)
	{
		if (objective.options.player == OBJECTIVES_ALL_PLAYERS || objective.options.player == static_cast<int32_t>(player))
		{
			result.push_back(objective);
		}
	}
	std::stable_sort(result.begin(), result.end(), displayOrder);
	return result;
}

uint32_t objectivesRevision()
{
	return revision;
}

bool objectivesAny(uint32_t player)
{
	return std::any_of(objectives.begin(), objectives.end(), [player](const Objective &o) {
		return o.options.player == OBJECTIVES_ALL_PLAYERS || o.options.player == static_cast<int32_t>(player);
	});
}

void objectivesSetDisplay(ObjectivesDisplay newDisplay)
{
	display = std::move(newDisplay);
}

const char *objectiveStateName(ObjectiveState state)
{
	switch (state)
	{
	case ObjectiveState::Active: return "active";
	case ObjectiveState::Done: return "done";
	case ObjectiveState::Failed: return "failed";
	}
	return "active";
}

nonstd::optional<ObjectiveState> objectiveStateFromName(const std::string &name)
{
	for (ObjectiveState state : {ObjectiveState::Active, ObjectiveState::Done, ObjectiveState::Failed})
	{
		if (name == objectiveStateName(state))
		{
			return state;
		}
	}
	return nonstd::nullopt;
}

void objectivesSetTitle(const std::string &newTitle)
{
	if (title != newTitle)
	{
		title = newTitle;
		++revision;
	}
}

const std::string &objectivesTitle()
{
	return title;
}

void objectivesReset()
{
	if (!objectives.empty() || !title.empty())
	{
		objectives.clear();
		title.clear();
		++revision;
	}
}

nlohmann::ordered_json objectivesWrite()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = OBJECTIVES_SECTION_VERSION;
	nlohmann::ordered_json list = nlohmann::ordered_json::array();
	for (const Objective &objective : objectives)
	{
		nlohmann::ordered_json o = nlohmann::ordered_json::object();
		o["id"] = objective.id;
		o["text"] = objective.text;
		o["state"] = objectiveStateName(objective.options.state);
		o["primary"] = objective.options.primary;
		if (objective.options.progress.has_value())
		{
			o["progress"] = {objective.options.progress->first, objective.options.progress->second};
		}
		if (!objective.options.marker.empty())
		{
			o["marker"] = objective.options.marker;
		}
		o["order"] = objective.options.order;
		o["player"] = objective.options.player;
		o["addedAt"] = objective.addedAt;
		o["changedAt"] = objective.changedAt;
		o["stateAt"] = objective.stateAt;
		list.push_back(std::move(o));
	}
	j["objectives"] = std::move(list);
	if (!title.empty())
	{
		j["title"] = title;
	}
	return j;
}

void objectivesRead(const nlohmann::ordered_json &j)
{
	objectives.clear();
	title.clear();
	++revision;
	if (!j.is_object() || !j.contains("objectives") || !j.at("objectives").is_array())
	{
		return;
	}
	title = j.value("title", std::string());
	for (const auto &o : j.at("objectives"))
	{
		Objective objective;
		objective.id = o.value("id", std::string());
		objective.text = o.value("text", std::string());
		objective.options.state = objectiveStateFromName(o.value("state", std::string("active"))).value_or(ObjectiveState::Active);
		objective.options.primary = o.value("primary", true);
		if (o.contains("progress") && o.at("progress").is_array() && o.at("progress").size() == 2)
		{
			objective.options.progress = std::make_pair(o.at("progress").at(0).get<int32_t>(), o.at("progress").at(1).get<int32_t>());
		}
		objective.options.marker = o.value("marker", std::string());
		objective.options.order = o.value("order", 0);
		objective.options.player = o.value("player", OBJECTIVES_ALL_PLAYERS);
		objective.addedAt = o.value("addedAt", 0u);
		objective.changedAt = o.value("changedAt", objective.addedAt);
		objective.stateAt = o.value("stateAt", objective.addedAt);
		if (!objective.id.empty())
		{
			objectives.push_back(std::move(objective));
		}
	}
}
