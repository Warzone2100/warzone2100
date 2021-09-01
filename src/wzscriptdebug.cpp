/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/**
 * @file wzscriptdebug.cpp
 *
 * New scripting system - debug GUI
 */

#include "wzscriptdebug.h"

#if (defined(WZ_OS_WIN) && defined(WZ_CC_MINGW))
#  if (defined(vsprintf) && !defined(_GL_STDIO_H) && defined(_LIBINTL_H))
// On mingw / MXE builds, libintl's define of vsprintf breaks string_cast.hpp
// So undef it here and restore it later (HACK)
#    define _wz_restore_libintl_vsprintf
#    undef vsprintf
#  endif
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

#if defined(_wz_restore_libintl_vsprintf)
#  undef _wz_restore_libintl_vsprintf
#  undef vsprintf
#  define vsprintf libintl_vsprintf
#endif

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/netplay/netplay.h"

#include "lib/widget/button.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/editbox.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/table.h"
#include "lib/widget/jsontable.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "intdisplay.h"

#include "action.h"
#include "difficulty.h"
#include "multiplay.h"
#include "objects.h"
#include "power.h"
#include "hci.h"
#include "display.h"
#include "keybind.h"
#include "loop.h"
#include "mission.h"
#include "message.h"
#include "transporter.h"
#include "template.h"
#include "multiint.h"
#include "challenge.h"

#include "wzapi.h"
#include "qtscript.h"

#include <numeric>
#include <algorithm>
#include <limits>

static std::shared_ptr<W_SCREEN> debugScreen = nullptr;
static std::shared_ptr<WZScriptDebugger> globalDialog = nullptr;
static jsDebugShutdownHandlerFunction globalDialogShutdownHandler;

#define SCRIPTDEBUG_WINDOW_HEIGHT_MAX 700
#define SCRIPTDEBUG_BUTTON_HEIGHT 24
#define SCRIPTDEBUG_ROW_HEIGHT 24
#define ACTION_BUTTON_ROW_SPACING 10
#define ACTION_BUTTON_SPACING 10

#define TITLE_TOP_PADDING 5
#define TITLE_HEIGHT 24
#define TITLE_BOTTOM (TITLE_TOP_PADDING + TITLE_HEIGHT)
#define TAB_BUTTONS_HEIGHT 24
#define TAB_BUTTONS_PADDING 10

const PIELIGHT WZCOL_DEBUG_FILL_COLOR = pal_RGBA(25, 0, 110, 220);
const PIELIGHT WZCOL_DEBUG_FILL_COLOR_DARK = pal_RGBA(10, 0, 70, 250);
const PIELIGHT WZCOL_DEBUG_BORDER_LIGHT = pal_RGBA(255, 255, 255, 80);
const PIELIGHT WZCOL_DEBUG_INDETERMINATE_PROGRESS_COLOR = pal_RGBA(240, 240, 255, 180);

// MARK: - Filling various models

static const std::vector<WzString> view_type = { "RES", "RPL", "PROX", "RPLX", "BEACON" };

struct RowDataModel
{
public:
	RowDataModel(size_t numberOfColumns)
	{
		m_currentMaxColumnWidths.resize(numberOfColumns, 0);
	}

	std::shared_ptr<TableRow> newRow(const std::vector<WzString>& columnTexts, int rowHeight = 0, bool skipCalculatingColumnWidth = false)
	{
		std::vector<std::shared_ptr<WIDGET>> columnWidgets;
		for (size_t i = 0; i < columnTexts.size(); i++)
		{
			columnWidgets.push_back(newColumnLabel(i, columnTexts[i], skipCalculatingColumnWidth));
		}
		m_rows.push_back(TableRow::make(columnWidgets, rowHeight));
		return m_rows.back();
	}

	const std::vector<std::shared_ptr<TableRow>>& rows() const { return m_rows; }
	const std::vector<size_t> currentMaxColumnWidths() const { return m_currentMaxColumnWidths; }

private:
	std::shared_ptr<W_LABEL> newColumnLabel(size_t colIdx, const WzString& text, bool skipCalculatingColumnWidth = false)
	{
		auto newLabel = std::make_shared<W_LABEL>();
		newLabel->setFont(font_regular, WZCOL_FORM_LIGHT);
		newLabel->setString(text);
		newLabel->setCanTruncate(true);
		newLabel->setTransparentToClicks(true);
		if (!skipCalculatingColumnWidth)
		{
			m_currentMaxColumnWidths[colIdx] = std::max(m_currentMaxColumnWidths[colIdx], static_cast<size_t>(std::max<int>(newLabel->getMaxLineWidth(), 0)));
		}
		return newLabel;
	}
private:
	std::vector<std::shared_ptr<TableRow>> m_rows;
	std::vector<size_t> m_currentMaxColumnWidths;
};

static RowDataModel fillViewdataModel()
{
	RowDataModel result(3);
	// Name, Type, Source
	std::vector<WzString> keys = getViewDataKeys();
	for (const WzString& key : keys)
	{
		VIEWDATA *ptr = getViewData(key);
		ASSERT(ptr->type < view_type.size(), "Bad viewdata type");

		result.newRow({key, view_type.at(ptr->type), ptr->fileName}, SCRIPTDEBUG_ROW_HEIGHT, true);
	}
	return result;
}

static RowDataModel fillMessageModel()
{
	const std::vector<WzString> msg_type = { "RESEARCH", "CAMPAIGN", "MISSION", "PROXIMITY" };
	const std::vector<WzString> msg_data_type = { "DEFAULT", "BEACON" };
	const std::vector<WzString> obj_type = { "DROID", "STRUCTURE", "FEATURE", "PROJECTILE", "TARGET" };

	RowDataModel result(6);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		for (const MESSAGE *psCurr = apsMessages[i]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			ASSERT(psCurr->type < msg_type.size(), "Bad message type");
			ASSERT(psCurr->dataType < msg_data_type.size(), "Bad viewdata type");
			std::vector<WzString> columnTexts;
			columnTexts.push_back(WzString::number(psCurr->id));
			columnTexts.push_back(msg_type.at(psCurr->type));
			columnTexts.push_back(msg_data_type.at(psCurr->dataType));
			columnTexts.push_back(WzString::number(psCurr->player));
			ASSERT(!psCurr->pViewData || !psCurr->psObj, "Both viewdata and object in message should be impossible!");
			if (psCurr->pViewData)
			{
				ASSERT(psCurr->pViewData->type < view_type.size(), "Bad viewdata type");
				columnTexts.push_back(psCurr->pViewData->name);
				columnTexts.push_back(view_type.at(psCurr->pViewData->type));
			}
			else if (psCurr->psObj)
			{
				columnTexts.push_back((objInfo(psCurr->psObj)));
				columnTexts.push_back(obj_type.at(psCurr->psObj->type));
			}
			else
			{
				columnTexts.push_back("");
				columnTexts.push_back("");
			}
			result.newRow(columnTexts, SCRIPTDEBUG_ROW_HEIGHT);
		}
	}
	return result;
}

static RowDataModel fillTriggersModel(const std::vector<scripting_engine::timerNodeSnapshot>& trigger_snapshot, wzapi::scripting_instance *context)
{
	RowDataModel result(7);
	for (const auto &node : trigger_snapshot)
	{
		if (node.instance != context)
		{
			continue;
		}
		std::vector<WzString> columnTexts;
		columnTexts.push_back(WzString::number(node.timerID));
		columnTexts.push_back(WzString::fromUtf8(node.timerName));
		if (node.baseobj >= 0)
		{
			columnTexts.push_back(WzString::number(node.baseobj));
		}
		else
		{
			columnTexts.push_back("-");
		}
		columnTexts.push_back(WzString::number(node.frameTime));
		columnTexts.push_back(WzString::number(node.ms));
		if (node.type == TIMER_ONESHOT_READY)
		{
			columnTexts.push_back("Oneshot");
		}
		else if (node.type == TIMER_ONESHOT_DONE)
		{
			columnTexts.push_back("Done");
		}
		else
		{
			columnTexts.push_back("Repeat");
		}
		columnTexts.push_back(WzString::number(node.calls));
		result.newRow(columnTexts, SCRIPTDEBUG_ROW_HEIGHT);
	}
	return result;
}

static nlohmann::ordered_json fillMainModel()
{
	const std::vector<std::string> lev_type = {
								   "LDS_COMPLETE", "LDS_CAMPAIGN", "LDS_CAMSTART", "LDS_CAMCHANGE",
	                               "LDS_EXPAND", "LDS_BETWEEN", "LDS_MKEEP", "LDS_MCLEAR",
	                               "LDS_EXPAND_LIMBO", "LDS_MKEEP_LIMBO", "LDS_NONE",
	                               "LDS_MULTI_TYPE_START", "CAMPAIGN", "", "SKIRMISH", "", "", "",
	                               "MULTI_SKIRMISH2", "MULTI_SKIRMISH3", "MULTI_SKIRMISH4" };
	const std::vector<std::string> difficulty_type = { "EASY", "NORMAL", "HARD", "INSANE" };
	nlohmann::ordered_json result = nlohmann::ordered_json::object();

	int8_t gameType = static_cast<int8_t>(game.type);
	int8_t missionType = static_cast<int8_t>(mission.type);
	ASSERT(gameType < lev_type.size() && gameType != 13 && gameType != 15 && gameType != 16 && gameType != 17, "Bad LEVEL_TYPE for game.type");
	result["game.type"] = lev_type.at(gameType);
	result["game.scavengers"] = game.scavengers;
	result["game.map"] = game.map;
	result["game.maxPlayers"] = game.maxPlayers;
	result["transporterGetLaunchTime"] = transporterGetLaunchTime();
	result["missionIsOffworld"] = missionIsOffworld();
	result["missionCanReEnforce"] = missionCanReEnforce();
	result["missionForReInforcements"] = missionForReInforcements();
	ASSERT(missionType < lev_type.size() && missionType != 13 && missionType != 15 && missionType != 16 && missionType != 17, "Bad LEVEL_TYPE for mission.type");
	result["mission.type"] = lev_type.at(missionType);
	result["getDroidsToSafetyFlag"] = getDroidsToSafetyFlag();
	result["scavengerSlot"] = scavengerSlot();
	result["scavengerPlayer"] = scavengerPlayer();
	result["bMultiPlayer"] = bMultiPlayer;
	result["challenge"] = challengeActive;
	ASSERT(getDifficultyLevel() < difficulty_type.size(), "Bad DIFFICULTY_LEVEL");
	result["difficultyLevel"] = difficulty_type.at(getDifficultyLevel());
	result["loopPieCount"] = loopPieCount;
	result["loopPolyCount"] = loopPolyCount;
	result["allowDesign"] = allowDesign;
	result["includeRedundantDesigns"] = includeRedundantDesigns;

	int features;
	int droids;
	int structures;
	objCount(&droids, &structures, &features);
	result["No. droids"] = droids;
	result["No. structures"] = structures;
	result["No. features"] = features;

	return result;
}

static nlohmann::ordered_json fillPlayerModel(int i)
{
	nlohmann::ordered_json result = nlohmann::ordered_json::object();
	result["ingame.skScores score"] = ingame.skScores[i][0];
	result["ingame.skScores kills"] = ingame.skScores[i][1];
	result["NetPlay.players.name"] = NetPlay.players[i].name;
	result["NetPlay.players.position"] = NetPlay.players[i].position;
	result["NetPlay.players.colour"] = NetPlay.players[i].colour;
	result["NetPlay.players.allocated"] = NetPlay.players[i].allocated;
	result["NetPlay.players.team"] = NetPlay.players[i].team;
	result["NetPlay.players.ai"] = NetPlay.players[i].ai;
	result["NetPlay.players.difficulty"] = static_cast<int8_t>(NetPlay.players[i].difficulty);
	result["NetPlay.players.autoGame"] = NetPlay.players[i].autoGame;
	result["NetPlay.players.IPtextAddress"] = NetPlay.players[i].IPtextAddress;
	result["NetPlay.players.isSpectator"] = NetPlay.players[i].isSpectator;
	result["Current power"] = getPower(i);
	result["Extracted power"] = getExtractedPower(i);
	result["Wasted power"] = getWastedPower(i);
	return result;
}

