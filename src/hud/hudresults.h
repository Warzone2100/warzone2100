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

/** @file
 *  The results screens in the Modern skin: a campaign mission's (its objectives, the fight, the force and its ranks),
 *  and a skirmish or multiplayer game's, the same for players, spectators and replays (Overview, Charts, Research,
 *  Timeline).
 */

#ifndef __INCLUDED_SRC_HUD_HUDRESULTS_H__
#define __INCLUDED_SRC_HUD_HUDRESULTS_H__

#include "lib/framework/wzglobal.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/widget.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

/// Whether the skin draws the results
bool hudResultsEnabled();
/// Whether they take their compact layout (a small screen, down to 640×360)
bool hudResultsCompact();
/// `text` cut (with an ellipsis) to at most `maxWidth` pixels in `font` (iV_TruncateTextToWidth())
std::string hudCappedText(const std::string &text, iV_fonts font, int maxWidth);

/// The results' display fonts (DejaVu Sans Condensed Bold, bound by the sheet for its layout):
/// the title, the header's figures, labels in capitals, the subtitle, the tabs, buttons and section headings, the chart's
/// list of metrics
constexpr iV_fonts RESULTS_TITLE_FONT = font_display_1;
constexpr iV_fonts RESULTS_FIGURE_FONT = font_display_2;
constexpr iV_fonts RESULTS_LABEL_FONT = font_display_3;
constexpr iV_fonts RESULTS_SUBTITLE_FONT = font_display_4;
constexpr iV_fonts RESULTS_TAB_FONT = font_display_5;
constexpr iV_fonts RESULTS_RAIL_FONT = font_display_6;
/// `text` in capitals (by Unicode code point)
std::string hudResultsCaps(const std::string &text);
/// The height of a display font's capitals (in points), as bound for the layout
float hudResultsCapHeight(iV_fonts font, bool compact);

/// How far the screen's entrance has come at `delayMs` into it, over `durationMs` (0 to 1, eased, 1 when reducing motion)
float hudResultsAppear(float delayMs, float durationMs);

/// The results' colors, from the panels' blue-grays: a surface's (the accent's hue, at most as a Subtle panel
/// tint), a chosen thing's (the accent's shades), and a neutral one (gray, the header's)
PIELIGHT hudResultsSurface(PIELIGHT color);
PIELIGHT hudResultsChosen(PIELIGHT color);
PIELIGHT hudResultsNeutral(PIELIGHT color);

struct HudResultsButton
{
	UDWORD id = 0;                    ///< (as the Classic screen's: the screen's run reports it)
	std::string label;
	bool primary = false;
	std::function<void ()> onClick;   ///< (or that)
};

/// A campaign mission's results, filling the screen
std::shared_ptr<WIDGET> hudMakeCampaignResults(bool won, std::vector<HudResultsButton> buttons);
/// A skirmish or multiplayer game's results, filling the screen: a player's (won or lost), or a spectator's (`spectator`).
/// `note`: a line under the title (ex. the host has quit)
std::shared_ptr<WIDGET> hudMakeGameResults(bool spectator, bool won, const std::string &note, std::vector<HudResultsButton> buttons);
/// The results last made, while they're attached (else nullptr)
std::shared_ptr<WIDGET> hudResultsRoot();

// The results' frame, shared by both

class HudResultsSheet : public WIDGET
{
public:
	/// `eyebrow`: the line over the title (not in the compact layout). `subtitle`: its parts, shown in capitals with a
	/// slash between (the first is cut short when they don't fit). `tinted`: the band in a dark tint of `color` (a
	/// player's Victory or Defeat), else neutral
	void setHeader(const std::string &icon, PIELIGHT color, const std::string &eyebrow, const std::string &title, std::vector<std::string> subtitle,
	               bool tinted = false);
	/// The footer's note (left of the buttons): an icon, a text and, if `onLink`, a link after it
	void setFooterNote(const std::string &icon, const std::string &text, const std::string &link = std::string(), std::function<void ()> onLink = nullptr);
	/// A tooltip on the subtitle (ex. more about the game)
	void setSubtitleTip(const std::string &tip);
	void setButtons(std::vector<HudResultsButton> buttons);
	/// The sheet's size on a screen
	virtual WzRect sheetRect(int screenW, int screenH) const = 0;
	/// Whether it has the compact layout (chosen when made, by the screen's size)
	bool isCompact() const { return compact; }

	void geometryChanged() override;
	void display(int xOffset, int yOffset) override;

protected:
	/// The header's height (with its edge in the result's color), taller where the sheet has room
	int headerH() const;
	/// How wide the subtitle can be: the title's room, or past the header's right where it runs below it
	int availableSubtitleWidth() const;
	int footerH() const { return compact ? 40 : 58; }
	int padW() const { return compact ? 10 : 22; }
	bool compact = false;
	/// Binds the display fonts, the title's at its full size until the title is fitted
	void bindFonts();

	/// The content's rectangle, inside the sheet (under the header, over the footer)
	WzRect contentRect() const;
	/// How wide the header's title can be, on this screen
	int availableTitleWidth() const;
	/// Where the header's lines go (baselines, in the sheet), and the left of its text
	struct HeaderLines
	{
		float eyebrowBase = 0.f;
		float titleBase = 0.f;
		float subtitleBase = 0.f;
		float subtitleCap = 0.f;
		int textX = 0;
	};
	HeaderLines headerLines() const;
	/// The header's middle, under its edge
	int headerCenterY() const { return edgeH() + (headerH() - edgeH()) / 2; }
	int edgeH() const { return compact ? 2 : 3; }
	int badgeSize() const { return compact ? 30 : 50; }
	/// What the header shows on its right, right-aligned at `right`, centered on `cy`
	virtual void displayHeaderRight(int right, int cy) { (void)right; (void)cy; }
	virtual void displayContent(int x0, int y0) { (void)x0; (void)y0; }
	virtual int headerExtra() const { return 0; }
	/// How wide displayHeaderRight's content is (the subtitle stops short of it)
	virtual int headerRightWidth() const { return 0; }
	/// The room the title at its full size and the whole subtitle leave on the header's right
	int availableHeaderRightWidth() const;

private:
	void layoutButtons();
	/// The title at the largest size that fits, down to a floor (then cut short, with an ellipsis)
	void fitTitle();
	void layoutSubtitle();
	int subtitleFullWidth() const;
	int footerNoteWidth() const;
	bool footerNoteFits() const;

	std::string iconName;
	PIELIGHT accent;
	bool bandTinted = false;
	std::string eyebrow;
	std::string title;
	std::vector<std::string> subtitle;
	int subtitleWidth = 0;
	int fittedFor = -1;   ///< (the title room the title was fitted to)
	float titleSize = 44.f;   ///< (the size the title's font is bound at)
	std::string footerIcon;
	std::string footerText;
	std::shared_ptr<W_BUTTON> footerLink;
	std::shared_ptr<W_BUTTON> subtitleTip;
	std::vector<std::shared_ptr<WIDGET>> buttons;
	WzText titleText;
	WzText eyebrowText;
	std::vector<WzText> subtitleTexts;
	WzText slashText;
	WzText footerTextCache;
};

#endif // __INCLUDED_SRC_HUD_HUDRESULTS_H__
