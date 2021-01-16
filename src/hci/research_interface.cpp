#include <memory>
#include "lib/ivis_opengl/bitimage.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/bar.h"
#include "lib/widget/label.h"
#include "research_interface.h"
#include "../display.h"
#include "../geometry.h"
#include "../warcam.h"
#include "../display3d.h"
#include "../intorder.h"
#include "../qtscript.h"
#include "../research.h"
#include "../intdisplay.h"
#include "../power.h"
#include "../component.h"

#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

static ImdObject getResearchObjectImage(RESEARCH *research);

void ResearchInterfaceController::updateFacilitiesList()
{
	objects.clear();

	for (auto psStruct = interfaceStructList(); psStruct != nullptr; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH && psStruct->status == SS_BUILT && psStruct->died == 0)
		{
			objects.push_back(psStruct);
		}
	}
}

nonstd::optional<size_t> ResearchInterfaceController::getSelectedFacilityIndex()
{
	if (!getSelectedObject())
	{
		return nonstd::nullopt;
	}

	auto found = std::find(objects.begin(), objects.end(), getSelectedObject());

	return found == objects.end() ? nonstd::nullopt : nonstd::optional<size_t>(found - objects.begin());
}

void ResearchInterfaceController::updateResearchOptionsList()
{
	stats.clear();

	auto selectedIndex = getSelectedFacilityIndex();
	auto research = selectedIndex.has_value() ? (RESEARCH *)getObjectStatsAt(selectedIndex.value()) : nullptr;
	auto researchIndex = research != nullptr ? nonstd::optional<UWORD>(research->index) : nonstd::nullopt;

	for (auto researchOption: fillResearchList(selectedPlayer, researchIndex, MAXRESEARCH))
	{
		stats.push_back(&asResearch[researchOption]);
	}

	std::sort(stats.begin(), stats.end(), [](const RESEARCH *a, const RESEARCH *b) {
		return mapIconToRID(a->iconID) < mapIconToRID(b->iconID);
	});
}

RESEARCH *ResearchInterfaceController::getObjectStatsAt(size_t objectIndex) const
{
	auto facility = castStructure(getObjectAt(objectIndex));
	ASSERT_OR_RETURN(nullptr, facility != nullptr, "Invalid Structure pointer");

	RESEARCH_FACILITY *psResearchFacility = &facility->pFunctionality->researchFacility;

	if (psResearchFacility->psSubjectPending != nullptr && !IsResearchCompleted(&asPlayerResList[facility->player][psResearchFacility->psSubjectPending->index]))
	{
		return psResearchFacility->psSubjectPending;
	}

	return psResearchFacility->psSubject;
}

void ResearchInterfaceController::selectFacility(STRUCTURE *facility)
{
	facility->selected = true;
	triggerEventSelected();
	jsDebugSelected(facility);
	setSelectedObject(facility);
}

void ResearchInterfaceController::jumpToSelectedFacility()
{
	auto facility = getSelectedObject();
	setPlayerPos(facility->pos.x, facility->pos.y);
	setWarCamActive(false);
}

void ResearchInterfaceController::refresh()
{
	updateFacilitiesList();
	updateResearchOptionsList();

	if (objectsSize() == 0)
	{
		closeInterface();
		return;
	}
}

void ResearchInterfaceController::closeInterface()
{
	intRemoveStats();
	intRemoveObject();
}

void ResearchInterfaceController::updateSelected()
{
	for (auto facility: objects)
	{
		if (facility->died == 0 && facility->selected)
		{
			setSelectedObject(facility);
			return;
		}
	}

	if (auto previouslySelected = getSelectedObject())
	{
		for (auto facility: objects)
		{
			if (facility->died == 0 && facility == previouslySelected)
			{
				setSelectedObject(facility);
				return;
			}
		}
	}

	setSelectedObject(objects.front());
}

