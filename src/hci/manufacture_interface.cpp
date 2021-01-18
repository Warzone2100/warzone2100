#include <memory>
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "manufacture_interface.h"
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
#include "../template.h"

#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

void ManufactureInterfaceController::updateData()
{
	updateFactoriesList();
	updateSelected();
	updateManufactureOptionsList();
}

void ManufactureInterfaceController::adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add)
{
	auto factory = castStructure(getSelectedObject());
	ASSERT_OR_RETURN(, factory != nullptr, "Invalid factory pointer");
	factoryProdAdjust(castStructure(getSelectedObject()), manufactureOption, add);
}

void ManufactureInterfaceController::adjustFactoryLoop(bool add)
{
	auto factory = castStructure(getSelectedObject());
	ASSERT_OR_RETURN(, factory != nullptr, "Invalid factory pointer");

	factoryLoopAdjust(factory, add);
}

void ManufactureInterfaceController::startDeliveryPointPosition()
{
	if (auto factory = castStructure(getSelectedObject()))
	{
		// make sure that the factory isn't assigned to a commander
		assignFactoryCommandDroid(factory, nullptr);
		auto psFlag = FindFactoryDelivery(factory);
		if (psFlag)
		{
			startDeliveryPosition(psFlag);
		}
	}
}

void ManufactureInterfaceController::updateFactoriesList()
{
	factories.clear();

	for (auto psBuilding = interfaceStructList(); psBuilding != nullptr; psBuilding = psBuilding->psNext)
	{
		if (psBuilding->status != SS_BUILT || psBuilding->died != 0)
		{
			continue;
		}

		auto structureType = psBuilding->pStructureType->type;
		if (structureType == REF_FACTORY || structureType == REF_CYBORG_FACTORY || structureType == REF_VTOL_FACTORY)
		{
			factories.push_back(psBuilding);
		}
	}
}

void ManufactureInterfaceController::updateManufactureOptionsList()
{
	auto factory = castStructure(getSelectedObject());
	if (!factory)
	{
		stats.clear();
		return;
	}

	auto newManufactureOptions = fillTemplateList(factory);

	stats = std::vector<DROID_TEMPLATE *>(newManufactureOptions.begin(), newManufactureOptions.end());
}

DROID_TEMPLATE *ManufactureInterfaceController::getObjectStatsAt(size_t objectIndex) const
{
	auto factory = getObjectAt(objectIndex);
	ASSERT_OR_RETURN(nullptr, factory != nullptr, "Invalid Structure pointer");

	return ((FACTORY *)factory->pFunctionality)->psSubject;
}

void ManufactureInterfaceController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
}

