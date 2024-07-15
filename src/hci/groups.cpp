/*
	This file is part of Warzone 2100.
	Copyright (C) 2022-2023  Warzone 2100 Project

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

#include "../hci.h"
#include "groups.h"
#include "objects_stats.h"
#include "../group.h"

static bool groupButtonEnabled = true;

void setGroupButtonEnabled(bool bNewState)
{
	groupButtonEnabled = bNewState;
}

bool getGroupButtonEnabled()
{
	return groupButtonEnabled;
}

class GroupsUIController: public std::enable_shared_from_this<GroupsUIController>
{
public:
	struct GroupDisplayInfo
	{
		size_t numberInGroup = 0;
		size_t numberDamagedInGroup = 0;
		size_t numberCommandedByGroup = 0; // the number of droids commanded by commanders in this group
		uint64_t totalGroupMaxHealth = 0;
		DROID_TEMPLATE displayDroidTemplate;

		uint8_t currAttackGlowAlpha = 0;

		// used for calculating display info
		uint32_t lastAttackedGameTime = 0;
		uint64_t lastAccumulatedDamage = 0;
		uint64_t lastUnitsKilled = 0;
	};
public:
	GroupDisplayInfo* getGroupInfoAt(size_t index)
	{
		ASSERT_OR_RETURN(nullptr, index < groupInfo.size(), "Invalid group index (%zu); max: (%zu)", index, groupInfo.size());
		return &groupInfo[index];
	}

	size_t size() const
	{
		return groupInfo.size();
	}

	void updateData();

	void addGroupDamageForCurrentTick(size_t group, uint64_t additionalDamage, bool unitKilled)
	{
		auto& curr = groupInfo[group];
		curr.lastAttackedGameTime = gameTime;
		curr.lastAccumulatedDamage += additionalDamage;
		if (unitKilled)
		{
			++curr.lastUnitsKilled;
			curr.lastAccumulatedDamage = std::max<uint64_t>(curr.lastAccumulatedDamage, 1);
		}
	}

	void addCommanderGroupDamageForCurrentTick(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled)
	{
		if (psCommander->group >= groupInfo.size())
		{
			return;
		}
		addGroupDamageForCurrentTick(psCommander->group, additionalDamage, unitKilled);
	}

	void selectGroup(size_t groupNumber)
	{
		// select the group
		kf_SelectGrouping(groupNumber);
	}

	void assignSelectedDroidsToGroup(size_t groupNumber)
	{
		assignObjectToGroup(selectedPlayer, groupNumber, true);
	}

private:
	std::array<GroupDisplayInfo, 10> groupInfo;
};

class GroupButton : public DynamicIntFancyButton
{
private:
	typedef DynamicIntFancyButton BaseWidget;
	std::shared_ptr<GroupsUIController> controller;
	std::shared_ptr<W_LABEL> groupNumberLabel;
	std::shared_ptr<W_LABEL> groupCountLabel;
	std::shared_ptr<W_LABEL> groupDamagedCountLabel;
	size_t groupNumber;
	uint32_t lastUpdatedGlowAlphaTime = 0;
protected:
	GroupButton() : DynamicIntFancyButton() { }
public:
	static std::shared_ptr<GroupButton> make(const std::shared_ptr<GroupsUIController>& controller, size_t groupNumber)
	{
		class make_shared_enabler: public GroupButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->groupNumber = groupNumber;
		widget->initialize();
		return widget;
	}

	void initialize()
	{
		attach(groupNumberLabel = std::make_shared<W_LABEL>());
		groupNumberLabel->setGeometry(OBJ_TEXTX, OBJ_B1TEXTY - 5, 16, 16);
		groupNumberLabel->setString(WzString::fromUtf8(astringf("%u", groupNumber)));
		groupNumberLabel->setTransparentToMouse(true);

		attach(groupCountLabel = std::make_shared<W_LABEL>());
		groupCountLabel->setGeometry(OBJ_TEXTX + 40, OBJ_B1TEXTY + 20, 16, 16);
		groupCountLabel->setString("");
		groupCountLabel->setTransparentToMouse(true);

		attach(groupDamagedCountLabel = std::make_shared<W_LABEL>());
		groupDamagedCountLabel->setGeometry(0, 0, 16, 16);
		groupDamagedCountLabel->setFontColour(pal_RGBA(255, 0, 0, 255) /* red */);
		groupDamagedCountLabel->setString("");
		groupDamagedCountLabel->setTransparentToMouse(true);

		buttonBackgroundEmpty = true;

		const std::string groupNumberStr = std::to_string(groupNumber);
		setTip(_("Select / Assign Group Number: ") + groupNumberStr);

		auto helpInfo = WidgetHelp()
		  .setTitle(WzString::fromUtf8(astringf(_("Group %u"), groupNumber)))
		  .addInteraction({WidgetHelp::InteractionTriggers::PrimaryClick}, WzString::fromUtf8(astringf(_("Select the Units in Group %u"), groupNumber)))
		  .addInteraction({WidgetHelp::InteractionTriggers::SecondaryClick, WidgetHelp::InteractionTriggers::ClickAndHold}, WzString::fromUtf8(astringf(_("Assign Selected Units to Group %u"), groupNumber)))
		  .addInteraction({WidgetHelp::InteractionTriggers::Misc},  _("Center Camera on this Group by clicking or tapping twice"))
		  .addRelatedKeybinding("SelectGrouping_" + groupNumberStr)
		  .addRelatedKeybinding("AssignGrouping_" + groupNumberStr)
		  .addRelatedKeybinding("AddGrouping_" + groupNumberStr);

		setHelp(helpInfo);
	}

	void clickPrimary() override
	{
		controller->selectGroup(groupNumber);
	}

	void clickSecondary(bool synthesizedFromHold) override
	{
		controller->assignSelectedDroidsToGroup(groupNumber);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		auto groupInfo = controller->getGroupInfoAt(groupNumber);
		if (!groupInfo)
		{
			return;
		}

		if (groupInfo->lastAccumulatedDamage > 0)
		{
			groupInfo->currAttackGlowAlpha = std::max<uint8_t>(groupInfo->currAttackGlowAlpha, accumulatedDamageToTargetGlowAlpha(groupInfo->lastAccumulatedDamage, groupInfo->totalGroupMaxHealth, groupInfo->lastUnitsKilled));
			lastUpdatedGlowAlphaTime = realTime;
			// reset consumed values
			groupInfo->lastAccumulatedDamage = 0;
			groupInfo->lastUnitsKilled = 0;
		}
		else if (groupInfo->currAttackGlowAlpha > 0 && (realTime - lastUpdatedGlowAlphaTime) > 100)
		{
			auto fadeAmount = 50 * (realTime - lastUpdatedGlowAlphaTime) / GAME_TICKS_PER_SEC;
			if (fadeAmount > static_cast<int32_t>(groupInfo->currAttackGlowAlpha))
			{
				groupInfo->currAttackGlowAlpha = 0;
			}
			else
			{
				groupInfo->currAttackGlowAlpha -= static_cast<uint8_t>(fadeAmount);
			}
			lastUpdatedGlowAlphaTime = realTime;
		}

		if (!groupInfo->numberInGroup && !groupInfo->numberDamagedInGroup)
		{
			groupCountLabel->setString("");
			displayBlank(xOffset, yOffset, false);
		}
		else
		{
			displayIMD(AtlasImage(), ImdObject::DroidTemplate(&(groupInfo->displayDroidTemplate)), xOffset, yOffset);
			groupCountLabel->setString(WzString::fromUtf8(astringf("%u", groupInfo->numberInGroup)));
			int32_t xNumberOffset = 0;
			const uint32_t xFitNumberInTheBox = 16;
			if (groupCountLabel->getMaxLineWidth() > xFitNumberInTheBox)
			{
				xNumberOffset -= groupCountLabel->getMaxLineWidth() - xFitNumberInTheBox;
			}
			groupCountLabel->move(
				OBJ_TEXTX + 40 + xNumberOffset - groupDamagedCountLabel->getMaxLineWidth(),
				groupCountLabel->y()
			);
		}

		if (!groupInfo->numberDamagedInGroup)
		{
			groupDamagedCountLabel->setString("");
		}
		else
		{
			groupDamagedCountLabel->setString(WzString::fromUtf8(astringf("+%u", groupInfo->numberDamagedInGroup)));
			groupDamagedCountLabel->move(groupCountLabel->x() + groupCountLabel->getMaxLineWidth(), groupCountLabel->y());
		}

		if (groupInfo->currAttackGlowAlpha > 0)
		{
			// Based on damage, display an inner glow (animated)
			iV_DrawImageTint(IntImages, IMAGE_BUT_INNER_GLOW, xOffset + x(), yOffset + y(), pal_RGBA(170, 0, 0, groupInfo->currAttackGlowAlpha));
		}
	}
	bool isHighlighted() const override
	{
		return false;
	}
