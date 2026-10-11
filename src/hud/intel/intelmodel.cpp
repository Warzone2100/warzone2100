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

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "intelmodel.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"

#include "../../levels.h"
#include "../../message.h"
#include "../../messagedef.h"
#include "../../research.h"
#include "../../statsdef.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>

namespace
{

constexpr uint32_t LOG_VERSION = 1;

struct LogItem
{
	HudIntelKind kind = HudIntelKind::Research;
	nonstd::optional<uint32_t> time;
	std::string level;
	size_t order = 0;   ///< arrival order, for items of the same time
};

std::unordered_map<std::string, LogItem> logItems;
std::unordered_map<std::string, std::string> levelTitles;
std::unordered_set<std::string> levelsTitledByBriefing;
size_t nextOrder = 0;

const char *kindName(HudIntelKind kind)
{
	switch (kind)
	{
	case HudIntelKind::Research: return "research";
	case HudIntelKind::Update: return "update";
	case HudIntelKind::Briefing: return "briefing";
	}
	return "research";
}

nonstd::optional<HudIntelKind> kindFromName(const std::string &name)
{
	for (HudIntelKind kind : {HudIntelKind::Research, HudIntelKind::Update, HudIntelKind::Briefing})
	{
		if (name == kindName(kind))
		{
			return kind;
		}
	}
	return nonstd::nullopt;
}

nonstd::optional<HudIntelKind> kindOf(const MESSAGE &message)
{
	switch (message.type)
	{
	case MSG_RESEARCH: return HudIntelKind::Research;
	case MSG_CAMPAIGN: return HudIntelKind::Update;
	case MSG_MISSION: return HudIntelKind::Briefing;
	default: return nonstd::nullopt;
	}
}

bool listed(const MESSAGE &message)
{
	return message.pViewData != nullptr && message.pViewData->type != VIEW_BEACON && kindOf(message).has_value();
}

const RESEARCH *researchFor(const VIEWDATA *viewData)
{
	if (viewData == nullptr)
	{
		return nullptr;
	}
	for (const RESEARCH &research : asResearch)
	{
		if (research.pViewData == viewData)
		{
			return &research;
		}
	}
	return nullptr;
}

std::string lowered(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

/// A briefing's lines: those of each of its sequences that has any, else the view's own
std::vector<const std::vector<WzString> *> briefingLines(const VIEWDATA &viewData)
{
	std::vector<const std::vector<WzString> *> parts;
	if ((viewData.type == VIEW_RPL || viewData.type == VIEW_RPLX) && viewData.pData != nullptr)
	{
		const auto *replay = static_cast<const VIEW_REPLAY *>(viewData.pData);
		for (const SEQ_DISPLAY &seq : replay->seqList)
		{
			if (!seq.textMsg.empty())
			{
				parts.push_back(&seq.textMsg);
			}
		}
	}
	if (parts.empty() && !viewData.textMsg.empty())
	{
		parts.push_back(&viewData.textMsg);
	}
	return parts;
}

/// Whether a sequence's first line names the briefing: a kind without lowercase letters and a title
/// ("TRANSPORT MISSION: Transport Down")
bool isHeadline(const std::string &line)
{
	const size_t colon = line.find(": ");
	if (colon == std::string::npos || colon == 0 || colon + 2 >= line.size())
	{
		return false;
	}
	const auto lowercase = [](unsigned char c) { return std::islower(c) != 0; };
	return std::none_of(line.begin(), line.begin() + static_cast<std::ptrdiff_t>(colon), lowercase);
}

void describeBriefing(HudIntelEntry &entry, const VIEWDATA &viewData)
{
	std::string search;
	for (const std::vector<WzString> *lines : briefingLines(viewData))
	{
		if (entry.headline.empty() && isHeadline(lines->front().toUtf8()))
		{
			entry.headline = lines->front().toUtf8();
		}
		for (const WzString &line : *lines)
		{
			search += line.toUtf8();
			search += '\n';
		}
	}
	if (!entry.headline.empty())
	{
		const size_t colon = entry.headline.find(": ");
		entry.headlineKind = entry.headline.substr(0, colon);
		entry.title = entry.headline.substr(colon + 2);
	}
	else
	{
		entry.title = (entry.kind == HudIntelKind::Update) ? _("Update") : _("Mission briefing");
	}
	entry.searchText = lowered(search);
}

void describeResearch(HudIntelEntry &entry, const VIEWDATA *viewData)
{
	std::string search;
	if (entry.research != nullptr)
	{
		entry.title = getLocalizedStatsName(entry.research);
		search = entry.title + '\n';
	}
	if (viewData != nullptr)
	{
		for (const WzString &line : viewData->textMsg)
		{
			search += line.toUtf8();
			search += '\n';
		}
		if (entry.title.empty())
		{
			entry.title = viewData->name.toUtf8();
		}
	}
	entry.searchText = lowered(search);
}

}

std::vector<HudIntelEntry> hudIntelEntries()
{
	std::vector<HudIntelEntry> entries;
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return entries;
	}
	std::unordered_map<std::string, size_t> byKey;
	size_t engineOrder = 0;
	std::vector<size_t> orders;
	for (MESSAGE *message : apsMessages[selectedPlayer])
	{
		if (message == nullptr || !listed(*message))
		{
			continue;
		}
		const std::string key = message->pViewData->name.toUtf8();
		auto known = byKey.find(key);
		if (known != byKey.end())
		{
			// The same report twice (ex. a topic given again): the unread one counts
			HudIntelEntry &entry = entries[known->second];
			entry.unread = entry.unread || !message->read;
			continue;
		}
		HudIntelEntry entry;
		entry.kind = *kindOf(*message);
		entry.key = key;
		entry.message = message;
		entry.unread = !message->read;
		if (entry.kind == HudIntelKind::Research)
		{
			entry.research = researchFor(message->pViewData);
			describeResearch(entry, message->pViewData);
		}
		else
		{
			describeBriefing(entry, *message->pViewData);
		}
		size_t order = SIZE_MAX / 2 + engineOrder++;
		auto logged = logItems.find(key);
		if (logged != logItems.end())
		{
			entry.time = logged->second.time;
			entry.level = logged->second.level;
			order = logged->second.order;
		}
		byKey.emplace(key, entries.size());
		entries.push_back(std::move(entry));
		orders.push_back(order);
	}
	// The reports read since (the engine deleted their messages)
	for (const auto &item : logItems)
	{
		if (item.second.kind != HudIntelKind::Research || byKey.count(item.first) != 0)
		{
			continue;
		}
		const VIEWDATA *viewData = getViewData(WzString::fromUtf8(item.first));
		const RESEARCH *research = researchFor(viewData);
		if (research == nullptr)
		{
			continue;
		}
		HudIntelEntry entry;
		entry.kind = HudIntelKind::Research;
		entry.key = item.first;
		entry.research = research;
		entry.time = item.second.time;
		entry.level = item.second.level;
		describeResearch(entry, viewData);
		byKey.emplace(item.first, entries.size());
		entries.push_back(std::move(entry));
		orders.push_back(item.second.order);
	}

