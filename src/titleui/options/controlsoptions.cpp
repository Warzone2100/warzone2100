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
 *  Controls Options form
 */

#include "optionsforms.h"

#include "../../warzoneconfig.h"
#include "../../keybind.h"
#include "../../display.h"
#include "lib/framework/wzapp.h"
#include "lib/sound/audio.h"
#include "lib/ivis_opengl/pieblitfunc.h"

static inline WzString keyFunctionInfoToOptionId(const KeyFunctionInfo& info)
{
	return WzString::fromUtf8("keybinding.") + WzString::fromUtf8(info.name);
}

// MARK: - KeyOptionsForm

class KeyOptionsForm : public OptionsForm
{
protected:
	KeyOptionsForm()
	: OptionsForm()
	{ }
public:
	static std::shared_ptr<KeyOptionsForm> make();

	~KeyOptionsForm();

	void run(W_CONTEXT *psContext) override;
	void runRecursive(W_CONTEXT *psContext) override;

	void informDidEditKeybinding();
private:
	bool didEditKeybinding = false;
};

std::shared_ptr<KeyOptionsForm> KeyOptionsForm::make()
{
	class make_shared_enabler : public KeyOptionsForm { };
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize();
	return result;
}

KeyOptionsForm::~KeyOptionsForm()
{
	if (didEditKeybinding)
	{
		gInputManager.saveMappings();
	}
}

void KeyOptionsForm::runRecursive(W_CONTEXT *psContext)
{
	inputRestoreMetaKeyState(); // HACK: to ensure meta keys are set to down if physically down (regardless of prior calls to clear logical input state)
	OptionsForm::runRecursive(psContext);
}

void KeyOptionsForm::run(W_CONTEXT *psContext)
{
	auto currMapping = gInputManager.findCurrentMapping(false, true);
	if (currMapping)
	{
		const auto& info = currMapping->get().info;
		auto keyOptionId = keyFunctionInfoToOptionId(info);
		jumpToOptionId(keyOptionId);
	}

	OptionsForm::run(psContext);
}

void KeyOptionsForm::informDidEditKeybinding()
{
	didEditKeybinding = true;
}

// MARK: -

class OptionsKeyBindingsEdit;

class OptionsKeyBindingWidget : public W_BUTTON
{
protected:
	OptionsKeyBindingWidget()
	{
		highlightBackgroundColor = WZCOL_MENU_SCORE_BUILT;
	}
public:
	static std::shared_ptr<OptionsKeyBindingWidget> make(const std::shared_ptr<OptionsKeyBindingsEdit>& parent, KeyMappingSlot slot);

	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
	std::string getTip() override;

	void setIsEditingMode(bool val);

	void update(const std::shared_ptr<OptionsKeyBindingsEdit>& parent, KeyMappingSlot slot);

protected:
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;

private:
	WzString text;
	WzCachedText wzText;
	iV_fonts FontID = font_regular;
	PIELIGHT bindingColor = WZCOL_FORM_TEXT;
	int32_t cachedIdealTextWidth = 0;
	bool bEditable = false;

	bool isSelected = false;
	bool isEditingMode = false;
	bool isTruncated = false;
	int32_t lastWidgetWidth = 0;

	PIELIGHT highlightBackgroundColor;
	const int32_t horizontalPadding = 5;
	const int32_t verticalPadding = 8;
};

class OptionsKeyBindingsEdit : public WIDGET, public OptionValueChangerInterface
{
protected:
	OptionsKeyBindingsEdit(const std::shared_ptr<KeyOptionsForm>& parentForm, InputManager& inputManager, const KeyFunctionInfo& info);
	void initialize();
public:
	static std::shared_ptr<OptionsKeyBindingsEdit> make(const std::shared_ptr<KeyOptionsForm>& parentForm, InputManager& inputManager, const KeyFunctionInfo& info);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;

	void update(bool force) override;
	void informAvailable(bool isAvailable) override;
	void addOnChangeHandler(std::function<void(WIDGET&)> handler) override;
	void addStartingCapturedEditingHandler(std::function<void(WIDGET&)> handler) override;
	void addEndingCapturedEditingHandler(std::function<void(WIDGET&)> handler) override;

private:
	void updateData();
	void updateButton(KeyMappingSlot slot);
	void updateButtons();
	bool triggerEditModeForSlot(KeyMappingSlot slot);
	void closeEditModeOverlay();
	bool onPushedKeyCombo(KeyMappingSlot slot, const KeyMappingInput input, optional<KEY_CODE> metaKey);
	void updateLayout();

private:
	friend class OptionsKeyBindingWidget;

	std::weak_ptr<KeyOptionsForm> parentOptionsForm;
	InputManager& inputManager;
	const KeyFunctionInfo& info;
	std::array<nonstd::optional<std::reference_wrapper<KeyMapping>>, static_cast<size_t>(KeyMappingSlot::LAST)> mappings;