// MARK: - componentToString

static const char *getObjType(const BASE_OBJECT *psObj)
{
	switch (psObj->type)
	{
	case OBJ_DROID: return "Droid";
	case OBJ_STRUCTURE: return "Structure";
	case OBJ_FEATURE: return "Feature";
	case OBJ_PROJECTILE: return "Projectile";
	default: break;
	}
	return "Unknown";
}

template<typename T>
static std::string arrayToString(const T *array, int length)
{
	WzString result;
	for (int i = 0; i < length; i++)
	{
		if (!result.isEmpty())
		{
			result.append(", ");
		}
		result.append(WzString::number(array[i]));
	}
	return result.toStdString();
}

// Using ^ to denote stats that are in templates, and as such do not change.
// Using : to denote stats that come from structure specializations.
nlohmann::ordered_json componentToString(const COMPONENT_STATS *psStats, int player)
{
	nlohmann::ordered_json key = nlohmann::ordered_json::object();

	key["Name"] = getStatsName(psStats);
	key["^Id"] = psStats->id.toUtf8();
	key["^Power"] = psStats->buildPower;
	key["^Build Points"] = psStats->buildPoints;
	key["^Weight"] = psStats->weight;
	key["^Hit points"] = psStats->getUpgrade(player).hitpoints;
	key["^Hit points +% of total"] = psStats->getUpgrade(player).hitpointPct;
	key["^Designable"] = psStats->designable;
	switch (psStats->compType)
	{
	case COMP_BODY:
		{
			const BODY_STATS *psBody = (const BODY_STATS *)psStats;
			key["^Size"] = psBody->size;
			key["^Max weapons"] = psBody->weaponSlots;
			key["^Body class"] = psBody->bodyClass.toUtf8();
			break;
		}
	case COMP_PROPULSION:
		{
			const PROPULSION_STATS *psProp = (const PROPULSION_STATS *)psStats;
			key["^Hit points +% of body"] = psProp->upgrade[player].hitpointPctOfBody;
			key["^Max speed"] = psProp->maxSpeed;
			key["^Propulsion type"] = psProp->propulsionType;
			key["^Turn speed"] = psProp->turnSpeed;
			key["^Spin speed"] = psProp->spinSpeed;
			key["^Spin angle"] = psProp->spinAngle;
			key["^Skid decelaration"] = psProp->skidDeceleration;
			key["^Deceleration"] = psProp->deceleration;
			key["^Acceleration"] = psProp->acceleration;
			break;
		}
	case COMP_BRAIN:
		{
			const BRAIN_STATS *psBrain = (const BRAIN_STATS *)psStats;
			std::string ranks;
			for (const std::string &s : psBrain->rankNames)
			{
				if (!ranks.empty())
				{
					ranks += ", ";
				}
				ranks += s;
			}
			std::string thresholds;
			for (int t : psBrain->upgrade[player].rankThresholds)
			{
				if (!thresholds.empty())
				{
					thresholds += ", ";
				}
				thresholds += WzString::number(t).toUtf8();
			}
			key["^Base command limit"] = psBrain->upgrade[player].maxDroids;
			key["^Extra command limit by level"] = psBrain->upgrade[player].maxDroidsMult;
			key["^Rank names"] = ranks;
			key["^Rank thresholds"] = thresholds;
			break;
		}
	case COMP_REPAIRUNIT:
		{
			const REPAIR_STATS *psRepair = (const REPAIR_STATS *)psStats;
			key["^Repair time"] = psRepair->time;
			key["^Base repair points"] = psRepair->upgrade[player].repairPoints;
			break;
		}
	case COMP_ECM:
		{
			const ECM_STATS *psECM = (const ECM_STATS *)psStats;
			key["^Base range"] = psECM->upgrade[player].range;
			break;
		}
	case COMP_SENSOR:
		{
			const SENSOR_STATS *psSensor = (const SENSOR_STATS *)psStats;
			key["^Sensor type"] = psSensor->type;
			key["^Base range"] = psSensor->upgrade[player].range;
			break;
		}
	case COMP_CONSTRUCT:
		{
			const CONSTRUCT_STATS *psCon = (const CONSTRUCT_STATS *)psStats;
			key["^Base construct points"] = psCon->upgrade[player].constructPoints;
			break;
		}
	case COMP_WEAPON:
		{
			const WEAPON_STATS *psWeap = (const WEAPON_STATS *)psStats;
			key["Max range"] = psWeap->upgrade[player].maxRange;
			key["Min range"] = psWeap->upgrade[player].minRange;
			key["Radius"] = psWeap->upgrade[player].radius;
			key["Number of Rounds"] = psWeap->upgrade[player].numRounds;
			key["Damage"] = psWeap->upgrade[player].damage;
			key["Minimum damage"] = psWeap->upgrade[player].minimumDamage;
			key["Periodical damage"] = psWeap->upgrade[player].periodicalDamage;
			key["Periodical damage radius"] = psWeap->upgrade[player].periodicalDamageRadius;
			key["Periodical damage time"] = psWeap->upgrade[player].periodicalDamageTime;
			key["Radius damage"] = psWeap->upgrade[player].radiusDamage;
			key["Reload time"] = psWeap->upgrade[player].reloadTime;
			key["Hit chance"] = psWeap->upgrade[player].hitChance;
			key["Short hit chance"] = psWeap->upgrade[player].shortHitChance;
			key["Short range"] = psWeap->upgrade[player].shortRange;
			break;
		}
	case COMP_NUMCOMPONENTS:
		ASSERT_OR_RETURN(key, "%s", "Invalid component: COMP_NUMCOMPONENTS!");
		break;
		// no "default" case because modern compiler plus "-Werror"
		// will raise error if a switch is forgotten
	}
	return key;
}

// MARK: - NoBackgroundTabWidget

class NoBackgroundTabWidget : public MultichoiceWidget
{
public:
	NoBackgroundTabWidget(int value = -1) : MultichoiceWidget(value) { }
	virtual void display(int xOffset, int yOffset) override { }
};

// MARK: - Debug buttons

struct TabButtonDisplayCache
{
	WzText text;
};

static void TabButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using TabButtonDisplayFunc must have its pUserData initialized to a (TabButtonDisplayCache*)
	assert(psWidget->pUserData != nullptr);
	TabButtonDisplayCache& cache = *static_cast<TabButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;
	int x1 = x0 + psButton->width();
	int y1 = y0 + psButton->height();

	bool haveText = !psButton->pText.isEmpty();

	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((psButton->getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto light_border = WZCOL_DEBUG_BORDER_LIGHT;
	auto fill_color = isDown || isDisabled ? WZCOL_DEBUG_FILL_COLOR_DARK : WZCOL_DEBUG_FILL_COLOR;
	iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0,0,0,0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, fill_color);
	if (isHighlight)
	{
		iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
	}

	if (haveText)
	{
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fw = cache.text.width();
		int fx = x0 + (psButton->width() - fw) / 2;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		cache.text.render(fx, fy, textColor);
	}
}

static std::shared_ptr<W_BUTTON> makeDebugButton(const char* text)
{
	auto button = std::make_shared<W_BUTTON>();
	button->setString(text);
	button->FontID = font_regular;
	button->displayFunction = TabButtonDisplayFunc;
	button->pUserData = new TabButtonDisplayCache();
	button->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<TabButtonDisplayCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
	button->setGeometry(0, 0, minButtonWidthForText + TAB_BUTTONS_PADDING, TAB_BUTTONS_HEIGHT);
	return button;
}

// MARK: - WzMainPanel

class WzMainPanel : public W_FORM
{
public:
	WzMainPanel(bool readOnly)
	: W_FORM()
	, readOnly(readOnly)
	{}
	~WzMainPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		playersDropdown->callCalcLayout();
		powerUpdateButton->callCalcLayout();
		powerEditField->callCalcLayout();
		aiAttachButton->callCalcLayout();
		aiPlayerDropdown->callCalcLayout();
		aiDropdown->callCalcLayout();
		table->callCalcLayout();
	}
