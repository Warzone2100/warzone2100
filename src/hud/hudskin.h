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

#include <glm/mat4x4.hpp>

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
	unsigned count = 0;          ///< a number to show on it (the intelligence button's unread items), when the skin shows one
	bool dot = false;            ///< a dot in its place, without a count (the intelligence button: the objectives changed unseen)
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

/// Icons that a skin may draw instead of the interface images they stand for
enum class HudIcon : uint8_t
{
	ResearchDroid,
	ResearchWeapon,
	ResearchCyborg,
	ResearchSensor,
	ResearchComputer,
	ResearchPower,
	ResearchDefense,
	ResearchStructure,
	Upgrade,
	Damage,
	Accuracy,
	RateOfFire,
	Repair,
	AllyResearch,
	Favorite,
	DeliveryPoint,
	ObsoleteShown,
	ObsoleteHidden,
	Loop,
	Options,
	Statistics,
};

enum class HudIconButtonStyle : uint8_t
{
	Framed,  ///< (in a panel, like its header buttons)
	Bare,    ///< (only the icon)
	Card,    ///< (on its own over the 3D view, like the panels)
};

/// A button that shows an icon (a stats panel's delivery point, obsolete, favorite and loop buttons, the in-game options,
/// the spectator statistics)
struct HudIconButtonLook
{
	WzRect rect;
	HudIcon icon = HudIcon::DeliveryPoint;
	HudIconButtonStyle style = HudIconButtonStyle::Framed;
	bool down = false;
	bool highlighted = false;
	bool active = false;  ///< (on: showing only favorites, a factory looping, the options open)
	bool disabled = false;
	float opacity = 1.f;
};

/// A button of the unit orders panel (or the guide's picture of one): an icon, or an assign-production button's number
struct HudOrderButtonLook
{
	WzRect rect;
	const char *icon = nullptr;  ///< (images/hud/icons/<icon>.svg, and its .fill.svg, if any, in fillTone)
	HudTone fillTone = HudTone::Neutral;
	int number = 0;
	bool down = false;
	bool highlighted = false;
	bool disabled = false;
};

/// A campaign transporter button in the timer row (or the transporter's contents panel): its ETA and load, or launch
struct HudTransporterButtonLook
{
	WzRect rect;
	bool launch = false;     ///< (else the ETA and load button, which opens the load menu)
	bool highlighted = false;
	bool flashOn = false;    ///< (the launch button flashes while the transporter is full)
};

/// The replay controls' buttons (src/hci/replay_controller.cpp)
enum class HudReplayButton : uint8_t
{
	Start,
	Back,
	Pause,
	Play,
	Forward,
	FastForward,
};

/// A replay control button: its screen rectangle and state
struct HudReplayButtonLook
{
	WzRect rect;
	HudReplayButton button = HudReplayButton::Play;
	bool down = false;
	bool highlighted = false;
	bool disabled = false;
	int fastForward = 0; ///< (the fast-forward button: 0 off, else at most how many times as fast it plays: 2 or 4)
};

/// A playhead on the replay timeline: where the replay is, where a drag would seek to, where a seek is going
enum class HudReplayPlayhead : uint8_t
{
	Current,
	Drag,
	SeekTarget,
};

/// A segment of a page control (the stats panels' pages, with panel filters): a page, or the previous or next page arrow
struct HudPageSegmentLook
{
	WzRect rect;
	TabSelectionButtonRole role = TabSelectionButtonRole::Tab;
	bool selected = false;
	bool hovered = false;
	bool disabled = false;   ///< (an arrow at the first or last page)
	bool separator = false;  ///< (a line on its left, between it and the segment before)
	bool large = false;      ///< (a button of the touch page picker, on its own)
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
	/// Whether a production run's size is a badge in the button's top-left corner, instead of Classic's label
	virtual bool drawsProductionRunBadge() const { return false; }
	/// How far a panel with an extension (or room for one) grows up so its frame's top edge clears the contents
	virtual int panelTopPadding() const { return 0; }
	/// A list's page tab or page scroll button (IntListTabWidget). False to draw its images
	virtual bool drawPageTab(const WzRect &, TabSelectionButtonRole, bool, bool) { return false; }
	/// A page control's frame, around its segments
	virtual void drawPageControlFrame(const WzRect &rect) = 0;
	/// Over a list's page just turned by a swipe (IntListTabWidget), as it comes in: `remaining` from 1 to 0
	virtual void drawPageTurnFade(const WzRect &rect, float remaining) { (void)rect; (void)remaining; }
	/// A page control's segment: the color of its number or arrow sign, or nothing if it drew the arrow
	virtual nonstd::optional<PIELIGHT> drawPageSegment(const HudPageSegmentLook &look) = 0;
	/// The mission and transporter timers' background
	virtual void drawTimerBackground(int x, int y, uint16_t image, bool flashOn, uint16_t flashImage) = 0;
	/// How much further right the mission timer's time starts, for the skin's icon before it
	virtual int timerIconGutter() const { return 0; }
	virtual nonstd::optional<WzTipStyle> tooltipStyle() const = 0;

