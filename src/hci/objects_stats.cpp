#include "lib/widget/button.h"
#include "lib/widget/bar.h"
#include "objects_stats.h"
#include "../display3d.h"
#include "../qtscript.h"
#include "../warcam.h"
#include "../geometry.h"
#include "groups.h"

void BaseObjectsController::clearSelection()
{
	::clearSelection();
}

void BaseObjectsController::clearStructureSelection()
{
	for (auto structure = interfaceStructList(); structure != nullptr; structure = structure->psNext)
	{
		structure->selected = false;
	}
}

void BaseObjectsController::selectObject(BASE_OBJECT *object)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);
	object->selected = true;
	triggerEventSelected();
	jsDebugSelected(object);
	setHighlightedObject(object);
	refresh();
}

void BaseObjectsController::prepareToClose()
{
	clearData();
}

void BaseObjectsController::jumpToObject(BASE_OBJECT *object)
{
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);
	setPlayerPos(object->pos.x, object->pos.y);
	setWarCamActive(false);
}

void BaseObjectsController::updateHighlighted()
{
	if (objectsSize() == 0)
	{
		setHighlightedObject(nullptr);
		return;
	}

	auto findAnySelectedObject = [&] (BASE_OBJECT *object) {
		if (object->died == 0 && object->selected)
		{
			setHighlightedObject(object);
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
				setHighlightedObject(object);
				return true;
			}

			return false;
		};

		if (findObject(findHighlightedObject))
		{
			return;
		}
	}

	setHighlightedObject(getObjectAt(0));
}

void BaseStatsController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto newStatsForm = makeStatsForm();
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

void DynamicIntFancyButton::updateLayout()
{
	updateHighlight();
	initDisplay();
}

void DynamicIntFancyButton::released(W_CONTEXT *context, WIDGET_KEY mouseButton)
{
	IntFancyButton::released(context, mouseButton);

	if (mouseButton == WKEY_PRIMARY)
	{
		clickPrimary();
	}
	else if (mouseButton == WKEY_SECONDARY)
	{
		clickSecondary();
	}
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
	attach(progressBar = std::make_shared<W_BARGRAPH>(&init));
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
		setPlayerPos(jumpPosition.x, jumpPosition.y);
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
	attach(costBar = std::make_shared<W_BARGRAPH>(&sBarInit));
	costBar->setBackgroundColour(WZCOL_BLACK);
	costBar->setTransparentToMouse(true);
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
			objectsList->goToChildPage(i);
			return;
		}
	}
}

void ObjectsForm::initialize()
{
	// creating an obj stat form
	id = IDOBJ_FORM;
	// TODO set if statement, subtract the 80 if enabled in options
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(OBJ_BACKX, OBJ_BACKY - 80, OBJ_BACKWIDTH, OBJ_BACKHEIGHT);
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
	init.pDisplay = intDisplayImageHilight;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto button = std::make_shared<W_BUTTON>(&init);
	attach(button);
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

void ObjectsForm::addNewButton()
{
	auto buttonIndex = buttonsCount++;
	auto buttonHolder = std::make_shared<WIDGET>();
	objectsList->addWidgetToLayout(buttonHolder);

	auto statButton = makeStatsButton(buttonIndex);
	buttonHolder->attach(statButton);
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
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(STAT_X, STAT_Y, STAT_WIDTH, STAT_HEIGHT);
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
	init.pDisplay = intDisplayImageHilight;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto button = std::make_shared<W_BUTTON>(&init);
	attach(button);
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
	optionList->id = IDSTAT_TABFORM;
	optionList->setChildSize(STAT_BUTWIDTH, STAT_BUTHEIGHT);
	optionList->setChildSpacing(STAT_GAP, STAT_GAP);
	int statListWidth = STAT_BUTWIDTH * 2 + STAT_GAP;
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
	getController().updateHighlightedObjectStats();
	auto highlighted = getController().getHighlightedObjectStats();
	if (highlighted != nullptr && previousHighlighted != highlighted)
	{
		goToHighlightedTab();
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
			optionList->goToChildPage(i);
			return;
		}
	}
}

void StatsForm::updateButtons()
{
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
	auto buttonIndex = buttonsCount++;
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