public:
	static std::shared_ptr<WzMainPanel> make(bool readOnly)
	{
		auto panel = std::make_shared<WzMainPanel>(readOnly);

		std::shared_ptr<W_BUTTON> previousRowButton;
		previousRowButton = panel->createButton(0, "Add droids", [](){ intOpenDebugMenu(OBJ_DROID); }, nullptr, true);
		previousRowButton = panel->createButton(0, "Add structures", [](){ intOpenDebugMenu(OBJ_STRUCTURE); }, previousRowButton, true);
		previousRowButton = panel->createButton(0, "Add features", [](){ intOpenDebugMenu(OBJ_FEATURE); }, previousRowButton, true);

		previousRowButton = panel->createButton(1, "Research all", kf_FinishAllResearch, nullptr, true);
		previousRowButton = panel->createButton(1, "Show sensors", kf_ToggleSensorDisplay, previousRowButton, false);
		previousRowButton = panel->createButton(1, "Shadows", [](){ setDrawShadows(!getDrawShadows()); }, previousRowButton, false);
		previousRowButton = panel->createButton(1, "Fog", kf_ToggleFog, previousRowButton, false);

		previousRowButton = panel->createButton(2, "Show gateways", kf_ToggleShowGateways, nullptr, false);
		previousRowButton = panel->createButton(2, "Reveal all", kf_ToggleGodMode, previousRowButton, true);
		previousRowButton = panel->createButton(2, "Weather", kf_ToggleWeather, previousRowButton, false);
		previousRowButton = panel->createButton(2, "Reveal mode", kf_ToggleVisibility, previousRowButton, true);

		int bottomOfButtonRows = previousRowButton->y() + previousRowButton->height() + ACTION_BUTTON_ROW_SPACING;

		// selected player
		auto selectedPlayerLabel = std::make_shared<W_LABEL>();
		panel->attach(selectedPlayerLabel);
		selectedPlayerLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		selectedPlayerLabel->setString("Selected Player:");
		selectedPlayerLabel->setGeometry(0, bottomOfButtonRows, selectedPlayerLabel->getMaxLineWidth(), std::max<int>(iV_GetTextLineSize(font_regular), TAB_BUTTONS_HEIGHT));

		// Add player chooser drop-down
		panel->playersDropdown = std::make_shared<DropdownWidget>();
		panel->attach(panel->playersDropdown);
		panel->playersDropdown->setListHeight(TAB_BUTTONS_HEIGHT * 5);

		int maxButtonTextWidth = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			WzString buttonLabel = WzString::number(i);
			auto button = makeDebugButton(buttonLabel.toUtf8().c_str());
			button->UserData = static_cast<UDWORD>(i);
			int minButtonTextWidth = button->width();
			maxButtonTextWidth = std::max<int>(minButtonTextWidth, maxButtonTextWidth);
			panel->playersDropdown->addItem(button);
		}
		if (selectedPlayer < MAX_PLAYERS)
		{
			panel->playersDropdown->setSelectedIndex(selectedPlayer);
		}
		panel->playersDropdown->setOnChange([](DropdownWidget& dropdown) {
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(dropdown.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			if (auto selectedIndex = dropdown.getSelectedIndex())
			{
				psParent->playerButtonClicked(static_cast<int>(selectedIndex.value()));
			}
		});
		int contextDropdownX0 = selectedPlayerLabel->x() + selectedPlayerLabel->width() + ACTION_BUTTON_SPACING;
		panel->playersDropdown->setCalcLayout([contextDropdownX0, bottomOfButtonRows](WIDGET *psWidget) {
			auto psParent = psWidget->parent();
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(contextDropdownX0, bottomOfButtonRows, psParent->width() - contextDropdownX0 - ACTION_BUTTON_SPACING, TAB_BUTTONS_HEIGHT);
		});

		int bottomOfSelectedPlayerRow = panel->playersDropdown->y() + panel->playersDropdown->height() + ACTION_BUTTON_ROW_SPACING;

		// Power input field
		// "Power:" label
		panel->powerLabel = std::make_shared<W_LABEL>();
		panel->attach(panel->powerLabel);
		panel->powerLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		panel->powerLabel->setString("Power:");
		panel->powerLabel->setGeometry(0, bottomOfSelectedPlayerRow, panel->powerLabel->getMaxLineWidth(), std::max<int>(iV_GetTextLineSize(font_regular), TAB_BUTTONS_HEIGHT));

		// Add "Set Power"
		panel->powerUpdateButton = makeDebugButton("Set Power");
		panel->powerUpdateButton->setGeometry(panel->powerUpdateButton->x(), panel->powerUpdateButton->y(), panel->powerUpdateButton->width(), panel->powerUpdateButton->height());
		panel->powerUpdateButton->setTip("Set Selected Player Power");
		panel->attach(panel->powerUpdateButton);
		panel->powerUpdateButton->addOnClickHandler([](W_BUTTON& button) {
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(button.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			// get the value of the field, and attempt to convert to an int32_t
			auto powerString = psParent->powerEditField->getString();
			int powerValue = 0;
			try {
				powerValue = std::stoi(powerString.toStdString());
			}
			catch (const std::exception&) {
				debug(LOG_ERROR, "Invalid power value (not convertable to an integer - use numbers only): %s", powerString.toUtf8().c_str());
				return;
			}
			auto selectedPlayerCopy = selectedPlayer;
			widgScheduleTask([selectedPlayerCopy, powerValue](){
				setPower(selectedPlayerCopy, powerValue);
			});
		});
		panel->powerUpdateButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->width() - psWidget->width();
			int bottomOfSelectedPlayerRow = psParent->playersDropdown->y() + psParent->playersDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(x0, bottomOfSelectedPlayerRow, psWidget->width(), psWidget->height());
		}));
		if (readOnly)
		{
			panel->powerUpdateButton->setState(WBUT_DISABLE);
		}

		// Power input edit box
		panel->powerEditField = std::make_shared<W_EDITBOX>();
		panel->attach(panel->powerEditField);
		panel->powerEditField->setGeometry(panel->powerEditField->x(), panel->powerEditField->y(), panel->powerEditField->width(), TAB_BUTTONS_HEIGHT);
		panel->powerEditField->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->powerLabel->x() + psParent->powerLabel->width() + ACTION_BUTTON_SPACING;
			int bottomOfSelectedPlayerRow = psParent->playersDropdown->y() + psParent->playersDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			int y0 = bottomOfSelectedPlayerRow + TAB_BUTTONS_HEIGHT - psWidget->height();
			int width = psParent->powerUpdateButton->x() - ACTION_BUTTON_SPACING - x0;
			psWidget->setGeometry(x0, y0, width, psWidget->height());
		}));
		WzString currPower = "0";
		if (selectedPlayer < MAX_PLAYERS)
		{
			currPower = WzString::number(getPower(selectedPlayer));
		}
		panel->powerEditField->setString(currPower);
		panel->powerEditField->setMaxStringSize(9); // shorten maximum length
		panel->powerEditField->setBoxColours(WZCOL_DEBUG_FILL_COLOR_DARK, WZCOL_DEBUG_BORDER_LIGHT, WZCOL_DEBUG_FILL_COLOR);
		if (readOnly)
		{
			panel->powerEditField->setState(WEDBS_DISABLE);
		}

		int bottomOfPowerRow = panel->powerEditField->y() + panel->powerEditField->height() + ACTION_BUTTON_ROW_SPACING;

		// Attach script to player
		panel->attachAItoPlayerLabel = std::make_shared<W_LABEL>();
		panel->attach(panel->attachAItoPlayerLabel);
		panel->attachAItoPlayerLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		panel->attachAItoPlayerLabel->setString("Attach AI to player:");
		panel->attachAItoPlayerLabel->setGeometry(0, bottomOfPowerRow, panel->attachAItoPlayerLabel->getMaxLineWidth(), std::max<int>(iV_GetTextLineSize(font_regular), TAB_BUTTONS_HEIGHT));

		// "Attach" button (right-aligned in form)
		panel->aiAttachButton = makeDebugButton("Attach");
		panel->attach(panel->aiAttachButton);
		panel->aiAttachButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->width() - psWidget->width();
			int bottomOfPowerRow = psParent->powerEditField->y() + psParent->powerEditField->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(x0, bottomOfPowerRow, psWidget->width(), psWidget->height());
		}));
		panel->aiAttachButton->addOnClickHandler([](W_BUTTON& button){
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(button.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			auto selectedAiButton = psParent->aiDropdown->getSelectedItem();
			ASSERT_OR_RETURN(, selectedAiButton != nullptr, "No selected AI?");
			auto selectedAiPlayerButton = psParent->aiPlayerDropdown->getSelectedItem();
			ASSERT_OR_RETURN(, selectedAiPlayerButton != nullptr, "No selected AI player?");
			const WzString script = selectedAiButton->getString();
			const int player = static_cast<int>(selectedAiPlayerButton->UserData);
			jsAutogameSpecific(WzString::fromUtf8("multiplay/skirmish/") + script, player);
			debug(LOG_INFO, "Script attached - close and reopen debug window to see its context");
		});
		if (readOnly)
		{
			panel->aiAttachButton->setState(WBUT_DISABLE);
		}

		// AI players drop-down
		panel->aiPlayerDropdown = std::make_shared<DropdownWidget>();
		panel->attach(panel->aiPlayerDropdown);
		panel->aiPlayerDropdown->setListHeight(TAB_BUTTONS_HEIGHT * static_cast<uint32_t>(std::min<uint8_t>(game.maxPlayers, 5)));

		maxButtonTextWidth = 0;
		for (int i = 0; i < game.maxPlayers; i++)
		{
			WzString buttonLabel = WzString::number(i);
			auto button = makeDebugButton(buttonLabel.toUtf8().c_str());
			button->UserData = static_cast<UDWORD>(i);
			maxButtonTextWidth = std::max<int>(button->width(), maxButtonTextWidth);
			panel->aiPlayerDropdown->addItem(button);
		}
		panel->aiPlayerDropdown->setSelectedIndex(0);
		panel->aiPlayerDropdown->setCalcLayout([maxButtonTextWidth](WIDGET *psWidget) {
			auto pAiPlayerDropdown = static_cast<DropdownWidget *>(psWidget);
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int width = maxButtonTextWidth + pAiPlayerDropdown->getScrollbarWidth();
			int x0 = psParent->aiAttachButton->x() - ACTION_BUTTON_SPACING - width;
			int bottomOfPowerRow = psParent->powerEditField->y() + psParent->powerEditField->height() + ACTION_BUTTON_ROW_SPACING;
			pAiPlayerDropdown->setGeometry(x0, bottomOfPowerRow, width, TAB_BUTTONS_HEIGHT);
		});

		// AI names dropdown
		const std::vector<WzString> AIs = getAINames();
		panel->aiDropdown = std::make_shared<DropdownWidget>();
		panel->attach(panel->aiDropdown);
		panel->aiDropdown->setListHeight(TAB_BUTTONS_HEIGHT * 5);
		maxButtonTextWidth = 0;
		for (const WzString &name : AIs)
		{
			auto button = makeDebugButton(name.toUtf8().c_str());
			maxButtonTextWidth = std::max<int>(button->width(), maxButtonTextWidth);
			panel->aiDropdown->addItem(button);
		}
		panel->aiDropdown->setSelectedIndex(0);
		panel->aiDropdown->setCalcLayout([](WIDGET *psWidget) {
			auto pAiDropdown = static_cast<DropdownWidget *>(psWidget);
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->attachAItoPlayerLabel->x() + psParent->attachAItoPlayerLabel->width() + ACTION_BUTTON_SPACING;
			int bottomOfPowerRow = psParent->powerEditField->y() + psParent->powerEditField->height() + ACTION_BUTTON_ROW_SPACING;
			int fillWidth = psParent->aiPlayerDropdown->x() - ACTION_BUTTON_SPACING - x0;
			pAiDropdown->setGeometry(x0, bottomOfPowerRow, fillWidth, TAB_BUTTONS_HEIGHT);
		});

		// JSONTable for main game state / info / model
		panel->table = JSONTableWidget::make("Game Details:");
		panel->attach(panel->table);
		panel->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int y0 = psParent->aiDropdown->y() + psParent->aiDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);
		}));
		panel->table->updateData(fillMainModel());
		panel->table->setUpdateButtonFunc([](JSONTableWidget& tableWidget){
			auto psParent = std::dynamic_pointer_cast<WzMainPanel>(tableWidget.parent());
			if (psParent == nullptr)
			{
				return;
			}
			psParent->table->updateData(fillMainModel(), true);
		}, 3 * GAME_TICKS_PER_SEC);

		return panel;
	}
private:
	std::shared_ptr<W_BUTTON> createButton(int row, const std::string &text, const std::function<void ()>& onClickFunc, const std::shared_ptr<W_BUTTON>& previousButton = nullptr, bool requiresWriteAccess = true)
	{
		auto button = makeDebugButton(text.c_str());
		button->setGeometry(button->x(), button->y(), button->width() + 10, button->height());
		attach(button);
		button->addOnClickHandler([onClickFunc](W_BUTTON& button) {
			widgScheduleTask([onClickFunc](){
				onClickFunc();
			});
		});
		if (requiresWriteAccess && readOnly)
		{
			button->setState(WBUT_DISABLE);
		}

		int previousButtonRight = (previousButton) ? previousButton->x() + previousButton->width() : 0;
		button->move((previousButtonRight > 0) ? previousButtonRight + ACTION_BUTTON_SPACING : 0, (row * (button->height() + ACTION_BUTTON_ROW_SPACING)));

		return button;
	}
	void playerButtonClicked(int value)
	{
		if (readOnly)
		{
			debug(LOG_ERROR, "Unable to change selectedPlayer - readOnly mode");
			if (selectedPlayer < MAX_PLAYERS)
			{
				playersDropdown->setSelectedIndex(selectedPlayer);
			}
			return;
		}
		ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Invalid selectedPlayer: %" PRIu32 "", selectedPlayer);
		// Do not change realSelectedPlayer here, so game doesn't pause.
		const int oldSelectedPlayer = selectedPlayer;
		selectedPlayer = value;
		NetPlay.players[selectedPlayer].allocated = !NetPlay.players[selectedPlayer].allocated;
		NetPlay.players[oldSelectedPlayer].allocated = !NetPlay.players[oldSelectedPlayer].allocated;
	}
public:
	std::shared_ptr<DropdownWidget> playersDropdown;
	std::shared_ptr<W_LABEL> powerLabel;
	std::shared_ptr<W_EDITBOX> powerEditField;
	std::shared_ptr<W_BUTTON> powerUpdateButton;
	std::shared_ptr<W_LABEL> attachAItoPlayerLabel;
	std::shared_ptr<DropdownWidget> aiDropdown;
	std::shared_ptr<DropdownWidget> aiPlayerDropdown;
	std::shared_ptr<W_BUTTON> aiAttachButton;
	std::shared_ptr<JSONTableWidget> table;
	bool readOnly = false;
};