	std::array<std::shared_ptr<OptionsKeyBindingWidget>, static_cast<size_t>(KeyMappingSlot::LAST)> keyMappingSlotButtons;
	std::shared_ptr<W_SCREEN> overlayScreen;

	std::vector<std::function<void(WIDGET&)>> onChangeHandlers;
	std::function<void(WIDGET&)> onStartEditingHandler;
	std::function<void(WIDGET&)> onEndEditingHandler;

	int32_t cachedIdealWidth = 0;
	int32_t cachedIdealHeight = 0;
	const int32_t minimumWidth = 10;
	const int32_t buttonWidth = 110;
	const int32_t buttonSpacing = 5;
};

// MARK: - OptionsKeyBindingWidget

std::shared_ptr<OptionsKeyBindingWidget> OptionsKeyBindingWidget::make(const std::shared_ptr<OptionsKeyBindingsEdit>& parent, KeyMappingSlot slot)
{
	class make_shared_enabler : public OptionsKeyBindingWidget { };
	auto result = std::make_shared<make_shared_enabler>();
	result->update(parent, slot);
	return result;
}

void OptionsKeyBindingWidget::setIsEditingMode(bool val)
{
	isEditingMode = val;
	optional<ProgressBorder> newProgressBorder;
	if (isEditingMode)
	{
		newProgressBorder = ProgressBorder::indeterminate(ProgressBorder::BorderInset(1,1,1,1));
	}
	setProgressBorder(newProgressBorder, pal_RGBA(255,255,255,200));
}

void OptionsKeyBindingWidget::update(const std::shared_ptr<OptionsKeyBindingsEdit>& parent, KeyMappingSlot slot)
{
	bEditable = (parent->info.type == KeyMappingType::ASSIGNABLE);
	if (!bEditable)
	{
		setState(WBUT_DISABLE);
	}
	else if (bEditable && (getState() & WBUT_DISABLE))
	{
		setState(0);
	}
	bindingColor = WZCOL_TEXT_BRIGHT;

	WzString newStringValue;
	const nonstd::optional<KeyMapping> mapping = parent->mappings[static_cast<unsigned int>(slot)];
	if (mapping && !mapping->keys.input.isCleared())
	{
		// Check to see if key is on the numpad, if so tell user and change color
		const bool isBoundToNumpad = mapping->keys.input.source == KeyMappingInputSource::KEY_CODE && mapping->keys.input.value.keyCode >= KEY_KP_0 && mapping->keys.input.value.keyCode <= KEY_KPENTER;
		if (isBoundToNumpad)
		{
			bindingColor = WZCOL_YELLOW;
		}
		newStringValue = WzString::fromUtf8(mapping->toString());
	}
	text = newStringValue;
	cachedIdealTextWidth = iV_GetTextWidth(text, FontID);
}

void OptionsKeyBindingWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int w = width();
	int h = height();

	bool isHighlight = bEditable && isSelected;

	iV_fonts fontID = wzText.getFontID();
	if (fontID == font_count || lastWidgetWidth != width() || wzText.getText() != text)
	{
		fontID = FontID;
		wzText.setText(text, fontID);
	}

	int availableButtonTextWidth = w - (horizontalPadding * 2);
	if (wzText->width() > availableButtonTextWidth)
	{
		// text would exceed the bounds of the button area
		// try to shrink font so it fits
		do {
			if (fontID == font_small || fontID == font_bar)
			{
				break;
			}
			auto result = iV_ShrinkFont(fontID);
			if (!result.has_value())
			{
				break;
			}
			fontID = result.value();
			wzText.setText(text, fontID);
		} while (wzText->width() > availableButtonTextWidth);
	}
	lastWidgetWidth = width();

	// Draw background
	PIELIGHT backgroundColor = WZCOL_FORM_DARK;
	backgroundColor.byte.a = static_cast<uint8_t>(backgroundColor.byte.a / ((bEditable) ? 2 : 3));
	int backX0 = x0;
	int backY0 = y0;
	int backX1 = x0 + w - 1;
	int backY1 = y0 + h - 1;
	bool drawHighlightBorder = (isHighlight && !isEditingMode);
	if (drawHighlightBorder)
	{
		pie_UniTransBoxFill(backX0, backY0, backX1, backY1, pal_RGBA(255,255,255,100));
		backX0 += 1;
		backY0 += 1;
		backX1 -= 1;
		backY1 -= 1;
	}
	if ((isHighlight || isEditingMode) && !highlightBackgroundColor.isTransparent())
	{
		backgroundColor = highlightBackgroundColor;
	}
	pie_UniTransBoxFill(backX0, backY0, backX1, backY1, backgroundColor);

	// Draw the main text
	PIELIGHT textColor = bindingColor;
	if (!bEditable)
	{
		textColor = WZCOL_TEXT_MEDIUM;
	}

	int maxDisplayableMainTextWidth = availableButtonTextWidth;

	int textX0 = x0 + horizontalPadding + (availableButtonTextWidth - std::min(maxDisplayableMainTextWidth, wzText->width())) / 2;
	int textY0 = y0 + (h - wzText->lineSize()) / 2 - wzText->aboveBase();

	isTruncated = maxDisplayableMainTextWidth < wzText->width();
	if (isTruncated)
	{
		maxDisplayableMainTextWidth -= (iV_GetEllipsisWidth(wzText.getFontID()) + 2);
	}
	wzText->render(textX0, textY0, textColor, 0.0f, maxDisplayableMainTextWidth);
	if (isTruncated)
	{
		// Render ellipsis
		iV_DrawEllipsis(wzText.getFontID(), Vector2f(textX0 + maxDisplayableMainTextWidth + 2, textY0), textColor);
	}
}

