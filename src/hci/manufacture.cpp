#include <memory>
#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "manufacture.h"
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

STRUCTURE *ManufactureController::highlightedFactory = nullptr;

FACTORY *getFactoryOrNullptr(STRUCTURE *factory)
{
	ASSERT_OR_RETURN(nullptr, StructIsFactory(factory), "Invalid factory pointer");
	return (FACTORY *)factory->pFunctionality;
}

void ManufactureController::updateData()
{
	updateFactoriesList();
	updateHighlighted();
	updateManufactureOptionsList();
}

void ManufactureController::adjustFactoryProduction(DROID_TEMPLATE *manufactureOption, bool add)
{
	factoryProdAdjust(getHighlightedObject(), manufactureOption, add);
}

void ManufactureController::adjustFactoryLoop(bool add)
{
	factoryLoopAdjust(getHighlightedObject(), add);
}

void ManufactureController::releaseFactoryProduction(STRUCTURE *structure)
{
	releaseProduction(structure, ModeQueue);
}

void ManufactureController::cancelFactoryProduction(STRUCTURE *structure)
{
	if (!StructureIsManufacturingPending(structure))
	{
		return;
	}

	if (!StructureIsOnHoldPending(structure))
	{
		holdProduction(structure, ModeQueue);
		return;
	}

	cancelProduction(structure, ModeQueue);
	audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
}

void ManufactureController::startDeliveryPointPosition()
{
	auto factory = getHighlightedObject();
	ASSERT_NOT_NULLPTR_OR_RETURN(, factory);

	// make sure that the factory isn't assigned to a commander
	assignFactoryCommandDroid(factory, nullptr);
	auto psFlag = FindFactoryDelivery(factory);
	if (psFlag)
	{
		startDeliveryPosition(psFlag);
	}
}

static inline bool compareFactories(STRUCTURE *a, STRUCTURE *b)
{
	if (a == nullptr || b == nullptr)
	{
		return (a == nullptr) < (b == nullptr);
	}
	auto x = getFactoryOrNullptr(a);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, x);
	auto y = getFactoryOrNullptr(b);
	ASSERT_NOT_NULLPTR_OR_RETURN(false, y);
	if (x->psAssemblyPoint->factoryType != y->psAssemblyPoint->factoryType)
	{
		return x->psAssemblyPoint->factoryType < y->psAssemblyPoint->factoryType;
	}

	return x->psAssemblyPoint->factoryInc < y->psAssemblyPoint->factoryInc;
}

void ManufactureController::updateFactoriesList()
{
	factories.clear();

	for (auto structure = interfaceStructList(); structure != nullptr; structure = structure->psNext)
	{
		if (structure->status == SS_BUILT && structure->died == 0 && StructIsFactory(structure))
		{
			factories.push_back(structure);
		}
	}

	std::sort(factories.begin(), factories.end(), compareFactories);
}

void ManufactureController::updateManufactureOptionsList()
{
	if (auto factory = getHighlightedObject())
	{
		stats = fillTemplateList(factory);
	}
	else
	{
		stats.clear();
	}
}

DROID_TEMPLATE *ManufactureController::getObjectStatsAt(size_t objectIndex) const
{
	auto factory = getFactoryOrNullptr(getObjectAt(objectIndex));
	ASSERT_NOT_NULLPTR_OR_RETURN(nullptr, factory);

	return factory->psSubject;
}

void ManufactureController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
}

void ManufactureController::setHighlightedObject(BASE_OBJECT *object)
{
	if (object == nullptr)
	{
		highlightedFactory = nullptr;
		return;
	}

	auto factory = castStructure(object);
	ASSERT_OR_RETURN(, StructIsFactory(factory), "Invalid factory pointer");
	highlightedFactory = factory;
}

