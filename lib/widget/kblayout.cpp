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
 */

#include "kblayout.h"

#include "lib/framework/frame.h"

#include <nlohmann/json.hpp>
#include <utf8proc/utf8proc.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace wzkb
{

const KeyLevels* LayoutData::key(const std::string& xkbName) const
{
	auto it = keys.find(xkbName);
	return (it != keys.end()) ? &it->second : nullptr;
}

bool LayoutData::hasAltGrLevel() const
{
	return std::any_of(keys.begin(), keys.end(), [](const std::pair<const std::string, KeyLevels>& entry) {
		return !entry.second[static_cast<size_t>(Level::AltGr)].isEmpty() || !entry.second[static_cast<size_t>(Level::AltGrShift)].isEmpty();
	});
}

const std::vector<std::string>& builtinLayoutIds()
{
	static const std::vector<std::string> ids = { "us", "de", "fr", "ru" };
	return ids;
}

std::string layoutIdForLanguage(const std::string& languageCode)
{
	// match on the language part only (ex. "de_DE", "fr-CA")
	std::string language = languageCode.substr(0, languageCode.find_first_of("_-.@"));
	std::transform(language.begin(), language.end(), language.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if (language == "de")
	{
		return "de";
	}
	if (language == "fr")
	{
		return "fr";
	}
	if (language == "ru")
	{
		return "ru";
	}
	return "us";
}

static WzString codepointString(uint32_t codepoint)
{
	return WzString::fromCodepoint(WzUniCodepoint::fromUTF32(codepoint));
}

static bool parseKeyOutput(const nlohmann::json& value, KeyOutput& output)
{
	output = KeyOutput();
	if (value.is_string())
	{
		output.text = WzString::fromUtf8(value.get<std::string>());
		output.label = output.text;
		return true;
	}
	if (value.is_object())
	{
		auto dead = value.find("dead");
		auto label = value.find("label");
		if (dead == value.end() || !dead->is_string() || label == value.end() || !label->is_string())
		{
			return false;
		}
		std::vector<uint32_t> mark = WzString::fromUtf8(dead->get<std::string>()).toUtf32();
		if (mark.size() != 1)
		{
			return false;
		}
		output.deadMark = mark[0];
		output.label = WzString::fromUtf8(label->get<std::string>());
		return true;
	}
	return false;
}

bool parseLayoutJSON(const char* data, size_t length, LayoutData& output, std::string* error)
{
	auto fail = [error](const std::string& message) {
		if (error)
		{
			*error = message;
		}
		return false;
	};

	nlohmann::json root = nlohmann::json::parse(data, data + length, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		return fail("not a JSON object");
	}
	auto format = root.find("format");
	if (format == root.end() || !format->is_number_integer() || format->get<int>() != 1)
	{
		return fail("unsupported or missing format version");
	}
	auto id = root.find("id");
	auto name = root.find("name");
	auto geometry = root.find("geometry");
	auto keys = root.find("keys");
	if (id == root.end() || !id->is_string() || name == root.end() || !name->is_string()
		|| geometry == root.end() || !geometry->is_string() || keys == root.end() || !keys->is_object())
	{
		return fail("missing id, name, geometry or keys");
	}

	LayoutData layout;
	layout.id = id->get<std::string>();
	layout.name = WzString::fromUtf8(name->get<std::string>());
	const std::string geometryName = geometry->get<std::string>();
	if (geometryName != "iso" && geometryName != "ansi")
	{
		return fail("unknown geometry: " + geometryName);
	}
	layout.iso = (geometryName == "iso");

	for (auto it = keys->begin(); it != keys->end(); ++it)
	{
		if (!it.value().is_array() || it.value().size() > NUM_LEVELS)
		{
			return fail("invalid levels for key " + it.key());
		}
		KeyLevels levels;
		size_t index = 0;
		for (const auto& levelValue : it.value())
		{
			if (!parseKeyOutput(levelValue, levels[index]))
			{
				return fail("invalid level " + std::to_string(index) + " for key " + it.key());
			}
			++index;
		}
		layout.keys[it.key()] = std::move(levels);
	}

	output = std::move(layout);
	return true;
}

LayoutData fallbackUSLayout()
{
	LayoutData layout;
	layout.id = "us";
	layout.name = WzString::fromUtf8("English (US, QWERTY)");
	layout.iso = false;
	struct Entry { const char* name; char base; char shift; };
	static const Entry entries[] = {
		{"TLDE", '`', '~'}, {"AE01", '1', '!'}, {"AE02", '2', '@'}, {"AE03", '3', '#'}, {"AE04", '4', '$'},
		{"AE05", '5', '%'}, {"AE06", '6', '^'}, {"AE07", '7', '&'}, {"AE08", '8', '*'}, {"AE09", '9', '('},
		{"AE10", '0', ')'}, {"AE11", '-', '_'}, {"AE12", '=', '+'},
		{"AD01", 'q', 'Q'}, {"AD02", 'w', 'W'}, {"AD03", 'e', 'E'}, {"AD04", 'r', 'R'}, {"AD05", 't', 'T'},
		{"AD06", 'y', 'Y'}, {"AD07", 'u', 'U'}, {"AD08", 'i', 'I'}, {"AD09", 'o', 'O'}, {"AD10", 'p', 'P'},
		{"AD11", '[', '{'}, {"AD12", ']', '}'}, {"BKSL", '\\', '|'},
		{"AC01", 'a', 'A'}, {"AC02", 's', 'S'}, {"AC03", 'd', 'D'}, {"AC04", 'f', 'F'}, {"AC05", 'g', 'G'},
		{"AC06", 'h', 'H'}, {"AC07", 'j', 'J'}, {"AC08", 'k', 'K'}, {"AC09", 'l', 'L'}, {"AC10", ';', ':'},
		{"AC11", '\'', '"'},
		{"AB01", 'z', 'Z'}, {"AB02", 'x', 'X'}, {"AB03", 'c', 'C'}, {"AB04", 'v', 'V'}, {"AB05", 'b', 'B'},
		{"AB06", 'n', 'N'}, {"AB07", 'm', 'M'}, {"AB08", ',', '<'}, {"AB09", '.', '>'}, {"AB10", '/', '?'},
	};
	for (const auto& entry : entries)
	{
		KeyLevels levels;
		levels[0].text = levels[0].label = codepointString(static_cast<uint32_t>(entry.base));
		levels[1].text = levels[1].label = codepointString(static_cast<uint32_t>(entry.shift));
		layout.keys[entry.name] = levels;
	}
	return layout;
}

LayoutData deriveLayout(const LayoutData& templateLayout, const std::string& id, const WzString& name,
	const std::function<uint32_t (const std::string& xkbName, bool shift, bool altGr)>& lookup)
{
	LayoutData layout = templateLayout;
	layout.id = id;
	layout.name = name;
	for (auto& entry : layout.keys)
	{
		for (size_t level = 0; level < NUM_LEVELS; ++level)
		{
			const bool shift = (level & 1) != 0;
			const bool altGr = (level & 2) != 0;
			const uint32_t codepoint = lookup(entry.first, shift, altGr);
			if (codepoint < 0x20 || codepoint == 0x7f)
			{
				continue;
			}
			KeyOutput& output = entry.second[level];
			if (output.isDead() && output.label == codepointString(codepoint))
			{
				// the OS reports a dead key by its spacing form - keep it dead
				continue;
			}
			output = KeyOutput();
			output.text = output.label = codepointString(codepoint);
		}
	}
	return layout;
}

WzString composeDeadKey(uint32_t deadMark, const WzString& deadLabel, const WzString& text)
{
	if (text.isEmpty())
	{
		return deadLabel;
	}
	std::vector<uint32_t> codepoints = text.toUtf32();
	if (codepoints[0] == ' ')
	{
		WzString result = deadLabel;
		result.append(text.substr(1));
		return result;
	}

	// NFC-compose the first character with the mark
	WzString decomposed = codepointString(codepoints[0]);
	decomposed.append(WzUniCodepoint::fromUTF32(deadMark));
	utf8proc_uint8_t* composed = utf8proc_NFC(reinterpret_cast<const utf8proc_uint8_t*>(decomposed.toUtf8().c_str()));
	if (composed != nullptr)
	{
		WzString composedString = WzString::fromUtf8(reinterpret_cast<const char*>(composed));
		free(composed);
		if (composedString.length() == 1)
		{
			composedString.append(text.substr(1));
			return composedString;
		}
	}

	WzString result = deadLabel;
	result.append(text);
	return result;
}

// MARK: - Keyboard geometry

static void addCharacterKey(std::vector<GridKey>& grid, const LayoutData& layout, const char* name, float x, int row)
{
	const KeyLevels* levels = layout.key(name);
	if (!levels)
	{
		return;
	}
	if (std::all_of(levels->begin(), levels->end(), [](const KeyOutput& output) { return output.isEmpty(); }))
	{
		return;
	}
	GridKey key;
	key.xkbName = name;
	key.x = x;
	key.row = row;
	grid.push_back(key);
}

static void addSpecialKey(std::vector<GridKey>& grid, SpecialKey special, float x, int row, float width)
{
	GridKey key;
	key.special = special;
	key.x = x;
	key.row = row;
	key.width = width;
	grid.push_back(key);
}

std::vector<GridKey> buildGrid(const LayoutData& layout)
{
	std::vector<GridKey> grid;
	char name[8];

	// Row 0: number row, then Backspace
	addCharacterKey(grid, layout, "TLDE", 0.f, 0);
	for (int i = 1; i <= 12; ++i)
	{
		snprintf(name, sizeof(name), "AE%02d", i);
		addCharacterKey(grid, layout, name, static_cast<float>(i), 0);
	}
	addSpecialKey(grid, SpecialKey::Backspace, 13.f, 0, 2.f);

	// Row 1: top letter row (ANSI: the backslash key ends it)
	for (int i = 1; i <= 12; ++i)
	{
		snprintf(name, sizeof(name), "AD%02d", i);
		addCharacterKey(grid, layout, name, 0.5f + static_cast<float>(i - 1), 1);
	}
	if (!layout.iso)
	{
		addCharacterKey(grid, layout, "BKSL", 12.5f, 1);
	}

	// Row 2: home row (ISO: the backslash key ends it), then Enter
	for (int i = 1; i <= 11; ++i)
	{
		snprintf(name, sizeof(name), "AC%02d", i);
		addCharacterKey(grid, layout, name, 0.75f + static_cast<float>(i - 1), 2);
	}
	float enterX = 11.75f;
	if (layout.iso)
	{
		addCharacterKey(grid, layout, "BKSL", 11.75f, 2);
		enterX = 12.75f;
	}
	addSpecialKey(grid, SpecialKey::Enter, enterX, 2, GRID_WIDTH_UNITS - enterX);

	// Row 3: Shift, (ISO: the extra key), bottom letter row, Shift
	float x = 0.f;
	const float leftShiftWidth = layout.iso ? 1.25f : 2.25f;
	addSpecialKey(grid, SpecialKey::Shift, x, 3, leftShiftWidth);
	x += leftShiftWidth;
	if (layout.iso)
	{
		const size_t sizeBefore = grid.size();
		addCharacterKey(grid, layout, "LSGT", x, 3);
		if (grid.size() == sizeBefore)
		{
			// no extra key in this layout - widen the Shift key instead
			grid.back().width += 1.f;
		}
		x += 1.f;
	}
	for (int i = 1; i <= 10; ++i)
	{
		snprintf(name, sizeof(name), "AB%02d", i);
		addCharacterKey(grid, layout, name, x, 3);
		x += 1.f;
	}
	addSpecialKey(grid, SpecialKey::Shift, x, 3, GRID_WIDTH_UNITS - x);

	// Row 4: (AltGr), Paste, Space, cursor keys, Hide
	x = 0.f;
	if (layout.hasAltGrLevel())
	{
		addSpecialKey(grid, SpecialKey::AltGr, x, 4, 1.75f);
		x += 1.75f;
	}
	addSpecialKey(grid, SpecialKey::Paste, x, 4, 1.75f);
	x += 1.75f;
	const float spaceWidth = 10.f - x;
	addSpecialKey(grid, SpecialKey::Space, x, 4, spaceWidth);
	x += spaceWidth;
	addSpecialKey(grid, SpecialKey::CursorLeft, x, 4, 1.25f);
	x += 1.25f;
	addSpecialKey(grid, SpecialKey::CursorRight, x, 4, 1.25f);
	x += 1.25f;
	addSpecialKey(grid, SpecialKey::Hide, x, 4, GRID_WIDTH_UNITS - x);

	return grid;
}

size_t navigateGrid(const std::vector<GridKey>& grid, size_t from, int dx, int dy)
{
	if (from >= grid.size() || (dx == 0 && dy == 0))
	{
		return from;
	}
	const GridKey& current = grid[from];
	const float currentCenter = current.x + current.width / 2.f;

	if (dy == 0)
	{
		// within the row, ordered by x, wrapping around
		std::vector<size_t> row;
		for (size_t i = 0; i < grid.size(); ++i)
		{
			if (grid[i].row == current.row)
			{
				row.push_back(i);
			}
		}
		std::sort(row.begin(), row.end(), [&grid](size_t a, size_t b) { return grid[a].x < grid[b].x; });
		auto it = std::find(row.begin(), row.end(), from);
		const int count = static_cast<int>(row.size());
		const int position = static_cast<int>(it - row.begin());
		const int next = ((position + (dx > 0 ? 1 : -1)) % count + count) % count;
		return row[static_cast<size_t>(next)];
	}

	// to the adjacent row (wrapping top <-> bottom), nearest by horizontal center
	int targetRow = current.row;
	for (int attempt = 0; attempt < GRID_ROWS; ++attempt)
	{
		targetRow = ((targetRow + (dy > 0 ? 1 : -1)) % GRID_ROWS + GRID_ROWS) % GRID_ROWS;
		size_t best = from;
		float bestDistance = 0.f;
		for (size_t i = 0; i < grid.size(); ++i)
		{
			if (grid[i].row != targetRow)
			{
				continue;
			}
			const GridKey& candidate = grid[i];
			float distance;
			if (currentCenter >= candidate.x && currentCenter < candidate.x + candidate.width)
			{
				distance = 0.f;  // directly below / above
			}
			else
			{
				distance = std::fabs((candidate.x + candidate.width / 2.f) - currentCenter);
			}
			if (best == from || distance < bestDistance)
			{
				best = i;
				bestDistance = distance;
			}
		}
		if (best != from)
		{
			return best;
		}
	}
	return from;
}

// MARK: - Typing state

bool TypingState::shiftAppliesTo(const KeyLevels& levels) const
{
	switch (shift)
	{
		case ShiftState::Off:
			return false;
		case ShiftState::Once:
			return true;
		case ShiftState::Locked:
		{
			// like Caps Lock: only for keys whose shifted form is the uppercase letter
			const size_t baseIndex = altGr ? static_cast<size_t>(Level::AltGr) : static_cast<size_t>(Level::Base);
			const KeyOutput& base = levels[baseIndex];
			const KeyOutput& shifted = levels[baseIndex + 1];
			if (base.isDead() || shifted.isDead() || base.text.length() != 1 || shifted.text.length() != 1)
			{
				return false;
			}
			const WzUniCodepoint baseChar = base.text.at(0);
			const WzUniCodepoint shiftedChar = shifted.text.at(0);
			return !(baseChar == shiftedChar) && baseChar.toUpper() == shiftedChar;
		}
	}
	return false;
}

Level TypingState::currentLevel(const KeyLevels& levels) const
{
	const bool shifted = shiftAppliesTo(levels);
	size_t index = (altGr ? 2 : 0) + (shifted ? 1 : 0);
	// fall back to a lower level when this one is empty
	if (levels[index].isEmpty() && index == static_cast<size_t>(Level::AltGrShift))
	{
		index = static_cast<size_t>(Level::AltGr);
	}
	if (levels[index].isEmpty() && index == static_cast<size_t>(Level::AltGr))
	{
		index = shifted ? static_cast<size_t>(Level::Shift) : static_cast<size_t>(Level::Base);
	}
	if (levels[index].isEmpty() && index == static_cast<size_t>(Level::Shift))
	{
		index = static_cast<size_t>(Level::Base);
	}
	return static_cast<Level>(index);
}

WzString TypingState::displayLabel(const KeyLevels& levels) const
{
	return levels[static_cast<size_t>(currentLevel(levels))].label;
}

WzString TypingState::pressCharacterKey(const KeyLevels& levels)
{
	const KeyOutput& output = levels[static_cast<size_t>(currentLevel(levels))];
	if (shift == ShiftState::Once)
	{
		shift = ShiftState::Off;
	}
	if (output.isEmpty())
	{
		return WzString();
	}

	if (output.isDead())
	{
		if (pendingDeadMark == 0)
		{
			pendingDeadMark = output.deadMark;
			pendingDeadLabelText = output.label;
			return WzString();
		}
		// a second dead key: the same one types its spacing form,
		// and another flushes the first and becomes pending
		WzString flushed = pendingDeadLabelText;
		if (pendingDeadMark == output.deadMark)
		{
			pendingDeadMark = 0;
			pendingDeadLabelText.clear();
		}
		else
		{
			pendingDeadMark = output.deadMark;
			pendingDeadLabelText = output.label;
		}
		return flushed;
	}

	if (pendingDeadMark != 0)
	{
		WzString composed = composeDeadKey(pendingDeadMark, pendingDeadLabelText, output.text);
		pendingDeadMark = 0;
		pendingDeadLabelText.clear();
		return composed;
	}
	return output.text;
}

WzString TypingState::pressSpace()
{
	if (shift == ShiftState::Once)
	{
		shift = ShiftState::Off;
	}
	if (pendingDeadMark != 0)
	{
		WzString label = pendingDeadLabelText;
		pendingDeadMark = 0;
		pendingDeadLabelText.clear();
		return label;
	}
	return WzString::fromUtf8(" ");
}

void TypingState::pressShift()
{
	switch (shift)
	{
		case ShiftState::Off: shift = ShiftState::Once; break;
		case ShiftState::Once: shift = ShiftState::Locked; break;
		case ShiftState::Locked: shift = ShiftState::Off; break;
	}
}

void TypingState::toggleAltGr()
{
	altGr = !altGr;
}

bool TypingState::cancelPendingDeadKey()
{
	if (pendingDeadMark == 0)
	{
		return false;
	}
	pendingDeadMark = 0;
	pendingDeadLabelText.clear();
	return true;
}

void TypingState::reset()
{
	shift = ShiftState::Off;
	altGr = false;
	pendingDeadMark = 0;
	pendingDeadLabelText.clear();
}

} // namespace wzkb
