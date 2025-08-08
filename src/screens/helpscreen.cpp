/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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
 *  Functionality for the help screen overlay
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "helpscreen.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/margin.h"
#include "lib/widget/alignment.h"
#include "lib/widget/scrollablelist.h"
#include "lib/widget/button.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

#include "../intimage.h"
#include "../display.h"

#include <array>

const PIELIGHT WZCOL_HELP_UI_BACKGROUND = pal_RGBA(5,9,235,190);
const PIELIGHT WZCOL_HELP_UI_BACKGROUND_DARK = pal_RGBA(2,5,173,255);
const PIELIGHT WZCOL_HELP_UI_BORDER_LIGHT = pal_RGBA(255, 255, 255, 80);

// MARK: - Custom Close Help UI button

struct HelpOverlayCloseButton : public W_BUTTON
{
	HelpOverlayCloseButton() : W_BUTTON() {}

	void display(int xOffset, int yOffset) override;

	unsigned downStateMask = WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK;
	unsigned greyStateMask = WBUT_DISABLE;
private:
	WzText text;
};

void HelpOverlayCloseButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	// Display the button.
	auto light_border = WZCOL_HELP_UI_BORDER_LIGHT;
	auto fill_color = isDown || isDisabled ? WZCOL_HELP_UI_BACKGROUND_DARK : WZCOL_HELP_UI_BACKGROUND;
	iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0,0,0,0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, fill_color);
	if (isHighlight)
	{
		iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
	}

	if (haveText)
	{
		text.setText(pText, FontID);
		int fw = text.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - text.lineSize()) / 2 - text.aboveBase();
		PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
		if (isDisabled)
		{
			textColor.byte.a = (textColor.byte.a / 2);
		}
		text.render(fx, fy, textColor);
	}
}

// MARK: - Help overlay screen

std::shared_ptr<W_HELP_OVERLAY_SCREEN> W_HELP_OVERLAY_SCREEN::make(const OnCloseFunc& _onCloseFunc)
{
	class make_shared_enabler: public W_HELP_OVERLAY_SCREEN {};
	auto newRootFrm = W_HELPSCREEN_CLICKFORM::make();
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	screen->onCloseFunc = _onCloseFunc;
	screen->rootHelpScreenForm = newRootFrm;

	std::weak_ptr<W_HELP_OVERLAY_SCREEN> psWeakHelpOverlayScreen(screen);
	newRootFrm->onClickedFunc = [psWeakHelpOverlayScreen]() {
		auto psOverlayScreen = psWeakHelpOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeHelpOverlayScreen();
		}
	};
	newRootFrm->onCancelPressed = newRootFrm->onClickedFunc;

	// Add "Close Help Mode" button
	#define CloseHelpPadding 12
	#define CloseHelpMargin 5
	auto button = std::make_shared<HelpOverlayCloseButton>();
	button->setString(_("Close Help Mode"));
	button->FontID = font_regular;
	int minButtonWidthForText = iV_GetTextWidth(button->pText, button->FontID);
	button->setCalcLayout([minButtonWidthForText](WIDGET *psWidget) {
		int width = minButtonWidthForText + CloseHelpPadding;
		int height = iV_GetTextLineSize(font_regular) + CloseHelpPadding;
		psWidget->setGeometry((screenWidth - width) / 2, CloseHelpMargin, width, height);
	});
	button->addOnClickHandler([psWeakHelpOverlayScreen](W_BUTTON& button) {
		auto psOverlayScreen = psWeakHelpOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeHelpOverlayScreen();
		}
	});
	newRootFrm->attach(button);

	return screen;
}

void W_HELP_OVERLAY_SCREEN::closeHelpOverlayScreen()
{
	widgRemoveOverlayScreen(shared_from_this());
	if (onCloseFunc)
	{
		onCloseFunc(std::dynamic_pointer_cast<W_HELP_OVERLAY_SCREEN>(shared_from_this()));
	}
}

