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
#include "lib/widget/bar.h"
#include "lib/ivis_opengl/pieclip.h"
#include "objects_stats.h"
#include "../display3d.h"
#include "../qtscript.h"
#include "../warcam.h"
#include "../geometry.h"
#include "groups.h"
#include "../power.h"
#include "../hud/huddetailcontents.h"
#include "../hud/hudids.h"
#include "../hud/hudpagecontrol.h"
#include "../hud/hudpanelextension.h"
#include "../hud/hudpanelfilters.h"
#include "../hud/hudrowsummary.h"
#include "../hud/hudsettings.h"
#include "../hud/hudskin.h"
#include "../hud/hudlayout.h"

#include <algorithm>
#include <iterator>

// How far the header moves down onto the page tabs' row when the page control is in it (7 px above the items)
static constexpr int COMPACT_HEADER_DROP = 9;
static constexpr int PAGE_ROW_PAD = 2;
static constexpr int PAGE_ROW_HEIGHT_OWN = 16;
static constexpr int PAGE_ROW_VISUAL_HEIGHT = 13;
static constexpr int HEADER_GAP = 4;
static constexpr int HEADER_MENU_X = 4;
static constexpr int HEADER_CONTROLS_TOP = 1;
static constexpr int TOUCH_CLOSE_WIDTH = 24;

void BaseObjectsController::clearSelection()
{
	::clearSelection();
}

void BaseObjectsController::clearStructureSelection()
{
	StructureList* intStrList = interfaceStructList();
	if (intStrList)
	{
		for (auto structure : *intStrList)
		{
			structure->selected = false;
		}
	}
}

void BaseObjectsController::selectObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);
	object->selected = true;
	triggerEventSelected();
	jsDebugSelected(object);
	setHighlightedObject(object, jumpToHighlightedStatsObject);
	refresh();
}

void BaseObjectsController::prepareToClose()
{
	clearData();
}

void BaseObjectsController::jumpToObject(BASE_OBJECT *object)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);
	setPlayerPos(gameWorld.map, object->pos.x, object->pos.y);
	setWarCamActive(false);
}

void BaseObjectsController::updateHighlighted()
{
	if (objectsSize() == 0)
	{
		setHighlightedObject(nullptr, false);
		return;
	}

	auto findAnySelectedObject = [&] (BASE_OBJECT *object) {
		if (object->died == 0 && object->selected)
		{
			setHighlightedObject(object, false);
			return true;
		}

		return false;
	};

	if (findObject(findAnySelectedObject))
	{
		return;
	}

	if (auto highlighted = getHighlightedObject())
	{
		auto findHighlightedObject = [&] (BASE_OBJECT *object) {
			if (object->died == 0 && object == highlighted)
			{
				setHighlightedObject(object, false);
				return true;
			}

			return false;
		};

		if (findObject(findHighlightedObject))
		{
			return;
		}
	}

	setHighlightedObject(getObjectAt(0), false);
}

void BaseStatsController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto newStatsForm = makeStatsForm();
		newStatsForm->addHudExtension();
		psWScreen->psForm->attach(newStatsForm);
		intMode = INT_STAT;
		setSecondaryWindowUp(true);
	}
}

// To be called from within widget click & run handlers / functions
void BaseStatsController::scheduleDisplayStatsForm(const std::shared_ptr<BaseStatsController>& controller)
{
	std::weak_ptr<BaseStatsController> psWeakController = controller;
	widgScheduleTask([psWeakController]() {
		if (auto psStrongController = psWeakController.lock())
		{
			psStrongController->displayStatsForm();
		}
	});
}

DynamicIntFancyButton::DynamicIntFancyButton()
{
	style |= WFORM_SECONDARY;
	setHudSkinned(true);
}

void DynamicIntFancyButton::updateLayout()
{
	updateHighlight();
	initDisplay();
}

void DynamicIntFancyButton::released(W_CONTEXT *context, WIDGET_KEY mouseButton)
{
	bool clickAndReleaseOnThisButton = ((state & WBUT_DOWN) != 0); // relies on W_CLICKFORM handling to properly set WBUT_DOWN

	IntFancyButton::released(context, mouseButton);

	if (!clickAndReleaseOnThisButton)
	{
		return; // do nothing
	}

	if (mouseButton == WKEY_PRIMARY)
	{
		clickPrimary();
	}
	else if (mouseButton == WKEY_SECONDARY)
	{
		clickSecondary(false);
	}
}

