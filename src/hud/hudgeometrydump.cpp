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

#include "hudgeometrydump.h"
#include "hudconnection.h"
#include "hudids.h"
#include "hudpagecontrol.h"
#include "hudpanelextension.h"
#include "intel/intelmodel.h"
#include "intel/intelpanel.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/demangle.hpp"
#include "lib/framework/physfs_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"

#include "../console.h"
#include "../hci.h"
#include "../objmem.h"
#include "../droid.h"
#include "../game_world.h"
#include "../screens/guidescreen.h"
#include "../mission.h"
#include "../message.h"
#include "../research.h"
#include "../spectatorwidgets.h"
#include "../hci/teamstrategy.h"
#include "../multigifts.h"
#include "../transporter.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <unordered_map>

// (lib/sdl: the pointer is moved for the states that point at a widget)
void wzWarpMouseToLogicalPos(int logicalX, int logicalY);

namespace
{

struct DumpState
{
	std::string name;
	std::function<bool ()> apply;
};

constexpr int FRAMES_BEFORE_DUMP = 4;
constexpr int FRAME_TO_CLICK = 2;
constexpr int FRAME_TO_HOVER = 3;
constexpr uint32_t HOVER_SETTLE_MS = 900;
constexpr int FRAMES_AFTER_HOVER = 5;
constexpr uint32_t MAX_SCREENSHOT_WAIT_MS = 30000;
constexpr int HIT_MAP_STEP = 2;
constexpr uint32_t DEFAULT_WIDGET_ID = 0xFFFFEEEEu; // (the ID WIDGET() gives a widget without one)

bool armed = false;
bool started = false;
std::string outputPath;
uint32_t startAt = 3000;
std::vector<std::string> stateNames;
size_t stateIndex = 0;
int frameInState = 0;
std::string hoverKey;
std::vector<std::string> clickKeys;
uint32_t hoveredAt = 0;
uint32_t lastStateDoneAt = 0;
std::vector<std::string> screenshots;
nlohmann::json result;

void clearSelection()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
	{
		psDroid->selected = false;
	}
}

bool selectAllDroids()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}
	bool any = false;
	for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
	{
		if (psDroid->died == 0 && !psDroid->isTransporter())
		{
			psDroid->selected = true;
			any = true;
		}
	}
	return any;
}

bool selectFirstDroid()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}
	DROID *first = nullptr;
	for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
	{
		if (psDroid->died == 0 && !psDroid->isTransporter() && (first == nullptr || psDroid->id < first->id))
		{
			first = psDroid;
		}
	}
	if (first == nullptr)
	{
		return false;
	}
	first->selected = true;
	return true;
}

void parkPointer()
{
	wzWarpMouseToLogicalPos(static_cast<int>(screenWidth) / 2, static_cast<int>(screenHeight) / 3);
}

/// A research report for the local player, as research gives it (read: then read and deleted, as the intelligence
/// screen does once it closes)
bool addResearchReport(const std::string &viewName, bool read)
{
	VIEWDATA *viewData = getViewData(WzString::fromUtf8(viewName));
	if (viewData == nullptr || selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}
	MESSAGE *message = addMessage(MSG_RESEARCH, false, selectedPlayer);
	if (message == nullptr)
	{
		return false;
	}
	message->pViewData = viewData;
	hudIntelNoteMessage(message);
	// (Researched, as a report's topic is, without a second report)
	if (const RESEARCH *research = getResearchForMsg(viewData))
	{
		researchResult(research->index, static_cast<UBYTE>(selectedPlayer), false, nullptr, false);
	}
	if (read)
	{
		removeMessage(message, selectedPlayer);
	}
	return true;
}

/// "<player>:<formed|requested|invitation|broken>": the local player's alliance with that player (no message sent)
bool setAllianceForTests(const std::string &setting)
{
	const size_t colon = setting.find(':');
	const unsigned long player = std::strtoul(setting.c_str(), nullptr, 10);
	if (colon == std::string::npos || player >= MAX_PLAYERS || player == selectedPlayer || selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}
	const uint8_t me = static_cast<uint8_t>(selectedPlayer);
	const uint8_t them = static_cast<uint8_t>(player);
	const std::string state = setting.substr(colon + 1);
	breakAlliance(me, them, false, false);
	if (state == "formed")
	{
		formAlliance(me, them, false, false, false);
	}
	else if (state == "requested")
	{
		requestAlliance(me, them, false, false);
	}
	else if (state == "invitation")
	{
		requestAlliance(them, me, false, false);
	}
	else if (state != "broken")
	{
		return false;
	}
	return true;
}