void W_HELP_OVERLAY_SCREEN::setHelpFromWidgets(const std::shared_ptr<WIDGET>& root)
{
	ASSERT_OR_RETURN(, root != nullptr, "Null widget?");
	// (Enumerate the child widgets in decreasing z-order (i.e. "top-down"))
	for (auto i = root->children().size(); i--;)
	{
		auto const &child = root->children()[i];
		if (!child->visible())
		{
			continue;
		}
		setHelpFromWidgets(child);
		// Use the widget's help, if available
		const auto pHelp = child->getHelp();
		if (pHelp)
		{
			setHelpForWidget(child, *pHelp);
		}
		else
		{
			// Try the widget tooltip, if available
			auto childTipStr = child->getTip();
			if (childTipStr.empty())
			{
				continue;
			}
			// Parse the tip
			// - First line as title
			// - Additional lines as description
			auto firstLinePos = childTipStr.find('\n');
			if (firstLinePos != std::string::npos)
			{
				auto generatedHelp = WidgetHelp().setTitle(WzString::fromUtf8(childTipStr.substr(0, firstLinePos)));
				if ((firstLinePos + 1) < childTipStr.size())
				{
					generatedHelp.setDescription(WzString::fromUtf8(childTipStr.substr(firstLinePos + 1)));
				}
				setHelpForWidget(child, generatedHelp);
			}
			else
			{
				setHelpForWidget(child, WidgetHelp().setTitle(WzString::fromUtf8(childTipStr)));
			}
		}
	}
}

void W_HELP_OVERLAY_SCREEN::setHelpForWidget(const std::shared_ptr<WIDGET>& widget, const WidgetHelp& help)
{
	rootHelpScreenForm->setHelpForWidget(widget, help);
}

void W_HELP_OVERLAY_SCREEN::removeHelpForWidget(const std::shared_ptr<WIDGET>& widget)
{
	rootHelpScreenForm->removeHelpForWidget(widget);
}

const WidgetHelp* W_HELP_OVERLAY_SCREEN::getHelpForWidget(const std::shared_ptr<WIDGET>& widget)
{
	return rootHelpScreenForm->getHelpForWidget(widget);
}

// MARK: - HelpDisplayForm

std::string to_string(const WidgetHelp::InteractionTriggers& trigger)
{
	switch (trigger)
	{
		case WidgetHelp::InteractionTriggers::PrimaryClick:
			return _("Click/Tap");
		case WidgetHelp::InteractionTriggers::SecondaryClick:
			return _("Right-Click");
		case WidgetHelp::InteractionTriggers::ClickAndHold:
			return _("Click/Tap and Hold");
		case WidgetHelp::InteractionTriggers::Misc:
			return _("Other");
	}
	return ""; // silence warning
}

std::string to_string(const std::set<WidgetHelp::InteractionTriggers>& triggers)
{
	std::string result;
	for (auto& trigger : triggers)
	{
		if (!result.empty())
		{
			result.append("\n");
		}
		result.append(to_string(trigger));
	}
	return result;
}

class HelpDisplayForm: public W_FORM
{
private:
	static constexpr int32_t INTERNAL_PADDING = 15;
	static constexpr int32_t MaxWidth = 450;
protected:
	HelpDisplayForm() { }
	void initialize(const WidgetHelp& help)
	{
		// Create title (bold)
		title = std::make_shared<W_LABEL>();
		title->setFont(font_regular_bold, WZCOL_TEXT_BRIGHT);
		title->setString(help.title);
		attach(title);

		// Create description (paragraph)
		if (!help.description.isEmpty())
		{
			description = std::make_shared<Paragraph>();
			description->setFontColour(WZCOL_TEXT_BRIGHT);
			description->setLineSpacing(2);
			description->setFont(font_regular);
			description->addText(help.description);
			attach(description);
		}

		// Create a list of interactions
		if (!help.interactions.empty())
		{
			auto grid = std::make_shared<GridLayout>();
			grid_allocation::slot row(0);

			auto interactions_label = std::make_shared<W_LABEL>();
			interactions_label->setFont(font_regular, WZCOL_TEXT_MEDIUM);
			interactions_label->setString(_("Interactions:"));
			interactions_label->setGeometry(0, 0, interactions_label->getMaxLineWidth(), interactions_label->requiredHeight());
			grid->place({0, 2}, row, Margin(5, 0).wrap(interactions_label));
			row.start++;

			for (const auto& interaction : help.interactions)
			{
				auto interaction_type = std::make_shared<Paragraph>();
				interaction_type->setFontColour(WZCOL_TEXT_BRIGHT);
				interaction_type->setLineSpacing(0);
				interaction_type->setFont(font_regular_bold);
				interaction_type->addText(WzString::fromUtf8(to_string(interaction.triggers)));
				interaction_type->setGeometry(0, 0, (MaxWidth / 2) - (5*2), 0);

				auto interaction_effect = std::make_shared<Paragraph>();
				interaction_effect->setFontColour(WZCOL_TEXT_BRIGHT);
				interaction_effect->setLineSpacing(0);
				interaction_effect->setFont(font_regular);
				interaction_effect->addText(interaction.description);
				interaction_effect->setGeometry(0, 0, MaxWidth / 2, 0);

				grid->place({0}, row, Margin(5, 5).wrap(interaction_type));
				grid->place({1, 1, false}, row, Alignment(Alignment::Vertical::Center, Alignment::Horizontal::Left).wrap(Margin(5, 5).wrap(interaction_effect)));
				row.start++;
			}

			grid->setGeometry(0, 0, MaxWidth, grid->idealHeight());

			interactions = ScrollableListWidget::make();
			interactions->addItem(grid);
			attach(interactions);
		}

		buildRelatedKeybindingsList(help);

		recalcLayout();
	}

#if !defined(__clang__) && defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

