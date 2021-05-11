#include "lib/widget/label.h"
#include "commander.h"
#include "../objmem.h"
#include "../cmddroid.h"
#include "../group.h"
#include "../intorder.h"

DROID *CommanderController::highlightedCommander = nullptr;

void CommanderController::updateData()
{
	updateCommandersList();
	updateHighlighted();
}

void CommanderController::updateCommandersList()
{
	commanders.clear();

	for (DROID *droid = apsDroidLists[selectedPlayer]; droid; droid = droid->psNext)
	{
		if (droid->droidType == DROID_COMMAND && droid->died == 0)
		{
			commanders.push_back(droid);
		}
	}

	std::reverse(commanders.begin(), commanders.end());
}

STRUCTURE_STATS *CommanderController::getObjectStatsAt(size_t objectIndex) const
{
	auto assignedFactory = getAssignedFactoryAt(objectIndex);
	return assignedFactory == nullptr ? nullptr : assignedFactory->pStructureType;
}

STRUCTURE *CommanderController::getAssignedFactoryAt(size_t objectIndex) const
{
	auto droid = getObjectAt(objectIndex);
	return droid == nullptr ? nullptr : droidGetCommandFactory(droid);
}

void CommanderController::refresh()
{
	updateData();

	if (objectsSize() == 0)
	{
		closeInterface();
	}
	// If order form is visible, refresh it.
	else if (widgGetFromID(psWScreen, IDORDER_FORM) != nullptr)
	{
		displayOrderForm();
	}
}

void CommanderController::setHighlightedObject(BASE_OBJECT *object)
{
	if (object == nullptr)
	{
		highlightedCommander = nullptr;
		return;
	}

	auto commander = castDroid(object);
	ASSERT_NOT_NULLPTR_OR_RETURN(, commander);
	ASSERT_OR_RETURN(, commander->droidType == DROID_COMMAND, "Droid is not a commander");
	highlightedCommander = commander;
}

class CommanderObjectButton : public ObjectButton
{
	typedef	ObjectButton BaseWidget;

protected:
	CommanderObjectButton() {}

public:
	static std::shared_ptr<CommanderObjectButton> make(const std::shared_ptr<CommanderController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public CommanderObjectButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->objectIndex = objectIndex;
		widget->initialize();
		return widget;
	}

	void clickPrimary() override
	{
		controller->clearSelection();
		controller->selectObject(controller->getObjectAt(objectIndex));
		jump();
		controller->displayOrderForm();
	}

private:
	void initialize()
	{
		attach(groupSizeLabel = std::make_shared<W_LABEL>());
		groupSizeLabel->setGeometry(OBJ_TEXTX, OBJ_B1TEXTY, 16, 16);

		attach(experienceStarsLabel = std::make_shared<W_LABEL>());
		experienceStarsLabel->setGeometry(STAT_POWERBARX, STAT_POWERBARY, 16, 16);
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
		displayIMD(Image(), ImdObject::Droid(droid), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		updateGroupSizeLabel(droid);
		updateExperienceStarsLabel(droid);
	}

	void updateGroupSizeLabel(DROID *droid)
	{
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		auto text = astringf("%u/%u", droid->psGroup ? droid->psGroup->getNumMembers() : 0, cmdDroidMaxGroup(droid));
		groupSizeLabel->setString(WzString::fromUtf8(text));
		groupSizeLabel->show();
	}

	void updateExperienceStarsLabel(DROID *droid)
	{
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		int numStars = std::max((int)getDroidLevel(droid) - 1, 0);
		experienceStarsLabel->setString(WzString(numStars, WzUniCodepoint::fromASCII('*')));
		experienceStarsLabel->show();
	}

	CommanderController &getController() const override
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
	std::shared_ptr<W_LABEL> groupSizeLabel;
	std::shared_ptr<W_LABEL> experienceStarsLabel;
	std::shared_ptr<CommanderController> controller;
};

class CommanderStatsButton: public StatsButton
{
private:
	typedef	StatsButton BaseWidget;

protected:
	CommanderStatsButton() {}

public:
	static std::shared_ptr<CommanderStatsButton> make(const std::shared_ptr<CommanderController> &controller, size_t objectIndex)
	{
		class make_shared_enabler: public CommanderStatsButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->objectIndex = objectIndex;
		widget->initialize();
		return widget;
	}

private:
	void initialize()
	{
		attach(assignedFactoriesLabel = std::make_shared<W_LABEL>());
		assignedFactoriesLabel->setGeometry(OBJ_TEXTX, OBJ_T1TEXTY, 16, 16);

		attach(assignedCyborgFactoriesLabel = std::make_shared<W_LABEL>());
		assignedCyborgFactoriesLabel->setGeometry(OBJ_TEXTX, OBJ_T2TEXTY, 16, 16);

		attach(assignedVtolFactoriesLabel = std::make_shared<W_LABEL>());
		assignedVtolFactoriesLabel->setGeometry(OBJ_TEXTX, OBJ_T3TEXTY, 16, 16);
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		updateLayout();
		auto factory = controller->getAssignedFactoryAt(objectIndex);
		displayIMD(Image(), factory ? ImdObject::Structure(factory): ImdObject::Component(nullptr), xOffset, yOffset);
		displayIfHighlight(xOffset, yOffset);
	}

