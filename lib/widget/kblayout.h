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
 *  On-screen keyboard layouts and typing logic.
 *
 *  Layouts come from the JSON files in data/base/keyboards (generated from xkeyboard-config),
 *  or from the current OS keyboard layout.
 *  NOTE: Keep this free of rendering and widget dependencies (it's unit tested on its own).
 */

#ifndef __INCLUDED_LIB_WIDGET_KBLAYOUT_H__
#define __INCLUDED_LIB_WIDGET_KBLAYOUT_H__

#include "lib/framework/wzstring.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace wzkb
{

// Shift levels, in xkeyboard-config order
enum class Level : uint8_t
{
	Base = 0,
	Shift = 1,
	AltGr = 2,
	AltGrShift = 3,
};
constexpr size_t NUM_LEVELS = 4;

// What one key produces on one shift level
struct KeyOutput
{
	WzString text;          // the characters typed (empty for none, or for a dead key)
	uint32_t deadMark = 0;  // the combining mark, for a dead key
	WzString label;         // what the key shows (the text, or a dead key's spacing form)

	bool isEmpty() const { return text.isEmpty() && deadMark == 0; }
	bool isDead() const { return deadMark != 0; }
};

using KeyLevels = std::array<KeyOutput, NUM_LEVELS>;

struct LayoutData
{
	std::string id;
	WzString name;
	bool iso = false;  // ISO geometry: backslash key on the home row, extra key before Z
	std::unordered_map<std::string, KeyLevels> keys;  // by XKB key name (ex. "AD01")

	const KeyLevels* key(const std::string& xkbName) const;
	bool hasAltGrLevel() const;
};

// Ids of the layouts in data/base/keyboards
const std::vector<std::string>& builtinLayoutIds();

// The default layout id for a game language code (ex. "de", "fr_FR"), or "us" if none matches
std::string layoutIdForLanguage(const std::string& languageCode);

// Parses a layout file (see data/base/keyboards/README.md). Returns false and fills error on failure
bool parseLayoutJSON(const char* data, size_t length, LayoutData& output, std::string* error = nullptr);

// Loads data/base/keyboards/<id>.json through PhysFS
bool loadLayout(const std::string& id, LayoutData& output);

// A minimal US QWERTY layout, used if the layout files cannot be loaded
LayoutData fallbackUSLayout();

// Builds a layout from a per-key character lookup (ex. the OS keyboard layout).
// lookup(xkbName, shift, altGr) returns the UTF-32 character, or 0 to keep the template's output.
LayoutData deriveLayout(const LayoutData& templateLayout, const std::string& id, const WzString& name,
	const std::function<uint32_t (const std::string& xkbName, bool shift, bool altGr)>& lookup);

// Combines a dead key's mark with the following text.
// Returns the dead key's label followed by the text if they don't compose into one character,
// and just the label if the text is a space.
WzString composeDeadKey(uint32_t deadMark, const WzString& deadLabel, const WzString& text);

// MARK: - Accents

// The letters offered when a key is held (data/base/keyboards/accents.json), per language and plain letter
struct AccentData
{
	using LetterGroups = std::unordered_map<std::string, std::vector<WzString>>;  // plain letter (UTF-8) -> letters offered
	std::unordered_map<std::string, LetterGroups> languages;  // by game language code (ex. "de", "pt_BR")

	// The groups for a language code, falling back to the language alone (ex. "de_AT" -> "de")
	const LetterGroups* forLanguage(const std::string& languageCode) const;
};

// Parses accents.json. Returns false and fills error on failure
bool parseAccentJSON(const char* data, size_t length, AccentData& output, std::string* error = nullptr);

// Loads data/base/keyboards/accents.json through PhysFS
bool loadAccents(AccentData& output);

// The language a built-in layout is for (ex. "us" -> "en"), or empty if none
std::string languageForLayoutId(const std::string& layoutId);

// The letters to offer for a key typing `text`, from each language in turn and without duplicates.
// Uppercase text gets uppercase letters.
std::vector<WzString> accentVariants(const AccentData& accents, const std::vector<std::string>& languageCodes, const WzString& text);

// MARK: - Keyboard geometry

enum class SpecialKey : uint8_t
{
	None,       // a character key (see GridKey::xkbName)
	Backspace,
	Enter,
	Shift,
	AltGr,
	Space,
	CursorLeft,
	CursorRight,
	Paste,
	Hide,
};

struct GridKey
{
	std::string xkbName;  // for character keys
	SpecialKey special = SpecialKey::None;
	float x = 0.f;        // in key units, from the left edge
	int row = 0;
	float width = 1.f;    // in key units
};

// The on-screen keyboard's keys and their positions, at most GRID_WIDTH_UNITS wide.
// Character keys with no output on any level are left out.
constexpr float GRID_WIDTH_UNITS = 15.f;
constexpr int GRID_ROWS = 5;
std::vector<GridKey> buildGrid(const LayoutData& layout);

// The key to move to from `from` in a direction (dx and dy in -1, 0, 1), or `from` if there's nowhere to go.
// Moves wrap within a row, and moves between rows pick the key nearest by horizontal center.
size_t navigateGrid(const std::vector<GridKey>& grid, size_t from, int dx, int dy);

// MARK: - Typing state

class TypingState
{
public:
	enum class ShiftState : uint8_t { Off, Once, Locked };

	// The level a key shows and types right now
	Level currentLevel(const KeyLevels& levels) const;
	// What a character key displays right now
	WzString displayLabel(const KeyLevels& levels) const;
	// What a character key types right now (before any pending dead key)
	const KeyOutput& currentOutput(const KeyLevels& levels) const;

	// Presses a character key and returns the text to type
	// (empty for a dead key, which combines with the next key)
	WzString pressCharacterKey(const KeyLevels& levels);
	// Typing a space resolves a pending dead key to its label
	WzString pressSpace();
	// Like pressCharacterKey(), for a letter picked from a held key's accents
	WzString pressVariant(const WzString& text);

	// Shift: tap for one character, tap again to lock, again to release
	void pressShift();
	void toggleAltGr();

	ShiftState shiftState() const { return shift; }
	bool altGrActive() const { return altGr; }
	bool hasPendingDeadKey() const { return pendingDeadMark != 0; }
	WzString pendingDeadLabel() const { return pendingDeadLabelText; }
	// Drops a pending dead key (ex. on Backspace), and returns whether there was one
	bool cancelPendingDeadKey();

	void reset();

private:
	bool shiftAppliesTo(const KeyLevels& levels) const;

	ShiftState shift = ShiftState::Off;
	bool altGr = false;
	uint32_t pendingDeadMark = 0;
	WzString pendingDeadLabelText;
};

} // namespace wzkb

#endif // __INCLUDED_LIB_WIDGET_KBLAYOUT_H__