	void buildRelatedKeybindingsList(const WidgetHelp& help)
	{
		if (help.relatedKeybindings.empty())
		{
			return;
		}

		auto grid = std::make_shared<GridLayout>();
		grid_allocation::slot row(0);

		auto keys_label = std::make_shared<W_LABEL>();
		keys_label->setFont(font_regular, WZCOL_TEXT_MEDIUM);
		keys_label->setString(_("Related Keymappings:"));
		keys_label->setGeometry(0, 0, keys_label->getMaxLineWidth(), keys_label->requiredHeight());
		grid->place({0, 2}, row, Margin(5, 0).wrap(keys_label));
		row.start++;

		for (const auto& keybinding : help.relatedKeybindings)
		{
			auto primary_mapping = gInputManager.mappings().get(keybinding.keybindingName, KeyMappingSlot::PRIMARY);
			auto secondary_mapping = gInputManager.mappings().get(keybinding.keybindingName, KeyMappingSlot::SECONDARY);
			WzString keybindingsStr;
			WzString keyDisplayName;
			if (primary_mapping.has_value())
			{
				if (!keybindingsStr.isEmpty()) { keybindingsStr.append("\n"); }
				keybindingsStr.append(WzString::fromUtf8(primary_mapping.value().get().toString()));
				keyDisplayName = WzString::fromUtf8(primary_mapping.value().get().info.displayName);
			}
			if (secondary_mapping.has_value())
			{
				if (!keybindingsStr.isEmpty()) { keybindingsStr.append("\n"); }
				keybindingsStr.append(WzString::fromUtf8(secondary_mapping.value().get().toString()));
				if (keyDisplayName.isEmpty())
				{
					keyDisplayName = WzString::fromUtf8(secondary_mapping.value().get().info.displayName);
				}
			}
			if (keybindingsStr.isEmpty() || keyDisplayName.isEmpty())
			{
				continue;
			}
			auto interaction_type = std::make_shared<W_LABEL>();
			interaction_type->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
			interaction_type->setString(keyDisplayName);
			interaction_type->setGeometry(0, 0, std::min(interaction_type->getMaxLineWidth(), MaxWidth / 2), interaction_type->requiredHeight());

			auto interaction_effect = std::make_shared<Paragraph>();
			interaction_effect->setFontColour(WZCOL_TEXT_MEDIUM);
			interaction_effect->setLineSpacing(0);
			interaction_effect->setFont(font_regular);
			interaction_effect->addText(keybindingsStr);
			interaction_effect->setGeometry(0, 0, MaxWidth / 2, 0);

			grid->place({0}, row, Margin(2, 5).wrap(interaction_type));
			grid->place({1, 1, false}, row, Alignment(Alignment::Vertical::Center, Alignment::Horizontal::Left).wrap(Margin(2, 5).wrap(interaction_effect)));
			row.start++;
		}

		grid->setGeometry(0, 0, MaxWidth, grid->idealHeight());

		relatedKeybindings = ScrollableListWidget::make();
		relatedKeybindings->addItem(grid);
		attach(relatedKeybindings);
	}

#if !defined(__clang__) && defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#pragma GCC diagnostic pop
#endif

public:
	static std::shared_ptr<HelpDisplayForm> make(const WidgetHelp& help)
	{
		class make_shared_enabler: public HelpDisplayForm {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(help);
		return widget;
	}
public:
	void display(int xOffset, int yOffset) override
	{
		if ((style & WFORM_INVISIBLE) == 0)
		{
			int x0 = x() + xOffset;
			int y0 = y() + yOffset;
			int x1 = x0 + width();
			int y1 = y0 + height();

			pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_HELP_UI_BACKGROUND);
			iV_Box(x0+1, y0+1, x1+1, y1+1, WZCOL_HELP_UI_BACKGROUND_DARK);
		}
	}
	void geometryChanged() override
	{
		recalcLayout();
	}
	int32_t idealWidth() override
	{
		if (description || interactions)
		{
			return MaxWidth;
		}
		else
		{
			// only title
			return title->getMaxLineWidth() + (INTERNAL_PADDING * 2);
		}
	}
	int32_t idealHeight() override
	{
		int32_t bottomOfLastElement = 0;
		if (relatedKeybindings)
		{
			bottomOfLastElement = relatedKeybindings->geometry().bottom();
		}
		else if (interactions)
		{
			bottomOfLastElement = interactions->geometry().bottom();
		}
		else if (description)
		{
			bottomOfLastElement = description->geometry().bottom();
		}
		else if (title)
		{
			bottomOfLastElement = INTERNAL_PADDING + title->requiredHeight();
		}
		return bottomOfLastElement + INTERNAL_PADDING;
	}
private:
	void recalcLayout()
	{
		int maxChildWidth = width() - (INTERNAL_PADDING * 2);
		const int contentsPadding = INTERNAL_PADDING;

		title->setGeometry(INTERNAL_PADDING, INTERNAL_PADDING, std::min(title->getMaxLineWidth(), maxChildWidth), title->requiredHeight());

		int bottomOfLastElement = title->y() + title->height();

		if (description)
		{
			int descriptionY = bottomOfLastElement + contentsPadding;
			description->setGeometry(INTERNAL_PADDING, descriptionY, maxChildWidth, description->height());
			bottomOfLastElement = description->y() + description->height();
		}

		if (interactions)
		{
			int interactionsY = bottomOfLastElement + contentsPadding;
			interactions->setGeometry(INTERNAL_PADDING, interactionsY, maxChildWidth, interactions->idealHeight());
			interactions->setGeometry(INTERNAL_PADDING, interactionsY, maxChildWidth, interactions->idealHeight()); // call again using calculated height from width change
			bottomOfLastElement = interactions->y() + interactions->height();
		}

		if (relatedKeybindings)
		{
			int relatedKeybindingsY = bottomOfLastElement + contentsPadding;
			relatedKeybindings->setGeometry(INTERNAL_PADDING, relatedKeybindingsY, maxChildWidth, relatedKeybindings->idealHeight());
			relatedKeybindings->setGeometry(INTERNAL_PADDING, relatedKeybindingsY, maxChildWidth, relatedKeybindings->idealHeight()); // call again using calculated height from width change
			bottomOfLastElement = interactions->y() + interactions->height();
		}
	}
private:
	std::shared_ptr<W_LABEL> title;
	std::shared_ptr<Paragraph> description;
	std::shared_ptr<ScrollableListWidget> interactions;
	std::shared_ptr<ScrollableListWidget> relatedKeybindings;
};