HudDetailAction DynamicIntFancyButton::secondaryAction(const WzString &label, const char *icon, bool destructive, bool enabled, const WzString &note)
{
	HudDetailAction action;
	action.label = label;
	action.icon = icon;
	action.note = note;
	action.destructive = destructive;
	action.enabled = enabled;
	std::weak_ptr<WIDGET> weakSelf = shared_from_this();
	action.run = [weakSelf]() {
		if (auto button = std::dynamic_pointer_cast<DynamicIntFancyButton>(weakSelf.lock()))
		{
			button->runSecondary();
		}
	};
	return action;
}

std::vector<HudDetailAction> hudObjectColumnActions(const std::shared_ptr<WIDGET> &holder)
{
	std::vector<HudDetailAction> result;
	if (holder == nullptr)
	{
		return result;
	}
	for (bool objectFirst : {true, false})
	{
		for (const auto &child : holder->children())
		{
			auto button = dynamic_cast<DynamicIntFancyButton *>(child.get());
			if (button == nullptr || (dynamic_cast<ObjectButton *>(button) != nullptr) != objectFirst)
			{
				continue;
			}
			for (auto &action : button->hudSecondaryActions())
			{
				result.push_back(std::move(action));
			}
		}
	}
	return result;
}

std::vector<HudDetailAction> ObjectButton::hudDetailActions()
{
	return hudObjectColumnActions(parent());
}

std::vector<HudDetailAction> ObjectButton::hudSecondaryActions()
{
	return {secondaryAction(_("Go to"), "show-on-map")};
}

bool DynamicIntFancyButton::clickHeld(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (key == WKEY_PRIMARY)
	{
		clickSecondary(true);
		return true;
	}
	return false;
}

void StatsButton::addProgressBar()
{
	auto init = W_BARINIT();
	init.x = STAT_PROGBARX;
	init.y = STAT_PROGBARY;
	init.width = STAT_PROGBARWIDTH;
	init.height = STAT_PROGBARHEIGHT;
	init.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	init.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	init.pTip = _("Progress Bar");
	init.style = WBAR_TROUGH | WIDG_HIDDEN;
	init.iRange = GAME_TICKS_PER_SEC;
	attach(progressBar = std::make_shared<HudButtonBar>(&init));
	progressBar->setBackgroundColour(WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND);
}

void ObjectButton::jump()
{
	auto object = getController().getObjectAt(objectIndex);
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);

	if ((jumpPosition.x == 0 && jumpPosition.y == 0) || !objectOnScreen(object, 0))
	{
		jumpPosition = getPlayerPos();
		getController().jumpToObject(object);
	}
	else
	{
		setPlayerPos(gameWorld.map, jumpPosition.x, jumpPosition.y);
		setWarCamActive(false);
		jumpPosition = {0, 0};
	}
}

void StatsFormButton::addCostBar()
{
	W_BARINIT sBarInit;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.y = STAT_TIMEBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

	sBarInit.iRange = GAME_TICKS_PER_SEC;
	attach(costBar = std::make_shared<HudButtonBar>(&sBarInit));
	costBar->setBackgroundColour(WZCOL_BLACK);
	costBar->setTransparentToMouse(true);
}

void StatsFormButton::tintCostBar()
{
	const uint32_t cost = getCost();
	const bool affordable = selectedPlayer < MAX_PLAYERS && getPowerMinusQueued(selectedPlayer) >= static_cast<int32_t>(cost);
	costBar->majorCol = currentHudSkin().statsCostBarColor(WZCOL_ACTION_PROGRESS_BAR_MAJOR, affordable);
}

void DynamicIntFancyButton::updateHighlight()
{
	if (isHighlighted()) {
		state |= WBUT_CLICKLOCK;
	} else {
		state &= ~WBUT_CLICKLOCK;
	}
}

void ObjectsForm::display(int xOffset, int yOffset)
{
	if (!isClosing())
	{
		updateButtons();
		getController().updateHighlighted();
		if (previousHighlighted != getController().getHighlightedObject())
		{
			goToHighlightedTab();
		}
		previousHighlighted = getController().getHighlightedObject();
	}
	BaseWidget::display(xOffset, yOffset);
}

