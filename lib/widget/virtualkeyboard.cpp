// SPDX-License-Identifier: GPL-2.0-or-later

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
 *  The built-in on-screen keyboard.
 *
 *  An overlay screen above the regular widgets.
 *  Each frame, virtualKeyboardUpdate() decides whether to show it for the edit box being edited.
 *  Keys type into the edit box through the text input buffer, and the edit box ignores clicks on the keyboard.
 *
 *  With a gamepad, South presses the key under the cursor, the D-pad moves between keys,
 *  and the other buttons are typing shortcuts (see handleGamepadInput()).
 */

#include "virtualkeyboard.h"

#include "kblayout.h"
#include "widget.h"
#include "form.h"
#include "editbox.h"

#include "lib/framework/frame.h"
#include "lib/framework/i18n.h"
#include "lib/framework/input.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/gamepad_input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/screen.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>

// Key repeat while a key (or its gamepad shortcut) is held
static constexpr uint32_t KEY_REPEAT_DELAY_MS = 450;
static constexpr uint32_t KEY_REPEAT_INTERVAL_MS = 60;
static constexpr uint32_t NAV_REPEAT_DELAY_MS = 350;
static constexpr uint32_t NAV_REPEAT_INTERVAL_MS = 90;
// How long a key lights up when triggered from a gamepad shortcut
static constexpr uint32_t KEY_FLASH_MS = 120;
// How long a letter key is held to offer its accents
static constexpr uint32_t LONG_PRESS_MS = 400;
// How long virtualKeyboardRequestForNextTextEntry() waits for an edit box to start editing
static constexpr uint32_t NEXT_SESSION_REQUEST_MS = 1000;
// A request this soon after a physical key press is taken to come from that key
static constexpr uint32_t PHYSICAL_KEY_REQUEST_WINDOW_MS = 250;

static constexpr int PANEL_MARGIN = 8;
static constexpr int PANEL_PADDING = 8;
static constexpr int KEY_GAP = 4;
static constexpr int MIN_KEY_UNIT = 24;
static constexpr int MAX_KEY_UNIT = 64;

static constexpr size_t NO_KEY = std::numeric_limits<size_t>::max();

class VirtualKeyboardForm;

static bool sameRect(const WzRect& a, const WzRect& b)
{
	return a.x() == b.x() && a.y() == b.y() && a.width() == b.width() && a.height() == b.height();
}

// MARK: - Module state

struct VirtualKeyboardState
{
	VirtualKeyboardMode mode = VirtualKeyboardMode::Automatic;
	std::string layoutPreference = VIRTUAL_KEYBOARD_LAYOUT_AUTO;

	std::shared_ptr<W_SCREEN> screen;
	std::shared_ptr<VirtualKeyboardForm> form;
	bool visible = false;

	// The current editing session
	std::weak_ptr<W_EDITBOX> sessionEditBox;
	bool sessionRequested = false;  // the player asked for the keyboard (code focusing an edit box doesn't count)
	bool sessionShown = false;
	uint32_t physicalKeyPressCount = 0;

	bool nextSessionRequested = false;  // by virtualKeyboardRequestForNextTextEntry()
	uint32_t nextSessionRequestExpiry = 0;

	uint32_t lastSeenPhysicalKeyPressCount = 0;
	uint32_t lastPhysicalKeyPressTime = 0;

	std::map<std::string, wzkb::LayoutData> layoutCache;

	wzkb::AccentData accents;
	bool accentsLoaded = false;
};

static VirtualKeyboardState& state()
{
	static VirtualKeyboardState instance;
	return instance;
}

// MARK: - Layout selection

static std::string languageLayoutId()
{
	const char* language = getLanguage();
	return wzkb::layoutIdForLanguage(language ? language : "");
}

static const wzkb::LayoutData& cachedLayout(const std::string& id)
{
	auto& cache = state().layoutCache;
	auto it = cache.find(id);
	if (it != cache.end())
	{
		return it->second;
	}
	wzkb::LayoutData layout;
	if (!wzkb::loadLayout(id, layout))
	{
		layout = wzkb::fallbackUSLayout();
	}
	return cache.emplace(id, std::move(layout)).first->second;
}

static wzkb::LayoutData systemLayout()
{
	// the keys come from the game language's layout, and the characters from the OS layout where it has them
	const std::string templateId = languageLayoutId();
	const wzkb::LayoutData& templateLayout = cachedLayout(templateId);
	return wzkb::deriveLayout(templateLayout, VIRTUAL_KEYBOARD_LAYOUT_SYSTEM, _("System layout"),
		[](const std::string& xkbName, bool shift, bool altGr) -> uint32_t {
			const utf_32_char character = inputCharacterInCurrentLayout(xkbName.c_str(), shift, altGr);
			if (altGr && character == inputCharacterInCurrentLayout(xkbName.c_str(), shift, false))
			{
				// the OS layout has nothing on AltGr for this key, and reports the plain character
				return 0;
			}
			return character;
		});
}

static wzkb::LayoutData resolveLayout()
{
	std::string preference = state().layoutPreference;
	if (preference == VIRTUAL_KEYBOARD_LAYOUT_AUTO)
	{
		// match a physical keyboard's layout
		preference = inputHasPhysicalKeyboard() ? VIRTUAL_KEYBOARD_LAYOUT_SYSTEM : languageLayoutId();
	}
	if (preference == VIRTUAL_KEYBOARD_LAYOUT_SYSTEM)
	{
		return systemLayout();
	}
	const auto& ids = wzkb::builtinLayoutIds();
	if (std::find(ids.begin(), ids.end(), preference) == ids.end())
	{
		preference = languageLayoutId();
	}
	return cachedLayout(preference);
}

static const wzkb::AccentData& accentData()
{
	auto& vk = state();
	if (!vk.accentsLoaded)
	{
		vk.accentsLoaded = true;
		wzkb::loadAccents(vk.accents);
	}
	return vk.accents;
}

// The languages whose accents a held key offers, in order
static std::vector<std::string> accentLanguages(const wzkb::LayoutData& layout)
{
	std::vector<std::string> languages;
	auto add = [&languages](const std::string& language) {
		if (!language.empty() && std::find(languages.begin(), languages.end(), language) == languages.end())
		{
			languages.push_back(language);
		}
	};
	const char* gameLanguage = getLanguage();
	add(gameLanguage ? gameLanguage : "");
	std::string layoutLanguage = wzkb::languageForLayoutId(layout.id);
	if (layoutLanguage.empty())
	{
		// the system layout is built on the game language's layout
		layoutLanguage = wzkb::languageForLayoutId(languageLayoutId());
	}
	add(layoutLanguage);
	add("en");
	return languages;
}

