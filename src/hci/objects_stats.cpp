#include <memory>
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/bar.h"
#include "build.h"
#include "../intdisplay.h"
#include "../objmem.h"
#include "../hci.h"
#include "../research.h"
#include "../statsdef.h"
#include "../order.h"
#include "../intorder.h"
#include "../mission.h"
#include "../qtscript.h"
#include "../display3d.h"
#include "../warcam.h"
#include "../geometry.h"

void BaseObjectsController::selectObject(BASE_OBJECT *object)
{
	object->selected = true;
	triggerEventSelected();
	jsDebugSelected(object);
	setHighlightedObject(object);
}

void BaseObjectsController::jumpToHighlighted()
{
	auto highlighted = getHighlightedObject();
	ASSERT_NOT_NULLPTR_OR_RETURN(, highlighted);
	setPlayerPos(highlighted->pos.x, highlighted->pos.y);
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

	if (auto previouslyHighlighted = getHighlightedObject())
	{
		auto findPreviouslyHighlightedObject = [&] (BASE_OBJECT *object) {
			if (object->died == 0 && object == previouslyHighlighted)
			{
				setHighlightedObject(object);
				return true;
			}

			return false;
		};

		if (findObject(findPreviouslyHighlightedObject))
		{
			return;
		}
	}

	setHighlightedObject(getObjectAt(0));
}

void DynamicIntFancyButton::updateLayout()
{
	updateSelection();
	initDisplay();
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
	progressBar->setBackgroundColour(WZCOL_BLACK);
}

void ObjectButton::selectAndJump()
{
	auto object = getController()->getObjectAt(objectIndex);
	ASSERT_NOT_NULLPTR_OR_RETURN(, object);

	clearSelection();
	getController()->selectObject(object);

	if ((jumpPosition.x == 0 && jumpPosition.y == 0) || !objectOnScreen(object, 0))
	{
		jumpPosition = getPlayerPos();
		getController()->jumpToHighlighted();
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
}

void DynamicIntFancyButton::updateSelection()
{
	if (isSelected()) {
		state |= WBUT_CLICKLOCK;
	} else {
		state &= ~WBUT_CLICKLOCK;
	}
}

void ObjectsForm::display(int xOffset, int yOffset)
{
	updateButtons();
	BaseWidget::display(xOffset, yOffset);
}

void ObjectsForm::initialize()
{
	id = IDOBJ_FORM;
	setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(OBJ_BACKX, OBJ_BACKY, OBJ_BACKWIDTH, OBJ_BACKHEIGHT);
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
		IntListTabWidget *objectsList = static_cast<IntListTabWidget *>(psWidget);
		assert(objectsList != nullptr);
		objectsList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
		objectsList->setChildSpacing(OBJ_GAP, OBJ_GAP);
		int objListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
		objectsList->setGeometry((OBJ_BACKWIDTH - objListWidth) / 2, OBJ_TABY, objListWidth, OBJ_BACKHEIGHT - OBJ_TABY);
	}));
}

void ObjectsForm::updateButtons()
{
	auto objectsCount = getController()->objectsSize();
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
	updateSelectedObjectStats();
	updateButtons();
}

void StatsForm::updateSelectedObjectStats()
{
	auto selectedObject = getController()->getHighlightedObject();
	auto objectsSize = getController()->objectsSize();
	selectedObjectStats = nullptr;

	for (auto i = 0; i < objectsSize; i++)
	{
		if (getController()->getObjectAt(i) == selectedObject)
		{
			selectedObjectStats = getController()->getObjectStatsAt(i);
			break;
		}
	}
}

void StatsForm::updateButtons()
{
	auto statsCount = getController()->statsSize();
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
	button->setStatsForm(std::static_pointer_cast<StatsForm>(shared_from_this()));
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

