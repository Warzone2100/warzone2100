#include <memory>
#include "lib/ivis_opengl/bitimage.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/bar.h"
#include "lib/widget/label.h"
#include "research.h"
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
#include "../mission.h"

STRUCTURE *ResearchController::highlightedFacility = nullptr;
static ImdObject getResearchObjectImage(RESEARCH *research);

void ResearchController::updateData()
{
	updateFacilitiesList();
	updateHighlighted();
	updateResearchOptionsList();
}

void ResearchController::updateFacilitiesList()
{
	facilities.clear();

	for (auto psStruct = interfaceStructList(); psStruct != nullptr; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH && psStruct->status == SS_BUILT && psStruct->died == 0)
		{
			facilities.push_back(psStruct);
		}
	}

	std::reverse(facilities.begin(), facilities.end());
}

nonstd::optional<size_t> ResearchController::getSelectedFacilityIndex()
{
	if (!getHighlightedObject())
	{
		return nonstd::nullopt;
	}

	auto found = std::find(facilities.begin(), facilities.end(), getHighlightedObject());

	return found == facilities.end() ? nonstd::nullopt : nonstd::optional<size_t>(found - facilities.begin());
}

void ResearchController::updateResearchOptionsList()
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

RESEARCH *ResearchController::getObjectStatsAt(size_t objectIndex) const
{
	auto facility = getObjectAt(objectIndex);
	ASSERT_NOT_NULLPTR_OR_RETURN(nullptr, facility);

	RESEARCH_FACILITY *psResearchFacility = &facility->pFunctionality->researchFacility;

	if (psResearchFacility->psSubjectPending != nullptr && !IsResearchCompleted(&asPlayerResList[facility->player][psResearchFacility->psSubjectPending->index]))
	{
		return psResearchFacility->psSubjectPending;
	}

	return psResearchFacility->psSubject;
}

void ResearchController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
}