// MARK: - WzScriptContextsPanel

class WzScriptContextsPanel : public W_FORM
{
public:
	WzScriptContextsPanel(): W_FORM() {}
	~WzScriptContextsPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		contextDropdown->callCalcLayout();
		runCommandButton->callCalcLayout();
		runCommandInputField->callCalcLayout();
		table->callCalcLayout();
	}
public:
	static std::shared_ptr<WzScriptContextsPanel> make(const std::shared_ptr<WZScriptDebugger>& parentScriptDebugger)
	{
		auto result = std::make_shared<WzScriptContextsPanel>();
		result->scriptDebugger = parentScriptDebugger;

		// Add "Viewing Context" label
		auto contextLabel = std::make_shared<W_LABEL>();
		contextLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		contextLabel->setString("Viewing Context:");
		contextLabel->setGeometry(0, 0, contextLabel->getMaxLineWidth() + 10, TAB_BUTTONS_HEIGHT);
		contextLabel->setCacheNeverExpires(true);
		result->attach(contextLabel);

		// Add context chooser drop-down
		result->contextDropdown = std::make_shared<DropdownWidget>();
		result->attach(result->contextDropdown);
		result->contextDropdown->setListHeight(TAB_BUTTONS_HEIGHT * std::min(static_cast<int>(parentScriptDebugger->getModelMap().size()), 5));

		std::unordered_map<size_t, wzapi::scripting_instance *> contextButtonMap;
		size_t numContextButtons = 0;
		int maxButtonTextWidth = 0;
		for (auto& it : parentScriptDebugger->getModelMap())
		{
			WzString buttonLabel = WzString::fromUtf8(it.first->scriptName());
			buttonLabel += ":" + WzString::number(it.first->player());
			auto button = makeDebugButton(buttonLabel.toUtf8().c_str());
			int minButtonTextWidth = button->width();
			maxButtonTextWidth = std::max<int>(minButtonTextWidth, maxButtonTextWidth);
			result->contextDropdown->addItem(button);
			contextButtonMap[numContextButtons] = it.first;
			numContextButtons++;
		}
		result->contextDropdown->setSelectedIndex(0);
		result->contextDropdown->setOnChange([contextButtonMap](DropdownWidget& dropdown) {
			if (auto selectedIndex = dropdown.getSelectedIndex())
			{
				// Switch context
				auto it = contextButtonMap.find(selectedIndex.value());
				ASSERT_OR_RETURN(, it != contextButtonMap.end(), "Invalid dropdown index value");
				auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(dropdown.parent());
				ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
				psParent->viewContext(it->second);
			}
		});
		int contextDropdownX0 = contextLabel->x() + contextLabel->width();
		result->contextDropdown->setCalcLayout([contextDropdownX0](WIDGET *psWidget) {
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(contextDropdownX0, 0, psParent->width() - contextDropdownX0, TAB_BUTTONS_HEIGHT);
		});

		// Add input field and "Run" button on the bottom
		result->runCommandButton = makeDebugButton("Run Command");
		result->attach(result->runCommandButton);
		result->runCommandButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(psParent->width() - psWidget->width(), psParent->height() - psWidget->height(), psWidget->width(), psWidget->height());
		}));
		result->runCommandButton->addOnClickHandler([](W_BUTTON& button){
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(button.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			ASSERT_OR_RETURN(, psParent->currentContext != nullptr, "currentContext is null");
			if (!psParent->runCommandInputField || psParent->runCommandInputField->getString().isEmpty())
			{
				return;
			}
			psParent->currentContext->debugEvaluateCommand(psParent->runCommandInputField->getString().toStdString());
		});

		result->runCommandInputField = std::make_shared<W_EDITBOX>();
		result->attach(result->runCommandInputField);
		result->runCommandInputField->setGeometry(result->runCommandInputField->x(), result->runCommandInputField->y(), result->runCommandInputField->width(), result->runCommandButton->height());
		result->runCommandInputField->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(0, psParent->height() - psWidget->height(), psParent->runCommandButton->x() - ACTION_BUTTON_SPACING, psWidget->height());
		}));
		result->runCommandInputField->setString("");
		result->runCommandInputField->setMaxStringSize(150); // extend maximum input length
		result->runCommandInputField->setBoxColours(WZCOL_DEBUG_FILL_COLOR_DARK, WZCOL_DEBUG_BORDER_LIGHT, WZCOL_DEBUG_FILL_COLOR);

		// Add Table
		result->table = JSONTableWidget::make("Globals:");
		result->attach(result->table);
		result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int y0 = psParent->contextDropdown->y() + psParent->contextDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(0, y0, psParent->width(), psParent->runCommandButton->y() - ACTION_BUTTON_ROW_SPACING - y0);
		}));

		result->table->setUpdateButtonFunc([contextButtonMap](JSONTableWidget& tableWidget){
			auto psParent = std::dynamic_pointer_cast<WzScriptContextsPanel>(tableWidget.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			if (auto scriptDebuggerStrong = psParent->scriptDebugger.lock())
			{
				scriptDebuggerStrong->updateModelMap();
				if (auto selectedIndex = psParent->contextDropdown->getSelectedIndex())
				{
					auto it = contextButtonMap.find(selectedIndex.value());
					ASSERT_OR_RETURN(, it != contextButtonMap.end(), "Invalid dropdown index value");
					psParent->viewContext(it->second, true);
				}
			}
		});

		// View the first context
		if (!parentScriptDebugger->getModelMap().empty())
		{
			result->viewContext(parentScriptDebugger->getModelMap().begin()->first);
		}

		return result;
	}
public:
	void viewContext(wzapi::scripting_instance *context, bool tryToPreservePath = false)
	{
		ASSERT_OR_RETURN(, context != nullptr, "context is null");
		if (auto scriptDebuggerStrong = scriptDebugger.lock())
		{
			const auto& modelMap = scriptDebuggerStrong->getModelMap();
			auto it = modelMap.find(context);
			ASSERT_OR_RETURN(, it != modelMap.end(), "context not found?!");
			table->updateData(it->second, tryToPreservePath, transformContextSpecialStrings(context));
			currentContext = context;
		}
	}
private:
	JSONTableWidget::SpecialJSONStringTypes toJSONSpecialStringType(wzapi::scripting_instance::DebugSpecialStringType debugStringType) const
	{
		switch (debugStringType)
		{
		case wzapi::scripting_instance::DebugSpecialStringType::TYPE_DESCRIPTION:
			return JSONTableWidget::SpecialJSONStringTypes::TYPE_DESCRIPTION;
		}
		return static_cast<JSONTableWidget::SpecialJSONStringTypes>(0); // silence warning
	}
	std::unordered_map<std::string, JSONTableWidget::SpecialJSONStringTypes> transformContextSpecialStrings(wzapi::scripting_instance *context)
	{
		ASSERT_OR_RETURN({}, context != nullptr, "null context");
		auto contextSpecialStringInfo = context->debugGetScriptGlobalSpecialStringValues();
		std::unordered_map<std::string, JSONTableWidget::SpecialJSONStringTypes> result;
		for (auto& it : contextSpecialStringInfo)
		{
			result[it.first] = toJSONSpecialStringType(it.second);
		}
		return result;
	}
public:
	wzapi::scripting_instance *currentContext = nullptr;
	std::weak_ptr<WZScriptDebugger> scriptDebugger;
	std::shared_ptr<DropdownWidget> contextDropdown;
	std::shared_ptr<JSONTableWidget> table;
	std::shared_ptr<W_EDITBOX> runCommandInputField;
	std::shared_ptr<W_BUTTON> runCommandButton;
};

// MARK: - WzScriptPlayersPanel

class WzScriptPlayersPanel : public W_FORM
{
public:
	WzScriptPlayersPanel(): W_FORM() {}
	~WzScriptPlayersPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		playersDropdown->callCalcLayout();
		table->callCalcLayout();
	}
public:
	static std::shared_ptr<WzScriptPlayersPanel> make(const std::shared_ptr<WZScriptDebugger>& parentScriptDebugger)
	{
		auto result = std::make_shared<WzScriptPlayersPanel>();
		result->scriptDebugger = parentScriptDebugger;

		// Add "Player" label
		auto contextLabel = std::make_shared<W_LABEL>();
		contextLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		contextLabel->setString("Viewing:");
		contextLabel->setGeometry(0, 0, contextLabel->getMaxLineWidth() + 10, TAB_BUTTONS_HEIGHT);
		result->attach(contextLabel);

		// Add player chooser drop-down
		result->playersDropdown = std::make_shared<DropdownWidget>();
		result->attach(result->playersDropdown);
		result->playersDropdown->setListHeight(TAB_BUTTONS_HEIGHT * 5);

		int maxButtonTextWidth = 0;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			WzString buttonLabel = WzString("Player ") + WzString::number(i);
			auto button = makeDebugButton(buttonLabel.toUtf8().c_str());
			button->UserData = static_cast<UDWORD>(i);
			int minButtonTextWidth = button->width();
			maxButtonTextWidth = std::max<int>(minButtonTextWidth, maxButtonTextWidth);
			result->playersDropdown->addItem(button);
		}
		result->playersDropdown->setSelectedIndex(0);
		result->playersDropdown->setOnChange([](DropdownWidget& dropdown) {
			if (auto selectedIndex = dropdown.getSelectedIndex())
			{
				// Switch player model
				auto psParent = std::dynamic_pointer_cast<WzScriptPlayersPanel>(dropdown.parent());
				ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
				psParent->viewPlayerModel(static_cast<int>(selectedIndex.value()));
			}
		});
		int contextDropdownX0 = contextLabel->x() + contextLabel->width();
		result->playersDropdown->setCalcLayout([contextDropdownX0](WIDGET *psWidget) {
			auto psParent = std::dynamic_pointer_cast<WzScriptPlayersPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(contextDropdownX0, 0, psParent->width() - contextDropdownX0, TAB_BUTTONS_HEIGHT);
		});

		// Add Table
		result->table = JSONTableWidget::make("Player Properties:");
		result->attach(result->table);
		result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptPlayersPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int y0 = psParent->playersDropdown->y() + psParent->playersDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);
		}));
		result->table->setUpdateButtonFunc([](JSONTableWidget& tableWidget){
			auto psParent = std::dynamic_pointer_cast<WzScriptPlayersPanel>(tableWidget.parent());
			if (psParent == nullptr)
			{
				return;
			}
			if (auto selectedIndex = psParent->playersDropdown->getSelectedIndex())
			{
				psParent->viewPlayerModel(static_cast<int>(selectedIndex.value()));
			}
		}, 3 * GAME_TICKS_PER_SEC);

		// View the first player
		result->viewPlayerModel(0);

		return result;
	}
public:
	void viewPlayerModel(int playerIdx)
	{
		ASSERT_OR_RETURN(, playerIdx >= 0 && playerIdx <= MAX_PLAYERS, "Invalid player index: %d", playerIdx);
		table->updateData(fillPlayerModel(playerIdx), true);
	}
private:
	std::weak_ptr<WZScriptDebugger> scriptDebugger;
	std::shared_ptr<DropdownWidget> playersDropdown;
	std::shared_ptr<JSONTableWidget> table;
};

// MARK: - WzScriptTriggersPanel

class WzScriptTriggersPanel : public W_FORM
{
public:
	WzScriptTriggersPanel(): W_FORM() {}
	~WzScriptTriggersPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		updateButton->callCalcLayout();
		contextDropdown->callCalcLayout();
		table->callCalcLayout();
	}