// MARK: - HelpTriggerWidget

class HelpTriggerWidget: public WIDGET
{
protected:
	HelpTriggerWidget() {}

	void initialize(const std::shared_ptr<W_HELPSCREEN_CLICKFORM>& parent, const std::shared_ptr<WIDGET>& target);

public:
	static std::shared_ptr<HelpTriggerWidget> make(const std::shared_ptr<W_HELPSCREEN_CLICKFORM>& parent, const std::shared_ptr<WIDGET>& target)
	{
		class make_shared_enabler: public HelpTriggerWidget {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(parent, target);
		widget->setGeometryFromScreenRect(widget->calculateIdealScreenPos());
		return widget;
	}

	WzRect calculateIdealScreenPos();
	void setGeometryFromScreenRect(WzRect const &r) override;

protected:
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
	void focusLost() override;
	void run(W_CONTEXT *) override;
	void display(int xOffset, int yOffset) override;
	void screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight) override;

protected:
	friend class W_HELPSCREEN_CLICKFORM;
	std::weak_ptr<WIDGET> getTarget() { return target; }

private:
	void showWidgetHelp();
	void closeWidgetHelp();
	WzRect calculateIdealScreenPos(const std::shared_ptr<WIDGET> parent, const std::shared_ptr<WIDGET>& target);
	bool setFocusToSelfIfPossible();

private:
	std::weak_ptr<W_HELPSCREEN_CLICKFORM> parent;
	std::weak_ptr<WIDGET> target;
	optional<std::chrono::steady_clock::time_point> hoverStart;
	uint32_t lastUpdatedIndicatorAnimationTime = 0;
	float indicatorAnimationProgress = 0.f;
	Vector2i indicator_inset = Vector2i(0, 0);

