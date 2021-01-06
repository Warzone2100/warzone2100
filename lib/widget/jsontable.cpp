/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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
/**
 * @file
 * Functions for scrollable JSON table widget.
 */

#include "jsontable.h"
#include "widgbase.h"
#include "button.h"
#include "label.h"
#include "table.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"

// MARK: - PathBarWidget

std::shared_ptr<PathBarWidget> PathBarWidget::make(const std::string& pathSeparator)
{
	auto result = std::make_shared<PathBarWidget>(pathSeparator);

	result->ellipsis = std::make_shared<W_LABEL>();
	result->attach(result->ellipsis);
	result->ellipsis->setFont(font_regular, WZCOL_FORM_TEXT);
	result->ellipsis->setString("...");
	result->ellipsis->setGeometry(0, 0, result->ellipsis->getMaxLineWidth(), iV_GetTextLineSize(font_regular));
	result->ellipsis->hide();

	result->pathSeparator = std::make_shared<W_LABEL>();
	result->pathSeparator->setFont(font_regular, WZCOL_FORM_TEXT);
	result->pathSeparator->setString(WzString::fromUtf8(pathSeparator));
	result->pathSeparator->setGeometry(0, 0, result->pathSeparator->getMaxLineWidth(), iV_GetTextLineSize(font_regular));
	// NOTE: Do not attach the pathSeparator widget - it's used for manual drawing

	return result;
}

void PathBarWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	// Draw the path bar background
	iV_ShadowBox(x0, y0, x1, y1, 0, pal_RGBA(20, 0, 88, 230), WZCOL_FORM_DARK, pal_RGBA(20, 0, 88, 230));

	if (pathComponents.size() <= 1) { return; }
	// Draw the path separator between all path components
	WidgetGraphicsContext baseContext;
	baseContext = baseContext.translatedBy(x() + xOffset, y() + yOffset);
	for (size_t idx = 0; idx < (pathComponents.size() - 1); idx++)
	{
		auto& button = componentButtons[idx];
		if (!button->visible())
		{
			continue;
		}
		WidgetGraphicsContext context = baseContext.translatedBy(button->x() + button->width(), button->y());
		pathSeparator->displayRecursive(context);
	}
}

void PathBarWidget::pushPathComponent(const std::string &pathComponent)
{
	pathComponents.push_back(pathComponent);
	size_t newComponentIndex = pathComponents.size() - 1;
	componentButtons[newComponentIndex] = createPathComponentButton(pathComponent, newComponentIndex);
	relayoutComponentButtons();
}

void PathBarWidget::popPathComponents(size_t numComponents)
{
	for (size_t i = 0; i < numComponents; i++)
	{
		pathComponents.pop_back();
		componentButtons[pathComponents.size()]->hide();
		componentButtons[pathComponents.size()]->deleteLater();
		componentButtons.erase(pathComponents.size());
	}
	relayoutComponentButtons();
}

void PathBarWidget::reset()
{
	pathComponents.clear();
	for (auto& button : componentButtons)
	{
		button.second->hide();
		button.second->deleteLater();
	}
	componentButtons.clear();
}

const std::vector<std::string>& PathBarWidget::currentPath() const
{
	return pathComponents;
}

std::string PathBarWidget::currentPathStr() const
{
	if (pathComponents.empty()) { return ""; }
	std::string fullPathString = pathComponents.front();
	for (size_t i = 1; i < pathComponents.size(); i++)
	{
		fullPathString += pathSeparatorStr + pathComponents[i];
	}
	return fullPathString;
}

struct PathButtonDisplayCache
{
	WzText text;
};

static void PathButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using PathButtonDisplayFunc must have its pUserData initialized to a (PathButtonDisplayCache*)
	assert(psWidget->pUserData != nullptr);
	PathButtonDisplayCache& cache = *static_cast<PathButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;
	int x1 = x0 + psButton->width();
	int y1 = y0 + psButton->height();

	bool haveText = !psButton->pText.isEmpty();

	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = (psButton->getState() & WBUT_HIGHLIGHT) != 0;

	// Display the button.
	auto light_border = pal_RGBA(255, 255, 255, 80);
	if (isDown || psButton->UserData == 0)
	{
		iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0,0,0,0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, isDown ? pal_RGBA(10, 0, 70, 250) : pal_RGBA(25, 0, 110, 220));
	}
	if (isHighlight)
	{
		iV_Box(x0, y0, x1, y1, WZCOL_FORM_LIGHT);
	}

	if (haveText)
	{
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fw = cache.text.width();
		int fx = x0 + (psButton->width() - fw) / 2;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			cache.text.render(fx + 1, fy + 1, WZCOL_FORM_LIGHT);
			textColor = WZCOL_FORM_DISABLE;
		}
		cache.text.render(fx, fy, textColor);
	}

	if (isDisabled)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + psButton->width(), y0 + psButton->height());
	}
}