void ResearchInterfaceController::startResearch(RESEARCH *research)
{
	triggerEvent(TRIGGER_MENU_RESEARCH_SELECTED);

	auto facility = castStructure(getSelectedObject());

	ASSERT_OR_RETURN(, facility != nullptr, "Invalid Structure pointer");
	auto psResFacilty = &facility->pFunctionality->researchFacility;

	if (bMultiMessages)
	{
		if (research != nullptr)
		{
			// Say that we want to do research [sic].
			sendResearchStatus(facility, research->ref - STAT_RESEARCH, selectedPlayer, true);
			setStatusPendingStart(*psResFacilty, research);  // Tell UI that we are going to research.
		}
		else
		{
			cancelResearch(facility, ModeQueue);
		}

		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
		return;
	}

	//initialise the subject
	psResFacilty->psSubject = nullptr;

	//set up the player_research
	if (research != nullptr)
	{
		auto count = research->ref - STAT_RESEARCH;
		//meant to still be in the list but greyed out
		auto pPlayerRes = &asPlayerResList[selectedPlayer][count];

		//set the subject up
		psResFacilty->psSubject = research;

		sendResearchStatus(facility, count, selectedPlayer, true);	// inform others, I'm researching this.

		MakeResearchStarted(pPlayerRes);
		psResFacilty->timeStartHold = 0;
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
}

class AllyResearchsIcons
{
private:
	static const size_t MAX_ALLY_ICONS = 4;
	std::shared_ptr<W_LABEL> allyIcons[MAX_ALLY_ICONS];

public:
	void initialize(WIDGET &parent)
	{
		for (auto i = 0; i < MAX_ALLY_ICONS; i++)
		{
			auto init = W_LABINIT();
			init.width = iV_GetImageWidth(IntImages, IMAGE_ALLY_RESEARCH);
			init.height = iV_GetImageHeight(IntImages, IMAGE_ALLY_RESEARCH);
			init.x = STAT_BUTWIDTH  - (init.width + 2) * i - init.width - 2;
			init.y = STAT_BUTHEIGHT - init.height - 3 - STAT_PROGBARHEIGHT;
			init.pDisplay = AllyResearchsIcons::displayAllyIcon;
			parent.attach(allyIcons[i] = std::make_shared<W_LABEL>(&init));
		}
	}

	static void displayAllyIcon(WIDGET *widget, UDWORD xOffset, UDWORD yOffset)
	{
		auto *label = (W_LABEL *)widget;
		auto x = label->x() + xOffset;
		auto y = label->y() + yOffset;

		iV_DrawImageTc(IntImages, IMAGE_ALLY_RESEARCH, IMAGE_ALLY_RESEARCH_TC, x, y, pal_GetTeamColour(label->UserData));
	}

	void hide()
	{
		for (const auto &icon: allyIcons)
		{
			icon->hide();
		}
	}

	void update(const std::vector<AllyResearch> &allyResearches)
	{
		for (auto i = 0; i < MAX_ALLY_ICONS && i < allyResearches.size(); i++)
		{
			allyIcons[i]->UserData = getPlayerColour(allyResearches[i].player);
			allyIcons[i]->setTip(getPlayerName(allyResearches[i].player));
			allyIcons[i]->show();
		}
	}

	void setTop(int y)
	{
		for (auto icon: allyIcons)
		{
			icon->move(icon->x(), y);
		}
	}
};

class ResearchObjectButton : public IntFancyButton
{
private:
	typedef IntFancyButton BaseWidget;

protected:
	ResearchObjectButton(): BaseWidget()
	{
		buttonType = BTMBUTTON;
	}

public:
	static std::shared_ptr<ResearchObjectButton> make(const std::shared_ptr<ResearchInterfaceController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ResearchObjectButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		widget->researchController = controller;
		widget->objectIndex = objectIndex;
		return widget;
	}

private:
	void initialize()
	{
		if (bMultiPlayer)
		{
			allyResearchIcons.initialize(*this);
			allyResearchIcons.setTop(10);
		}
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);

		auto facility = castStructure(researchController->getObjectAt(objectIndex));
		ASSERT_OR_RETURN(, facility != nullptr, "Invalid facility pointer");

		clearSelection();
		researchController->selectFacility(facility);

		if ((jumpPosition.x == 0 && jumpPosition.y == 0) || !objectOnScreen(facility, 0))
		{
			jumpPosition = getPlayerPos();
			researchController->jumpToSelectedFacility();
		}
		else
		{
			setPlayerPos(jumpPosition.x, jumpPosition.y);
			setWarCamActive(false);
			jumpPosition = {0, 0};
		}

		researchController->displayStatsForm();
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		if (bMultiPlayer)
		{
			updateAllyStatus();
		}

		auto facility = castStructure(researchController->getObjectAt(objectIndex));
		initDisplay();

		if (facility && !isDead(facility))
		{
			displayIMD(Image(), ImdObject::Structure(facility), xOffset, yOffset);
		}

		displayIfHighlight(xOffset, yOffset);
	}

	void updateAllyStatus()
	{
		allyResearchIcons.hide();

		auto research = researchController->getObjectStatsAt(objectIndex);
		if (research == nullptr)
		{
			return;
		}

		const std::vector<AllyResearch> &allyResearches = listAllyResearch(research->ref);
		if (allyResearches.empty())
		{
			return;
		}

		allyResearchIcons.update(allyResearches);
	}

	std::string getTip() override
	{
		auto facility = castStructure(researchController->getObjectAt(objectIndex));
		ASSERT_OR_RETURN("", facility != nullptr, "Invalid facility pointer");
		return getStatsName(facility->pStructureType);
	}

private:
	std::shared_ptr<ResearchInterfaceController> researchController;
	size_t objectIndex;
	Vector2i jumpPosition = {0, 0};
	AllyResearchsIcons allyResearchIcons;
};

