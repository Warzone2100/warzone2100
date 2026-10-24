/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2023  Warzone 2100 Project

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

#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "manufacture.h"
#include "../hud/hudbadges.h"
#include "../hud/huddetailcontents.h"
#include "../hud/hudiconbutton.h"
#include "../hud/hudpanelextension.h"
#include "../hud/hudranks.h"
#include "../hud/hudsettings.h"
#include "../hud/hudskin.h"
#include "../hud/hudtext.h"
#include "../mission.h"
#include "../qtscript.h"
#include "../display3d.h"

STRUCTURE *ManufactureController::highlightedFactory = nullptr;

void ManufactureController::visitUiObjectRefs(const UiObjectRefVisitor &visit)
{
	visitUiObjectRef(highlightedFactory, visit);
}

static const uint8_t nullBrainComponent = 0; // "0" to reference "NULLxxx" components
static const uint8_t commandBrainComponent = 1; // hmm there is only 1 "CommandBrain01" anyway..

FACTORY *getFactoryOrNullptr(STRUCTURE *factory)
{
	ASSERT_OR_RETURN(nullptr, factory && factory->isFactory(), "Invalid factory pointer");
	return (FACTORY *)factory->pFunctionality;
}

static uint8_t getProductionLoops(STRUCTURE *structure)
{
	if (structure == nullptr)
	{
		return 0;
	}

	auto psFactory = getFactoryOrNullptr(structure);
	ASSERT_NOT_NULLPTR_OR_RETURN(0, psFactory);
	return psFactory->productionLoops;
}

/// A looping factory's count: the runs (the loops and the one in progress), or infinity
static std::string loopCountText(uint8_t productionLoops)
{
	switch (productionLoops)
	{
	case 0:
		return "";
	case INFINITE_PRODUCTION:
		return "∞";
	default:
		return astringf("%u", productionLoops + 1);
	}
}

static std::shared_ptr<W_LABEL> makeProductionRunSizeLabel()
{
	auto init = W_LABINIT();
	init.x = OBJ_TEXTX;
	init.y = OBJ_T1TEXTY;
	init.width = 16;
	init.height = 16;
	auto label = std::make_shared<W_LABEL>(&init);
	label->setTransparentToMouse(true);
	return label;
}

void ManufactureController::updateData()
{
	updateFactoriesList();
	updateHighlighted();
	updateManufactureOptionsList();
}

void ManufactureController::adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add)
{
	factoryProdAdjust(getHighlightedObject(), manufactureOption, add);
}

void ManufactureController::adjustFactoryLoop(bool add)
{
	factoryLoopAdjust(getHighlightedObject(), add);
}

void ManufactureController::releaseFactoryProduction(STRUCTURE *structure)
{
	releaseProduction(structure, ModeQueue);
}

void ManufactureController::cancelFactoryProduction(STRUCTURE *structure)
{
	if (!StructureIsManufacturingPending(structure))
	{
		return;
	}

	if (!StructureIsOnHoldPending(structure))
	{
		holdProduction(structure, ModeQueue);
		return;
	}

	cancelProduction(structure, ModeQueue);
	audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
}

void ManufactureController::startDeliveryPointPosition()
{
	auto factory = getHighlightedObject();
	ASSERT_NOT_NULLPTR_OR_RETURN(, factory);

	// make sure that the factory isn't assigned to a commander
	assignFactoryCommandDroid(factory, nullptr, ModeQueue);
	auto psFlag = FindFactoryDelivery(factory);
	if (psFlag)
	{
		startDeliveryPosition(psFlag);
	}
}

static inline bool compareFactories(STRUCTURE *a, STRUCTURE *b)
{
	if (a == nullptr || b == nullptr)
	{
		return (a == nullptr) < (b == nullptr);
	}
	auto x = getFactoryOrNullptr(a);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, x);
	auto y = getFactoryOrNullptr(b);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, y);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, x->psAssemblyPoint);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, y->psAssemblyPoint);
	if (x->psAssemblyPoint->factoryType != y->psAssemblyPoint->factoryType)
	{
		return x->psAssemblyPoint->factoryType < y->psAssemblyPoint->factoryType;
	}

	return x->psAssemblyPoint->factoryInc < y->psAssemblyPoint->factoryInc;
}

