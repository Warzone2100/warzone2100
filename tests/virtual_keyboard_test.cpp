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

// Unit tests for the on-screen keyboard logic (lib/widget/kblayout.cpp), against the shipped data files.
// Build with -DWZ_BUILD_VIRTUAL_KEYBOARD_TEST=ON (target: virtual_keyboard_test). Exits nonzero on failure.

#include "lib/widget/kblayout.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

#ifndef WZ_KEYBOARDS_DIR
# error "WZ_KEYBOARDS_DIR must point at data/base/keyboards"
#endif

// Stubs for the SDL-backend symbols framework/debug.cpp references (as in tests/framework_linktest.cpp)
bool wzIsFullscreen()
{
	return false;
}

bool wzChangeWindowMode(WINDOW_MODE, bool)
{
	return true;
}

void wzDisplayDialog(DialogType, const char*, const char*)
{
}

// framework/frame.cpp (pulled in for its PhysFS helpers) references these
void inputInitialise()
{
}

uint32_t wzGetTicks()
{
	return 0;
}

using namespace wzkb;

static int failures = 0;
static int checks = 0;

#define CHECK_TRUE(cond, ...) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			std::printf("FAIL %s:%d: ", __FILE__, __LINE__); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

#define CHECK_STR(actual, expected, ...) \
	do { \
		checks++; \
		const std::string a_ = (actual); \
		const std::string e_ = (expected); \
		if (a_ != e_) { \
			failures++; \
			std::printf("FAIL %s:%d: \"%s\" != \"%s\": ", __FILE__, __LINE__, a_.c_str(), e_.c_str()); \
			std::printf(__VA_ARGS__); \
			std::printf("\n"); \
		} \
	} while (0)

static bool loadShippedLayout(const std::string& id, LayoutData& layout)
{
	const std::string path = std::string(WZ_KEYBOARDS_DIR) + "/" + id + ".json";
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		std::printf("cannot open %s\n", path.c_str());
		return false;
	}
	std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::string error;
	if (!parseLayoutJSON(contents.data(), contents.size(), layout, &error))
	{
		std::printf("cannot parse %s: %s\n", path.c_str(), error.c_str());
		return false;
	}
	return true;
}

static const KeyLevels& keyOf(const LayoutData& layout, const char* name)
{
	static const KeyLevels empty;
	const KeyLevels* levels = layout.key(name);
	return levels ? *levels : empty;
}

static std::string typeKey(TypingState& state, const LayoutData& layout, const char* name)
{
	return state.pressCharacterKey(keyOf(layout, name)).toUtf8();
}

static void testShippedLayoutsParse()
{
	for (const auto& id : builtinLayoutIds())
	{
		LayoutData layout;
		CHECK_TRUE(loadShippedLayout(id, layout), "layout %s loads", id.c_str());
		CHECK_STR(layout.id, id, "layout id matches its file name");
		CHECK_TRUE(!layout.name.isEmpty(), "layout %s has a name", id.c_str());
		CHECK_TRUE(layout.keys.size() >= 47, "layout %s has all the keys (%zu)", id.c_str(), layout.keys.size());
		for (int i = 1; i <= 10; ++i)
		{
			char name[8];
			snprintf(name, sizeof(name), "AD%02d", i);
			CHECK_TRUE(layout.key(name) && !(*layout.key(name))[0].isEmpty(), "layout %s has a base character on %s", id.c_str(), name);
		}
	}

	LayoutData us, de, fr, ru;
	loadShippedLayout("us", us);
	loadShippedLayout("de", de);
	loadShippedLayout("fr", fr);
	loadShippedLayout("ru", ru);
	CHECK_TRUE(!us.iso && de.iso && fr.iso && !ru.iso, "geometries");
	CHECK_STR(keyOf(us, "AD01")[0].text.toUtf8(), "q", "US QWERTY");
	CHECK_STR(keyOf(de, "AD06")[0].text.toUtf8(), "z", "German QWERTZ");
	CHECK_STR(keyOf(de, "AC10")[0].text.toUtf8(), "ö", "German umlaut");
	CHECK_STR(keyOf(fr, "AD01")[0].text.toUtf8(), "a", "French AZERTY");
	CHECK_STR(keyOf(fr, "AE01")[1].text.toUtf8(), "1", "French digits on Shift");
	CHECK_STR(keyOf(ru, "AD01")[0].text.toUtf8(), "й", "Russian ЙЦУКЕН");
	CHECK_TRUE(keyOf(fr, "AD11")[0].isDead(), "French circumflex is a dead key");
	CHECK_TRUE(!us.hasAltGrLevel() && de.hasAltGrLevel() && fr.hasAltGrLevel(), "AltGr levels");
}

