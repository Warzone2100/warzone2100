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

#ifndef __INCLUDED_SRC_HUD_HUDCLASSICSKIN_H__
#define __INCLUDED_SRC_HUD_HUDCLASSICSKIN_H__

#include "hudskin.h"

/// The Classic HUD, drawn like it used to
class ClassicHudSkin : public HudSkin
{
public:
	HudSkinId id() const override { return HudSkinId::Classic; }
	const HudColors &colors() const override;
	const HudMetrics &metrics() const override;

	void drawPanelFrame(const WzRect &rect, const WzRect &openRect) override;
	void drawFancyButtonBackground(const HudFancyButtonLook &look) override;
	void drawFancyButtonHighlight(const HudFancyButtonLook &look) override;
	void drawFancyButtonOnHold(const HudFancyButtonLook &look) override;
	void drawReticuleButton(const HudReticuleButtonLook &look) override;
	void drawPowerBar(PowerBar &bar, int xOffset, int yOffset) override;
	void drawButtonBar(W_BARGRAPH &bar, int xOffset, int yOffset) override;
	void drawCloseButton(int x, int y, int width, int height, bool highlighted) override;
	void drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t flashImage) override;
	nonstd::optional<WzTipStyle> tooltipStyle() const override;

	void drawCard(const WzRect &rect, HudCardState state) override;
	void drawPill(const WzRect &rect, HudTone tone, bool filled) override;
	void drawProgress(const WzRect &rect, float fraction, HudProgressKind kind) override;
	void drawPopoverFrame(const WzRect &rect) override;
	void drawInputField(const WzRect &rect, bool editing) override;
	void drawPageControlFrame(const WzRect &rect) override;
	nonstd::optional<PIELIGHT> drawPageSegment(const HudPageSegmentLook &look) override;
	PIELIGHT toneColor(HudTone tone) const override;
	void drawOverlayText(WzText &text, float x, float y, PIELIGHT color) override;
};

#endif // __INCLUDED_SRC_HUD_HUDCLASSICSKIN_H__