void ManufactureController::updateFactoriesList()
{
	factories.clear();

	auto* intStrList = interfaceStructList();
	if (intStrList)
	{
		for (auto structure : *intStrList)
		{
			if (structure->status == SS_BUILT && structure->died == 0 && structure->isFactory())
			{
				factories.push_back(structure);
			}
		}
	}

	std::sort(factories.begin(), factories.end(), compareFactories);
}

void ManufactureController::updateManufactureOptionsList()
{
	if (auto factory = getHighlightedObject())
	{
		stats = fillTemplateList(factory);
	}
	else
	{
		stats.clear();
	}
}

DROID_TEMPLATE *ManufactureController::getObjectStatsAt(size_t objectIndex) const
{
	auto factory = getFactoryOrNullptr(getObjectAt(objectIndex));
	return factory == nullptr ? nullptr : factory->psSubject;
}

void ManufactureController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
}

void ManufactureController::clearData()
{
	factories.clear();
	setHighlightedObject(nullptr, false);
	stats.clear();
}

void ManufactureController::setHighlightedObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject)
{
	if (object == nullptr)
	{
		highlightedFactory = nullptr;
		return;
	}

	auto factory = castStructure(object);
	ASSERT_OR_RETURN(, factory && factory->isFactory(), "Invalid factory pointer");
	queuedJumpToHighlightedStatsObject = queuedJumpToHighlightedStatsObject || jumpToHighlightedStatsObject;
	highlightedFactory = factory;
}

class ManufactureObjectButton : public ObjectButton
{
private:
	typedef ObjectButton BaseWidget;

protected:
	ManufactureObjectButton() {}

public:
	static std::shared_ptr<ManufactureObjectButton> make(const std::shared_ptr<ManufactureController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ManufactureObjectButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->objectIndex = objectIndex;
		widget->initialize();
		return widget;
	}

	void jump() override
	{
		if (!offWorldKeepLists)
		{
			BaseWidget::jump();
		}
	}

	void clickPrimary() override
	{
		controller->clearStructureSelection();
		controller->selectObject(controller->getObjectAt(objectIndex), false);
		jump();
		BaseStatsController::scheduleDisplayStatsForm(controller);
	}

protected:
	void initialize()
	{
		attach(factoryNumberLabel = std::make_shared<W_LABEL>());
		attach(factoryAssignGroupLabel = std::make_shared<W_LABEL>());
		factoryNumberLabel->setGeometry(OBJ_TEXTX, OBJ_B1TEXTY, 16, 16);
		factoryAssignGroupLabel->setGeometry(OBJ_TEXTX + 48, OBJ_B1TEXTY, 16, 16);
		factoryAssignGroupLabel->setFontColour(pal_RGBA(255, 220, 115, 255) /* gold */);
	}

	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		if (isDead(factory))
		{
			ASSERT_FAILURE(!isDead(factory), "!isDead(factory)", AT_MACRO, __FUNCTION__, "Factory is dead");
			// ensure the backing information is refreshed before the next draw
			intRefreshScreen();
			return;
		}
		displayIMD(AtlasImage(), ImdObject::Structure(factory), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto psStruct = controller->getObjectAt(objectIndex);
		auto factory = getFactoryOrNullptr(psStruct);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		if (factory->psAssemblyPoint == nullptr)
		{
			factoryNumberLabel->setString("");
		}
		else
		{
			factoryNumberLabel->setString(WzString::format("%u", factory->psAssemblyPoint->factoryInc + 1));
		}
		if (psStruct->productToGroup != UBYTE_MAX)
		{
			factoryAssignGroupLabel->setString(WzString::format("%u", static_cast<unsigned>(psStruct->productToGroup)));
		}
		else
		{
			factoryAssignGroupLabel->setString("");
		}
	}

	std::string getTip() override
	{
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN("", factory);
		return getLocalizedStatsName(factory->pStructureType);
	}

	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<ManufactureController> controller;
	std::shared_ptr<W_LABEL> factoryNumberLabel;
	std::shared_ptr<W_LABEL> factoryAssignGroupLabel;
};