void ObjectsForm::goToHighlightedTab()
{
	auto highlighted = getController().getHighlightedObject();
	for (auto i = 0; i < getController().objectsSize(); i++)
	{
		if (highlighted == getController().getObjectAt(i))
		{
			if (auto position = listPositionOf(i))
			{
				objectsList->goToChildPage(position.value());
			}
			return;
		}
	}
}

optional<size_t> ObjectsForm::listPositionOf(size_t objectIndex) const
{
	if (!filteredLayout)
	{
		return objectIndex;
	}
	auto it = std::find(shownIndices.begin(), shownIndices.end(), objectIndex);
	return it != shownIndices.end() ? optional<size_t>(static_cast<size_t>(it - shownIndices.begin())) : nullopt;
}

BASE_OBJECT *ObjectsForm::objectUnderPointer() const
{
	ListWidget *listWidget = objectsList != nullptr ? objectsList->listWidget() : nullptr;
	if (listWidget == nullptr)
	{
		return nullptr;
	}
	for (auto widget = getMouseOverWidget().lock(); widget != nullptr; widget = widget->parent())
	{
		if (widget->parent().get() != listWidget)
		{
			continue;
		}
		const auto &children = listWidget->children();
		const size_t position = static_cast<size_t>(std::find(children.begin(), children.end(), widget) - children.begin());
		const size_t index = filteredLayout ? (position < shownIndices.size() ? shownIndices[position] : SIZE_MAX) : position;
		auto &controller = getController();
		return index < controller.objectsSize() ? controller.getObjectAt(index) : nullptr;
	}
	return nullptr;
}

void ObjectsForm::setObjectFilter(const std::function<bool (BASE_OBJECT *)> &filter)
{
	objectFilter = filter;
	updateButtons();
	objectsList->goToChildPage(0);
}

void ObjectsForm::removeAllButtons()
{
	while (buttonsCount > 0)
	{
		removeLastButton();
	}
	shownIndices.clear();
}

void ObjectsForm::initialize()
{
	// creating an obj stat form
	id = IDOBJ_FORM;
	setHudSkinned(true);
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(hudObjectsPanelRect());
	}));

	addCloseButton();
	addTabList();
}

void ObjectsForm::addCloseButton()
{
	W_BUTINIT init;
	init.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(OBJ_BACKWIDTH - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
	});
	init.pTip = _("Close");
	init.pDisplay = intDisplayHudCloseButton;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto button = std::make_shared<W_BUTTON>(&init);
	attach(button);
	closeButton = button;
	button->addOnClickHandler([](W_BUTTON&) {
		widgScheduleTask([]() {
			intResetScreen(false);
			intMode = INT_NORMAL;
		});
	});
}

void ObjectsForm::addTabList()
{
	attach(objectsList = IntListTabWidget::make());
	objectsList->setHudSkinned();
	objectsList->id = IDOBJ_TABFORM;
	objectsList->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		IntListTabWidget *pObjectsList = static_cast<IntListTabWidget *>(psWidget);
		assert(pObjectsList != nullptr);
		pObjectsList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
		pObjectsList->setChildSpacing(OBJ_GAP, OBJ_GAP);
		int objListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
		pObjectsList->setGeometry((OBJ_BACKWIDTH - objListWidth) / 2, OBJ_TABY, objListWidth, OBJ_BACKHEIGHT - OBJ_TABY);
	}));
}

void ObjectsForm::updateButtons()
{
	if (objectFilter)
	{
		updateFilteredButtons();
		return;
	}
	if (filteredLayout)
	{
		removeAllButtons();
		filteredLayout = false;
	}
	auto objectsCount = getController().objectsSize();
	while (buttonsCount < objectsCount)
	{
		addNewButton();
	}

	while (buttonsCount > objectsCount)
	{
		removeLastButton();
	}
}

void ObjectsForm::updateFilteredButtons()
{
	auto &controller = getController();
	std::vector<size_t> wanted;
	for (size_t i = 0; i < controller.objectsSize(); ++i)
	{
		if (objectFilter(controller.getObjectAt(i)))
		{
			wanted.push_back(i);
		}
	}
	if (filteredLayout && wanted == shownIndices)
	{
		return;
	}
	removeAllButtons();
	filteredLayout = true;
	shownIndices = wanted;
	for (size_t index : shownIndices)
	{
		addNewButton(index);
	}

	std::weak_ptr<ObjectsForm> weakForm = std::static_pointer_cast<ObjectsForm>(shared_from_this());
	widgScheduleTask([weakForm]() {
		if (auto form = weakForm.lock())
		{
			form->highlightFirstShown();
		}
	});
}

