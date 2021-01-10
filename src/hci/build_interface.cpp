#include <memory>
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "build_interface.h"
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
#include "../intdisplay.h"

#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

void BuildInterfaceController::updateBuildersList()
{
	std::vector<DROID *> newBuilders;
	for (DROID *droid = apsDroidLists[selectedPlayer]; droid; droid = droid->psNext)
	{
		if ((droid->droidType == DROID_CONSTRUCT || droid->droidType == DROID_CYBORG_CONSTRUCT) && droid->died == 0)
		{
			newBuilders.push_back(droid);
		}
	}

	objects = std::vector<BASE_OBJECT *>(newBuilders.rbegin(), newBuilders.rend());
}

void BuildInterfaceController::updateBuildOptionsList()
{
	std::vector<STRUCTURE_STATS *> newBuildOptions;

	bool researchModule;
	bool factoryModule;
	bool powerModule;

	//check to see if able to build research/factory modules
	researchModule = factoryModule = powerModule = false;

	//if currently on a mission can't build factory/research/power/derricks
	if (!missionIsOffworld())
	{
		for (auto psCurr = apsStructLists[selectedPlayer]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESEARCH && psCurr->status == SS_BUILT)
			{
				researchModule = true;
			}
			else if (psCurr->pStructureType->type == REF_FACTORY && psCurr->status == SS_BUILT)
			{
				factoryModule = true;
			}
			else if (psCurr->pStructureType->type == REF_POWER_GEN && psCurr->status == SS_BUILT)
			{
				powerModule = true;
			}
		}
	}

	//set the list of Structures to build
	for (auto inc = 0; inc < numStructureStats; inc++)
	{
		//if the structure is flagged as available, add it to the list
		if (apStructTypeLists[selectedPlayer][inc] != AVAILABLE && (!includeRedundantDesigns || apStructTypeLists[selectedPlayer][inc] != REDUNDANT))
		{
			continue;
		}

		auto &structureStat = asStructureStats[inc];

		//check not built the maximum allowed already
		if (structureStat.curCount[selectedPlayer] < structureStat.upgrade[selectedPlayer].limit)
		{
			//don't want corner wall to appear in list
			if (structureStat.type == REF_WALLCORNER)
			{
				continue;
			}

			// Remove the demolish stat from the list for tutorial
			// tjc 4-dec-98  ...
			if (bInTutorial && structureStat.type == REF_DEMOLISH)
			{
				continue;
			}

			//can't build list when offworld
			if (missionIsOffworld())
			{
				if (structureStat.type == REF_FACTORY ||
					structureStat.type == REF_POWER_GEN ||
					structureStat.type == REF_RESOURCE_EXTRACTOR ||
					structureStat.type == REF_RESEARCH ||
					structureStat.type == REF_CYBORG_FACTORY ||
					structureStat.type == REF_VTOL_FACTORY)
				{
					continue;
				}
			}

			if (structureStat.type == REF_RESEARCH_MODULE)
			{
				//don't add to list if Research Facility not presently built
				if (!researchModule)
				{
					continue;
				}
			}
			else if (structureStat.type == REF_FACTORY_MODULE)
			{
				//don't add to list if Factory not presently built
				if (!factoryModule)
				{
					continue;
				}
			}
			else if (structureStat.type == REF_POWER_MODULE)
			{
				//don't add to list if Power Gen not presently built
				if (!powerModule)
				{
					continue;
				}
			}
			// show only favorites?
			if (shouldShowFavorites() && !asStructureStats[inc].isFavorite)
			{
				continue;
			}

			debug(LOG_NEVER, "adding %s (%x)", getStatsName(&structureStat), apStructTypeLists[selectedPlayer][inc]);
			newBuildOptions.push_back(&structureStat);
		}
	}

	stats = std::vector<BASE_STATS *>(newBuildOptions.begin(), newBuildOptions.end());
}