// MARK: - Gamepad button glyphs

// Textures of the connected controller's button glyphs, made on first use and dropped when the controller changes
class GamepadGlyphCache
{
public:
	gfx_api::texture* glyph(GAMEPAD_INPUT button, unsigned int size)
	{
		const char* guid = gamepadDeviceGUID();
		if (deviceGUID != guid || pixelSize != size)
		{
			textures.clear();
			deviceGUID = guid;
			pixelSize = size;
		}
		auto it = textures.find(button);
		if (it == textures.end())
		{
			// failures are cached too, as nullptr
			it = textures.emplace(button, makeTexture(button, size)).first;
		}
		return it->second.get();
	}

private:
	static std::unique_ptr<gfx_api::texture> makeTexture(GAMEPAD_INPUT button, unsigned int size)
	{
		std::vector<unsigned char> rgba;
		unsigned int width = 0;
		unsigned int height = 0;
		if (!gamepadGetButtonGlyph(button, size, rgba, width, height))
		{
			return nullptr;
		}
		iV_Image image;
		if (!image.allocate(width, height, 4, false))
		{
			return nullptr;
		}
		memcpy(image.bmp_w(), rgba.data(), rgba.size());
		return std::unique_ptr<gfx_api::texture>(gfx_api::context::get().loadTextureFromUncompressedImage(std::move(image), gfx_api::texture_type::user_interface, "virtualkeyboardglyph"));
	}

	std::map<GAMEPAD_INPUT, std::unique_ptr<gfx_api::texture>> textures;
	std::string deviceGUID;
	unsigned int pixelSize = 0;
};

// MARK: - Typing into the edit box

static void injectText(const WzString& text)
{
	for (uint32_t codepoint : text.toUtf32())
	{
		inputInjectText(codepoint);
	}
}

// MARK: - Key widget

class VirtualKeyWidget : public WIDGET
{
public:
	VirtualKeyWidget(VirtualKeyboardForm* owner, size_t index)
		: WIDGET()
		, owner(owner)
		, index(index)
	{ }

	void clicked(W_CONTEXT*, WIDGET_KEY key) override;
	void released(W_CONTEXT*, WIDGET_KEY key) override
	{
		if (key == WKEY_PRIMARY)
		{
			held = false;
		}
	}
	void highlight(W_CONTEXT*) override;
	void highlightLost() override
	{
		hovered = false;
	}
	bool isGamepadCursorMagnetTarget() const override
	{
		return true;
	}
	void display(int xOffset, int yOffset) override;

	bool held = false;
	bool hovered = false;
	uint32_t heldSince = 0;
	uint32_t lastRepeat = 0;
	uint32_t flashUntil = 0;

private:
	VirtualKeyboardForm* owner;
	size_t index;
	WzText labelText;
	WzString cachedLabel;
	iV_fonts cachedFont = font_count;
	WzText accentHintText;
	WzString cachedAccentHint;
};

// MARK: - Pop-up

// One item in the pop-up (a letter offered when a letter key is held)
class PopupItemWidget : public WIDGET
{
public:
	PopupItemWidget(VirtualKeyboardForm* owner, size_t index)
		: WIDGET()
		, owner(owner)
		, index(index)
	{ }

	void clicked(W_CONTEXT*, WIDGET_KEY key) override;
	void highlight(W_CONTEXT*) override;
	void highlightLost() override
	{
		hovered = false;
	}
	bool isGamepadCursorMagnetTarget() const override
	{
		return true;
	}
	void display(int xOffset, int yOffset) override;

	bool hovered = false;

private:
	VirtualKeyboardForm* owner;
	size_t index;
	WzText labelText;
	WzString cachedLabel;
	iV_fonts cachedFont = font_count;
};

// The pop-up's panel, holding its items
class PopupWidget : public WIDGET
{
public:
	PopupWidget() : WIDGET() { }

	void display(int xOffset, int yOffset) override
	{
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		pie_DrawRoundedBox(x0, y0, x0 + static_cast<float>(width()), y0 + static_cast<float>(height()),
			pal_RGBA(20, 24, 36, 250), 6.f, pal_RGBA(170, 190, 240, 255), 1.f);
	}
};

// A gamepad shortcut shown below the keys: its buttons' glyphs (or names) and what they do
struct GamepadHint
{
	std::vector<GAMEPAD_INPUT> buttons;
	WzString label;
};

// MARK: - Keyboard form

class VirtualKeyboardForm : public WIDGET
{
public:
	VirtualKeyboardForm() : WIDGET() { }

	void setLayout(const wzkb::LayoutData& newLayout)
	{
		closePopup();
		longPressKey = NO_KEY;
		layout = newLayout;
		grid = wzkb::buildGrid(layout);
		typing.reset();
		accentLanguageCodes = accentLanguages(layout);
		for (auto& keyWidget : keyWidgets)
		{
			detach(keyWidget);
		}
		keyWidgets.clear();
		for (size_t i = 0; i < grid.size(); ++i)
		{
			auto keyWidget = std::make_shared<VirtualKeyWidget>(this, i);
			attach(keyWidget);
			keyWidgets.push_back(keyWidget);
		}
		focusIndex = defaultFocusIndex();
		layoutDirty = true;
	}

	const wzkb::LayoutData& currentLayout() const { return layout; }
	wzkb::TypingState& typingState() { return typing; }

	size_t defaultFocusIndex() const
	{
		for (size_t i = 0; i < grid.size(); ++i)
		{
			if (grid[i].xkbName == "AD01")
			{
				return i;
			}
		}
		return 0;
	}

