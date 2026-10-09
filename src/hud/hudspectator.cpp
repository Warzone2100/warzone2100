// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/


#include "hudspectator.h"
#include "hudids.h"
#include "hudsettings.h"
#include "hudunittypes.h"
#include "hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/form.h"

#include "../component.h"
#include "../game_world.h"
#include "../loop.h"
#include "../multiplay.h"
#include "../power.h"
#include "../scores.h"
#include "../spectatorwidgets.h"

#include <vector>

namespace
{

constexpr int ONE_LINE_HEIGHT = 22;
constexpr int TWO_LINE_HEIGHT = 30;
/// The height of each of a player's two lines
constexpr int LINE_HEIGHT = 12;
constexpr int TWO_LINE_BUTTON = 20;
/// The gap between the parts, with a divider in its middle
constexpr int GAP = 14;
constexpr int MARK = 8;
constexpr int MARK_GAP = 4;
constexpr int NAME_GAP = 6;
constexpr int SYMBOL_GAP = 2;
constexpr int PAIR_GAP = 6;
constexpr int NAME_WIDTH_ONE_LINE = 80;
constexpr int NAME_WIDTH_TWO_LINES = 64;
constexpr int ICON_SIZE = 11;

bool spectating()
{
	return selectedPlayer >= MAX_PLAYERS || NETisReplay() || (bMultiPlayer && NetPlay.players[selectedPlayer].isSpectator);
}

bool playerIsInGame(unsigned player)
{
	if (player >= MAX_PLAYERS || player >= game.maxPlayers || NetPlay.players[player].isSpectator)
	{
		return false;
	}
	return !gameWorld.objects.structures[player].empty() || !gameWorld.objects.droids[player].empty();
}

float baselineIn(WzText &text, int top, int height)
{
	return std::round(static_cast<float>(top) + static_cast<float>(height - text.lineSize()) / 2.f - static_cast<float>(text.aboveBase()));
}

/// Power's or units' symbol: the skin's icon, or a glyph centered on its drawn pixels (symbol fonts sit lower than the
/// text)
class StatSymbol
{
public:
	StatSymbol(const char *iconName, const char *glyphText) : icon(iconName)
	{
		glyph.setText(WzString::fromUtf8(glyphText), font_small);
	}
	int width()
	{
		return currentHudSkin().hasNamedIcon(icon) ? ICON_SIZE : glyph.width();
	}
	void draw(int x, int top, int height, PIELIGHT color)
	{
		auto &skin = currentHudSkin();
		if (skin.drawNamedIcon(icon, WzRect(x, top + (height - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE), color))
		{
			return;
		}
		const float y = std::round(static_cast<float>(top) + (static_cast<float>(height) - glyph.inkHeight()) / 2.f - glyph.inkTop());
		skin.drawOverlayText(glyph, static_cast<float>(x), y, color);
	}

private:
	std::string icon;
	WzText glyph;
};

/// One player: a team-colored mark, the name (cut short), power and units, on one line or the name above the rest
class PlayerEntry : public WIDGET
{
public:
	void set(unsigned newPlayer, bool twoLines, bool withName)
	{
		player = newPlayer;
		lines = twoLines ? 2 : 1;
		named = withName || twoLines;
		if (named)
		{
			setName(WzString::fromUtf8(getPlayerName(player)), twoLines ? NAME_WIDTH_TWO_LINES : NAME_WIDTH_ONE_LINE);
		}
		power.setText(WzString::fromUtf8(std::to_string(getPower(player))), font_small);
		units.setText(WzString::fromUtf8(std::to_string(getNumDroids(player) + getNumMissionDroids(player) + getNumTransporterDroids(player))), font_small);
		setTip(astringf(_("%s: power and units"), getPlayerName(player)));
	}
	int32_t idealWidth() override
	{
		if (lines == 2)
		{
			return std::max(MARK + MARK_GAP + name.width(), statsWidth());
		}
		return MARK + MARK_GAP + (named ? name.width() + NAME_GAP : 0) + statsWidth();
	}
	int32_t idealHeight() override { return lines == 2 ? LINE_HEIGHT * 2 : HUD_CHIP_HEIGHT; }
	std::string getTip() override { return tip; }
	void setTip(std::string newTip) override { tip = newTip; }

protected:
	void display(int xOffset, int yOffset) override
	{
		auto &skin = currentHudSkin();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const int lineHeight = height() / lines;
		const int markY = y0 + (lineHeight - MARK) / 2;
		hudDrawPlayerMark(x0, markY, MARK, player);
		int x = x0 + MARK + MARK_GAP;
		if (named)
		{
			skin.drawOverlayText(name, static_cast<float>(x), baselineIn(name, y0, lineHeight), skin.colors().textPrimary);
			x += name.width() + NAME_GAP;
		}
		int top = y0;
		PIELIGHT color = skin.colors().textPrimary;
		if (lines == 2)
		{
			x = x0;
			top = y0 + lineHeight;
			color = skin.colors().textSecondary;
		}
		powerSymbol.draw(x, top, lineHeight, skin.colors().stateWarn);
		x += powerSymbol.width() + SYMBOL_GAP;
		skin.drawOverlayText(power, static_cast<float>(x), baselineIn(power, top, lineHeight), color);
		x += power.width() + PAIR_GAP;
		unitsSymbol.draw(x, top, lineHeight, color);
		x += unitsSymbol.width() + SYMBOL_GAP;
		skin.drawOverlayText(units, static_cast<float>(x), baselineIn(units, top, lineHeight), color);
	}

private:
	/// Cut short to the width, with an ellipsis
	void setName(const WzString &fullName, int limit)
	{
		name.setText(fullName, font_small, std::max(limit, 1));
	}
	int statsWidth() { return powerSymbol.width() + SYMBOL_GAP + power.width() + PAIR_GAP + unitsSymbol.width() + SYMBOL_GAP + units.width(); }