void OptionsKeyBindingWidget::run(W_CONTEXT *psContext)
{
	wzText.tick();
}

void OptionsKeyBindingWidget::geometryChanged()
{
	// currently no-op
}

int32_t OptionsKeyBindingWidget::idealWidth()
{
	return (horizontalPadding * 2) + cachedIdealTextWidth;
}

int32_t OptionsKeyBindingWidget::idealHeight()
{
	return (verticalPadding * 2) + iV_GetTextLineSize(FontID);
}

std::string OptionsKeyBindingWidget::getTip()
{
	if (!isTruncated) { return std::string(); }
	return text.toUtf8();
}

void OptionsKeyBindingWidget::highlight(W_CONTEXT *psContext)
{
	isSelected = true;
}

void OptionsKeyBindingWidget::highlightLost()
{
	isSelected = false;
}

// MARK: - OptionsKeyBindingsEdit

std::shared_ptr<OptionsKeyBindingsEdit> OptionsKeyBindingsEdit::make(const std::shared_ptr<KeyOptionsForm>& parentForm, InputManager& inputManager, const KeyFunctionInfo& info)
{
	class make_shared_enabler : public OptionsKeyBindingsEdit {
	public:
		make_shared_enabler(const std::shared_ptr<KeyOptionsForm>& parentForm, InputManager& inputManager, const KeyFunctionInfo& info)
		: OptionsKeyBindingsEdit(parentForm, inputManager, info)
		{ }
	};
	auto result = std::make_shared<make_shared_enabler>(parentForm, inputManager, info);
	result->initialize();
	return result;
}

OptionsKeyBindingsEdit::OptionsKeyBindingsEdit(const std::shared_ptr<KeyOptionsForm>& parentForm, InputManager& inputManager, const KeyFunctionInfo& info)
: parentOptionsForm(parentForm), inputManager(inputManager), info(info)
{ }

void OptionsKeyBindingsEdit::initialize()
{
	updateData();

	auto weakSelf = std::weak_ptr<OptionsKeyBindingsEdit>(std::dynamic_pointer_cast<OptionsKeyBindingsEdit>(shared_from_this()));

	const unsigned int numSlots = static_cast<unsigned int>(KeyMappingSlot::LAST);
	for (unsigned int slotIndex = 0; slotIndex < numSlots; ++slotIndex)
	{
		const auto slot = static_cast<KeyMappingSlot>(slotIndex);
		auto button = OptionsKeyBindingWidget::make(std::static_pointer_cast<OptionsKeyBindingsEdit>(shared_from_this()), slot);
		attach(button);
		keyMappingSlotButtons[slotIndex] = button;

		button->addOnClickHandler([weakSelf, slot](W_BUTTON&) {
			auto strongBindingsEdit = weakSelf.lock();
			ASSERT_OR_RETURN(, strongBindingsEdit != nullptr, "Null parent?");
			strongBindingsEdit->triggerEditModeForSlot(slot);
		});
	}

	updateLayout();
}

void OptionsKeyBindingsEdit::updateData()
{
	const unsigned int numSlots = static_cast<unsigned int>(KeyMappingSlot::LAST);
	for (unsigned int slotIndex = 0; slotIndex < numSlots; ++slotIndex)
	{
		const auto slot = static_cast<KeyMappingSlot>(slotIndex);
		mappings[slotIndex] = inputManager.mappings().get(info, slot);
	}
}

void OptionsKeyBindingsEdit::updateButton(KeyMappingSlot slot)
{
	auto slotIndex = static_cast<size_t>(slot);
	keyMappingSlotButtons[slotIndex]->update(std::static_pointer_cast<OptionsKeyBindingsEdit>(shared_from_this()), slot);
}

void OptionsKeyBindingsEdit::updateButtons()
{
	const unsigned int numSlots = static_cast<unsigned int>(KeyMappingSlot::LAST);
	for (unsigned int slotIndex = 0; slotIndex < numSlots; ++slotIndex)
	{
		const auto slot = static_cast<KeyMappingSlot>(slotIndex);
		updateButton(slot);
	}
}