class ManufactureStatsButton: public StatsButton
{
private:
	typedef	StatsButton BaseWidget;

protected:
	ManufactureStatsButton() {}

public:
	static std::shared_ptr<ManufactureStatsButton> make(const std::shared_ptr<ManufactureController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ManufactureStatsButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->objectIndex = objectIndex;
		widget->initialize();
		return widget;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();

		auto factory = controller->getObjectAt(objectIndex);
		auto production = getStats();
		auto productionPending = factory && StructureIsManufacturingPending(factory);
		auto objectImage = productionPending && production ? ImdObject::DroidTemplate(production): ImdObject::Component(nullptr);
		displayIMD(AtlasImage(), objectImage, xOffset, yOffset);

		if (productionPending && StructureIsOnHoldPending(factory))
		{
			currentHudSkin().drawFancyButtonOnHold(hudLook(xOffset, yOffset));
		}
		else
		{
			displayIfHighlight(xOffset, yOffset);
		}
		drawNextDroidRank(xOffset, yOffset);
		hudDrawJobBadges(factory, xOffset + x(), yOffset + y(), height());
	}

	// show a little icon on the bottom, indicating what rank next unit will be
	// remember that when it's a commander, same rank will require twice experience
	void drawNextDroidRank(int xOffset, int yOffset)
	{
		const auto factory = controller->getObjectAt(objectIndex);
		if (!factory)
		{
			return;
		}
		const auto player = factory->player;
		auto exp = getTopExperience(player);
		unsigned int lvl = 0;
		if (!factory->pFunctionality->factory.psSubject)
		{
			// not showing icon when nothing is being built
			return;
		}
		bool isCommander = factory->pFunctionality->factory.psSubject->droidType == DROID_COMMAND;
		if (isCommander || factory->pFunctionality->factory.psSubject->droidType == DROID_SENSOR)
		{
			if (isCommander)
			{
				exp = std::min(factory->pFunctionality->factory.psSubject->getBrainStats()->productionCommanderExpLimit * 65536, exp);
			}
			lvl = getDroidLevel(exp, player, commandBrainComponent);
		}
		else if (factory->pFunctionality->factory.psSubject->droidType == DROID_CONSTRUCT
		|| factory->pFunctionality->factory.psSubject->droidType == DROID_CYBORG_CONSTRUCT
		|| factory->pFunctionality->factory.psSubject->droidType == DROID_CYBORG_REPAIR
		|| factory->pFunctionality->factory.psSubject->droidType == DROID_REPAIR
		|| factory->pFunctionality->factory.psSubject->droidType == DROID_TRANSPORTER
		|| factory->pFunctionality->factory.psSubject->droidType == DROID_SUPERTRANSPORTER)
		{
			lvl = 0;
		}
		else
		{
			lvl = getDroidLevel(exp, player, nullBrainComponent);
		}
		const auto expgfx = getDroidRankGraphicFromLevel(lvl);
		const bool insignia = currentHudSkin().drawsRankInsignia() && hudDrawRank(lvl, xOffset + 44, yOffset + 4, HudRankSize::Button);
		if (!insignia && expgfx != UDWORD_MAX)
		{
			// FIXME: use offsets relative to template positon, not hardcoded values ?
			iV_DrawImage(IntImages, (UWORD)expgfx, xOffset + 45, yOffset + 4);
		}
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto factory = controller->getObjectAt(objectIndex);
		updateProgressBar(factory);
		updateProductionRunSizeLabel(factory, FactoryGetTemplate(StructureGetFactory(factory)));
	}

private:
	DROID_TEMPLATE *getStats() override
	{
		return controller->getObjectStatsAt(objectIndex);
	}

	void initialize()
	{
		addProgressBar();
		addProductionRunSizeLabel();
	}

	void addProductionRunSizeLabel()
	{
		attach(productionRunSizeLabel = makeProductionRunSizeLabel());
		productionRunSizeLabel->setFontColour(WZCOL_ACTION_PRODUCTION_RUN_TEXT);
	}