class ManufactureObjectButton : public ObjectButton
{
private:
	typedef ObjectButton BaseWidget;

public:
	ManufactureObjectButton(const std::shared_ptr<ManufactureInterfaceController> &controller, size_t newObjectIndex)
		: BaseWidget()
		, controller(controller)
	{
		objectIndex = newObjectIndex;
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		selectAndJump();
		controller->displayStatsForm();
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_OR_RETURN(, factory != nullptr && !isDead(factory), "Invalid factory pointer");
		displayIMD(Image(), ImdObject::Structure(factory), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	std::string getTip() override
	{
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_OR_RETURN("", factory != nullptr, "Invalid factory pointer");
		return getStatsName(factory->pStructureType);
	}

	std::shared_ptr<BaseObjectsController> getController() const override
	{
		return controller;
	}

private:
	std::shared_ptr<ManufactureInterfaceController> controller;
};

class ProductionRunSizeLabel
{
private:
	std::shared_ptr<W_LABEL> label;

public:
	void initialize(WIDGET &parent)
	{
		auto init = W_LABINIT();
		init.x = OBJ_TEXTX;
		init.y = OBJ_T1TEXTY;
		init.width = 16;
		init.height = 16;
		parent.attach(label = std::make_shared<W_LABEL>(&init));
	}

	void hide()
	{
		label->hide();
	}

	void update(STRUCTURE *factory, DROID_TEMPLATE *droidTemplate)
	{
		label->hide();

		if (!StructureIsManufacturingPending(factory))
		{
			return;
		}

		auto remaining = getProduction(factory, droidTemplate).numRemaining();
		if (remaining > 0)
		{
			label->setString(WzString::fromUtf8(astringf("%d", remaining)));
			label->show();
		}
	}
};

class ManufactureStatsButton: public StatsButton
{
private:
	typedef	StatsButton BaseWidget;

protected:
	ManufactureStatsButton() {}

public:
	static std::shared_ptr<ManufactureStatsButton> make(const std::shared_ptr<ManufactureInterfaceController> &controller, size_t objectIndex)
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
		auto stat = getStats();
		displayIMD(Image(), stat ? ImdObject::DroidTemplate(stat): ImdObject::Component(nullptr), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto factory = controller->getObjectAt(objectIndex);
		updateProgressBar(factory);
		productionRunSizeLabel.update(factory, FactoryGetTemplate(StructureGetFactory(factory)));
	}

private:
	DROID_TEMPLATE *getStats() override
	{
		return controller->getObjectStatsAt(objectIndex);
	}

	void initialize()
	{
		addProgressBar();
		productionRunSizeLabel.initialize(*this);
	}

	void updateProgressBar(STRUCTURE *factory)
	{
		progressBar->hide();

		ASSERT_OR_RETURN(, !isDead(factory), "Factory is dead");

		if (!StructureIsManufacturingPending(factory))
		{
			return;
		}

		auto manufacture = StructureGetFactory(factory);

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

	bool isSelected() const override
	{
		auto factory = controller->getObjectAt(objectIndex);
		return factory && (factory->selected || factory == controller->getSelectedObject());
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_OR_RETURN(, factory != nullptr, "Invalid factory pointer");

		clearSelection();
		controller->selectObject(factory);

		controller->displayStatsForm();
	}

	std::shared_ptr<ManufactureInterfaceController> controller;
	ProductionRunSizeLabel productionRunSizeLabel;
	size_t objectIndex;
};

class ManufactureOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureOptionButton> make(const std::shared_ptr<ManufactureInterfaceController> &controller, size_t manufactureOptionIndex)
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
		ASSERT_OR_RETURN(, stat != nullptr, "Invalid stat pointer");

		displayIMD(Image(), ImdObject::DroidTemplate(stat), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

private:
	DROID_TEMPLATE *getStats() override
	{
		return controller->getStatsAt(manufactureOptionIndex);
	}

	void initialize()
	{
		addCostBar();
		productionRunSizeLabel.initialize(*this);
	}

	bool isSelected() const override
	{
		if (auto form = statsForm.lock())
		{
			return form->getSelectedStats() == controller->getStatsAt(manufactureOptionIndex);
		}

		return false;
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
		productionRunSizeLabel.update(castStructure(controller->getSelectedObject()), getStats());
	}

	uint32_t getCost() override
	{
		return calcTemplatePower(getStats());
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
		auto clickedStats = controller->getStatsAt(manufactureOptionIndex);
		ASSERT_OR_RETURN(, clickedStats != nullptr, "Invalid template pointer");

		auto controllerRef = controller;
		widgScheduleTask([mouseButton, clickedStats, controllerRef]() {
			controllerRef->adjustFactoryProduction(clickedStats, mouseButton == WKEY_PRIMARY);
		});
	}

	std::shared_ptr<ManufactureInterfaceController> controller;
	ProductionRunSizeLabel productionRunSizeLabel;
	size_t manufactureOptionIndex;
};

class ManufactureObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureObjectsForm> make(const std::shared_ptr<ManufactureInterfaceController> &controller)
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
		return std::make_shared<ManufactureObjectButton>(controller, buttonIndex);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		controller->updateSelected();
		BaseWidget::display(xOffset, yOffset);
	}

	std::shared_ptr<BaseObjectsController> getController() const override
	{
		return controller;
	}

private:
	std::shared_ptr<ManufactureInterfaceController> controller;
};

class ManufactureStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;
	using BaseWidget::BaseWidget;

	class LoopProductionButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		LoopProductionButton(const std::shared_ptr<ManufactureInterfaceController> &controller): BaseWidget(), controller(controller)
		{
			style |= WBUT_SECONDARY;
			setImages(Image(IntImages, IMAGE_LOOP_UP), Image(IntImages, IMAGE_LOOP_DOWN), Image(IntImages, IMAGE_LOOP_HI));
			setTip(_("Loop Production"));
		}

		void released(W_CONTEXT *psContext, WIDGET_KEY key) override
		{
			BaseWidget::released(psContext, key);
			controller->adjustFactoryLoop(key == WKEY_PRIMARY);
		}
	private:
		std::shared_ptr<ManufactureInterfaceController> controller;
	};