static void testParseErrors()
{
	LayoutData layout;
	std::string error;
	const std::string notJson = "{ nope";
	CHECK_TRUE(!parseLayoutJSON(notJson.data(), notJson.size(), layout, &error), "invalid JSON is rejected");
	const std::string wrongFormat = R"({"format": 2, "id": "x", "name": "x", "geometry": "iso", "keys": {}})";
	CHECK_TRUE(!parseLayoutJSON(wrongFormat.data(), wrongFormat.size(), layout, &error), "unknown format version is rejected");
	const std::string badGeometry = R"({"format": 1, "id": "x", "name": "x", "geometry": "jis", "keys": {}})";
	CHECK_TRUE(!parseLayoutJSON(badGeometry.data(), badGeometry.size(), layout, &error), "unknown geometry is rejected");
	const std::string tooManyLevels = R"({"format": 1, "id": "x", "name": "x", "geometry": "ansi", "keys": {"AD01": ["a","b","c","d","e"]}})";
	CHECK_TRUE(!parseLayoutJSON(tooManyLevels.data(), tooManyLevels.size(), layout, &error), "more than four levels is rejected");
	const std::string badDead = R"({"format": 1, "id": "x", "name": "x", "geometry": "ansi", "keys": {"AD01": [{"dead": "ab", "label": "^"}]}})";
	CHECK_TRUE(!parseLayoutJSON(badDead.data(), badDead.size(), layout, &error), "a dead key must be one combining mark");
	const std::string good = R"({"format": 1, "id": "x", "name": "X", "geometry": "ansi", "keys": {"AD01": ["a", "A"]}})";
	CHECK_TRUE(parseLayoutJSON(good.data(), good.size(), layout, &error), "a minimal layout parses: %s", error.c_str());
}

static void testGrid()
{
	for (const auto& id : builtinLayoutIds())
	{
		LayoutData layout;
		loadShippedLayout(id, layout);
		const auto grid = buildGrid(layout);
		std::set<std::string> charKeys;
		for (size_t i = 0; i < grid.size(); ++i)
		{
			const GridKey& key = grid[i];
			CHECK_TRUE(key.row >= 0 && key.row < GRID_ROWS, "%s: row in range", id.c_str());
			CHECK_TRUE(key.x >= 0.f && key.x + key.width <= GRID_WIDTH_UNITS + 0.001f, "%s: key %s fits in the grid", id.c_str(), key.xkbName.c_str());
			if (key.special == SpecialKey::None)
			{
				charKeys.insert(key.xkbName);
			}
			for (size_t j = i + 1; j < grid.size(); ++j)
			{
				const GridKey& other = grid[j];
				const bool overlap = (other.row == key.row) && (other.x < key.x + key.width - 0.001f) && (key.x < other.x + other.width - 0.001f);
				CHECK_TRUE(!overlap, "%s: keys %zu and %zu do not overlap", id.c_str(), i, j);
			}
		}
		for (const auto& entry : layout.keys)
		{
			CHECK_TRUE(charKeys.count(entry.first) == 1, "%s: key %s is on the grid", id.c_str(), entry.first.c_str());
		}
	}
}