	// Positions the keyboard on screen, out of the way of the edit box being edited
	void updateGeometry(const WzRect& editBoxRect)
	{
		const bool showHints = gamepadIsConnected();
		if (!layoutDirty && sameRect(editBoxRect, lastEditBoxRect) && screenWidth == lastScreenWidth && screenHeight == lastScreenHeight && showHints == lastShowHints)
		{
			return;
		}
		layoutDirty = false;
		closePopup();
		lastEditBoxRect = editBoxRect;
		lastScreenWidth = screenWidth;
		lastScreenHeight = screenHeight;
		lastShowHints = showHints;

		const int available = static_cast<int>(screenWidth) - 2 * PANEL_MARGIN - 2 * PANEL_PADDING;
		unit = std::clamp(available / static_cast<int>(wzkb::GRID_WIDTH_UNITS), MIN_KEY_UNIT, MAX_KEY_UNIT);
		keyHeight = std::clamp(unit * 4 / 5, 22, 52);
		labelFont = (unit >= 44) ? font_medium : font_regular;
		echoHeight = std::max(iV_GetTextLineSize(font_regular) + 8, keyHeight * 4 / 5);
		glyphSize = std::clamp(keyHeight / 2, 18, 26);
		hintHeight = showHints ? std::max(iV_GetTextLineSize(font_small), glyphSize) + 2 : 0;

		const int panelWidth = unit * static_cast<int>(wzkb::GRID_WIDTH_UNITS) + 2 * PANEL_PADDING;
		int panelHeight = PANEL_PADDING + echoHeight + KEY_GAP + wzkb::GRID_ROWS * keyHeight + (wzkb::GRID_ROWS - 1) * KEY_GAP + PANEL_PADDING;
		if (hintHeight > 0)
		{
			panelHeight += KEY_GAP + hintHeight;
		}

		const int panelX = std::max(0, (static_cast<int>(screenWidth) - panelWidth) / 2);
		int panelY = static_cast<int>(screenHeight) - PANEL_MARGIN - panelHeight;
		if (editBoxRect.bottom() > panelY && editBoxRect.top() > panelHeight + PANEL_MARGIN)
		{
			// the edit box is where the keyboard would go - move to the top
			panelY = PANEL_MARGIN;
		}
		panelY = std::max(0, panelY);
		setGeometry(panelX, panelY, panelWidth, panelHeight);

		const int keysTop = PANEL_PADDING + echoHeight + KEY_GAP;
		for (size_t i = 0; i < grid.size(); ++i)
		{
			const auto& key = grid[i];
			const int x = PANEL_PADDING + static_cast<int>(key.x * static_cast<float>(unit) + 0.5f);
			const int width = static_cast<int>(key.width * static_cast<float>(unit) + 0.5f) - KEY_GAP;
			const int y = keysTop + key.row * (keyHeight + KEY_GAP);
			keyWidgets[i]->setGeometry(x, y, width, keyHeight);
		}
	}

	WzString keyLabel(size_t index) const
	{
		const auto& key = grid[index];
		switch (key.special)
		{
			case wzkb::SpecialKey::None:
			{
				const auto* levels = layout.key(key.xkbName);
				return levels ? typing.displayLabel(*levels) : WzString();
			}
			case wzkb::SpecialKey::Backspace: return WzString::fromUtf8("⌫");
			case wzkb::SpecialKey::Enter: return WzString::fromUtf8("⏎");
			case wzkb::SpecialKey::Shift:
				return WzString::fromUtf8(typing.shiftState() == wzkb::TypingState::ShiftState::Locked ? "⇪" : "⇧");
			case wzkb::SpecialKey::AltGr: return WzString::fromUtf8("AltGr");
			case wzkb::SpecialKey::Space: return WzString::fromUtf8(_("Space"));
			case wzkb::SpecialKey::CursorLeft: return WzString::fromUtf8("←");
			case wzkb::SpecialKey::CursorRight: return WzString::fromUtf8("→");
			case wzkb::SpecialKey::Paste: return WzString::fromUtf8(_("Paste"));
			case wzkb::SpecialKey::Hide: return WzString::fromUtf8("⌨ ↓");
		}
		return WzString();
	}

	bool keyIsSpecial(size_t index) const { return grid[index].special != wzkb::SpecialKey::None; }

	bool keyIsLatched(size_t index) const
	{
		switch (grid[index].special)
		{
			case wzkb::SpecialKey::Shift: return typing.shiftState() != wzkb::TypingState::ShiftState::Off;
			case wzkb::SpecialKey::AltGr: return typing.altGrActive();
			default: return false;
		}
	}

	bool keyIsLocked(size_t index) const
	{
		return grid[index].special == wzkb::SpecialKey::Shift && typing.shiftState() == wzkb::TypingState::ShiftState::Locked;
	}

	bool keyRepeats(size_t index) const
	{
		switch (grid[index].special)
		{
			case wzkb::SpecialKey::Backspace:
			case wzkb::SpecialKey::CursorLeft:
			case wzkb::SpecialKey::CursorRight:
				return true;
			default:
				return false;
		}
	}

	bool keyHasFocus(size_t index) const
	{
		return index == focusIndex && isGamepadActiveInput();
	}

	void keyHovered(size_t index)
	{
		focusIndex = index;
	}

	// The letters a held key offers, besides its own
	std::vector<WzString> keyVariants(size_t index) const
	{
		if (index >= grid.size() || grid[index].special != wzkb::SpecialKey::None)
		{
			return {};
		}
		const auto* levels = layout.key(grid[index].xkbName);
		if (!levels)
		{
			return {};
		}
		const wzkb::KeyOutput& output = typing.currentOutput(*levels);
		if (output.isDead() || output.text.isEmpty())
		{
			return {};
		}
		return wzkb::accentVariants(accentData(), accentLanguageCodes, output.text);
	}

	// A key was pressed (by mouse, touch, or South on the gamepad cursor)
	void keyPressed(size_t index)
	{
		if (popupOpen())
		{
			// a press anywhere else dismisses the pop-up
			closePopup();
			return;
		}
		focusIndex = index;
		if (!keyVariants(index).empty())
		{
			// types on release, unless held for its accents
			longPressKey = index;
			return;
		}
		activateKey(index);
	}

	void activateKey(size_t index)
	{
		if (index >= grid.size())
		{
			return;
		}
		const auto& key = grid[index];
		switch (key.special)
		{
			case wzkb::SpecialKey::None:
				if (const auto* levels = layout.key(key.xkbName))
				{
					injectText(typing.pressCharacterKey(*levels));
				}
				break;
			case wzkb::SpecialKey::Backspace:
				if (!typing.cancelPendingDeadKey())
				{
					inputInjectEditingKey(KEY_BACKSPACE);
				}
				break;
			case wzkb::SpecialKey::Enter:
				if (typing.hasPendingDeadKey())
				{
					injectText(typing.pendingDeadLabel());
					typing.cancelPendingDeadKey();
				}
				inputInjectEditingKey(KEY_RETURN);
				break;
			case wzkb::SpecialKey::Shift:
				typing.pressShift();
				break;
			case wzkb::SpecialKey::AltGr:
				typing.toggleAltGr();
				break;
			case wzkb::SpecialKey::Space:
				injectText(typing.pressSpace());
				break;
			case wzkb::SpecialKey::CursorLeft:
				inputInjectEditingKey(KEY_LEFTARROW);
				break;
			case wzkb::SpecialKey::CursorRight:
				inputInjectEditingKey(KEY_RIGHTARROW);
				break;
			case wzkb::SpecialKey::Paste:
				inputInjectPaste();
				break;
			case wzkb::SpecialKey::Hide:
				state().sessionRequested = false;
				break;
		}
	}