	bool clickedDownOnThis = false;
	bool queuedWidgetShowOnRelease = false;
	bool hasFocus = false;
	bool isHighlight = false;
};

void HelpTriggerWidget::initialize(const std::shared_ptr<W_HELPSCREEN_CLICKFORM>& _parent, const std::shared_ptr<WIDGET>& _target)
{
	parent = _parent;
	target = _target;
}

void HelpTriggerWidget::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	auto strongParent = parent.lock();
	ASSERT_OR_RETURN(, strongParent != nullptr, "Parent no longer exists?");

	clickedDownOnThis = true;

	if (!strongParent->helperWidgetHasFocusLock())
	{
		// immediately trigger the widget help popup
		auto triggerWidget = std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this());
		widgScheduleTask([triggerWidget]{
			triggerWidget->showWidgetHelp();
		});
	}
	else
	{
		queuedWidgetShowOnRelease = true;
	}
}

void HelpTriggerWidget::focusLost()
{
	auto triggerWidget = std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this());
	widgScheduleTask([triggerWidget]{
		triggerWidget->closeWidgetHelp();
	});
	hasFocus = false;
	clickedDownOnThis = false;
	queuedWidgetShowOnRelease = false;
}

void HelpTriggerWidget::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (clickedDownOnThis)
	{
		/* Tell the screen that this has focus */
		if (auto lockedScreen = screenPointer.lock())
		{
			lockedScreen->setFocus(shared_from_this());
			hasFocus = true;
		}

		clickedDownOnThis = false;

		if (queuedWidgetShowOnRelease)
		{
			auto triggerWidget = std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this());
			widgScheduleTask([triggerWidget]{
				triggerWidget->showWidgetHelp();
			});
			queuedWidgetShowOnRelease = false;
		}
	}
}

void HelpTriggerWidget::highlight(W_CONTEXT *psContext)
{
	hoverStart = std::chrono::steady_clock::now();
}

void HelpTriggerWidget::highlightLost()
{
	hoverStart = nullopt;
	isHighlight = false;
	clickedDownOnThis = false;
	queuedWidgetShowOnRelease = false;
	if (!hasFocus)
	{
		auto triggerWidget = std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this());
		widgScheduleTask([triggerWidget]{
			triggerWidget->closeWidgetHelp();
		});
	}
}

void HelpTriggerWidget::run(W_CONTEXT *)
{
	if (hoverStart.has_value())
	{
		// require hover to be maintained for a minimum duration before triggering the help for this widget
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - hoverStart.value()).count() >= 100)
		{
			hoverStart = nullopt;
			isHighlight = true;
			if (auto strongScreen = screenPointer.lock())
			{
				auto focusWidget = strongScreen->getWidgetWithFocus();
				if (focusWidget != nullptr && focusWidget != strongScreen->psForm)
				{
					// another trigger has the focus lock, due to a click
					// do not show widget help
					return;
				}
			}
			auto triggerWidget = std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this());
			widgScheduleTask([triggerWidget]{
				triggerWidget->showWidgetHelp();
			});
		}
	}
}

