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
 *  Detail panels: the building blocks of a rich, custom-drawn description of one thing
 *  (a header with a model, a title and badges, a row of stat cells, a body of sections, and actions),
 *  shared by the research tree's detail popover and the HUD's detail popovers.
 */

#ifndef __INCLUDED_SRC_WIDGETS_DETAILPANEL_DETAILPANEL_H__
#define __INCLUDED_SRC_WIDGETS_DETAILPANEL_DETAILPANEL_H__

#include "lib/widget/widget.h"
#include "lib/widget/button.h"

#include <functional>
#include <memory>
#include <vector>

class Paragraph;

/// Colors and sizes of a detail panel. The research tree uses the defaults, and the HUD fills it from its skin.
struct DetailPanelStyle
{
	PIELIGHT background = pal_RGBA(24, 24, 24, 255);
	PIELIGHT border = pal_RGBA(0, 0, 0, 0);
	PIELIGHT divider = pal_RGBA(0, 0, 0, 0);
	float cornerRadius = 6.f;
	uint8_t restingAlpha = 232;
	bool solidWhenPointedAt = true;
	/// Draws the panel's background instead (the HUD's popover frame), when set
	std::function<void (const WzRect &rect)> drawBackground;
	/// Draws a stat cell's icon (an IntImages id) another way, ex. as a skin's vector icon. False: as the image
	std::function<bool (uint16_t image, float x, float y, float size, PIELIGHT colour)> drawIcon;

	static DetailPanelStyle researchTree();
};

/// Whether the status goes in a band of its own under the header, or as a badge in the header
enum class DetailStatusPlacement : uint8_t
{
	/// In a band when the header has stat cells (which leave no room for it), as a badge otherwise
	Automatic,
	Band,
	Badge,
};

// Panel describing one thing.
// The background is deliberately not solid (in the research tree's style), so what is drawn underneath stays
// faintly readable through it. Pointing at it makes it solid.
class DetailPanelContents : public WIDGET
{
public:
	static constexpr int32_t MAX_PANEL_HEIGHT = 400;
	static constexpr int MODEL_SIZE = 74;
	static constexpr int PADDING = 10;

	// `group` and `status` are badges beside the model. Either may be null, and so may the `model`, the `subtitle` and
	// the `body`, in the case of a panel that describes nothing.
	static std::shared_ptr<DetailPanelContents> make(std::shared_ptr<WIDGET> model, std::shared_ptr<WIDGET> title, std::shared_ptr<WIDGET> subtitle, std::shared_ptr<WIDGET> group, std::shared_ptr<WIDGET> status, std::vector<std::shared_ptr<WIDGET>> stats, std::shared_ptr<WIDGET> body, std::shared_ptr<WIDGET> actions, int32_t contentWidth, const DetailPanelStyle &panelStyle = DetailPanelStyle::researchTree(), DetailStatusPlacement statusPlacement = DetailStatusPlacement::Automatic, int modelSize = MODEL_SIZE);

	// What the background settles at when the pointer is elsewhere
	void setRestingAlpha(uint8_t alpha) { m_style.restingAlpha = alpha; }
	// The tallest the panel may be. (Whatever the body cannot fit, scrolls.)
	// Call before the popover is made, since that takes the size as it stands.
	void setMaxHeight(int32_t maxHeight);
	// Actions along the bottom (at their ideal height), set after the panel is made
	void setActions(std::shared_ptr<WIDGET> actions);
	// A small widget in the header's top right corner (ex. a close button), which the title leaves room for
	void setCorner(std::shared_ptr<WIDGET> corner);

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	int32_t idealWidth() override { return width(); }
	int32_t idealHeight() override { return height(); }

private:
	void recalcHeight();
	int headerHeight() const;
	int statsY() const;
	bool statusInBar() const;
	int statusBarHeight() const;
	int actionsHeight() const;
	// Whether the header has anything under it to be divided from.
	bool anythingUnderHeader() const;

	std::shared_ptr<WIDGET> m_model;
	std::shared_ptr<WIDGET> m_title;
	std::shared_ptr<WIDGET> m_subtitle;
	std::shared_ptr<WIDGET> m_group;		// null on a subject that stands alone
	std::shared_ptr<WIDGET> m_status;
	// What it costs, what it takes, how long it will be.
	std::vector<std::shared_ptr<WIDGET>> m_stats;
	std::shared_ptr<WIDGET> m_body;
	std::shared_ptr<WIDGET> m_actions;		// null on a panel that only describes
	std::shared_ptr<WIDGET> m_corner;
	int32_t m_maxHeight = MAX_PANEL_HEIGHT;
	DetailPanelStyle m_style;
	DetailStatusPlacement m_statusPlacement = DetailStatusPlacement::Automatic;
	int m_modelSize = MODEL_SIZE;
};

/// The bold title, shrunk to fit, then cut with an ellipsis (the full text is then its tooltip)
std::shared_ptr<WIDGET> makeDetailPanelTitle(const WzString& text);
/// A stat cell: a small label over a value, with an optional icon (an IntImages id)
std::shared_ptr<WIDGET> makeDetailPanelStat(const WzString& label, const WzString& value, optional<uint16_t> icon, const std::string& tip);
/// A fully rounded badge
std::shared_ptr<WIDGET> makeDetailPanelBadge(const WzString& text, PIELIGHT fill, PIELIGHT textColor);
/// A status, followed by the names of players it concerns (each in their color), or their count when they do not fit
std::shared_ptr<WIDGET> makeDetailPanelStatusLine(const WzString& standing, PIELIGHT color, const WzString& prefix, const std::vector<uint32_t>& players);
/// What something requires, as a checklist wrapped across the width: a check for what is done, an empty circle otherwise
std::shared_ptr<WIDGET> makeDetailPanelChecklist(const WzString& heading, std::vector<std::pair<WzString, bool>> items);
/// A box that draws a model (or anything) fitted inside it, on a dark plate
std::shared_ptr<WIDGET> makeDetailPanelModel(const std::function<void (int x0, int y0, int width, int height)>& drawModel);

/// A section heading in a body paragraph, with the text after it in the body color
void detailPanelAddSection(const std::shared_ptr<Paragraph>& body, const WzString& heading);
/// A paragraph set up as a panel body of the given width
std::shared_ptr<Paragraph> makeDetailPanelParagraph(int32_t width);

/// A player's color, lifted until it can be read as text
PIELIGHT detailPanelReadableTeamColor(PIELIGHT color);

// A small bordered button, shared by a panel's actions and the research view's toolbar
std::shared_ptr<W_BUTTON> makeDetailPanelActionButton(const WzString& text, const WzString& tip);
// The same button without the accent, for secondary buttons beside whatever is the primary action
std::shared_ptr<W_BUTTON> makeDetailPanelQuietButton(const WzString& text, const WzString& tip);
/// A row that lays out action buttons side by side
std::shared_ptr<WIDGET> makeDetailPanelActionRow();

// Shared measures
static constexpr int DETAIL_PANEL_BODY_LINE_SPACING = 2;
PIELIGHT detailPanelHeadingColor();
PIELIGHT detailPanelBuffColor();
PIELIGHT detailPanelNerfColor();

#endif // __INCLUDED_SRC_WIDGETS_DETAILPANEL_DETAILPANEL_H__