BASE_STATS *BuildInterfaceController::getObjectStatsAt(size_t objectIndex) const
{
	auto builder = castDroid(getObjectAt(objectIndex));
	if (!builder)
	{
		return nullptr;
	}

	if (!(droidType(builder) == DROID_CONSTRUCT || droidType(builder) == DROID_CYBORG_CONSTRUCT))
	{
		return nullptr;
	}

	BASE_STATS *stats;
	if (orderStateStatsLoc(builder, DORDER_BUILD, &stats)) // Moving to build location?
	{
		return stats;
	}

	if (builder->order.type == DORDER_BUILD && orderStateObj(builder, DORDER_BUILD)) // Is building
	{
		return builder->order.psStats;
	}

	if (builder->order.type == DORDER_HELPBUILD || builder->order.type == DORDER_LINEBUILD) // Is helping
	{
		if (auto structure = orderStateObj(builder, DORDER_HELPBUILD))
		{
			return ((STRUCTURE *)structure)->pStructureType;
		}
	}

	if (orderState(builder, DORDER_DEMOLISH))
	{
		return structGetDemolishStat();
	}

	return nullptr;
}


void BuildInterfaceController::startBuildPosition(BASE_STATS *buildOption)
{
	ASSERT_OR_RETURN(, castDroid(getSelectedObject()) != nullptr, "invalid droid pointer");
	
	triggerEvent(TRIGGER_MENU_BUILD_SELECTED);

	//check for demolish first
	if (buildOption == structGetDemolishStat())
	{
		objMode = IOBJ_DEMOLISHSEL;
	}
	else
	{
		intSetPositionStats(buildOption);

		/* Now start looking for a location for the structure */
		objMode = IOBJ_BUILDSEL;

		intStartStructPosition(buildOption);
	}

	// Close the stats box
	intRemoveStats();
	intMode = INT_OBJECT;

	closeInterface();
}

void BuildInterfaceController::selectBuilder(BASE_OBJECT *builder)
{
	builder->selected = true;
	triggerEventSelected();
	jsDebugSelected(builder);
	setSelectedObject(builder);
}

void BuildInterfaceController::jumpToSelectedBuilder()
{
	auto builder = getSelectedObject();
	setPlayerPos(builder->pos.x, builder->pos.y);
	setWarCamActive(false);
}

void BuildInterfaceController::toggleFavorites(BASE_STATS *buildOption)
{
	auto index = buildOption->index;
	std::weak_ptr<BuildInterfaceController> weakController = shared_from_this();
	widgScheduleTask([index, weakController](){
		if (auto controller = weakController.lock())
		{
			asStructureStats[index].isFavorite = !controller->shouldShowFavorites();
			controller->updateBuildOptionsList();
		}
	});
}

void BuildInterfaceController::refresh()
{
	updateBuildersList();
	updateBuildOptionsList();

	if (objectsSize() == 0)
	{
		closeInterface();
		return;
	}
}

void BuildInterfaceController::closeInterface()
{
	intRemoveStats();
	intRemoveObject();
}

void BuildInterfaceController::updateSelected()
{
	for (auto builder: objects)
	{
		if (builder->died == 0 && builder->selected)
		{
			setSelectedObject(builder);
			return;
		}
	}

	if (auto previouslySelected = getSelectedObject())
	{
		for (auto builder: objects)
		{
			if (builder->died == 0 && builder == previouslySelected)
			{
				setSelectedObject(builder);
				return;
			}
		}
	}

	setSelectedObject(objects.front());
}

void BuildInterfaceController::toggleBuilderSelection(DROID *droid)
{
	if (droid->selected)
	{
		droid->selected = false;
	}
	else
	{
		if (getSelectedObject())
		{
			getSelectedObject()->selected = true;
		}
		selectBuilder(droid);
	}
	triggerEventSelected();
}

class BuildObjectButton : public IntFancyButton
{
private:
	typedef IntFancyButton BaseWidget;

public:
	BuildObjectButton(const std::shared_ptr<BuildInterfaceController> &controller, size_t objectIndex)
		: BaseWidget()
		, buildController(controller)
		, objectIndex(objectIndex)
	{
		buttonType = BTMBUTTON;
	}

	void clicked(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::clicked(context, mouseButton);
		auto droid = castDroid(buildController->getObjectAt(objectIndex));
		ASSERT_OR_RETURN(, droid != nullptr, "Invalid droid pointer");

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			buildController->toggleBuilderSelection(droid);
			return;
		}

		clearSelection();
		buildController->selectBuilder(droid);

		if ((jumpPosition.x == 0 && jumpPosition.y == 0) || !objectOnScreen(droid, 0))
		{
			jumpPosition = getPlayerPos();
			buildController->jumpToSelectedBuilder();
		}
		else
		{
			setPlayerPos(jumpPosition.x, jumpPosition.y);
			setWarCamActive(false);
			jumpPosition = {0, 0};
		}