	std::vector<size_t> index(entries.size());
	for (size_t i = 0; i < index.size(); ++i)
	{
		index[i] = i;
	}
	std::stable_sort(index.begin(), index.end(), [&](size_t a, size_t b) {
		const HudIntelEntry &ea = entries[a];
		const HudIntelEntry &eb = entries[b];
		if (ea.time.has_value() != eb.time.has_value())
		{
			return ea.time.has_value();
		}
		if (ea.time.has_value() && *ea.time != *eb.time)
		{
			return *ea.time > *eb.time;
		}
		return ea.time.has_value() ? orders[a] > orders[b] : orders[a] < orders[b];
	});
	std::vector<HudIntelEntry> sorted;
	sorted.reserve(entries.size());
	for (size_t i : index)
	{
		sorted.push_back(std::move(entries[i]));
	}
	return sorted;
}

size_t hudIntelUnreadCount()
{
	size_t count = 0;
	for (const HudIntelEntry &entry : hudIntelEntries())
	{
		count += entry.unread ? 1 : 0;
	}
	return count;
}

void hudIntelNoteMessage(const MESSAGE *message)
{
	if (message == nullptr || message->player != selectedPlayer || !listed(*message))
	{
		return;
	}
	const std::string key = message->pViewData->name.toUtf8();
	const HudIntelKind kind = *kindOf(*message);
	auto existing = logItems.find(key);
	// A report keeps when it first came, and a briefing given again (a later level's) is new
	if (existing != logItems.end() && kind == HudIntelKind::Research)
	{
		return;
	}
	LogItem &item = logItems[key];
	item.kind = kind;
	item.time = gameTime;
	item.level = hudIntelCurrentLevel();
	item.order = nextOrder++;
	// A mission briefing names its level, else the first update
	const bool briefing = kind == HudIntelKind::Briefing;
	if (kind != HudIntelKind::Research && !item.level.empty() && levelsTitledByBriefing.count(item.level) == 0
	    && (briefing || levelTitles.count(item.level) == 0))
	{
		HudIntelEntry entry;
		describeBriefing(entry, *message->pViewData);
		if (!entry.headline.empty())
		{
			levelTitles[item.level] = entry.title;
			if (briefing)
			{
				levelsTitledByBriefing.insert(item.level);
			}
		}
	}
}

