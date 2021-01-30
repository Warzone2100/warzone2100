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
class ObjectStatsForm;

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
	void clearStructureSelection();
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

class BaseStatsController
{
public:
	virtual ~BaseStatsController() = default;
	virtual size_t statsSize() const = 0;
	virtual std::shared_ptr<StatsForm> makeStatsForm() = 0;
	void displayStatsForm();
	virtual BASE_STATS *getStatsAt(size_t) const = 0;
};

class BaseObjectsStatsController: public BaseStatsController, public BaseObjectsController
{
public:
	void updateSelectedObjectStats();

	bool isSelectedObjectStats(size_t statsIndex)
	{
		return getStatsAt(statsIndex) == selectedObjectStats;
	}

private:
	BASE_STATS *selectedObjectStats;
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

	virtual BaseObjectsController &getController() const = 0;
	void jump();

	size_t objectIndex;

private:
	Vector2i jumpPosition = {0, 0};
};

class StatsFormButton : public StatsButton
{
public:
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
	virtual BaseObjectsController &getController() const = 0;

	std::shared_ptr<IntListTabWidget> objectsList;
	size_t buttonsCount = 0;
};

class StatsForm: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;

protected:
	StatsForm(): BaseWidget(false) {}

	virtual void initialize();
	virtual void updateLayout();
	void display(int xOffset, int yOffset) override;
	void addCloseButton();
	void addTabList();
	void addNewButton();
	void removeLastButton();
	virtual std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const = 0;
	virtual BaseStatsController &getController() const = 0;

private:
	void updateButtons();

	std::shared_ptr<IntListTabWidget> optionList;
	size_t buttonsCount = 0;
};

class ObjectStatsForm: public StatsForm
{
private:
	typedef StatsForm BaseWidget;

public:
	virtual BaseObjectsStatsController &getController() const override = 0;

protected:
	void updateLayout() override;
};

#endif // __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