public:
	static std::shared_ptr<ManufactureStatsForm> make(const std::shared_ptr<ManufactureInterfaceController> &controller)
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
	std::shared_ptr<BaseStatsController> getController() const override
	{
		return controller;
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		updateDeliveryPointButton();
		updateLoopProduction();
	}

	void updateDeliveryPointButton()
	{
		auto factory = castStructure(controller->getSelectedObject());

		switch (factory->pStructureType->type)
		{
		default:
		case REF_FACTORY:
			deliveryPointButton->setImages(Image(IntImages, IMAGE_FDP_UP), Image(IntImages, IMAGE_FDP_DOWN), Image(IntImages, IMAGE_FDP_HI));
			break;
		case REF_CYBORG_FACTORY:
			deliveryPointButton->setImages(Image(IntImages, IMAGE_CDP_UP), Image(IntImages, IMAGE_CDP_DOWN), Image(IntImages, IMAGE_CDP_HI));
			break;
		case REF_VTOL_FACTORY:
			deliveryPointButton->setImages(Image(IntImages, IMAGE_VDP_UP), Image(IntImages, IMAGE_VDP_DOWN), Image(IntImages, IMAGE_VDP_HI));
			break;
		}
	}

	void updateLoopProduction()
	{
		auto structure = castStructure(controller->getSelectedObject());
		ASSERT_OR_RETURN(, structure != nullptr, "Invalid structure pointer");
		auto psFactory = (FACTORY *)structure->pFunctionality;
		ASSERT_OR_RETURN(, psFactory != nullptr, "Invalid factory pointer");

		auto productionLoops = psFactory->productionLoops;

		if (psFactory->psSubject == nullptr || productionLoops == 0)
		{
			loopProductionButton->setState(0);
			loopProductionLabel->hide();
			return;
		}

		loopProductionButton->setState(WBUT_CLICKLOCK);
		loopProductionLabel->setString(productionLoops == INFINITE_PRODUCTION ? WzString::fromUtf8("∞"): WzString::fromUtf8(astringf("%u", productionLoops + 1)));
		loopProductionLabel->show();
	}

private:
	void initialize() override
	{
		BaseWidget::initialize();
		addObsoleteButton();
		addDeliveryPointButton();
		addLoopProductionWidgets();
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

		auto weakController = std::weak_ptr<ManufactureInterfaceController>(controller);
		obsoleteButton->addOnClickHandler([weakController](W_BUTTON &button) {
			if (auto manufactureController = weakController.lock())
			{
				auto &obsoleteButton = static_cast<MultipleChoiceButton &>(button);
				auto newValue = !obsoleteButton.getChoice();
				manufactureController->setShouldShowRedundantDesign(newValue);
				obsoleteButton.setChoice(newValue);
			}
		});
	}

	void addDeliveryPointButton()
	{
		attach(deliveryPointButton = std::make_shared<W_BUTTON>());
		deliveryPointButton->style |= WBUT_SECONDARY;
		deliveryPointButton->move(4, STAT_SLDY);
		deliveryPointButton->setTip(_("Factory Delivery Point"));

		auto weakController = std::weak_ptr<ManufactureInterfaceController>(controller);
		deliveryPointButton->addOnClickHandler([weakController](W_BUTTON &) {
			auto controller = weakController.lock();
			ASSERT_OR_RETURN(, controller != nullptr, "Invalid controller pointer");
			controller->startDeliveryPointPosition();
		});
	}

	void addLoopProductionWidgets()
	{
		attach(loopProductionButton = std::make_shared<LoopProductionButton>(controller));
		loopProductionButton->move(STAT_SLDX + STAT_SLDWIDTH + 2, STAT_SLDY);

		attach(loopProductionLabel = std::make_shared<W_LABEL>());
		loopProductionLabel->setGeometry(loopProductionButton->x() - 15, loopProductionButton->y(), 12, 15);
	}

	std::shared_ptr<ManufactureInterfaceController> controller;
	std::shared_ptr<MultipleChoiceButton> obsoleteButton;
	std::shared_ptr<W_BUTTON> deliveryPointButton;
	std::shared_ptr<LoopProductionButton> loopProductionButton;
	std::shared_ptr<W_LABEL> loopProductionLabel;
};

bool ManufactureInterfaceController::showInterface()
{
	closeInterfaceNoAnim();

	updateData();
	if (factories.empty())
	{
		return false;
	}

	auto objectsForm = ManufactureObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);

	displayStatsForm();

	intMode = INT_STAT;
	triggerEvent(TRIGGER_MENU_MANUFACTURE_UP);

	return true;
}

void ManufactureInterfaceController::displayStatsForm()
{
	if (widgGetFromID(psWScreen, IDSTAT_FORM) == nullptr)
	{
		auto statForm = ManufactureStatsForm::make(shared_from_this());
		psWScreen->psForm->attach(statForm);
		intMode = INT_STAT;
	}
}