std::shared_ptr<W_BUTTON> PathBarWidget::createPathComponentButton(const std::string& pathComponent, size_t newComponentIndex)
{
	W_BUTINIT butInit;
	butInit.pText = pathComponent.c_str();
	butInit.FontID = font_regular;

	int textWidth = iV_GetTextWidth(pathComponent.c_str(), butInit.FontID);
	int buttonWidth = textWidth + 2;
	butInit.width = buttonWidth;
	butInit.height = iV_GetTextLineSize(butInit.FontID);
	butInit.UserData = static_cast<UDWORD>(newComponentIndex);
	butInit.pDisplay = PathButtonDisplayFunc;
	butInit.initPUserDataFunc = []() -> void * { return new PathButtonDisplayCache(); };
	butInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<PathButtonDisplayCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};

	auto button = std::make_shared<W_BUTTON>(&butInit);
	attach(button);
	button->addOnClickHandler([newComponentIndex](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<PathBarWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		if (psParent->onClickPath)
		{
			psParent->onClickPath(*psParent, newComponentIndex);
		}
	});

	return button;
}

void PathBarWidget::relayoutComponentButtons()
{
	ellipsis->hide();

	if (componentButtons.empty()) { return; }

	const int requiredWidthForEllipsis = ellipsis->width();
	// start from the end / right-side, and display as many path component buttons as possible
	// while always displaying the first path component!
	int remainingWidth = width();
	const int spaceBetweenButtons = pathSeparator->width();

	int widthRequiredToDisplayAllButtonsInFull = componentButtons[0]->width();
	for (size_t idx = pathComponents.size() - 1; idx > 0; idx--)
	{
		widthRequiredToDisplayAllButtonsInFull += spaceBetweenButtons + componentButtons[idx]->width();
	}

	// display the first path component on the left side
	componentButtons[0]->move(0, componentButtons[0]->y());

	remainingWidth -= (componentButtons[0]->width() + spaceBetweenButtons);
	int lastButtonX = width() + spaceBetweenButtons;
	for (size_t idx = pathComponents.size() - 1; idx > 0; idx--)
	{
		auto& button = componentButtons[idx];
		bool additionalButtonsRemaining = (idx > 1);
		int requiredRemainingWidthToDisplayThisButton = button->width() + spaceBetweenButtons;
		if (additionalButtonsRemaining)
		{
			requiredRemainingWidthToDisplayThisButton += requiredWidthForEllipsis + spaceBetweenButtons;
		}
		if (requiredRemainingWidthToDisplayThisButton >= remainingWidth)
		{
			// not enough space to display this button + remaining buttons
			// stop looping and add the ellipsis instead
			ellipsis->move(componentButtons[0]->x() + componentButtons[0]->width() + spaceBetweenButtons, ellipsis->y());
			ellipsis->show();
			int extraSpace = lastButtonX - (ellipsis->x() + ellipsis->width() + spaceBetweenButtons);
			if (extraSpace > 0)
			{
				// then shift all the buttons (besides the first one) over by any remaining width
				for (size_t i = idx + 1; i < pathComponents.size(); i++)
				{
					componentButtons[i]->move(componentButtons[i]->x() - extraSpace, componentButtons[i]->y());
				}
			}
			// finally, set any remaining buttons to invisible
			for (size_t i = idx; i > 0; i--)
			{
				componentButtons[i]->hide();
			}
			break;
		}
		button->move(lastButtonX - spaceBetweenButtons - button->width(), button->y());
		button->show();
		lastButtonX = button->x();
		remainingWidth -= (spaceBetweenButtons + button->width());
	}

	if (!ellipsis->visible() && pathComponents.size() > 1)
	{
		int extraSpace = componentButtons[1]->x() - (componentButtons[0]->x() + componentButtons[0]->width() + spaceBetweenButtons);
		if (extraSpace > 0)
		{
			// then shift all the buttons (besides the first one) over by any remaining width
			for (size_t i = 1; i < pathComponents.size(); i++)
			{
				componentButtons[i]->move(componentButtons[i]->x() - extraSpace, componentButtons[i]->y());
			}
		}
	}
}

// MARK: - JSONTableWidget

#define JSON_ACTION_BUTTONS_PADDING 10
#define JSON_ACTION_BUTTONS_SPACING 10
#define MENU_BUTTONS_PADDING 20