void ObjectsForm::highlightFirstShown()
{
	auto &controller = getController();
	auto highlighted = controller.getHighlightedObject();
	std::vector<size_t> valid;
	std::copy_if(shownIndices.begin(), shownIndices.end(), std::back_inserter(valid), [&](size_t i) { return i < controller.objectsSize(); });
	const bool highlightShown = std::any_of(valid.begin(), valid.end(), [&](size_t i) { return controller.getObjectAt(i) == highlighted; });
	if (highlightShown || valid.empty())
	{
		return;
	}
	BASE_OBJECT *first = controller.getObjectAt(valid.front());
	if (first == nullptr || first->died)
	{
		return;
	}
	if (first->type == OBJ_STRUCTURE)
	{
		controller.clearStructureSelection();
	}
	else
	{
		controller.clearSelection();
	}
	controller.selectObject(first, false);
}

void ObjectsForm::addNewButton()
{
	addNewButton(buttonsCount);
}

void ObjectsForm::addNewButton(size_t buttonIndex)
{
	buttonsCount++;
	auto buttonHolder = std::make_shared<WIDGET>();
	objectsList->addWidgetToLayout(buttonHolder);

	auto statButton = makeStatsButton(buttonIndex);
	buttonHolder->attach(statButton);
	std::weak_ptr<ObjectsForm> weakForm = std::static_pointer_cast<ObjectsForm>(shared_from_this());
	std::weak_ptr<WIDGET> weakHolder = buttonHolder;
	hudDetailRegister(statButton, HudDetailProvider{[weakForm, buttonIndex]() -> std::shared_ptr<WIDGET> {
		auto form = weakForm.lock();
		if (form == nullptr || buttonIndex >= form->objectsController().objectsSize())
		{
			return nullptr;
		}
		return hudObjectDetail(form->objectsController().getObjectAt(buttonIndex));
	}, HudDetailPlacement::Above, IDOBJ_FORM, [weakHolder]() {
		return hudObjectColumnActions(weakHolder.lock());
	}});
	statButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
	statButton->style |= WFORM_SECONDARY;

	auto objectButton = makeObjectButton(buttonIndex);
	buttonHolder->attach(objectButton);
	objectButton->setGeometry(0, OBJ_STARTY, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
}

void ObjectsForm::removeLastButton()
{
	buttonsCount--;
	if (auto listWidget = objectsList->listWidget())
	{
		auto lastButton = listWidget->children().back();
		listWidget->detach(lastButton);
	}
}

void StatsForm::initialize()
{
	id = IDSTAT_FORM;
	setHudSkinned(true);
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(hudStatsPanelRect());
	}));

	addCloseButton();
	addTabList();
}

void StatsForm::addCloseButton()
{
	W_BUTINIT init;
	init.x = STAT_WIDTH - CLOSE_WIDTH;
	init.y = 0;
	init.width = CLOSE_WIDTH;
	init.height = CLOSE_HEIGHT;
	init.pTip = _("Close");
	init.pDisplay = intDisplayHudCloseButton;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto button = std::make_shared<W_BUTTON>(&init);
	attach(button);
	closeButton = button;
	button->addOnClickHandler([](W_BUTTON&) {
		widgScheduleTask([]() {
			intRemoveStats();
			intMode = INT_OBJECT;
		});
	});
}

void StatsForm::addTabList()
{
	attach(optionList = IntListTabWidget::make());
	optionList->setHudSkinned();
	optionList->id = IDSTAT_TABFORM;
	optionList->setChildSize(STAT_BUTWIDTH, STAT_BUTHEIGHT);
	optionList->setChildSpacing(STAT_GAP, STAT_GAP);
	int statListWidth = STAT_BUTWIDTH * 2 + STAT_GAP;
	if (hudCompactLayout())
	{
		// The page control goes in the header's row (addHudExtension): the rows start under the header
		optionList->setTabRowHidden();
		optionList->setGeometry((STAT_WIDTH - statListWidth) / 2, hudCompactStatsHeaderHeight(), statListWidth, hudCompactStatsRows() * (STAT_BUTHEIGHT + STAT_GAP) - STAT_GAP);
		return;
	}
	optionList->setGeometry((STAT_WIDTH - statListWidth) / 2, STAT_TABFORMY, statListWidth, STAT_HEIGHT - STAT_TABFORMY);
}