public:
	static std::shared_ptr<WzScriptTriggersPanel> make(const std::shared_ptr<WZScriptDebugger>& parentScriptDebugger)
	{
		auto result = std::make_shared<WzScriptTriggersPanel>();
		result->scriptDebugger = parentScriptDebugger;

		// Add "Viewing Context" label
		auto contextLabel = std::make_shared<W_LABEL>();
		contextLabel->setFont(font_regular, WZCOL_FORM_TEXT);
		contextLabel->setString("Triggers for Context:");
		contextLabel->setGeometry(0, 0, contextLabel->getMaxLineWidth() + 10, TAB_BUTTONS_HEIGHT);
		contextLabel->setCacheNeverExpires(true);
		result->attach(contextLabel);

		// Add "update"/refresh button
		result->updateButton = makeDebugButton("\u21BB"); // "↻"
		result->updateButton->setGeometry(result->updateButton->x(), result->updateButton->y(), result->updateButton->width(), iV_GetTextLineSize(font_regular));
		result->updateButton->setTip("Update");
		result->attach(result->updateButton);
		result->updateButton->addOnClickHandler([](W_BUTTON& button) {
			auto psParent = std::dynamic_pointer_cast<WzScriptTriggersPanel>(button.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			widgScheduleTask([psParent](){
				psParent->updateTriggerSnapshot();
			});
		});
		result->updateButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptTriggersPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->width() - psWidget->width();
			int y0 = TAB_BUTTONS_HEIGHT - psWidget->height();
			psWidget->setGeometry(x0, y0, psWidget->width(), psWidget->height());
		}));

		// Add context chooser drop-down
		const MODELMAP& modelMap = parentScriptDebugger->getModelMap();
		result->contextDropdown = std::make_shared<DropdownWidget>();
		result->attach(result->contextDropdown);
		result->contextDropdown->setListHeight(TAB_BUTTONS_HEIGHT * std::min<uint32_t>(static_cast<uint32_t>(modelMap.size()), static_cast<uint32_t>(5)));

		std::unordered_map<size_t, wzapi::scripting_instance *> contextButtonMap;
		size_t numContextButtons = 0;
		int maxButtonTextWidth = 0;
		for (auto& it : modelMap)
		{
			WzString buttonLabel = WzString::fromUtf8(it.first->scriptName());
			buttonLabel += ":" + WzString::number(it.first->player());
			auto button = makeDebugButton(buttonLabel.toUtf8().c_str());
			int minButtonTextWidth = button->width();
			maxButtonTextWidth = std::max<int>(minButtonTextWidth, maxButtonTextWidth);
			result->contextDropdown->addItem(button);
			contextButtonMap[numContextButtons] = it.first;
			numContextButtons++;
		}
		result->contextDropdown->setSelectedIndex(0);
		result->contextDropdown->setOnChange([contextButtonMap](DropdownWidget& dropdown) {
			if (auto selectedIndex = dropdown.getSelectedIndex())
			{
				// Switch context
				auto it = contextButtonMap.find(selectedIndex.value());
				ASSERT_OR_RETURN(, it != contextButtonMap.end(), "Invalid dropdown index value");
				auto psParent = std::dynamic_pointer_cast<WzScriptTriggersPanel>(dropdown.parent());
				ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
				psParent->viewContext(it->second);
			}
		});
		int contextDropdownX0 = contextLabel->x() + contextLabel->width();
		result->contextDropdown->setCalcLayout([contextDropdownX0](WIDGET *psWidget) {
			auto psParent = std::dynamic_pointer_cast<WzScriptTriggersPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->setGeometry(contextDropdownX0, 0, psParent->updateButton->x() - contextDropdownX0 - ACTION_BUTTON_SPACING, TAB_BUTTONS_HEIGHT);
		});

		// Create column headers for triggers table
		auto idLabel = createColHeaderLabel("timerID");
		auto functionLabel = createColHeaderLabel("Function");
		auto objectLabel = createColHeaderLabel("Object");
		auto timeLabel = createColHeaderLabel("Time");
		auto intervalLabel = createColHeaderLabel("Interval");
		auto typeLabel = createColHeaderLabel("Type");
		auto callsLabel = createColHeaderLabel("Calls");
		std::vector<TableColumn> columns {
			{idLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{functionLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{objectLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{timeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{intervalLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{typeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{callsLabel, TableColumn::ResizeBehavior::RESIZABLE}
		};
		std::vector<size_t> minimumColumnWidths;
		for (auto& column : columns)
		{
			minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(std::dynamic_pointer_cast<W_LABEL>(column.columnWidget)->getMaxLineWidth(), 0)));
		}

		// Create + attach "Triggers" scrollable table view
		result->table = ScrollableTableWidget::make(columns);
		result->attach(result->table);
		result->table->setMinimumColumnWidths(minimumColumnWidths);
		result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptTriggersPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int oldWidth = psWidget->width();
			int y0 = psParent->contextDropdown->y() + psParent->contextDropdown->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);

			if (oldWidth != psWidget->width())
			{
				psParent->resizeTableColumnWidths();
			}
		}));

		// View the first context
		if (!parentScriptDebugger->getModelMap().empty())
		{
			result->viewContext(parentScriptDebugger->getModelMap().begin()->first);
		}

		return result;
	}
public:
	void viewContext(wzapi::scripting_instance *context)
	{
		ASSERT_OR_RETURN(, context != nullptr, "context is null");
		if (auto scriptDebuggerStrong = scriptDebugger.lock())
		{
			auto model = fillTriggersModel(scriptDebuggerStrong->getTriggerSnapshot(), context);
			auto oldScrollPosition = table->getScrollPosition();
			table->clearRows();
			if (!model.rows().empty())
			{
				for (auto& row : model.rows())
				{
					table->addRow(row);
				}
				currentMaxColumnWidths = model.currentMaxColumnWidths();
				table->changeColumnWidths(model.currentMaxColumnWidths());
				table->setScrollPosition(oldScrollPosition);
			}
			viewingContext = context;
		}
	}
	void updateTriggerSnapshot()
	{
		if (auto scriptDebuggerStrong = scriptDebugger.lock())
		{
			scriptDebuggerStrong->updateTriggerSnapshot();
			viewContext(viewingContext);
		}
	}
private:
	static std::shared_ptr<W_LABEL> createColHeaderLabel(const char* text)
	{
		auto label = std::make_shared<W_LABEL>();
		label->setString(text);
		label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
		label->setCacheNeverExpires(true);
		return label;
	}
	void resizeTableColumnWidths()
	{
		table->changeColumnWidths(currentMaxColumnWidths);
	}
public:
	std::weak_ptr<WZScriptDebugger> scriptDebugger;
	std::shared_ptr<W_BUTTON> updateButton;
	std::shared_ptr<DropdownWidget> contextDropdown;
	std::shared_ptr<ScrollableTableWidget> table;
	std::vector<size_t> currentMaxColumnWidths;
	wzapi::scripting_instance *viewingContext = nullptr;
};

// MARK: - WzScriptMessagesPanel

class WzScriptMessagesPanel : public W_FORM
{
public:
	WzScriptMessagesPanel(): W_FORM() {}
	~WzScriptMessagesPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		pageTabs->callCalcLayout();
		updateButton->callCalcLayout();
		// Fixed column widths for Viewdata table, for performance
		if (currentMaxColumnWidths.size() >= 2)
		{
			ASSERT(currentMaxColumnWidths[1].size() == 3, "Unexpected number of columns");
			currentMaxColumnWidths[1][0] = static_cast<size_t>(width() / 3);
			currentMaxColumnWidths[1][1] = 0;
			for (const auto& str : view_type)
			{
				currentMaxColumnWidths[1][1] = std::max(currentMaxColumnWidths[1][1], static_cast<size_t>(iV_GetTextWidth(str.toUtf8().c_str(), font_regular)));
			}
			currentMaxColumnWidths[1][2] = static_cast<size_t>(width() - currentMaxColumnWidths[1][0] - currentMaxColumnWidths[1][1]);
		}
		for (auto& table : tables)
		{
			table->callCalcLayout();
		}
	}
public:
	static std::shared_ptr<WzScriptMessagesPanel> make()
	{
		auto result = std::make_shared<WzScriptMessagesPanel>();

		// Add "update"/refresh button
		result->updateButton = makeDebugButton("\u21BB"); // "↻"
		result->updateButton->setGeometry(result->updateButton->x(), result->updateButton->y(), result->updateButton->width(), iV_GetTextLineSize(font_regular));
		result->updateButton->setTip("Update");
		result->attach(result->updateButton);

		// Add tabs
		result->pageTabs = std::make_shared<NoBackgroundTabWidget>(0);
		result->attach(result->pageTabs);
		result->pageTabs->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
		auto createdButton = makeDebugButton("Created");
		result->pageTabs->addButton(0, createdButton);
		auto viewDataButton = makeDebugButton("Viewdata");
		result->pageTabs->addButton(1, viewDataButton);
		result->pageTabs->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
			auto psParent = std::dynamic_pointer_cast<WzScriptMessagesPanel>(widget.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
			// Switch actively-displayed "tab" (table)
			ASSERT_OR_RETURN(, newValue >= 0, "invalid value");
			psParent->switchTable(static_cast<size_t>(newValue));
		});
		result->pageTabs->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptMessagesPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
			int newWidth = psParent->width() - (2 * psParent->updateButton->width()) - (2 * ACTION_BUTTON_SPACING);
			psWidget->setGeometry((psParent->width() - newWidth) / 2, 0, newWidth, TAB_BUTTONS_HEIGHT);
		}));

		// Configure "update"/refresh button
		result->updateButton->addOnClickHandler([](W_BUTTON& button) {
			auto psParent = std::dynamic_pointer_cast<WzScriptMessagesPanel>(button.parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			widgScheduleTask([psParent](){
				if (psParent->updateButtonFuncs[psParent->currentVisibleTable])
				{
					psParent->updateButtonFuncs[psParent->currentVisibleTable](*psParent);
				}
			});
		});
		result->updateButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptMessagesPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->width() - psWidget->width();
			int y0 = psParent->pageTabs->y() + psParent->pageTabs->height() - psWidget->height();
			psWidget->setGeometry(x0, y0, psWidget->width(), psWidget->height());
		}));
		result->updateButton->hide(); // hide by default - switchTable shows if available

		result->tables.push_back(make_createdTable());
		result->tables.push_back(make_viewdataTable());

		auto tableCalcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptMessagesPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int oldWidth = psWidget->width();
			int y0 = psParent->pageTabs->y() + psParent->pageTabs->height() + ACTION_BUTTON_ROW_SPACING;
			psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);

			if (oldWidth != psWidget->width())
			{
				psParent->resizeTableColumnWidths();
			}
		});

		for (auto& table : result->tables)
		{
			result->attach(table);
			result->currentMaxColumnWidths.push_back(table->getMinimumColumnWidths());
			table->setCalcLayout(tableCalcLayout);
			result->updateButtonFuncs.push_back(nullptr);
		}

		// set update func for Messages table
		result->updateButtonFuncs[0] = [](WzScriptMessagesPanel& panel){
			panel.updateMessageTable();
		};

		result->switchTable(0);

		result->updateMessageTable();
		result->updateViewDataTable();

		return result;
	}
public:
	void updateMessageTable()
	{
		updateTableWithModel(0, fillMessageModel());
	}
	void updateViewDataTable()
	{
		updateTableWithModel(1, fillViewdataModel(), true);
	}