template<typename json_type>
void setTableToJson(const json_type& json, JSONTableWidget& jsonTable)
{
	jsonTable.table->clearRows();
	jsonTable.currentMaxColumnWidths = {0,0,0};
	PIELIGHT expandableValueTextColor = WZCOL_TEXT_MEDIUM;
	expandableValueTextColor.byte.a = static_cast<uint8_t>((float)expandableValueTextColor.byte.a * 0.75f);
	for (auto item : json.items())
	{
		auto keyLabel = std::make_shared<W_LABEL>();
		keyLabel->setFont(font_regular, WZCOL_FORM_LIGHT);
		keyLabel->setString(WzString::fromUtf8(item.key()));
		keyLabel->setCanTruncate(true);
		keyLabel->setTransparentToClicks(true);
		jsonTable.currentMaxColumnWidths[0] = std::max(jsonTable.currentMaxColumnWidths[0], keyLabel->getMaxLineWidth());

		auto valueLabel = std::make_shared<W_LABEL>();
		std::string valueStr;
		PIELIGHT valueTextColor = WZCOL_FORM_LIGHT;
		bool expandableItem = false;
		if (item.value().is_object())
		{
			valueStr = "[Object]";
			if (!item.value().empty())
			{
				expandableItem = true;
				valueStr += " (items: " + std::to_string(item.value().size()) + ")";
			}
			else
			{
				valueStr += " (empty)";
			}
			valueTextColor = expandableValueTextColor;
		}
		else if (item.value().is_array())
		{
			valueStr = "[Array]";
			if (!item.value().empty())
			{
				expandableItem = true;
				valueStr += "[" + std::to_string(item.value().size()) + "]";
			}
			else
			{
				valueStr += " (empty)";
			}
			valueTextColor = expandableValueTextColor;
		}
		else if (item.value().is_null())
		{
			valueStr = "[null]";
		}
		else
		{
			try {
				valueStr = item.value().dump();
			}
			catch (const std::exception& e)
			{
				debug(LOG_ERROR, "Failed to dump value: %s", e.what());
				valueStr = "<failed to dump value>";
				valueTextColor = pal_RGBA(150, 0, 0, 250);
			}
		}
		valueLabel->setFont(font_regular, valueTextColor);
		valueLabel->setString(WzString::fromUtf8(valueStr));
		valueLabel->setCanTruncate(true);
		valueLabel->setTransparentToClicks(true);
		jsonTable.currentMaxColumnWidths[1] = std::max(jsonTable.currentMaxColumnWidths[1], valueLabel->getMaxLineWidth());

		auto expandButton = std::make_shared<W_LABEL>();
		expandButton->setFont(font_regular, WZCOL_TEXT_MEDIUM);
		if (expandableItem)
		{
			expandButton->setString("\u27A4"); // ➤
		}
		expandButton->setCanTruncate(false);
		expandButton->setTransparentToClicks(true);

		std::vector<std::shared_ptr<WIDGET>> columnWidgets {keyLabel, valueLabel, expandButton};
		auto row = TableRow::make(columnWidgets, 24);
		// Add custom "on click" handler TableRow itself to "dive into" objects / arrays (if expandableItem)
		if (expandableItem)
		{
			std::string itemKey = item.key();
			std::weak_ptr<JSONTableWidget> psWeakJsonTable = std::dynamic_pointer_cast<JSONTableWidget>(jsonTable.shared_from_this());
			row->addOnClickHandler([itemKey, psWeakJsonTable](W_BUTTON& button) {
				widgScheduleTask([psWeakJsonTable, itemKey]{
					if (auto jsonTable = psWeakJsonTable.lock())
					{
						jsonTable->pushJSONPath(itemKey);
					}
				});
			});
			row->setHighlightsOnMouseOver(true);
		}
		jsonTable.table->addRow(row);
	}
}

template<typename json_type>
static bool jsonHasExpandableChildren(const json_type& json)
{
	for (auto item : json.items())
	{
		if (item.value().is_object())
		{
			if (!item.value().empty())
			{
				return true;
			}
		}
		else if (item.value().is_array())
		{
			if (!item.value().empty())
			{
				return true;
			}
		}
	}
	return false;
}

JSONTableWidget::~JSONTableWidget()
{
	if (optionsOverlayScreen)
	{
		widgRemoveOverlayScreen(optionsOverlayScreen);
	}
}

struct PopoverMenuButtonDisplayCache
{
	WzText text;
};

static void PopoverMenuButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using PopoverMenuButtonDisplayFunc must have its pUserData initialized to a (PopoverMenuButtonDisplayCache*)
	assert(psWidget->pUserData != nullptr);
	PopoverMenuButtonDisplayCache& cache = *static_cast<PopoverMenuButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;

	bool haveText = !psButton->pText.isEmpty();

	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = (psButton->getState() & WBUT_HIGHLIGHT) != 0;

	// Display the button background
	PIELIGHT backgroundColor;
	backgroundColor.rgba = 0;
	if (isDown)
	{
		backgroundColor = pal_RGBA(10, 0, 70, 250); //WZCOL_FORM_DARK;
	}
	else if (isHighlight)
	{
		backgroundColor = pal_RGBA(25, 0, 110, 220); //WZCOL_TEXT_MEDIUM;
	}
	if (backgroundColor.rgba != 0)
	{
		// Draw the background
		pie_UniTransBoxFill(x0, y0, x0 + psButton->width(), y0 + psButton->height(), backgroundColor);
	}

	if (haveText)
	{
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fx = x0 + JSON_ACTION_BUTTONS_PADDING;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			cache.text.render(fx + 1, fy + 1, WZCOL_FORM_LIGHT);
			textColor = WZCOL_FORM_DISABLE;
		}
		cache.text.render(fx, fy, textColor);
	}

	if (isDisabled)
	{
		// disabled, render something over it!
		iV_TransBoxFill(x0, y0, x0 + psButton->width(), y0 + psButton->height());
	}
}

