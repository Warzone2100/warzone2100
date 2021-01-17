#ifndef __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
#define __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__

#include <vector>
#include "../hci.h"
#include "../intdisplay.h"

class StatsForm;

class BaseObjectsStatsController
{
public:
	virtual ~BaseObjectsStatsController() = default;
	virtual size_t objectsSize() const = 0;
	virtual size_t statsSize() const = 0;
	virtual BASE_OBJECT *getObjectAt(size_t index) const = 0;
	virtual BASE_STATS *getObjectStatsAt(size_t index) const = 0;
	virtual BASE_STATS *getStatsAt(size_t index) const = 0;
	virtual void refresh() = 0;
	void selectObject(BASE_OBJECT *object);
	void jumpToSelected();

	BASE_OBJECT *getSelectedObject() const
	{
		return intGetSelectedObject();
	}

	void setSelectedObject(BASE_OBJECT *value)
	{
		intSetSelectedObject(value);
	}
};

class DynamicIntFancyButton: public IntFancyButton
{
protected:
	virtual bool isSelected() const = 0;
	void updateSelection();
};

class StatsButton: public DynamicIntFancyButton
{
protected:
	void addProgressBar();
	virtual void updateLayout();

	std::shared_ptr<W_BARGRAPH> progressBar;
};

class ObjectButton : public IntFancyButton
{
public:
	ObjectButton()
	{
		buttonType = BTMBUTTON;
	}

protected:
	virtual std::shared_ptr<BaseObjectsStatsController> getController() const = 0;
	void selectAndJump();

	size_t objectIndex;

private:
	Vector2i jumpPosition = {0, 0};
};

class StatsFormButton : public StatsButton
{
public:
	void setStatsForm(const std::shared_ptr<StatsForm> &value)
	{
		statsForm = value;
	}

	void addCostBar();

protected:
	std::string getTip() override
	{
		WzString costString = WzString::fromUtf8(_("\nCost: %1"));
		costString.replace("%1", WzString::number(getCost()));
		WzString tipString = getStatsName(getStats());
		tipString.append(costString);
		return tipString.toUtf8();
	}

	virtual BASE_STATS *getStats() = 0;
	virtual uint32_t getCost() = 0;

	std::weak_ptr<StatsForm> statsForm;
	std::shared_ptr<W_BARGRAPH> costBar;
};

class ObjectsForm: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;

protected:
	ObjectsForm(): BaseWidget(false) {}

	void display(int xOffset, int yOffset);
	void initialize();
	void addCloseButton();
	void addTabList();
	void updateButtons();
	void addNewButton();
	void removeLastButton();
	virtual std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<IntFancyButton> makeObjectButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<BaseObjectsStatsController> getController() const = 0;

	std::shared_ptr<IntListTabWidget> objectsList;
	size_t buttonsCount = 0;
};

class StatsForm: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;

public:
	BASE_STATS *getSelectedStats() const
	{
		return selectedObjectStats;
	}

protected:
	StatsForm(): IntFormAnimated(false) {}

	void display(int xOffset, int yOffset) override;
	virtual void initialize();
	void addCloseButton();
	void addTabList();
	void updateSelectedObjectStats();
	void updateButtons();
	void addNewButton();
	void removeLastButton();
	virtual std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<BaseObjectsStatsController> getController() const = 0;

	std::shared_ptr<IntListTabWidget> optionList;
	size_t buttonsCount = 0;
	BASE_STATS *selectedObjectStats = nullptr;
};

#endif // __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