void OptionsKeyBindingsEdit::display(int xOffset, int yOffset)
{
	// no-op
}

void OptionsKeyBindingsEdit::geometryChanged()
{
	// reposition the mapping slot buttons
	if (width() == 0 || height() == 0)
	{
		return;
	}
	updateLayout();
}

void OptionsKeyBindingsEdit::updateLayout()
{
	int w = width();
	int h = height();

	cachedIdealWidth = 0;
	cachedIdealHeight = 0;

	auto hasDefaultMapping = [&](size_t slot) -> bool {
		return (slot < info.defaultMappings.size()) && !info.defaultMappings[slot].second.input.isCleared();
	};

	int availableWidth = w;
	int nextPosX0 = availableWidth - buttonWidth;

	int numButtonsShown = 0;
	for (size_t i = 0; i < mappings.size(); ++i)
	{
		const bool slotHasMapping = mappings[i] && !mappings[i]->get().keys.input.isCleared();
		const bool slotHasDefaultMapping = hasDefaultMapping(i);
		if (slotHasMapping || slotHasDefaultMapping || i == 0)
		{
			++numButtonsShown;
		}
	}

	int32_t actualButtonWidth = std::min<int32_t>((numButtonsShown > 0) ? availableWidth / numButtonsShown : 0, buttonWidth);
	for (size_t i = 0; i < mappings.size(); ++i)
	{
		const bool slotHasMapping = mappings[i] && !mappings[i]->get().keys.input.isCleared();
		const bool slotHasDefaultMapping = hasDefaultMapping(i);
		const auto buttonIdealHeight = keyMappingSlotButtons[i]->idealHeight();
		if (slotHasMapping || slotHasDefaultMapping || i == 0)
		{
			int buttonY0 = (h - buttonIdealHeight) / 2;
			keyMappingSlotButtons[i]->setGeometry(nextPosX0, buttonY0, actualButtonWidth, buttonIdealHeight);
			keyMappingSlotButtons[i]->show();
			cachedIdealWidth += buttonWidth + buttonSpacing;
			nextPosX0 -= actualButtonWidth + buttonSpacing;
		}
		else
		{
			keyMappingSlotButtons[i]->hide();
		}
		cachedIdealHeight = std::max(cachedIdealHeight, buttonIdealHeight);
	}

	cachedIdealWidth = std::max(cachedIdealWidth, minimumWidth);
}

int32_t OptionsKeyBindingsEdit::idealWidth()
{
	return cachedIdealWidth;
}

int32_t OptionsKeyBindingsEdit::idealHeight()
{
	return cachedIdealHeight;
}

void OptionsKeyBindingsEdit::update(bool force)
{
	updateData();
	updateButtons();
	updateLayout();
}

void OptionsKeyBindingsEdit::informAvailable(bool isAvailable)
{
	// TODO: Handle disabled?
}

void OptionsKeyBindingsEdit::addOnChangeHandler(std::function<void(WIDGET&)> handler)
{
	onChangeHandlers.push_back(handler);
}

void OptionsKeyBindingsEdit::addStartingCapturedEditingHandler(std::function<void(WIDGET&)> handler)
{
	onStartEditingHandler = handler;
}

void OptionsKeyBindingsEdit::addEndingCapturedEditingHandler(std::function<void(WIDGET&)> handler)
{
	onEndEditingHandler = handler;
}

static nonstd::optional<KEY_CODE> scanKeyBoardForPressedBindableKey()
{
	for (unsigned int i = 0; i < KEY_MAXSCAN; i++)
	{
		if (keyPressed((KEY_CODE)i))
		{
			if (i !=	KEY_RALT			// exceptions
				&& i !=	KEY_LALT
				&& i != KEY_RCTRL
				&& i != KEY_LCTRL
				&& i != KEY_RSHIFT
				&& i != KEY_LSHIFT
				&& i != KEY_LMETA
				&& i != KEY_RMETA
			   )
			{
				return (KEY_CODE)i;             // top row key pressed
			}
		}
	}
	return nonstd::nullopt;
}

static nonstd::optional<MOUSE_KEY_CODE> scanMouseForPressedBindableKey()
{
	for (unsigned int i = 0; i < MOUSE_KEY_CODE::MOUSE_END; i++)
	{
		const MOUSE_KEY_CODE mouseKeyCode = (MOUSE_KEY_CODE)i;
		if (mousePressed(mouseKeyCode))
		{
			if (   mouseKeyCode != MOUSE_KEY_CODE::MOUSE_LMB // exceptions
				&& mouseKeyCode != MOUSE_KEY_CODE::MOUSE_RMB
				&& mouseKeyCode != MOUSE_KEY_CODE::MOUSE_MMB)
			{
				return mouseKeyCode; // Bindable mouse key pressed
			}
		}
	}
	return nonstd::nullopt;
}