std::shared_ptr<WIDGET> JSONTableWidget::createOptionsPopoverForm()
{
	// create all the buttons / option rows
	std::weak_ptr<JSONTableWidget> weakJSONTableWidget = std::dynamic_pointer_cast<JSONTableWidget>(shared_from_this());
	auto createOptionsButton = [weakJSONTableWidget](const WzString& text, const std::function<void (W_BUTTON& button)>& onClickFunc) -> std::shared_ptr<W_BUTTON> {
		auto button = std::make_shared<W_BUTTON>();
		button->setString(text);
		button->FontID = font_regular;
		button->displayFunction = PopoverMenuButtonDisplayFunc;
		button->pUserData = new PopoverMenuButtonDisplayCache();
		button->setOnDelete([](WIDGET *psWidget) {
			assert(psWidget->pUserData != nullptr);
			delete static_cast<PopoverMenuButtonDisplayCache *>(psWidget->pUserData);
			psWidget->pUserData = nullptr;
		});
		int minButtonWidthForText = iV_GetTextWidth(text.toUtf8().c_str(), button->FontID);
		button->setGeometry(0, 0, minButtonWidthForText + MENU_BUTTONS_PADDING, iV_GetTextLineSize(button->FontID) + 4);

		// On click, perform the designated onClickHandler and close the popover form / overlay
		button->addOnClickHandler([weakJSONTableWidget, onClickFunc](W_BUTTON& button) {
			if (auto JSONTableWidget = weakJSONTableWidget.lock())
			{
				widgRemoveOverlayScreen(JSONTableWidget->optionsOverlayScreen);
				onClickFunc(button);
				JSONTableWidget->optionsOverlayScreen.reset();
			}
		});
		return button;
	};

	std::vector<std::shared_ptr<W_BUTTON>> buttons;
	if (pathBar && pathBar->visible())
	{
		buttons.push_back(createOptionsButton(_("Copy Path to Clipboard"), [weakJSONTableWidget](W_BUTTON& button){
			if (auto JSONTableWidget = weakJSONTableWidget.lock())
			{
				wzSetClipboardText(JSONTableWidget->currentJSONPathStr().c_str());
			}
		}));
	}
	buttons.push_back(createOptionsButton(_("Copy JSON to Clipboard"), [weakJSONTableWidget](W_BUTTON& button){
		if (auto JSONTableWidget = weakJSONTableWidget.lock())
		{
			std::string dumpedData = JSONTableWidget->dumpDataAtCurrentPath();
			wzSetClipboardText(dumpedData.c_str());
		}
	}));
	buttons.push_back(createOptionsButton(_("Dump JSON to StdOut"), [weakJSONTableWidget](W_BUTTON& button){
		if (auto JSONTableWidget = weakJSONTableWidget.lock())
		{
			std::string dumpedData = JSONTableWidget->dumpDataAtCurrentPath();
			fprintf(stdout, "[Dumped JSON from path]: %s\n%s\n", JSONTableWidget->currentJSONPathStr().c_str(), dumpedData.c_str());
		}
	}));

	// determine required height for all buttons
	int totalButtonHeight = std::accumulate(buttons.begin(), buttons.end(), 0, [](int a, const std::shared_ptr<W_BUTTON>& b) {
		return a + b->height();
	});
	int maxButtonWidth = (*(std::max_element(buttons.begin(), buttons.end(), [](const std::shared_ptr<W_BUTTON>& a, const std::shared_ptr<W_BUTTON>& b){
		return a->width() < b->width();
	})))->width();

	auto itemsList = ScrollableListWidget::make();
	itemsList->setBackgroundColor(WZCOL_MENU_BACKGROUND);
	itemsList->setPadding({3, 4, 3, 4});
	const int itemSpacing = 1;
	itemsList->setItemSpacing(itemSpacing);
	itemsList->setGeometry(itemsList->x(), itemsList->y(), maxButtonWidth + 8, totalButtonHeight + (static_cast<int>(buttons.size()) * itemSpacing) + 6);
	for (auto& button : buttons)
	{
		itemsList->addItem(button);
	}

	return itemsList;
}

static void displayChildDropShadows(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	PIELIGHT dropShadowColor = pal_RGBA(0, 0, 0, 40);
	const int widerPadding = 4;
	const int closerPadding = 2;
	int childXOffset = psWidget->x() + xOffset;
	int childYOffset = psWidget->y() + yOffset;
	for (auto& child : psWidget->children())
	{
		if (!child->visible()) { continue; }
		int childX0 = child->x() + childXOffset;
		int childY0 = child->y() + childYOffset;
		int childDropshadowWiderX0 = std::max(childX0 - widerPadding, 0);
		int childDropshadowWiderX1 = std::min(childX0 + child->width() + widerPadding, pie_GetVideoBufferWidth());
		int childDropshadowWiderY1 = std::min(childY0 + child->height() + widerPadding, pie_GetVideoBufferHeight());
		int childDropshadowCloserX0 = std::max(childX0 - closerPadding, 0);
		int childDropshadowCloserX1 = std::min(childX0 + child->width() + closerPadding, pie_GetVideoBufferWidth());
		int childDropshadowCloserY1 = std::min(childY0 + child->height() + closerPadding, pie_GetVideoBufferHeight());
		pie_UniTransBoxFill(childDropshadowWiderX0, childY0, childDropshadowWiderX1, childDropshadowWiderY1, dropShadowColor);
		pie_UniTransBoxFill(childDropshadowCloserX0, childY0, childDropshadowCloserX1, childDropshadowCloserY1, dropShadowColor);
	}
}