	unsigned player = 0;
	int lines = 1;
	bool named = true;
	WzText name;
	WzText power;
	WzText units;
	StatSymbol powerSymbol = StatSymbol("research-power", "⚡");
	StatSymbol unitsSymbol = StatSymbol("research-droid", hudUnitTypeSymbol(HudUnitType::Tank));
	std::string tip;
};

/// The strip's background, with a divider between each of its parts
class StripForm : public W_FORM
{
public:
	void setHelp(nonstd::optional<WidgetHelp> newHelp) override { help = newHelp; }
	WidgetHelp const *getHelp() const override { return help.has_value() ? &help.value() : nullptr; }
	std::vector<int> dividers;

protected:
	void display(int xOffset, int yOffset) override
	{
		auto &skin = currentHudSkin();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (!skin.drawSpectatorStrip(rect))
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), WZCOL_TRANSPARENT_BOX, 2.f, skin.colors().panelBorder, 1.f);
		}
		for (int divider : dividers)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x() + divider), static_cast<float>(rect.y() + 5), static_cast<float>(rect.x() + divider + 1), static_cast<float>(rect.bottom() - 5), skin.colors().panelBorder);
		}
	}

private:
	nonstd::optional<WidgetHelp> help;
};

int stripBottom = 0;

class SpectatorStripModule : public HudModule
{
public:
	HudModuleId moduleId() const override { return HudModuleId::SpectatorStrip; }

	void create(const std::shared_ptr<W_FORM> &root) override
	{
		strip = std::make_shared<StripForm>();
		strip->id = IDHUD_SPECTATOR_STRIP;
		strip->setHelp(WidgetHelp()
			.setTitle(_("Spectator Strip"))
			.setDescription(_("Each player's power and units. The button on the left shows one or two lines per player, and the one on the right opens the full statistics.")));
		root->attach(strip);
		toggle = HudChip::make(WzString());
		toggle->setCardShape(true);
		toggle->onPressed = [this](HudChip &, bool) {
			hudSetSpectatorStripTwoLines(!hudSpectatorStripTwoLines());
			layout();
		};
		strip->attach(toggle);
		more = HudChip::make(WzString());
		more->onPressed = [this](HudChip &, bool) { toggleStats(); };
		strip->attach(more);
		statsButton = HudChip::make(WzString::fromUtf8(_("Stats")), _("Show or hide the spectator statistics"));
		statsButton->setIcon("statistics");
		statsButton->setCardShape(true);
		statsButton->onPressed = [this](HudChip &, bool) { toggleStats(); };
		strip->attach(statsButton);
		strip->setCalcLayout([this](WIDGET *) { layout(); });
		update();
	}

	void destroy() override
	{
		if (strip)
		{
			if (auto parent = strip->parent())
			{
				parent->detach(strip);
			}
			strip.reset();
		}
		entries.clear();
		stripBottom = 0;
		specSetStatsButtonHidden(false);
	}

	void setVisible(bool visible) override
	{
		shown = visible;
		update();
	}

