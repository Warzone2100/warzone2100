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

#include "huddetailcontents.h"
#include "huddetail.h"
#include "hudbuildcategories.h"
#include "hudkeys.h"
#include "hudmodel.h"
#include "hudobjectinfo.h"
#include "hudskin.h"
#include "hudtext.h"
#include "hudunittypes.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/scrollablelist.h"

#include "../widgets/detailpanel/detailpanel.h"
#include "../widgets/detailpanel/research/researchdetail.h"
#include "../action.h"
#include "../cmddroid.h"
#include "../component.h"
#include "../droid.h"
#include "../group.h"
#include "../hci.h"
#include "../intdisplay.h"
#include "../selectionidle.h"
#include "../intimage.h"
#include "../loop.h"
#include "../map.h"
#include "../multiplay.h"
#include "../objmem.h"
#include "../order.h"
#include "../power.h"
#include "../projectile.h"
#include "../research.h"
#include "../stats.h"
#include "../structure.h"
#include "../game_world.h"

namespace
{

constexpr int COMPACT_MODEL_SIZE = 48;
constexpr size_t MAX_QUEUE_LINES = 4;
constexpr int PADDING = DetailPanelContents::PADDING;
constexpr int UNLIMITED_STORAGE = 1000000;
constexpr uint32_t MODEL_ROTATE_TIME = 3 * GAME_TICKS_PER_SEC;

PIELIGHT withAlpha(PIELIGHT color, uint8_t alpha)
{
	color.byte.a = alpha;
	return color;
}

DetailPanelStyle hudPanelStyle()
{
	DetailPanelStyle style = DetailPanelStyle::researchTree();
	style.drawBackground = [](const WzRect &rect) {
		currentHudSkin().drawPopoverFrame(rect);
	};
	style.divider = withAlpha(currentHudSkin().colors().panelBorder, 140);
	style.drawIcon = [](uint16_t image, float x, float y, float size, PIELIGHT) {
		// (the skin's vector bolt, where it has one, in the warning tone)
		const int side = static_cast<int>(std::lround(size));
		HudSkin &skin = currentHudSkin();
		return image == IMAGE_DES_POWER
			&& skin.drawNamedIcon("power", WzRect(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), side, side), skin.toneColor(HudTone::Warn));
	};
	return style;
}

Vector3i modelRotation()
{
	return Vector3i(-30, static_cast<int>((realTime % MODEL_ROTATE_TIME) * 360 / MODEL_ROTATE_TIME), 0);
}

int structureScale(unsigned baseSize)
{
	return baseSize == 1 ? SMALL_STRUCT_SCALE : (baseSize == 2 ? MED_STRUCT_SCALE : LARGE_STRUCT_SCALE);
}

std::shared_ptr<WIDGET> structureStatsModel(STRUCTURE_STATS *stats)
{
	return makeDetailPanelModel([stats](int x0, int y0, int w, int h) {
		const int box = std::min(w, h);
		pie_SetGeometricOffset(x0 + w / 2, y0 + h / 2 + box / 6);
		const Vector3i rotation = modelRotation();
		const Vector3i position(0, 0, BUTTON_DEPTH);
		displayStructureStatButton(stats, &rotation, &position, structureScale(getStructureStatSizeMax(stats)) * box / 46);
	});
}

std::shared_ptr<WIDGET> templateModel(DROID_TEMPLATE *psTemplate)
{
	return makeDetailPanelModel([psTemplate](int x0, int y0, int w, int h) {
		const int box = std::min(w, h);
		pie_SetGeometricOffset(x0 + w / 2, y0 + h / 2 + box / 5);
		const Vector3i rotation = modelRotation();
		const Vector3i position(0, 0, BUTTON_DEPTH);
		int scale = DROID_BUT_SCALE * box / 46;
		if (isTransporter(psTemplate))
		{
			scale /= 2;
		}
		displayComponentButtonTemplate(psTemplate, &rotation, &position, scale);
	});
}

