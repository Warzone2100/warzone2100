#ifndef __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
#define __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__

#include <vector>
#include "../hci.h"
#include "../intdisplay.h"
#include "../intorder.h"

#define STAT_GAP	   2
#define STAT_BUTWIDTH  60
#define STAT_BUTHEIGHT 46

class StatsForm;

class BaseObjectsController
{
public:
	virtual ~BaseObjectsController() = default;
	virtual size_t objectsSize() const = 0;
	virtual BASE_OBJECT *getObjectAt(size_t index) const = 0;
	virtual BASE_STATS *getObjectStatsAt(size_t index) const = 0;
	virtual bool findObject(std::function<bool (BASE_OBJECT *)> iteration) const = 0;
	virtual void refresh() = 0;
	virtual bool showInterface() = 0;
	void jumpToHighlighted();
	void updateHighlighted();
	void selectObject(BASE_OBJECT *object);

	virtual BASE_OBJECT *getHighlightedObject() const = 0;
	virtual void setHighlightedObject(BASE_OBJECT *object) = 0;

	void closeInterface()
	{
		intResetScreen(false);
	}

	void closeInterfaceNoAnim()
	{
		intResetScreen(true);
	}

protected:
	template<typename A, typename B>
	static bool findObject(std::vector<A> vector, std::function<bool (B)> iteration)
	{
		for (auto item: vector)
		{
			if (iteration(item))
			{
				return true;
			}
		}

		return false;
	}
};

class BaseStatsController: public BaseObjectsController
{
public:
	virtual ~BaseStatsController() = default;
	virtual size_t statsSize() const = 0;
	virtual std::shared_ptr<StatsForm> makeStatsForm() = 0;
	void displayStatsForm();
};

class DynamicIntFancyButton: public IntFancyButton
{
protected:
	virtual bool isSelected() const = 0;
	virtual void updateLayout();
	void updateSelection();
};

class StatsButton: public DynamicIntFancyButton
{
protected:
	virtual BASE_STATS *getStats() = 0;

	std::string getTip() override
	{
		auto stats = getStats();
		return stats == nullptr ? "": getStatsName(stats);
	}

	void addProgressBar();
	std::shared_ptr<W_BARGRAPH> progressBar;
};

class ObjectButton : public DynamicIntFancyButton
{
public:
	ObjectButton()
	{
		buttonType = BTMBUTTON;
	}

protected:
	bool isSelected() const override
	{
		return false;
	}

	virtual std::shared_ptr<BaseObjectsController> getController() const = 0;
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
	virtual std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<BaseObjectsController> getController() const = 0;

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

	virtual void initialize();
	virtual void updateLayout();
	void display(int xOffset, int yOffset) override;
	void addCloseButton();
	void addTabList();
	void addNewButton();
	void removeLastButton();
	virtual std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<BaseStatsController> getController() const = 0;

private:
	void updateSelectedObjectStats();
	void updateButtons();

	std::shared_ptr<IntListTabWidget> optionList;
	size_t buttonsCount = 0;
	BASE_STATS *selectedObjectStats = nullptr;
};

#endif // __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