void StatsForm::display(int xOffset, int yOffset)
{
	updateLayout();
	BaseWidget::display(xOffset, yOffset);
}

void StatsForm::updateLayout()
{
	updateButtons();
}

void BaseObjectsStatsController::updateHighlightedObjectStats()
{
	auto highlightedObject = getHighlightedObject();
	auto size = objectsSize();

	for (auto i = 0; i < size; i++)
	{
		if (getObjectAt(i) == highlightedObject)
		{
			highlightedObjectStats = getObjectStatsAt(i);
			return;
		}
	}

	highlightedObjectStats = nullptr;
}

void ObjectStatsForm::updateLayout()
{
	BaseWidget::updateLayout();
	auto& controller = getController();
	controller.updateHighlightedObjectStats();
	auto highlighted = getController().getHighlightedObjectStats();
	if (highlighted != nullptr && controller.getQueuedJumpToHighlightedStatsObject())
	{
		goToHighlightedTab();
		controller.clearQueuedJumpToHighlightedStatsObject();
	}
	previousHighlighted = highlighted;
}

void ObjectStatsForm::goToHighlightedTab()
{
	auto highlighted = getController().getHighlightedObjectStats();
	for (auto i = 0; i < getController().statsSize(); i++)
	{
		if (highlighted == getController().getStatsAt(i))
		{
			if (auto position = listPositionOf(i))
			{
				optionList->goToChildPage(position.value());
			}
			return;
		}
	}
}

optional<size_t> StatsForm::listPositionOf(size_t statsIndex) const
{
	if (!filteredLayout)
	{
		return statsIndex;
	}
	auto it = std::find(shownIndices.begin(), shownIndices.end(), statsIndex);
	return it != shownIndices.end() ? optional<size_t>(static_cast<size_t>(it - shownIndices.begin())) : nullopt;
}

void StatsForm::setOptionFilter(const std::function<bool (BASE_STATS *)> &filter)
{
	optionFilter = filter;
	updateButtons();
	optionList->goToChildPage(0);
}

void StatsForm::removeAllButtons()
{
	while (buttonsCount > 0)
	{
		removeLastButton();
	}
	shownIndices.clear();
}

void StatsForm::updateFilteredButtons()
{
	auto &controller = getController();
	std::vector<size_t> wanted;
	for (size_t i = 0; i < controller.statsSize(); ++i)
	{
		if (optionFilter(controller.getStatsAt(i)))
		{
			wanted.push_back(i);
		}
	}
	if (filteredLayout && wanted == shownIndices)
	{
		return;
	}
	removeAllButtons();
	filteredLayout = true;
	shownIndices = wanted;
	for (size_t index : shownIndices)
	{
		addNewButton(index);
	}
}

void StatsForm::updateButtons()
{
	if (optionFilter)
	{
		updateFilteredButtons();
		return;
	}
	if (filteredLayout)
	{
		removeAllButtons();
		filteredLayout = false;
	}
	auto statsCount = getController().statsSize();
	while (buttonsCount < statsCount)
	{
		addNewButton();
	}

	while (buttonsCount > statsCount)
	{
		removeLastButton();
	}
}

void StatsForm::addNewButton()
{
	addNewButton(buttonsCount);
}

void StatsForm::addNewButton(size_t buttonIndex)
{
	buttonsCount++;
	auto button = makeOptionButton(buttonIndex);
	optionList->addWidgetToLayout(button);
	button->style |= WFORM_SECONDARY;
}

void StatsForm::removeLastButton()
{
	buttonsCount--;
	if (auto listWidget = optionList->listWidget())
	{
		auto lastButton = listWidget->children().back();
		listWidget->detach(lastButton);
	}
}


std::shared_ptr<WIDGET> ObjectButton::makeHudDetail()
{
	auto &controller = getController();
	return objectIndex < controller.objectsSize() ? hudObjectDetail(controller.getObjectAt(objectIndex)) : nullptr;
}