static void DrawAnimatedIndicator(int x0, int y0, int size, PIELIGHT indicatorColor, float& indicatorAnimationProgress, uint32_t& lastUpdatedIndicatorAnimationTime)
{
	if ((realTime - lastUpdatedIndicatorAnimationTime) >= 50)
	{
		float changeAmount = 100.f * static_cast<float>(realTime - lastUpdatedIndicatorAnimationTime) / static_cast<float>(GAME_TICKS_PER_SEC);
		indicatorAnimationProgress = (indicatorAnimationProgress + changeAmount);
		while (indicatorAnimationProgress > 100.f)
		{
			indicatorAnimationProgress -= 100.f;
		}
		lastUpdatedIndicatorAnimationTime = realTime;
	}

	float indicator_animation_size = static_cast<float>(size) + (static_cast<float>(indicatorAnimationProgress * 5.f) / 100.f);
	float indicator_animation_offset = (indicator_animation_size - static_cast<float>(size)) / 2.f;
	float animation_progress_inv_val = 1.f - (indicatorAnimationProgress / 100.f);
	UBYTE indicator_animation_alpha = static_cast<UBYTE>(animation_progress_inv_val * 255.f);

	if (!IntImages)
	{
		// Interface Images are not loaded - just use simple rectangles

		float animatedX0 = static_cast<float>(x0) - indicator_animation_offset;
		float animatedY0 = static_cast<float>(y0) - indicator_animation_offset;
		pie_UniTransBoxFill(animatedX0, animatedY0, animatedX0 + indicator_animation_size, animatedY0 + indicator_animation_size, pal_RGBA(indicatorColor.byte.r,indicatorColor.byte.g,indicatorColor.byte.b,indicator_animation_alpha));

		pie_UniTransBoxFill(x0, y0, x0 + size, y0 + size, indicatorColor);
		return;
	}

	iV_DrawImageTint(IntImages, IMAGE_INDICATOR_DOT, x0, y0, indicatorColor, Vector2f(size, size));

	iV_DrawImageTint(IntImages, IMAGE_INDICATOR_DOT_EXPAND, static_cast<float>(x0) - indicator_animation_offset, static_cast<float>(y0) - indicator_animation_offset, pal_RGBA(indicatorColor.byte.r,indicatorColor.byte.g,indicatorColor.byte.b,indicator_animation_alpha), Vector2f(indicator_animation_size, indicator_animation_size));
}

void HelpTriggerWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	if (isHighlight || hasFocus)
	{
		iV_Box(x0 - 1, y0 - 1, x1 + 1, y1 + 1, pal_RGBA(255, 255, 255, 125));
		return;
	}

	const int indicator_size = 10;
	int indicator_x0 = x1 - (indicator_size + indicator_inset.x);
	int indicator_y0 = y0 + indicator_inset.y;

	PIELIGHT indicatorColor = pal_RGBA(35,252,115,255);

	DrawAnimatedIndicator(indicator_x0, indicator_y0,indicator_size, indicatorColor, indicatorAnimationProgress, lastUpdatedIndicatorAnimationTime);
}

WzRect HelpTriggerWidget::calculateIdealScreenPos()
{
	auto strongTarget = target.lock();
	auto strongParent = parent.lock();
	if (!strongTarget)
	{
		// target no longer exists? clean up this trigger widget
		deleteLater();
		return {};
	}
	ASSERT_OR_RETURN({}, strongParent != nullptr, "Parent no longer exists?");
	return calculateIdealScreenPos(strongParent, strongTarget);
}

WzRect HelpTriggerWidget::calculateIdealScreenPos(const std::shared_ptr<WIDGET> _parent, const std::shared_ptr<WIDGET>& _target)
{
	ASSERT_OR_RETURN(WzRect(), _parent != nullptr, "Parent no longer exists?");
	auto targetScreenGeometry = _target->screenGeometry();

	// Overlay the target's screen geometry, in full
	return targetScreenGeometry;
}

void HelpTriggerWidget::screenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// reposition based on the target widget location (in case that changed)
	setGeometryFromScreenRect(calculateIdealScreenPos());
}

void HelpTriggerWidget::setGeometryFromScreenRect(WzRect const &r)
{
	auto strongParent = parent.lock();
	ASSERT_OR_RETURN(, strongParent != nullptr, "Parent no longer exists?");
	int xOffset = strongParent->screenPosX();
	int yOffset = strongParent->screenPosY();
	WzRect parentRelativeRect = WzRect(r.x() - xOffset, r.y() - yOffset, r.width(), r.height());
	setGeometry(parentRelativeRect);
}