private:
	#define MAX_DAMAGE_GLOW_ALPHA 100
	inline uint8_t accumulatedDamageToTargetGlowAlpha(uint64_t accumulatedDamage, uint64_t totalGroupMaxHealth, uint64_t unitsKilled)
	{
		uint64_t damageVisualPercent = (totalGroupMaxHealth > 0) ? ((accumulatedDamage * 100) / totalGroupMaxHealth) : 0;
		if (unitsKilled > 0)
		{
			damageVisualPercent = 100;
		}
		return static_cast<uint8_t>((std::min<uint64_t>(damageVisualPercent, 100) * MAX_DAMAGE_GLOW_ALPHA) / 100);
	}
};

void GroupsForum::display(int xOffset, int yOffset)
{
	// draw the background
	BaseWidget::display(xOffset, yOffset);
}

void GroupsForum::initialize()
{
	groupsUIController = std::make_shared<GroupsUIController>();

	// the layout should be like this when the build menu is open
	id = IDOBJ_GROUP;
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(GROUP_BACKX, GROUP_BACKY, GROUP_BACKWIDTH, GROUP_BACKHEIGHT);
	}));
	addTabList();
	// create the 11 buttons for each group
	for (size_t i = 1; i <= 10; i++)
	{
		// check if the 10th group works
		auto buttonHolder = std::make_shared<WIDGET>();
		groupsList->addWidgetToLayout(buttonHolder);
		auto groupButton = makeGroupButton(i % 10);
		buttonHolder->attach(groupButton);
		groupButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	}

	WzString descriptionStr = _("View and configure groups of units, which can be quickly selected and ordered.");
	descriptionStr += "\n\n";
	descriptionStr += _("Group buttons will glow red when units are lost (or taking lots of damage).");
	setHelp(WidgetHelp()
	  .setTitle(_("Unit Groups"))
	  .setDescription(descriptionStr)
	  .addInteraction({WidgetHelp::InteractionTriggers::PrimaryClick}, _("Select a Group"))
	  .addInteraction({WidgetHelp::InteractionTriggers::SecondaryClick, WidgetHelp::InteractionTriggers::ClickAndHold}, _("Assign Selected Units to a Group"))
	  .addInteraction({WidgetHelp::InteractionTriggers::Misc},  _("Center Camera on a Group by clicking or tapping twice on the group button")));
}