		buildController->displayStatsForm();
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		auto droid = castDroid(buildController->getObjectAt(objectIndex));
		initDisplay();

		if (droid && !isDead(droid))
		{
			displayIMD(Image(), ImdObject::Droid(droid), xOffset, yOffset);
		}

		displayIfHighlight(xOffset, yOffset);
		doneDisplay();
	}

	std::string getTip() override
	{
		return droidGetName(castDroid(buildController->getObjectAt(objectIndex)));
	}

private:
	std::shared_ptr<BuildInterfaceController> buildController;
	size_t objectIndex;
	Vector2i jumpPosition = {0, 0};
};

class BuildStatsButton: public ObjectStatsButton
{
private:
	typedef	ObjectStatsButton BaseWidget;

protected:
	BuildStatsButton (const std::shared_ptr<BuildInterfaceController> &controller, size_t objectIndex):
		BaseWidget(controller, objectIndex),
		buildController(controller)
	{}

public:
	static std::shared_ptr<BuildStatsButton> make(const std::shared_ptr<BuildInterfaceController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public BuildStatsButton
		{
		public:
			make_shared_enabler (const std::shared_ptr<BuildInterfaceController> &controller, size_t objectIndex):
				BuildStatsButton(controller, objectIndex)
			{}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller, objectIndex);
		widget->initialize();
		return widget;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		auto droid = castDroid(controller->getObjectAt(objectIndex));

		updateProgressBar(droid);
		updateProductionRunSizeLabel(droid);

		BaseWidget::display(xOffset, yOffset);
	}

	std::string getTip() override
	{
		if (auto stats = getStats())
		{
			return getStatsName(stats);
		}

		return "";
	}

private:
	void initialize()
	{
		addProgressBar();
		addProductionRunSizeLabel();
	}

	void addProgressBar()
	{
		W_BARINIT init;
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
	}

	void addProductionRunSizeLabel()
	{
		W_LABINIT init;
		init.style = WIDG_HIDDEN;
		init.x = OBJ_TEXTX;
		init.y = OBJ_T1TEXTY;
		init.width = 16;
		init.height = 16;
		init.pText = WzString::fromUtf8("BUG! (a)");

		attach(productionRunSizeLabel = std::make_shared<W_LABEL>(&init));
	}

	void updateProgressBar(DROID *droid)
	{
		progressBar->hide();

		if (!DroidIsBuilding(droid))
		{
			return;
		}

		ASSERT(droid->asBits[COMP_CONSTRUCT], "Invalid droid type");

		if (auto structure = DroidGetBuildStructure(droid))
		{
			//show progress of build
			if (structure->currentBuildPts != 0)
			{
				formatTime(progressBar.get(), structure->currentBuildPts, structureBuildPointsToCompletion(*structure), structure->lastBuildRate, _("Build Progress"));
			}
			else
			{
				formatPower(progressBar.get(), checkPowerRequest(structure), structure->pStructureType->powerToBuild);
			}
		}
	}

	void updateProductionRunSizeLabel(DROID *droid)
	{
		int remaining = -1;

		STRUCTURE_STATS const *stats = nullptr;
		int count = 0;
		auto processOrder = [&](DroidOrder const &order) {
			STRUCTURE_STATS *newStats = nullptr;
			int deltaCount = 0;
			switch (order.type)
			{
				case DORDER_BUILD:
				case DORDER_LINEBUILD:
					newStats = order.psStats;
					deltaCount = order.type == DORDER_LINEBUILD? 1 + (abs(order.pos.x - order.pos2.x) + abs(order.pos.y - order.pos2.y))/TILE_UNITS : 1;
					break;
				case DORDER_HELPBUILD:
					if (STRUCTURE *target = castStructure(order.psObj))
					{
						newStats = target->pStructureType;
						deltaCount = 1;
					}
					break;
				default:
					return false;
			}
			if (newStats != nullptr && (stats == nullptr || stats == newStats))
			{
				stats = newStats;
				count += deltaCount;
				return true;
			}
			return false;
		};

		if (processOrder(droid->order))
		{
			for (auto const &order: droid->asOrderList)
			{
				if (!processOrder(order))
				{
					break;
				}
			}
		}
		if (count > 1)
		{
			remaining = count;
		}

		if (remaining != -1)
		{
			productionRunSizeLabel->setString(WzString::fromUtf8(astringf("%d", remaining)));
			productionRunSizeLabel->show();
		}
		else
		{
			productionRunSizeLabel->hide();
		}
	}