bool applyState(const std::string &name)
{
	clearSelection();
	if (name.find("guide=") == std::string::npos)
	{
		// (closing it takes effect after the frame: a guide state shows its topic in the open guide)
		closeGuideScreen();
	}
	specStatsViewClose();
	intRemoveMissionTimer();
	intRemoveTransporterTimer();
	intRemoveTransporterLaunch();
	intTestShowForm(5);
	parkPointer();
	hoverKey.clear();
	clickKeys.clear();
	hoveredAt = 0;
	hudSetTouchInputForTests(false);
	hudShowConnectionWarningsForTests(false, false);
	bool ok = true;
	size_t start = 0;
	while (start <= name.size())
	{
		const size_t end = std::min(name.find('+', start), name.size());
		const std::string part = name.substr(start, end - start);
		if (part == "none" || part.empty())
		{
		}
		else if (part == "build") { ok = intTestShowForm(0) && ok; }
		else if (part == "manufacture") { ok = intTestShowForm(1) && ok; }
		else if (part == "research") { ok = intTestShowForm(2) && ok; }
		else if (part == "commander") { ok = intTestShowForm(3) && ok; }
		else if (part == "order") { ok = selectFirstDroid() && intTestShowForm(4) && ok; }
		else if (part == "select") { ok = selectFirstDroid() && ok; }
		else if (part == "selectall") { ok = selectAllDroids() && ok; }
		else if (part == "touch") { hudSetTouchInputForTests(true); }
		else if (part == "timers") { ok = missionShowTimerRowForTests(false) && ok; }
		else if (part == "launch") { ok = missionShowTimerRowForTests(true) && ok; }
		else if (part == "netwarn") { hudShowConnectionWarningsForTests(true, false); }
		else if (part == "netwarnopen") { hudShowConnectionWarningsForTests(true, true); }
		else if (part == "stats") { specStatsViewCreate(); ok = specStatsViewShown() && ok; }
		else if (part == "debug=droids") { intOpenDebugMenu(OBJ_DROID); }
		else if (part == "debug=structures") { intOpenDebugMenu(OBJ_STRUCTURE); }
		else if (part == "debug=features") { intOpenDebugMenu(OBJ_FEATURE); }
		else if (part == "flash") { flashMissionButton(IDTIMER_FORM); flashMissionButton(IDTRANS_LAUNCH); }
		else if (part.rfind("guide=", 0) == 0) { addGuideTopic(part.substr(6), ShowTopicFlags::NeverViewed); }
		else if (part == "intel") { addIntelScreen(); ok = hudIntelScreenIsOpen() && ok; }
		else if (part.rfind("report=", 0) == 0) { ok = addResearchReport(part.substr(7), false) && ok; }
		else if (part.rfind("readreport=", 0) == 0) { ok = addResearchReport(part.substr(11), true) && ok; }
		else if (part.rfind("intel.", 0) == 0) { ok = hudIntelScreenApplyForTests(part.substr(6)) && ok; }
		else if (part.rfind("alliance=", 0) == 0) { ok = setAllianceForTests(part.substr(9)) && ok; }
		else if (part.rfind("teamplans=", 0) == 0) { ok = teamStrategyFillForTests(std::strtoul(part.substr(10).c_str(), nullptr, 10)) > 0 && ok; }
		else if (part.rfind("hover=", 0) == 0) { hoverKey = part.substr(6); }
		else if (part.rfind("click=", 0) == 0) { clickKeys.push_back(part.substr(6)); }
		else
		{
			debug(LOG_ERROR, "HUD geometry dump: unknown state %s", part.c_str());
			ok = false;
		}
		start = end + 1;
	}
	return ok;
}

const char *typeName(WIDGET_TYPE type)
{
	switch (type)
	{
	case WIDG_FORM: return "form";
	case WIDG_LABEL: return "label";
	case WIDG_BUTTON: return "button";
	case WIDG_EDITBOX: return "editbox";
	case WIDG_BARGRAPH: return "bargraph";
	case WIDG_SLIDER: return "slider";
	case WIDG_UNSPECIFIED_TYPE: break;
	}
	return "widget";
}

class ScreenWalker
{
public:
	explicit ScreenWalker(std::string prefix): prefix(std::move(prefix)) {}