private:
	void switchTable(size_t tableId)
	{
		ASSERT_OR_RETURN(, tableId < tables.size(), "Invalid tableId: %zu", tableId);
		for (size_t idx = 0; idx < tables.size(); idx++)
		{
			if (idx != tableId)
			{
				tables[idx]->hide();
			}
			else
			{
				tables[idx]->show();
			}
		}
		if (updateButtonFuncs[tableId])
		{
			updateButton->show();
			updateButton->setTip("Live Refresh");
			updateButton->setState(WBUT_DISABLE);
			updateButton->setProgressBorder(TableRow::ProgressBorder::indeterminate(TableRow::ProgressBorder::BorderInset()), WZCOL_DEBUG_INDETERMINATE_PROGRESS_COLOR);
		}
		else
		{
			updateButton->hide();
		}
		currentVisibleTable = tableId;
	}
	static std::shared_ptr<W_LABEL> createColHeaderLabel(const char* text)
	{
		auto label = std::make_shared<W_LABEL>();
		label->setString(text);
		label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
		label->setCacheNeverExpires(true);
		return label;
	}
	static std::shared_ptr<ScrollableTableWidget> make_createdTable()
	{
		// Create column headers for "Created" view
		auto idLabel = createColHeaderLabel("ID");
		auto typeLabel = createColHeaderLabel("Type");
		auto dataTypeLabel = createColHeaderLabel("Data Type");
		auto playerLabel = createColHeaderLabel("Player");
		auto nameLabel = createColHeaderLabel("Name");
		auto viewDataLabel = createColHeaderLabel("ViewData Type");
		std::vector<TableColumn> columns {
			{idLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{typeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{dataTypeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{playerLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{nameLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{viewDataLabel, TableColumn::ResizeBehavior::RESIZABLE}
		};
		std::vector<size_t> minimumColumnWidths;
		for (auto& column : columns)
		{
			minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(std::dynamic_pointer_cast<W_LABEL>(column.columnWidget)->getMaxLineWidth(), 0)));
		}

		// Create + attach "Created" scrollable table view
		auto table = ScrollableTableWidget::make(columns);
		table->setMinimumColumnWidths(minimumColumnWidths);
		return table;
	}
	static std::shared_ptr<ScrollableTableWidget> make_viewdataTable()
	{
		// Create column headers for "Viewdata" view
		auto nameLabel = createColHeaderLabel("Name");
		auto typeLabel = createColHeaderLabel("Type");
		auto sourceLabel = createColHeaderLabel("Source");
		std::vector<TableColumn> columns {
			{nameLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{typeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{sourceLabel, TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND}
		};
		std::vector<size_t> minimumColumnWidths;
		for (auto& column : columns)
		{
			minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(std::dynamic_pointer_cast<W_LABEL>(column.columnWidget)->getMaxLineWidth(), 0)));
		}

		// Create + attach "Viewdata" scrollable table view
		auto table = ScrollableTableWidget::make(columns);
		table->setMinimumColumnWidths(minimumColumnWidths);
		return table;
	}
	void updateTableWithModel(size_t tableIdx, const RowDataModel& model, bool skipColumnWidths = false)
	{
		ASSERT_OR_RETURN(, tableIdx < tables.size(), "invalid table index");
		auto& selectedTable = tables[tableIdx];
		selectedTable->clearRows();
		if (!model.rows().empty())
		{
			for (auto& row : model.rows())
			{
				selectedTable->addRow(row);
			}
			if (!skipColumnWidths)
			{
				currentMaxColumnWidths[tableIdx] = model.currentMaxColumnWidths();
				selectedTable->changeColumnWidths(model.currentMaxColumnWidths());
			}
		}
	}
	void resizeTableColumnWidths()
	{
		for (size_t tableIdx = 0; tableIdx < tables.size(); tableIdx++)
		{
			tables[tableIdx]->changeColumnWidths(currentMaxColumnWidths[tableIdx]);
		}
	}
private:
	std::shared_ptr<MultibuttonWidget> pageTabs;
	std::shared_ptr<W_BUTTON> updateButton;
	std::vector<std::shared_ptr<ScrollableTableWidget>> tables;
	std::vector<std::function<void (WzScriptMessagesPanel&)>> updateButtonFuncs;
	std::vector<std::vector<size_t>> currentMaxColumnWidths;
	size_t currentVisibleTable = 0;
};

// MARK: - WzScriptLabelsPanel

class WzScriptLabelsPanel : public W_FORM
{
public:
	WzScriptLabelsPanel(): W_FORM() {}
	~WzScriptLabelsPanel() {}
public:
	virtual void display(int xOffset, int yOffset) override
	{
		// no background
	}
	virtual void geometryChanged() override
	{
		updateButton->callCalcLayout();
		for (auto& button : bottomButtons)
		{
			button->callCalcLayout();
		}
		table->callCalcLayout();
	}
public:
	static std::shared_ptr<WzScriptLabelsPanel> make(const std::shared_ptr<WZScriptDebugger>& parentScriptDebugger)
	{
		auto result = std::make_shared<WzScriptLabelsPanel>();
		result->scriptDebugger = parentScriptDebugger;

		// Add "Labels:" label
		auto contextLabel = std::make_shared<W_LABEL>();
		contextLabel->setFont(font_regular_bold, WZCOL_FORM_TEXT);
		contextLabel->setString("Labels:");
		contextLabel->setGeometry(0, 0, contextLabel->getMaxLineWidth() + 10, TAB_BUTTONS_HEIGHT);
		contextLabel->setCacheNeverExpires(true);
		result->attach(contextLabel);

		// Add "update"/refresh button, that signifies "live updating"
		result->updateButton = makeDebugButton("\u21BB"); // "↻"
		result->updateButton->setGeometry(result->updateButton->x(), result->updateButton->y(), result->updateButton->width(), iV_GetTextLineSize(font_regular));
		result->updateButton->setTip("Live Refresh");
		result->attach(result->updateButton);
		result->updateButton->setState(WBUT_DISABLE);
		result->updateButton->setProgressBorder(TableRow::ProgressBorder::indeterminate(TableRow::ProgressBorder::BorderInset()), WZCOL_DEBUG_INDETERMINATE_PROGRESS_COLOR);
		result->updateButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptLabelsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int x0 = psParent->width() - psWidget->width();
			int y0 = TAB_BUTTONS_HEIGHT - psWidget->height();
			psWidget->setGeometry(x0, y0, psWidget->width(), psWidget->height());
		}));

		// Create bottom buttons
		std::shared_ptr<W_BUTTON> previousRowButton;
		auto addBottomButton = [&previousRowButton, &result](const char *text, const std::function<void ()>& onClickFunc){
			previousRowButton = result->createBottomButton(text, onClickFunc, previousRowButton);
			result->bottomButtons.push_back(previousRowButton);
		};
		addBottomButton("Show All", WzScriptLabelsPanel::labelClickedAll);
		addBottomButton("Show Active", WzScriptLabelsPanel::labelClickedActive);
		addBottomButton("Clear", WzScriptLabelsPanel::labelClear);

		// Create column headers for Labels table
		auto labelLabel = createColHeaderLabel("Label");
		auto typeLabel = createColHeaderLabel("Type");
		auto triggerLabel = createColHeaderLabel("Trigger");
		auto ownerLabel = createColHeaderLabel("Owner");
		auto subscriberLabel = createColHeaderLabel("Subscriber");
		std::vector<TableColumn> columns {
			{labelLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{typeLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{triggerLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{ownerLabel, TableColumn::ResizeBehavior::RESIZABLE},
			{subscriberLabel, TableColumn::ResizeBehavior::RESIZABLE}
		};
		std::vector<size_t> minimumColumnWidths;
		for (auto& column : columns)
		{
			minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(std::dynamic_pointer_cast<W_LABEL>(column.columnWidget)->getMaxLineWidth(), 0)));
		}

		// Create + attach "Labels" scrollable table view
		result->table = ScrollableTableWidget::make(columns);
		result->attach(result->table);
		result->table->setMinimumColumnWidths(minimumColumnWidths);
		result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
			auto psParent = std::dynamic_pointer_cast<WzScriptLabelsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int oldWidth = psWidget->width();
			int y0 = psParent->updateButton->y() + psParent->updateButton->height() + ACTION_BUTTON_ROW_SPACING;
			int height = ((!psParent->bottomButtons.empty()) ? psParent->bottomButtons[0]->y() - ACTION_BUTTON_ROW_SPACING : psParent->height()) - y0;
			psWidget->setGeometry(0, y0, psParent->width(), height);

			if (oldWidth != psWidget->width())
			{
				psParent->resizeTableColumnWidths();
			}
		}));

		result->currentMaxColumnWidths = minimumColumnWidths;
		result->populateLabels();

		return result;
	}
public:
	void populateLabels()
	{
		if (auto scriptDebuggerStrong = scriptDebugger.lock())
		{
			auto model = fillLabelsModel(scriptDebuggerStrong->getLabelModel());
			auto oldScrollPosition = table->getScrollPosition();
			table->clearRows();
			if (!model.rows().empty())
			{
				std::weak_ptr<WzScriptLabelsPanel> psWeakParent = std::dynamic_pointer_cast<WzScriptLabelsPanel>(shared_from_this());
				for (auto& row : model.rows())
				{
					table->addRow(row);
				}
				currentMaxColumnWidths = model.currentMaxColumnWidths();
				table->changeColumnWidths(model.currentMaxColumnWidths());
				table->setScrollPosition(oldScrollPosition);
			}
		}
	}
	void updateLabelData()
	{
		populateLabels();
	}
	void labelClicked(const std::string& label)
	{
		if (auto scriptDebuggerStrong = scriptDebugger.lock())
		{
			scriptDebuggerStrong->labelClicked(label);
		}
	}
	static void labelClear()
	{
		if (globalDialog)
		{
			globalDialog->labelClear();
		}
	}
	static void labelClickedAll()
	{
		if (globalDialog)
		{
			globalDialog->labelClickedAll();
		}
	}
	static void labelClickedActive()
	{
		if (globalDialog)
		{
			globalDialog->labelClickedActive();
		}
	}
private:
	static std::shared_ptr<W_LABEL> createColHeaderLabel(const char* text)
	{
		auto label = std::make_shared<W_LABEL>();
		label->setString(text);
		label->setGeometry(0, 0, label->getMaxLineWidth(), 0);
		label->setCacheNeverExpires(true);
		return label;
	}
	void resizeTableColumnWidths()
	{
		table->changeColumnWidths(currentMaxColumnWidths);
	}
	std::shared_ptr<W_BUTTON> createBottomButton(const std::string &text, const std::function<void ()>& onClickFunc, const std::shared_ptr<W_BUTTON>& previousButton = nullptr)
	{
		auto button = makeDebugButton(text.c_str());
		button->setGeometry(button->x(), button->y(), button->width() + 10, button->height());
		attach(button);
		button->addOnClickHandler([onClickFunc](W_BUTTON& button) {
			widgScheduleTask([onClickFunc](){
				onClickFunc();
			});
		});
		int previousButtonRight = (previousButton) ? previousButton->x() + previousButton->width() : 0;
		button->move((previousButtonRight > 0) ? previousButtonRight + ACTION_BUTTON_SPACING : 0, height() - button->height());
		button->setCalcLayout([](WIDGET *psWidget) {
			auto psParent = std::dynamic_pointer_cast<WzScriptLabelsPanel>(psWidget->parent());
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			psWidget->move(psWidget->x(), psParent->height() - psWidget->height());
		});

		return button;
	}
	RowDataModel fillLabelsModel(const std::vector<scripting_engine::LabelInfo>& labels)
	{
		RowDataModel result(5);
		std::weak_ptr<WzScriptLabelsPanel> psWeakParent = std::dynamic_pointer_cast<WzScriptLabelsPanel>(shared_from_this());
		for (const auto &label : labels)
		{
			std::vector<WzString> columnTexts = {label.label, label.type, label.trigger, label.owner, label.subscriber};
			auto row = result.newRow(columnTexts, SCRIPTDEBUG_ROW_HEIGHT);
			std::string labelStringCopy = label.label.toStdString();
			row->addOnClickHandler([labelStringCopy, psWeakParent](W_BUTTON& button) {
				widgScheduleTask([psWeakParent, labelStringCopy]{
					if (auto psParent = psWeakParent.lock())
					{
						psParent->labelClicked(labelStringCopy);
					}
				});
			});
			row->setHighlightsOnMouseOver(true);
		}
		return result;
	}