std::shared_ptr<WIDGET> objectModel(BASE_OBJECT *object)
{
	const uint32_t id = object->id;
	const OBJECT_TYPE type = object->type;
	const unsigned player = object->player;
	return makeDetailPanelModel([id, type, player](int x0, int y0, int w, int h) {
		const int box = std::min(w, h);
		const Vector3i rotation = modelRotation();
		const Vector3i position(0, 0, BUTTON_DEPTH);
		if (type == OBJ_DROID)
		{
			DROID *psDroid = IdToDroid(gameWorld.objects, id, player);
			if (psDroid == nullptr)
			{
				return;
			}
			pie_SetGeometricOffset(x0 + w / 2, y0 + h / 2 + box / 5);
			displayComponentButtonObject(psDroid, &rotation, &position, DROID_BUT_SCALE * box / 46);
		}
		else if (type == OBJ_STRUCTURE)
		{
			STRUCTURE *psStruct = IdToStruct(id, player);
			if (psStruct == nullptr)
			{
				return;
			}
			pie_SetGeometricOffset(x0 + w / 2, y0 + h / 2 + box / 6);
			displayStructureButton(psStruct, &rotation, &position, structureScale(getStructureSizeMax(psStruct)) * box / 46);
		}
	});
}

std::shared_ptr<W_LABEL> subtitleLabel(const WzString &text)
{
	if (text.isEmpty())
	{
		return nullptr;
	}
	auto subtitle = std::make_shared<W_LABEL>();
	subtitle->setFont(font_small, pal_RGBA(150, 190, 235, 255));
	subtitle->setString(text);
	subtitle->setCanTruncate(true);
	return subtitle;
}

std::shared_ptr<WIDGET> powerStat(unsigned cost)
{
	return makeDetailPanelStat(WzString::fromUtf8(_("Cost")), WzString::number(cost), IMAGE_DES_POWER, _("Power to build it"));
}

std::shared_ptr<WIDGET> textStat(const char *label, const WzString &value, const char *tip = "")
{
	return makeDetailPanelStat(WzString::fromUtf8(label), value, nullopt, tip);
}

/// A body of short lines (and sections), with a width
class Body
{
public:
	explicit Body(int width): paragraph(makeDetailPanelParagraph(width)), width(width) {}

	void line(const WzString &text, PIELIGHT color = WZCOL_TEXT_MEDIUM)
	{
		paragraph->setFontColour(color);
		paragraph->addText(text + "\n");
		paragraph->setFontColour(WZCOL_TEXT_MEDIUM);
		empty = false;
	}
	void line(const std::string &text, PIELIGHT color = WZCOL_TEXT_MEDIUM) { line(WzString::fromUtf8(text), color); }
	void line(const char *text, PIELIGHT color = WZCOL_TEXT_MEDIUM) { line(WzString::fromUtf8(text), color); }
	void section(const char *heading)
	{
		detailPanelAddSection(paragraph, WzString::fromUtf8(heading));
		empty = false;
	}
	std::shared_ptr<WIDGET> widget()
	{
		if (empty)
		{
			return nullptr;
		}
		// (A paragraph only lays its text out when its width changes, and the list reads its height)
		paragraph->setGeometry(0, 0, width - 1, paragraph->height());
		paragraph->setGeometry(0, 0, width, paragraph->height());
		auto body = ScrollableListWidget::make();
		body->setGeometry(0, 0, width, 10);
		body->addItem(paragraph);
		return body;
	}

private:
	std::shared_ptr<Paragraph> paragraph;
	int width;
	bool empty = true;
};

std::shared_ptr<WIDGET> finish(std::shared_ptr<WIDGET> model, const WzString &title, std::shared_ptr<WIDGET> subtitle, std::shared_ptr<WIDGET> group,
                               std::shared_ptr<WIDGET> status, std::vector<std::shared_ptr<WIDGET>> stats, std::shared_ptr<WIDGET> body)
{
	const int width = hudDetailWidth();
	if (hudDetailSmall())
	{
		// (the button it describes shows it, beside the popover)
		model.reset();
	}
	auto contents = DetailPanelContents::make(std::move(model), makeDetailPanelTitle(title), std::move(subtitle), std::move(group), std::move(status),
	                                          std::move(stats), std::move(body), nullptr, width, hudPanelStyle(), DetailStatusPlacement::Automatic,
	                                          COMPACT_MODEL_SIZE);
	contents->setMaxHeight(hudDetailMaxHeight());
	return contents;
}

std::shared_ptr<WIDGET> statusLine(const WzString &text, HudTone tone)
{
	auto line = std::make_shared<W_LABEL>();
	line->setFont(font_small, currentHudSkin().toneColor(tone));
	line->setString(text);
	line->setCanTruncate(true);
	return line;
}

