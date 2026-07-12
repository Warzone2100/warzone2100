/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file
 *  Controller layout overlay screen.
 */

#include "gamepadlayoutscreen.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/gamepad_input.h"
#include "lib/widget/widget.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/multibutform.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"

#include "../display.h"
#include "../input/manager.h"
#include "../input/keyconfig.h"
#include "../warzoneconfig.h"
#include "../loop.h"

#include <limits>
#include <cstring>
#include <algorithm>
#include <map>
#include <utility>

static std::shared_ptr<W_SCREEN> gamepadLayoutScreen = nullptr;

// glyphs draw at this logical size, rendered at double resolution for crispness
static const int GLYPH_DRAW_SIZE = 28;
static const int ROW_HEIGHT = 32;
static const int ROW_SPACING = 2;
static const int ZONE_TITLE_HEIGHT = 20;
static const int ZONE_FOOTNOTE_SPACING = 4;
static const int PANEL_PADDING = 20;
// Zones lay out in three columns when the screen is wide enough, mirroring the
// pad's left / center / right regions, and stack vertically otherwise - the
// column width follows the widest label, within these bounds
static const int ZONE_COLUMN_MIN_WIDTH = 220;
static const int ZONE_COLUMN_MAX_WIDTH = 460;
static const int ZONE_SPACING = 10;
static const int BAND_SPACING = 8;
// minimum gap between the header title and the layer tabs, and the space
// separating the header (title and controller name) from the content below
static const int HEADER_TABS_GAP = 24;
static const int HEADER_BOTTOM_SPACING = 24;
// room for a glyph before each row's label
static const int ROW_TEXT_INDENT = GLYPH_DRAW_SIZE + 8;

// Which meta layer of bindings is displayed - no modifier, or a held shoulder
enum class MetaLayer
{
	NONE = 0,
	LB = 1,
	RB = 2
};

enum class BindingState
{
	BOUND,
	INHERITED,
	NOT_BOUND
};

// What a row shows - marked labels get a trailing asterisk referring to the
// zone's footnote
struct BindingLabel
{
	WzString text;
	bool footnoteMark = false;
};

// Labels keyed by (meta layer, button)
typedef std::map<std::pair<int, int>, BindingLabel> BindingLabels;

static void setBindingLabel(BindingLabels& labels, MetaLayer layer, GAMEPAD_INPUT button, const WzString& label, bool footnoteMark = false)
{
	labels[{static_cast<int>(layer), static_cast<int>(button)}] = BindingLabel{label, footnoteMark};
}