	void updateProductionRunSizeLabel(STRUCTURE *factory, DROID_TEMPLATE *droidTemplate)
	{
		auto productionRemaining = getProduction(factory, droidTemplate).numRemaining();
		if (productionRemaining > 0 && factory && StructureIsManufacturingPending(factory) && !hudModuleActive(HudModuleId::ObjectBadges))
		{
			productionRunSizeLabel->setString(WzString::format("%d", productionRemaining));
			productionRunSizeLabel->show();
		}
		else
		{
			productionRunSizeLabel->hide();
		}
	}

	void updateProgressBar(STRUCTURE *factory)
	{
		progressBar->hide();

		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		ASSERT_OR_RETURN(, !isDead(factory), "Factory is dead");

		if (!StructureIsManufacturingPending(factory))
		{
			return;
		}

		auto manufacture = StructureGetFactory(factory);
		ASSERT_NOT_NULLPTR_OR_RETURN(, manufacture);

		if (manufacture->psSubject != nullptr && manufacture->buildPointsRemaining < calcTemplateBuild(manufacture->psSubject))
		{
			// Started production. Set the colour of the bar to yellow.
			int buildPointsTotal = calcTemplateBuild(FactoryGetTemplate(manufacture));
			int buildRate = manufacture->timeStartHold == 0 ? getBuildingProductionPoints(factory) : 0;
			formatTime(progressBar.get(), buildPointsTotal - manufacture->buildPointsRemaining, buildPointsTotal, buildRate, _("Construction Progress"));
		}
		else
		{
			// Not yet started production.
			int neededPower = checkPowerRequest(factory);
			int powerToBuild = manufacture->psSubject ? calcTemplatePower(manufacture->psSubject) : 0;
			formatPower(progressBar.get(), neededPower, powerToBuild);
		}
	}

	bool isHighlighted() const override
	{
		auto factory = controller->getObjectAt(objectIndex);
		return factory && (factory->selected || factory == controller->getHighlightedObject());
	}

	void clickPrimary() override
	{
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		controller->releaseFactoryProduction(factory);
		controller->clearStructureSelection();
		controller->selectObject(factory, true);
		BaseStatsController::scheduleDisplayStatsForm(controller);
	}

	std::vector<HudDetailAction> hudSecondaryActions() override
	{
		auto factory = controller->getObjectAt(objectIndex);
		if (factory == nullptr || !StructureIsManufacturingPending(factory))
		{
			return {};
		}
		// (as the right-click: a first one puts it on hold, the next cancels it)
		if (StructureIsOnHoldPending(factory))
		{
			return {secondaryAction(_("Cancel production"), "never", true)};
		}
		return {secondaryAction(_("Pause production"), "replay-pause")};
	}

	void clickSecondary(bool synthesizedFromHold) override
	{
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		controller->clearStructureSelection();
		controller->cancelFactoryProduction(factory);
		controller->setHighlightedObject(factory, true);
		controller->refresh();
		BaseStatsController::scheduleDisplayStatsForm(controller);
	}

	std::shared_ptr<ManufactureController> controller;
	std::shared_ptr<W_LABEL> productionRunSizeLabel;
	size_t objectIndex;
};

class ManufactureOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureOptionButton> make(const std::shared_ptr<ManufactureController> &controller, size_t manufactureOptionIndex)
	{
		class make_shared_enabler: public ManufactureOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->manufactureOptionIndex = manufactureOptionIndex;
		widget->initialize();
		return widget;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto stat = getStats();
		ASSERT_NOT_NULLPTR_OR_RETURN(, stat);

		displayIMD(AtlasImage(), ImdObject::DroidTemplate(stat), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
		if (!runSizeBadge.empty())
		{
			runSizeBadge.draw(xOffset + x() + 2, yOffset + y() + 2);
		}
	}

private:
	DROID_TEMPLATE *getStats() override
	{
		return controller->getStatsAt(manufactureOptionIndex);
	}

	void initialize()
	{
		addCostBar();
		addProductionRunSizeLabel();
	}

	void addProductionRunSizeLabel()
	{
		attach(productionRunSizeLabel = makeProductionRunSizeLabel());
	}

	void updateProductionRunSizeLabel(STRUCTURE *structure, DROID_TEMPLATE *droidTemplate)
	{
		auto production = getProduction(structure, droidTemplate);
		if (production.isValid())
		{
			auto productionLoops = getProductionLoops(structure);
			auto labelText = astringf(productionLoops > 0 ? "%d/%d" : "%d", production.numRemaining(), production.quantity);
			if (currentHudSkin().drawsProductionRunBadge())
			{
				productionRunSizeLabel->hide();
				runSizeBadge.set(productionLoops > 0 ? labelText : "×" + labelText, false);
				return;
			}
			productionRunSizeLabel->setString(WzString::fromUtf8(labelText));
			productionRunSizeLabel->show();
		}
		else
		{
			productionRunSizeLabel->hide();
		}
		runSizeBadge.set(std::string(), false);
	}

	bool isHighlighted() const override
	{
		return controller->isHighlightedObjectStats(manufactureOptionIndex);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		if (isMouseOverWidget())
		{
			intSetShadowPower(getCost());
		}
		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
		tintCostBar();
		updateProductionRunSizeLabel(controller->getHighlightedObject(), getStats());
	}

	uint32_t getCost() override
	{
		DROID_TEMPLATE* psTemplate = getStats();
		return psTemplate ? calcTemplatePower(psTemplate) : 0;
	}

	std::shared_ptr<WIDGET> makeHudDetail() override
	{
		return hudTemplateDetail(getStats(), controller->getHighlightedObject());
	}

	std::string getTip() override
	{
		std::string tip = BaseWidget::getTip();
		DROID_TEMPLATE *psTemplate = getStats();
		STRUCTURE *factory = controller->getHighlightedObject();
		const int rate = factory != nullptr ? getBuildingProductionPoints(factory) : 0;
		if (psTemplate != nullptr && rate > 0)
		{
			tip += "\n" + astringf(_("Production Time: %s"), hudFormatDuration(hudWorkSeconds(calcTemplateBuild(psTemplate), rate)).c_str());
		}
		return tip;
	}

	void clickPrimary() override
	{
		adjustFactoryProduction(true);
	}

	std::vector<HudDetailAction> hudSecondaryActions() override
	{
		STRUCTURE *factory = controller->getHighlightedObject();
		DROID_TEMPLATE *psTemplate = getStats();
		if (factory == nullptr || psTemplate == nullptr)
		{
			return {};
		}
		auto production = getProduction(factory, psTemplate);
		if (!production.isValid() || production.numRemaining() <= 0)
		{
			return {};
		}
		return {secondaryAction(_("Remove one"), "minimize", false, true, WzString::fromUtf8(astringf(_("%d in the queue"), production.numRemaining())))};
	}

	void clickSecondary(bool synthesizedFromHold) override
	{
		adjustFactoryProduction(false);
	}

	void adjustFactoryProduction(bool add)
	{
		auto clickedStats = controller->getStatsAt(manufactureOptionIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		auto controllerRef = controller;
		widgScheduleTask([add, clickedStats, controllerRef]() {
			controllerRef->adjustFactoryProduction(clickedStats, add);
		});
	}

	std::shared_ptr<ManufactureController> controller;
	std::shared_ptr<W_LABEL> productionRunSizeLabel;
	HudCountBadge runSizeBadge;
	size_t manufactureOptionIndex;
};

class ManufactureObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureObjectsForm> make(const std::shared_ptr<ManufactureController> &controller)
	{
		class make_shared_enabler: public ManufactureObjectsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return ManufactureStatsButton::make(controller, buttonIndex);
	}

	std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const override
	{
		return ManufactureObjectButton::make(controller, buttonIndex);
	}

protected:
	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<ManufactureController> controller;
};

class ManufactureStatsForm: public ObjectStatsForm
{
private:
	typedef ObjectStatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureStatsForm> make(const std::shared_ptr<ManufactureController> &controller)
	{
		class make_shared_enabler: public ManufactureStatsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return ManufactureOptionButton::make(controller, buttonIndex);
	}

protected:
	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	void initialize() override
	{
		BaseWidget::initialize();
		addObsoleteButton();
		attach(deliveryPointButton = std::make_shared<DeliveryPointButton>(controller));
		addLoopProductionWidgets();
	}

