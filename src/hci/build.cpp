#include <memory>
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "build.h"
#include "../objmem.h"
#include "../hci.h"
#include "../statsdef.h"
#include "../order.h"
#include "../intorder.h"
#include "../mission.h"
#include "../qtscript.h"
#include "../display3d.h"
#include "../warcam.h"
#include "../geometry.h"
#include "../intdisplay.h"

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

	STRUCTURE_STATS *stats;
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

void BuildController::toggleBuilderSelection(DROID *droid)
{
	if (droid->selected)
	{
		droid->selected = false;
	}
	else
	{
		if (auto selectedObject = getHighlightedObject())
		{
			selectedObject->selected = true;
		}
		selectObject(droid);
	}
	triggerEventSelected();
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

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			controller->toggleBuilderSelection(droid);
			return;
		}

		selectAndJump();

		controller->displayStatsForm();
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_OR_RETURN(, droid != nullptr && !isDead(droid), "Invalid droid pointer");
		displayIMD(Image(), ImdObject::Droid(droid), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	std::shared_ptr<BaseObjectsController> getController() const override
	{
		return controller;
	}

	std::string getTip() override
	{
		return droidGetName(controller->getObjectAt(objectIndex));
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
		displayIMD(Image(), stat ? ImdObject::StructureStat(stat): ImdObject::Component(nullptr), xOffset, yOffset);
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

	bool isSelected() const override
	{
		auto droid = controller->getObjectAt(objectIndex);
		return droid && (droid->selected || droid == controller->getHighlightedObject());
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);

		if (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT))
		{
			controller->toggleBuilderSelection(droid);
		}
		else
		{
			clearSelection();
			controller->selectObject(droid);
		}

		controller->displayStatsForm();
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

		displayIMD(Image(), ImdObject::StructureStat(stat), xOffset, yOffset);
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

	bool isSelected() const override
	{
		if (auto form = statsForm.lock())
		{
			return form->getSelectedStats() == controller->getStatsAt(buildOptionIndex);
		}

		return false;
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
	}

	uint32_t getCost() override
	{
		return getStats()->powerToBuild;
	}

	void run(W_CONTEXT *context) override
	{
		BaseWidget::run(context);

		if (isMouseOverWidget())
		{
			intSetShadowPower(getCost());
		}
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto clickedStats = controller->getStatsAt(buildOptionIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		auto controllerRef = controller;
		widgScheduleTask([mouseButton, clickedStats, controllerRef]() {
			if (mouseButton == WKEY_PRIMARY)
			{
				controllerRef->startBuildPosition(clickedStats);
			}
			else if (mouseButton == WKEY_SECONDARY)
			{
				controllerRef->toggleFavorites(clickedStats);
			}
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
	std::shared_ptr<BuildController> controller;
};

class BuildStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;
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
	std::shared_ptr<BaseStatsController> getController() const override
	{
		return controller;
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
		obsoleteButton->setImages(false, MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
		obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
		obsoleteButton->setImages(true,  MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
		obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
		obsoleteButton->move(4 + Image(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildController>(controller);
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
		favoriteButton->setChoice(controller->shouldShowFavorites());
		favoriteButton->setImages(false, MultipleChoiceButton::Images(Image(IntImages, IMAGE_ALLY_RESEARCH), Image(IntImages, IMAGE_ALLY_RESEARCH), Image(IntImages, IMAGE_ALLY_RESEARCH)));
		favoriteButton->setTip(false, _("Showing All Tech\nRight-click to add to Favorites"));
		favoriteButton->setImages(true,  MultipleChoiceButton::Images(Image(IntImages, IMAGE_ALLY_RESEARCH_TC), Image(IntImages, IMAGE_ALLY_RESEARCH_TC), Image(IntImages, IMAGE_ALLY_RESEARCH_TC)));
		favoriteButton->setTip(true, _("Showing Only Favorite Tech\nRight-click to remove from Favorites"));
		favoriteButton->move(4 * 2 + Image(IntImages, IMAGE_FDP_UP).width() * 2 + 4 * 2, STAT_SLDY);

		auto weakController = std::weak_ptr<BuildController>(controller);
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

void BuildController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto statForm = BuildStatsForm::make(shared_from_this());
		psWScreen->psForm->attach(statForm);
		intMode = INT_STAT;
	}
}