class ResearchStatsButton: public ObjectStatsButton
{
private:
	typedef	ObjectStatsButton BaseWidget;

protected:
	ResearchStatsButton (const std::shared_ptr<ResearchInterfaceController> &controller, size_t objectIndex):
		BaseWidget(controller, objectIndex),
		researchController(controller)
	{}

public:
	static std::shared_ptr<ResearchStatsButton> make(const std::shared_ptr<ResearchInterfaceController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ResearchStatsButton
		{
		public:
			make_shared_enabler (const std::shared_ptr<ResearchInterfaceController> &controller, size_t objectIndex):
				ResearchStatsButton(controller, objectIndex)
			{}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller, objectIndex);
		widget->initialize();
		return widget;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto facility = castStructure(researchController->getObjectAt(objectIndex));
		auto research = researchController->getObjectStatsAt(objectIndex);

		ImdObject objectImage;

		auto researchPending = structureIsResearchingPending(facility);

		if (researchPending && research)
		{
			objectImage = getResearchObjectImage(research);
		}
		else
		{
			objectImage = ImdObject::Component(nullptr);
		}

		displayIMD(Image(), objectImage, xOffset, yOffset);

		if (researchPending && StructureIsOnHoldPending(facility))
		{
			iV_DrawImage(IntImages, ((realTime / 250) % 2) == 0 ? IMAGE_BUT0_DOWN : IMAGE_BUT_HILITE, xOffset + x(), yOffset + y());
		}
		else
		{
			displayIfHighlight(xOffset, yOffset);
		}
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		updateProgressBar();
	}

	std::string getTip() override
	{
		if (auto stats = getResearch())
		{
			return getStatsName(stats);
		}

		return "";
	}

private:
	RESEARCH *getResearch()
	{
		return researchController->getObjectStatsAt(objectIndex);
	}

	void initialize()
	{
		addProgressBar();
	}

	void updateProgressBar()
	{
		auto facility = castStructure(researchController->getObjectAt(objectIndex));
		progressBar->hide();

		auto research = StructureGetResearch(facility);
		if (research->psSubject == nullptr)
		{
			return;
		}

		auto currentPoints = asPlayerResList[selectedPlayer][research->psSubject->index].currentPoints;
		if (currentPoints != 0)
		{
			int researchRate = research->timeStartHold == 0 ? getBuildingResearchPoints(facility) : 0;
			formatTime(progressBar.get(), currentPoints, research->psSubject->researchPoints, researchRate, _("Research Progress"));
		}
		else
		{
			// Not yet started production.
			int neededPower = checkPowerRequest(facility);
			int powerToBuild = research->psSubject != nullptr ? research->psSubject->researchPower : 0;
			formatPower(progressBar.get(), neededPower, powerToBuild);
		}
	}

