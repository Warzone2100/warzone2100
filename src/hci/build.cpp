#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "build.h"
#include "../objmem.h"
#include "../order.h"
#include "../qtscript.h"
#include "../power.h"
#include "../map.h"

DROID *BuildController::highlightedBuilder = nullptr;
bool BuildController::showFavorites = false;

void BuildController::updateData()
{
	updateBuildersList();
	updateHighlighted();
	updateBuildOptionsList();
}

void BuildController::updateBuildersList()
{
	builders.clear();

	ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "selectedPlayer = %" PRIu32 "", selectedPlayer);

	for (DROID *droid = apsDroidLists[selectedPlayer]; droid; droid = droid->psNext)
	{
		if (isConstructionDroid(droid) && droid->died == 0)
		{
			builders.push_back(droid);
		}
	}

	std::reverse(builders.begin(), builders.end());
}

void BuildController::updateBuildOptionsList()
{
	auto newBuildOptions = fillStructureList(selectedPlayer, MAXSTRUCTURES - 1, shouldShowFavorites());

	stats = std::vector<STRUCTURE_STATS *>(newBuildOptions.begin(), newBuildOptions.end());
}

STRUCTURE_STATS *BuildController::getObjectStatsAt(size_t objectIndex) const
{
	auto builder = getObjectAt(objectIndex);
	if (!builder)
	{
		return nullptr;
	}

	if (!(droidType(builder) == DROID_CONSTRUCT || droidType(builder) == DROID_CYBORG_CONSTRUCT))
	{
		return nullptr;
	}

	STRUCTURE_STATS *builderStats;
	if (orderStateStatsLoc(builder, DORDER_BUILD, &builderStats)) // Moving to build location?
	{
		return builderStats;
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


void BuildController::startBuildPosition(STRUCTURE_STATS *buildOption)
{
	auto builder = getHighlightedObject();
	ASSERT_NOT_NULLPTR_OR_RETURN(, builder);

	triggerEvent(TRIGGER_MENU_BUILD_SELECTED);

	if (buildOption == structGetDemolishStat())
	{
		objMode = IOBJ_DEMOLISHSEL;
	}
	else
	{
		objMode = IOBJ_BUILDSEL;
		intStartConstructionPosition(builder, buildOption);
	}

	intRemoveStats();
	intMode = INT_OBJECT;
}

void BuildController::toggleFavorites(STRUCTURE_STATS *buildOption)
{
	asStructureStats[buildOption->index].isFavorite = !shouldShowFavorites();
	updateBuildOptionsList();
}

void BuildController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
}

void BuildController::clearData()
{
	builders.clear();
	setHighlightedObject(nullptr);
	stats.clear();
}

void BuildController::toggleBuilderSelection(DROID *droid)
{
	if (droid->selected)
	{
		droid->selected = false;
	}
	else
	{
		if (auto highlightedObject = getHighlightedObject())
		{
			highlightedObject->selected = true;
		}
		selectObject(droid);
	}
	triggerEventSelected();
}

void BuildController::setHighlightedObject(BASE_OBJECT *object)
{
	if (object == nullptr)
	{
		highlightedBuilder = nullptr;
		return;
	}

	auto builder = castDroid(object);
	ASSERT_NOT_NULLPTR_OR_RETURN(, builder);
	ASSERT_OR_RETURN(, isConstructionDroid(builder), "Droid is not a construction droid");
	highlightedBuilder = builder;
}

class BuildObjectButton : public ObjectButton
{
private:
	typedef ObjectButton BaseWidget;

public:
	BuildObjectButton(const std::shared_ptr<BuildController> &controller, size_t newObjectIndex)
		: controller(controller)
	{
		objectIndex = newObjectIndex;
	}

	void clickPrimary() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			controller->toggleBuilderSelection(droid);
			return;
		}

		controller->clearSelection();
		controller->selectObject(controller->getObjectAt(objectIndex));
		jump();

		BaseStatsController::scheduleDisplayStatsForm(controller);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		if (isDead(droid))
		{
			ASSERT_FAILURE(!isDead(droid), "!isDead(droid)", AT_MACRO, __FUNCTION__, "Droid is dead");
			// ensure the backing information is refreshed before the next draw
			intRefreshScreen();
			return;
		}
		displayIMD(AtlasImage(), ImdObject::Droid(droid), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	BuildController &getController() const override
	{
		return *controller.get();
	}

	std::string getTip() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN("", droid);
		return droidGetName(droid);
	}

private:
	std::shared_ptr<BuildController> controller;
};