static optional<KEY_CODE> scanForPressedMetaKey()
{
	optional<KEY_CODE> metakey;

	if (keyDown(KEY_RALT) || keyDown(KEY_LALT))
	{
		metakey = KEY_LALT;
	}
	else if (keyDown(KEY_RCTRL) || keyDown(KEY_LCTRL))
	{
		metakey = KEY_LCTRL;
	}
	else if (keyDown(KEY_RSHIFT) || keyDown(KEY_LSHIFT))
	{
		metakey = KEY_LSHIFT;
	}
	else if (keyDown(KEY_RMETA) || keyDown(KEY_LMETA))
	{
		metakey = KEY_LMETA;
	}

	return metakey;
}

class WzFullscreenKeyBindingEditingOverlay : public W_FULLSCREENOVERLAY_CLICKFORM
{
protected:
	WzFullscreenKeyBindingEditingOverlay(W_FORMINIT const *init)
	: W_FULLSCREENOVERLAY_CLICKFORM(init)
	{ }
public:
	typedef std::function<void(const KeyMappingInput input, optional<KEY_CODE> metaKey)> OnPushedKeyComboFunc;

	static std::shared_ptr<WzFullscreenKeyBindingEditingOverlay> make()
	{
		W_FORMINIT sInit;
		sInit.id = 0;
		sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
		sInit.x = 0;
		sInit.y = 0;
		sInit.width = screenWidth - 1;
		sInit.height = screenHeight - 1;
		sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
			psWidget->setGeometry(0, 0, screenWidth, screenHeight);
		});

		class make_shared_enabler: public WzFullscreenKeyBindingEditingOverlay
		{
		public:
			make_shared_enabler(W_FORMINIT const *init): WzFullscreenKeyBindingEditingOverlay(init) {}
		};
		return std::make_shared<make_shared_enabler>(&sInit);
	}

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override
	{
		if (key != WKEY_PRIMARY)
		{
			return;
		}

		if (auto strongCutout = cutoutWidget.lock())
		{
			auto screenContext = psContext->convertToScreenContext();
			if (strongCutout->screenGeometry().contains(screenContext.mx, screenContext.my))
			{
				// ignore click inside the cutout widget
				return;
			}
		}

		mouseDownOutsideTargetWidget = true;
	}

	void released(W_CONTEXT *, WIDGET_KEY key) override
	{
		if (!mouseDownOutsideTargetWidget)
		{
			return;
		}

		mouseDownOutsideTargetWidget = false;

		if (onClickedFunc)
		{
			onClickedFunc();
		}
	}

	void highlightLost() override
	{
		mouseDownOutsideTargetWidget = false;
	}

	void run(W_CONTEXT *psContext) override
	{
		inputRestoreMetaKeyState(); // HACK: to ensure meta keys are set to down if physically down (regardless of prior calls to clear logical input state)

		auto metaKey = scanForPressedMetaKey();

		if (const optional<KEY_CODE> kc = scanKeyBoardForPressedBindableKey())
		{
			if (kc.value() == KEY_ESC)
			{
				// Special-handling for ESC - unhighlight the selection
				inputLoseFocus();	// clear the input buffer.
				if (onCancelPressed) { onCancelPressed(); }
				return;
			}
			onPushedKeyCombo(*kc, metaKey);
		}

		if (const optional<MOUSE_KEY_CODE> mkc = scanMouseForPressedBindableKey())
		{
			onPushedKeyCombo(*mkc, metaKey);
		}

		inputLoseFocus();	// clear the input buffer.
	}

public:
	OnPushedKeyComboFunc onPushedKeyCombo;

private:
	void updateSavedMetaKeyState(KEY_CODE key, bool& s)
	{
		if (keyDown(key))
		{
			s = true;
		}
		else if (keyReleased(key))
		{
			s = false;
		}
	}

	void updateSavedMetaKeyStates()
	{
		updateSavedMetaKeyState(KEY_RALT, metakeyDown_RALT);
		updateSavedMetaKeyState(KEY_LALT, metakeyDown_LALT);

		updateSavedMetaKeyState(KEY_RCTRL, metakeyDown_RCTRL);
		updateSavedMetaKeyState(KEY_LCTRL, metakeyDown_LCTRL);

		updateSavedMetaKeyState(KEY_RSHIFT, metakeyDown_RSHIFT);
		updateSavedMetaKeyState(KEY_LSHIFT, metakeyDown_LSHIFT);

		updateSavedMetaKeyState(KEY_RMETA, metakeyDown_RMETA);
		updateSavedMetaKeyState(KEY_LMETA, metakeyDown_LMETA);
	}

private:
	bool metakeyDown_RALT = false;
	bool metakeyDown_LALT = false;

	bool metakeyDown_RCTRL = false;
	bool metakeyDown_LCTRL = false;

	bool metakeyDown_RSHIFT = false;
	bool metakeyDown_LSHIFT = false;

	bool metakeyDown_RMETA = false;
	bool metakeyDown_LMETA = false;

	bool mouseDownOutsideTargetWidget = false;
};

