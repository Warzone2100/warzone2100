// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
/** \file
 *  Widgets for displaying a list of changeable options
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/button.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/form.h"
#include "lib/widget/table.h"
#include "lib/widget/popover.h"
#include <vector>
#include <memory>


struct OptionChoiceHelpDescription
{
	WzString displayName;
	WzString helpDescription;
};

// OptionValueChangerInterface
// - OptionsForm::addOption takes an optionValueChangerWidget, which is expected to be a custom WIDGET subclass that
//   also implements OptionValueChangerInterface to handle certain events and expose certain functionality to OptionsForm
class OptionValueChangerInterface
{
public:
	virtual ~OptionValueChangerInterface();
	virtual void update(bool force = false) = 0;
	virtual void informAvailable(bool isAvailable) = 0;
	virtual void addOnChangeHandler(std::function<void(WIDGET&)> handler) = 0;
	virtual void addCloseFormManagedPopoversHandler(std::function<void(WIDGET&)> handler) = 0;
	virtual optional<OptionChoiceHelpDescription> getHelpDescriptionForCurrentValue() { return nullopt; }
	virtual optional<std::vector<OptionChoiceHelpDescription>> getHelpDescriptions() { return nullopt; }
};

class WzOptionsChoiceWidget : public WIDGET
{
protected:
	WzOptionsChoiceWidget(iV_fonts desiredFontID);

public:
	static std::shared_ptr<WzOptionsChoiceWidget> make(iV_fonts desiredFontID);

	void setString(WzString string) override;
	void setDisabled(bool value);
	bool getIsDisabled() const;
	void setDisplayHighlighted(bool highlighted);
	void setPadding(int horizontal, int vertical);
	void setTextAlignment(WzTextAlignment align);
	void setHighlightBackgroundColor(PIELIGHT color);

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	std::string getTip() override;
	WzString getString() const override;

protected:
	void run(W_CONTEXT *) override;
	void display(int xOffset, int yOffset) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
private:
	void cacheIdealTextSize();
private:
	WzString text;
	WzCachedText wzText;
	iV_fonts FontID;
	PIELIGHT highlightBackgroundColor;
	int32_t cachedIdealTextWidth = 0;
	int32_t cachedIdealTextLineSize = 0;
	int lastWidgetWidth = 0;
	int horizontalPadding = 10;
	int verticalPadding = 5;
	bool isHighlighted = false;
	bool forceDisplayHighlight = false;
	bool isDisabled = false;
	bool isTruncated = false;
};

// WzClickableOptionsChoiceWidget- A WzOptionsChoiceWidget that is directly clickable (and has an onclick handler)
class WzClickableOptionsChoiceWidget : public WzOptionsChoiceWidget
{
protected:
	WzClickableOptionsChoiceWidget(iV_fonts desiredFontID);
public:
	typedef std::function<void (WzClickableOptionsChoiceWidget& button, WIDGET_KEY key)> OnClickHandler;
	static std::shared_ptr<WzClickableOptionsChoiceWidget> make(iV_fonts desiredFontID);
public:
	void addOnClickHandler(const OnClickHandler& onClickFunc);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
private:
	bool mouseDownOnWidget = false;
	std::vector<OnClickHandler> onClickHandlers;
};

class WzOptionsDropdownWidget : public DropdownWidget
{
public:
	WzOptionsDropdownWidget();
	void setTextAlignment(WzTextAlignment align);

	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
protected:
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	virtual void drawOpenedHighlight(int xOffset, int yOffset) override;
	virtual void drawSelectedItem(const std::shared_ptr<WIDGET>& item, const WzRect& screenDisplayArea) override;
	virtual void onSelectedItemChanged() override;
	virtual int calculateDropdownListScreenPosX() const override;
	virtual int calculateDropdownListDisplayWidth() const override;
private:
	void updateCurrentChoiceDisplayWidgetGeometry();
	int calculateCurrentUsedWidth() const;
private:
	std::shared_ptr<WzOptionsChoiceWidget> currentOptionChoiceDisplayWidg;
	bool isHighlighted = false;
};

// A unique option choice
template<typename T>
class OptionChoice
{
public:
	WzString displayName;
	WzString helpDescription;
	T value;
	bool disabled = false;

	inline bool operator== (const OptionChoice& b) const
	{
		return (value == b.value) && (displayName == b.displayName) && (helpDescription == b.helpDescription) && (disabled == b.disabled);
	}
};

template<typename T>
struct OptionChoices
{
	std::vector<OptionChoice<T>> choices;
	size_t currentIdx = 0;
};

// OptionsDropdown<T>
// - A custom DropdownWidget subclass with a simplified interface that handles OptionValueChangerInterface
//
// Example Usage:
// ```cpp
//	auto valueChanger = OptionsDropdown<bool>::make(
//		// populate
//		[]() {
//			OptionChoices<bool> result;
//			result.choices = {
//				{ _("Enabled"), _("A description of the impact of being Enabled (the default)."), true },
//				{ _("Disabled"), _("A description of the impact of being Disabled."), false }
//			};
//			bool currentValue = true; // Obtain the current option value from somewhere
//			result.currentIdx = (currentValue) ? 0 : 1; // Set the currentIdx based on the currentValue
//			return result;
//		},
//		// onSelect
//		[](const auto& newValue) -> bool {
//			// TO IMPLEMENT: Set / store / persist the new option value - return true/false on success/failure
//		},
//		false // needsStateUpdates
//	);
// ```
// NOTE: The important use of _("") to tag strings that need to be translated when constructing the OptionChoices in the populate function
template<typename T>
class OptionsDropdown : public WzOptionsDropdownWidget, public OptionValueChangerInterface
{
protected:
	OptionsDropdown()
	{ }
public:
	typedef std::function<OptionChoices<T> (void)> PopulateFunc;
	typedef std::function<bool (const T& newValue)> OnSelectItemFunc; // note: return true if able to select item, false if a failure
public:
	static std::shared_ptr<OptionsDropdown<T>> make(PopulateFunc populate, OnSelectItemFunc onSelect, bool needsStateUpdates, int maxListItemsShown = 5)
	{
		ASSERT_OR_RETURN(nullptr, populate != nullptr, "Invalid populate func");

		class make_shared_enabler : public OptionsDropdown<T> { };
		auto result = std::make_shared<make_shared_enabler>();
		result->populateFunc = populate;
		result->onSelectFunc = onSelect;
		result->maxListItemsShown = maxListItemsShown;
		result->needsStateUpdates = needsStateUpdates;
		result->setTextAlignment(WLAB_ALIGNRIGHT);

		result->current = populate();
		result->populateWithCurrentItems();

		result->setCanChange([](DropdownWidget &widget, size_t newIndex, std::shared_ptr<WIDGET> newSelectedWidget) -> bool {
			auto strongSelf = std::static_pointer_cast<OptionsDropdown<T>>(widget.shared_from_this());
			ASSERT_OR_RETURN(false, newIndex < strongSelf->current.choices.size(), "Invalid index");
			if (strongSelf->current.choices[newIndex].disabled) { return false; }
			auto result = strongSelf->onSelectFunc(strongSelf->current.choices[newIndex].value);
			if (result)
			{
				strongSelf->current.currentIdx = newIndex;
			}
			return result;
		});

		return result;
	}

	void update(bool force) override
	{
		if (!force && !needsStateUpdates)
		{
			return;
		}

		auto newItemsList = populateFunc();
		if (newItemsList.choices != current.choices)
		{
			current = newItemsList;
			populateWithCurrentItems();
		}
		else if (newItemsList.currentIdx != current.currentIdx)
		{
			current.currentIdx = newItemsList.currentIdx;
			setSelectedIndex(current.currentIdx);
		}
	}

	void informAvailable(bool isAvailable) override
	{
		setDisabled(!isAvailable);
	}

	void addOnChangeHandler(std::function<void(WIDGET&)> handler) override
	{
		// in the future, call *DropdownWiget::addOnChange*
		setOnChange(handler);
	}

	void addCloseFormManagedPopoversHandler(std::function<void(WIDGET&)> handler) override
	{
		// Since dropdowns create a popover, add this to the onOpen handler
		setOnOpen(handler);
	}

	optional<OptionChoiceHelpDescription> getHelpDescriptionForCurrentValue() override
	{
		auto currIndex = getSelectedIndex();
		if (!currIndex.has_value())
		{
			return nullopt;
		}
		ASSERT_OR_RETURN(nullopt, currIndex.value() < current.choices.size(), "Invalid current index");
		const auto& option = current.choices[currIndex.value()];
		return OptionChoiceHelpDescription{option.displayName, option.helpDescription};
	}

	optional<std::vector<OptionChoiceHelpDescription>> getHelpDescriptions() override
	{
		std::vector<OptionChoiceHelpDescription> result;
		for (const auto& option : current.choices)
		{
			if (option.helpDescription.isEmpty())
			{
				continue;
			}
			result.push_back({option.displayName, option.helpDescription});
		}
		return result;
	}

private:
	void populateWithCurrentItems()
	{
		clear();

		int maxItemHeight = 0;
		for (const auto& option : current.choices)
		{
			auto item = WzOptionsChoiceWidget::make(font_regular);
			item->setString(option.displayName);
			item->setDisabled(option.disabled);
			item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
			maxItemHeight = std::max(maxItemHeight, item->idealHeight());
			addItem(item);
		}

		const Padding& listPadding = getDropdownMenuOuterPadding();
		setListHeight(maxItemHeight * std::min<uint32_t>(maxListItemsShown, current.choices.size()) + listPadding.top + listPadding.bottom);

		setSelectedIndex(current.currentIdx);
	}

private:
	OptionChoices<T> current;
	PopulateFunc populateFunc;
	OnSelectItemFunc onSelectFunc;
	int maxListItemsShown;
	bool needsStateUpdates;
};

// OptionInfo - An option title and other information
//
// Example Usage:
// ```cpp
// OptionInfo("general.something", N_("Campaign Difficulty"), N_("Help description"));
// ```
// NOTE: The important use of N_("") to tag strings that need to be translated (but we always want to provide the untranslated original string to OptionInfo, which handles getting the translation as needed)
class OptionInfo
{
public:
	struct AvailabilityResult
	{
		bool available;
		WzString localizedUnavailabilityReason;
	};
	typedef std::function<AvailabilityResult (const OptionInfo&)> OptionAvailabilityCondition;
public:
	OptionInfo(const WzString& optionId, const WzString& displayName, const WzString& helpDescription);
public:
	OptionInfo& addAvailabilityCondition(const OptionAvailabilityCondition& condition);
	WzString getTranslatedDisplayName() const;
	WzString getTranslatedHelpDescription() const;
	bool isAvailable() const;
	std::vector<AvailabilityResult> getAvailabilityResults() const;
public:
	WzString optionId;
private:
	WzString displayName;
	WzString helpDescription;
	std::vector<OptionAvailabilityCondition> availabilityConditions;
};

// OptionAvailabilityConditions
OptionInfo::AvailabilityResult IsNotInGame(const OptionInfo&);
OptionInfo::AvailabilityResult IsNotMultiplayer(const OptionInfo&);

// OptionsSection - A section header for a subsequent series of options
//
// Example Usage:
// ```cpp
// OptionsSection(N_("Campaign Difficulty"), N_("Help description"));
// ```
// NOTE: The important use of N_("") to tag strings that need to be translated (but we always want to provide the untranslated original string to OptionSection, which handles getting the translation as needed)
class OptionsSection
{
public:
	OptionsSection(const WzString& displayName, const WzString& helpDescription);
public:
	const WzString& sectionId() const;
	WzString getTranslatedDisplayName() const;
	WzString getTranslatedHelpDescription() const;
private:
	WzString displayName;
	WzString helpDescription;
};

// OptionsForm displays a list of changeable options.
// Handles displaying based on option availability, refreshing options when others change, and help popovers.
class OptionsForm : public W_FORM
{
protected:
	OptionsForm();
	void initialize();

public:
	static std::shared_ptr<OptionsForm> make();

	// Add an option to the options form, along with an associated widget used to change its value
	template<typename T>
	std::enable_if_t<std::is_base_of<OptionValueChangerInterface, T>::value && std::is_base_of<WIDGET, T>::value> addOption(const OptionInfo& optionInfo, const std::shared_ptr<T>& optionValueChangerWidget, bool bottomBorder = false, int16_t indentLevel = 0)
	{
		addOptionInternal(optionInfo, optionValueChangerWidget, bottomBorder, indentLevel);
	}

	void refreshOptions();
	void refreshOptionAvailability();

	// WIDGET overrides
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override;
	bool processClickRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void display(int xOffset, int yOffset) override;

private:
	void addOptionInternal(const OptionInfo& optionInfo, const std::shared_ptr<WIDGET>& optionValueChangerWidget, bool bottomBorder = false, int16_t indentLevel = 0);
	void handleOptionValueChanged(const WzString& optionId);
	void refreshOptionsInternal(const std::unordered_set<WzString>& except);
	void openHelpPopover(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, const std::shared_ptr<WIDGET>& parent);
	void closeHelpPopover();
	void handleMouseIsOverRow(const std::shared_ptr<WIDGET>& rowWidget);
private:
	struct OptionDetails
	{
		OptionInfo info;
		std::shared_ptr<WIDGET> optionLabelWidget;
		std::shared_ptr<WIDGET> optionValueChangerWidgetWrapper;
		size_t idxInOptionsList;
	};
	std::unordered_map<WzString, OptionDetails> optionDetailsMap; // maps optionId -> option info and widgets

	WzString lastConfiguredTranslationLocale; // cached so we can check if this was updated and we need to potentially update translated strings for everything
	std::shared_ptr<ScrollableListWidget> optionsList;

	optional<Vector2i> mouseOverFormPosThisFrame = nullopt;
	std::shared_ptr<WIDGET> priorMouseOverRowWidget;

	std::shared_ptr<PopoverWidget> currentHelpPopoverWidget;

	bool isInRefreshOptionsInternal = false;
};