	/// Shapes for HUD additions (modules, popovers), in the skin's style
	virtual void drawCard(const WzRect &rect, HudCardState state) = 0;
	virtual void drawPill(const WzRect &rect, HudTone tone, bool filled) = 0;
	/// A chip (HudChip): the pill's colors, with the corners of the skin's buttons
	virtual void drawChip(const WzRect &rect, HudTone tone, bool filled) { drawPill(rect, tone, filled); }
	virtual void drawProgress(const WzRect &rect, float fraction, HudProgressKind kind) = 0;
	virtual void drawPopoverFrame(const WzRect &rect) = 0;
	virtual void drawInputField(const WzRect &rect, bool editing) = 0;
	/// A text button in a panel's header (the research panel's View Tree): the text color,
	/// or nothing to keep the panel's own look
	virtual nonstd::optional<PIELIGHT> drawHeaderButton(const WzRect &, bool, bool) { return nonstd::nullopt; }
	virtual PIELIGHT toneColor(HudTone tone) const = 0;
	/// Text drawn over the 3D view (console, timers, status line), with an outline when that option is on
	virtual void drawOverlayText(WzText &text, float x, float y, PIELIGHT color) = 0;
	/// Whether it draws HudIcons instead of the images they stand for
	virtual bool drawsIcons() const { return false; }
	/// Whether units' ranks are the rank insignia (hudranks.h) instead of the rank images
	virtual bool drawsRankInsignia() const { return false; }
	/// The radar's blips as the skin's shapes (hudradarblips.h), rather than the images
	virtual bool drawsRadarBlips() const { return false; }
	/// Draws the multiplayer connection warnings as a panel (hudconnection.h) instead of the Classic column of images
	virtual bool drawsConnectionPanel() const { return false; }
	/// Shows the intelligence screen of src/hud/intel (in single player) instead of the Classic one
	virtual bool drawsIntelligenceScreen() const { return false; }
	/// Whether it draws the screen before a networked game starts (src/hud/pregamescreen.h)
	virtual bool drawsPregameScreen() const { return false; }
	/// Whether it draws quick chat (src/hud/hudquickchat.h)
	virtual bool drawsQuickChat() const { return false; }
	/// Whether it draws the mission and game results (src/hud/hudresults.h)
	virtual bool drawsResultsScreens() const { return false; }
	/// Whether it draws the design screen (src/hud/huddesign.h)
	virtual bool drawsDesignScreen() const { return false; }
	/// Whether it draws the in-game console and its chat (src/hud/hudconsole.h)
	virtual bool drawsConsole() const { return false; }
	/// `color` (one of the panels' blue-grays) moved to the accent's hue with `tint` of the accent's chroma
	/// (0: a gray, 1: as the accent's own shades), as the panels follow the accent. The Classic skin keeps it
	virtual PIELIGHT followAccent(PIELIGHT color, float tint) const { (void)tint; return color; }
	/// `color` (its lightness) in the hue of `hueOf`, with the chroma given (OKLCH): a dark tint of a state's color
	virtual PIELIGHT inHueOf(PIELIGHT color, PIELIGHT hueOf, float chroma) const { (void)hueOf; (void)chroma; return color; }
	/// Draws the radar's frame (and, when `north`, its north arrow) before the minimap, in the minimap's own space
	/// (`mapSpace`: the map from 0, 0 to width, height, turned with the camera when the radar turns). False: the radar
	/// draws its Classic images
	virtual bool drawRadarFrame(const glm::mat4 &/*mapSpace*/, float /*width*/, float /*height*/, bool /*north*/) { return false; }
	/// The replay controls: their bar's background, a button, the timeline's track (played up to playedX), a playhead,
	/// the frame of the time the pointer is over on the timeline. False: the Classic look
	virtual bool drawReplayBar(const WzRect &) { return false; }
	virtual bool drawReplayButton(const HudReplayButtonLook &) { return false; }
	virtual bool drawReplayTrack(const WzRect &, float /*playedX*/) { return false; }
	virtual bool drawReplayPlayhead(float /*centerX*/, float /*centerY*/, HudReplayPlayhead) { return false; }
	virtual bool drawReplayTimeTip(const WzRect &) { return false; }
	/// The spectator strip's background. False: the Classic look
	virtual bool drawSpectatorStrip(const WzRect &) { return false; }
	/// Whether it draws the spectator statistics as a panel, with its buttons in its table's header and no title strip.
	/// Then it draws the panel, the table's header (with lines across at `columnLinesX`) and every other row's fill
	virtual bool drawsSpectatorStats() const { return false; }
	virtual void drawSpectatorStatsPanel(const WzRect &) {}
	virtual void drawSpectatorStatsHeader(const WzRect &, const std::vector<int> &/*columnLinesX*/) {}
	virtual PIELIGHT spectatorStatsAlternateRowFill() const { return WZCOL_TRANSPARENT_BOX; }
	/// An icon, in `color` or else the skin's color for it, over whatever is under it: false if it can't (draw the image)
	virtual bool drawHudIcon(HudIcon, const WzRect &, nonstd::optional<PIELIGHT> = nonstd::nullopt) { return false; }
	/// False to draw the button's images
	virtual bool drawIconButton(const HudIconButtonLook &) { return false; }
	/// False to draw the button's images
	virtual bool drawOrderButton(const HudOrderButtonLook &) { return false; }
	/// False to draw the button's images
	virtual bool drawTransporterButton(const HudTransporterButtonLook &) { return false; }
	/// Whether it draws an icon named in the data (images/hud/icons/<name>.svg, ex. a build category's)
	virtual bool hasNamedIcon(const std::string &) { return false; }
	virtual bool drawNamedIcon(const std::string &, const WzRect &, PIELIGHT) { return false; }
	/// Free the text it keeps rendered (the in-game interface is shut down)
	virtual void releaseCaches() {}
};

/// The skin to draw with now: the chosen one in a game, Classic everywhere else (the title screens)
HudSkin &currentHudSkin();
/// The skin the HUD skin setting chooses, in a game or not (for pictures of the in-game interface, like the guide's)
HudSkin &hudSkinForSetting();
HudSkin &classicHudSkin();
HudSkin &modernHudSkin();

#endif // __INCLUDED_SRC_HUD_HUDSKIN_H__