// Collects what every displayed button does on each layer - the hardcoded core
// behaviors plus every live gamepad-slot mapping bucketed by its meta
static BindingLabels buildBindingLabels()
{
	BindingLabels labels;

	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_LEFT_SHOULDER, _("Modifier (hold)"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_LEFT_TRIGGER, _("Zoom Out"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_LSTICK_UP, _("Move Cursor"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_LEFT_STICK, _("Precision Cursor (hold)"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_BACK, _("Objectives / Alliances"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_START, _("Menu / Back"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_RIGHT_SHOULDER, _("Modifier (hold)"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_RIGHT_TRIGGER, _("Zoom In"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_RSTICK_UP, _("Pan Camera / Scroll"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_SOUTH, _("Left Click / Select"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_EAST, _("Right Click / Order"));
	setBindingLabel(labels, MetaLayer::NONE, GPAD_BTN_WEST, _("Confirm / Chat"));

	setBindingLabel(labels, MetaLayer::LB, GPAD_BTN_SOUTH, _("Add to Selection"));
	setBindingLabel(labels, MetaLayer::LB, GPAD_BTN_EAST, _("Queue Move / Order"));
	setBindingLabel(labels, MetaLayer::RB, GPAD_BTN_RSTICK_UP, _("Rotate / Pitch Camera"));

	// d-pad directions map to unit groups clockwise from up, the left shoulder
	// shifts to the second bank
	const GAMEPAD_INPUT dpadButtons[4] = {GPAD_BTN_DPAD_UP, GPAD_BTN_DPAD_RIGHT, GPAD_BTN_DPAD_DOWN, GPAD_BTN_DPAD_LEFT};
	for (unsigned int i = 0; i < 4; ++i)
	{
		setBindingLabel(labels, MetaLayer::NONE, dpadButtons[i], WzString::format(_("Group %u"), i + 1), true);
		setBindingLabel(labels, MetaLayer::LB, dpadButtons[i], WzString::format(_("Group %u"), i + 5), true);
	}

	for (const KeyFunctionInfo& info : gKeyFuncConfig.allKeyFunctionEntries())
	{
		if (info.type == KeyMappingType::HIDDEN)
		{
			continue;
		}
		const auto mapping = gInputManager.mappings().get(info, KeyMappingSlot::GAMEPAD);
		if (!mapping.has_value() || mapping->get().isInvalid())
		{
			continue;
		}
		const KeyCombination& keys = mapping->get().keys;
		const auto input = keys.input.asGamepadInput();
		if (!input.has_value())
		{
			continue;
		}
		MetaLayer layer = MetaLayer::NONE;
		if (const auto meta = keys.meta.asGamepadInput())
		{
			switch (meta.value())
			{
			case GPAD_BTN_LEFT_SHOULDER: layer = MetaLayer::LB; break;
			case GPAD_BTN_RIGHT_SHOULDER: layer = MetaLayer::RB; break;
			default: continue;
			}
		}
		setBindingLabel(labels, layer, input.value(), WzString::fromUtf8(_(info.displayName.c_str())));
	}

	return labels;
}

// What a button's row shows on a layer - the layer's own binding, the base
// behavior that persists under a held modifier, or nothing
static std::pair<BindingLabel, BindingState> resolveBinding(const BindingLabels& labels, MetaLayer layer, GAMEPAD_INPUT button)
{
	if (layer != MetaLayer::NONE)
	{
		const auto it = labels.find({static_cast<int>(layer), static_cast<int>(button)});
		if (it != labels.end())
		{
			return {it->second, BindingState::BOUND};
		}
	}
	const auto it = labels.find({static_cast<int>(MetaLayer::NONE), static_cast<int>(button)});
	if (it != labels.end())
	{
		return {it->second, (layer == MetaLayer::NONE) ? BindingState::BOUND : BindingState::INHERITED};
	}
	return {BindingLabel{_("Not bound"), false}, BindingState::NOT_BOUND};
}

static std::unique_ptr<gfx_api::texture> makeGlyphTexture(GAMEPAD_INPUT button)
{
	std::vector<unsigned char> rgba;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!gamepadGetButtonGlyph(button, GLYPH_DRAW_SIZE * 2, rgba, width, height))
	{
		return nullptr;
	}
	iV_Image image;
	if (!image.allocate(width, height, 4, false))
	{
		return nullptr;
	}
	memcpy(image.bmp_w(), rgba.data(), rgba.size());
	return std::unique_ptr<gfx_api::texture>(gfx_api::context::get().loadTextureFromUncompressedImage(std::move(image), gfx_api::texture_type::user_interface, "gamepadglyph"));
}

// One button's row - its glyph followed by what it does on the displayed
// layer. Buttons without glyph art fall back to their name in the text
class GamepadLayoutRow : public WIDGET
{
public:
	static std::shared_ptr<GamepadLayoutRow> make(GAMEPAD_INPUT button)
	{
		auto result = std::make_shared<GamepadLayoutRow>();
		result->rowButton = button;
		result->glyphTexture = makeGlyphTexture(button);
		result->setGeometry(0, 0, ZONE_COLUMN_MIN_WIDTH - ZONE_SPACING, ROW_HEIGHT);
		return result;
	}

	GAMEPAD_INPUT button() const
	{
		return rowButton;
	}

	void setBinding(const BindingLabel& label, BindingState state)
	{
		bindingState = state;
		hasFootnoteMark = label.footnoteMark;
		if (hasFootnoteMark && !footnoteMark)
		{
			footnoteMark = std::make_unique<WzCachedText>();
			footnoteMark->setText("*", font_regular);
		}
		WzString displayText = label.text;
		if (!glyphTexture)
		{
			displayText = WzString(gamepadButtonName(rowButton)) + " : " + label.text;
		}
		fullLabel = hasFootnoteMark ? displayText + "*" : displayText;
		text.setText(displayText, font_regular);
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		if (isPhysicallyPressed())
		{
			// highlight the row while its button is held, like pressing a
			// bound input on the options keybinding lists
			pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WZCOL_MENU_SCORE_BUILT);
		}
		PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
		PIELIGHT glyphColor = WZCOL_WHITE;
		switch (bindingState)
		{
		case BindingState::BOUND:
			break;
		case BindingState::INHERITED:
			textColor = WZCOL_TEXT_MEDIUM;
			break;
		case BindingState::NOT_BOUND:
			textColor = pal_RGBA(255, 255, 255, 80);
			glyphColor = pal_RGBA(255, 255, 255, 110);
			break;
		}
		if (glyphTexture)
		{
			const int glyphY = y0 + (height() - GLYPH_DRAW_SIZE) / 2;
			iV_DrawImageAnisotropic(*glyphTexture, Vector2i(x0, glyphY), Vector2f(0.f, 0.f), Vector2f(GLYPH_DRAW_SIZE, GLYPH_DRAW_SIZE), 0.f, glyphColor);
		}
		const int textX = x0 + ROW_TEXT_INDENT;
		const int textY = y0 + (height() - text->lineSize()) / 2 - text->aboveBase();
		int maxTextWidth = width() - ROW_TEXT_INDENT;
		if (hasFootnoteMark)
		{
			maxTextWidth -= (*footnoteMark)->width() + 2;
		}
		isTruncated = text->width() > maxTextWidth;
		if (isTruncated)
		{
			maxTextWidth -= (iV_GetEllipsisWidth(text.getFontID()) + 2);
		}
		text->render(textX, textY, textColor, 0.0f, maxTextWidth);
		int textEndX = textX + std::min(text->width(), maxTextWidth);
		if (isTruncated)
		{
			iV_DrawEllipsis(text.getFontID(), Vector2f(textX + maxTextWidth + 2, textY), textColor);
			textEndX = textX + maxTextWidth + 2 + iV_GetEllipsisWidth(text.getFontID());
		}
		if (hasFootnoteMark)
		{
			// the asterisk refers to the zone's footnote and shares its color
			(*footnoteMark)->render(textEndX + 2, textY, WZCOL_TEXT_MEDIUM);
		}
	}

	void run(W_CONTEXT *) override
	{
		text.tick();
		if (footnoteMark)
		{
			footnoteMark->tick();
		}
	}

	std::string getTip() override
	{
		if (!isTruncated)
		{
			return std::string();
		}
		return fullLabel.toUtf8();
	}

	int32_t idealHeight() override
	{
		return ROW_HEIGHT;
	}

private:
	bool isPhysicallyPressed() const
	{
		// the stick rows stand in for all four deflection directions
		switch (rowButton)
		{
		case GPAD_BTN_LSTICK_UP:
			return gamepadButtonPhysicallyDown(GPAD_BTN_LSTICK_UP) || gamepadButtonPhysicallyDown(GPAD_BTN_LSTICK_DOWN) || gamepadButtonPhysicallyDown(GPAD_BTN_LSTICK_LEFT) || gamepadButtonPhysicallyDown(GPAD_BTN_LSTICK_RIGHT);
		case GPAD_BTN_RSTICK_UP:
			return gamepadButtonPhysicallyDown(GPAD_BTN_RSTICK_UP) || gamepadButtonPhysicallyDown(GPAD_BTN_RSTICK_DOWN) || gamepadButtonPhysicallyDown(GPAD_BTN_RSTICK_LEFT) || gamepadButtonPhysicallyDown(GPAD_BTN_RSTICK_RIGHT);
		default:
			return gamepadButtonPhysicallyDown(rowButton);
		}
	}

	GAMEPAD_INPUT rowButton = GPAD_BTN_MAX;
	std::unique_ptr<gfx_api::texture> glyphTexture;
	WzCachedText text;
	std::unique_ptr<WzCachedText> footnoteMark;
	WzString fullLabel;
	BindingState bindingState = BindingState::NOT_BOUND;
	bool hasFootnoteMark = false;
	bool isTruncated = false;
};

// A titled cluster of button rows matching one physical region of the pad
class GamepadLayoutZone : public WIDGET
{
public:
	static std::shared_ptr<GamepadLayoutZone> make(const WzString& title, const std::vector<GAMEPAD_INPUT>& buttons, const WzString& footnote = WzString())
	{
		auto result = std::make_shared<GamepadLayoutZone>();
		result->titleLabel = std::make_shared<W_LABEL>();
		result->titleLabel->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
		result->titleLabel->setString(title);
		result->attach(result->titleLabel);
		for (GAMEPAD_INPUT button : buttons)
		{
			auto row = GamepadLayoutRow::make(button);
			result->attach(row);
			result->rows.push_back(row);
		}
		if (!footnote.isEmpty())
		{
			result->footnoteLabel = std::make_shared<W_LABEL>();
			result->footnoteLabel->setFont(font_small, WZCOL_TEXT_MEDIUM);
			result->footnoteLabel->setString(footnote);
			result->footnoteLabel->setCanTruncate(true);
			result->attach(result->footnoteLabel);
		}
		result->setGeometry(0, 0, ZONE_COLUMN_MIN_WIDTH - ZONE_SPACING, result->idealHeight());
		return result;
	}

	void updateBindings(const BindingLabels& labels, MetaLayer layer)
	{
		for (const auto& row : rows)
		{
			const auto resolved = resolveBinding(labels, layer, row->button());
			row->setBinding(resolved.first, resolved.second);
		}
	}

	// a partially scrolled zone still draws the rows that are fully in view
	void displayRecursive(WidgetGraphicsContext const& context) override
	{
		WIDGET::displayRecursive(context.setAllowChildDisplayRecursiveIfSelfClipped(true));
	}

	// scrolling steps by the title and individual rows, not the whole zone
	nonstd::optional<std::vector<uint32_t>> getScrollSnapOffsets() override
	{
		if (snapOffsetsDirty)
		{
			snapOffsetsDirty = false;
			snapOffsets.clear();
			snapOffsets.push_back(0);
			for (const auto& row : rows)
			{
				snapOffsets.push_back(static_cast<uint32_t>(row->y()));
			}
			if (footnoteLabel)
			{
				snapOffsets.push_back(static_cast<uint32_t>(footnoteLabel->y()));
			}
		}
		return snapOffsets;
	}

	void geometryChanged() override
	{
		int y0 = 0;
		titleLabel->setGeometry(0, y0, width(), ZONE_TITLE_HEIGHT);
		y0 += ZONE_TITLE_HEIGHT + ROW_SPACING;
		for (const auto& row : rows)
		{
			row->setGeometry(0, y0, width(), ROW_HEIGHT);
			y0 += ROW_HEIGHT + ROW_SPACING;
		}
		if (footnoteLabel)
		{
			footnoteLabel->setGeometry(0, y0 + ZONE_FOOTNOTE_SPACING, width(), footnoteLabel->idealHeight());
		}
		snapOffsetsDirty = true;
	}

	int32_t idealHeight() override
	{
		return ZONE_TITLE_HEIGHT + ROW_SPACING + static_cast<int32_t>(rows.size()) * (ROW_HEIGHT + ROW_SPACING) + (footnoteLabel ? ZONE_FOOTNOTE_SPACING + footnoteLabel->idealHeight() + ZONE_FOOTNOTE_SPACING : 0);
	}

private:
	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<W_LABEL> footnoteLabel;
	std::vector<std::shared_ptr<GamepadLayoutRow>> rows;
	std::vector<uint32_t> snapOffsets;
	bool snapOffsetsDirty = true;
};

// A horizontal band of zones at fixed column positions - a column can stay
// empty, leaving room for a future controller outline in the middle
class GamepadLayoutBand : public WIDGET
{
public:
	static std::shared_ptr<GamepadLayoutBand> make(const std::vector<std::pair<std::shared_ptr<WIDGET>, int>>& items, int columnWidth)
	{
		auto result = std::make_shared<GamepadLayoutBand>();
		result->items = items;
		int maxHeight = 0;
		for (const auto& item : items)
		{
			result->attach(item.first);
			result->numColumns = std::max(result->numColumns, item.second + 1);
			maxHeight = std::max(maxHeight, item.first->idealHeight());
		}
		result->setGeometry(0, 0, result->numColumns * columnWidth, maxHeight);
		return result;
	}

	// the list resizes bands to its inner width, e.g. when its scrollbar
	// appears, so the columns follow the actual width
	void geometryChanged() override
	{
		const int columnWidth = (width() + ZONE_SPACING) / std::max(numColumns, 1);
		for (const auto& item : items)
		{
			item.first->setGeometry(item.second * columnWidth, 0, columnWidth - ZONE_SPACING, item.first->idealHeight());
		}
		snapOffsetsDirty = true;
	}

	// a partially scrolled band still draws its zones' visible rows
	void displayRecursive(WidgetGraphicsContext const& context) override
	{
		WIDGET::displayRecursive(context.setAllowChildDisplayRecursiveIfSelfClipped(true));
	}

	// the merged row positions across the band's zones, so scrolling steps
	// by row here too
	nonstd::optional<std::vector<uint32_t>> getScrollSnapOffsets() override
	{
		if (snapOffsetsDirty)
		{
			snapOffsetsDirty = false;
			snapOffsets.clear();
			for (const auto& item : items)
			{
				const auto childOffsets = item.first->getScrollSnapOffsets().value_or(std::vector<uint32_t>{0});
				for (uint32_t childOffset : childOffsets)
				{
					snapOffsets.push_back(static_cast<uint32_t>(item.first->y()) + childOffset);
				}
			}
			std::sort(snapOffsets.begin(), snapOffsets.end());
			snapOffsets.erase(std::unique(snapOffsets.begin(), snapOffsets.end()), snapOffsets.end());
		}
		return snapOffsets;
	}

private:
	std::vector<std::pair<std::shared_ptr<WIDGET>, int>> items;
	std::vector<uint32_t> snapOffsets;
	int numColumns = 1;
	bool snapOffsetsDirty = true;
};

// MultichoiceWidget draws a menu-style background box, unwanted behind the tabs
class GamepadLayerTabs : public MultichoiceWidget
{
public:
	GamepadLayerTabs(int value = -1) : MultichoiceWidget(value) {}
	void display(int, int) override {}
};

class GamepadLayoutPanel : public WIDGET
{
public:
	static std::shared_ptr<GamepadLayoutPanel> make()
	{
		auto result = std::make_shared<GamepadLayoutPanel>();
		result->initialize();
		return result;
	}

	void display(int xOffset, int yOffset) override
	{
		int x0 = x() + xOffset;
		int y0 = y() + yOffset;
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(5, 12, 28, 235));
		iV_Box(x0, y0, x0 + width(), y0 + height(), pal_RGBA(255, 255, 255, 60));
	}

	void run(W_CONTEXT *) override
	{
		// physically holding a shoulder previews its layer, snapping back to
		// the pinned tab on release
		MetaLayer effective = pinnedLayer;
		if (gamepadButtonPhysicallyDown(GPAD_BTN_LEFT_SHOULDER))
		{
			effective = MetaLayer::LB;
		}
		else if (gamepadButtonPhysicallyDown(GPAD_BTN_RIGHT_SHOULDER))
		{
			effective = MetaLayer::RB;
		}
		if (effective != displayedLayer)
		{
			applyLayer(effective);
		}
	}

	void geometryChanged() override
	{
		if (width() == 0 || height() == 0)
		{
			return;
		}
		int y0 = PANEL_PADDING;
		// the tabs share the title row, right-aligned
		const int tabsWidth = std::min(layerTabs->idealWidth(), width() / 2);
		layerTabs->setGeometry(width() - PANEL_PADDING - tabsWidth, y0, tabsWidth, layerTabs->idealHeight());
		titleLabel->setGeometry(PANEL_PADDING, y0, width() - PANEL_PADDING * 2 - tabsWidth - HEADER_TABS_GAP, titleLabel->idealHeight());
		y0 += std::max(titleLabel->height(), layerTabs->height()) + 4;
		if (deviceLabel)
		{
			deviceLabel->setGeometry(PANEL_PADDING, y0, width() - PANEL_PADDING * 2, deviceLabel->idealHeight());
			y0 += deviceLabel->height();
		}
		y0 += HEADER_BOTTOM_SPACING;
		const int footerHeight = footerLabel->idealHeight() + PANEL_PADDING;
		zonesList->setGeometry(PANEL_PADDING, y0, width() - PANEL_PADDING * 2, height() - y0 - footerHeight - 8);
		footerLabel->setGeometry(PANEL_PADDING, height() - footerHeight, width() - PANEL_PADDING * 2, footerLabel->idealHeight());
	}

	int32_t idealWidth() override
	{
		// the zone columns without the last column's trailing spacing, plus
		// room for the list's scrollbar - never narrower than the title row
		const int32_t zonesWidth = PANEL_PADDING * 2 + (wideMode ? 3 : 1) * columnWidth - ZONE_SPACING + scrollbarAllowance();
		const int32_t headerWidth = PANEL_PADDING * 2 + titleLabel->idealWidth() + HEADER_TABS_GAP + layerTabs->idealWidth();
		return std::max(zonesWidth, headerWidth);
	}

	int32_t idealHeight() override
	{
		return PANEL_PADDING * 2 + std::max(titleLabel->idealHeight(), layerTabs->idealHeight()) + 4 + (deviceLabel ? deviceLabel->idealHeight() : 0) + HEADER_BOTTOM_SPACING + contentHeight + footerLabel->idealHeight() + PANEL_PADDING + 8;
	}

	void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override
	{
		WIDGET::screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
		const bool newWideMode = wideModeForScreenWidth(newWidth);
		const int newColumnWidth = columnWidthForScreen(newWideMode, newWidth);
		if (newWideMode != wideMode || newColumnWidth != columnWidth)
		{
			wideMode = newWideMode;
			columnWidth = newColumnWidth;
			rebuildZones();
			callCalcLayout();
		}
	}

private:
	// the widest label across every layer sizes the columns, so geometry
	// stays put when switching layers
	int computeContentColumnWidth() const
	{
		int maxLabelWidth = static_cast<int>(iV_GetTextWidth(_("Not bound"), font_regular));
		const int markWidth = static_cast<int>(iV_GetTextWidth("*", font_regular)) + 2;
		for (const auto& entry : bindingLabels)
		{
			int labelWidth = static_cast<int>(iV_GetTextWidth(entry.second.text, font_regular));
			if (entry.second.footnoteMark)
			{
				labelWidth += markWidth;
			}
			maxLabelWidth = std::max(maxLabelWidth, labelWidth);
		}
		return std::clamp(ROW_TEXT_INDENT + maxLabelWidth + ZONE_SPACING, ZONE_COLUMN_MIN_WIDTH, ZONE_COLUMN_MAX_WIDTH);
	}

	int scrollbarAllowance() const
	{
		return zonesList ? zonesList->getScrollbarWidth() : 0;
	}

	bool wideModeForScreenWidth(int forScreenWidth) const
	{
		return forScreenWidth - 40 >= PANEL_PADDING * 2 + 3 * contentColumnWidth - ZONE_SPACING + scrollbarAllowance();
	}

	// stacked zones use the width the screen affords, up to the content width
	int columnWidthForScreen(bool wide, int forScreenWidth) const
	{
		if (wide)
		{
			return contentColumnWidth;
		}
		const int available = forScreenWidth - 40 - PANEL_PADDING * 2 - scrollbarAllowance();
		return std::min(contentColumnWidth, std::max(available, ZONE_COLUMN_MIN_WIDTH));
	}

	void initialize()
	{
		bindingLabels = buildBindingLabels();
		contentColumnWidth = computeContentColumnWidth();

		titleLabel = std::make_shared<W_LABEL>();
		titleLabel->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
		titleLabel->setString(_("Controller Layout"));
		attach(titleLabel);

		const WzString deviceName = WzString::fromUtf8(gamepadDeviceName());
		if (!deviceName.isEmpty())
		{
			deviceLabel = std::make_shared<W_LABEL>();
			deviceLabel->setFont(font_regular, WZCOL_TEXT_MEDIUM);
			deviceLabel->setString(deviceName);
			deviceLabel->setCanTruncate(true);
			attach(deviceLabel);
		}

		layerTabs = std::make_shared<GamepadLayerTabs>(0);
		layerTabs->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
		layerTabs->addButton(static_cast<int>(MetaLayer::NONE), WzPanelTabButton::make(_("Normal")));
		layerTabs->addButton(static_cast<int>(MetaLayer::LB), WzPanelTabButton::make(gamepadButtonName(GPAD_BTN_LEFT_SHOULDER)));
		layerTabs->addButton(static_cast<int>(MetaLayer::RB), WzPanelTabButton::make(gamepadButtonName(GPAD_BTN_RIGHT_SHOULDER)));
		layerTabs->choose(static_cast<int>(MetaLayer::NONE));
		layerTabs->addOnChooseHandler([this](MultibuttonWidget&, int newValue) {
			// choose() also fires this when applyLayer syncs the tabs, and a
			// shoulder-held preview must not re-pin its own layer
			if (syncingTabs)
			{
				return;
			}
			pinnedLayer = static_cast<MetaLayer>(newValue);
		});
		attach(layerTabs);

		zonesList = ScrollableListWidget::make();
		zonesList->setItemSpacing(BAND_SPACING);
		attach(zonesList);
		wideMode = wideModeForScreenWidth(static_cast<int>(screenWidth));
		columnWidth = columnWidthForScreen(wideMode, static_cast<int>(screenWidth));
		rebuildZones();

		footerLabel = std::make_shared<W_LABEL>();
		footerLabel->setFont(font_small, WZCOL_TEXT_MEDIUM);
		footerLabel->setString(_("Hold LB or RB to preview. Click, or press Start, to close"));
		attach(footerLabel);
	}

	void applyLayer(MetaLayer layer)
	{
		displayedLayer = layer;
		for (const auto& zone : zones)
		{
			zone->updateBindings(bindingLabels, layer);
		}
		syncingTabs = true;
		layerTabs->choose(static_cast<int>(layer));
		syncingTabs = false;
	}

	void addListItem(const std::shared_ptr<WIDGET>& item)
	{
		if (contentHeight > 0)
		{
			contentHeight += BAND_SPACING;
		}
		contentHeight += item->height();
		zonesList->addItem(item);
	}

	// builds the zones mirroring the pad's regions and lays them out in a
	// left / center / right grid, or a vertical stack on narrow screens
	void rebuildZones()
	{
		zonesList->clear();
		zones.clear();
		contentHeight = 0;

		// zones mirror the physical pad, so with swapped sticks each side
		// lists the logical stick that physically sits there - glyphs
		// already map logical to physical, and the binding text follows
		// the logical input
		const bool swapSticks = war_GetGamepadSwapSticks();
		const GAMEPAD_INPUT leftMove = swapSticks ? GPAD_BTN_RSTICK_UP : GPAD_BTN_LSTICK_UP;
		const GAMEPAD_INPUT leftPress = swapSticks ? GPAD_BTN_RIGHT_STICK : GPAD_BTN_LEFT_STICK;
		const GAMEPAD_INPUT rightMove = swapSticks ? GPAD_BTN_LSTICK_UP : GPAD_BTN_RSTICK_UP;
		const GAMEPAD_INPUT rightPress = swapSticks ? GPAD_BTN_LEFT_STICK : GPAD_BTN_RIGHT_STICK;
		auto leftZone = GamepadLayoutZone::make(_("Left Side"), {GPAD_BTN_LEFT_SHOULDER, GPAD_BTN_LEFT_TRIGGER, leftMove, leftPress});
		auto centerZone = GamepadLayoutZone::make(_("Center"), {GPAD_BTN_BACK, GPAD_BTN_START});
		auto rightZone = GamepadLayoutZone::make(_("Right Side"), {GPAD_BTN_RIGHT_SHOULDER, GPAD_BTN_RIGHT_TRIGGER, rightMove, rightPress});
		auto dpadZone = GamepadLayoutZone::make(_("D-Pad"), {GPAD_BTN_DPAD_UP, GPAD_BTN_DPAD_RIGHT, GPAD_BTN_DPAD_DOWN, GPAD_BTN_DPAD_LEFT}, WzString("* ") + _("tap: recall, hold: assign"));
		auto faceZone = GamepadLayoutZone::make(_("Face Buttons"), {GPAD_BTN_SOUTH, GPAD_BTN_EAST, GPAD_BTN_WEST, GPAD_BTN_NORTH});
		zones = {leftZone, centerZone, rightZone, dpadZone, faceZone};

		if (wideMode)
		{
			addListItem(GamepadLayoutBand::make({{leftZone, 0}, {centerZone, 1}, {rightZone, 2}}, columnWidth));
			addListItem(GamepadLayoutBand::make({{dpadZone, 0}, {faceZone, 2}}, columnWidth));
		}
		else
		{
			// stacked zones go in the list directly so they track its inner width
			for (const auto& zone : zones)
			{
				addListItem(zone);
			}
		}

		applyLayer(displayedLayer);
	}

	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<W_LABEL> deviceLabel;
	std::shared_ptr<MultichoiceWidget> layerTabs;
	std::shared_ptr<ScrollableListWidget> zonesList;
	std::shared_ptr<W_LABEL> footerLabel;
	std::vector<std::shared_ptr<GamepadLayoutZone>> zones;
	BindingLabels bindingLabels;
	MetaLayer pinnedLayer = MetaLayer::NONE;
	MetaLayer displayedLayer = MetaLayer::NONE;
	bool syncingTabs = false;
	int contentHeight = 0;
	int contentColumnWidth = ZONE_COLUMN_MIN_WIDTH;
	int columnWidth = ZONE_COLUMN_MIN_WIDTH;
	bool wideMode = true;
};

void showGamepadLayoutScreen()
{
	if (gamepadLayoutScreen)
	{
		return;
	}

	gamepadLayoutScreen = W_SCREEN::make();
	auto rootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
	rootFrm->onClickedFunc = []() { closeGamepadLayoutScreen(); };
	rootFrm->onCancelPressed = rootFrm->onClickedFunc;
	gamepadLayoutScreen->psForm->attach(rootFrm);

	auto panel = GamepadLayoutPanel::make();
	panel->setCalcLayout([](WIDGET *psWidget) {
		auto psPanel = std::static_pointer_cast<GamepadLayoutPanel>(psWidget->shared_from_this());
		const int w = std::min<int>(psPanel->idealWidth(), screenWidth - 40);
		const int h = std::min<int>(psPanel->idealHeight(), screenHeight - 40);
		psWidget->setGeometry((screenWidth - w) / 2, (screenHeight - h) / 2, w, h);
	});
	rootFrm->attach(panel);

	widgRegisterOverlayScreen(gamepadLayoutScreen, std::numeric_limits<uint16_t>::max() - 1);
}

void closeGamepadLayoutScreen()
{
	if (!gamepadLayoutScreen)
	{
		return;
	}
	widgRemoveOverlayScreen(gamepadLayoutScreen);
	gamepadLayoutScreen = nullptr;
}

bool isGamepadLayoutScreenUp()
{
	return gamepadLayoutScreen != nullptr;
}

void gamepadLayoutMaybeAutoShow()
{
	if (!war_GetGamepadShowLayoutOnConnect() || !gamepadIsConnected() || isGamepadLayoutScreenUp())
	{
		return;
	}
	if (loop_GetVideoStatus())
	{
		return;
	}
	// the keymap-driven labels are empty until the initial mappings load runs
	// during frontend init, so hold off until then
	if (gInputManager.cmappings().empty())
	{
		return;
	}
	const char* deviceGUID = gamepadDeviceGUID();
	if (deviceGUID[0] == '\0')
	{
		return;
	}
	// once a device has been handled skip the seen-list copy and search below
	static std::string handledGUID;
	if (handledGUID == deviceGUID)
	{
		return;
	}
	const std::string& seen = war_GetGamepadLayoutSeenDevices();
	if (seen.find(deviceGUID) != std::string::npos)
	{
		handledGUID = deviceGUID;
		return;
	}
	// remember a bounded number of device models, dropping the oldest
	std::string updated = seen.empty() ? deviceGUID : seen + "," + deviceGUID;
	while (updated.size() > 512)
	{
		const size_t comma = updated.find(',');
		if (comma == std::string::npos)
		{
			break;
		}
		updated.erase(0, comma + 1);
	}
	war_SetGamepadLayoutSeenDevices(updated);
	handledGUID = deviceGUID;
	showGamepadLayoutScreen();
}
