/*
	This file is part of Warzone 2100.
	Copyright (C) 2021-2023  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
#define __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__

#include "../hci.h"
#include "../intdisplay.h"

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
	virtual void prepareToClose();
	virtual void clearData() = 0;
	void jumpToObject(BASE_OBJECT *object);
	void updateHighlighted();
	void clearSelection();
	void clearStructureSelection();
	void selectObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject);

	virtual BASE_OBJECT *getHighlightedObject() const = 0;
	virtual void setHighlightedObject(BASE_OBJECT *object, bool jumpToHighlightedStatsObject) = 0;
	virtual bool getQueuedJumpToHighlightedStatsObject() const = 0;
	virtual void clearQueuedJumpToHighlightedStatsObject() = 0;

	void closeInterface()
	{
		widgScheduleTask([]() {
			intResetScreen(false);
		});
	}

	void closeInterfaceNoAnim()
	{
		widgScheduleTask([]() {
			intResetScreen(true);
		});
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
	static void scheduleDisplayStatsForm(const std::shared_ptr<BaseStatsController>& controller);
	virtual BASE_STATS *getStatsAt(size_t) const = 0;
};

class BaseObjectsStatsController: public BaseStatsController, public BaseObjectsController
{
public:
	void updateHighlightedObjectStats();

	bool isHighlightedObjectStats(size_t statsIndex)
	{
		return getStatsAt(statsIndex) == highlightedObjectStats;
	}

	BASE_STATS *getHighlightedObjectStats()
	{
		return highlightedObjectStats;
	}

private:
	BASE_STATS *highlightedObjectStats;
};

class DynamicIntFancyButton: public IntFancyButton
{
protected:
	virtual bool isHighlighted() const = 0;
	virtual void updateLayout();
	void updateHighlight();
	virtual void clickPrimary() {}
	virtual void clickSecondary(bool synthesizedFromHold) {}
	void released(W_CONTEXT *context, WIDGET_KEY mouseButton = WKEY_PRIMARY) override;
	bool clickHeld(W_CONTEXT *psContext, WIDGET_KEY key) override;
public:
	DynamicIntFancyButton();
};

class StatsButton: public DynamicIntFancyButton
{
protected:
	virtual BASE_STATS *getStats() = 0;

	std::string getTip() override
	{
		auto stats = getStats();
		return stats == nullptr ? "": getLocalizedStatsName(stats);
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
	bool isHighlighted() const override
	{
		return false;
	}

	virtual BaseObjectsController &getController() const = 0;
	virtual void jump();

	void clickSecondary(bool synthesizedFromHold) override
	{
		jump();
	}

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
		WzString costString = WzString::fromUtf8(astringf(_("Cost: %u"), getCost()));
		auto stats = getStats();
		WzString tipString = (stats == nullptr) ? "" : getLocalizedStatsName(stats);
		tipString.append("\n");
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
	void goToHighlightedTab();
	virtual std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const = 0;
	virtual BaseObjectsController &getController() const = 0;

	std::shared_ptr<IntListTabWidget> objectsList;
	size_t buttonsCount = 0;
	BASE_OBJECT *previousHighlighted = nullptr;
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

	std::shared_ptr<IntListTabWidget> optionList;

private:
	void updateButtons();

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
	void goToHighlightedTab();
	BASE_STATS *previousHighlighted = nullptr;
};

#endif // __INCLUDED_SRC_HCI_OBJECTS_STATS_INTERFACE_H__