	void update() override
	{
		if (!strip)
		{
			return;
		}
		const bool visible = shown && spectating() && hudTopZone().width() > 0;
		strip->show(visible);
		specSetStatsButtonHidden(visible);
		statsButton->setActive(visible && specStatsViewShown());
		if (!visible)
		{
			stripBottom = 0;
			return;
		}
		players.clear();
		for (unsigned player = 0; player < MAX_PLAYERS; ++player)
		{
			if (playerIsInGame(player))
			{
				players.push_back(player);
			}
		}
		while (entries.size() < players.size())
		{
			auto entry = std::make_shared<PlayerEntry>();
			strip->attach(entry);
			entries.push_back(entry);
		}
		layout();
	}

private:
	void toggleStats()
	{
		specToggleOverlays();
		statsButton->setActive(specStatsViewShown());
	}

	void layout()
	{
		if (!strip)
		{
			return;
		}
		const WzRect zone = hudTopZone();
		const bool twoLines = hudSpectatorStripTwoLines();
		toggle->setText(WzString::fromUtf8(twoLines ? "▴" : "▾"));
		toggle->setIcon(twoLines ? "strip-collapse" : "strip-expand");
		toggle->setTip(twoLines ? _("One line per player") : _("Two lines per player, with the names"));
		const int height = twoLines ? TWO_LINE_HEIGHT : ONE_LINE_HEIGHT;
		const int buttonHeight = twoLines ? TWO_LINE_BUTTON : HUD_CHIP_HEIGHT;
		// The end buttons as far from the sides as from the top and bottom
		const int padding = (height - buttonHeight) / 2;

		auto widthOf = [this, padding](size_t count) {
			int width = padding * 2 + toggle->idealWidth() + GAP + statsButton->idealWidth();
			for (size_t i = 0; i < count; ++i)
			{
				width += entries[i]->idealWidth() + GAP;
			}
			if (count < players.size())
			{
				more->setText(WzString::fromUtf8(astringf("+%zu", players.size() - count)));
				width += more->idealWidth() + GAP;
			}
			return width;
		};
		auto mostThatFit = [&]() {
			size_t count = players.size();
			while (count > 0 && widthOf(count) > zone.width())
			{
				--count;
			}
			return count;
		};
		auto setEntries = [this, twoLines](bool withNames) {
			for (size_t i = 0; i < players.size(); ++i)
			{
				entries[i]->set(players[i], twoLines, withNames);
			}
		};
		setEntries(true);
		size_t count = mostThatFit();
		if (!twoLines && count < players.size())
		{
			setEntries(false);
			count = mostThatFit();
		}
		const int width = widthOf(count);

		std::string hidden;
		for (size_t i = count; i < players.size(); ++i)
		{
			hidden += (hidden.empty() ? "" : ", ") + std::string(getPlayerName(players[i]));
		}
		more->setTip(astringf(_("Also playing: %s"), hidden.c_str()));

		strip->setGeometry(zone.x() + (zone.width() - width) / 2, zone.y(), width, height);
		stripBottom = strip->visible() ? zone.y() + height : 0;
		strip->dividers.clear();
		int x = padding;
		auto place = [&](WIDGET &part, int partWidth, int partHeight, bool last) {
			part.setGeometry(x, (height - partHeight) / 2, partWidth, partHeight);
			x += partWidth + GAP;
			if (!last)
			{
				strip->dividers.push_back(x - GAP / 2);
			}
		};
		place(*toggle, toggle->idealWidth(), buttonHeight, false);
		for (size_t i = 0; i < entries.size(); ++i)
		{
			entries[i]->show(i < count);
			if (i < count)
			{
				place(*entries[i], entries[i]->idealWidth(), entries[i]->idealHeight(), false);
			}
		}
		more->show(count < players.size());
		if (count < players.size())
		{
			place(*more, more->idealWidth(), HUD_CHIP_HEIGHT, false);
		}
		place(*statsButton, statsButton->idealWidth(), buttonHeight, true);
	}

	std::shared_ptr<StripForm> strip;
	std::shared_ptr<HudChip> toggle;
	std::shared_ptr<HudChip> more;
	std::shared_ptr<HudChip> statsButton;
	std::vector<std::shared_ptr<PlayerEntry>> entries;
	std::vector<unsigned> players;
	bool shown = true;
};

}

std::unique_ptr<HudModule> hudMakeSpectatorStrip()
{
	return std::make_unique<SpectatorStripModule>();
}

int hudSpectatorStripBottom()
{
	return stripBottom;
}