void HelpTriggerWidget::showWidgetHelp()
{
	auto strongTarget = target.lock();
	auto strongParent = parent.lock();
	if (!strongTarget)
	{
		// target no longer exists? clean up this trigger widget
		deleteLater();
		return;
	}
	ASSERT_OR_RETURN(, strongParent != nullptr, "Parent no longer exists?");

	// Get help from parent
	auto pWidgetHelp = strongParent->getHelpForWidget(strongTarget);
	if (!pWidgetHelp)
	{
		return;
	}

	auto widgetHelpDisplayForm = HelpDisplayForm::make(*pWidgetHelp);

	// Figure out tbe actual needed height based on the ideal width
	widgetHelpDisplayForm->setGeometry(100, 100, widgetHelpDisplayForm->idealWidth(), widgetHelpDisplayForm->idealHeight());
	widgetHelpDisplayForm->setGeometry(100, 100, widgetHelpDisplayForm->idealWidth(), widgetHelpDisplayForm->idealHeight()); // twice to pick up calculated idealHeight for this width

	// Figure out position based on parent trigger widget
	std::weak_ptr<WIDGET> weakParent = shared_from_this();
	widgetHelpDisplayForm->setCalcLayout([weakParent](WIDGET *psWidget) {
		auto psParent = std::dynamic_pointer_cast<HelpTriggerWidget>(weakParent.lock());
		ASSERT_OR_RETURN(, psParent != nullptr, "parent is null");

		// Get the bounding rect of the parent (trigger) widget, in screen coordinates
		WzRect targetedBoundingRect = psParent->screenGeometry();

		// Try to get the position of the parent (target) widget as well
		auto psTarget = psParent->target.lock();
		if (psTarget)
		{
			targetedBoundingRect = targetedBoundingRect.minimumBoundingRect(psTarget->screenGeometry());
		}

		// Now try to position the help such that it is adjacent to this bounding rect, and aligned as best we can - but fully on-screen
		if (!psWidget->parent())
		{
			// skip setting - not attached to parent yet
			return;
		}

		// Try to align with the left side of the targeted bounding rect
		int x0 = targetedBoundingRect.x();
		if (x0 + psWidget->width() > screenWidth)
		{
			x0 = screenWidth - psWidget->width();
		}

		// Try to align so that it's "above" the targeted bounding rect
		int y0 = targetedBoundingRect.y() - (psWidget->height() + 1);
		if (y0 < 0)
		{
			// Would be off-screen
			// Try to align so that it's "below" the targeted bounding rect
			y0 = targetedBoundingRect.bottom() + 1;
			if (y0 + psWidget->height() > screenHeight)
			{
				// Would be off-screen
				// Just make sure it's on-screen as best we can
				y0 = std::max<int>(screenHeight - psWidget->height(), 0);
			}
		}

		psWidget->setGeometryFromScreenRect(WzRect(x0, y0, psWidget->width(), psWidget->height()));
	});

	// Inform parent (so it can highlight the current interface element)
	strongParent->setCurrentHelpWidget(std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this()), widgetHelpDisplayForm);
}

void HelpTriggerWidget::closeWidgetHelp()
{
	auto strongTarget = target.lock();
	auto strongParent = parent.lock();
	if (!strongTarget)
	{
		// target no longer exists? clean up this trigger widget
		deleteLater();
		return;
	}
	if (!strongParent)
	{
		return;
	}
	strongParent->unsetCurrentHelpWidget(std::dynamic_pointer_cast<HelpTriggerWidget>(shared_from_this()));
}

// MARK: - W_FULLSCREENOVERLAY_CLICKFORM

W_HELPSCREEN_CLICKFORM::W_HELPSCREEN_CLICKFORM(W_FORMINIT const *init) : W_CLICKFORM(init) {}
W_HELPSCREEN_CLICKFORM::W_HELPSCREEN_CLICKFORM() : W_CLICKFORM() {}

std::shared_ptr<W_HELPSCREEN_CLICKFORM> W_HELPSCREEN_CLICKFORM::make(UDWORD formID)
{
	W_FORMINIT sInit;
	sInit.id = formID;
	sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;
	sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	});

	class make_shared_enabler: public W_HELPSCREEN_CLICKFORM
	{
	public:
		make_shared_enabler(W_FORMINIT const *init): W_HELPSCREEN_CLICKFORM(init) {}
	};
	return std::make_shared<make_shared_enabler>(&sInit);
}

