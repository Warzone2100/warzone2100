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
#include "../hud/huddetail.h"

#include <limits>

#define STAT_GAP	   2
#define STAT_BUTWIDTH  60
#define STAT_BUTHEIGHT 46

class StatsForm;
class ObjectStatsForm;
class HudPageControl;

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

class ObjectButton : public DynamicIntFancyButton, public HudDetailSource
{
public:
	ObjectButton()
	{
		buttonType = BTMBUTTON;
	}

	std::shared_ptr<WIDGET> makeHudDetail() override;
	uint32_t hudDetailPanelId() const override { return IDOBJ_FORM; }

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

class StatsFormButton : public StatsButton, public HudDetailSource
{
public:
	void addCostBar();

	HudDetailPlacement hudDetailPlacement() const override
	{
		return HudDetailPlacement::Right;
	}
	uint32_t hudDetailPanelId() const override { return IDSTAT_FORM; }

protected:
	std::string getTip() override
	{
		WzString costString = WzString::format(_("Cost: %u"), getCost());
		auto stats = getStats();
		WzString tipString = (stats == nullptr) ? "" : getLocalizedStatsName(stats);
		tipString.append("\n");
		tipString.append(costString);
		return tipString.toUtf8();
	}

	virtual uint32_t getCost() = 0;
	/// The cost bar's color: whether it can be afforded now
	void tintCostBar();

	std::shared_ptr<W_BARGRAPH> costBar;
};

class ObjectsForm: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;

public:
	/// Lists only the objects the filter keeps, in their Classic order (none: all of them, exactly as in Classic)
	void setObjectFilter(const std::function<bool (BASE_OBJECT *)> &filter);
	BaseObjectsController &objectsController() const { return getController(); }
	/// The object whose button (or job button) the pointer is on, if any
	BASE_OBJECT *objectUnderPointer() const;
	/// The row summary strip and its filter chips, when that module is on
	void addHudExtension();
	const std::shared_ptr<IntListTabWidget> &objectsListWidget() const { return objectsList; }

protected:
	ObjectsForm(): BaseWidget(false) {}

	void display(int xOffset, int yOffset);
	void initialize();
	void addCloseButton();
	void addTabList();
	void updateButtons();
	void addNewButton();
	void addNewButton(size_t buttonIndex);
	void removeLastButton();
	void goToHighlightedTab();
	virtual std::shared_ptr<StatsButton> makeStatsButton(size_t buttonIndex) const = 0;
	virtual std::shared_ptr<ObjectButton> makeObjectButton(size_t buttonIndex) const = 0;
	virtual BaseObjectsController &getController() const = 0;

	std::shared_ptr<IntListTabWidget> objectsList;
	std::shared_ptr<W_BUTTON> closeButton;
	size_t buttonsCount = 0;
	BASE_OBJECT *previousHighlighted = nullptr;

private:
	void updateFilteredButtons();
	void removeAllButtons();
	/// (A filter hid the highlighted object: the first shown one takes its place, as if clicked)
	void highlightFirstShown();
	optional<size_t> listPositionOf(size_t objectIndex) const;

	std::function<bool (BASE_OBJECT *)> objectFilter;
	std::vector<size_t> shownIndices;
	bool filteredLayout = false;
};

class StatsForm: public IntFormAnimated
{
private:
	typedef IntFormAnimated BaseWidget;

public:
	/// Lists only the options the filter keeps, in their Classic order (none: all of them, exactly as in Classic)
	void setOptionFilter(const std::function<bool (BASE_STATS *)> &filter);
	BaseStatsController &statsController() const { return getController(); }
	/// The filters and search field, when that module is on
	void addHudExtension();

protected:
	StatsForm(): BaseWidget(false) {}

	virtual void initialize();
	virtual void updateLayout();
	void display(int xOffset, int yOffset) override;
	void addCloseButton();
	void addTabList();
	void addNewButton();
	void addNewButton(size_t buttonIndex);
	void removeLastButton();
	virtual std::shared_ptr<StatsFormButton> makeOptionButton(size_t buttonIndex) const = 0;
	virtual BaseStatsController &getController() const = 0;
	/// With the page control in place of the page tabs (panel filters): puts the header's controls side by side, `drop`
	/// below their Classic row, and returns their bounds
	virtual WzRect layoutCompactHeader(int drop) { return WzRect(0, STAT_SLDY + drop, 0, 13); }
	/// With more pages than this, the page control takes the page tabs' row, and the header keeps its own
	virtual size_t compactHeaderMaxPages() const { return std::numeric_limits<size_t>::max(); }

	std::shared_ptr<IntListTabWidget> optionList;
	std::shared_ptr<W_BUTTON> closeButton;
	optional<size_t> listPositionOf(size_t statsIndex) const;

private:
	void addPageControl();
	bool compactHeaderInRow() const;
	void layoutHudHeader();
	void updateButtons();
	void updateFilteredButtons();
	void removeAllButtons();

	size_t buttonsCount = 0;
	std::shared_ptr<HudPageControl> pageControl;
	std::function<bool (BASE_STATS *)> optionFilter;
	std::vector<size_t> shownIndices;
	bool filteredLayout = false;
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