	bool isSelected() const override
	{
		auto facility = controller->getObjectAt(objectIndex);
		return facility && (facility->selected || facility == controller->getSelectedObject());
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto facility = castStructure(controller->getObjectAt(objectIndex));
		ASSERT_OR_RETURN(, facility != nullptr, "Invalid facility pointer");

		if (mouseButton == WKEY_PRIMARY)
		{
			//might need to cancel the hold on research facility
			releaseResearch(facility, ModeQueue);
			clearSelection();
			researchController->selectFacility(facility);
			researchController->displayStatsForm();
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			researchController->requestResearchCancellation(facility);
		}
	}

	std::shared_ptr<ResearchInterfaceController> researchController;
};

class ResearchOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	static const size_t MAX_ALLY_ICONS = 4;

	ResearchOptionButton(): BaseWidget() {}
public:
	static std::shared_ptr<ResearchOptionButton> make(const std::shared_ptr<ResearchInterfaceController> &controller, size_t researchOptionIndex)
	{
		class make_shared_enabler: public ResearchOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->researchOptionIndex = researchOptionIndex;
		widget->initialize();
		return widget;
	}

private:
	RESEARCH *getResearch()
	{
		return controller->getStatsAt(researchOptionIndex);
	}

	void initialize()
	{
		addCostBar();

		if (bMultiPlayer)
		{
			addProgressBar();
			allyResearchIcons.initialize(*this);
			progressBar->move(STAT_TIMEBARX, STAT_TIMEBARY);
			progressBar->setTip(_("Ally progress"));
		}
	}

	void display(int xOffset, int yOffset) override
	{
		updateLayout();

		auto research = getResearch();
		ASSERT_OR_RETURN(, research != nullptr, "Invalid research pointer");

		auto researchIcon = research->iconID != NO_RESEARCH_ICON ? Image(IntImages, research->iconID) : Image();
		displayIMD(researchIcon, getResearchObjectImage(research), xOffset, yOffset);

		if (research->subGroup != NO_RESEARCH_ICON)
		{
			iV_DrawImage(IntImages, research->subGroup, xOffset + x() + STAT_BUTWIDTH - 16, yOffset + y() + 3);
		}

		displayIfHighlight(xOffset, yOffset);
	}

	bool isSelected() const override
	{
		if (auto form = statsForm.lock())
		{
			return form->getSelectedStats() == controller->getStatsAt(researchOptionIndex);
		}

		return false;
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto research = getResearch();

		updateCostBar(research);

		if (bMultiPlayer)
		{
			updateAllyStatus(research);
		}
	}

	void updateCostBar(RESEARCH *stat)
	{
		costBar->majorSize = std::min(100, (int32_t)(stat->researchPower / POWERPOINTS_DROIDDIV));
	}

	void updateAllyStatus(RESEARCH *research)
	{
		costBar->move(STAT_TIMEBARX, STAT_TIMEBARY);
		progressBar->hide();
		allyResearchIcons.hide();

		if (research == nullptr)
		{
			return;
		}

		const std::vector<AllyResearch> &allyResearches = listAllyResearch(research->ref);
		if (allyResearches.empty())
		{
			return;
		}

		intDisplayUpdateAllyBar(progressBar.get(), *research, allyResearches);

		allyResearchIcons.update(allyResearches);

		costBar->move(STAT_TIMEBARX, STAT_TIMEBARY - STAT_PROGBARHEIGHT - 2);
		progressBar->show();
	}

	std::string getTip() override
	{
		auto research = getResearch();
		auto powerCost = research->researchPower;
		WzString costString = WzString::fromUtf8(_("\nCost: %1"));
		costString.replace("%1", WzString::number(powerCost));
		WzString tipString = getStatsName(research);
		tipString.append(costString);
		return tipString.toUtf8();
	}

	void run(W_CONTEXT *context) override
	{
		BaseWidget::run(context);

		if (BaseWidget::isMouseOverWidget())
		{
			intSetShadowPower(controller->getStatsAt(researchOptionIndex)->researchPower);
		}
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto clickedStats = controller->getStatsAt(researchOptionIndex);
		ASSERT_OR_RETURN(, clickedStats != nullptr, "Invalid template pointer");

		if (mouseButton == WKEY_PRIMARY)
		{
			controller->startResearch(clickedStats);
			intRemoveStats();
		}
	}

	std::shared_ptr<ResearchInterfaceController> controller;
	size_t researchOptionIndex;
	AllyResearchsIcons allyResearchIcons;
};

class ResearchObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;

protected:
	ResearchObjectsForm(const std::shared_ptr<ResearchInterfaceController> &controller): BaseWidget(controller), researchController(controller)
	{
	}

public:
	static std::shared_ptr<ResearchObjectsForm> make(const std::shared_ptr<ResearchInterfaceController> &controller)
	{
		class make_shared_enabler: public ResearchObjectsForm
		{
		public:
			make_shared_enabler(const std::shared_ptr<ResearchInterfaceController> &controller): ResearchObjectsForm(controller)
			{
			}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller);
		widget->initialize();
		return widget;
	}

	std::shared_ptr<ObjectStatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return ResearchStatsButton::make(researchController, buttonIndex);
	}

	std::shared_ptr<IntFancyButton> makeObjectButton(size_t buttonIndex) const override
	{
		return ResearchObjectButton::make(researchController, buttonIndex);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		researchController->updateSelected();
		BaseWidget::display(xOffset, yOffset);
	}

private:
	std::shared_ptr<ResearchInterfaceController> researchController;
};

class ResearchStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ResearchStatsForm> make(const std::shared_ptr<ResearchInterfaceController> &controller)
	{
		class make_shared_enabler: public ResearchStatsForm
		{
		public:
			make_shared_enabler(const std::shared_ptr<ResearchInterfaceController> &controller): ResearchStatsForm(controller)
			{
			}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller);
		widget->researchController = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return ResearchOptionButton::make(researchController, buttonIndex);
	}

private:
	std::shared_ptr<ResearchInterfaceController> researchController;
};

bool ResearchInterfaceController::showInterface()
{
	intRemoveStatsNoAnim();
	intRemoveOrderNoAnim();
	intRemoveObjectNoAnim();

	if (objects.empty())
	{
		return false;
	}

	auto sharedThis = shared_from_this();

	auto objectsForm = ResearchObjectsForm::make(sharedThis);
	psWScreen->psForm->attach(objectsForm);

	sharedThis->displayStatsForm();

	intMode = INT_STAT;
	triggerEvent(TRIGGER_MENU_RESEARCH_UP);

	return true;
}

void ResearchInterfaceController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto statForm = ResearchStatsForm::make(shared_from_this());
		psWScreen->psForm->attach(statForm);
		intMode = INT_STAT;
	}
}

void ResearchInterfaceController::requestResearchCancellation(STRUCTURE *facility)
{
	if (!structureIsResearchingPending(facility))
	{
		return;
	}

	if (!StructureIsOnHoldPending(facility))
	{
		holdResearch(facility, ModeQueue);
		return;
	}

	cancelResearch(facility, ModeQueue);
	audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
}

static ImdObject getResearchObjectImage(RESEARCH *research)
{
	BASE_STATS *psResGraphic = research->psStat;

	if (!psResGraphic)
	{
		// no Stat for this research topic so just use the graphic provided
		// if Object != NULL the there must be a IMD so set the object to
		// equal the Research stat
		if (research->pIMD != nullptr)
		{
			return ImdObject::Research(research);
		}

		return ImdObject();
	}

	// we have a Stat associated with this research topic
	if (StatIsStructure(psResGraphic))
	{
		// overwrite the Object pointer
		return ImdObject::StructureStat(psResGraphic);
	}

	auto compID = StatIsComponent(psResGraphic);
	if (compID != COMP_NUMCOMPONENTS)
	{
		return ImdObject::Component(psResGraphic);
	}

	ASSERT(false, "Invalid Stat for research button");
	return ImdObject::Research(nullptr);
}