	// Triggers a special key from a gamepad shortcut, lighting it up
	void activateSpecialKey(wzkb::SpecialKey special)
	{
		if (popupOpen())
		{
			// Backspace backs out of the pop-up
			const bool cancel = (special == wzkb::SpecialKey::Backspace);
			closePopup(cancel);
			if (cancel)
			{
				return;
			}
		}
		for (size_t i = 0; i < grid.size(); ++i)
		{
			if (grid[i].special == special)
			{
				keyWidgets[i]->flashUntil = wzGetTicks() + KEY_FLASH_MS;
				activateKey(i);
				return;
			}
		}
	}

	// Moves the gamepad focus (and the cursor) to the next key in a direction
	void moveFocus(int dx, int dy)
	{
		if (popupOpen())
		{
			if (dx != 0)
			{
				movePopupFocus(dx);
			}
			return;
		}
		if (grid.empty())
		{
			return;
		}
		if (focusIndex >= grid.size())
		{
			focusIndex = defaultFocusIndex();
		}
		else
		{
			focusIndex = wzkb::navigateGrid(grid, focusIndex, dx, dy);
		}
		moveCursorToFocus();
	}

	void moveCursorToFocus()
	{
		if (focusIndex >= keyWidgets.size())
		{
			return;
		}
		const auto& keyWidget = keyWidgets[focusIndex];
		gamepadCursorJumpTo(keyWidget->screenPosX() + keyWidget->width() / 2, keyWidget->screenPosY() + keyWidget->height() / 2);
	}

	// Long presses, key repeat, and releasing keys whose button went up off the key
	void updateHeldKeys()
	{
		const bool buttonDown = mouseDown(MOUSE_LMB) || gamepadSyntheticClickHeld();
		const uint32_t now = wzGetTicks();

		if (longPressKey < keyWidgets.size())
		{
			auto& keyWidget = keyWidgets[longPressKey];
			const bool stillHeld = keyWidget->held && buttonDown;
			if (stillHeld && !popupOpen() && now - keyWidget->heldSince >= LONG_PRESS_MS)
			{
				openPopup(longPressKey);
			}
			else if (!stillHeld)
			{
				keyWidget->held = false;
				const size_t key = longPressKey;
				longPressKey = NO_KEY;
				if (!popupOpen())
				{
					// a tap
					activateKey(key);
				}
				else
				{
					// released over a letter types it, and anywhere else leaves the pop-up open
					const size_t item = popupItemAt(mouseX(), mouseY());
					if (item != NO_KEY)
					{
						chooseAccent(item);
					}
				}
			}
		}
		else if (popupOpen() && mousePressed(MOUSE_LMB) && popupItemAt(mouseX(), mouseY()) == NO_KEY
			&& !geometry().contains(mouseX(), mouseY()))
		{
			// pressed outside the keyboard
			closePopup();
		}
		for (size_t i = 0; i < keyWidgets.size(); ++i)
		{
			auto& keyWidget = keyWidgets[i];
			if (!keyWidget->held || i == longPressKey)
			{
				continue;
			}
			if (!buttonDown)
			{
				keyWidget->held = false;
				continue;
			}
			if (keyRepeats(i) && now - keyWidget->heldSince >= KEY_REPEAT_DELAY_MS && now - keyWidget->lastRepeat >= KEY_REPEAT_INTERVAL_MS)
			{
				keyWidget->lastRepeat = now;
				activateKey(i);
			}
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
			pal_RGBA(12, 14, 22, 235), 8.f, pal_RGBA(90, 100, 130, 255), 1.f);
		displayEchoLine(x0 + PANEL_PADDING, y0 + PANEL_PADDING, width() - 2 * PANEL_PADDING, echoHeight);
		if (hintHeight > 0)
		{
			displayHints(x0 + PANEL_PADDING, y0 + height() - PANEL_PADDING - hintHeight, width() - 2 * PANEL_PADDING);
		}
	}

	iV_fonts keyLabelFont() const { return labelFont; }

	// The first letter a held key would offer, shown small in its corner
	WzString accentHint(size_t index) const
	{
		if (keyHeight < 30)
		{
			return WzString();  // no room
		}
		const auto variants = keyVariants(index);
		return variants.empty() ? WzString() : variants.front();
	}

	// MARK: Pop-up

	bool popupOpen() const { return popup != nullptr; }

	// Opens the pop-up of a held key's letters, above the key
	void openPopup(size_t keyIndex)
	{
		closePopup();
		const auto* levels = layout.key(grid[keyIndex].xkbName);
		if (!levels)
		{
			return;
		}
		popupLabels.clear();
		popupLabels.push_back(typing.currentOutput(*levels).text);
		for (const auto& variant : keyVariants(keyIndex))
		{
			popupLabels.push_back(variant);
		}
		const int itemWidth = std::max(unit - KEY_GAP, keyHeight);
		const size_t maxItems = static_cast<size_t>(std::max(2, (width() - 2 * PANEL_PADDING) / (itemWidth + KEY_GAP)));
		if (popupLabels.size() > maxItems)
		{
			popupLabels.resize(maxItems);
		}

		const auto& key = keyWidgets[keyIndex];
		const int count = static_cast<int>(popupLabels.size());
		const int popupWidth = count * itemWidth + (count - 1) * KEY_GAP + 2 * KEY_GAP;
		const int popupHeight = keyHeight + 2 * KEY_GAP;
		int popupX = key->x() + key->width() / 2 - popupWidth / 2;
		popupX = std::clamp(popupX, PANEL_PADDING / 2, std::max(PANEL_PADDING / 2, width() - PANEL_PADDING / 2 - popupWidth));
		const int popupY = std::max(2, key->y() - popupHeight - KEY_GAP / 2);

		popup = std::make_shared<PopupWidget>();
		popup->setGeometry(popupX, popupY, popupWidth, popupHeight);
		for (int i = 0; i < count; ++i)
		{
			auto item = std::make_shared<PopupItemWidget>(this, static_cast<size_t>(i));
			item->setGeometry(KEY_GAP + i * (itemWidth + KEY_GAP), KEY_GAP, itemWidth, keyHeight);
			popup->attach(item);
			popupItems.push_back(item);
		}
		// last, so it draws over the keys and gets their clicks
		attach(popup);
		popupKey = keyIndex;
		popupFocus = (count > 1) ? 1 : 0;
		if (isGamepadActiveInput())
		{
			// onto the first accented letter - releasing South there types it
			moveCursorToPopupFocus();
		}
	}