void JSONTableWidget::displayOptionsOverlay(const std::shared_ptr<WIDGET>& psParent)
{
	auto lockedScreen = screenPointer.lock();
	ASSERT(lockedScreen != nullptr, "The JSONTableWidget does not have an associated screen pointer?");

	// Initialize the options overlay screen
	optionsOverlayScreen = W_SCREEN::make();
	auto newRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	newRootFrm->displayFunction = displayChildDropShadows;
	std::weak_ptr<W_SCREEN> psWeakOptionsOverlayScreen(optionsOverlayScreen);
	std::weak_ptr<JSONTableWidget> psWeakJsonTable(std::dynamic_pointer_cast<JSONTableWidget>(shared_from_this()));
	newRootFrm->onClickedFunc = [psWeakOptionsOverlayScreen, psWeakJsonTable]() {
		if (auto psOverlayScreen = psWeakOptionsOverlayScreen.lock())
		{
			widgRemoveOverlayScreen(psOverlayScreen);
		}
		// Destroy Options overlay / overlay screen
		if (auto strongJsonTable = psWeakJsonTable.lock())
		{
			strongJsonTable->optionsOverlayScreen.reset();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;
	optionsOverlayScreen->psForm->attach(newRootFrm);

	// Create the pop-over form
	auto optionsPopOver = createOptionsPopoverForm();
	newRootFrm->attach(optionsPopOver);

	// Position the pop-over form
	std::weak_ptr<WIDGET> weakParent = psParent;
	optionsPopOver->setCalcLayout([weakParent](WIDGET *psWidget, unsigned int, unsigned int, unsigned int newScreenWidth, unsigned int newScreenHeight){
		auto psParent = weakParent.lock();
		ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");
		// (Ideally, with its left aligned with the left side of the "parent" widget, but ensure full visibility on-screen)
		int popOverX0 = psParent->screenPosX();
		if (popOverX0 + psWidget->width() > newScreenWidth)
		{
			popOverX0 = newScreenWidth - psWidget->width();
		}
		// (Ideally, with its top aligned with the bottom of the "parent" widget, but ensure full visibility on-screen)
		int popOverY0 = psParent->screenPosY() + psParent->height();
		if (popOverY0 + psWidget->height() > newScreenHeight)
		{
			popOverY0 = newScreenHeight - psWidget->height();
		}
		psWidget->move(popOverX0, popOverY0);
	});

	widgRegisterOverlayScreenOnTopOfScreen(optionsOverlayScreen, lockedScreen);
}

struct JsonActionButtonDisplayCache
{
	WzText text;
};

static void JsonActionButtonDisplayFunc(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	// Any widget using JsonActionButtonDisplayFunc must have its pUserData initialized to a (JsonActionButtonDisplayCache*)
	assert(psWidget->pUserData != nullptr);
	JsonActionButtonDisplayCache& cache = *static_cast<JsonActionButtonDisplayCache*>(psWidget->pUserData);

	W_BUTTON *psButton = dynamic_cast<W_BUTTON*>(psWidget);
	ASSERT_OR_RETURN(, psButton, "psWidget is null");

	int x0 = psButton->x() + xOffset;
	int y0 = psButton->y() + yOffset;
	int x1 = x0 + psButton->width();
	int y1 = y0 + psButton->height();

	bool haveText = !psButton->pText.isEmpty();

	bool isDown = (psButton->getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (psButton->getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((psButton->getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto light_border = pal_RGBA(255, 255, 255, 80);
	auto fill_color = isDown || isDisabled ? pal_RGBA(10, 0, 70, 250) : pal_RGBA(25, 0, 110, 220);
	iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0,0,0,0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, fill_color);
	if (isHighlight)
	{
		iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
	}

	if (haveText)
	{
		cache.text.setText(psButton->pText.toUtf8(), psButton->FontID);
		int fw = cache.text.width();
		int fx = x0 + (psButton->width() - fw) / 2;
		int fy = y0 + (psButton->height() - cache.text.lineSize()) / 2 - cache.text.aboveBase();
		PIELIGHT textColor = WZCOL_FORM_TEXT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		cache.text.render(fx, fy, textColor);
	}
}

static std::shared_ptr<W_BUTTON> makeJsonActionButton(const char* text)
{
	auto button = std::make_shared<W_BUTTON>();
	button->setString(text);
	button->FontID = font_regular;
	button->displayFunction = JsonActionButtonDisplayFunc;
	button->pUserData = new JsonActionButtonDisplayCache();
	button->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<JsonActionButtonDisplayCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	int minButtonWidthForText = iV_GetTextWidth(text, button->FontID);
	button->setGeometry(0, 0, minButtonWidthForText + JSON_ACTION_BUTTONS_PADDING, iV_GetTextLineSize(button->FontID));
	return button;
}

std::shared_ptr<JSONTableWidget> JSONTableWidget::make(const std::string& title)
{
	auto result = std::make_shared<JSONTableWidget>();

	if (!title.empty())
	{
		// Add title text
		result->titleLabel = std::make_shared<W_LABEL>();
		result->titleLabel->setFont(font_regular_bold, WZCOL_FORM_TEXT);
		result->titleLabel->setString(WzString::fromUtf8(title));
		result->titleLabel->setGeometry(/*5*/0, 0, result->titleLabel->getMaxLineWidth() + JSON_ACTION_BUTTONS_PADDING, iV_GetTextLineSize(font_regular_bold));
		result->titleLabel->setCacheNeverExpires(true);
		result->attach(result->titleLabel);
	}

	// Add "gear" / options button
	result->optionsButton = makeJsonActionButton("\u2699"); // "⚙"
	result->optionsButton->setTip("Options");
	result->attach(result->optionsButton);
	result->optionsButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		// Display a "pop-over" options menu
		psParent->displayOptionsOverlay(button.shared_from_this());
	});
	result->optionsButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = psParent->width() - psWidget->width();
		psWidget->setGeometry(x0, psWidget->y(), psWidget->width(), psWidget->height());
	}));

	// Add "update"/refresh button
	result->updateButton = makeJsonActionButton("\u21BB"); // "↻"
	result->updateButton->setTip("Update");
	result->attach(result->updateButton);
	result->updateButton->addOnClickHandler([](W_BUTTON& button) {
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(button.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		widgScheduleTask([psParent](){
			if (psParent->updateButtonFunc)
			{
				psParent->updateButtonFunc(*psParent);
			}
		});
	});
	result->updateButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = psParent->optionsButton->x() - psWidget->width() - JSON_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(x0, psWidget->y(), psWidget->width(), psWidget->height());
	}));
	result->updateButton->hide(); // hidden by default until an updateButtonFunc is set

	// Add "path bar" widget
	result->pathBar = PathBarWidget::make(".");
	result->pathBar->setGeometry(0, 0, 0, iV_GetTextLineSize(font_regular));
	result->pathBar->pushPathComponent(" $ ");
	result->pathBar->setOnClickPath([](PathBarWidget& pathBar, size_t componentIndex) {
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(pathBar.parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		std::weak_ptr<JSONTableWidget> psWeakJsonTable = psParent;
		size_t numComponentsToPop = pathBar.numPathComponents() - (componentIndex + 1);
		if (numComponentsToPop == 0) { return; }
		widgScheduleTask([psWeakJsonTable, numComponentsToPop]{
			if (auto jsonTable = psWeakJsonTable.lock())
			{
				jsonTable->popJSONPath(numComponentsToPop);
			}
		});
	});
	result->attach(result->pathBar);
	result->pathBar->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int x0 = (psParent->titleLabel) ? psParent->titleLabel->x() + psParent->titleLabel->width() : 0;
		int rightMostEndPoint = ((psParent->updateButton->visible()) ? psParent->updateButton->x() : psParent->optionsButton->x()) - JSON_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(x0, 0, rightMostEndPoint - x0, psWidget->height());
	}));
	result->pathBar->hide(); // hide path bar by default (until data is set)

	// Create Key / Value / "Expand" column headers
	auto keyLabel = std::make_shared<W_LABEL>();
	keyLabel->setString("Key");
	keyLabel->setGeometry(0, 0, 150, 0);
	keyLabel->setCacheNeverExpires(true);
	auto valueLabel = std::make_shared<W_LABEL>();
	valueLabel->setString("Value");
	valueLabel->setGeometry(0, 0, 300, 0);
	valueLabel->setCacheNeverExpires(true);
	auto expandLabel = std::make_shared<W_LABEL>();
	expandLabel->setString("+");
	expandLabel->setGeometry(0, 0, expandLabel->getMaxLineWidth(), 0);
	expandLabel->setCacheNeverExpires(true);
	std::vector<TableColumn> columns {{keyLabel, TableColumn::ResizeBehavior::RESIZABLE}, {valueLabel, TableColumn::ResizeBehavior::RESIZABLE_AUTOEXPAND}, {expandLabel, TableColumn::ResizeBehavior::FIXED_WIDTH}};

	std::vector<size_t> minimumColumnWidths;
	minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(keyLabel->getMaxLineWidth(), 0)));
	minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(valueLabel->getMaxLineWidth(), 0)));
	minimumColumnWidths.push_back(static_cast<size_t>(std::max<int>(expandLabel->getMaxLineWidth(), 0)));

	// Create + attach key / value scrollable table view
	result->table = ScrollableTableWidget::make(columns);
	result->attach(result->table);
	result->table->setMinimumColumnWidths(minimumColumnWidths);
	result->table->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = std::dynamic_pointer_cast<JSONTableWidget>(psWidget->parent());
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int oldWidth = psWidget->width();
		int y0 = psParent->pathBar->y() + psParent->pathBar->height() + JSON_ACTION_BUTTONS_SPACING;
		psWidget->setGeometry(0, y0, psParent->width(), psParent->height() - y0);

		if (oldWidth != psWidget->width())
		{
			psParent->resizeColumnWidths(false);
		}
	}));

	return result;
}