	void updateProductionRunSize(DROID *droid)
	{
		progressBar->hide();

		if (!DroidIsBuilding(droid))
		{
			return;
		}

		ASSERT(droid->asBits[COMP_CONSTRUCT], "Invalid droid type");

		if (auto structure = DroidGetBuildStructure(droid))
		{
			//show progress of build
			if (structure->currentBuildPts != 0)
			{
				formatTime(progressBar.get(), structure->currentBuildPts, structureBuildPointsToCompletion(*structure), structure->lastBuildRate, _("Build Progress"));
			}
			else
			{
				formatPower(progressBar.get(), checkPowerRequest(structure), structure->pStructureType->powerToBuild);
			}
		}
	}

	bool isSelected() const override
	{
		auto droid = controller->getObjectAt(objectIndex);
		return droid && (droid->selected || droid == controller->getSelectedObject());
	}

	void clicked(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::clicked(context, mouseButton);
		auto droid = castDroid(controller->getObjectAt(objectIndex));
		ASSERT_OR_RETURN(, droid != nullptr, "Invalid droid pointer");

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			buildController->toggleBuilderSelection(droid);
		}
		else
		{
			clearSelection();
			buildController->selectBuilder(droid);
		}

		buildController->displayStatsForm();
	}

	std::shared_ptr<W_BARGRAPH> progressBar;
	std::shared_ptr<W_LABEL> productionRunSizeLabel;
	std::shared_ptr<BuildInterfaceController> buildController;
};

class BuildOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;

	BuildOptionButton(): BaseWidget() {}
public:
	static std::shared_ptr<BuildOptionButton> make(const std::shared_ptr<BuildInterfaceController> &controller, size_t buildOptionIndex)
	{
		class make_shared_enabler: public BuildOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->buildOptionIndex = buildOptionIndex;
		widget->initialize();
		return widget;
	}

private:
	BASE_STATS *getStats() override
	{
		return controller->getStatsAt(buildOptionIndex);
	}

	void initialize()
	{
		W_BARINIT sBarInit;
		sBarInit.x = STAT_TIMEBARX;
		sBarInit.y = STAT_TIMEBARY;
		sBarInit.width = STAT_PROGBARWIDTH;
		sBarInit.height = STAT_PROGBARHEIGHT;
		sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
		sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

		sBarInit.iRange = GAME_TICKS_PER_SEC;
		attach(bar = std::make_shared<W_BARGRAPH>(&sBarInit));
		bar->setBackgroundColour(WZCOL_BLACK);
	}

	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		BaseWidget::display(xOffset, yOffset);
	}

	bool isSelected() const override
	{
		if (auto form = statsForm.lock())
		{
			return form->getSelectedStats() == controller->getStatsAt(buildOptionIndex);
		}

		return false;
	}

	void updateLayout()
	{
		auto stat = getStats();
		auto powerCost = ((STRUCTURE_STATS *)stat)->powerToBuild;
		bar->majorSize = std::min(100, (int32_t)(powerCost / POWERPOINTS_DROIDDIV));
	}

	std::string getTip() override
	{
		auto stat = getStats();
		auto powerCost = ((STRUCTURE_STATS *)stat)->powerToBuild;
		WzString costString = WzString::fromUtf8(_("\nCost: %1"));
		costString.replace("%1", WzString::number(powerCost));
		WzString tipString = getStatsName(stat);
		tipString.append(costString);
		return tipString.toUtf8();
	}

	void run(W_CONTEXT *context) override
	{
		BaseWidget::run(context);

		if (isMouseOverWidget())
		{
			auto stats = controller->getStatsAt(buildOptionIndex);
			intSetShadowPower(((STRUCTURE_STATS *)stats)->powerToBuild);
		}
	}

	void clicked(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::clicked(context, mouseButton);
		auto clickedStats = controller->getStatsAt(buildOptionIndex);
		ASSERT_OR_RETURN(, clickedStats != nullptr, "Invalid template pointer");

		if (mouseButton == WKEY_PRIMARY)
		{
			controller->startBuildPosition(clickedStats);
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			controller->toggleFavorites(clickedStats);
		}
	}

	std::shared_ptr<BuildInterfaceController> controller;
	std::shared_ptr<W_BARGRAPH> bar;
	size_t buildOptionIndex;
};

class BuildObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;

protected:
	BuildObjectsForm(const std::shared_ptr<BuildInterfaceController> &controller): BaseWidget(controller), buildController(controller)
	{
	}