	// returnFocus: put the gamepad cursor back on the key the pop-up came from
	void closePopup(bool returnFocus = false)
	{
		if (!popup)
		{
			return;
		}
		detach(popup);
		popup.reset();
		popupItems.clear();
		popupLabels.clear();
		// a key still held for the pop-up doesn't type when it's released
		longPressKey = NO_KEY;
		if (returnFocus && isGamepadActiveInput() && popupKey < keyWidgets.size())
		{
			focusIndex = popupKey;
			moveCursorToFocus();
		}
		popupKey = NO_KEY;
	}

	void chooseAccent(size_t item)
	{
		if (item >= popupLabels.size())
		{
			return;
		}
		const WzString letter = popupLabels[item];
		closePopup(true);
		injectText(typing.pressVariant(letter));
	}

	void popupItemHovered(size_t item)
	{
		popupFocus = item;
	}

	size_t popupItemAt(int screenX, int screenY) const
	{
		for (size_t i = 0; i < popupItems.size(); ++i)
		{
			const auto& item = popupItems[i];
			if (WzRect(item->screenPosX(), item->screenPosY(), item->width(), item->height()).contains(screenX, screenY))
			{
				return i;
			}
		}
		return NO_KEY;
	}

	const WzString& popupLabel(size_t item) const { return popupLabels[item]; }

	bool popupItemHasFocus(size_t item) const
	{
		return item == popupFocus && isGamepadActiveInput();
	}

private:
	void movePopupFocus(int dx)
	{
		if (popupItems.empty())
		{
			return;
		}
		const int last = static_cast<int>(popupItems.size()) - 1;
		popupFocus = static_cast<size_t>(std::clamp(static_cast<int>(popupFocus) + dx, 0, last));
		moveCursorToPopupFocus();
	}

	void moveCursorToPopupFocus()
	{
		if (popupFocus >= popupItems.size())
		{
			return;
		}
		const auto& item = popupItems[popupFocus];
		gamepadCursorJumpTo(item->screenPosX() + item->width() / 2, item->screenPosY() + item->height() / 2);
	}

	// Shows the text being edited
	void displayEchoLine(int x0, int y0, int boxWidth, int boxHeight)
	{
		pie_DrawRoundedBox(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x0 + boxWidth), static_cast<float>(y0 + boxHeight),
			pal_RGBA(4, 5, 9, 255), 4.f, pal_RGBA(60, 68, 90, 255), 1.f);

		auto editBox = state().sessionEditBox.lock();
		if (!editBox)
		{
			return;
		}
		const WzString text = editBox->getString();
		const int caret = std::clamp(editBox->insPos, 0, text.length());
		const int textPadding = 6;
		const int maxTextWidth = boxWidth - 2 * textPadding;

		// scroll so the caret stays in view
		int start = 0;
		while (start < caret && static_cast<int>(iV_GetTextWidth(text.substr(static_cast<size_t>(start), static_cast<size_t>(caret - start)), font_regular)) > maxTextWidth - 12)
		{
			++start;
		}
		const WzString visible = text.substr(static_cast<size_t>(start));
		const int caretOffset = static_cast<int>(iV_GetTextWidth(text.substr(static_cast<size_t>(start), static_cast<size_t>(caret - start)), font_regular));

		const int textX = x0 + textPadding;
		const int textY = y0 + (boxHeight - iV_GetTextLineSize(font_regular)) / 2 - iV_GetTextAboveBase(font_regular);
		if (echoCached != visible)
		{
			echoCached = visible;
			echoText.setText(visible, font_regular);
		}
		echoText.render(static_cast<float>(textX), static_cast<float>(textY), WZCOL_TEXT_BRIGHT, 0.f, maxTextWidth);

