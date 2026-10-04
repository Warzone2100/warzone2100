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

#ifndef __INCLUDED_SRC_HUD_HUDSKIN_H__
#define __INCLUDED_SRC_HUD_HUDSKIN_H__

#include "hudsettings.h"

#include "lib/framework/geometry.h"
#include "lib/framework/wzstring.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/listwidget.h"
#include "lib/widget/widget.h"

class PowerBar;
class W_BARGRAPH;

/// Semantic colors: what a color means, not which one it is
struct HudColors
{
	PIELIGHT panelBackground;
	PIELIGHT panelBackgroundBottom;
	PIELIGHT panelBorder;
	PIELIGHT accent;
	PIELIGHT textPrimary;
	PIELIGHT textSecondary;
	PIELIGHT stateOk;
	PIELIGHT stateWarn;
	PIELIGHT stateBad;
	PIELIGHT stateDisabled;
	PIELIGHT progressBuild;
	PIELIGHT progressResearch;
	PIELIGHT progressPowerWait;
	PIELIGHT highlight;
	PIELIGHT shadow;
};

struct HudMetrics
{
	float panelRadius;
	float cardRadius;
	float borderWidth;
	int padding;
	iV_fonts titleFont;
	iV_fonts labelFont;
	iV_fonts numberFont;
};

/// One of the object and stats panels' buttons (IntFancyButton): its screen rectangle and state
struct HudFancyButtonLook
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	bool bottomButton = false;  ///< (the object row's lower half: the object, with an arrow up to its job)
	bool down = false;
	bool hovered = false;
	bool emptyBackground = false;
};

/// A reticule button's state, as the scripts and the player left it (src/hci.cpp)
struct HudReticuleButtonLook
{
	int slot = 0;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	WzString upImage;
	WzString downImage;
	bool isCancel = false;
	bool disabled = false;
	bool pressed = false;
	bool firstPressedFrame = false;
	bool flashing = false;
	bool paused = false;
	bool highlighted = false;
	bool panelOpen = false;
};

enum class HudCardState : uint8_t
{
	Normal,
	Highlighted,
	Selected,
	Disabled,
};

enum class HudProgressKind : uint8_t
{
	Build,
	Research,
	PowerWait,
	Neutral,
};

enum class HudTone : uint8_t
{
	Neutral,
	Accent,
	Ok,
	Warn,
	Bad,
};

/// How the HUD's elements are drawn. The geometry is never the skin's business: every skin draws inside the
/// rectangles the Classic layout gives it.
class HudSkin
{
public:
	virtual ~HudSkin() = default;

	virtual HudSkinId id() const = 0;
	virtual const HudColors &colors() const = 0;
	virtual const HudMetrics &metrics() const = 0;

	/// An IntFormAnimated panel's frame, at its current (maybe animating) rectangle
	virtual void drawPanelFrame(const WzRect &rect, const WzRect &openRect) = 0;
	virtual void drawFancyButtonBackground(const HudFancyButtonLook &look) = 0;
	virtual void drawFancyButtonHighlight(const HudFancyButtonLook &look) = 0;
	/// A factory or research facility on hold
	virtual void drawFancyButtonOnHold(const HudFancyButtonLook &look) = 0;
	/// A button with nothing to show (an idle factory's or research facility's job), instead of its background: false to
	/// draw the background and the question mark image
	virtual bool drawFancyButtonEmpty(const HudFancyButtonLook &) { return false; }
	virtual void drawReticuleButton(const HudReticuleButtonLook &look) = 0;
	virtual void drawPowerBar(PowerBar &bar, int xOffset, int yOffset) = 0;
	/// The progress and cost bars inside the object and stats buttons
	virtual void drawButtonBar(W_BARGRAPH &bar, int xOffset, int yOffset) = 0;
	/// A panel's close button
	virtual void drawCloseButton(int x, int y, int width, int height, bool highlighted) = 0;
	/// A stats panel item's cost bar: `normal`, or (Modern) a warning when it can't be afforded now
	virtual PIELIGHT statsCostBarColor(PIELIGHT normal, bool) const { return normal; }
	/// A list's page tab or page scroll button (IntListTabWidget). False to draw its images
	virtual bool drawPageTab(const WzRect &, TabSelectionButtonRole, bool, bool) { return false; }
	/// The mission and transporter timers' background
	virtual void drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t flashImage) = 0;
	virtual nonstd::optional<WzTipStyle> tooltipStyle() const = 0;

	/// Shapes for HUD additions (modules, popovers), in the skin's style
	virtual void drawCard(const WzRect &rect, HudCardState state) = 0;
	virtual void drawPill(const WzRect &rect, HudTone tone, bool filled) = 0;
	/// A chip (HudChip): the pill's colors, with the corners of the skin's buttons
	virtual void drawChip(const WzRect &rect, HudTone tone, bool filled) { drawPill(rect, tone, filled); }
	virtual void drawProgress(const WzRect &rect, float fraction, HudProgressKind kind) = 0;
	virtual void drawPopoverFrame(const WzRect &rect) = 0;
	/// A text button in a panel's header (the research panel's View Tree): the text color,
	/// or nothing to keep the panel's own look
	virtual nonstd::optional<PIELIGHT> drawHeaderButton(const WzRect &, bool, bool) { return nonstd::nullopt; }
	virtual PIELIGHT toneColor(HudTone tone) const = 0;
	/// Text drawn over the 3D view (console, timers, status line), with an outline when that option is on
	virtual void drawOverlayText(WzText &text, float x, float y, PIELIGHT color) = 0;
	/// Free the text it keeps rendered (the in-game interface is shut down)
	virtual void releaseCaches() {}
};

/// The skin to draw with now: the chosen one in a game, Classic everywhere else (the title screens)
HudSkin &currentHudSkin();
HudSkin &classicHudSkin();
HudSkin &modernHudSkin();

#endif // __INCLUDED_SRC_HUD_HUDSKIN_H__