void StatsForm::addHudExtension()
{
	auto self = std::static_pointer_cast<StatsForm>(shared_from_this());
	std::weak_ptr<StatsForm> weakSelf = self;
	// (in the Compact layout there's no room above the panel for the filters: they fold into a tab beside it)
	const bool compact = hudCompactLayout();
	const bool touchHeader = !compact && hudLargerTouchTargets();
	auto content = hudModuleActive(HudModuleId::PanelFilters) ? hudMakePanelFilters(self) : nullptr;
	if (compact)
	{
		addPageControl();
		if (hudCompactHeaderMenu() && !headerMenuEntries().empty())
		{
			headerMenu = HudHeaderMenuButton::make([weakSelf]() {
				auto strongSelf = weakSelf.lock();
				return strongSelf != nullptr ? strongSelf->headerMenuEntries() : std::vector<HudHeaderMenuEntry>();
			});
			headerMenu->id = IDHUD_STATS_MENU;
			attach(headerMenu);
			pageControl->setSingleSegment(true);
		}
	}
	if (content == nullptr && currentHudSkin().panelTopPadding() <= 0 && !touchHeader)
	{
		if (compact)
		{
			setCalcLayout([weakSelf](WIDGET *psWidget) {
				psWidget->setGeometry(hudStatsPanelRect());
				if (auto strongSelf = weakSelf.lock())
				{
					strongSelf->layoutHudHeader();
				}
			});
			callCalcLayout();
		}
		return;
	}
	HudPanelExtensionSpec spec;
	spec.contentId = IDHUD_STATS_CONTENT;
	spec.classicRect = []() { return hudStatsPanelRect(); };
	spec.availableHeightAbove = [](const WzRect &classic) { return classic.y() - hudTopBand(); };
	// (Compact: the tab is in the top row, clear of the object row)
	spec.availableWidthRight = [](const WzRect &classic) {
		return hudCompactLayout() ? static_cast<int>(pie_GetVideoBufferWidth()) - classic.right() : OBJ_BACKX - classic.right();
	};
	spec.closeButton = content != nullptr && !compact ? closeButton : nullptr;
	if (content != nullptr || compact || touchHeader)
	{
		spec.afterLayout = [weakSelf]() {
			if (auto strongSelf = weakSelf.lock())
			{
				strongSelf->layoutHudHeader();
			}
		};
	}
	if (content != nullptr && !compact)
	{
		addPageControl();
	}
	if ((content != nullptr && !compact) || touchHeader)
	{
		// (raised by the taller header's growth)
		spec.topInset = [weakSelf]() {
			auto strongSelf = weakSelf.lock();
			if (strongSelf == nullptr)
			{
				return 0;
			}
			const int drop = strongSelf->pageControl != nullptr && strongSelf->compactHeaderInRow() ? COMPACT_HEADER_DROP : 0;
			return drop - strongSelf->headerGrowth();
		};
	}
	hudExtendPanel(self, content, spec);
}

// The page control in place of the page tabs: in the header's row, which moves down onto theirs, so the panel is
// shorter and the items keep their places (or, with many pages, in the page tabs' own row)
void StatsForm::addPageControl()
{
	pageControl = HudPageControl::make(optionList);
	pageControl->id = IDHUD_STATS_PAGES;
	attach(pageControl);
	auto tabs = std::static_pointer_cast<WIDGET>(optionList->tabWidget()->shared_from_this());
	tabs->hide();
	hudSetReplacedPageTabs(tabs);
	std::weak_ptr<StatsForm> weakSelf = std::static_pointer_cast<StatsForm>(shared_from_this());
	pageControl->onPagesChanged = [weakSelf](HudPageControl &) {
		if (auto strongSelf = weakSelf.lock())
		{
			strongSelf->callCalcLayout();
		}
	};
}

bool StatsForm::compactHeaderInRow() const
{
	return optionList->pages() <= compactHeaderMaxPages();
}

int StatsForm::headerGrowth() const
{
	// Only up into free room, clear of the band under the screen's top edge (ex. the campaign's timers)
	const int growth = hudTouchHeaderGrowth();
	return hudStatsPanelRect().y() - hudTopBand() >= growth ? growth : 0;
}

void StatsForm::sizeHeaderButton(W_BUTTON &button, int grow, AtlasImage fallback)
{
	const AtlasImage image = !button.images.normal.isNull() ? button.images.normal : fallback;
	if (grow > 0 && !image.isNull())
	{
		button.setImageButtonSize(image.width() + grow, image.height() + grow);
	}
	else
	{
		button.setImageButtonSize(0, 0);
	}
}