const char *structurePurpose(const STRUCTURE_STATS *stats)
{
	switch (stats->type)
	{
	case REF_HQ: return _("Command center: gives you the minimap and the design screen.");
	case REF_FACTORY: return _("Produces tanks and trucks.");
	case REF_CYBORG_FACTORY: return _("Produces cyborgs.");
	case REF_VTOL_FACTORY: return _("Produces VTOLs. They need rearm pads.");
	case REF_FACTORY_MODULE: return _("Makes a factory produce faster.");
	case REF_POWER_GEN: return _("Turns the oil of up to four derricks into power.");
	case REF_POWER_MODULE: return _("Makes a power generator produce more power.");
	case REF_RESOURCE_EXTRACTOR: return _("Extracts oil. Needs a power generator to give power.");
	case REF_RESEARCH: return _("Researches new technology.");
	case REF_RESEARCH_MODULE: return _("Makes a research facility research faster.");
	case REF_REPAIR_FACILITY: return _("Repairs units.");
	case REF_COMMAND_CONTROL: return _("Lets you build commanders, and sets how many.");
	case REF_REARM_PAD: return _("Rearms and repairs a VTOL.");
	case REF_SAT_UPLINK: return _("Reveals the whole map.");
	case REF_LASSAT: return _("Fires a laser satellite strike anywhere on the map.");
	case REF_MISSILE_SILO: return _("Launches missiles.");
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE: return _("Blocks the way of enemy units.");
	case REF_DEFENSE:
	case REF_FORTRESS: return _("Defends an area.");
	case REF_GENERIC: return stats->numWeaps > 0 ? _("Defends an area.") : (stats->pSensor != nullptr ? _("Spots targets for artillery.") : "");
	case REF_DEMOLISH: return _("Removes one of your structures, giving back part of its cost.");
	default: return "";
	}
}

void weaponLines(Body &body, const WEAPON_STATS &weapon, unsigned player)
{
	const int range = proj_GetLongRange(weapon, player);
	const int rof = weaponROF(weapon, player);
	body.line(astringf(_("%s: damage %d, range %.1f tiles, %d shots a minute"), getLocalizedStatsName(&weapon), weaponDamage(weapon, player),
	                   static_cast<double>(range) / TILE_UNITS, rof));
}

void queueSection(Body &body, STRUCTURE *psStruct)
{
	const auto queue = hudFactoryQueue(psStruct);
	if (queue.empty())
	{
		return;
	}
	unsigned total = 0;
	for (const auto &entry : queue)
	{
		total += entry.remaining;
	}
	body.section(astringf(ngettext("Queue: %u unit", "Queue: %u units", total), total).c_str());
	for (size_t i = 0; i < queue.size() && i < MAX_QUEUE_LINES; ++i)
	{
		body.line(astringf("%s %s ×%u", i == 0 ? "▸" : "·", getLocalizedStatsName(queue[i].psTemplate), queue[i].remaining));
	}
	if (queue.size() > MAX_QUEUE_LINES)
	{
		body.line(astringf(_("+%u more"), static_cast<unsigned>(queue.size() - MAX_QUEUE_LINES)), WZCOL_TEXT_DARK);
	}
}

unsigned droidCount(unsigned player)
{
	return getNumDroids(player) + getNumMissionDroids(player) + getNumTransporterDroids(player);
}

}