class ManufactureObjectButton : public ObjectButton
{
private:
	typedef ObjectButton BaseWidget;

public:
	ManufactureObjectButton(const std::shared_ptr<ManufactureController> &controller, size_t newObjectIndex)
		: BaseWidget()
		, controller(controller)
	{
		objectIndex = newObjectIndex;
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
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);
		ASSERT_OR_RETURN(, !isDead(factory), "Factory is dead");
		displayIMD(Image(), ImdObject::Structure(factory), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	std::string getTip() override
	{
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN("", factory);
		return getStatsName(factory->pStructureType);
	}

	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<ManufactureController> controller;
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
	static std::shared_ptr<ManufactureStatsButton> make(const std::shared_ptr<ManufactureController> &controller, size_t objectIndex)
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

		auto factory = controller->getObjectAt(objectIndex);
		auto production = getStats();
		auto productionPending = StructureIsManufacturingPending(factory);
		auto objectImage = productionPending && production ? ImdObject::DroidTemplate(production): ImdObject::Component(nullptr);

		displayIMD(Image(), objectImage, xOffset, yOffset);

		if (productionPending && StructureIsOnHoldPending(factory))
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

	bool isHighlighted() const override
	{
		auto factory = controller->getObjectAt(objectIndex);
		return factory && (factory->selected || factory == controller->getHighlightedObject());
	}

	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override
	{
		BaseWidget::released(context, mouseButton);
		auto factory = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, factory);

		if (mouseButton == WKEY_PRIMARY)
		{
			controller->releaseFactoryProduction(factory);
			controller->clearStructureSelection();
			controller->selectObject(factory);
			controller->displayStatsForm();
		}
		else if (mouseButton == WKEY_SECONDARY)
		{
			controller->cancelFactoryProduction(factory);
		}
	}

	std::shared_ptr<ManufactureController> controller;
	ProductionRunSizeLabel productionRunSizeLabel;
	size_t objectIndex;
};

class ManufactureOptionButton: public StatsFormButton
{
private:
	typedef StatsFormButton BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureOptionButton> make(const std::shared_ptr<ManufactureController> &controller, size_t manufactureOptionIndex)
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
		ASSERT_NOT_NULLPTR_OR_RETURN(, stat);

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

	bool isHighlighted() const override
	{
		return controller->isHighlightedObjectStats(manufactureOptionIndex);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		costBar->majorSize = std::min(100, (int32_t)(getCost() / POWERPOINTS_DROIDDIV));
		productionRunSizeLabel.update(controller->getHighlightedObject(), getStats());
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
		ASSERT_NOT_NULLPTR_OR_RETURN(, clickedStats);

		auto controllerRef = controller;
		widgScheduleTask([mouseButton, clickedStats, controllerRef]() {
			controllerRef->adjustFactoryProduction(clickedStats, mouseButton == WKEY_PRIMARY);
		});
	}

	std::shared_ptr<ManufactureController> controller;
	ProductionRunSizeLabel productionRunSizeLabel;
	size_t manufactureOptionIndex;
};

class ManufactureObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureObjectsForm> make(const std::shared_ptr<ManufactureController> &controller)
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
	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<ManufactureController> controller;
};

class ManufactureStatsForm: public ObjectStatsForm
{
private:
	typedef ObjectStatsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<ManufactureStatsForm> make(const std::shared_ptr<ManufactureController> &controller)
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
	ManufactureController &getController() const override
	{
		return *controller.get();
	}

private:
	void initialize() override
	{
		BaseWidget::initialize();
		addObsoleteButton();
		attach(std::make_shared<DeliveryPointButton>(controller));
		addLoopProductionWidgets();
	}