	// The buttons 2 px apart, with the loop count on the loop button, to leave room for the pages
	WzRect layoutCompactHeader(int drop) override
	{
		const int y = STAT_SLDY + drop;
		// (the delivery point button takes its size from its images when it's first drawn)
		deliveryPointButton->move(4, y);
		obsoleteButton->move(deliveryPointButton->x() + AtlasImage(IntImages, IMAGE_FDP_UP).width() + 2, y);
		loopProductionButton->move(obsoleteButton->x() + obsoleteButton->width() + 2, y);
		loopProductionButton->countBadge = true;
		loopProductionLabel->setGeometry(loopProductionButton->geometry());
		loopProductionLabel->setTransparentToMouse(true);
		loopProductionLabel->drawn = false;
		for (const auto &control : std::initializer_list<std::shared_ptr<WIDGET>>{deliveryPointButton, obsoleteButton, loopProductionButton, loopProductionLabel})
		{
			hudSetMovedPanelControl(control);
		}
		const int right = loopProductionButton->x() + loopProductionButton->width();
		return WzRect(4, y, right - 4, loopProductionButton->height());
	}

	size_t compactHeaderMaxPages() const override
	{
		return 2;
	}

	void addObsoleteButton()
	{
		auto iconButton = std::make_shared<HudIconChoiceButton>();
		attach(obsoleteButton = iconButton);
		iconButton->setIcon(false, HudIcon::ObsoleteHidden);
		iconButton->setIcon(true, HudIcon::ObsoleteShown);
		obsoleteButton->style |= WBUT_SECONDARY;
		obsoleteButton->setChoice(controller->shouldShowRedundantDesign());
		obsoleteButton->setImages(false, MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
		obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
		obsoleteButton->setImages(true,  MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
		obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
		obsoleteButton->move(4 + AtlasImage(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);

		auto weakController = std::weak_ptr<ManufactureController>(controller);
		obsoleteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto manufactureController = weakController.lock())
			{
				auto &_obsoleteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !_obsoleteButton.getChoice();
				manufactureController->setShouldShowRedundantDesign(newValue);
				_obsoleteButton.setChoice(newValue);
			}
		});
	}

	void addLoopProductionWidgets()
	{
		loopProductionButton = std::make_shared<LoopProductionButton>(controller);
		attach(loopProductionButton);
		loopProductionButton->move(STAT_SLDX + STAT_SLDWIDTH + 2, STAT_SLDY);

		loopProductionLabel = std::make_shared<LoopProductionLabel>(controller, loopProductionButton->height());
		attach(loopProductionLabel);
		loopProductionLabel->setGeometry(loopProductionButton->x() - 15, loopProductionButton->y(), 12, 15);
	}

	class DeliveryPointButton;
	class LoopProductionButton;
	class LoopProductionLabel;

	std::shared_ptr<ManufactureController> controller;
	std::shared_ptr<MultipleChoiceButton> obsoleteButton;
	std::shared_ptr<DeliveryPointButton> deliveryPointButton;
	std::shared_ptr<LoopProductionButton> loopProductionButton;
	std::shared_ptr<LoopProductionLabel> loopProductionLabel;

	class DeliveryPointButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		DeliveryPointButton(const std::shared_ptr<ManufactureController> &controller): BaseWidget(), controller(controller)
		{
			style |= WBUT_SECONDARY;
			move(4, STAT_SLDY);
			setTip(_("Factory Delivery Point"));

			auto weakController = std::weak_ptr<ManufactureController>(controller);
			addOnClickHandler([weakController](W_BUTTON &) {
				auto controller = weakController.lock();
				ASSERT_NOT_NULLPTR_OR_RETURN(, controller);
				controller->startDeliveryPointPosition();
			});
		}

	protected:
		void display(int xOffset, int yOffset)
		{
			updateLayout();
			if (!hudDrawIconButton(*this, HudIcon::DeliveryPoint, false, HudIconButtonStyle::Framed, xOffset, yOffset))
			{
				BaseWidget::display(xOffset, yOffset);
			}
		}