public:
	std::weak_ptr<WZScriptDebugger> scriptDebugger;
	std::shared_ptr<W_BUTTON> updateButton;
	std::vector<std::shared_ptr<W_BUTTON>> bottomButtons;
	std::shared_ptr<ScrollableTableWidget> table;
	std::vector<size_t> currentMaxColumnWidths;
};

// MARK: - WZScriptDebugger

std::shared_ptr<W_BUTTON> WZScriptDebugger::createButton(int row, const std::string &text, const std::function<void ()>& onClickFunc, const std::shared_ptr<WIDGET>& parent, const std::shared_ptr<W_BUTTON>& previousButton /*= nullptr*/)
{
	auto button = makeDebugButton(text.c_str());
	button->setGeometry(button->x(), button->y(), button->width() + 10, button->height());
	parent->attach(button);
	button->addOnClickHandler([onClickFunc](W_BUTTON& button) {
		widgScheduleTask([onClickFunc](){
			onClickFunc();
		});
	});

	int previousButtonRight = (previousButton) ? previousButton->x() + previousButton->width() : 0;
	button->move(previousButtonRight + ACTION_BUTTON_SPACING, (row * (button->height() + ACTION_BUTTON_ROW_SPACING)));

	return button;
}

void WZScriptDebugger::addTextTabButton(const std::shared_ptr<MultibuttonWidget>& mbw, WZScriptDebugger::ScriptDebuggerPanel value, const char* text)
{
	auto button = makeDebugButton(text);
	mbw->addButton(static_cast<int>(value), button);
}

void WZScriptDebugger::switchPanel(WZScriptDebugger::ScriptDebuggerPanel newPanel)
{
	if (newPanel == currentPanel && currentPanelWidget != nullptr)
	{
		return;
	}
	// Close currently-open panel
	if (currentPanelWidget)
	{
		widgDelete(currentPanelWidget.get());
		currentPanelWidget = nullptr;
	}
	// Open new panel
	int pageTabsBottom = pageTabs->y() + pageTabs->height();
	std::shared_ptr<WIDGET> psPanel;
	switch (newPanel)
	{
		case ScriptDebuggerPanel::Main:
			psPanel = createMainPanel();
			break;
		case ScriptDebuggerPanel::Selected:
			psPanel = createSelectedPanel();
			break;
		case ScriptDebuggerPanel::Contexts:
			psPanel = createContextsPanel();
			break;
		case ScriptDebuggerPanel::Players:
			psPanel = createPlayersPanel();
			break;
		case ScriptDebuggerPanel::Triggers:
			psPanel = createTriggersPanel();
			break;
		case ScriptDebuggerPanel::Messages:
			psPanel = createMessagesPanel();
			break;
		case ScriptDebuggerPanel::Labels:
			psPanel = createLabelsPanel();
			break;
		default:
			debug(LOG_ERROR, "Panel not implemented yet");
			break;
	}
	if (psPanel)
	{
		attach(psPanel);
		psPanel->setCalcLayout([pageTabsBottom](WIDGET *psWidget) {
			auto psParent = psWidget->parent();
			ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
			int y0 = pageTabsBottom + 20;
			psWidget->setGeometry(15, y0, psParent->width() - 30, psParent->height() - (y0 + 15));
		});
	}
	currentPanelWidget = psPanel;
	currentPanel = newPanel;
}

WZScriptDebugger::WZScriptDebugger(const std::shared_ptr<scripting_engine::DebugInterface>& _debugInterface, bool readOnly)
: debugInterface(_debugInterface)
, readOnly(readOnly)
{
	modelMap = debugInterface->debug_GetGlobalsSnapshot();
	trigger_snapshot = debugInterface->debug_GetTimersSnapshot();
	labels = debugInterface->debug_GetLabelInfo();
}

void WZScriptDebugger::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	// draw "drop-shadow"
	int dropShadowX0 = std::max<int>(x0 - 6, 0);
	int dropShadowY0 = std::max<int>(y0 - 6, 0);
	int dropShadowX1 = std::min<int>(x1 + 6, pie_GetVideoBufferWidth());
	int dropShadowY1 = std::min<int>(y1 + 6, pie_GetVideoBufferHeight());
	pie_UniTransBoxFill((float)dropShadowX0, (float)dropShadowY0, (float)dropShadowX1, (float)dropShadowY1, pal_RGBA(0, 0, 0, 40));

	// draw background + border
	pie_UniTransBoxFill((float)x0, (float)y0, (float)x1, (float)y1, pal_RGBA(5, 0, 15, 170));
	iV_Box(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 150));

	// draw title
	int titleXPadding = 10;
	cachedTitleText.setText(_("Script Debugger"), font_regular_bold);

	Vector2i textBoundingBoxOffset(0, 0);
	textBoundingBoxOffset.y = y0 + TITLE_TOP_PADDING + (TITLE_HEIGHT - cachedTitleText.lineSize()) / 2;

	int fw = cachedTitleText.width();
	int fx = x0 + titleXPadding + (width() - titleXPadding - fw) / 2;
	float fy = float(textBoundingBoxOffset.y) - float(cachedTitleText.aboveBase());
	cachedTitleText.render(textBoundingBoxOffset.x + fx, fy, WZCOL_FORM_TEXT);
}

std::shared_ptr<W_FORM> WZScriptDebugger::createMainPanel()
{
	return WzMainPanel::make(readOnly);
}

std::shared_ptr<WIDGET> WZScriptDebugger::createSelectedPanel()
{
	auto result = JSONTableWidget::make("Selected Object:");

	if (!selectedObjectDetails.empty())
	{
		result->updateData(selectedObjectDetails);
	}
	return result;
}

std::shared_ptr<WIDGET> WZScriptDebugger::createContextsPanel()
{
	auto panel = WzScriptContextsPanel::make(std::dynamic_pointer_cast<WZScriptDebugger>(shared_from_this()));
	return panel;
}

std::shared_ptr<WIDGET> WZScriptDebugger::createPlayersPanel()
{
	auto panel = WzScriptPlayersPanel::make(std::dynamic_pointer_cast<WZScriptDebugger>(shared_from_this()));
	return panel;
}

std::shared_ptr<WIDGET> WZScriptDebugger::createTriggersPanel()
{
	auto panel = WzScriptTriggersPanel::make(std::dynamic_pointer_cast<WZScriptDebugger>(shared_from_this()));
	return panel;
}

std::shared_ptr<WIDGET> WZScriptDebugger::createMessagesPanel()
{
	auto panel = WzScriptMessagesPanel::make();
	return panel;
}

std::shared_ptr<WIDGET> WZScriptDebugger::createLabelsPanel()
{
	return WzScriptLabelsPanel::make(std::dynamic_pointer_cast<WZScriptDebugger>(shared_from_this()));
}

struct CornerButtonDisplayCache
{
	WzText text;
};

static void CornerButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using CornerButtonDisplayFunc must have its pUserData initialized to a (CornerButtonDisplayCache*)
	assert(psWidget->pUserData != nullptr);
	CornerButtonDisplayCache& cache = *static_cast<CornerButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;
	int x1 = x0 + psButton->width();
	int y1 = y0 + psButton->height();

	bool haveText = !psButton->pText.isEmpty();

//	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = (psButton->getState() & WBUT_HIGHLIGHT) != 0;

	// Display the button.
	if (isHighlight)
	{
		pie_UniTransBoxFill(static_cast<float>(x0 + 1), static_cast<float>(y0 + 1), static_cast<float>(x1 - 1), static_cast<float>(y1 - 1), WZCOL_DEBUG_FILL_COLOR);
		iV_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, pal_RGBA(255, 255, 255, 150));
	}

	if (haveText)
	{
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fw = cache.text.width();
		int fx = x0 + (psButton->width() - fw) / 2;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			cache.text.render(fx + 1, fy + 1, WZCOL_FORM_LIGHT);
			textColor = WZCOL_FORM_DISABLE;
		}
		cache.text.render(fx, fy, textColor);
	}

	if (isDisabled)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + psButton->width(), y0 + psButton->height());
	}
}

static std::shared_ptr<W_BUTTON> makeCornerButton(const char* text)
{
	auto button = std::make_shared<W_BUTTON>();
	button->setString(text);
	button->FontID = font_regular_bold;
	button->displayFunction = CornerButtonDisplayFunc;
	button->pUserData = new CornerButtonDisplayCache();
	button->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<CornerButtonDisplayCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
	int buttonSize = std::max({minButtonWidthForText + TAB_BUTTONS_PADDING, 18, iV_GetTextLineSize(button->FontID)});
	button->setGeometry(0, 0, buttonSize, buttonSize);
	return button;
}

std::shared_ptr<WZScriptDebugger> WZScriptDebugger::make(const std::shared_ptr<scripting_engine::DebugInterface>& debugInterface, bool readOnly)
{
	auto result = std::make_shared<WZScriptDebugger>(debugInterface, readOnly);

	// make minimizable
	result->enableMinimizing("Script Debugger", WZCOL_FORM_TEXT);

	// Main "parent" form
	result->id = MULTIOP_OPTIONS;
	int newFormWidth = FRONTEND_TOPFORM_WIDEW;
	result->setGeometry((pie_GetVideoBufferWidth() - newFormWidth) / 2, 10, newFormWidth, pie_GetVideoBufferHeight() - 50);
	result->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		// update the height
		int newHeight = std::min<int>(pie_GetVideoBufferHeight() - 50, SCRIPTDEBUG_WINDOW_HEIGHT_MAX);
		// update the x if necessary to ensure the entire form is visible
		int x0 = std::min(psWidget->x(), pie_GetVideoBufferWidth() - psWidget->width());
		// update the y if necessary to ensure the entire form is visible
		int y0 = std::min(psWidget->y(), pie_GetVideoBufferHeight() - newHeight);
		psWidget->setGeometry(x0, y0, psWidget->width(), newHeight);
	}));
	result->userMovable = true;

	// Add the "minimize" button
	auto minimizeButton = makeCornerButton("\u21B8"); // ↸
	result->attach(minimizeButton);
	minimizeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<WZScriptDebugger>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([psParent](){
			psParent->toggleMinimized();
		});
	});

	// Add the "close" button
	W_BUTINIT sButInit;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT, IMAGE_CLOSE);
	auto closeButton = std::make_shared<W_BUTTON>(&sButInit);
	result->attach(closeButton);
	closeButton->addOnClickHandler([](W_BUTTON& button){
		auto psParent = std::dynamic_pointer_cast<WZScriptDebugger>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([](){
			jsDebugShutdown();
		});
	});
	closeButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<WZScriptDebugger>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		psWidget->setGeometry(psParent->width() - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
	}));

	// Add tabs
	result->pageTabs = std::make_shared<NoBackgroundTabWidget>(0);
	result->attach(result->pageTabs);
	result->pageTabs->id = MULTIOP_TECHLEVEL;
	result->pageTabs->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Main, "Main");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Selected, "Selected");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Contexts, "Contexts");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Players, "Players");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Triggers, "Triggers");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Messages, "Messages");
	addTextTabButton(result->pageTabs, ScriptDebuggerPanel::Labels, "Labels");
	result->pageTabs->addOnChooseHandler([](MultibuttonWidget& widget, int newValue){
		// Switch actively-displayed "tab"
		widgScheduleTask([newValue](){
			globalDialog->switchPanel(static_cast<WZScriptDebugger::ScriptDebuggerPanel>(newValue));
		});
	});
	result->pageTabs->setGeometry(5, TITLE_BOTTOM + 5, result->width() - 10, TAB_BUTTONS_HEIGHT);

	// Create and add Main panel
	result->switchPanel(ScriptDebuggerPanel::Main);

	return result;
}

