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
	virtual void addStartingCapturedEditingHandler(std::function<void(WIDGET&)> handler) { }
	virtual void addEndingCapturedEditingHandler(std::function<void(WIDGET&)> handler) { }
	virtual optional<OptionChoiceHelpDescription> getHelpDescriptionForCurrentValue() { return nullopt; }
	virtual optional<std::vector<OptionChoiceHelpDescription>> getHelpDescriptions() { return nullopt; }
};

// OptionsSlider
class OptionsSlider : public WIDGET, public OptionValueChangerInterface
{
protected:
	OptionsSlider();
public:
	typedef std::function<int32_t(void)> CurrentValueFunc;
	typedef std::function<void (int32_t newValue)> OnChangeValueFunc;
public:
	static std::shared_ptr<OptionsSlider> make(int32_t minValue, int32_t maxValue, int32_t stepValue, CurrentValueFunc currentValFunc, OnChangeValueFunc onChange, bool needsStateUpdates);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

	void update(bool force) override;
	void informAvailable(bool isAvailable) override;
	void addOnChangeHandler(std::function<void(WIDGET&)> handler) override;

	int32_t currentValue() const;

private:
	void handleSliderPosChanged();
	void updateSliderValueText();
	void cacheIdealTextSize();
	size_t maxNumValueCharacters();

private:
	std::shared_ptr<W_SLIDER> sliderWidget;
	int32_t cachedIdealSliderWidth = 0;
	WzString text;
	WzCachedText wzText;
	iV_fonts FontID = font_regular;
	int32_t cachedIdealTextWidth = 0;
	int32_t cachedIdealTextLineSize = 0;
	int lastWidgetWidth = 0;
	int outerPaddingY = 8;
	int horizontalPadding = 10;
	bool isDisabled = false;
	bool isTruncated = false;
	CurrentValueFunc currentValFunc;
	OnChangeValueFunc onChangeValueFunc;
	int32_t minValue = 0;
	int32_t maxValue = 0;
	uint32_t totalRange = 0;
	int32_t stepValue = 1;
	bool needsStateUpdates;
};

class WzOptionsChoiceWidget : public WIDGET
{
protected:
	WzOptionsChoiceWidget(iV_fonts desiredFontID);

public:
	static std::shared_ptr<WzOptionsChoiceWidget> make(iV_fonts desiredFontID);