void GroupsUIController::updateData()
{
	struct AccumulatedGroupInfo
	{
		size_t numberInGroup = 0;
		size_t numberDamagedInGroup = 0;
		size_t numberCommandedByGroup = 0; // the number of droids commanded by commanders in this group
		uint64_t totalGroupMaxHealth = 0;
		DROID *displayDroid = nullptr;
		std::map<std::vector<uint32_t>, size_t> unitcounter;
		size_t most_droids_of_same_type_in_group = 0;
	};

	std::array<AccumulatedGroupInfo, 10> calculatedGroupInfo;
	for (DROID *psDroid : apsDroidLists[selectedPlayer])
	{
		if (psDroid->repairGroup < calculatedGroupInfo.size()) {
			calculatedGroupInfo[psDroid->repairGroup].numberDamagedInGroup++;
		}

		auto groupIdx = psDroid->group;
		if (psDroid->group >= calculatedGroupInfo.size())
		{
			if ((psDroid->psGroup) && (psDroid->psGroup->psCommander) && (psDroid->psGroup->psCommander->group < calculatedGroupInfo.size()))
			{
				groupIdx = psDroid->psGroup->psCommander->group; // accumulate in commander's group
				++(calculatedGroupInfo[groupIdx].numberCommandedByGroup);
			}
			else
			{
				continue;
			}
		}

		auto& currGroupInfo = calculatedGroupInfo[groupIdx];

		// display whatever unit occurs the most in this group
		// find the identifier for this droid
		std::vector<uint32_t> components = buildComponentsFromDroid(psDroid);
		if (++currGroupInfo.unitcounter[components] > currGroupInfo.most_droids_of_same_type_in_group)
		{
			currGroupInfo.most_droids_of_same_type_in_group = currGroupInfo.unitcounter[components];
			currGroupInfo.displayDroid = psDroid;
		}
		currGroupInfo.numberInGroup++;
		currGroupInfo.totalGroupMaxHealth += psDroid->originalBody;
	}

	for (size_t idx = 0; idx < calculatedGroupInfo.size(); ++idx)
	{
		const auto& calculatedInfo = calculatedGroupInfo[idx];
		auto& storedGroupInfo = groupInfo[idx];
		storedGroupInfo.numberInGroup = calculatedInfo.numberInGroup;
		storedGroupInfo.numberDamagedInGroup = calculatedInfo.numberDamagedInGroup;
		storedGroupInfo.numberCommandedByGroup = calculatedInfo.numberCommandedByGroup;
		storedGroupInfo.totalGroupMaxHealth = calculatedInfo.totalGroupMaxHealth;
		if (calculatedInfo.numberInGroup > 0)
		{
			// generate a DROID_TEMPLATE from the displayDroid
			templateSetParts(calculatedInfo.displayDroid, &(storedGroupInfo.displayDroidTemplate));
		}
	}
}