bool OptionsKeyBindingsEdit::triggerEditModeForSlot(KeyMappingSlot slot)
{
	if (info.type != KeyMappingType::ASSIGNABLE)
	{
		return false;
	}

	auto weakSelf = std::weak_ptr<OptionsKeyBindingsEdit>(std::dynamic_pointer_cast<OptionsKeyBindingsEdit>(shared_from_this()));
	auto targetSlotButton = keyMappingSlotButtons[static_cast<size_t>(slot)];

	auto lockedScreen = screenPointer.lock();
	ASSERT_OR_RETURN(false, lockedScreen != nullptr, "The CampaignStartOptionsForm does not have an associated screen pointer?");

	if (onStartEditingHandler)
	{
		onStartEditingHandler(*this);
	}

	// Initialize the editing overlay screen
	overlayScreen = W_SCREEN::make();
	auto newRootFrm = WzFullscreenKeyBindingEditingOverlay::make();
	std::weak_ptr<W_SCREEN> psWeakOverlayScreen(overlayScreen);
	newRootFrm->onPushedKeyCombo = [weakSelf, slot](const KeyMappingInput input, optional<KEY_CODE> metaKey) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "Widget already gone");
		if (strongSelf->onPushedKeyCombo(slot, input, metaKey))
		{
			if (auto strongParentOptionsForm = strongSelf->parentOptionsForm.lock())
			{
				strongParentOptionsForm->informDidEditKeybinding();
			}
			else
			{
				ASSERT(false, "Unable to inform parent options form?");
			}
		}
		strongSelf->closeEditModeOverlay();
	};
	newRootFrm->onCancelPressed = [weakSelf]() {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "Widget already gone");
		strongSelf->closeEditModeOverlay();
	};
	newRootFrm->onClickedFunc = newRootFrm->onCancelPressed;
	overlayScreen->psForm->attach(newRootFrm);

	targetSlotButton->setIsEditingMode(true);
	newRootFrm->setCutoutWidget(targetSlotButton);

	widgRegisterOverlayScreenOnTopOfScreen(overlayScreen, lockedScreen);

	return true;
}

void OptionsKeyBindingsEdit::closeEditModeOverlay()
{
	if (overlayScreen)
	{
		if (onEndEditingHandler)
		{
			onEndEditingHandler(*this);
		}

		widgRemoveOverlayScreen(overlayScreen);
		overlayScreen.reset();
	}

	for (const auto& b : keyMappingSlotButtons)
	{
		b->setIsEditingMode(false);
	}
}

bool OptionsKeyBindingsEdit::onPushedKeyCombo(KeyMappingSlot slot, const KeyMappingInput input, optional<KEY_CODE> metaKeyOpt)
{
	KEY_CODE metakey = metaKeyOpt.value_or(KEY_IGNORE);

	const auto selectedInfo = &info;
	/* Disallow modifying non-assignable mappings. (Null-check the `info` in case assertions are disabled) */
	if (!selectedInfo || selectedInfo->type != KeyMappingType::ASSIGNABLE)
	{
		return false;
	}

	/* Disallow conflicts with ALWAYS_ACTIVE keybinds, as that context always has max priority, preventing
	   any conflicting keys from triggering. */
	for (const KeyMapping& mapping : inputManager.mappings().findConflicting(metakey, input, selectedInfo->context, inputManager.contexts()))
	{
		const InputContext context = inputManager.contexts().get(mapping.info.context);
		if (context.isAlwaysActive())
		{
			audio_PlayBuildFailedOnce();
			return false;
		}
	}

	/* Clear conflicting mappings using these keys */
	inputManager.mappings().removeConflicting(metakey, input, selectedInfo->context, inputManager.contexts());

	/* Try and see if the mapping already exists. Remove the old mapping if one does exist */
	const auto maybeOld = inputManager.mappings().get(*selectedInfo, slot);
	if (maybeOld.has_value())
	{
		inputManager.mappings().remove(*maybeOld);
	}

	/* Figure out which `KeyAction` the mapping should use */
	const auto foundPrimary = std::find_if(selectedInfo->defaultMappings.cbegin(), selectedInfo->defaultMappings.cend(), [](const std::pair<KeyMappingSlot, KeyCombination>& defaultMapping) {
		return defaultMapping.first == KeyMappingSlot::PRIMARY;
	});

	const auto foundMatch = std::find_if(selectedInfo->defaultMappings.cbegin(), selectedInfo->defaultMappings.cend(), [&input](const std::pair<KeyMappingSlot, KeyCombination>& defaultMapping) {
		const KeyCombination combination = defaultMapping.second;
		return combination.input == input;
	});

	KeyAction action = KeyAction::PRESSED;
	if (foundMatch != selectedInfo->defaultMappings.cend())
	{
		action = foundMatch->second.action;
	}
	else if (foundPrimary != selectedInfo->defaultMappings.cend())
	{
		action = foundPrimary->second.action;
	}


	/* Finally, create the new mapping */
	KeyMapping& newMapping = inputManager.mappings().add({ metakey, input, action }, *selectedInfo, slot);

	auto slotIndex = static_cast<size_t>(slot);
	mappings[slotIndex] = newMapping;
	updateButton(slot);

	for (const auto& onChangeFunc : onChangeHandlers)
	{
		onChangeFunc(*this);
	}

	return true;
}