std::string hudIntelLevelTitle(const std::string &level)
{
	auto it = levelTitles.find(level);
	return it != levelTitles.end() ? it->second : std::string();
}

std::string hudIntelCurrentLevel()
{
	const char *level = getLevelName();
	return level != nullptr ? level : "";
}

void hudIntelReset()
{
	logItems.clear();
	levelTitles.clear();
	levelsTitledByBriefing.clear();
	nextOrder = 0;
}

nlohmann::ordered_json hudIntelSaveLog()
{
	std::vector<std::pair<std::string, const LogItem *>> items;
	for (const auto &item : logItems)
	{
		items.emplace_back(item.first, &item.second);
	}
	std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.second->order < b.second->order; });
	nlohmann::ordered_json list = nlohmann::ordered_json::array();
	for (const auto &item : items)
	{
		nlohmann::ordered_json j = nlohmann::ordered_json::object();
		j["key"] = item.first;
		j["kind"] = kindName(item.second->kind);
		if (item.second->time.has_value())
		{
			j["time"] = *item.second->time;
		}
		if (!item.second->level.empty())
		{
			j["level"] = item.second->level;
		}
		list.push_back(std::move(j));
	}
	nlohmann::ordered_json log = nlohmann::ordered_json::object();
	log["version"] = LOG_VERSION;
	log["items"] = std::move(list);
	nlohmann::ordered_json levels = nlohmann::ordered_json::object();
	for (const auto &title : levelTitles)
	{
		levels[title.first] = title.second;
	}
	log["levels"] = std::move(levels);
	return log;
}

void hudIntelRestoreLog(const nlohmann::ordered_json &log)
{
	hudIntelReset();
	if (!log.is_object() || log.value("version", 0u) != LOG_VERSION || !log.contains("items") || !log.at("items").is_array())
	{
		return;
	}
	for (const nlohmann::ordered_json &j : log.at("items"))
	{
		if (!j.is_object() || !j.contains("key") || !j.at("key").is_string())
		{
			continue;
		}
		const nonstd::optional<HudIntelKind> kind = kindFromName(j.value("kind", std::string()));
		if (!kind.has_value())
		{
			continue;
		}
		LogItem item;
		item.kind = *kind;
		if (j.contains("time") && j.at("time").is_number_unsigned())
		{
			item.time = j.at("time").get<uint32_t>();
		}
		item.level = j.value("level", std::string());
		item.order = nextOrder++;
		logItems[j.at("key").get<std::string>()] = std::move(item);
	}
	if (log.contains("levels") && log.at("levels").is_object())
	{
		for (const auto &level : log.at("levels").items())
		{
			if (level.value().is_string())
			{
				levelTitles[level.key()] = level.value().get<std::string>();
			}
		}
	}
}