		// the caret, followed by a pending dead key
		const int caretX = textX + caretOffset;
		const int caretTop = y0 + 4;
		const int caretBottom = y0 + boxHeight - 4;
		pie_BoxFill(caretX, caretTop, caretX + 2, caretBottom, WZCOL_FORM_CURSOR);
		if (typing.hasPendingDeadKey())
		{
			if (pendingCached != typing.pendingDeadLabel())
			{
				pendingCached = typing.pendingDeadLabel();
				pendingText.setText(pendingCached, font_regular);
			}
			pendingText.render(static_cast<float>(caretX + 3), static_cast<float>(textY), WZCOL_YELLOW);
		}
	}

	std::vector<GamepadHint> currentHints() const
	{
		auto text = [](const char* utf8) { return WzString::fromUtf8(utf8); };
		std::vector<GamepadHint> hints;
		if (popupOpen())
		{
			hints.push_back({ { GPAD_BTN_DPAD_LEFT, GPAD_BTN_DPAD_RIGHT }, text(_("Choose")) });
			hints.push_back({ { GPAD_BTN_SOUTH }, text(_("Type")) });
			hints.push_back({ { GPAD_BTN_EAST }, text(_("Cancel")) });
			return hints;
		}
		hints.push_back({ { GPAD_BTN_EAST }, text("⌫") });
		hints.push_back({ { GPAD_BTN_WEST }, text("␣") });
		hints.push_back({ { GPAD_BTN_NORTH }, text("⇧") });
		hints.push_back({ { GPAD_BTN_LEFT_SHOULDER, GPAD_BTN_RIGHT_SHOULDER }, text("← →") });
		if (layout.hasAltGrLevel())
		{
			hints.push_back({ { GPAD_BTN_LEFT_TRIGGER }, text("AltGr") });
		}
		hints.push_back({ { GPAD_BTN_START }, text("⏎") });
		hints.push_back({ { GPAD_BTN_BACK }, text(_("Hide")) });
		if (anyKeyHasAccents())
		{
			// TRANSLATORS: shown after a gamepad button: hold it on a letter of the on-screen keyboard for accented letters
			hints.push_back({ { GPAD_BTN_SOUTH }, text(_("Hold: Accents")) });
		}
		return hints;
	}

	bool anyKeyHasAccents() const
	{
		for (size_t i = 0; i < grid.size(); ++i)
		{
			if (!keyVariants(i).empty())
			{
				return true;
			}
		}
		return false;
	}

	// Gamepad shortcuts, with the connected controller's button glyphs
	void displayHints(int x0, int y0, int hintWidth)
	{
		const auto hints = currentHints();
		constexpr int GLYPH_GAP = 2;
		constexpr int LABEL_GAP = 4;
		constexpr int HINT_GAP = 14;

		// lay out the hints that fit
		struct Placed { const GamepadHint* hint; WzText* label; int width; };
		std::vector<Placed> placed;
		int totalWidth = 0;
		hintTexts.resize(hints.size());
		hintLabelsCached.resize(hints.size());
		for (size_t i = 0; i < hints.size(); ++i)
		{
			const auto& hint = hints[i];
			if (!(hintLabelsCached[i] == hint.label))
			{
				hintLabelsCached[i] = hint.label;
				hintTexts[i].setText(hint.label, font_small);
			}
			int width = hintTexts[i].width() + LABEL_GAP;
			for (auto button : hint.buttons)
			{
				width += buttonWidth(button) + GLYPH_GAP;
			}
			const int widthWithGap = width + (placed.empty() ? 0 : HINT_GAP);
			if (totalWidth + widthWithGap > hintWidth)
			{
				break;
			}
			totalWidth += widthWithGap;
			placed.push_back({ &hint, &hintTexts[i], width });
		}

		int x = x0 + std::max(0, (hintWidth - totalWidth) / 2);
		const int textY = y0 + (hintHeight - iV_GetTextLineSize(font_small)) / 2 - iV_GetTextAboveBase(font_small);
		for (const auto& entry : placed)
		{
			for (auto button : entry.hint->buttons)
			{
				if (gfx_api::texture* glyph = glyphCache.glyph(button, static_cast<unsigned int>(glyphSize * 2)))
				{
					iV_DrawImageAnisotropic(*glyph, Vector2i(x, y0 + (hintHeight - glyphSize) / 2), Vector2f(0.f, 0.f), Vector2f(static_cast<float>(glyphSize), static_cast<float>(glyphSize)), 0.f, WZCOL_WHITE);
				}
				else
				{
					buttonNameText(button).render(static_cast<float>(x), static_cast<float>(textY), WZCOL_TEXT_BRIGHT);
				}
				x += buttonWidth(button) + GLYPH_GAP;
			}
			entry.label->render(static_cast<float>(x + LABEL_GAP), static_cast<float>(textY), WZCOL_TEXT_MEDIUM);
			x += LABEL_GAP + entry.label->width() + HINT_GAP;
		}
	}

	// A button's glyph width, or its name's when there is no glyph
	int buttonWidth(GAMEPAD_INPUT button)
	{
		if (glyphCache.glyph(button, static_cast<unsigned int>(glyphSize * 2)))
		{
			return glyphSize;
		}
		return buttonNameText(button).width();
	}

	WzText& buttonNameText(GAMEPAD_INPUT button)
	{
		// names follow the connected controller (ex. A or Cross)
		const char* name = gamepadButtonName(button);
		auto& entry = buttonNames[button];
		if (entry.first != name)
		{
			entry.first = name;
			entry.second.setText(WzString::fromUtf8(name), font_small);
		}
		return entry.second;
	}

	wzkb::LayoutData layout;
	std::vector<wzkb::GridKey> grid;
	std::vector<std::shared_ptr<VirtualKeyWidget>> keyWidgets;
	wzkb::TypingState typing;
	size_t focusIndex = 0;

	bool layoutDirty = true;
	WzRect lastEditBoxRect;
	unsigned lastScreenWidth = 0;
	unsigned lastScreenHeight = 0;
	bool lastShowHints = false;
	int unit = MIN_KEY_UNIT;
	int keyHeight = 22;
	int echoHeight = 22;
	int hintHeight = 0;
	int glyphSize = 18;
	iV_fonts labelFont = font_regular;

	WzText echoText;
	WzString echoCached;
	WzText pendingText;
	WzString pendingCached;
	std::vector<WzText> hintTexts;
	std::vector<WzString> hintLabelsCached;
	std::map<GAMEPAD_INPUT, std::pair<std::string, WzText>> buttonNames;
	GamepadGlyphCache glyphCache;

	std::vector<std::string> accentLanguageCodes;
	size_t longPressKey = NO_KEY;  // a key with accents being pressed, not typed yet
	std::shared_ptr<PopupWidget> popup;
	std::vector<std::shared_ptr<PopupItemWidget>> popupItems;
	std::vector<WzString> popupLabels;
	size_t popupKey = NO_KEY;
	size_t popupFocus = 0;
};

void VirtualKeyWidget::clicked(W_CONTEXT*, WIDGET_KEY key)
{
	if (key != WKEY_PRIMARY)
	{
		return;
	}
	held = true;
	heldSince = lastRepeat = wzGetTicks();
	owner->keyPressed(index);
}

void VirtualKeyWidget::highlight(W_CONTEXT*)
{
	hovered = true;
	owner->keyHovered(index);
}

void VirtualKeyWidget::display(int xOffset, int yOffset)
{
	const float x0 = static_cast<float>(xOffset + x());
	const float y0 = static_cast<float>(yOffset + y());
	const float x1 = x0 + static_cast<float>(width());
	const float y1 = y0 + static_cast<float>(height());

	const bool special = owner->keyIsSpecial(index);
	const bool flashing = wzGetTicks() < flashUntil;
	const bool focused = owner->keyHasFocus(index);
	PIELIGHT fill = special ? pal_RGBA(32, 37, 52, 255) : pal_RGBA(46, 52, 70, 255);
	if (owner->keyIsLocked(index))
	{
		fill = pal_RGBA(40, 140, 70, 255);
	}
	else if (owner->keyIsLatched(index))
	{
		fill = pal_RGBA(60, 110, 70, 255);
	}
	if (held || flashing)
	{
		fill = pal_RGBA(100, 120, 170, 255);
	}
	else if (hovered || focused)
	{
		fill = pal_RGBA(70, 82, 112, 255);
	}
	const bool outlined = focused || hovered;
	pie_DrawRoundedBox(x0, y0, x1, y1, fill, 4.f, outlined ? pal_RGBA(170, 190, 240, 255) : pal_RGBA(70, 78, 100, 255), outlined ? 2.f : 1.f);

	const WzString label = owner->keyLabel(index);
	const iV_fonts font = owner->keyLabelFont();
	if (label.isEmpty())
	{
		return;
	}
	if (label != cachedLabel || font != cachedFont)
	{
		cachedLabel = label;
		cachedFont = font;
		labelText.setText(label, font);
	}
	const int maxLabelWidth = width() - 6;
	const float textX = x0 + static_cast<float>(std::max(3, (width() - labelText.width()) / 2));
	const float textY = y0 + static_cast<float>((height() - labelText.lineSize()) / 2 - labelText.aboveBase());
	labelText.render(textX, textY, WZCOL_TEXT_BRIGHT, 0.f, maxLabelWidth);

	// a hint that holding the key offers accents
	const WzString accentHint = special ? WzString() : owner->accentHint(index);
	if (!accentHint.isEmpty())
	{
		if (!(accentHint == cachedAccentHint))
		{
			cachedAccentHint = accentHint;
			accentHintText.setText(accentHint, font_small);
		}
		accentHintText.render(x1 - static_cast<float>(accentHintText.width() + 3), y0 + 2.f - static_cast<float>(accentHintText.aboveBase()), pal_RGBA(140, 150, 180, 255));
	}
}