	void setString(WzString string) override;
	void setIcon(const AtlasImageDef *icon);
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
	void geometryChanged() override;
	void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
private:
	void recalcIconDrawSize();
	WzSize scaleIconSizeFor(optional<int> width, optional<int> height);
private:
	WzString text;
	WzCachedText wzText;
	iV_fonts FontID;
	const AtlasImageDef *icon = nullptr;
	int iconDrawWidth = 0;
	int iconDrawHeight = 0;
	PIELIGHT highlightBackgroundColor;
	int lastWidgetWidth = 0;
	int horizontalPadding = 10;
	int iconPaddingAfter = 5;
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

	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
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
	int32_t paddingLeft = 4;
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
	AtlasImageDef *icon = nullptr;

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

public:
	inline bool setCurrentIdxForValue(const T& value)
	{
		auto it = std::find_if(choices.begin(), choices.end(), [value](const OptionChoice<T>& choice) -> bool {
			return choice.value == value;
		});
		if (it == choices.end())
		{
			return false;
		}
		currentIdx = it - choices.begin();
		return true;
	}
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
				if (strongSelf->needsStateUpdates)
				{
					widgScheduleTask([strongSelf]() { strongSelf->update(false); });
				}
			}
			else
			{
				strongSelf->current.choices[newIndex].disabled = true;
				std::dynamic_pointer_cast<WzOptionsChoiceWidget>(newSelectedWidget)->setDisabled(true);
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

	void addStartingCapturedEditingHandler(std::function<void(WIDGET&)> handler) override
	{
		// Since dropdowns create an overlay screen, add this to the onOpen handler
		setOnOpen(handler);
	}

	void addEndingCapturedEditingHandler(std::function<void(WIDGET&)> handler) override
	{
		setOnClose(handler);
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
			item->setIcon(option.icon);
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
	OptionInfo& setRequiresRestart(bool restartRequired);
	WzString getTranslatedDisplayName() const;
	WzString getTranslatedHelpDescription() const;
	bool isAvailable() const;
	std::vector<AvailabilityResult> getAvailabilityResults() const;
	bool requiresRestart() const;
public:
	WzString optionId;
private:
	WzString displayName;
	WzString helpDescription;
	std::vector<OptionAvailabilityCondition> availabilityConditions;
	bool bRequiresRestart = false;
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

	virtual ~OptionsForm();

	void addSection(const OptionsSection& optionsSection, bool bottomBorder = false);

	// Add an option to the options form, along with an associated widget used to change its value
	template<typename T>
	std::enable_if_t<std::is_base_of<OptionValueChangerInterface, T>::value && std::is_base_of<WIDGET, T>::value> addOption(const OptionInfo& optionInfo, const std::shared_ptr<T>& optionValueChangerWidget, bool bottomBorder = false, int16_t indentLevel = 0)
	{
		addOptionInternal(optionInfo, optionValueChangerWidget, bottomBorder, indentLevel);
	}

	void refreshOptions(bool forceRowLayoutUpdates = false);
	void refreshOptionsLayoutCalc();
	void refreshOptionAvailability();

	bool hasOptionsThatRequireRestart() const;

	bool jumpToSectionId(const WzString& sectionId);
	bool jumpToOptionId(const WzString& optionId);

	size_t numSections() const;
	std::vector<OptionsSection> getSectionInfos() const;

	void setMaxListWidth(int32_t maxListWidth);
	void setRefreshOptionsOnScreenSizeChange(bool enabled);

	// WIDGET overrides
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;
protected:
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override;
	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;
	void display(int xOffset, int yOffset) override;

private:
	void addOptionInternal(const OptionInfo& optionInfo, const std::shared_ptr<WIDGET>& optionValueChangerWidget, bool bottomBorder = false, int16_t indentLevel = 0);
	void handleOptionValueChanged(const WzString& optionId);
	void refreshOptionsInternal(const std::unordered_set<WzString>& except);
	void openHelpPopover(const OptionInfo& optionInfo, const std::vector<OptionChoiceHelpDescription>& choiceHelpDescriptions, const std::shared_ptr<WIDGET>& parent);
	void closeHelpPopover();
	void handleStartedRowEditing(const WzString& optionId);
	void handleEndedRowEditing(const WzString& optionId);
	void handleMouseIsOverRow(const std::shared_ptr<WIDGET>& rowWidget);
	bool jumpToItemIdx(size_t idx, bool flash);
private:
	struct OptionDetails
	{
		OptionInfo info;
		std::shared_ptr<WIDGET> optionLabelWidget;
		std::shared_ptr<WIDGET> optionValueChangerWidgetWrapper;
		size_t idxInOptionsList;
	};
	std::unordered_map<WzString, OptionDetails> optionDetailsMap; // maps optionId -> option info and widgets

	struct SectionDetails
	{
		OptionsSection info;
		std::shared_ptr<WIDGET> optionSectionWidget;
		size_t idxInOptionsList;
	};
	std::unordered_map<WzString, SectionDetails> sectionDetailsMap; // maps sectionId -> index in scrollable list widget
	std::vector<WzString> orderedSectionIds;

	WzString lastConfiguredTranslationLocale; // cached so we can check if this was updated and we need to potentially update translated strings for everything
	std::shared_ptr<ScrollableListWidget> optionsList;

	optional<Vector2i> mouseOverFormPosThisFrame = nullopt;
	std::shared_ptr<WIDGET> priorMouseOverRowWidget;

	std::shared_ptr<WIDGET> capturedEditingRowWidget;

	std::shared_ptr<PopoverWidget> currentHelpPopoverWidget;

	int32_t maxListWidth = 0;
	bool refreshOptionsOnScreenSizeChange = false;

	bool isInRefreshOptionsInternal = false;
};