void JSONTableWidget::display(int xOffset, int yOffset)
{
	// currently, no-op
}

void JSONTableWidget::geometryChanged()
{
	if (optionsButton)
	{
		optionsButton->callCalcLayout();
	}
	if (updateButton)
	{
		updateButton->callCalcLayout();
	}
	if (pathBar)
	{
		pathBar->callCalcLayout();
	}
	if (table)
	{
		// depends on the positioning of pathBar
		table->callCalcLayout();
	}
}

void JSONTableWidget::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// call default implementation (which ultimately propagates to all children
	W_FORM::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);

	// NOTE: To properly support resizing the options overlay screen based on underlying screen layer recalculations
	// this must occur *after* the default implementation (above)
	if (optionsOverlayScreen == nullptr) return;
	optionsOverlayScreen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

std::vector<std::string> JSONTableWidget::currentPathFromRoot() const
{
	return actualPushedPathComponents;
}

void JSONTableWidget::resetPath()
{
	// Reset pathBar
	pathBar->reset();
	pathBar->pushPathComponent(" $ ");
	pathBar->show();

	actualPushedPathComponents.clear();
}

bool JSONTableWidget::rootJsonHasExpandableChildren() const
{
	if (_orderedjson.has_value())
	{
		return jsonHasExpandableChildren(_orderedjson.value());
	}
	else if (_json.has_value())
	{
		return jsonHasExpandableChildren(_json.value());
	}
	return false;
}