public:
	static std::shared_ptr<BuildObjectsForm> make(const std::shared_ptr<BuildInterfaceController> &controller)
	{
		class make_shared_enabler: public BuildObjectsForm
		{
		public:
			make_shared_enabler(const std::shared_ptr<BuildInterfaceController> &controller): BuildObjectsForm(controller)
			{
			}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller);
		widget->initialize();
		return widget;
	}

	std::shared_ptr<ObjectStatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return BuildStatsButton::make(buildController, buttonIndex);
	}

	std::shared_ptr<IntFancyButton> makeObjectButton(size_t buttonIndex) const override
	{
		return std::make_shared<BuildObjectButton>(buildController, buttonIndex);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		buildController->updateSelected();
		BaseWidget::display(xOffset, yOffset);
	}

private:
	std::shared_ptr<BuildInterfaceController> buildController;
};

class BuildStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<BuildStatsForm> make(const std::shared_ptr<BuildInterfaceController> &controller)
	{
		class make_shared_enabler: public BuildStatsForm
		{
		public:
			make_shared_enabler(const std::shared_ptr<BuildInterfaceController> &controller): BuildStatsForm(controller)
			{
			}
		};
		auto widget = std::make_shared<make_shared_enabler>(controller);
		widget->buildController = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return BuildOptionButton::make(buildController, buttonIndex);
	}

private:
	void initialize() override
	{
		BaseWidget::initialize();
		addObsoleteButton();
		addFavoriteButton();
	}

	void addObsoleteButton()
	{
		attach(obsoleteButton = std::make_shared<MultipleChoiceButton>());
		obsoleteButton->style |= WBUT_SECONDARY;
		obsoleteButton->setChoice(buildController->shouldShowRedundantDesign());
		obsoleteButton->setImages(false, MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
		obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
		obsoleteButton->setImages(true,  MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
		obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
		obsoleteButton->move(4 + Image(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildInterfaceController>(buildController);
		obsoleteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto buildController = weakController.lock())
			{
				auto &obsoleteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !obsoleteButton.getChoice();
				buildController->setShouldShowRedundantDesign(newValue);
				obsoleteButton.setChoice(newValue);
			}
		});
	}

	void addFavoriteButton()
	{
		attach(favoriteButton = std::make_shared<MultipleChoiceButton>());
		favoriteButton->style |= WBUT_SECONDARY;
		favoriteButton->setChoice(buildController->shouldShowFavorites());
		favoriteButton->setImages(false, MultipleChoiceButton::Images(Image(IntImages, IMAGE_ALLY_RESEARCH), Image(IntImages, IMAGE_ALLY_RESEARCH), Image(IntImages, IMAGE_ALLY_RESEARCH)));
		favoriteButton->setTip(false, _("Showing All Tech\nRight-click to add to Favorites"));
		favoriteButton->setImages(true,  MultipleChoiceButton::Images(Image(IntImages, IMAGE_ALLY_RESEARCH_TC), Image(IntImages, IMAGE_ALLY_RESEARCH_TC), Image(IntImages, IMAGE_ALLY_RESEARCH_TC)));
		favoriteButton->setTip(true, _("Showing Only Favorite Tech\nRight-click to remove from Favorites"));
		favoriteButton->move(4 * 2 + Image(IntImages, IMAGE_FDP_UP).width() * 2 + 4 * 2, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildInterfaceController>(buildController);
		favoriteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto buildController = weakController.lock())
			{
				auto &favoriteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !favoriteButton.getChoice();
				buildController->setShouldShowFavorite(newValue);
				favoriteButton.setChoice(newValue);
			}
		});
	}

	std::shared_ptr<BuildInterfaceController> buildController;
	std::shared_ptr<MultipleChoiceButton> obsoleteButton;
	std::shared_ptr<MultipleChoiceButton> favoriteButton;
};

bool BuildInterfaceController::showInterface()
{
	intRemoveStatsNoAnim();
	intRemoveOrderNoAnim();

	if (objects.empty())
	{
		return false;
	}

	auto objectsForm = BuildObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);

	displayStatsForm();

	intMode = INT_STAT;
	triggerEvent(TRIGGER_MENU_BUILD_UP);

	return true;
}

void BuildInterfaceController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto statForm = BuildStatsForm::make(shared_from_this());
		psWScreen->psForm->attach(statForm);
	}
}
