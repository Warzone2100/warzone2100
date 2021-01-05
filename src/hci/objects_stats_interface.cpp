#include <memory>
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/bar.h"
#include "build_interface.h"
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

#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

static void statGetResearchImage(BASE_STATS *psStat, Image *image, iIMDShape **Shape, BASE_STATS **ppGraphicData, bool drawTechIcon);

void StatsButton::display(int xOffset, int yOffset)
{
	updateSelection();

	auto stat = getStats();
	initDisplay();

	ImdObject object;
	Image image;

	if (stat)
	{
		if (StatIsStructure(stat))
		{
			object = ImdObject::StructureStat(stat);
		}
		else if (StatIsTemplate(stat))
		{
			object = ImdObject::DroidTemplate(stat);
		}
		else if (StatIsFeature(stat))
		{
			object = ImdObject::Feature(stat);
		}
		else
		{
			auto compID = StatIsComponent(stat); // This fails for viper body.
			if (compID != COMP_NUMCOMPONENTS)
			{
				object = ImdObject::Component(stat);
			}
			else if (StatIsResearch(stat))
			{
				iIMDShape *shape;
				BASE_STATS *psResGraphic;
				statGetResearchImage(stat, &image, &shape, &psResGraphic, true);
				if (psResGraphic)
				{
					//we have a Stat associated with this research topic
					if (StatIsStructure(psResGraphic))
					{
						object = ImdObject::StructureStat(psResGraphic);
					}
					else
					{
						compID = StatIsComponent(psResGraphic);
						if (compID != COMP_NUMCOMPONENTS)
						{
							//overwrite the Object pointer
							object = ImdObject::Component(psResGraphic);
						}
						else
						{
							ASSERT(false, "Invalid Stat for research button");
							object = ImdObject::Research(nullptr);
						}
					}
				}
				else
				{
					//no Stat for this research topic so just use the graphic provided
					//if Object != NULL the there must be a IMD so set the object to
					//equal the Research stat
					if (shape != nullptr)
					{
						object = ImdObject::Research(stat);
					}
				}
			}
		}
	}
	else
	{
		//BLANK button for now - AB 9/1/98
		object = ImdObject::Component(nullptr);
	}

	displayIMD(image, object, xOffset, yOffset);
	displayIfHighlight(xOffset, yOffset);

	doneDisplay();
}

void DynamicIntFancyButton::updateSelection()
{
	if (isSelected()) {
		state |= WBUT_CLICKLOCK;
	} else {
		state &= ~WBUT_CLICKLOCK;
	}
}

void ObjectsForm::run(W_CONTEXT *)
{
	updateButtons();
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
	init.id = IDOBJ_CLOSE;
	init.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(OBJ_BACKWIDTH - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
	});
	init.pTip = _("Close");
	init.pDisplay = intDisplayImageHilight;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	auto button = std::static_pointer_cast<WIDGET>(std::make_shared<W_BUTTON>(&init));
	attach(button);
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
	auto objectsCount = controller->objectsSize();
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
	init.formID = IDSTAT_FORM;
	init.id = IDSTAT_CLOSE;
	init.x = STAT_WIDTH - CLOSE_WIDTH;
	init.y = 0;
	init.width = CLOSE_WIDTH;
	init.height = CLOSE_HEIGHT;
	init.pTip = _("Close");
	init.pDisplay = intDisplayImageHilight;
	init.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	attach(std::make_shared<W_BUTTON>(&init));
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
	updateSelectedObjectStats();
	updateButtons();
	BaseWidget::display(xOffset, yOffset);
}

void StatsForm::updateSelectedObjectStats()
{
	auto selectedObject = controller->getSelectedObject();
	auto objectsSize = controller->objectsSize();
	selectedObjectStats = nullptr;

	for (auto i = 0; i < objectsSize; i++)
	{
		if (controller->getObjectAt(i) == selectedObject)
		{
			selectedObjectStats = controller->getObjectStatsAt(i);
			break;
		}
	}
}

void StatsForm::updateButtons()
{
	auto statsCount = controller->statsSize();
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

static void statGetResearchImage(BASE_STATS *psStat, Image *image, iIMDShape **Shape, BASE_STATS **ppGraphicData, bool drawTechIcon)
{
	if (drawTechIcon && ((RESEARCH *)psStat)->iconID != NO_RESEARCH_ICON)
	{
		*image = Image(IntImages, ((RESEARCH *)psStat)->iconID);
	}
	//if the research has a Stat associated with it - use this as display in the button
	if (((RESEARCH *)psStat)->psStat)
	{
		*ppGraphicData = ((RESEARCH *)psStat)->psStat;
		//make sure the IMDShape is initialised
		*Shape = nullptr;
	}
	else
	{
		//no stat so just just the IMD associated with the research
		*Shape = ((RESEARCH *)psStat)->pIMD;
		//make sure the stat is initialised
		*ppGraphicData = nullptr;
	}
}