// MARK: -

KeyFunctionEntries getVisibleKeyFunctionEntries(const KeyFunctionConfiguration& keyFuncConfig)
{
	KeyFunctionEntries visible;
	for (const KeyFunctionInfo& info : keyFuncConfig.allKeyFunctionEntries())
	{
		if (info.type != KeyMappingType::HIDDEN)
		{
			visible.push_back(info);
		}
	}

	return visible;
}

size_t addKeyBindingsToOptionsForm(const std::shared_ptr<KeyOptionsForm>& result, InputManager& inputManager, const KeyFunctionConfiguration& keyFuncConfig)
{
	auto infos = getVisibleKeyFunctionEntries(keyFuncConfig);
	std::sort(infos.begin(), infos.end(), [&](const KeyFunctionInfo& a, const KeyFunctionInfo& b) {
		const bool bContextsAreSame = a.context == b.context;
		const InputContext& contextA = inputManager.contexts().get(a.context);
		const InputContext& contextB = inputManager.contexts().get(b.context);
		return bContextsAreSame
			? a.displayName < b.displayName
			: contextA.getDisplayName() < contextB.getDisplayName();
	});

	for (KeyFunctionEntries::const_iterator i = infos.begin(); i != infos.end(); ++i)
	{
		const KeyFunctionInfo& info = *i;

		/* Add separator if changing categories */
		const bool bShouldAddSeparator = i == infos.begin() || std::prev(i)->get().context != info.context;
		if (bShouldAddSeparator)
		{
			result->addSection(OptionsSection(WzString::fromUtf8(inputManager.contexts().get(info.context).getDisplayName()), ""), true);
		}

		auto optionInfo = OptionInfo(keyFunctionInfoToOptionId(info), WzString::fromUtf8(info.displayName), "");
		auto valueChanger = OptionsKeyBindingsEdit::make(result, inputManager, info);
		result->addOption(optionInfo, valueChanger, true);
	}

	return infos.size();
}

static WzString mouseKeyCodeToWzString(MOUSE_KEY_CODE code)
{
	char asciiSub[20] = "\0";
	mouseKeyCodeToString(code, (char*)&asciiSub, 20);
	return WzString(asciiSub);
}

OptionInfo::AvailabilityResult MouseDragToRotateIsBound(const OptionInfo&)
{
	OptionInfo::AvailabilityResult result;
	result.available = getRotateMouseKey().has_value();
	return result;
}

// MARK: -