void JSONTableWidget::updateData_Internal(const std::function<void ()>& setJsonFunc, bool tryToPreservePath)
{
	std::vector<std::string> previousPath = currentPathFromRoot();
	auto oldScrollPosition = table->getScrollPosition();
	resetPath();

	setJsonFunc();

	bool hasExpandableChildren = rootJsonHasExpandableChildren();
	if (!tryToPreservePath)
	{
		previousPath.clear();
	}
	// return to the same path (or as close to it as possible) in the updated JSON
	bool returnedToSameExactPath = pushJSONPath(previousPath, !tryToPreservePath); // this already calls rebuildTableFromJSON()
	if (returnedToSameExactPath)
	{
		table->setScrollPosition(oldScrollPosition);
	}
	if (!hasExpandableChildren)
	{
		// hide path bar
		pathBar->hide();
	}
	refreshUpdateButtonState();
}

void JSONTableWidget::updateData(const nlohmann::ordered_json& json, bool tryToPreservePath /*= false*/)
{
	updateData_Internal([this, &json]{
		_orderedjson = json;
		_orderedjson_currentPointer = {&(_orderedjson.value())};

		_json = nullopt;
		_json_currentPointer.clear();
	}, tryToPreservePath);
}

void JSONTableWidget::updateData(const nlohmann::json& json, bool tryToPreservePath /*= false*/)
{
	updateData_Internal([this, &json]{
		_json = json;
		_json_currentPointer = {&(_json.value())};

		_orderedjson = nullopt;
		_orderedjson_currentPointer.clear();
	}, tryToPreservePath);
}

std::string JSONTableWidget::currentJSONPathStr() const
{
	ASSERT_OR_RETURN("", pathBar != nullptr, "pathBar is null");
	return pathBar->currentPathStr();
}

std::string JSONTableWidget::dumpDataAtCurrentPath() const
{
	if (_orderedjson.has_value())
	{
		ASSERT_OR_RETURN("", !_orderedjson_currentPointer.empty(), "Should not be empty");
		return (*(_orderedjson_currentPointer.back())).dump(1, '\t', true);
	}
	else if (_json.has_value())
	{
		ASSERT_OR_RETURN("", !_json_currentPointer.empty(), "Should not be empty");
		return (*(_json_currentPointer.back())).dump(1, '\t', true);
	}
	return "";
}

void JSONTableWidget::setUpdateButtonFunc(const CallbackFunc& newUpdateButtonFunc, optional<UDWORD> newAutomaticUpdateInterval)
{
	updateButtonFunc = newUpdateButtonFunc;
	automaticUpdateInterval = newAutomaticUpdateInterval;
	refreshUpdateButtonState();
}

void JSONTableWidget::setAutomaticUpdateInterval(optional<UDWORD> newAutomaticUpdateInterval)
{
	automaticUpdateInterval = newAutomaticUpdateInterval;
	refreshUpdateButtonState();
}

void JSONTableWidget::refreshUpdateButtonState()
{
	bool updateButtonWasVisible = updateButton->visible();
	if (!updateButtonFunc)
	{
		updateButton->hide();
	}
	else
	{
		updateButton->show();
		if (automaticUpdateInterval.has_value())
		{
			updateButton->setProgressBorder(W_BUTTON::ProgressBorder::timed(automaticUpdateInterval.value(), true, W_BUTTON::ProgressBorder::BorderInset(1, 1, 1, 1)), pal_RGBA(100, 100, 255, 200));
			lastUpdateTime = realTime;
		}
		else
		{
			updateButton->setProgressBorder(nullopt);
		}
	}
	if (updateButtonWasVisible != updateButton->visible())
	{
		if (pathBar)
		{
			pathBar->callCalcLayout();
		}
	}
}

void JSONTableWidget::run(W_CONTEXT *psContext)
{
	if (!automaticUpdateInterval.has_value() || !updateButtonFunc)
	{
		return;
	}

	if (realTime - lastUpdateTime >= automaticUpdateInterval.value())
	{
		// trigger update call
		auto jsonTable = std::dynamic_pointer_cast<JSONTableWidget>(shared_from_this());
		widgScheduleTask([jsonTable](){
			if (jsonTable->updateButtonFunc)
			{
				jsonTable->updateButtonFunc(*jsonTable);
				jsonTable->lastUpdateTime = realTime;
				jsonTable->refreshUpdateButtonState();
			}
		});
		lastUpdateTime = realTime;
	}
}