void WZScriptDebugger::updateMessages()
{
	if (currentPanel == ScriptDebuggerPanel::Messages)
	{
		auto messagesPanel = std::dynamic_pointer_cast<WzScriptMessagesPanel>(currentPanelWidget);
		if (messagesPanel)
		{
			messagesPanel->updateMessageTable();
		}
	}
}

void WZScriptDebugger::updateModelMap()
{
	modelMap = debugInterface->debug_GetGlobalsSnapshot();
}

void WZScriptDebugger::updateTriggerSnapshot()
{
	trigger_snapshot = debugInterface->debug_GetTimersSnapshot();
}

void WZScriptDebugger::updateLabelModel()
{
	labels = debugInterface->debug_GetLabelInfo();
	// If the labels panel is up, tell it to update its contents
	if (currentPanel == ScriptDebuggerPanel::Labels)
	{
		auto labelsPanel = std::dynamic_pointer_cast<WzScriptLabelsPanel>(currentPanelWidget);
		if (labelsPanel)
		{
			labelsPanel->updateLabelData();
		}
	}
}

void WZScriptDebugger::selected(const BASE_OBJECT *psObj)
{
	selectedObjectDetails = nlohmann::ordered_json::object();
	selectedObjectId = nullopt;
	if (psObj != nullptr)
	{
		selectedObjectId = SelectedObjectId(psObj);
		selectedObjectDetails["Name"] = objInfo(psObj);
		selectedObjectDetails["Type"] = getObjType(psObj);
		selectedObjectDetails["Id"] = psObj->id;
		selectedObjectDetails["Player"] = psObj->player;
		selectedObjectDetails["Born"] = psObj->born;
		selectedObjectDetails["Died"] = psObj->died;
		selectedObjectDetails["Group"] = psObj->group;
		selectedObjectDetails["Watched tiles"] = psObj->watchedTiles.size();
		selectedObjectDetails["Last hit"] = psObj->timeLastHit;
		selectedObjectDetails["Hit points"] = psObj->body;
		selectedObjectDetails["Periodical start"] = psObj->periodicalDamageStart;
		selectedObjectDetails["Periodical damage"] = psObj->periodicalDamage;
		selectedObjectDetails["Animation event"] = psObj->animationEvent;
		selectedObjectDetails["Number of weapons"] = psObj->numWeaps;
		selectedObjectDetails["Last hit weapon"] = psObj->lastHitWeapon;
		selectedObjectDetails["Visible"] = arrayToString(psObj->visible, MAX_PLAYERS);
		selectedObjectDetails["Seen last tick"] = arrayToString(psObj->seenThisTick, MAX_PLAYERS);

		nlohmann::ordered_json weapons = nlohmann::ordered_json::array();
		for (int i = 0; i < psObj->numWeaps; i++)
		{
			if (psObj->asWeaps[i].nStat > 0)
			{
				WEAPON_STATS *psWeap = asWeaponStats + psObj->asWeaps[i].nStat;
				auto component = componentToString(psWeap, psObj->player);
				component["Ammo"] = psObj->asWeaps[i].ammo;
				component["Last fired time"] = psObj->asWeaps[i].lastFired;
				component["Shots fired"] = psObj->asWeaps[i].shotsFired;
				component["Used ammo"] = psObj->asWeaps[i].usedAmmo;
				component["Origin"] = psObj->asWeaps[i].origin;
				weapons.push_back(component);
			}
		}
		selectedObjectDetails["Weapons"] = weapons;

		if (psObj->type == OBJ_DROID)
		{
			const DROID *psDroid = castDroid(psObj);
			selectedObjectDetails["Droid type"] = psDroid->droidType;
			selectedObjectDetails["Weight"] = psDroid->weight;
			selectedObjectDetails["Base speed"] = psDroid->baseSpeed;
			selectedObjectDetails["Original hit points"] = psDroid->originalBody;
			selectedObjectDetails["Experience"] = psDroid->experience;
			selectedObjectDetails["Frustrated time"] = psDroid->lastFrustratedTime;
			selectedObjectDetails["Resistance"] = psDroid->resistance;
			selectedObjectDetails["Secondary order"] = psDroid->secondaryOrder;
			selectedObjectDetails["Action"] = getDroidActionName(psDroid->action);
			selectedObjectDetails["Action position"] = glm::to_string(psDroid->actionPos);
			selectedObjectDetails["Action started"] = psDroid->actionStarted;
			selectedObjectDetails["Action points"] = psDroid->actionPoints;
			selectedObjectDetails["Illumination"] = psDroid->illumination;
			selectedObjectDetails["Blocked bits"] = psDroid->blockedBits;
			selectedObjectDetails["Move status"] = psDroid->sMove.Status;
			selectedObjectDetails["Move index"] = psDroid->sMove.pathIndex;
			selectedObjectDetails["Move points"] = psDroid->sMove.asPath.size();
			selectedObjectDetails["Move destination"] = glm::to_string(psDroid->sMove.destination);
			selectedObjectDetails["Move source"] = glm::to_string(psDroid->sMove.src);
			selectedObjectDetails["Move target"] = glm::to_string(psDroid->sMove.target);
			selectedObjectDetails["Move bump pos"] = glm::to_string(psDroid->sMove.bumpPos);
			selectedObjectDetails["Move speed"] = psDroid->sMove.speed;
			selectedObjectDetails["Move direction"] = psDroid->sMove.moveDir;
			selectedObjectDetails["Move bump dir"] = psDroid->sMove.bumpDir;
			selectedObjectDetails["Move bump time"] = psDroid->sMove.bumpTime;
			selectedObjectDetails["Move last bump"] = psDroid->sMove.lastBump;
			selectedObjectDetails["Move pause time"] = psDroid->sMove.pauseTime;
			selectedObjectDetails["Move shuffle start"] = psDroid->sMove.shuffleStart;
			selectedObjectDetails["Move vert speed"] = psDroid->sMove.iVertSpeed;
			selectedObjectDetails["Body"] = componentToString(asBodyStats + psDroid->asBits[COMP_BODY], psObj->player);
			selectedObjectDetails["Brain"] = componentToString(asBrainStats + psDroid->asBits[COMP_BRAIN], psObj->player);
			selectedObjectDetails["Propulsion"] = componentToString(asPropulsionStats + psDroid->asBits[COMP_PROPULSION], psObj->player);
			selectedObjectDetails["ECM"] = componentToString(asECMStats + psDroid->asBits[COMP_ECM], psObj->player);
			selectedObjectDetails["Sensor"] = componentToString(asSensorStats + psDroid->asBits[COMP_SENSOR], psObj->player);
			selectedObjectDetails["Construct"] = componentToString(asConstructStats + psDroid->asBits[COMP_CONSTRUCT], psObj->player);
			selectedObjectDetails["Repair"] = componentToString(asRepairStats + psDroid->asBits[COMP_REPAIRUNIT], psObj->player);
		}
		else if (psObj->type == OBJ_STRUCTURE)
		{
			const STRUCTURE *psStruct = castStructure(psObj);
			selectedObjectDetails["Build points"] = psStruct->currentBuildPts;
			selectedObjectDetails["Build rate"] = psStruct->buildRate;
			selectedObjectDetails["Resistance"] = psStruct->resistance;
			selectedObjectDetails["Foundation depth"] = psStruct->foundationDepth;
			selectedObjectDetails["Capacity"] = psStruct->capacity;
			selectedObjectDetails["^Type"] = psStruct->pStructureType->type;
			selectedObjectDetails["^Build points"] = structureBuildPointsToCompletion(*psStruct);
			selectedObjectDetails["^Power points"] = psStruct->pStructureType->powerToBuild;
			selectedObjectDetails["^Height"] = psStruct->pStructureType->height;
			selectedObjectDetails["ECM"] = componentToString(psStruct->pStructureType->pECM, psObj->player);
			selectedObjectDetails["Sensor"] = componentToString(psStruct->pStructureType->pSensor, psObj->player);
			if (psStruct->pStructureType->type == REF_REARM_PAD)
			{
				selectedObjectDetails[":timeStarted"] = psStruct->pFunctionality->rearmPad.timeStarted;
				selectedObjectDetails[":timeLastUpdated"] = psStruct->pFunctionality->rearmPad.timeLastUpdated;
				selectedObjectDetails[":Rearm target"] = objInfo(psStruct->pFunctionality->rearmPad.psObj);
			}
		}
		else if (psObj->type == OBJ_FEATURE)
		{
			const FEATURE *psFeat = castFeature(psObj);
			selectedObjectDetails["^Feature type"] = psFeat->psStats->subType;
			selectedObjectDetails["^Needs drawn"] = psFeat->psStats->tileDraw;
			selectedObjectDetails["^Visible at start"] = psFeat->psStats->visibleAtStart;
			selectedObjectDetails["^Damageable"] = psFeat->psStats->damageable;
			selectedObjectDetails["^Hit points"] = psFeat->psStats->body;
			selectedObjectDetails["^Armour"] = psFeat->psStats->armourValue;
		}
	}

	if (currentPanel == ScriptDebuggerPanel::Selected)
	{
		auto psSelectedTable = std::dynamic_pointer_cast<JSONTableWidget>(currentPanelWidget);
		ASSERT_OR_RETURN(, psSelectedTable != nullptr, "selected table widget is null?");
		psSelectedTable->updateData(selectedObjectDetails);
	}
}

void WZScriptDebugger::labelClear()
{
	clearMarks();
}

void WZScriptDebugger::labelClickedAll()
{
	clearMarks();
	debugInterface->markAllLabels(false);
}

void WZScriptDebugger::labelClickedActive()
{
	clearMarks();
	debugInterface->markAllLabels(true);
}

void WZScriptDebugger::labelClicked(const std::string& label)
{
	debugInterface->showLabel(label);
}

WZScriptDebugger::~WZScriptDebugger()
{
	// currently nothing
}

// MARK: - jsDebug APIs

void jsDebugMessageUpdate()
{
	if (globalDialog)
	{
		globalDialog->updateMessages();
	}
}

void jsDebugSelected(const BASE_OBJECT *psObj)
{
	if (globalDialog)
	{
		globalDialog->selected(psObj);
	}
}

void jsDebugUpdateLabels()
{
	if (globalDialog)
	{
		globalDialog->updateLabelModel();
	}
}

bool jsDebugShutdown()
{
	if (debugScreen)
	{
		widgRemoveOverlayScreen(debugScreen);
	}
	if (globalDialog)
	{
		globalDialog->deleteLater();
		globalDialog = nullptr;
	}
	if(globalDialogShutdownHandler)
	{
		globalDialogShutdownHandler();
		globalDialogShutdownHandler = nullptr;
	}
	debugScreen = nullptr;
	return true;
}

void jsDebugCreate(const std::shared_ptr<scripting_engine::DebugInterface>& debugInterface, const jsDebugShutdownHandlerFunction& shutdownFunc, bool readOnly /*= false*/)
{
	jsDebugShutdown();
	globalDialogShutdownHandler = shutdownFunc;
	ASSERT_OR_RETURN(, debugInterface != nullptr, "debugInterface is null");
	debugScreen = W_SCREEN::make();
	debugScreen->psForm->hide(); // hiding the root form does not stop display of children, but *does* prevent it from accepting mouse over itself - i.e. basically makes it transparent
	widgRegisterOverlayScreen(debugScreen, std::numeric_limits<uint16_t>::max() - 2);
	globalDialog = WZScriptDebugger::make(debugInterface, readOnly);
	debugScreen->psForm->attach(globalDialog);
}