// MARK: - Pop-up items

void PopupItemWidget::clicked(W_CONTEXT*, WIDGET_KEY key)
{
	if (key == WKEY_PRIMARY)
	{
		owner->chooseAccent(index);
	}
}

void PopupItemWidget::highlight(W_CONTEXT*)
{
	hovered = true;
	owner->popupItemHovered(index);
}

void PopupItemWidget::display(int xOffset, int yOffset)
{
	const float x0 = static_cast<float>(xOffset + x());
	const float y0 = static_cast<float>(yOffset + y());
	const float x1 = x0 + static_cast<float>(width());
	const float y1 = y0 + static_cast<float>(height());
	const bool focused = owner->popupItemHasFocus(index);
	const bool active = focused || hovered;
	pie_DrawRoundedBox(x0, y0, x1, y1, active ? pal_RGBA(100, 120, 170, 255) : pal_RGBA(46, 52, 70, 255), 4.f,
		active ? pal_RGBA(220, 230, 255, 255) : pal_RGBA(70, 78, 100, 255), active ? 2.f : 1.f);

	const WzString& label = owner->popupLabel(index);
	const iV_fonts font = owner->keyLabelFont();
	if (!(label == cachedLabel) || font != cachedFont)
	{
		cachedLabel = label;
		cachedFont = font;
		labelText.setText(label, font);
	}
	const float textX = x0 + static_cast<float>(std::max(3, (width() - labelText.width()) / 2));
	const float textY = y0 + static_cast<float>((height() - labelText.lineSize()) / 2 - labelText.aboveBase());
	labelText.render(textX, textY, WZCOL_TEXT_BRIGHT, 0.f, width() - 6);
}

// MARK: - Showing and hiding

static void showKeyboard(const std::shared_ptr<W_EDITBOX>& editBox)
{
	auto& vk = state();
	if (!vk.screen)
	{
		vk.screen = W_SCREEN::make();
		// hiding the root form stops it from taking clicks, without hiding its children
		vk.screen->psForm->hide();
		vk.form = std::make_shared<VirtualKeyboardForm>();
		vk.screen->psForm->attach(vk.form);
	}
	if (!vk.visible)
	{
		vk.form->setLayout(resolveLayout());
		vk.form->updateGeometry(WzRect(editBox->screenPosX(), editBox->screenPosY(), editBox->width(), editBox->height()));
		// just below the gamepad cursor, which uses the maximum z-order
		widgRegisterOverlayScreen(vk.screen, std::numeric_limits<uint16_t>::max() - 1);
		gamepadSetTextEntryMode(true);
		vk.visible = true;
		vk.sessionShown = true;
		if (isGamepadActiveInput())
		{
			vk.form->moveCursorToFocus();
		}
		debug(LOG_INPUT, "On-screen keyboard shown (%s)", vk.form->currentLayout().id.c_str());
	}
}

static void hideKeyboard()
{
	auto& vk = state();
	if (!vk.visible)
	{
		return;
	}
	vk.form->closePopup();
	widgRemoveOverlayScreen(vk.screen);
	gamepadSetTextEntryMode(false);
	vk.visible = false;
	debug(LOG_INPUT, "On-screen keyboard hidden");
}

// Gamepad typing shortcuts, besides pressing the key under the cursor with South
static void handleGamepadInput()
{
	auto& form = *state().form;
	const uint32_t now = wzGetTicks();

	struct RepeatingButton
	{
		GAMEPAD_INPUT button;
		uint32_t delay;
		uint32_t interval;
		std::function<void ()> action;
	};
	static uint32_t pressedAt[GPAD_BTN_MAX] = {};
	static uint32_t repeatedAt[GPAD_BTN_MAX] = {};
	const RepeatingButton repeatingButtons[] = {
		{ GPAD_BTN_EAST, KEY_REPEAT_DELAY_MS, KEY_REPEAT_INTERVAL_MS, [&form]() { form.activateSpecialKey(wzkb::SpecialKey::Backspace); } },
		{ GPAD_BTN_LEFT_SHOULDER, KEY_REPEAT_DELAY_MS, KEY_REPEAT_INTERVAL_MS, [&form]() { form.activateSpecialKey(wzkb::SpecialKey::CursorLeft); } },
		{ GPAD_BTN_RIGHT_SHOULDER, KEY_REPEAT_DELAY_MS, KEY_REPEAT_INTERVAL_MS, [&form]() { form.activateSpecialKey(wzkb::SpecialKey::CursorRight); } },
		{ GPAD_BTN_DPAD_UP, NAV_REPEAT_DELAY_MS, NAV_REPEAT_INTERVAL_MS, [&form]() { form.moveFocus(0, -1); } },
		{ GPAD_BTN_DPAD_DOWN, NAV_REPEAT_DELAY_MS, NAV_REPEAT_INTERVAL_MS, [&form]() { form.moveFocus(0, 1); } },
		{ GPAD_BTN_DPAD_LEFT, NAV_REPEAT_DELAY_MS, NAV_REPEAT_INTERVAL_MS, [&form]() { form.moveFocus(-1, 0); } },
		{ GPAD_BTN_DPAD_RIGHT, NAV_REPEAT_DELAY_MS, NAV_REPEAT_INTERVAL_MS, [&form]() { form.moveFocus(1, 0); } },
	};
	for (const auto& entry : repeatingButtons)
	{
		if (gamepadButtonPressed(entry.button))
		{
			pressedAt[entry.button] = repeatedAt[entry.button] = now;
			entry.action();
		}
		else if (gamepadButtonDown(entry.button) && now - pressedAt[entry.button] >= entry.delay && now - repeatedAt[entry.button] >= entry.interval)
		{
			repeatedAt[entry.button] = now;
			entry.action();
		}
	}

	if (gamepadButtonPressed(GPAD_BTN_WEST))
	{
		form.activateSpecialKey(wzkb::SpecialKey::Space);
	}
	if (gamepadButtonPressed(GPAD_BTN_NORTH))
	{
		form.activateSpecialKey(wzkb::SpecialKey::Shift);
	}
	if (gamepadButtonPressed(GPAD_BTN_LEFT_TRIGGER) && form.currentLayout().hasAltGrLevel())
	{
		form.activateSpecialKey(wzkb::SpecialKey::AltGr);
	}
	if (gamepadButtonPressed(GPAD_BTN_START))
	{
		form.activateSpecialKey(wzkb::SpecialKey::Enter);
	}
	if (gamepadButtonPressed(GPAD_BTN_BACK))
	{
		if (form.popupOpen())
		{
			form.closePopup(true);
		}
		else
		{
			state().sessionRequested = false;
		}
	}
}