std::shared_ptr<WIDGET> hudStructureDetail(STRUCTURE_STATS *stats, unsigned constructionRate)
{
	if (stats == nullptr || selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}
	const unsigned player = selectedPlayer;
	std::vector<std::shared_ptr<WIDGET>> statCells;
	const bool demolish = stats->type == REF_DEMOLISH;
	if (!demolish)
	{
		statCells.push_back(powerStat(stats->powerToBuild));
		if (constructionRate > 0)
		{
			statCells.push_back(textStat(_("Time"), WzString::fromUtf8(hudFormatDuration(hudWorkSeconds(stats->buildPoints, constructionRate))),
			                             _("With the trucks that would build it")));
		}
	}
	const unsigned limit = stats->upgrade[player].limit;
	const unsigned count = stats->curCount[player];
	bool atLimit = false;
	if (!demolish && limit != LOTS_OF)
	{
		atLimit = count >= limit;
		statCells.push_back(textStat(_("Built"), WzString::fromUtf8(astringf("%u/%u", count, limit)), _("How many you have, of how many you may have")));
	}

	std::shared_ptr<WIDGET> group;
	if (!demolish)
	{
		if (const HudBuildCategory *category = hudFindBuildCategory(hudBuildCategoryOf(stats)))
		{
			group = makeDetailPanelBadge(category->name, pal_RGBA(40, 40, 40, 255), WZCOL_FORM_TEXT);
		}
	}

	Body body(hudDetailWidth() - PADDING * 2);
	const char *purpose = structurePurpose(stats);
	if (purpose[0] != '\0')
	{
		body.line(purpose);
	}
	if (!demolish)
	{
		const auto &upgrade = stats->upgrade[player];
		body.line(astringf(_("Hit points: %u · Armor: %u · Thermal armor: %u"), upgrade.hitpoints, upgrade.armour, upgrade.thermal));
		for (unsigned i = 0; i < stats->numWeaps && i < MAX_WEAPONS; ++i)
		{
			if (stats->psWeapStat[i] != nullptr)
			{
				weaponLines(body, *stats->psWeapStat[i], player);
			}
		}
		if (stats->pSensor != nullptr && (stats->numWeaps > 0 || stats->type == REF_DEFENSE || stats->type == REF_GENERIC))
		{
			body.line(astringf(_("Sensor range: %.1f tiles"), static_cast<double>(sensorRange(*stats->pSensor, player)) / TILE_UNITS));
		}
	}
	if (atLimit)
	{
		body.line(_("⚠ Limit reached"), currentHudSkin().toneColor(HudTone::Warn));
	}

	std::shared_ptr<WIDGET> status;
	if (!statCells.empty() && selectedPlayer < MAX_PLAYERS && getPowerMinusQueued(player) < static_cast<int32_t>(stats->powerToBuild))
	{
		status = statusLine(WzString::fromUtf8(_("✖ Not enough power right now")), HudTone::Bad);
	}
	return finish(structureStatsModel(stats), WzString::fromUtf8(getLocalizedStatsName(stats)), nullptr, group, status, std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudTemplateDetail(DROID_TEMPLATE *psTemplate, STRUCTURE *factory)
{
	if (psTemplate == nullptr || selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}
	const unsigned player = selectedPlayer;
	std::vector<std::shared_ptr<WIDGET>> statCells;
	const unsigned cost = calcTemplatePower(psTemplate);
	statCells.push_back(powerStat(cost));
	const int rate = factory != nullptr ? getBuildingProductionPoints(factory) : 0;
	if (rate > 0)
	{
		statCells.push_back(textStat(_("Time"), WzString::fromUtf8(hudFormatDuration(hudWorkSeconds(calcTemplateBuild(psTemplate), rate))),
		                             _("At this factory")));
	}
	statCells.push_back(textStat(_("Units"), WzString::fromUtf8(astringf("%u/%d", droidCount(player), getMaxDroids(player))),
	                             _("How many units you have, of how many you may have")));

	WzString parts;
	auto addPart = [&parts](const BASE_STATS *part) {
		if (part == nullptr || part->name.isEmpty())
		{
			return;
		}
		if (!parts.isEmpty())
		{
			parts += WzString::fromUtf8(" · ");
		}
		parts += WzString::fromUtf8(getLocalizedStatsName(part));
	};
	addPart(psTemplate->getBodyStats());
	addPart(psTemplate->getPropulsionStats());
	for (int i = 0; i < psTemplate->numWeaps; ++i)
	{
		addPart(psTemplate->getWeaponStats(i));
	}

	Body body(hudDetailWidth() - PADDING * 2);
	const BODY_STATS *bodyStats = psTemplate->getBodyStats();
	if (bodyStats != nullptr)
	{
		body.line(astringf(_("Hit points: %u · Armor: %d · Thermal armor: %d"), calcTemplateBody(psTemplate, player),
		                   bodyArmour(*bodyStats, player, WC_KINETIC), bodyArmour(*bodyStats, player, WC_HEAT)));
	}
	const PROPULSION_STATS *propulsion = psTemplate->getPropulsionStats();
	if (propulsion != nullptr)
	{
		const unsigned weight = calcDroidWeight(psTemplate);
		const unsigned baseSpeed = calcDroidBaseSpeed(psTemplate, weight, player);
		const unsigned propIndex = psTemplate->asParts[COMP_PROPULSION];
		const unsigned onRoad = calcDroidSpeed(baseSpeed, TER_ROAD, propIndex, 0);
		const unsigned offRoad = calcDroidSpeed(baseSpeed, TER_SANDYBRUSH, propIndex, 0);
		body.line(onRoad == offRoad ? astringf(_("Speed: %u"), onRoad) : astringf(_("Speed: %u on road, %u off road"), onRoad, offRoad));
	}
	for (int i = 0; i < psTemplate->numWeaps; ++i)
	{
		if (const WEAPON_STATS *weapon = psTemplate->getWeaponStats(i))
		{
			weaponLines(body, *weapon, player);
		}
	}

	std::shared_ptr<WIDGET> status;
	if (IsPlayerDroidLimitReached(player))
	{
		status = statusLine(WzString::fromUtf8(_("⚠ Unit limit reached")), HudTone::Warn);
	}
	else if (getPowerMinusQueued(player) < static_cast<int32_t>(cost))
	{
		status = statusLine(WzString::fromUtf8(_("✖ Not enough power right now")), HudTone::Bad);
	}
	return finish(templateModel(psTemplate), WzString::fromUtf8(getLocalizedStatsName(psTemplate)), subtitleLabel(parts), nullptr, status,
	              std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudResearchDetail(RESEARCH *research)
{
	if (research == nullptr || selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}
	ResearchTreeContext context;
	context.source = ResearchTreeContext::Source::LivePlayerState;
	context.player = selectedPlayer;
	context.viewer = selectedPlayer;
	ResearchDetailOptions options;
	options.style = hudPanelStyle();
	options.compact = true;
	options.small = hudDetailSmall();
	options.routineStatus = false;
	options.requirements = false;
	ResearchDetailSubject subject = researchDetailSubjectForPlayer(research->index, selectedPlayer);
	if (!subject.stepText.isEmpty())
	{
		subject.stepText = WzString::fromUtf8(astringf("%u / %u", static_cast<unsigned>(research->categoryProgress), static_cast<unsigned>(research->categoryMax)));
	}
	auto contents = makeResearchTopicDetailContents(subject, context, hudDetailWidth(), hudDetailWidth() + 60, ResearchDetailActions(), options);
	if (contents != nullptr)
	{
		contents->setMaxHeight(hudDetailMaxHeight());
	}
	return contents;
}

std::shared_ptr<WIDGET> hudObjectDetail(BASE_OBJECT *object)
{
	if (object == nullptr || object->died || object->player != selectedPlayer)
	{
		return nullptr;
	}
	std::vector<std::shared_ptr<WIDGET>> statCells;
	Body body(hudDetailWidth() - PADDING * 2);
	std::shared_ptr<WIDGET> status;
	WzString title;
	WzString subtitle;

	if (object->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = static_cast<STRUCTURE *>(object);
		title = WzString::fromUtf8(getLocalizedStatsName(psStruct->pStructureType));
		const auto job = hudStructureJob(psStruct);
		if (psStruct->isFactory())
		{
			FACTORY *factory = StructureGetFactory(psStruct);
			if (job.has_value() && job->subject != nullptr)
			{
				subtitle = WzString::fromUtf8(astringf(_("Producing: %s"), getLocalizedStatsName(job->subject)));
			}
			if (factory->psAssemblyPoint != nullptr)
			{
				body.line(astringf(_("Delivery point %u"), factory->psAssemblyPoint->factoryInc + 1));
			}
			if (factory->productionLoops > 0)
			{
				body.line(factory->productionLoops == INFINITE_PRODUCTION ? std::string(_("↻ Repeats forever"))
				          : astringf(ngettext("↻ Repeats %u more time", "↻ Repeats %u more times", factory->productionLoops), static_cast<unsigned>(factory->productionLoops)));
			}
			if (factory->psCommander != nullptr)
			{
				body.line(astringf(_("Units go to commander %s"), droidGetName(factory->psCommander)));
			}
			body.line(astringf(_("Production: %u points a second"), job.has_value() ? job->pointsPerSecond : 0));
		}
		else if (job.has_value())
		{
			if (job->subject != nullptr)
			{
				subtitle = WzString::fromUtf8(astringf(_("Researching: %s"), getLocalizedStatsName(job->subject)));
			}
			body.line(astringf(_("Research: %u points a second"), job->pointsPerSecond));
		}
		if (job.has_value())
		{
			if (job->onHold)
			{
				status = statusLine(WzString::fromUtf8(_("‖ On hold")), HudTone::Warn);
			}
			else if (job->powerNeeded.has_value())
			{
				status = statusLine(WzString::fromUtf8(astringf(_("⚡ Waiting for power: %d"), job->powerNeeded.value())), HudTone::Warn);
			}
			else if (job->subject == nullptr)
			{
				status = statusLine(WzString::fromUtf8(_("● Idle")), HudTone::Neutral);
			}
		}
		const unsigned hp = psStruct->structureBody();
		body.line(astringf(_("Hit points: %u of %u"), psStruct->body, hp));
		queueSection(body, psStruct);
	}
	else if (object->type == OBJ_DROID)
	{
		DROID *psDroid = static_cast<DROID *>(object);
		title = WzString::fromUtf8(droidGetName(psDroid));
		subtitle = WzString::fromUtf8(getDroidLevelName(psDroid));
		statCells.push_back(textStat(_("Health"), WzString::fromUtf8(astringf("%u%%", psDroid->originalBody > 0 ? psDroid->body * 100 / psDroid->originalBody : 0))));
		if (psDroid->droidType == DROID_COMMAND && psDroid->psGroup != nullptr)
		{
			statCells.push_back(textStat(_("Units"), WzString::fromUtf8(astringf("%u / %u", psDroid->psGroup->getNumMembers(), cmdDroidMaxGroup(psDroid)))));
		}
		if (DroidIsBuilding(psDroid))
		{
			if (STRUCTURE *target = DroidGetBuildStructure(psDroid))
			{
				body.line(astringf(_("Building %s"), getLocalizedStatsName(target->pStructureType)));
			}
		}
		else if (DroidGoingToBuild(psDroid))
		{
			if (BASE_STATS *target = DroidGetBuildStats(psDroid))
			{
				body.line(astringf(_("Going to build %s"), getLocalizedStatsName(target)));
			}
		}
		else if (psDroid->action != DACTION_NONE)
		{
			body.line(getDroidActionName(psDroid->action));
		}
		if (droidIsIdleTruck(psDroid))
		{
			status = statusLine(WzString::fromUtf8(_("● Idle")), HudTone::Neutral);
		}
	}
	else
	{
		return nullptr;
	}
	return finish(objectModel(object), title, subtitleLabel(subtitle), nullptr, status, std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudReticuleDetail(int slot, const std::string &tip, const std::string &keyFunction)
{
	if (tip.empty())
	{
		return nullptr;
	}
	std::vector<std::shared_ptr<WIDGET>> statCells;
	const std::string key = keyFunction.empty() ? std::string() : hudKeyBindingText(keyFunction);
	if (!key.empty() && tip.find("(" + key + ")") == std::string::npos)
	{
		statCells.push_back(textStat(_("Key"), WzString::fromUtf8(key)));
	}
	Body body(hudDetailWidth() - PADDING * 2);
	unsigned idleFactories = 0;
	unsigned idleLabs = 0;
	unsigned idleTrucks = 0;
	if (selectedPlayer < MAX_PLAYERS)
	{
		for (STRUCTURE *psStruct : gameWorld.objects.structures[selectedPlayer])
		{
			if (structureIsIdlePending(psStruct))
			{
				(psStruct->pStructureType->type == REF_RESEARCH ? idleLabs : idleFactories) += 1;
			}
		}
		for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
		{
			idleTrucks += droidIsIdleTruck(psDroid) ? 1 : 0;
		}
	}
	std::shared_ptr<WIDGET> status;
	switch (slot)
	{
	case RETBUT_FACTORY:
		body.line(_("Your factories, and the units they can produce."));
		if (idleFactories > 0)
		{
			status = statusLine(WzString::fromUtf8(astringf(ngettext("● %u factory idle", "● %u factories idle", idleFactories), idleFactories)), HudTone::Warn);
		}
		break;
	case RETBUT_RESEARCH:
		body.line(_("Your research facilities, and the topics they can research."));
		if (idleLabs > 0)
		{
			status = statusLine(WzString::fromUtf8(astringf(ngettext("● %u research facility idle", "● %u research facilities idle", idleLabs), idleLabs)), HudTone::Warn);
		}
		break;
	case RETBUT_BUILD:
		body.line(_("Your trucks, and the structures they can build."));
		if (idleTrucks > 0)
		{
			status = statusLine(WzString::fromUtf8(astringf(ngettext("● %u truck idle", "● %u trucks idle", idleTrucks), idleTrucks)), HudTone::Warn);
		}
		break;
	case RETBUT_DESIGN:
		body.line(_("Design new units from the components you have researched."));
		break;
	case RETBUT_INTELMAP:
		body.line(_("Messages, research results and mission briefings."));
		break;
	case RETBUT_COMMAND:
		body.line(_("Your commanders, and their orders."));
		break;
	default:
		body.line(_("Closes the open panel."));
		break;
	}
	return finish(nullptr, WzString::fromUtf8(tip), nullptr, nullptr, status, std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudPowerDetail()
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}
	const auto &power = hudModel().power();
	std::vector<std::shared_ptr<WIDGET>> statCells;
	statCells.push_back(makeDetailPanelStat(WzString::fromUtf8(_("Power")), WzString::number(power.current), IMAGE_DES_POWER, _("What you have")));
	statCells.push_back(textStat(_("Queued"), WzString::number(power.queued), _("What waiting production and research will take")));
	statCells.push_back(textStat(_("Income"), power.incomePerMinute.has_value() ? WzString::fromUtf8(astringf(_("%d/min"), power.incomePerMinute.value())) : WzString::fromUtf8("…"),
	                             _("Averaged over the last minute")));
	Body body(hudDetailWidth() - PADDING * 2);
	body.line(power.maxStorage < UNLIMITED_STORAGE ? astringf(_("After what is queued: %d · Storage: %d"), power.afterQueued, power.maxStorage)
	                                               : astringf(_("After what is queued: %d"), power.afterQueued));
	body.line(power.derrickSummary(),
	          power.unusedDerricks > 0 ? currentHudSkin().toneColor(HudTone::Warn) : WZCOL_TEXT_MEDIUM);
	if (power.unusedDerricks > 0)
	{
		body.line(_("Each power generator takes up to four derricks."));
	}
	std::shared_ptr<WIDGET> status;
	if (power.afterQueued < 0)
	{
		status = statusLine(WzString::fromUtf8(_("✖ Queued items cost more than you have")), HudTone::Bad);
	}
	return finish(nullptr, WzString::fromUtf8(_("Power")), nullptr, nullptr, status, std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudGroupDetail(unsigned group)
{
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return nullptr;
	}
	HudUnitCounts counts = {};
	unsigned total = 0;
	unsigned damaged = 0;
	DROID *representative = nullptr;
	for (DROID *psDroid : gameWorld.objects.droids[selectedPlayer])
	{
		if (psDroid->group != group || psDroid->died)
		{
			continue;
		}
		++total;
		++counts[static_cast<size_t>(hudUnitTypeOf(psDroid))];
		if (psDroid->body < psDroid->originalBody)
		{
			++damaged;
		}
		if (representative == nullptr)
		{
			representative = psDroid;
		}
	}
	if (total == 0)
	{
		return nullptr;
	}
	std::vector<std::shared_ptr<WIDGET>> statCells;
	statCells.push_back(textStat(_("Units"), WzString::number(total)));
	statCells.push_back(textStat(_("Damaged"), WzString::number(damaged)));
	Body body(hudDetailWidth() - PADDING * 2);
	body.section(_("Made of"));
	for (size_t i = 0; i < HUD_UNIT_TYPE_COUNT; ++i)
	{
		if (counts[i] > 0)
		{
			body.line(astringf("%s %u %s", hudUnitTypeSymbol(static_cast<HudUnitType>(i)), counts[i], hudUnitTypeName(static_cast<HudUnitType>(i), counts[i])));
		}
	}
	return finish(objectModel(representative), WzString::fromUtf8(astringf(_("Group %u"), group)), nullptr, nullptr, nullptr, std::move(statCells), body.widget());
}

std::shared_ptr<WIDGET> hudGroupEmptyDetail(unsigned group)
{
	return finish(nullptr, WzString::fromUtf8(astringf(_("Group %u"), group)), subtitleLabel(WzString::fromUtf8(_("Empty"))), nullptr, nullptr, {}, nullptr);
}