static size_t findKey(const std::vector<GridKey>& grid, const std::string& name)
{
	for (size_t i = 0; i < grid.size(); ++i)
	{
		if (grid[i].xkbName == name)
		{
			return i;
		}
	}
	return grid.size();
}

static size_t findSpecial(const std::vector<GridKey>& grid, SpecialKey special)
{
	for (size_t i = 0; i < grid.size(); ++i)
	{
		if (grid[i].special == special)
		{
			return i;
		}
	}
	return grid.size();
}

static void testNavigation()
{
	LayoutData us;
	loadShippedLayout("us", us);
	const auto grid = buildGrid(us);
	const size_t q = findKey(grid, "AD01");
	CHECK_STR(grid[navigateGrid(grid, q, 1, 0)].xkbName, "AD02", "right from Q is W");
	CHECK_STR(grid[navigateGrid(grid, q, 0, 1)].xkbName, "AC01", "down from Q is A");
	CHECK_STR(grid[navigateGrid(grid, q, 0, -1)].xkbName, "AE01", "up from Q is 1");
	const size_t tilde = findKey(grid, "TLDE");
	CHECK_TRUE(grid[navigateGrid(grid, tilde, -1, 0)].special == SpecialKey::Backspace, "left from ` wraps to Backspace");
	const size_t space = findSpecial(grid, SpecialKey::Space);
	const size_t up = navigateGrid(grid, space, 0, -1);
	CHECK_TRUE(grid[up].row == 3, "up from Space reaches the bottom letter row");
	CHECK_TRUE(grid[navigateGrid(grid, space, 0, 1)].row == 0, "down from Space wraps to the number row");
}

static void testShiftAndCaps()
{
	LayoutData us;
	loadShippedLayout("us", us);
	TypingState state;
	CHECK_STR(typeKey(state, us, "AD01"), "q", "base level");
	state.pressShift();
	CHECK_STR(typeKey(state, us, "AE01"), "!", "Shift once applies to symbols");
	CHECK_STR(typeKey(state, us, "AD01"), "q", "Shift once is released after a key");
	state.pressShift();
	state.pressShift();
	CHECK_TRUE(state.shiftState() == TypingState::ShiftState::Locked, "second Shift tap locks");
	CHECK_STR(typeKey(state, us, "AD01"), "Q", "Caps applies to letters");
	CHECK_STR(typeKey(state, us, "AE01"), "1", "Caps leaves digits alone");
	CHECK_STR(typeKey(state, us, "AD02"), "W", "Caps stays on");
	state.pressShift();
	CHECK_STR(typeKey(state, us, "AD01"), "q", "third Shift tap releases");

	LayoutData ru;
	loadShippedLayout("ru", ru);
	state.reset();
	state.pressShift();
	state.pressShift();
	CHECK_STR(typeKey(state, ru, "AD01"), "Й", "Caps applies to Cyrillic letters");
}

static void testAltGr()
{
	LayoutData de;
	loadShippedLayout("de", de);
	TypingState state;
	state.toggleAltGr();
	CHECK_STR(typeKey(state, de, "AD01"), "@", "AltGr+Q is @ on German");
	CHECK_STR(typeKey(state, de, "AE08"), "[", "AltGr+8 is [ on German");
	CHECK_TRUE(state.altGrActive(), "AltGr stays on");
	CHECK_STR(state.displayLabel(keyOf(de, "AD01")).toUtf8(), "@", "label follows the level");
	state.toggleAltGr();
	CHECK_STR(typeKey(state, de, "AD01"), "q", "AltGr off");

	// a key without an AltGr character falls back to its base level
	LayoutData us;
	loadShippedLayout("us", us);
	TypingState usState;
	usState.toggleAltGr();
	CHECK_STR(typeKey(usState, us, "AD01"), "q", "no AltGr level: base character");
	usState.pressShift();
	CHECK_STR(typeKey(usState, us, "AD01"), "Q", "no AltGr level: shifted character");
}