void StatsForm::layoutHudHeader()
{
	const int grow = headerGrowth();
	if (grow > 0 && closeButton != nullptr && !hudIsMovedCloseButton(*closeButton))
	{
		closeButton->setGeometry(STAT_WIDTH - CLOSE_WIDTH - grow, -grow, CLOSE_WIDTH + grow, CLOSE_HEIGHT + grow);
	}
	if (pageControl == nullptr)
	{
		if (!hudCompactLayout() && hudLargerTouchTargets())
		{
			layoutCompactHeader(0, grow, false);
		}
		return;
	}
	const int listLeft = optionList->x();
	const int listRight = optionList->x() + optionList->width();
	if (hudCompactLayout())
	{
		// Always in the header's row (with arrows when its pages don't all fit), up to the close button
		WzRect header = layoutCompactHeader(0, 0, true);
		if (headerMenu != nullptr)
		{
			// The menu button in place of the header's buttons, all of them as tall
			for (const auto &entry : headerMenuEntries())
			{
				if (entry.button != nullptr)
				{
					entry.button->hide();
				}
			}
			header = WzRect(HEADER_MENU_X, HEADER_CONTROLS_TOP, HUD_HEADER_MENU_BUTTON_WIDTH, HUD_TOUCH_HEADER_CONTROL_HEIGHT);
			headerMenu->setGeometry(header);
			if (closeButton != nullptr)
			{
				closeButton->setGeometry(STAT_WIDTH - TOUCH_CLOSE_WIDTH, HEADER_CONTROLS_TOP, TOUCH_CLOSE_WIDTH, HUD_TOUCH_HEADER_CONTROL_HEIGHT);
			}
		}
		pageControl->setVisualHeight(header.height());
		const int left = header.width() > 0 ? header.x() + header.width() + HEADER_GAP : listLeft;
		const int right = closeButton != nullptr ? closeButton->x() - HEADER_GAP : listRight;
		pageControl->setGeometry(left, header.y() - PAGE_ROW_PAD, std::max(0, right - left), header.height() + PAGE_ROW_PAD * 2);
		return;
	}
	if (!compactHeaderInRow())
	{
		layoutCompactHeader(0, grow, false);
		pageControl->setVisualHeight(grow > 0 ? PAGE_ROW_HEIGHT_OWN : PAGE_ROW_VISUAL_HEIGHT);
		pageControl->setGeometry(listLeft, STAT_TABFORMY - 1, listRight - listLeft, PAGE_ROW_HEIGHT_OWN);
		return;
	}
	const WzRect header = layoutCompactHeader(COMPACT_HEADER_DROP, grow, true);
	// As tall as the header's controls, and level with them (its hit area a little taller)
	pageControl->setVisualHeight(header.height());
	const int top = header.y() - PAGE_ROW_PAD;
	const int rowHeight = header.height() + PAGE_ROW_PAD * 2;
	int right = listRight;
	// Short of the close button, when it's in this row (no band above the contents)
	auto anchor = pageControl->parent();
	if (anchor != nullptr && closeButton != nullptr && closeButton->parent() == anchor->parent())
	{
		const int closeTop = closeButton->y() - anchor->y();
		if (closeTop < top + rowHeight && closeTop + closeButton->height() > top)
		{
			right = std::min(right, closeButton->x() - HEADER_GAP);
		}
	}
	const int left = header.x() + header.width() + HEADER_GAP;
	pageControl->setGeometry(left, top, std::max(0, right - left), rowHeight);
}

void ObjectsForm::addHudExtension()
{
	auto self = std::static_pointer_cast<ObjectsForm>(shared_from_this());
	auto content = hudModuleActive(HudModuleId::RowSummary) ? hudMakeRowSummary(self) : nullptr;
	if (content == nullptr && currentHudSkin().panelTopPadding() <= 0)
	{
		return;
	}
	HudPanelExtensionSpec spec;
	spec.contentId = IDHUD_OBJECTS_CONTENT;
	spec.classicRect = []() { return hudObjectsPanelRect(); };
	spec.availableHeightAbove = [](const WzRect &classic) { return classic.y() - hudTopBand(); };
	spec.closeButton = content != nullptr ? closeButton : nullptr;
	// The strip shows the page tabs, so it may cover their row
	spec.bandOverlap = content != nullptr ? OBJ_TABY + objectsList->tabWidget()->height() : 0;
	hudExtendPanel(self, content, spec);
}