	void addObsoleteButton()
	{
		auto obsoleteButton = std::make_shared<MultipleChoiceButton>();
		attach(obsoleteButton);
		obsoleteButton->style |= WBUT_SECONDARY;
		obsoleteButton->setChoice(controller->shouldShowRedundantDesign());
		obsoleteButton->setImages(false, MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
		obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
		obsoleteButton->setImages(true,  MultipleChoiceButton::Images(Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
		obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
		obsoleteButton->move(4 + Image(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);

		auto weakController = std::weak_ptr<ManufactureController>(controller);
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

	void addLoopProductionWidgets()
	{
		auto loopProductionButton = std::make_shared<LoopProductionButton>(controller);
		attach(loopProductionButton);
		loopProductionButton->move(STAT_SLDX + STAT_SLDWIDTH + 2, STAT_SLDY);

		auto loopProductionLabel = std::make_shared<LoopProductionLabel>(controller);
		attach(loopProductionLabel);
		loopProductionLabel->setGeometry(loopProductionButton->x() - 15, loopProductionButton->y(), 12, 15);
	}

	std::shared_ptr<ManufactureController> controller;

	class DeliveryPointButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		DeliveryPointButton(const std::shared_ptr<ManufactureController> &controller): BaseWidget(), controller(controller)
		{
			style |= WBUT_SECONDARY;
			move(4, STAT_SLDY);
			setTip(_("Factory Delivery Point"));

			auto weakController = std::weak_ptr<ManufactureController>(controller);
			addOnClickHandler([weakController](W_BUTTON &) {
				auto controller = weakController.lock();
				ASSERT_NOT_NULLPTR_OR_RETURN(, controller);
				controller->startDeliveryPointPosition();
			});
		}

	protected:
		void display(int xOffset, int yOffset)
		{
			updateLayout();
			BaseWidget::display(xOffset, yOffset);
		}

	private:
		void updateLayout()
		{
			auto factory = controller->getHighlightedObject();
			ASSERT_NOT_NULLPTR_OR_RETURN(, factory);

			switch (factory->pStructureType->type)
			{
			default:
			case REF_FACTORY:
				setImages(Image(IntImages, IMAGE_FDP_UP), Image(IntImages, IMAGE_FDP_DOWN), Image(IntImages, IMAGE_FDP_HI));
				break;
			case REF_CYBORG_FACTORY:
				setImages(Image(IntImages, IMAGE_CDP_UP), Image(IntImages, IMAGE_CDP_DOWN), Image(IntImages, IMAGE_CDP_HI));
				break;
			case REF_VTOL_FACTORY:
				setImages(Image(IntImages, IMAGE_VDP_UP), Image(IntImages, IMAGE_VDP_DOWN), Image(IntImages, IMAGE_VDP_HI));
				break;
			}
		}

		std::shared_ptr<ManufactureController> controller;
	};

	class LoopProductionButton: public W_BUTTON
	{
	private:
		typedef W_BUTTON BaseWidget;

	public:
		LoopProductionButton(const std::shared_ptr<ManufactureController> &controller): BaseWidget(), controller(controller)
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
		void run(W_CONTEXT *) override
		{
			setState(ManufactureStatsForm::getProductionLoops(controller->getHighlightedObject()) == 0 ? 0: WBUT_CLICKLOCK);
		}

		std::shared_ptr<ManufactureController> controller;
	};

	class LoopProductionLabel: public W_LABEL
	{
	private:
		typedef W_LABEL BaseWidget;

	public:
		LoopProductionLabel(const std::shared_ptr<ManufactureController> &controller): BaseWidget(), controller(controller)
		{
		}

	private:
		void run(W_CONTEXT *) override
		{
			auto productionLoops = ManufactureStatsForm::getProductionLoops(controller->getHighlightedObject());
			if (productionLoops != lastProductionLoop)
			{
				lastProductionLoop = productionLoops;
				setString(WzString::fromUtf8(getNewString()));
			}
		}

		std::string getNewString()
		{
			switch (lastProductionLoop)
			{
			case 0:
				return "";
			case INFINITE_PRODUCTION:
				return "∞";
			default:
				return astringf("%u", lastProductionLoop + 1);
			}
		}

		uint8_t lastProductionLoop = 0;
		std::shared_ptr<ManufactureController> controller;
	};

	static uint8_t getProductionLoops(STRUCTURE *structure)
	{
		if (structure == nullptr)
		{
			return 0;
		}

		auto psFactory = getFactoryOrNullptr(structure);
		ASSERT_NOT_NULLPTR_OR_RETURN(0, psFactory);
		return psFactory->psSubject == nullptr ? 0 : psFactory->productionLoops;
	}
};

bool ManufactureController::showInterface()
{
	updateData();
	if (factories.empty())
	{
		return false;
	}

	auto objectsForm = ManufactureObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);
	displayStatsForm();
	triggerEvent(TRIGGER_MENU_MANUFACTURE_UP);
	return true;
}

std::shared_ptr<StatsForm> ManufactureController::makeStatsForm()
{
	return ManufactureStatsForm::make(shared_from_this());
}