	void walk(const std::shared_ptr<W_FORM> &root)
	{
		for (auto const &child : root->children())
		{
			walkWidget(child, prefix, false);
		}
	}

	nlohmann::json widgets = nlohmann::json::array();
	std::unordered_map<const WIDGET *, std::string> keys;

private:
	void walkWidget(const std::shared_ptr<WIDGET> &widget, const std::string &namedPath, bool inAddition, bool inReplacedTabs = false)
	{
		// A list's page tabs that an extension shows in their place still count, where they were, so keys don't change
		const bool replacedRoot = !widget->visible() && hudIsReplacedPageTabs(*widget);
		if (!widget->visible() && !replacedRoot)
		{
			return;
		}
		const bool replacedTabs = inReplacedTabs || replacedRoot;
		if (hudIsWrapperId(widget->id))
		{
			addEntry(*widget, namedPath + "/wrapper:" + std::to_string(widget->id - IDHUD_WRAPPER_FIRST), true);
			for (auto const &child : widget->children())
			{
				walkWidget(child, namedPath, inAddition);
			}
			return;
		}
		const bool addition = inAddition || hudIsAdditionId(widget->id);
		std::string key;
		std::string childPath = namedPath;
		if (widget->id != 0 && widget->id != DEFAULT_WIDGET_ID)
		{
			key = namedPath + "/" + std::to_string(widget->id);
			const int duplicate = namedKeys[key]++;
			if (duplicate > 0)
			{
				key += "~" + std::to_string(duplicate);
			}
			childPath = key;
		}
		else
		{
			const std::string counter = namedPath + (addition ? "@" : "#");
			key = counter + std::to_string(ordinals[counter]++);
		}
		addEntry(*widget, key, addition);
		if (replacedTabs)
		{
			widgets.back()["replacedPageTabs"] = true;
		}
		// A panel's close button was its first child in Classic, before its extension moved it: it keeps that key
		auto children = widget->children();
		std::stable_partition(children.begin(), children.end(), [](const std::shared_ptr<WIDGET> &child) {
			return hudIsMovedCloseButton(*child);
		});
		for (auto const &child : children)
		{
			walkWidget(child, childPath, addition, replacedTabs);
		}
	}

	void addEntry(const WIDGET &widget, const std::string &key, bool addition)
	{
		const WzRect rect = widget.screenGeometry();
		nlohmann::json entry;
		entry["key"] = key;
		entry["id"] = widget.id;
		entry["type"] = typeName(widget.type);
		entry["class"] = cxxDemangle(typeid(widget).name());
		entry["rect"] = {rect.x(), rect.y(), rect.width(), rect.height()};
		entry["z"] = static_cast<int>(widgets.size());
		entry["addition"] = addition;
		entry["mouseTransparent"] = widget.transparentToMouse();
		if (hudIsMovedCloseButton(widget))
		{
			entry["movedClose"] = true;
		}
		if (hudIsMovedPanelControl(widget))
		{
			entry["movedControl"] = true;
		}
		if (hudIsSkinPlacedWidget(widget))
		{
			entry["skinPlaced"] = true;
		}
		if (const int inset = hudPanelTopInset(widget))
		{
			entry["topInset"] = inset;
		}
		widgets.push_back(entry);
		keys[&widget] = key;
	}

	std::string prefix;
	std::map<std::string, int> namedKeys;
	std::map<std::string, int> ordinals;
};

std::shared_ptr<WIDGET> findWidget(const std::string &key)
{
	ScreenWalker walker("");
	walker.walk(psWScreen->psForm);
	for (const auto &entry : walker.keys)
	{
		if (entry.second == key)
		{
			return std::const_pointer_cast<WIDGET>(entry.first->shared_from_this());
		}
	}
	return nullptr;
}

bool clickWidget(const std::string &key)
{
	auto widget = findWidget(key);
	if (widget == nullptr)
	{
		debug(LOG_ERROR, "HUD geometry dump: no widget %s to click", key.c_str());
		return false;
	}
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	widget->clicked(&context, WKEY_PRIMARY);
	widget->released(&context, WKEY_PRIMARY);
	return true;
}

bool hoverWidget(const std::string &key)
{
	auto widget = findWidget(key);
	if (widget == nullptr)
	{
		debug(LOG_ERROR, "HUD geometry dump: no widget %s to point at", key.c_str());
		return false;
	}
	const WzRect rect = widget->screenGeometry();
	wzWarpMouseToLogicalPos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
	return true;
}