// Whether the keyboard should show for the current editing session
static bool wantKeyboard()
{
	auto& vk = state();
	if (!vk.sessionRequested)
	{
		return false;
	}
	switch (vk.mode)
	{
		case VirtualKeyboardMode::BuiltIn:
			return true;
		case VirtualKeyboardMode::Automatic:
			// once shown it stays for the session, even if the mouse moves
			return vk.sessionShown
				|| ((isGamepadActiveInput() || inputLastPointerInputWasTouch()) && !wzSystemScreenKeyboardWillShow());
		case VirtualKeyboardMode::System:
		case VirtualKeyboardMode::Disabled:
			return false;
	}
	return false;
}

static void applySystemKeyboardPolicy()
{
	// the built-in keyboard replaces platform keyboards in these modes
	const auto mode = state().mode;
	wzSetSystemScreenKeyboardSuppressed(mode == VirtualKeyboardMode::BuiltIn || mode == VirtualKeyboardMode::Disabled);
}

// MARK: - Public interface

void virtualKeyboardInit()
{
	applySystemKeyboardPolicy();
}

void virtualKeyboardShutdown()
{
	hideKeyboard();
	auto& vk = state();
	vk.form.reset();
	vk.screen.reset();
	vk.sessionEditBox.reset();
	vk.layoutCache.clear();
}

void virtualKeyboardSetMode(VirtualKeyboardMode mode)
{
	state().mode = mode;
	applySystemKeyboardPolicy();
}

VirtualKeyboardMode virtualKeyboardGetMode()
{
	return state().mode;
}

void virtualKeyboardSetLayoutPreference(const std::string& preference)
{
	auto& vk = state();
	if (vk.layoutPreference == preference)
	{
		return;
	}
	vk.layoutPreference = preference;
	if (vk.visible)
	{
		vk.form->setLayout(resolveLayout());
	}
}

const std::string& virtualKeyboardGetLayoutPreference()
{
	return state().layoutPreference;
}

std::vector<std::pair<std::string, WzString>> virtualKeyboardLayoutChoices()
{
	std::vector<std::pair<std::string, WzString>> choices;
	choices.emplace_back(VIRTUAL_KEYBOARD_LAYOUT_AUTO, WzString::fromUtf8(_("Automatic")));
	choices.emplace_back(VIRTUAL_KEYBOARD_LAYOUT_SYSTEM, WzString::fromUtf8(_("System layout")));
	for (const auto& id : wzkb::builtinLayoutIds())
	{
		choices.emplace_back(id, cachedLayout(id).name);
	}
	return choices;
}

static void notePhysicalKeyPresses()
{
	auto& vk = state();
	const uint32_t count = inputPhysicalKeyPressCount();
	if (count != vk.lastSeenPhysicalKeyPressCount)
	{
		vk.lastSeenPhysicalKeyPressCount = count;
		vk.lastPhysicalKeyPressTime = wzGetTicks();
	}
}

void virtualKeyboardRequestForNextTextEntry()
{
	auto& vk = state();
	notePhysicalKeyPresses();
	const uint32_t now = wzGetTicks();
	if (vk.lastPhysicalKeyPressTime != 0 && now - vk.lastPhysicalKeyPressTime <= PHYSICAL_KEY_REQUEST_WINDOW_MS)
	{
		vk.nextSessionRequested = false;
		return;
	}
	vk.nextSessionRequested = true;
	vk.nextSessionRequestExpiry = now + NEXT_SESSION_REQUEST_MS;
}

void virtualKeyboardUpdate()
{
	auto& vk = state();
	notePhysicalKeyPresses();
	auto editBox = W_EDITBOX::currentEditBox();
	if (!editBox || vk.mode == VirtualKeyboardMode::System || vk.mode == VirtualKeyboardMode::Disabled)
	{
		hideKeyboard();
		vk.sessionEditBox.reset();
		return;
	}

	const bool editBoxPressed = mousePressed(MOUSE_LMB)
		&& WzRect(editBox->screenPosX(), editBox->screenPosY(), editBox->width(), editBox->height()).contains(mouseX(), mouseY());
	if (vk.sessionEditBox.lock() != editBox)
	{
		// a new editing session
		const bool requestedByAction = vk.nextSessionRequested && wzGetTicks() <= vk.nextSessionRequestExpiry;
		vk.nextSessionRequested = false;
		vk.sessionEditBox = editBox;
		vk.sessionRequested = editBoxPressed || requestedByAction;
		vk.sessionShown = false;
		vk.physicalKeyPressCount = inputPhysicalKeyPressCount();
		hideKeyboard();
	}

	// typing on a physical keyboard withdraws the request
	const uint32_t physicalKeyPresses = inputPhysicalKeyPressCount();
	if (physicalKeyPresses != vk.physicalKeyPressCount)
	{
		vk.physicalKeyPressCount = physicalKeyPresses;
		vk.sessionRequested = false;
		vk.sessionShown = false;
	}
	else if (editBoxPressed)
	{
		// pressing the edit box (again) asks for the keyboard
		vk.sessionRequested = true;
	}

	if (!wantKeyboard())
	{
		hideKeyboard();
		return;
	}
	showKeyboard(editBox);

	vk.form->updateGeometry(WzRect(editBox->screenPosX(), editBox->screenPosY(), editBox->width(), editBox->height()));
	vk.form->updateHeldKeys();
}

void virtualKeyboardProcessGamepadInput()
{
	auto& vk = state();
	if (!vk.visible || !vk.form || !isGamepadActiveInput())
	{
		return;
	}
	handleGamepadInput();
	// (a shortcut that dismissed the keyboard takes effect in virtualKeyboardUpdate)
}

bool virtualKeyboardIsVisible()
{
	return state().visible;
}

bool virtualKeyboardContainsScreenPoint(int x, int y)
{
	auto& vk = state();
	if (!vk.visible || !vk.form)
	{
		return false;
	}
	return vk.form->geometry().contains(x, y);
}