std::shared_ptr<OptionsForm> makeControlsOptionsForm()
{
	auto result = KeyOptionsForm::make();

	// Mouse:
	result->addSection(OptionsSection(N_("Mouse"), ""), true);
	{
		auto optionInfo = OptionInfo("controls.mouse.rightClickOrders", N_("Right-Click Orders"), N_("By default, Warzone 2100 uses left-click for both selection and ordering / targeting. If you prefer modern RTS mouse input, set this to On. If you are using a trackpad or touch input, you may want to set this to Off."));
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), _("Use left-click for both selection and ordering / targeting. (The classic Warzone 2100 default.)"), false },
					{ _("On"), _("Order units to move or target with right-click. Select units with left-click. (Matches other common RTS game defaults.)"), true },
				};
				result.setCurrentIdxForValue(getRightClickOrders());
				return result;
			},
			[](const auto& newValue) -> bool {
				setRightClickOrders(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.trapCursor", N_("Trap Cursor"), "");
		auto valueChanger = OptionsDropdown<TrapCursorMode>::make(
			[]() {
				OptionChoices<TrapCursorMode> result;
				result.choices = {
					{ _("Off"), "", TrapCursorMode::Disabled },
					{ _("On"), "", TrapCursorMode::Enabled },
					{ _("Auto"), "", TrapCursorMode::Automatic },
				};
				result.setCurrentIdxForValue(war_GetTrapCursor());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_SetTrapCursor(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.coloredCursors", N_("Colored Cursors"), "");
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(war_GetColouredCursor());
				return result;
			},
			[](const auto& newValue) -> bool {
				war_SetColouredCursor(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.cursorSize", N_("Cursor Size"), "");
		auto valueChanger = OptionsDropdown<unsigned int>::make(
			[]() {
				OptionChoices<unsigned int> result;
				result.choices = {
					{ "100%", "", 100 },
					{ "125%", "", 125 },
					{ "150%", "", 150 },
					{ "200%", "", 200 },
				};
				auto currValue = war_getCursorScale();
				if (!result.setCurrentIdxForValue(currValue))
				{
					// add "Custom" item
					result.choices.push_back({WzString::fromUtf8(astringf("(%u%%)", currValue)), "", currValue});
					result.currentIdx = result.choices.size() - 1;
				}
				return result;
			},
			[](const auto& newValue) -> bool {
				return wzChangeCursorScale(newValue);
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	// Camera:
	result->addSection(OptionsSection(N_("Camera"), ""), true);
	{
		auto optionInfo = OptionInfo("controls.camMoveSpeed", N_("Camera Speed"), "");
		auto valueChanger = OptionsSlider::make(CAMERASPEED_MIN, CAMERASPEED_MAX, CAMERASPEED_STEP,
			[]() { return war_GetCameraSpeed(); },
			[](int32_t newValue) {
				war_SetCameraSpeed(newValue);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.camStartZoom", N_("Camera Starting Zoom"), "");
		auto valueChanger = OptionsSlider::make(MINDISTANCE_CONFIG, MAXDISTANCE, MAP_ZOOM_CONFIG_STEP,
			[]() { return war_GetMapZoom(); },
			[](int32_t newValue) {
				war_SetMapZoom(newValue);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.camZomRate", N_("Camera Zoom Rate"), "");
		auto valueChanger = OptionsSlider::make(MAP_ZOOM_RATE_MIN, MAP_ZOOM_RATE_MAX, MAP_ZOOM_RATE_STEP,
			[]() { return war_GetMapZoomRate(); },
			[](int32_t newValue) {
				war_SetMapZoomRate(newValue);
			}, false
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.rotateCamera", N_("Mouse Rotate"), "");
		auto valueChanger = OptionsDropdown<optional<MOUSE_KEY_CODE>>::make(
			[]() {
				OptionChoices<optional<MOUSE_KEY_CODE>> result;
				result.choices = {
					{ _("Disabled"), "", nullopt },
					{ mouseKeyCodeToWzString(MOUSE_MMB), "", MOUSE_MMB },
					{ mouseKeyCodeToWzString(MOUSE_RMB), "", MOUSE_RMB, getRightClickOrders() },
				};
				result.setCurrentIdxForValue(getRotateMouseKey());
				return result;
			},
			[](const auto& newValue) -> bool {
				return setRotateMouseKey(newValue);
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.reverseRotation", N_("Reverse Rotation"), "");
		optionInfo.addAvailabilityCondition(MouseDragToRotateIsBound);
		auto valueChanger = OptionsDropdown<bool>::make(
			[]() {
				OptionChoices<bool> result;
				result.choices = {
					{ _("Off"), "", false },
					{ _("On"), "", true },
				};
				result.setCurrentIdxForValue(getInvertMouseStatus());
				return result;
			},
			[](const auto& newValue) -> bool {
				setInvertMouseStatus(newValue);
				return true;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true, 1);
	}
	{
		auto optionInfo = OptionInfo("controls.mouse.panCamera", N_("Mouse Pan"), "");
		auto valueChanger = OptionsDropdown<optional<MOUSE_KEY_CODE>>::make(
			[]() {
				OptionChoices<optional<MOUSE_KEY_CODE>> result;
				result.choices = {
					{ _("Edge Scrolling"), "", nullopt },
					{ mouseKeyCodeToWzString(MOUSE_MMB), "", MOUSE_MMB },
					{ mouseKeyCodeToWzString(MOUSE_RMB), "", MOUSE_RMB, getRightClickOrders() },
				};
				result.setCurrentIdxForValue(getPanMouseKey());
				return result;
			},
			[](const auto& newValue) -> bool {
				return setPanMouseKey(newValue);
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	addKeyBindingsToOptionsForm(result, gInputManager, gKeyFuncConfig);

	result->addSection(OptionsSection(N_("Reset"), ""), true);
	{
		auto optionInfo = OptionInfo("controls.keys.resetKeymappings", N_("Reset Keys to:"), "");
		auto valueChanger = OptionsDropdown<WzString>::make(
			[]() {
				OptionChoices<WzString> result;
				result.choices = {
					{ _("(Select Profile)"), "", "" },
					{ _("Reset Keys to: Defaults"), "", "BUILTIN::DEFAULTS" }
				};
				result.currentIdx = 0;
				return result;
			},
			[/*, weakOptionForm*/](const auto& newValue) mutable -> bool {
				if (newValue.isEmpty())
				{
					return true;
				}
				// FUTURE TODO: Prompt before resetting?
				if (newValue == "BUILTIN::DEFAULTS")
				{
					// reinitialise default key mappings
					gInputManager.resetMappings(true, gKeyFuncConfig);
					return true;
				}
				return false;
			}, true
		);
		result->addOption(optionInfo, valueChanger, true);
	}

	return result;
}