nlohmann::json dumpState(const std::string &name, bool available)
{
	nlohmann::json state;
	state["name"] = name;
	state["available"] = available;
	state["intMode"] = static_cast<int>(intMode);

	ScreenWalker mainWalker("");
	mainWalker.walk(psWScreen->psForm);
	state["widgets"] = mainWalker.widgets;

	struct Overlay
	{
		std::shared_ptr<W_SCREEN> screen;
		std::unique_ptr<ScreenWalker> walker;
	};
	std::vector<Overlay> overlays;
	nlohmann::json overlayWidgets = nlohmann::json::array();
	widgForEachOverlayScreen([&](const std::shared_ptr<W_SCREEN> &psScreen, uint16_t zOrder) -> bool {
		if (psScreen == nullptr || psScreen->psForm == nullptr)
		{
			return true;
		}
		Overlay overlay;
		overlay.screen = psScreen;
		overlay.walker = std::make_unique<ScreenWalker>("overlay" + std::to_string(zOrder) + ":" + std::to_string(overlays.size()));
		overlay.walker->walk(psScreen->psForm);
		for (auto &widget : overlay.walker->widgets)
		{
			overlayWidgets.push_back(widget);
		}
		overlays.push_back(std::move(overlay));
		return true;
	});
	state["overlayWidgets"] = overlayWidgets;

	std::vector<std::string> hitKeys;
	std::unordered_map<std::string, int> hitKeyIndex;
	auto indexOf = [&](const std::string &key) -> int {
		auto it = hitKeyIndex.find(key);
		if (it != hitKeyIndex.end())
		{
			return it->second;
		}
		const int index = static_cast<int>(hitKeys.size());
		hitKeys.push_back(key);
		hitKeyIndex[key] = index;
		return index;
	};
	auto target = [&](int x, int y) -> int {
		for (auto const &overlay : overlays)
		{
			W_CONTEXT context(0, 0, x, y);
			auto hit = overlay.screen->psForm->findMouseTargetRecursive(&context, WKEY_NONE, false);
			if (hit == overlay.screen->psForm)
			{
				// (A screen's own form takes the click unless it's hidden or transparent, as the widget library does)
				return indexOf("overlay:root");
			}
			if (hit != nullptr)
			{
				auto it = overlay.walker->keys.find(hit.get());
				return indexOf(it != overlay.walker->keys.end() ? it->second : "overlay:?");
			}
		}
		W_CONTEXT context(0, 0, x, y);
		auto hit = psWScreen->psForm->findMouseTargetRecursive(&context, WKEY_NONE, false);
		if (hit == nullptr || hit == psWScreen->psForm)
		{
			return -1;
		}
		auto it = mainWalker.keys.find(hit.get());
		return indexOf(it != mainWalker.keys.end() ? it->second : "?");
	};

	nlohmann::json rows = nlohmann::json::array();
	for (int y = 0; y < static_cast<int>(screenHeight); y += HIT_MAP_STEP)
	{
		nlohmann::json row = nlohmann::json::array();
		int previous = -2;
		for (int x = 0; x < static_cast<int>(screenWidth); x += HIT_MAP_STEP)
		{
			const int hit = target(x, y);
			if (hit != previous)
			{
				row.push_back(x);
				row.push_back(hit);
				previous = hit;
			}
		}
		rows.push_back(row);
	}
	state["hitMap"] = {{"step", HIT_MAP_STEP}, {"keys", hitKeys}, {"rows", rows}};
	return state;
}

std::string screenshotName(const std::string &state)
{
	std::string name = state;
	for (char &c : name)
	{
		if (!isalnum(static_cast<unsigned char>(c)) && c != '-')
		{
			c = '_';
		}
	}
	return "hudtest/" + name + ".png";
}

/// What the intelligence screen would list, at the end of the dump
nlohmann::json dumpIntelligence()
{
	nlohmann::json entries = nlohmann::json::array();
	for (const HudIntelEntry &entry : hudIntelEntries())
	{
		nlohmann::json j;
		j["kind"] = entry.kind == HudIntelKind::Research ? "research" : (entry.kind == HudIntelKind::Update ? "update" : "briefing");
		j["key"] = entry.key;
		j["title"] = entry.title;
		if (!entry.headlineKind.empty())
		{
			j["headlineKind"] = entry.headlineKind;
		}
		if (entry.time.has_value())
		{
			j["time"] = *entry.time;
		}
		if (!entry.level.empty())
		{
			j["level"] = entry.level;
		}
		j["unread"] = entry.unread;
		j["message"] = entry.message != nullptr;
		entries.push_back(std::move(j));
	}
	return entries;
}