static void testDeadKeys()
{
	LayoutData fr;
	loadShippedLayout("fr", fr);
	TypingState state;
	CHECK_STR(typeKey(state, fr, "AD11"), "", "dead key types nothing yet");
	CHECK_TRUE(state.hasPendingDeadKey(), "dead key is pending");
	CHECK_STR(typeKey(state, fr, "AD03"), "ê", "^ + e = ê");
	CHECK_TRUE(!state.hasPendingDeadKey(), "dead key consumed");

	typeKey(state, fr, "AD11");
	CHECK_STR(typeKey(state, fr, "AB02"), "^x", "^ + x does not compose");
	typeKey(state, fr, "AD11");
	CHECK_STR(state.pressSpace().toUtf8(), "^", "^ + space = ^");
	typeKey(state, fr, "AD11");
	CHECK_STR(typeKey(state, fr, "AD11"), "^", "^ + ^ = ^");
	CHECK_TRUE(!state.hasPendingDeadKey(), "repeated dead key consumed");

	state.pressShift();
	CHECK_STR(typeKey(state, fr, "AD11"), "", "Shift + dead key (diaeresis)");
	CHECK_STR(typeKey(state, fr, "AD08"), "ï", "¨ + i = ï");

	typeKey(state, fr, "AD11");
	CHECK_TRUE(state.cancelPendingDeadKey(), "dead key canceled");
	CHECK_STR(typeKey(state, fr, "AD03"), "e", "after canceling");

	CHECK_STR(composeDeadKey(0x0308, WzString::fromUtf8("¨"), WzString::fromUtf8("U")).toUtf8(), "Ü", "uppercase compose");
	CHECK_STR(composeDeadKey(0x0301, WzString::fromUtf8("´"), WzString::fromUtf8("")).toUtf8(), "´", "nothing to compose with");
}

static void testLanguageDefaults()
{
	CHECK_STR(layoutIdForLanguage("de"), "de", "German");
	CHECK_STR(layoutIdForLanguage("de_DE"), "de", "German with region");
	CHECK_STR(layoutIdForLanguage("fr-CA"), "fr", "French with region");
	CHECK_STR(layoutIdForLanguage("ru"), "ru", "Russian");
	CHECK_STR(layoutIdForLanguage("pl"), "us", "others fall back to US");
	CHECK_STR(layoutIdForLanguage(""), "us", "empty falls back to US");
}

static void testDeriveAndFallback()
{
	const LayoutData fallback = fallbackUSLayout();
	CHECK_TRUE(fallback.keys.size() == 47, "fallback has all ANSI keys (%zu)", fallback.keys.size());
	const LayoutData derived = deriveLayout(fallback, "system", WzString::fromUtf8("System"),
		[](const std::string& name, bool shift, bool altGr) -> uint32_t {
			if (name == "AD01" && !altGr)
			{
				return shift ? 0x0410 : 0x0430; // Cyrillic А / а
			}
			return 0;
		});
	CHECK_STR(derived.id, "system", "derived id");
	CHECK_STR(keyOf(derived, "AD01")[0].text.toUtf8(), "а", "derived character");
	CHECK_STR(keyOf(derived, "AD01")[1].text.toUtf8(), "А", "derived shifted character");
	CHECK_STR(keyOf(derived, "AD02")[0].text.toUtf8(), "w", "unknown keys keep the template");
}

int main()
{
	testShippedLayoutsParse();
	testParseErrors();
	testGrid();
	testNavigation();
	testShiftAndCaps();
	testAltGr();
	testDeadKeys();
	testLanguageDefaults();
	testDeriveAndFallback();

	std::printf("%s: %d checks, %d failures\n", failures == 0 ? "PASS" : "FAIL", checks, failures);
	return failures == 0 ? 0 : 1;
}