void W_HELPSCREEN_CLICKFORM::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (auto lockedScreen = screenPointer.lock())
	{
		auto widgetWithFocus = lockedScreen->getWidgetWithFocus();
		if (widgetWithFocus != nullptr && widgetWithFocus != shared_from_this())
		{
			// something else has focus
			lockedScreen->setFocus(nullptr);
			return;
		}
	}
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void W_HELPSCREEN_CLICKFORM::display(int xOffset, int yOffset)
{
	if (!visible())
	{
		return; // skip if hidden
	}

	if (backgroundColor.isTransparent())
	{
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	auto strongCutoutWidget = currentHelpWidget.lock();
	WzRect screenRect = screenGeometry();
	WzRect currentHelpWidgetScreenRect;
	if (strongCutoutWidget)
	{
		// Any widget that has help is expected to have a parent (i.e. actually be on a screen)
		auto checkedParent = strongCutoutWidget->parent();
		if (checkedParent)
		{
			currentHelpWidgetScreenRect = strongCutoutWidget->screenGeometry();
		}
		else
		{
			// This is probably an old widget that gets deleted and recreated constantly (there are a few of these left, sigh)
			// Skip trying to do a cutout, since we can't determine an updated screen position (as it has no parent)
			strongCutoutWidget.reset();
		}
	}
	WzRect cutoutRect = (strongCutoutWidget) ? screenRect.intersectionWith(currentHelpWidgetScreenRect) : WzRect();

	if (cutoutRect.width() <= 0 || cutoutRect.height() <= 0)
	{
		// simple path - draw background over everything
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
	}
	else
	{
		std::array<WzRect, 4> surroundingRects = {
			// left column until left edge of cutout rect
			WzRect({screenRect.left(), screenRect.top()}, {cutoutRect.left(), screenRect.bottom()}),
			// top band above cutout rect
			WzRect({cutoutRect.left(), screenRect.top()}, {cutoutRect.right(), cutoutRect.top()}),
			// right column after cutout rect
			WzRect({cutoutRect.right(), screenRect.top()}, {screenRect.right(), screenRect.bottom()}),
			// bottom band below cutout rect
			WzRect({cutoutRect.left(), cutoutRect.bottom()}, {cutoutRect.right(), screenRect.bottom()})
		};

		for (const auto& rect : surroundingRects)
		{
			if (rect.width() <= 0 || rect.height() <= 0)
			{
				continue;
			}
			pie_UniTransBoxFill(rect.left(), rect.top(), rect.right(), rect.bottom(), backgroundColor);
		}
	}
}

void W_HELPSCREEN_CLICKFORM::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		if (onCancelPressed)
		{
			onCancelPressed();
		}
	}
	inputLoseFocus();	// clear the input buffer.
}

void W_HELPSCREEN_CLICKFORM::setHelpForWidget(const std::shared_ptr<WIDGET>& widget, const WidgetHelp& widgetHelp)
{
	if (!widget) { return; }
	auto it = registeredWidgets.find(widget);
	if (it != registeredWidgets.end())
	{
		// update help
		it->second.help = widgetHelp;
		return;
	}
	WidgetInfo info(widgetHelp);
	info.helpTriggerWidget = HelpTriggerWidget::make(std::dynamic_pointer_cast<W_HELPSCREEN_CLICKFORM>(shared_from_this()), widget);
	attach(info.helpTriggerWidget, ChildZPos::Back);
	registeredWidgets[widget] = info;
}

void W_HELPSCREEN_CLICKFORM::removeHelpForWidget(const std::shared_ptr<WIDGET>& widget)
{
	if (!widget) { return; }
	auto it = registeredWidgets.find(widget);
	if (it != registeredWidgets.end())
	{
		it->second.helpTriggerWidget->deleteLater();
		registeredWidgets.erase(it);
	}
}

const WidgetHelp* W_HELPSCREEN_CLICKFORM::getHelpForWidget(const std::shared_ptr<WIDGET>& widget)
{
	auto it = registeredWidgets.find(widget);
	if (it == registeredWidgets.end())
	{
		return nullptr;
	}
	return &(it->second.help);
}

void W_HELPSCREEN_CLICKFORM::setCurrentHelpWidget(const std::shared_ptr<HelpTriggerWidget>& triggerWidget, const std::shared_ptr<W_FORM>& helpPanel)
{
	currentHelpWidget = triggerWidget;
	if (currentHelpPanel)
	{
		currentHelpPanel->deleteLater();
	}
	currentHelpPanel = helpPanel;
	attach(currentHelpPanel);
	currentHelpPanel->callCalcLayout();
}

void W_HELPSCREEN_CLICKFORM::unsetCurrentHelpWidget(const std::shared_ptr<HelpTriggerWidget>& triggerWidget)
{
	if (currentHelpWidget.lock() == triggerWidget)
	{
		currentHelpWidget.reset();
		if (currentHelpPanel)
		{
			currentHelpPanel->deleteLater();
			currentHelpPanel.reset();
		}
	}
}

bool W_HELPSCREEN_CLICKFORM::helperWidgetHasFocusLock()
{
	if (auto lockedScreen = screenPointer.lock())
	{
		auto widgetWithFocus = lockedScreen->getWidgetWithFocus();
		return (widgetWithFocus != nullptr && widgetWithFocus != shared_from_this());
	}
	return false;
}