	void updateLayout() override
	{
		BaseWidget::updateLayout();
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		updateAssignedFactoriesLabel(assignedFactoriesLabel, droid, DSS_ASSPROD_SHIFT);
		updateAssignedFactoriesLabel(assignedCyborgFactoriesLabel, droid, DSS_ASSPROD_CYBORG_SHIFT);
		updateAssignedFactoriesLabel(assignedVtolFactoriesLabel, droid, DSS_ASSPROD_VTOL_SHIFT);
	}

private:
	void updateAssignedFactoriesLabel(const std::shared_ptr<W_LABEL> &label, DROID *droid, uint32_t factoryTypeShift)
	{
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		/**
		 * TODO Support up to MAX_FACTORY (which won't fit in the ugly secondaryOrder bitmask hack).
		 * Comment taken from commit 34d8148e4a
		 **/
		static const auto maxAssignedFactories = 5;
		char text[maxAssignedFactories + 1] = {0};

		auto index = 0;
		for (auto i = 0; i < maxAssignedFactories; ++i)
		{
			if (droid->secondaryOrder & (1 << (i + factoryTypeShift)))
			{
				text[index++] = '1' + i;
			}
		}

		if (index > 0)
		{
			label->setString(text);
			label->show();
		}
		else
		{
			label->hide();
		}
	}

	STRUCTURE_STATS *getStats() override
	{
		return controller->getObjectStatsAt(objectIndex);
	}

	bool isHighlighted() const override
	{
		auto droid = controller->getObjectAt(objectIndex);
		return droid && droid == controller->getHighlightedObject();
	}

	void clickPrimary() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		controller->clearSelection();
		controller->selectObject(droid);
		controller->displayOrderForm();
	}

	void clickSecondary() override
	{
		auto droid = controller->getObjectAt(objectIndex);
		ASSERT_NOT_NULLPTR_OR_RETURN(, droid);
		auto highlighted = controller->getHighlightedObject();

		// prevent highlighting a commander when another commander is already selected
		if (droid == highlighted || (highlighted && !highlighted->selected))
		{
			controller->setHighlightedObject(droid);
			controller->displayOrderForm();
		}
	}

	std::shared_ptr<W_LABEL> assignedFactoriesLabel;
	std::shared_ptr<W_LABEL> assignedCyborgFactoriesLabel;
	std::shared_ptr<W_LABEL> assignedVtolFactoriesLabel;
	std::shared_ptr<CommanderController> controller;
	size_t objectIndex;
};

class CommanderObjectsForm: public ObjectsForm
{
private:
	typedef ObjectsForm BaseWidget;
	using BaseWidget::BaseWidget;

public:
	static std::shared_ptr<CommanderObjectsForm> make(const std::shared_ptr<CommanderController> &controller)
	{
		class make_shared_enabler: public CommanderObjectsForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->controller = controller;
		widget->initialize();
		return widget;
	}

	std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const override
	{
		return CommanderStatsButton::make(controller, buttonIndex);
	}

	std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const override
	{
		return CommanderObjectButton::make(controller, buttonIndex);
	}

protected:
	BaseObjectsController &getController() const override
	{
		return *controller.get();
	}

private:
	std::shared_ptr<CommanderController> controller;
};

bool CommanderController::showInterface()
{
	updateData();
	if (commanders.empty())
	{
		return false;
	}

	auto objectsForm = CommanderObjectsForm::make(shared_from_this());
	psWScreen->psForm->attach(objectsForm);
	displayOrderForm();
	return true;
}

void CommanderController::displayOrderForm()
{
	auto psWeakControllerRef = std::weak_ptr<CommanderController>(shared_from_this());
	widgScheduleTask([psWeakControllerRef]() {
		DROID *psDroid = nullptr;
		if (auto strongControllerRef = psWeakControllerRef.lock())
		{
			psDroid = strongControllerRef->getHighlightedObject();
		}
		intAddOrder(psDroid);
		intMode = INT_CMDORDER;
	});
}