class BuildStatsButton: public StatsButton
{
private:
	typedef	StatsButton BaseWidget;

protected:
	BuildStatsButton() {}

public:
	static std::shared_ptr<BuildStatsButton> make(const std::shared_ptr<BuildController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public BuildStatsButton {};
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
		auto stat = getStats();
		displayIMD(AtlasImage(), stat ? ImdObject::StructureStat(stat): ImdObject::Component(nullptr), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto droid = controller->getObjectAt(objectIndex);
		updateProgressBar(droid);
		updateProductionRunSizeLabel(droid);
	}

private:
	STRUCTURE_STATS *getStats() override
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
		W_LABINIT init;
		init.style = WIDG_HIDDEN;
		init.x = OBJ_TEXTX;
		init.y = OBJ_T1TEXTY;
		init.width = 16;
		init.height = 16;
		init.pText = WzString::fromUtf8("BUG! (a)");

		attach(productionRunSizeLabel = std::make_shared<W_LABEL>(&init));
		productionRunSizeLabel->setFontColour(WZCOL_ACTION_PRODUCTION_RUN_TEXT);
	}

	void updateProgressBar(DROID *droid)
	{
		progressBar->hide();

		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
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
					deltaCount = order.type == DORDER_LINEBUILD? 1 + (abs(order.pos.x - order.pos2.x) + abs(order.pos.y - order.pos2.y)) / TILE_UNITS : 1;
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

		if (droid && processOrder(droid->order))
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

	bool isHighlighted() const override
	{
		auto droid = controller->getObjectAt(objectIndex);
		return droid && (droid->selected || droid == controller->getHighlightedObject());
	}

	void clickPrimary() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			controller->toggleBuilderSelection(droid);
		}
		else
		{
			controller->clearSelection();
			controller->selectObject(droid);
		}

		BaseStatsController::scheduleDisplayStatsForm(controller);
	}

	void clickSecondary() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		auto highlighted = controller->getHighlightedObject();

		// prevent highlighting a builder when another builder is already selected
		if (droid == highlighted || !highlighted->selected)
		{
			controller->setHighlightedObject(droid);
			BaseStatsController::scheduleDisplayStatsForm(controller);
		}
	}

	std::shared_ptr<W_LABEL> productionRunSizeLabel;
	std::shared_ptr<BuildController> controller;
	size_t objectIndex;
};

class BuildOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<BuildOptionButton> make(const std::shared_ptr<BuildController> &controller, size_t buildOptionIndex)
	{
		class make_shared_enabler: public BuildOptionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->buildOptionIndex = buildOptionIndex;
		widget->initialize();
		return widget;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto stat = getStats();
		ASSERT_NOT_NULLPTR_OR_RETURN(, stat);

		displayIMD(AtlasImage(), ImdObject::StructureStat(stat), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

private:
	STRUCTURE_STATS *getStats() override
	{
		return controller->getStatsAt(buildOptionIndex);
	}

	void initialize()
	{
		addCostBar();
	}

	bool isHighlighted() const override
	{
		return controller->isHighlightedObjectStats(buildOptionIndex);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();

		if (isMouseOverWidget())
		{
			intSetShadowPower(getCost());
		}

		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
	}

	uint32_t getCost() override
	{
		STRUCTURE_STATS * psStats = getStats();
		return psStats ? psStats->powerToBuild : 0;
	}

	void clickPrimary() override
	{
		auto clickedStats = controller->getStatsAt(buildOptionIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		auto controllerRef = controller;
		widgScheduleTask([clickedStats, controllerRef]() {
			controllerRef->startBuildPosition(clickedStats);
		});
	}

	void clickSecondary() override
	{
		auto clickedStats = controller->getStatsAt(buildOptionIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		auto controllerRef = controller;
		widgScheduleTask([clickedStats, controllerRef]() {
			controllerRef->toggleFavorites(clickedStats);
		});
	}

	std::shared_ptr<BuildController> controller;
	size_t buildOptionIndex;
};

class BuildObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<BuildObjectsForm> make(const std::shared_ptr<BuildController> &controller)
	{
		class make_shared_enabler: public BuildObjectsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return BuildStatsButton::make(controller, buttonIndex);
	}

	std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const override
	{
		return std::make_shared<BuildObjectButton>(controller, buttonIndex);
	}

protected:
	BuildController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<BuildController> controller;
};

class BuildStatsForm: public ObjectStatsForm
{
private:
	typedef ObjectStatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<BuildStatsForm> make(const std::shared_ptr<BuildController> &controller)
	{
		class make_shared_enabler: public BuildStatsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const override
	{
		return BuildOptionButton::make(controller, buttonIndex);
	}

protected:
	BuildController &getController() const override
	{
		return *controller.get();
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
		obsoleteButton->setChoice(controller->shouldShowRedundantDesign());
		obsoleteButton->setImages(false, MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
		obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
		obsoleteButton->setImages(true,  MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_UP), AtlasImage(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
		obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
		obsoleteButton->move(4 + AtlasImage(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildController>(controller);
		obsoleteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto buildController = weakController.lock())
			{
				auto &_obsoleteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !_obsoleteButton.getChoice();
				buildController->setShouldShowRedundantDesign(newValue);
				_obsoleteButton.setChoice(newValue);
			}
		});
	}

	void addFavoriteButton()
	{
		attach(favoriteButton = std::make_shared<MultipleChoiceButton>());
		favoriteButton->style |= WBUT_SECONDARY;
		favoriteButton->setChoice(controller->shouldShowFavorites());
		favoriteButton->setImages(false, MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_ALLY_RESEARCH), AtlasImage(IntImages, IMAGE_ALLY_RESEARCH), AtlasImage(IntImages, IMAGE_ALLY_RESEARCH)));
		favoriteButton->setTip(false, _("Showing All Tech\nRight-click to add to Favorites"));
		favoriteButton->setImages(true,  MultipleChoiceButton::Images(AtlasImage(IntImages, IMAGE_ALLY_RESEARCH_TC), AtlasImage(IntImages, IMAGE_ALLY_RESEARCH_TC), AtlasImage(IntImages, IMAGE_ALLY_RESEARCH_TC)));
		favoriteButton->setTip(true, _("Showing Only Favorite Tech\nRight-click to remove from Favorites"));
		favoriteButton->move(4 * 2 + AtlasImage(IntImages, IMAGE_FDP_UP).width() * 2 + 4 * 2, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildController>(controller);
		favoriteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto buildController = weakController.lock())
			{
				auto &_favoriteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !_favoriteButton.getChoice();
				buildController->setShouldShowFavorite(newValue);
				_favoriteButton.setChoice(newValue);
			}
		});
	}

	std::shared_ptr<BuildController> controller;
	std::shared_ptr<MultipleChoiceButton> obsoleteButton;
	std::shared_ptr<MultipleChoiceButton> favoriteButton;
};

bool BuildController::showInterface()
{
	updateData();
	if (builders.empty())
	{
		return false;
	}

	auto objectsForm = BuildObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);
	displayStatsForm();
	triggerEvent(TRIGGER_MENU_BUILD_UP);
	return true;
}

std::shared_ptr<StatsForm> BuildController::makeStatsForm()
{
	return BuildStatsForm::make(shared_from_this());
}