void finish()
{
	result["intelligence"] = dumpIntelligence();
	std::ofstream out(outputPath);
	if (!out)
	{
		debug(LOG_ERROR, "HUD geometry dump: can't write %s", outputPath.c_str());
		wzQuit(EXIT_FAILURE);
		return;
	}
	out << result.dump(1);
	out.close();
	debug(LOG_INFO, "HUD geometry dump: wrote %s", outputPath.c_str());
	wzQuit(EXIT_SUCCESS);
}

} // namespace

bool hudSetGeometryDump(const std::string &spec)
{
	std::vector<std::string> parts;
	size_t start = 0;
	while (start <= spec.size())
	{
		const size_t end = std::min(spec.find(',', start), spec.size());
		parts.push_back(spec.substr(start, end - start));
		start = end + 1;
	}
	if (parts.empty() || parts[0].empty())
	{
		return false;
	}
	outputPath = parts[0];
	if (parts.size() > 1 && !parts[1].empty())
	{
		if (parts[1].find_first_not_of("0123456789") != std::string::npos)
		{
			return false;
		}
		startAt = static_cast<uint32_t>(std::stoul(parts[1]));
	}
	stateNames.assign(parts.begin() + std::min<size_t>(parts.size(), 2), parts.end());
	if (stateNames.empty())
	{
		stateNames = {"none", "build", "manufacture", "research", "commander", "order", "timers", "launch"};
	}
	armed = true;
	return true;
}

bool hudGeometryDumpActive()
{
	return armed;
}

void hudGeometryDumpAfterMessages()
{
	if (!armed || started || gameTime < startAt || psWScreen == nullptr)
	{
		return;
	}
	started = true;
	gameTimeStop();
	PHYSFS_mkdir("hudtest");
	result = nlohmann::json::object();
	result["version"] = 1;
	result["screen"] = {{"width", screenWidth}, {"height", screenHeight}, {"displayScale", wzGetCurrentDisplayScale()}};
	result["gameTime"] = gameTime;
	result["states"] = nlohmann::json::array();
	stateIndex = 0;
	frameInState = 0;
	debug(LOG_INFO, "HUD geometry dump: started at gameTime %u", gameTime);
}

void hudGeometryDumpFrame()
{
	if (!started || stateIndex > stateNames.size())
	{
		return;
	}
	if (stateIndex >= stateNames.size())
	{
		const bool allSaved = std::all_of(screenshots.begin(), screenshots.end(), [](const std::string &name) {
			return PHYSFS_exists(name.c_str()) != 0;
		});
		if ((allSaved && realTime - lastStateDoneAt > 1000) || realTime - lastStateDoneAt > MAX_SCREENSHOT_WAIT_MS)
		{
			stateIndex = stateNames.size() + 1;
			finish();
		}
		return;
	}
	// (Console text, ex. a log line with a source line number, is no part of the HUD's look)
	flushConsoleMessages();
	static bool available = false;
	const std::string &name = stateNames[stateIndex];
	if (frameInState == 0)
	{
		available = applyState(name);
	}
	else if (frameInState == FRAME_TO_CLICK && !clickKeys.empty())
	{
		for (const auto &key : clickKeys)
		{
			available = clickWidget(key) && available;
		}
	}
	else if (frameInState == FRAME_TO_HOVER && !hoverKey.empty())
	{
		available = hoverWidget(hoverKey) && available;
		hoveredAt = realTime;
	}
	else if (frameInState >= FRAMES_BEFORE_DUMP && (hoveredAt == 0 || (realTime - hoveredAt >= HOVER_SETTLE_MS && frameInState >= FRAME_TO_HOVER + FRAMES_AFTER_HOVER)))
	{
		result["states"].push_back(dumpState(name, available));
		screenshots.push_back(screenshotName(name));
		PHYSFS_delete(screenshots.back().c_str());
		screenDumpToFile(screenshots.back().c_str());
		debug(LOG_INFO, "HUD geometry dump: state %s (%s)", name.c_str(), available ? "available" : "unavailable");
		++stateIndex;
		frameInState = 0;
		lastStateDoneAt = realTime;
		return;
	}
	++frameInState;
}