void GroupsForum::updateData()
{
	groupsUIController->updateData();
}

void GroupsForum::updateSelectedGroup(size_t group)
{
	size_t groupButtonIdx = (group > 0) ? group - 1 : 9;
	if (groupButtonIdx >= groupsList->childrenSize())
	{
		return;
	}
	// ensure that the page that contains this group is displayed
	groupsList->goToChildPage(groupButtonIdx);
}

void GroupsForum::addGroupDamageForCurrentTick(size_t group, uint64_t additionalDamage, bool unitKilled)
{
	groupsUIController->addGroupDamageForCurrentTick(group, additionalDamage, unitKilled);
}

void GroupsForum::addCommanderGroupDamageForCurrentTick(const DROID *psCommander, uint64_t additionalDamage, bool unitKilled)
{
	groupsUIController->addCommanderGroupDamageForCurrentTick(psCommander, additionalDamage, unitKilled);
}

void GroupsForum::addTabList()
{
	attach(groupsList = IntListTabWidget::make(TabAlignment::RightAligned));
	groupsList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
	groupsList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int groupListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
	groupsList->setGeometry((OBJ_BACKWIDTH - groupListWidth) / 2, OBJ_TABY, groupListWidth, OBJ_BACKHEIGHT - OBJ_TABY);
	WzString unitGroupsStr = _("Unit Groups:");
	unitGroupsStr += " ";
	groupsList->setTitle(unitGroupsStr);
}

std::shared_ptr<GroupButton> GroupsForum::makeGroupButton(size_t groupNumber)
{
	return GroupButton::make(groupsUIController, groupNumber);
}

void GroupsForum::setHelp(optional<WidgetHelp> _help)
{
	help = _help;
}