void JSONTableWidget::resizeColumnWidths(bool overrideUserColumnSizing)
{
	if (table->isUserDraggingColumnHeader())
	{
		return;
	}

	// if necessary / possible, resize the key/value column widths
	std::vector<size_t> columnWidths = table->getColumnWidths();
	columnWidths[0] = currentMaxColumnWidths[0];
	columnWidths[1] = currentMaxColumnWidths[1];
	table->changeColumnWidths(columnWidths, overrideUserColumnSizing);
}

void JSONTableWidget::rebuildTableFromJSON(bool overrideUserColumnSizing)
{
	if (_orderedjson.has_value())
	{
		ASSERT_OR_RETURN(, !_orderedjson_currentPointer.empty(), "Should not be empty");
		setTableToJson(*(_orderedjson_currentPointer.back()), *this);
	}
	else if (_json.has_value())
	{
		ASSERT_OR_RETURN(, !_json_currentPointer.empty(), "Should not be empty");
		setTableToJson(*(_json_currentPointer.back()), *this);
	}
	resizeColumnWidths(overrideUserColumnSizing);
}

template<typename json_type>
static optional<std::string> pushJSONPath(const std::string& keyInCurrentJSONPointer, std::vector<json_type>& jsonPointerVec)
{
	ASSERT_OR_RETURN(nullopt, !jsonPointerVec.empty(), "Should not be empty");
	auto& current_json_level = *(jsonPointerVec.back());
	if (current_json_level.is_object())
	{
		auto it = current_json_level.find(keyInCurrentJSONPointer);
		ASSERT_OR_RETURN(nullopt, it != current_json_level.end(), "Could not find key");
		jsonPointerVec.push_back(&(it.value()));
		return keyInCurrentJSONPointer;
	}
	else if (current_json_level.is_array())
	{
		bool ok = false;
		int arrayIndex = WzString::fromUtf8(keyInCurrentJSONPointer).toInt(&ok);
		ASSERT_OR_RETURN(nullopt, ok, "Failed to convert %s to int", keyInCurrentJSONPointer.c_str());
		ASSERT_OR_RETURN(nullopt, arrayIndex >= 0 && arrayIndex < current_json_level.size(), "Array index out of bounds: %d", arrayIndex);
		jsonPointerVec.push_back(&(current_json_level[arrayIndex]));
		return std::string("[") + keyInCurrentJSONPointer + "]";
	}
	return nullopt;
}

template<typename json_type>
static size_t popJSONPath(std::vector<json_type>& jsonPointerVec, size_t numLevels = 1)
{
	if (numLevels == 0) { return 0; }
	ASSERT_OR_RETURN(0, !jsonPointerVec.empty(), "Should not be empty");
	size_t i = 0;
	for (; i < numLevels; i++)
	{
		if (jsonPointerVec.size() <= 1)
		{
			break;
		}
		jsonPointerVec.pop_back();
	}
	return i;
}

bool JSONTableWidget::pushJSONPath(const std::vector<std::string>& pathInCurrentJSONPointer, bool overrideUserColumnSizing)
{
	for (const auto& newPathComponent : pathInCurrentJSONPointer)
	{
		optional<std::string> descriptionOfNewPathComponent;
		if (_orderedjson.has_value())
		{
			descriptionOfNewPathComponent = ::pushJSONPath(newPathComponent, _orderedjson_currentPointer);
		}
		else if (_json.has_value())
		{
			descriptionOfNewPathComponent = ::pushJSONPath(newPathComponent, _json_currentPointer);
		}
		if (!descriptionOfNewPathComponent.has_value())
		{
			debug(LOG_WZ, "Path component %s no longer exists at parent path: %s", newPathComponent.c_str(), pathBar->currentPathStr().c_str());
			rebuildTableFromJSON(true); // new path, always override user column sizing (if possible)
			return false;
		}
		pathBar->pushPathComponent(descriptionOfNewPathComponent.value());
		actualPushedPathComponents.push_back(newPathComponent);
	}
	rebuildTableFromJSON(!pathInCurrentJSONPointer.empty() && overrideUserColumnSizing);
	return true;
}

bool JSONTableWidget::pushJSONPath(const std::string& keyInCurrentJSONPointer)
{
	return pushJSONPath(std::vector<std::string>({keyInCurrentJSONPointer}), true);
}

size_t JSONTableWidget::popJSONPath(size_t numLevels /*= 1*/)
{
	size_t result = 0;
	if (_orderedjson.has_value())
	{
		result = ::popJSONPath(_orderedjson_currentPointer, numLevels);
	}
	else
	{
		result = ::popJSONPath(_json_currentPointer, numLevels);
	}
	pathBar->popPathComponents(result);
	for (size_t i = 0; i < result; i++)
	{
		actualPushedPathComponents.pop_back();
	}
	rebuildTableFromJSON(true);
	return result;
}