void ResearchController::startResearch(RESEARCH *research)
{
	triggerEvent(TRIGGER_MENU_RESEARCH_SELECTED);

	auto facility = getHighlightedObject();

	ASSERT_NOT_NULLPTR_OR_RETURN(, facility);
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

void ResearchController::setHighlightedObject(BASE_OBJECT *object)
{
	if (object == nullptr)
	{
		highlightedFacility = nullptr;
		return;
	}

	auto facility = castStructure(object);
	ASSERT_NOT_NULLPTR_OR_RETURN(, facility);
	ASSERT_OR_RETURN(, facility->pStructureType->type == REF_RESEARCH, "Invalid facility pointer");
	highlightedFacility = facility;
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

class ResearchObjectButton : public ObjectButton
{
private:
	typedef ObjectButton BaseWidget;

protected:
	ResearchObjectButton (): BaseWidget() {}

public:
	static std::shared_ptr<ResearchObjectButton> make(const std::shared_ptr<ResearchController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ResearchObjectButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize();
		widget->controller = controller;
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

		controller->clearStructureSelection();
		controller->selectObject(controller->getObjectAt(objectIndex));
		if (!offWorldKeepLists)
		{
			jump();
		}

		controller->displayStatsForm();
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto facility = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, facility);
		ASSERT_OR_RETURN(, !isDead(facility), "Facility is dead");
		displayIMD(Image(), ImdObject::Structure(facility), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();

		if (bMultiPlayer)
		{
			updateAllyStatus();
		}
	}

	void updateAllyStatus()
	{
		allyResearchIcons.hide();

		auto research = controller->getObjectStatsAt(objectIndex);
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
		auto facility = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN("", facility);
		return getStatsName(facility->pStructureType);
	}

	std::shared_ptr<BaseObjectsController> getController() const override
	{
		return controller;
	}

private:
	std::shared_ptr<ResearchController> controller;
	AllyResearchsIcons allyResearchIcons;
};

class ResearchStatsButton: public StatsButton
{
private:
	typedef	StatsButton BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ResearchStatsButton> make(const std::shared_ptr<ResearchController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public ResearchStatsButton {};
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
		auto facility = controller->getObjectAt(objectIndex);
		auto research = controller->getObjectStatsAt(objectIndex);

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

private:
	RESEARCH *getStats() override
	{
		return controller->getObjectStatsAt(objectIndex);
	}

	void initialize()
	{
		addProgressBar();
	}

	void updateProgressBar()
	{
		auto facility = controller->getObjectAt(objectIndex);
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
		return facility && (facility->selected || facility == controller->getHighlightedObject());
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto facility = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, facility);

		if (mouseButton == WKEY_PRIMARY)
		{
			//might need to cancel the hold on research facility
			releaseResearch(facility, ModeQueue);
			controller->clearStructureSelection();
			controller->selectObject(facility);
			controller->displayStatsForm();
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			controller->requestResearchCancellation(facility);
		}
	}

	std::shared_ptr<ResearchController> controller;
	size_t objectIndex;
};

class ResearchOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	using BaseWidget::BaseWidget;
	static const size_t MAX_ALLY_ICONS = 4;

public:
	static std::shared_ptr<ResearchOptionButton> make(const std::shared_ptr<ResearchController> &controller, size_t researchOptionIndex)
	{
		class make_shared_enabler: public ResearchOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->researchOptionIndex = researchOptionIndex;
		widget->initialize();
		return widget;
	}

private:
	RESEARCH *getStats() override
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

		auto research = getStats();
		ASSERT_NOT_NULLPTR_OR_RETURN(, research);

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
		auto research = getStats();

		updateCostBar(research);

		if (bMultiPlayer)
		{
			updateAllyStatus(research);
		}
	}

	void updateCostBar(RESEARCH *stat)
	{
		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
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

	uint32_t getCost() override
	{
		return getStats()->researchPower;
	}

	void run(W_CONTEXT *context) override
	{
		BaseWidget::run(context);

		if (BaseWidget::isMouseOverWidget())
		{
			intSetShadowPower(getCost());
		}
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto clickedStats = controller->getStatsAt(researchOptionIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		if (mouseButton == WKEY_PRIMARY)
		{
			controller->startResearch(clickedStats);
			intRemoveStats();
		}
	}

	std::shared_ptr<ResearchController> controller;
	size_t researchOptionIndex;
	AllyResearchsIcons allyResearchIcons;
};

class ResearchObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ResearchObjectsForm> make(const std::shared_ptr<ResearchController> &controller)
	{
		class make_shared_enabler: public ResearchObjectsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return ResearchStatsButton::make(controller, buttonIndex);
	}

	std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const override
	{
		return ResearchObjectButton::make(controller, buttonIndex);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		controller->updateHighlighted();
		BaseWidget::display(xOffset, yOffset);
	}

	std::shared_ptr<BaseObjectsController> getController() const override
	{
		return controller;
	}

private:
	std::shared_ptr<ResearchController> controller;
};

class ResearchStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ResearchStatsForm> make(const std::shared_ptr<ResearchController> &controller)
	{
		class make_shared_enabler: public ResearchStatsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return ResearchOptionButton::make(controller, buttonIndex);
	}

protected:
	std::shared_ptr<BaseStatsController> getController() const override
	{
		return controller;
	}

private:
	std::shared_ptr<ResearchController> controller;
};

bool ResearchController::showInterface()
{
	updateData();
	if (facilities.empty())
	{
		return false;
	}

	auto objectsForm = ResearchObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);
	displayStatsForm();
	triggerEvent(TRIGGER_MENU_RESEARCH_UP);
	return true;
}

std::shared_ptr<StatsForm> ResearchController::makeStatsForm()
{
	return ResearchStatsForm::make(shared_from_this());
}

void ResearchController::requestResearchCancellation(STRUCTURE *facility)
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