	private:
		void updateLayout()
		{
			auto factory = controller->getHighlightedObject();
			ASSERT_NOT_NULLPTR_OR_RETURN(, factory);

			switch (factory->pStructureType->type)
			{
			default:
			case REF_FACTORY:
				setImages(AtlasImage(IntImages, IMAGE_FDP_UP), AtlasImage(IntImages, IMAGE_FDP_DOWN), AtlasImage(IntImages, IMAGE_FDP_HI));
				break;
			case REF_CYBORG_FACTORY:
				setImages(AtlasImage(IntImages, IMAGE_CDP_UP), AtlasImage(IntImages, IMAGE_CDP_DOWN), AtlasImage(IntImages, IMAGE_CDP_HI));
				break;
			case REF_VTOL_FACTORY:
				setImages(AtlasImage(IntImages, IMAGE_VDP_UP), AtlasImage(IntImages, IMAGE_VDP_DOWN), AtlasImage(IntImages, IMAGE_VDP_HI));
				break;
			}
		}

		std::shared_ptr<ManufactureController> controller;
	};

	class LoopProductionButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		LoopProductionButton(const std::shared_ptr<ManufactureController> &controller): BaseWidget(), controller(controller)
		{
			style |= WBUT_SECONDARY;
			setImages(AtlasImage(IntImages, IMAGE_LOOP_UP), AtlasImage(IntImages, IMAGE_LOOP_DOWN), AtlasImage(IntImages, IMAGE_LOOP_HI));
			setTip(_("Loop Production"));
		}

		void released(W_CONTEXT *psContext, WIDGET_KEY key) override
		{
			BaseWidget::released(psContext, key);
			controller->adjustFactoryLoop(key == WKEY_PRIMARY);
		}

		/// The loop count over its top-right corner (in place of the label on its left)
		bool countBadge = false;

	protected:
		void display(int xOffset, int yOffset) override
		{
			updateLayout();
			if (!hudDrawIconButton(*this, HudIcon::Loop, (getState() & WBUT_CLICKLOCK) != 0, HudIconButtonStyle::Framed, xOffset, yOffset))
			{
				BaseWidget::display(xOffset, yOffset);
			}
			if (countBadge && !badge.empty())
			{
				badge.draw(xOffset + x() + width() + 3 - badge.width(), yOffset + y() - 5);
			}
		}

		void updateLayout()
		{
			const uint8_t loops = getProductionLoops(controller->getHighlightedObject());
			setState(loops == 0 ? 0: WBUT_CLICKLOCK);
			if (countBadge)
			{
				badge.set(loopCountText(loops), false);
			}
		}

	private:
		std::shared_ptr<ManufactureController> controller;
		HudCountBadge badge;
	};

	class LoopProductionLabel: public W_LABEL
	{
	private:
		typedef W_LABEL BaseWidget;

	public:
		LoopProductionLabel(const std::shared_ptr<ManufactureController> &controller, int buttonHeight): BaseWidget(), controller(controller), buttonHeight(buttonHeight)
		{
		}

		/// (the loop button shows the count)
		bool drawn = true;

	protected:
		void display(int xOffset, int yOffset) override
		{
			updateLayout();
			// Centered on the loop button, which is shorter than the label (the label keeps its area, for hit tests)
			if (drawn && !text.getText().isEmpty())
			{
				text.render(xOffset + x(), yOffset + y() + (buttonHeight - text.lineSize()) / 2 - text.aboveBase(), WZCOL_FORM_TEXT);
			}
		}

		void updateLayout()
		{
			auto productionLoops = getProductionLoops(controller->getHighlightedObject());
			if (productionLoops != lastProductionLoop)
			{
				lastProductionLoop = productionLoops;
				setString(WzString::fromUtf8(loopCountText(productionLoops)));
				text.setText(WzString::fromUtf8(loopCountText(productionLoops)), font_regular);
			}
		}

	private:
		uint8_t lastProductionLoop = 0;
		std::shared_ptr<ManufactureController> controller;
		int buttonHeight;
		WzText text;
	};
};

bool ManufactureController::showInterface()
{
	updateData();
	if (factories.empty())
	{
		return false;
	}

	auto objectsForm = ManufactureObjectsForm::make(shared_from_this());
	objectsForm->addHudExtension();
	psWScreen->psForm->attach(objectsForm);
	displayStatsForm();
	triggerEvent(TRIGGER_MENU_MANUFACTURE_UP);
	return true;
}

std::shared_ptr<StatsForm> ManufactureController::makeStatsForm()
{
	return ManufactureStatsForm::make(shared_from_this());
}
