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
 *  The research tree canvas.
 */

#include "researchtreecanvas.h"

#include "lib/framework/input.h"
#include "lib/framework/gamepad_input.h"
#include "lib/framework/math_ext.h" // for M_PI on MSVC
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "../research.h"
#include "../component.h"
#include "../intimage.h"
#include "../hci.h"
#include "../hci/research.h"
#include "../multiplay.h"	// for IdToStruct
#include "../console.h"
#include "../frontend.h"		// for displayTextOption / DisplayTextOptionCache
#include "lib/widget/button.h"
#include "lib/widget/popovermenu.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

// Sufficient gap so that a dragged canvas never quite leaves the widget empty
static constexpr int32_t CANVAS_MARGIN = 80;
static constexpr float ZOOM_MIN = 0.30f;
static constexpr float ZOOM_MAX = 1.60f;
static constexpr int DRAG_THRESHOLD = 5;

// Sizes that stop scaling once the view is zoomed out, so they stay legible
static constexpr float ICON_MIN_PX = 7.f;
static constexpr float UNIT_PAD_MIN_PX = 4.f;
static constexpr float UNIT_PAD_PER_ROW_PX = 0.15f;
static constexpr float PIP_MIN_PX = 2.5f;
// Based on the row rather than the zoom (since the two densities reach a given row height at different zooms)
static constexpr float PIP_PER_ROW_PX = 0.125f;

// Two densities differing only in grid pitch, so moving between them slides
static const LayoutMetrics COMPACT_METRICS = { 32, 10, 72, 4, 0 };
static const LayoutMetrics LABELED_METRICS = { 168, 30, 240, 14, LABELED_LEFT_GUTTER };

static constexpr uint32_t PITCH_TIME = 250;

// A unit's name sits outside it, which leaves the inside for its steps
static constexpr float GROUP_NAME_GAP_PX = 6.f;
static constexpr float PINNED_NAME_PAD_PX = 5.f;
static constexpr float GROUP_NAME_CLEARANCE_PX = 10.f;
static constexpr float GROUP_NAME_LEAST_PX = 34.f;
// Names on the steps need the room between one pip and the next
static constexpr float ZOOM_STEP_NAMES = 0.95f;
// Below this the labels are unreadable, so the nodes become plain markers
static constexpr float ZOOM_LABELS = 0.45f;
static constexpr float ZOOM_DETAIL = 0.95f;

static constexpr int32_t DETAIL_WIDTH = 300;
static constexpr int32_t DETAIL_MAX_WIDTH = 460;
static constexpr int DETAIL_NODE_GAP = 8;
static constexpr int DETAIL_PANEL_DROP = 4; // what the popover adds below its anchor

// The panel never fully hides the canvas, and becomes more transparent while the view is moving
static constexpr uint8_t DETAIL_ALPHA_RESTING = 232;
static constexpr uint8_t DETAIL_ALPHA_MOVING = 130;
static constexpr uint32_t DETAIL_MOVING_LINGER = 500;

// Outside the pip rather than on its edge, so it stays a ring (instead of appearing as a thicker pip)
static constexpr float TEAM_RING_LEAST_PX = 2.f;

// Two labs on one topic is throughput going nowhere, and something the player would want to undo
static const PIELIGHT WASTED_RING_COLOR = pal_Colour(240, 120, 70);
static const PIELIGHT WASTED_ROW_COLOR = pal_RGBA(240, 120, 70, 60);

// How often the game is asked what has changed (i.e. more costly state updates)
static constexpr uint32_t STATE_REFRESH_MS = 250;

// Long enough to follow movement on reflow, but short enough that the tree doesn't take too long to settle
static constexpr uint32_t REFLOW_TIME = 250;

// Long enough to catch the eye after the view jumps
static constexpr uint32_t REVEAL_MARK_TIME = 2500;
static constexpr uint32_t REVEAL_MARK_PERIOD = 700;

static constexpr uint32_t PIP_PULSE_PERIOD = 1500;
// How far the ripple travels beyond the pip and how strong it starts
static constexpr float RIPPLE_LEAST_PX = 5.f;
static constexpr float RIPPLE_ALPHA = 190.f;
// How much larger a startable step is drawn, and how quickly that arrives as the pip grows.
// Keyed to the pip rather than the zoom (since the zoom a tree opens at depends on its size).
static constexpr float ACTIONABLE_PIP_BOOST = 1.3f;
static constexpr float ACTIONABLE_EXTRA_PER_PX = 0.6f;
// A ring in the state's color needs room to read as an actual ring.
// Under this value the dark cut carries the state alone.
static constexpr float STATE_RING_LEAST_RADIUS_PX = 3.5f;
// The dark cut around a startable step, and the ring of its color outside it.
static constexpr float KEYLINE_WIDTH_PX = 1.f;

static constexpr float GAMEPAD_ZOOM_RATE = 2.6f;

// Wide enough for: a lab, its rate, and what it is being asked to drop
static constexpr int LAB_ROW_WIDTH = 330;
static constexpr int LAB_ROW_HEIGHT = 16;

static PIELIGHT laneColor(uint8_t lane)
{
	// Distinct hues, intended to separate at a glance across eight bands.
	static const PIELIGHT colors[] = {
		pal_Colour(120, 170, 220),	// blue, droids
		pal_Colour(220, 130, 110),	// red, weapons
		pal_Colour(150, 200, 140),	// green, computers
		pal_Colour(215, 190, 110),	// amber, power
		pal_Colour(165, 85, 140),	// magenta, systems
		pal_Colour(130, 200, 200),	// teal, structures
		pal_Colour(120, 110, 240),	// indigo, cyborgs
		pal_Colour(175, 180, 190),	// gray, defenses
		pal_Colour(200, 200, 130),	// olive, anything uncategorized
	};
	return colors[lane % (sizeof(colors) / sizeof(colors[0]))];
}

// The canvas base color - what a node's fill is mixed against.
// (Neutral, since multiple lane hues and state colors sit on it.)
static PIELIGHT canvasBase() { return pal_RGBA(14, 14, 14, 255); }

// A traced layout keeps a progression whole, so steps past the target stay drawn but are faded
static constexpr float OFF_PATH_FADE = 0.28f;

// How much of its lane color a node's fill takes.
// (Never all of it, since edges run under the nodes and a line would otherwise vanish completely behind one.)
// "Solidity" says whether the progression holds anything startable.
static constexpr float NODE_TINT = 0.20f;
static constexpr float SOLIDITY_ACTIVE = 0.88f;	// something here can be started
static constexpr float SOLIDITY_IDLE = 0.62f;
static constexpr float SOLIDITY_HOVER = 0.08f;

static PIELIGHT blendToward(PIELIGHT from, PIELIGHT to, float amount)
{
	const auto mix = [amount](uint8_t a, uint8_t b) {
		return static_cast<uint8_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * amount);
	};
	return pal_RGBA(mix(from.byte.r, to.byte.r), mix(from.byte.g, to.byte.g), mix(from.byte.b, to.byte.b), 255);
}

struct StatePaint
{
	float solidity;		// how much of what is behind the node it keeps out
	uint8_t contentAlpha;	// icon and label
};

static StatePaint statePaintFor(NodeState state, bool active, bool hovered)
{
	const float solidity = std::min(1.f, (active ? SOLIDITY_ACTIVE : SOLIDITY_IDLE)
	                                     + (hovered ? SOLIDITY_HOVER : 0.f));
	switch (state)
	{
	case NodeState::Available:	return { solidity, 255 };
	case NodeState::InProgress:	return { solidity, 250 };
	case NodeState::Researched:	return { solidity, 225 };
	case NodeState::Locked:		return { solidity, 120 };
	case NodeState::Disabled:	return { solidity, 105 };
	default:			return { solidity, 150 };
	}
}

// How one step is drawn.
// Every pip is the same size whatever its state, and filled (vs hollow) says whether the player has it.
// Startable and in-progress take colors of their own, being the only actionable states.
static float actionableExtraPx(float radius)
{
	return std::min(radius * (ACTIONABLE_PIP_BOOST - 1.f),
	                std::max(0.f, (radius - PIP_MIN_PX) * ACTIONABLE_EXTRA_PER_PX));
}

static bool stateRingVisible(float radius)
{
	return radius >= STATE_RING_LEAST_RADIUS_PX;
}

// How far out a step is drawn (past its pip wherever the state puts something around it).
// Names are placed off this, so one never sits against a ring.
// `grown` is the larger pip a startable step gets, `ringed` the dark cut and color band outside it,
// (which a step can have without growing).
static float drawnPipReachPx(float radius, bool grown, bool ringed = false)
{
	const float pip = radius + (grown ? actionableExtraPx(radius) : 0.f);
	if (!grown && !ringed)
	{
		return pip;
	}
	return pip + ((stateRingVisible(radius) || ringed) ? 2.f : 1.f) * KEYLINE_WIDTH_PX;
}

// How much of its lane color a finished step keeps
static constexpr float RESEARCHED_PIP_STRENGTH = 0.70f;

// Clear of whatever the pip drew around itself, so a step name starting there does not run into it
static constexpr int HELD_LABEL_GAP_PX = 5;

PIELIGHT researchTreeCanvasGround()
{
	return canvasBase();
}

PIELIGHT researchLaneSwatchColor(uint8_t lane)
{
	return blendToward(canvasBase(), laneColor(lane), RESEARCHED_PIP_STRENGTH);
}

struct PipPaint
{
	PIELIGHT color;
	bool hollow;
};

static PipPaint pipPaintFor(NodeState state, PIELIGHT lane)
{
	switch (state)
	{
	case NodeState::Available:	return { pal_Colour(70, 200, 255), false };
	case NodeState::InProgress:	return { researchInProgressColor(), false };
	// Held back off its lane color.
	// A filled pip already says finished, so the strongest color is better spent on what the player can act on.
	case NodeState::Researched:	return { blendToward(canvasBase(), lane, RESEARCHED_PIP_STRENGTH), false };
	// Not startable yet, but everything under it is researched or on its way.
	// Keeps its lane (since where a line leads is worth knowing before it can be taken).
	case NodeState::Reachable:	return { blendToward(canvasBase(), lane, 0.70f), true };
	case NodeState::Disabled:	return { blendToward(canvasBase(), pal_Colour(150, 105, 105), 0.45f), true };
	default:			return { blendToward(canvasBase(), pal_Colour(120, 132, 150), 0.45f), true };
	}
}

WzResearchTreeCanvas::~WzResearchTreeCanvas()
{
	m_rectBatch.reset();
}

std::shared_ptr<WzResearchTreeCanvas> WzResearchTreeCanvas::make(const ResearchTreeContext& context)
{
	class make_shared_enabler : public WzResearchTreeCanvas {};
	auto canvas = std::make_shared<make_shared_enabler>();
	canvas->m_rectBatch.initialize();
	canvas->setContext(context);
	return canvas;
}

void WzResearchTreeCanvas::setContext(const ResearchTreeContext& context)
{
	m_context = context;
	rebuild();
}

void WzResearchTreeCanvas::rebuild()
{
	m_graph = ResearchGraph::build(m_context);
	m_visible = researchVisibleTo(m_context);
	m_lanesShown = researchLanesInUse(m_visible);
	m_known = researchKnownTo(m_context);
	m_duplicated = duplicatedResearch(m_context);
	refreshHeldCounts();
	const ResearchPrereqClosure closure(asResearch);
	m_tracks = deriveResearchTracks(closure);
	m_focusTarget.reset();
	m_focus = ResearchFocus();
	rebuildLayout();
}

// The graph and the progressions outlive a change of focus - only which are placed (and where) has to be calculated again
void WzResearchTreeCanvas::rebuildLayout()
{
	// What a traced path wants, and what the player may see.
	// A topic out of view leaves its progression, one merely off the path stays and is faded-out.
	m_layout = ResearchTreeLayout::build(m_graph, m_tracks, ResearchTreeLayout::TrackMode::Collapse,
	                                     ResearchTreeLayout::Ordering::LaneGrouped, LayoutMetrics(),
	                                     m_focusTarget ? &m_focus.included : nullptr,
	                                     m_visible.empty() ? nullptr : &m_visible);
	m_layout.setMetrics(presentationMetrics());

	// NOTE: Units are re-numbered here, so anything still traveling from the last
	// time they were placed is now describing whichever unit inherited its number
	m_reflowOffset.clear();
	m_reflowStart = 0;

	closeDetail();
	m_hover.reset();
	m_selection.reset();
	m_visibleUnits.clear();

	m_unitLabels.clear();
	m_unitLabels.resize(m_layout.unitCount());
	m_stepLabels.clear();
	m_stepLabels.resize(m_layout.unitCount());
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		m_stepLabels[unit].resize(m_layout.units()[unit].members.size());
	}
	rebuildLabelText();

	m_unitOfResearch.assign(asResearch.size(), UINT32_MAX);
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		for (const auto member : m_layout.units()[unit].members)
		{
			if (member < m_unitOfResearch.size())
			{
				m_unitOfResearch[member] = unit;
			}
		}
	}

	m_picksInReadingOrder.clear();
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		for (size_t member = 0; member < m_layout.units()[unit].members.size(); ++member)
		{
			m_picksInReadingOrder.push_back({unit, member});
		}
	}
	std::sort(m_picksInReadingOrder.begin(), m_picksInReadingOrder.end(), [this](const TreePick& a, const TreePick& b) {
		const auto& units = m_layout.units();
		const int32_t tierA = units[a.unit].memberTiers[a.member];
		const int32_t tierB = units[b.unit].memberTiers[b.member];
		if (tierA != tierB) { return tierA < tierB; }
		return units[a.unit].row + units[a.unit].memberRows[a.member]
		     < units[b.unit].row + units[b.unit].memberRows[b.member];
	});

	m_viewMoved = false;
	fitToView();
	rebuildLabels();
}

void WzResearchTreeCanvas::stepSelection(int delta)
{
	if (m_picksInReadingOrder.empty() || delta == 0)
	{
		return;
	}
	size_t at = 0;
	if (m_selection)
	{
		const auto found = std::find(m_picksInReadingOrder.begin(), m_picksInReadingOrder.end(), *m_selection);
		if (found != m_picksInReadingOrder.end())
		{
			const int64_t count = static_cast<int64_t>(m_picksInReadingOrder.size());
			const int64_t index = static_cast<int64_t>(found - m_picksInReadingOrder.begin());
			at = static_cast<size_t>(((index + delta) % count + count) % count);
		}
	}
	const TreePick& pick = m_picksInReadingOrder[at];
	revealResearch(m_layout.units()[pick.unit].members[pick.member]);
}

// Whether a topic is one the traced target actually rests on. (Everything counts when no path is being traced.)
bool WzResearchTreeCanvas::onTracedPath(uint16_t researchIndex) const
{
	return !m_focusTarget
		|| (researchIndex < m_focus.included.size() && m_focus.included[researchIndex]);
}

// Where a unit's box stops being part of a traced path, in screen x.
// The cut goes past the furthest step the path reaches, so everything drawn after it is faded.
// Steps before the cut that the path does not need keep the fill and are marked by their own
// pips, to avoid one (cut) box appearing as multiple boxes.
// Empty when no path is traced and when nothing is drawn after the cut.
optional<float> WzResearchTreeCanvas::tracedSplitX(uint32_t unit, const WzRect& box, int originX) const
{
	if (!m_focusTarget)
	{
		return nullopt;
	}
	const auto& members = m_layout.units()[unit].members;
	const auto& tiers = m_layout.units()[unit].memberTiers;
	int32_t lastOnTier = INT32_MIN;
	float lastOnPath = -std::numeric_limits<float>::max();
	for (size_t m = 0; m < members.size(); ++m)
	{
		if (onTracedPath(members[m]))
		{
			lastOnTier = std::max(lastOnTier, tiers[m]);
			lastOnPath = std::max(lastOnPath, pipScreenX(unit, m, box, originX));
		}
	}
	if (lastOnTier == INT32_MIN
	    || std::none_of(tiers.begin(), tiers.end(), [lastOnTier](int32_t tier) { return tier > lastOnTier; }))
	{
		return nullopt;
	}
	// Half a tier past the last step on the path (where the next pip would sit)
	return lastOnPath + static_cast<float>(m_layout.metrics().tierSpacing) * m_zoom / 2.f;
}

optional<uint16_t> WzResearchTreeCanvas::actionableTopic(uint32_t unit) const
{
	if (unit >= m_layout.unitCount())
	{
		return nullopt;
	}
	// A progression is one box, and what can be started is its first step that is not done
	for (const auto member : m_layout.units()[unit].members)
	{
		const auto node = m_graph.nodeForResearchIndex(member);
		if (node && m_graph.nodes()[*node].state != NodeState::Researched)
		{
			return member;
		}
	}
	return nullopt;
}

void WzResearchTreeCanvas::requestResearch(uint16_t researchIndex, const std::shared_ptr<WIDGET>& from)
{
	if (researchIndex >= asResearch.size() || !canStartResearchNow(m_context.player, researchIndex))
	{
		return;
	}

	std::vector<ResearchLabOption> options = researchLabOptionsFor(m_context.player, asResearch[researchIndex]);
	// A lab already on this very topic has nothing to offer
	options.erase(std::remove_if(options.begin(), options.end(),
	                             [](const ResearchLabOption& option) { return option.researchingThis; }),
	              options.end());

	if (options.empty())
	{
		addConsoleMessage(_("No research facility is ready"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		return;
	}
	// An idle lab costs nothing to use, and the list already has the fastest of those first
	if (options.front().idle)
	{
		if (STRUCTURE *facility = IdToStruct(options.front().facilityId, m_context.player))
		{
			startResearchAt(facility, asResearch[researchIndex], m_context.player);
			refreshState();
		}
		return;
	}
	// Everything is busy - it is the player's call which piece of work gets moved
	openLabPicker(researchIndex, options, from);
}

class WzLabRow : public W_BUTTON
{
public:
	bool doubledUp = false;

protected:
	void display(int xOffset, int yOffset) override
	{
		if (doubledUp)
		{
			const int x0 = x() + xOffset;
			const int y0 = y() + yOffset;
			pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), WASTED_ROW_COLOR);
		}
		W_BUTTON::display(xOffset, yOffset);
	}
};

static std::shared_ptr<W_BUTTON> makeLabRow(const ResearchLabOption& option, int width, bool doubledUp)
{
	auto row = std::make_shared<WzLabRow>();
	row->doubledUp = doubledUp;
	WzString text = WzString::format(_("Lab %u"), option.facilityId);
	for (uint32_t module = 0; module < option.modules; ++module)
	{
		text += WzString::fromUtf8(" +");
	}
	text += "  ";
	text += WzString::format(_("%d/sec"), option.pointsPerSecond);
	bool progressKept = false;
	if (option.idle)
	{
		text += "  ";
		text += WzString::fromUtf8(_("idle"));
	}
	else if (option.onHold)
	{
		text += "  ";
		text += WzString::fromUtf8(_("on hold:"));
		text += " ";
		text += WzString::format("%s %d%%", option.currentSubject.toUtf8().c_str(), option.currentPercent);
		progressKept = true;
	}
	else if (option.waitingForPower)
	{
		text += "  ";
		text += WzString::fromUtf8(_("no power:"));
		text += " ";
		text += WzString::format("%s %d%%", option.currentSubject.toUtf8().c_str(), option.currentPercent);
		progressKept = true;
	}
	else if (doubledUp)
	{
		text += "  ";
		text += WzString::format("%s %d%%", option.currentSubject.toUtf8().c_str(), option.currentPercent);
		text += ", ";
		text += WzString::fromUtf8(_("doubled up"));
		progressKept = true;
	}
	else
	{
		text += "  ";
		text += WzString::format("%s %d%%", option.currentSubject.toUtf8().c_str(), option.currentPercent);
		progressKept = true;
	}

	if (progressKept)
	{
		text += ", ";
		text += WzString::fromUtf8(_("progress kept"));
	}

	row->setString(text);
	row->FontID = font_small;
	row->setGeometry(0, 0, width, LAB_ROW_HEIGHT);
	row->displayFunction = displayTextOption;
	row->pUserData = new DisplayTextOptionCache();
	row->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	return row;
}

void WzResearchTreeCanvas::openLabPicker(uint16_t researchIndex, const std::vector<ResearchLabOption>& options, const std::shared_ptr<WIDGET>& from)
{
	if (from == nullptr)
	{
		return;
	}

	auto menu = PopoverMenuWidget::make();
	const uint32_t player = m_context.player;
	for (const auto& option : options)
	{
		const bool doubledUp = !option.idle
			&& std::find(m_duplicated.begin(), m_duplicated.end(), option.subjectIndex) != m_duplicated.end();
		auto row = makeLabRow(option, LAB_ROW_WIDTH, doubledUp);
		// A lab can be destroyed or finish what it was doing while this is open, so we can't
		// trust the state from when the row was built. Held by id, since a destroyed lab is
		// freed and the pointer this was built from would not survive to be tested.
		const uint32_t facilityId = option.facilityId;
		std::weak_ptr<WzResearchTreeCanvas> weakSelf = std::dynamic_pointer_cast<WzResearchTreeCanvas>(shared_from_this());
		row->addOnClickHandler([facilityId, researchIndex, player, weakSelf](W_BUTTON&) {
			widgScheduleTask([facilityId, researchIndex, player, weakSelf]() {
				STRUCTURE *facility = IdToStruct(facilityId, player);
				if (facility == nullptr || facility->died != 0 || facility->status != SS_BUILT
				    || researchIndex >= asResearch.size())
				{
					addConsoleMessage(_("That research facility is no longer available"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
					return;
				}
				startResearchAt(facility, asResearch[researchIndex], player);
				if (auto self = weakSelf.lock())
				{
					self->refreshState();
				}
			});
		});
		menu->addMenuItem(row, true);
	}

	const int32_t menuHeight = std::min(menu->idealHeight(), static_cast<int32_t>(screenHeight));
	menu->setGeometry(menu->x(), menu->y(), menu->idealWidth(), menuHeight);

	m_labPicker = menu->openMenu(from, PopoverWidget::Alignment::RightOfParent, Vector2i(0, 4));
}

void WzResearchTreeCanvas::closePopovers()
{
	closeDetail();
	if (m_labPicker)
	{
		m_labPicker->close();
		m_labPicker.reset();
	}
}

void WzResearchTreeCanvas::clearSelection()
{
	m_selection.reset();
	if (m_detailPinned)
	{
		closeDetail();
	}
}

void WzResearchTreeCanvas::requestFocusToggle()
{
	if (m_onFocusRequested)
	{
		m_onFocusRequested();
	}
	else
	{
		toggleFocusOnSelection();
	}
}

bool WzResearchTreeCanvas::toggleFocusOnSelection()
{
	if (m_focusTarget)
	{
		m_focusTarget.reset();
		m_focus = ResearchFocus();
		rebuildLayout();
		return false;
	}
	if (!m_selection)
	{
		return false;
	}

	// The picked step (not the whole progression)
	const uint16_t target = m_layout.units()[m_selection->unit].members[m_selection->member];
	m_focus = computeResearchFocus(m_graph, {target});
	m_focusTarget = target;
	rebuildLayout();
	revealResearch(target);
	return true;
}

// How far along a progression is, as well as some progression names, depend on current research state
void WzResearchTreeCanvas::rebuildLabelText()
{
	m_unitLabelText.assign(m_layout.unitCount(), std::string());
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		const auto& members = m_layout.units()[unit].members;
		// A progression is named after what it does - skip naming the progression if no steps are known
		if (std::none_of(members.begin(), members.end(), [this](uint16_t member) { return topicKnown(member); }))
		{
			continue;
		}
		std::string label = m_layout.units()[unit].name.toUtf8();
		if (label.empty())
		{
			// A progression with no name takes the name of the step the player has reached
			// - this reads better than always naming its first step
			uint16_t showing = members.front();
			for (const auto member : members)
			{
				const auto node = m_graph.nodeForResearchIndex(member);
				if (node && m_graph.nodes()[*node].state == NodeState::Researched)
				{
					showing = member;
				}
			}
			label = getLocalizedStatsName(&asResearch[showing]);
		}
		if (members.size() > 1)
		{
			size_t done = 0;
			for (const auto member : members)
			{
				const auto node = m_graph.nodeForResearchIndex(member);
				done += (node && m_graph.nodes()[*node].state == NodeState::Researched) ? 1 : 0;
			}
			label += " " + std::to_string(done) + "/" + std::to_string(members.size());
		}
		m_unitLabelText[unit] = label;
	}
}

void WzResearchTreeCanvas::setLabeled(bool labeled)
{
	const float target = labeled ? 1.f : 0.f;
	if (target == m_labeledTarget)
	{
		return;
	}
	m_labeledTarget = target;
	m_labeledFrom = m_labeled;
	m_pitchStart = realTime;
	m_densityAsked = true;
	closeDetail();

	// A density is only worth being in at a zoom that suits it, so the switch takes the view there
	if (labeled)
	{
		// Nothing is written below ZOOM_LABELS, so turning names on from further
		// out than that reduces the density but puts nothing in the space
		m_zoom = std::max(m_zoom, ZOOM_LABELS);
		m_viewMoved = true;
	}
	else
	{
		// The compact tree is for seeing the whole shape of it, so returning to compact
		// view goes back to where it's all visible.
		m_refitOnSettle = true;
	}
}

LayoutMetrics WzResearchTreeCanvas::presentationMetrics() const
{
	const auto pitch = [this](int32_t compact, int32_t labeled) {
		return static_cast<int32_t>(std::lround(static_cast<float>(compact) + static_cast<float>(labeled - compact) * m_labeled));
	};
	LayoutMetrics metrics;
	metrics.unitWidth = pitch(COMPACT_METRICS.unitWidth, LABELED_METRICS.unitWidth);
	metrics.unitHeight = pitch(COMPACT_METRICS.unitHeight, LABELED_METRICS.unitHeight);
	metrics.tierSpacing = pitch(COMPACT_METRICS.tierSpacing, LABELED_METRICS.tierSpacing);
	metrics.rowPadding = pitch(COMPACT_METRICS.rowPadding, LABELED_METRICS.rowPadding);
	metrics.leftGutter = pitch(COMPACT_METRICS.leftGutter, LABELED_METRICS.leftGutter);
	return metrics;
}

void WzResearchTreeCanvas::applyPresentation()
{
	const LayoutMetrics metrics = presentationMetrics();
	if (metrics.unitWidth == m_layout.metrics().unitWidth && metrics.unitHeight == m_layout.metrics().unitHeight
	    && metrics.tierSpacing == m_layout.metrics().tierSpacing && metrics.rowPadding == m_layout.metrics().rowPadding)
	{
		return;
	}

	// Everything moves toward the origin as the tree compresses, so hold the middle of the
	// view rather than letting the tree slide out from underneath
	const Vector2i before = m_layout.canvasSize();
	const Vector2f middle(m_pan.x + static_cast<float>(width()) / (2.f * m_zoom),
	                      m_pan.y + static_cast<float>(height()) / (2.f * m_zoom));
	const Vector2f held(before.x > 0 ? middle.x / static_cast<float>(before.x) : 0.5f,
	                    before.y > 0 ? middle.y / static_cast<float>(before.y) : 0.5f);

	m_layout.setMetrics(metrics);
	m_zoom = std::max(m_zoom, minimumZoom());

	const Vector2i after = m_layout.canvasSize();
	m_pan = Vector2f(held.x * static_cast<float>(after.x) - static_cast<float>(width()) / (2.f * m_zoom),
	                 held.y * static_cast<float>(after.y) - static_cast<float>(height()) / (2.f * m_zoom));
	clampPan();
	followDetail();
}

// Clear space between the things sitting along a unit (sized off the row)
float WzResearchTreeCanvas::unitPadPx() const
{
	const float rowPx = static_cast<float>(m_layout.metrics().unitHeight) * m_zoom;
	const float floorRowPx = static_cast<float>(COMPACT_METRICS.unitHeight) * ZOOM_MIN;
	return std::max(UNIT_PAD_MIN_PX, UNIT_PAD_MIN_PX + (rowPx - floorRowPx) * UNIT_PAD_PER_ROW_PX);
}

float WzResearchTreeCanvas::iconSizePx(int boxHeight) const
{
	// Never taller than its container - a compact node has no room for an icon at all (determined by ICON_MIN_PX)
	const float wanted = 0.6f * static_cast<float>(m_layout.metrics().unitHeight) * m_zoom;
	return std::min(wanted, std::max(0.f, static_cast<float>(boxHeight) - 2.f));
}


float WzResearchTreeCanvas::rowOffsetPx(uint32_t unit, int32_t rowWithinUnit) const
{
	// Calculated off the box's own *drawn* height rather than unitRowOffset() (which divides in
	// whole logical pixels and may lose up to half of a pixel before the zoom is applied)
	const auto& placed = m_layout.units()[unit];
	const float height = static_cast<float>(m_layout.unitSize(unit).y) * m_zoom;
	return height * static_cast<float>(2 * rowWithinUnit + 1)
		/ (2.f * static_cast<float>(std::max(1, placed.rowSpan)));
}

bool WzResearchTreeCanvas::labelsVisible() const
{
	return m_labeled >= 1.f && m_zoom >= ZOOM_LABELS;
}

// Where a step's pip lands on screen, taken from the pip's own place on the canvas
float WzResearchTreeCanvas::pipScreenX(uint32_t unit, size_t member, const WzRect& box, int originX) const
{
	const auto& placed = m_layout.units()[unit];
	if (placed.members.size() > 1 || !labelsVisible())
	{
		const auto& metrics = m_layout.metrics();
		const float across = static_cast<float>(metrics.leftGutter + placed.memberTiers[member] * metrics.tierSpacing)
			+ static_cast<float>(metrics.unitWidth) / 2.f;
		return logicalToScreenF(Vector2f(across, 0.f), originX, 0).x;
	}
	return static_cast<float>(box.x()) + bulletOffsetPx(box.height());
}

// Where a lone topic's single pip goes while its name is on show: just after the icon
float WzResearchTreeCanvas::bulletOffsetPx(int boxHeight) const
{
	const float icon = iconSizePx(boxHeight);
	const float pad = unitPadPx();
	return pad + ((icon >= ICON_MIN_PX) ? icon + pad : 0.f) + pipRadiusPx();
}

float WzResearchTreeCanvas::pipRadiusPx() const
{
	// A line, in the height of the row on the screen, so that a pip is the same size no matter
	// which pitch and zoom put it there.
	const float rowPx = static_cast<float>(m_layout.metrics().unitHeight) * m_zoom;
	const float floorRowPx = static_cast<float>(COMPACT_METRICS.unitHeight) * ZOOM_MIN;
	return std::max(PIP_MIN_PX, PIP_MIN_PX + (rowPx - floorRowPx) * PIP_PER_ROW_PX);
}

// Where along a unit the step at a tier is drawn, as an offset from its left edge
float WzResearchTreeCanvas::pipOffsetPx(uint32_t unit, int32_t tier) const
{
	const auto& metrics = m_layout.metrics();
	const int32_t from = m_layout.units()[unit].firstTier;
	return static_cast<float>((tier - from) * metrics.tierSpacing + metrics.unitWidth / 2) * m_zoom;
}

// The step after the first one (which is as far right as a label may extend)
optional<int32_t> WzResearchTreeCanvas::secondTierOf(uint32_t unit) const
{
	const auto& placed = m_layout.units()[unit];
	optional<int32_t> second;
	for (const auto tier : placed.memberTiers)
	{
		if (tier > placed.firstTier && (!second || tier < *second))
		{
			second = tier;
		}
	}
	return second;
}

float WzResearchTreeCanvas::labelOffsetPx(uint32_t unit) const
{
	const int boxHeight = static_cast<int>(static_cast<float>(m_layout.metrics().unitHeight) * m_zoom);
	if (m_layout.units()[unit].members.size() <= 1)
	{
		return bulletOffsetPx(boxHeight) + stepReachPx(unit, 0) + unitPadPx();
	}
	// A progression carries a pip at every step, and the first sits about where the label would (otherwise) start, so the text goes after it
	const float icon = iconSizePx(boxHeight);
	const float pad = unitPadPx();
	const float base = pad + ((icon >= ICON_MIN_PX) ? icon + pad : 0.f);
	return std::max(base, pipOffsetPx(unit, m_layout.units()[unit].firstTier) + stepReachPx(unit, 0) + pad);
}

int WzResearchTreeCanvas::labelRoomPx(uint32_t unit) const
{
	// A progression is named to its left, in the clear space every tier leaves - so the inside belongs to its steps.
	// A lone topic has no steps to make room for and keeps its name inside.
	if (m_layout.units()[unit].members.size() <= 1)
	{
		const float boxWidth = static_cast<float>(m_layout.unitSize(unit).x) * m_zoom;
		return std::max(0, static_cast<int>(boxWidth - unitPadPx() - labelOffsetPx(unit)));
	}
	// Whatever is actually clear to the left is as much space as a name can have in the layout
	// without overlapping whatever else shares the row
	const float clear = static_cast<float>(m_layout.clearLeftOf(unit)) * m_zoom
		- GROUP_NAME_GAP_PX - GROUP_NAME_CLEARANCE_PX;
	return std::max(0, static_cast<int>(clear));
}

// How far a step is drawn out to, which is past its pip where the state also rings it.
// (What is written beside a step starts from this point.)
float WzResearchTreeCanvas::stepReachPx(uint32_t unit, size_t member) const
{
	const auto& members = m_layout.units()[unit].members;
	if (member >= members.size())
	{
		return pipRadiusPx();
	}
	const auto node = m_graph.nodeForResearchIndex(members[member]);
	const NodeState state = node ? m_graph.nodes()[*node].state : NodeState::Locked;
	return drawnPipReachPx(pipRadiusPx(),
	                       state == NodeState::Available || state == NodeState::InProgress);
}

// How much room a step's name has: either to the next pip, or to the end of the unit
int WzResearchTreeCanvas::stepNameRoomPx(uint32_t unit, size_t member) const
{
	const auto& placed = m_layout.units()[unit];
	const int32_t tier = placed.memberTiers[member];
	// The next step along, so that a name stops clear of whatever is drawn around that one as well
	int32_t next = INT32_MAX;
	size_t nextMember = member;
	for (size_t other = 0; other < placed.memberTiers.size(); ++other)
	{
		if (placed.memberTiers[other] > tier && placed.memberTiers[other] < next)
		{
			next = placed.memberTiers[other];
			nextMember = other;
		}
	}
	const float pad = unitPadPx();
	const float from = pipOffsetPx(unit, tier) + stepReachPx(unit, member) + pad;
	const float to = (next == INT32_MAX)
		? static_cast<float>(m_layout.unitSize(unit).x) * m_zoom - pad
		: pipOffsetPx(unit, next) - stepReachPx(unit, nextMember) - pad;
	return std::max(0, static_cast<int>(to - from));
}

// Room taken beside a step by the figure saying how much of the team holds it (including the gap after it), if relevant & displayed
int WzResearchTreeCanvas::heldLabelRoomPx(uint32_t unit, size_t member)
{
	const auto& members = m_layout.units()[unit].members;
	if (m_heldLabels.empty() || member >= members.size() || members[member] >= m_heldCount.size())
	{
		return 0;
	}
	const uint32_t held = m_heldCount[members[member]];
	if (held == 0 || held >= m_context.aggregate.size())
	{
		return 0;
	}
	const float rowPx = static_cast<float>(m_layout.metrics().unitHeight) * m_zoom;
	if (rowPx < static_cast<float>(iV_GetTextLineSize(font_small)))
	{
		return 0;
	}
	const int wanted = m_heldLabels[held].getTextWidth() + HELD_LABEL_GAP_PX * 2;
	return (wanted <= stepNameRoomPx(unit, member)) ? wanted : 0;
}

// The text of every label - stable until the layout or the player's progress changes.
// (How much of it fits is decided where it is drawn, so nothing here depends on zoom.)
void WzResearchTreeCanvas::rebuildLabels()
{
	for (uint32_t unit = 0; unit < m_unitLabels.size(); ++unit)
	{
		m_unitLabels[unit].setText(WzString::fromUtf8(m_unitLabelText[unit]), font_small);
	}
	for (uint32_t unit = 0; unit < m_stepLabels.size(); ++unit)
	{
		const auto& placed = m_layout.units()[unit];
		// A lone topic has no progression name of its own - so its unit label is
		// already that topic's name, in the place a step name would go
		if (placed.members.size() < 2)
		{
			continue;
		}
		for (size_t member = 0; member < m_stepLabels[unit].size(); ++member)
		{
			// A step the player has not found is only drawn as its pip
			const WzString name = topicKnown(placed.members[member])
				? WzString::fromUtf8(getLocalizedStatsName(&asResearch[placed.members[member]])) : WzString();
			m_stepLabels[unit][member].setText(name, font_small);
		}
	}
}

// Release cached textures for labels that have been off screen for a bit
void WzResearchTreeCanvas::tickLabels()
{
	for (auto& label : m_unitLabels)
	{
		label.tick();
	}
	for (auto& unit : m_stepLabels)
	{
		for (auto& label : unit)
		{
			label.tick();
		}
	}
	for (auto& label : m_heldLabels)
	{
		label.tick();
	}
}

// Unit name placement (and room)
// A progression is named in the clear space to its left, but that might be off the visible canvas (in which case the name is displayed inside)
UnitLabelPlacement WzResearchTreeCanvas::unitLabelPlacement(uint32_t unit, const WzRect& box, const WzRect& view, bool namedOutside) const
{
	UnitLabelPlacement placement;
	if (unit >= m_unitLabels.size())
	{
		return placement;
	}
	const int natural = const_cast<WzCachedText&>(m_unitLabels[unit]).getTextWidth();
	if (!namedOutside)
	{
		placement.room = labelRoomPx(unit);
		placement.x = box.x() + static_cast<int>(labelOffsetPx(unit));
		placement.draw = placement.room > 0;
		return placement;
	}

	// The gutter, minus however much of it is off the left of the view.
	// A name is laid back from the strip it names, so a long name runs off the edge first and is cut by the viewport.
	const int gap = static_cast<int>(GROUP_NAME_GAP_PX);
	const int gutter = std::min(labelRoomPx(unit), box.x() - gap - view.x());
	if (gutter >= static_cast<int>(GROUP_NAME_LEAST_PX))
	{
		placement.room = gutter;
		placement.x = box.x() - gap - std::min(natural, gutter);
		placement.draw = true;
		return placement;
	}

	// A strip off the left has no gutter on screen to put a name in, so its name is positioned
	// inside on an overlay drawn after everything else (letting the steps pass under it).
	//
	// NOTE: A strip fully on screen whose neighbor leaves it no gutter gives up its name instead,
	// since an overlay there would always sit over its first step.
	if (box.x() >= view.x())
	{
		return placement;
	}
	placement.pinned = true;
	placement.x = view.x() + gap;
	placement.room = std::min(natural, box.right() - gap - placement.x);
	placement.draw = placement.room >= static_cast<int>(GROUP_NAME_LEAST_PX);
	return placement;
}

// The corner a panel starts from - positioned down and to the right of it. Beside the step it describes rather than the whole strip.
// A traced path is wide and shallow and a panel near the middle hides it, so in that case the panel moves to the corner.
WzRect WzResearchTreeCanvas::detailAnchorRect(TreePick pick) const
{
	if (m_focusTarget)
	{
		const int panelWidth = m_detailContents ? m_detailContents->width() : DETAIL_WIDTH;
		return WzRect(std::max(0, width() - DETAIL_CORNER_INSET - panelWidth), DETAIL_CORNER_INSET, 0, 0);
	}
	const WzRect step = stepOnScreen(pick, 0, 0);
	const WzRect unit = unitOnScreen(pick.unit, 0, 0);
	const int panelHeight = detailPanelHeight();
	int hangsFrom = unit.bottom();
	const int above = unit.y() - DETAIL_PANEL_DROP - DETAIL_NODE_GAP - panelHeight;
	if (panelHeight > 0 && hangsFrom + DETAIL_PANEL_DROP + panelHeight > height() && above >= 0)
	{
		hangsFrom = above;
	}
	return WzRect(step.right() + DETAIL_NODE_GAP, hangsFrom, 0, 0);
}

// How tall the current detail panel is, which decides whether it can hang below its unit or has to sit above it
int WzResearchTreeCanvas::detailPanelHeight() const
{
	if (m_detailPopover)
	{
		return m_detailPopover->height();
	}
	return m_detailContents ? m_detailContents->height() : 0;
}

WzRect WzResearchTreeCanvas::stepOnScreen(TreePick pick, int originX, int originY) const
{
	const WzRect box = unitOnScreen(pick.unit, originX, originY);
	const auto& placed = m_layout.units()[pick.unit];
	if (pick.member >= placed.members.size())
	{
		return box;
	}

	const int radius = static_cast<int>(pipRadiusPx());
	const int height = static_cast<int>(static_cast<float>(m_layout.metrics().unitHeight) * m_zoom);
	const float alongX = pipScreenX(pick.unit, pick.member, box, originX);
	return WzRect(static_cast<int>(alongX) - radius,
	              box.y() + static_cast<int>(rowOffsetPx(pick.unit, placed.memberRows[pick.member])) - height / 2,
	              radius * 2, height);
}

WzRect WzResearchTreeCanvas::unitOnScreen(uint32_t unit, int originX, int originY) const
{
	// The rect ends where the drawn box ends.
	// (Logical size times zoom rounds separately from the corner and can leave the two a pixel apart.)
	const Vector2f topLeft = unitOriginLogical(unit);
	const Vector2i size = m_layout.unitSize(unit);
	const Vector2i at = logicalToScreen(Vector2i(static_cast<int>(std::lround(topLeft.x)), static_cast<int>(std::lround(topLeft.y))), originX, originY);
	const Vector2i to = logicalToScreen(Vector2i(static_cast<int>(std::lround(topLeft.x)) + size.x, static_cast<int>(std::lround(topLeft.y)) + size.y), originX, originY);
	return WzRect(at.x, at.y, to.x - at.x, to.y - at.y);
}

void WzResearchTreeCanvas::setPerspective(uint32_t player, std::vector<uint32_t> aggregate, bool allowAssignment)
{
	if (m_context.player == player && m_context.aggregate == aggregate
	    && m_context.allowAssignment == allowAssignment)
	{
		return;
	}
	m_context.player = player;
	m_context.aggregate = std::move(aggregate);
	m_context.allowAssignment = allowAssignment;
	refreshState();
}

void WzResearchTreeCanvas::refreshHeldCounts()
{
	if (m_context.aggregate.empty())
	{
		m_heldCount.clear();
		m_heldLabels.clear();
		return;
	}
	m_heldCount.assign(asResearch.size(), 0);
	for (size_t index = 0; index < m_heldCount.size(); ++index)
	{
		m_heldCount[index] = static_cast<uint8_t>(researchHeldCount(m_context, static_cast<uint16_t>(index)));
	}
	const size_t members = m_context.aggregate.size();
	if (m_heldLabels.size() != members + 1)
	{
		m_heldLabels.clear();
		m_heldLabels.resize(members + 1);
		for (size_t held = 1; held < members; ++held)
		{
			m_heldLabels[held].setText(WzString::fromUtf8(astringf("%zu/%zu", held, members)), font_small);
		}
	}
}

void WzResearchTreeCanvas::refreshState()
{
	m_graph.refreshState(m_context);
	refreshHeldCounts();
	rebuildLabelText();
	rebuildLabels();
	if (m_detailContents)
	{
		// Whatever the popover is describing may have just been restated
		closeDetail();
	}
}

// The game carries on behind the view (in most modes) - a lab may finish, power may run out, etc - and none of it reports itself here,
// so the state is re-read on a timer.
//
// Only a state that has actually moved is worth acting on, since re-reading walks the nodes while what follows rebuilds every label.
void WzResearchTreeCanvas::tickState()
{
	if (realTime - m_lastStateRefresh < STATE_REFRESH_MS)
	{
		return;
	}
	m_lastStateRefresh = realTime;

	std::vector<NodeState> before;
	before.reserve(m_graph.nodes().size());
	for (const auto& node : m_graph.nodes())
	{
		before.push_back(node.state);
	}
	m_graph.refreshState(m_context);

	// What the popover is describing, since the panel is only stale if its subject moved.
	// (Everything else is drawn from the state each frame and follows on its own.)
	optional<uint16_t> subject;
	if (m_detailPick && m_detailPick->unit < m_layout.unitCount()
	    && m_detailPick->member < m_layout.units()[m_detailPick->unit].members.size())
	{
		subject = m_layout.units()[m_detailPick->unit].members[m_detailPick->member];
	}

	bool moved = false;
	bool subjectMoved = false;
	for (size_t node = 0; node < before.size(); ++node)
	{
		if (before[node] == m_graph.nodes()[node].state)
		{
			continue;
		}
		moved = true;
		subjectMoved = subjectMoved || (subject && m_graph.nodes()[node].primaryResearchIndex == *subject);
	}

	m_duplicated = duplicatedResearch(m_context);

	// Reading a whole team, the second member to finish a topic moves nothing the graph
	// knows about (since the team held it since the first). It does move what the panel
	// says about who holds it - so it counts here.
	std::vector<uint8_t> heldBefore;
	heldBefore.swap(m_heldCount);
	refreshHeldCounts();
	if (subject && *subject < m_heldCount.size() && *subject < heldBefore.size()
	    && m_heldCount[*subject] != heldBefore[*subject])
	{
		subjectMoved = true;
	}

	// Campaign hands topics out as the player discovers / earns them, and a topic arriving is not a
	// topic changing state: it has to be given a place, which may mean placing the tree again.
	std::vector<bool> discovered = researchVisibleTo(m_context);
	if (discovered != m_visible)
	{
		reflowForDiscovery(std::move(discovered));
		return;
	}

	// A campaign already showing the shape of what the player has not found places nothing new when they find it.
	// It can simply be named now, which is a label rather than a layout.
	std::vector<bool> known = researchKnownTo(m_context);
	if (known != m_known)
	{
		m_known = std::move(known);
		rebuildLabelText();
		rebuildLabels();
		if (m_detailPick)
		{
			const TreePick pick = *m_detailPick;
			const bool pinned = m_detailPinned;
			openDetail(pick, pinned);
		}
		return;
	}

	if (!moved && !subjectMoved)
	{
		return;
	}

	// A progression's name and how far along it is both depend on what has been researched
	if (moved)
	{
		rebuildLabelText();
		rebuildLabels();
	}

	// Restated rather than closed, so a panel being read does not vanish.
	// What it says about cost and what its buttons offer are calculated when it is built, so we must build it again.
	if (subjectMoved)
	{
		const TreePick pick = *m_detailPick;
		const bool pinned = m_detailPinned;
		openDetail(pick, pinned);
	}
}

// A topic the player has just been given joins a progression, which can push every
// tier after it along and hand out rows again - so the answer is the whole layout
// rather than a patch of it.
//
// Two things must survive:
// - where the player was looking
// - where everything they can see was a moment ago
void WzResearchTreeCanvas::reflowForDiscovery(std::vector<bool> discovered)
{
	// Where each topic is being drawn, not where it was packed, so that a second
	// topic arriving mid-movement carries on from where the tree has gotten to
	std::unordered_map<uint16_t, Vector2f> wasAt;
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		const Vector2f topLeft = unitOriginLogical(unit);
		for (const auto member : m_layout.units()[unit].members)
		{
			wasAt[member] = topLeft;
		}
	}

	// The player's place, by topic.
	// A rebuild renumbers every unit, so nothing holding an index means anything afterwards.
	optional<uint16_t> selected;
	optional<uint16_t> pinned;
	if (m_selection)
	{
		selected = m_layout.units()[m_selection->unit].members[m_selection->member];
	}
	if (m_detailPick && m_detailPinned)
	{
		pinned = m_layout.units()[m_detailPick->unit].members[m_detailPick->member];
	}
	const Vector2f pan = m_pan;
	const float zoom = m_zoom;
	const bool viewMoved = m_viewMoved;

	m_visible = std::move(discovered);
	m_lanesShown = researchLanesInUse(m_visible);
	// Anything newly placed is newly found, and the labels are built by the rebuild
	m_known = researchKnownTo(m_context);
	rebuildLayout();

	// A rebuild fits the tree to the view, which is right when the view opens and wrong here,
	// since the player has not asked to move
	m_pan = pan;
	m_zoom = zoom;
	m_viewMoved = viewMoved;

	m_reflowOffset.assign(m_layout.unitCount(), Vector2f(0.f, 0.f));
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		for (const auto member : m_layout.units()[unit].members)
		{
			const auto at = wasAt.find(member);
			if (at == wasAt.end())
			{
				continue;
			}
			// Any step already on show says where its progression came from.
			// One made completely of new topics has nowhere to travel from - and is drawn in-place.
			const Vector2i topLeft = m_layout.unitTopLeft(unit);
			m_reflowOffset[unit] = at->second - Vector2f(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y));
			break;
		}
	}
	m_reflowStart = realTime;
	clampPan();

	if (selected)
	{
		m_selection = pickForResearch(*selected);
	}
	if (pinned)
	{
		if (const auto pick = pickForResearch(*pinned))
		{
			openDetail(*pick, true);
		}
	}
}

float WzResearchTreeCanvas::reflowPart() const
{
	// Nothing to do once the time is up (which is also the state the tree spends almost all of its life in)
	if (m_reflowStart == 0 || realTime - m_reflowStart >= REFLOW_TIME)
	{
		return 0.f;
	}
	const float part = static_cast<float>(realTime - m_reflowStart) / static_cast<float>(REFLOW_TIME);
	// Eased out, so the tree leaves quickly and settles
	return (1.f - part) * (1.f - part);
}

Vector2f WzResearchTreeCanvas::unitOriginLogical(uint32_t unit) const
{
	const Vector2i topLeft = m_layout.unitTopLeft(unit);
	const Vector2f at(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y));
	const float left = reflowPart();
	if (left <= 0.f || unit >= m_reflowOffset.size())
	{
		return at;
	}
	return at + m_reflowOffset[unit] * left;
}

optional<TreePick> WzResearchTreeCanvas::pickForResearch(uint16_t researchIndex) const
{
	const uint32_t unit = unitOfResearch(researchIndex);
	if (unit >= m_layout.unitCount())
	{
		return nullopt;
	}
	const auto& members = m_layout.units()[unit].members;
	const auto at = std::find(members.begin(), members.end(), researchIndex);
	if (at == members.end())
	{
		return nullopt;
	}
	return TreePick{unit, static_cast<size_t>(at - members.begin())};
}

void WzResearchTreeCanvas::geometryChanged()
{
	// Keep fitting until the player moves the view themselves, so that we handle initial widget creation
	// as well as sizing / re-sizing.
	if (!m_viewMoved)
	{
		fitToView();
	}
	// How far out the view may go == how far out the whole tree still fits, which moves with the window.
	// A window that grows leaves the zoom under its floor, drawing the tree smaller than it need be, so
	// the zoom increases with it. A window that shrank lowers the floor and takes nothing away.
	m_zoom = std::max(m_zoom, minimumZoom());
	clampPan();
	rebuildLabels();
	followDetail();
}

// Zooming out past what's needed for the whole tree to fit just makes everything harder to read, so that is where it stops.
// Where the tree cannot fit at a useful size at all (the "labeled" density), the absolute floor takes over.
float WzResearchTreeCanvas::minimumZoom() const
{
	const Vector2i canvas = m_layout.canvasSize();
	if (canvas.x <= 0 || canvas.y <= 0 || width() <= 0 || height() <= 0)
	{
		return ZOOM_MIN;
	}
	const float fitX = static_cast<float>(width()) / static_cast<float>(canvas.x + CANVAS_MARGIN * 2);
	const float fitY = static_cast<float>(height()) / static_cast<float>(canvas.y + CANVAS_MARGIN * 2);
	return std::clamp(std::min(fitX, fitY), ZOOM_MIN, ZOOM_MAX);
}

// What the tree would measure with names on, asked of the layout (since row heights depend on which rows hold a unit).
bool WzResearchTreeCanvas::labeledPitchFits()
{
	const LayoutMetrics was = m_layout.metrics();
	m_layout.setMetrics(LABELED_METRICS);
	const Vector2i size = m_layout.canvasSize();
	m_layout.setMetrics(was);
	if (size.x <= 0 || size.y <= 0)
	{
		return false;
	}
	const float fitX = static_cast<float>(width()) / static_cast<float>(size.x + CANVAS_MARGIN * 2);
	const float fitY = static_cast<float>(height()) / static_cast<float>(size.y + CANVAS_MARGIN * 2);
	return std::min(fitX, fitY) >= ZOOM_LABELS;
}

void WzResearchTreeCanvas::fitToView()
{
	if (width() <= 0 || height() <= 0)
	{
		return;
	}

	// The compact pitch is for a tree that will not fit any other way.
	// A tree that fits legibly with its names on opens that way instead.
	// (Example: A campaign starting with four topics has no use for a view built for hundreds.)
	//
	// Decided again on every resize, and skipped once the player says which density they want, since toggling
	// back out to fit a small tree lands here and would otherwise immediately turn the names back on.
	const bool wantsLabeled = !m_densityAsked && labeledPitchFits();
	if (!m_densityAsked && wantsLabeled != (m_labeledTarget > 0.5f))
	{
		m_labeled = wantsLabeled ? 1.f : 0.f;
		m_labeledTarget = m_labeled;
		m_labeledFrom = m_labeled;
		m_layout.setMetrics(wantsLabeled ? LABELED_METRICS : COMPACT_METRICS);
		rebuildLabels();
	}

	const Vector2i canvas = m_layout.canvasSize();
	if (canvas.x <= 0 || canvas.y <= 0)
	{
		return;
	}
	// Compact fits, so fit it.
	// Labeled cannot, often being several screens wide, so there the floor is
	// whatever the labels survive (rather than the whole tree).
	const float fitX = static_cast<float>(width()) / static_cast<float>(canvas.x + CANVAS_MARGIN * 2);
	const float fitY = static_cast<float>(height()) / static_cast<float>(canvas.y + CANVAS_MARGIN * 2);
	const float lowest = (m_labeled >= 1.f) ? ZOOM_LABELS : ZOOM_MIN;
	m_zoom = std::clamp(std::min(fitX, fitY), lowest, ZOOM_MAX);
	m_pan = Vector2f(static_cast<float>(canvas.x) / 2.f - static_cast<float>(width()) / (2.f * m_zoom),
	                 static_cast<float>(canvas.y) / 2.f - static_cast<float>(height()) / (2.f * m_zoom));
	clampPan();
}

// The band across the top of the canvas a parked panel is sitting in.
// Only a pinned panel counts, since moving the tree out from under a panel that the pointer opened ends pip hover.
int WzResearchTreeCanvas::reservedTopPx() const
{
	if (!m_focusTarget || !m_detailPopover || !m_detailPinned)
	{
		return 0;
	}
	return DETAIL_CORNER_INSET * 2 + m_detailPopover->height();
}

void WzResearchTreeCanvas::clampPan()
{
	const Vector2i canvas = m_layout.canvasSize();
	const float visibleW = static_cast<float>(width()) / m_zoom;
	const float visibleH = static_cast<float>(height()) / m_zoom;
	const float lowX = -CANVAS_MARGIN;
	const float lowY = -CANVAS_MARGIN;
	const float highX = static_cast<float>(canvas.x) + CANVAS_MARGIN - visibleW;
	const float highY = static_cast<float>(canvas.y) + CANVAS_MARGIN - visibleH;
	if (highX < lowX)
	{
		// Narrower than the view, so centered instead of panned.
		// On the units, not the box holding them, which reserves a gutter left of the first tier and nothing to
		// its right, so centering the box leaves every unit half a gutter right of the middle.
		// Never past the box's *own* left edge - so a tree that only _just_ fits still has the gutter that its
		// names need on screen.
		const float gutter = static_cast<float>(m_layout.metrics().leftGutter);
		m_pan.x = std::min(0.f, (gutter + static_cast<float>(canvas.x) - visibleW) / 2.f);
	}
	else
	{
		m_pan.x = std::clamp(m_pan.x, lowX, highX);
	}
	if (highY >= lowY)
	{
		m_pan.y = std::clamp(m_pan.y, lowY, highY);
		return;
	}

	// Shorter than the view, so centered instead of panned.
	// A traced path is often wide and shallow with its panel parked over the top, so move it down to clear the
	// panel as far as room below permits.
	m_pan.y = (lowY + highY) / 2.f;
	const float contentHeight = static_cast<float>(canvas.y) + CANVAS_MARGIN * 2.f;
	const float above = (visibleH - contentHeight) / 2.f;
	const float wanted = static_cast<float>(reservedTopPx()) / m_zoom - above;
	m_pan.y -= std::max(0.f, std::min(wanted, visibleH - contentHeight - above));
}

// Where a logical point lands, unrounded.
// Everything drawn goes through this - so a pip and the box it sits in are placed by one calculation.
Vector2f WzResearchTreeCanvas::logicalToScreenF(Vector2f logical, int originX, int originY) const
{
	return Vector2f(static_cast<float>(originX) + (logical.x - m_pan.x) * m_zoom,
	                static_cast<float>(originY) + (logical.y - m_pan.y) * m_zoom);
}

// The same as above, on the pixel grid, for hit testing and text.
// Rounded rather than truncated - a row three pixels tall cannot afford truncation.
Vector2i WzResearchTreeCanvas::logicalToScreen(Vector2i logical, int originX, int originY) const
{
	const Vector2f at = logicalToScreenF(Vector2f(logical.x, logical.y), originX, originY);
	return Vector2i(static_cast<int>(std::lround(at.x)), static_cast<int>(std::lround(at.y)));
}

Vector2f WzResearchTreeCanvas::screenToLogical(Vector2i screen, int originX, int originY) const
{
	return Vector2f(static_cast<float>(screen.x - originX) / m_zoom + m_pan.x,
	                static_cast<float>(screen.y - originY) / m_zoom + m_pan.y);
}

bool WzResearchTreeCanvas::revealResearch(uint16_t researchIndex)
{
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		const auto& members = m_layout.units()[unit].members;
		const auto at = std::find(members.begin(), members.end(), researchIndex);
		if (at == members.end())
		{
			continue;
		}
		const TreePick pick{unit, static_cast<size_t>(at - members.begin())};
		const Vector2f center = pipCenter(unit, pick.member);
		m_viewMoved = true;
		m_pan = Vector2f(center.x - static_cast<float>(width()) / (2.f * m_zoom),
		                 center.y - static_cast<float>(height()) / (2.f * m_zoom));
		m_selection = pick;
		m_revealedUntil = realTime + REVEAL_MARK_TIME;
		clampPan();
		openDetail(pick, true);
		return true;
	}
	return false;
}

// The middle of a step's pip, in layout coordinates.
// NOTE: While a lone topic draws its pip as a bullet near its left edge, for *picking* it is the one thing the unit holds,
// so where it is drawn doesn't matter here.
Vector2f WzResearchTreeCanvas::pipCenter(uint32_t unit, size_t member) const
{
	const auto& placed = m_layout.units()[unit];
	const auto& metrics = m_layout.metrics();
	const Vector2f topLeft = unitOriginLogical(unit);
	return Vector2f(static_cast<float>(metrics.leftGutter + placed.memberTiers[member] * metrics.tierSpacing + metrics.unitWidth / 2),
	                topLeft.y + static_cast<float>(m_layout.unitRowOffset(unit, placed.memberRows[member])));
}

optional<uint16_t> WzResearchTreeCanvas::firstStartableTopic() const
{
	for (const auto& pick : m_picksInReadingOrder)
	{
		const uint16_t member = m_layout.units()[pick.unit].members[pick.member];
		if (canStartResearchNow(m_context.player, member))
		{
			return member;
		}
	}
	return nullopt;
}

optional<TreePick> WzResearchTreeCanvas::pickAt(Vector2i localPoint) const
{
	const Vector2f logical = screenToLogical(localPoint, 0, 0);
	for (const auto unit : m_visibleUnits)
	{
		const Vector2f topLeft = unitOriginLogical(unit);
		const Vector2i size = m_layout.unitSize(unit);
		if (logical.x < topLeft.x || logical.x > topLeft.x + static_cast<float>(size.x)
		    || logical.y < topLeft.y || logical.y > topLeft.y + static_cast<float>(size.y))
		{
			continue;
		}

		// Whichever step is nearest, so pointing anywhere along a strip asks about the topic under the pointer.
		// Measured in layout coordinates (where a tier is far wider than a row is tall).
		size_t nearest = 0;
		float shortest = -1.f;
		for (size_t member = 0; member < m_layout.units()[unit].members.size(); ++member)
		{
			const Vector2f at = pipCenter(unit, member);
			const float dx = logical.x - at.x;
			const float dy = logical.y - at.y;
			const float distance = dx * dx + dy * dy;
			if (shortest < 0.f || distance < shortest)
			{
				shortest = distance;
				nearest = member;
			}
		}
		return TreePick{unit, nearest};
	}
	return nullopt;
}

void WzResearchTreeCanvas::updateRevealedEdges()
{
	// Showing the whole graph at once is unreadable, so show only what the player is pointing at or has picked.
	m_revealedEdges.clear();
	m_revealedInside.clear();
	m_revealedInsideUnit.reset();
	m_revealAllInside = false;

	// A traced layout holds the target and what it rests on and nothing else, so every line in it is part of the answer.
	if (m_focusTarget)
	{
		m_revealAllInside = true;
		for (uint32_t e = 0; e < m_layout.edges().size(); ++e)
		{
			m_revealedEdges.insert(e);
		}
		return;
	}

	// A pinned panel is the target subject until dismissed, so the lines and the panel are about the same step.
	const auto subject = (m_detailPinned && m_selection) ? m_selection : (m_hover ? m_hover : m_selection);
	if (!subject)
	{
		return;
	}

	// The lines the picked step is part of, and only those.
	// What a step needs may be both inside its own progression and outside of it, so both are gathered.
	const auto& placed = m_layout.units()[subject->unit];
	const int32_t tier = placed.memberTiers[subject->member];
	const int32_t row = placed.memberRows[subject->member];
	for (uint32_t e = 0; e < m_layout.edges().size(); ++e)
	{
		const auto& edge = m_layout.edges()[e];
		if ((edge.from == subject->unit && edge.fromTier == tier && edge.fromRow == row)
		    || (edge.to == subject->unit && edge.toTier == tier && edge.toRow == row))
		{
			m_revealedEdges.insert(e);
		}
	}

	m_revealedInside.clear();
	m_revealedInsideUnit = subject->unit;
	for (size_t e = 0; e < placed.internalEdges.size(); ++e)
	{
		if (placed.internalEdges[e].first == subject->member || placed.internalEdges[e].second == subject->member)
		{
			m_revealedInside.insert(e);
		}
	}
}

uint32_t WzResearchTreeCanvas::unitOfResearch(uint16_t researchIndex) const
{
	return (researchIndex < m_unitOfResearch.size()) ? m_unitOfResearch[researchIndex] : UINT32_MAX;
}

std::shared_ptr<WIDGET> WzResearchTreeCanvas::findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed)
{
	auto result = WIDGET::findMouseTargetRecursive(psContext, key, wasPressed);

	// This runs on every mouse event and once a frame for hover - ensure we only process this once per frame
	const uint32_t frame = frameGetFrameNumber();
	const int wheel = getMouseWheelSpeed().y;
	if (wheel != 0 && frame != m_lastWheelFrame)
	{
		m_lastWheelFrame = frame;
		const Vector2i local(psContext->mx - x(), psContext->my - y());
		const Vector2f before = screenToLogical(local, 0, 0);
		m_viewMoved = true;
		m_zoom = std::clamp(m_zoom * std::pow(1.12f, static_cast<float>(wheel)), minimumZoom(), ZOOM_MAX);
		const Vector2f after = screenToLogical(local, 0, 0);
		m_pan += before - after;	// keep whatever is under the pointer under it
		clampPan();
		rebuildLabels();
		m_lastCanvasMoveTime = realTime;
		if (m_detailPinned || !m_detailFollowsNode) { followDetail(); } else { closeDetail(); }
	}
	return result;
}

void WzResearchTreeCanvas::mouseDragged(WIDGET_KEY key, W_CONTEXT *start, W_CONTEXT *current)
{
	if (key != WKEY_PRIMARY)
	{
		return;
	}
	const int dx = current->mx - start->mx;
	const int dy = current->my - start->my;
	if (!m_dragMoved)
	{
		if (std::abs(dx) < DRAG_THRESHOLD && std::abs(dy) < DRAG_THRESHOLD)
		{
			return;
		}
		m_dragMoved = true;
		m_dragStartPan = m_pan;
	}
	m_viewMoved = true;
	m_pan = m_dragStartPan - Vector2f(static_cast<float>(dx) / m_zoom, static_cast<float>(dy) / m_zoom);
	clampPan();
	m_lastCanvasMoveTime = realTime;
	if (m_detailPinned || !m_detailFollowsNode) { followDetail(); } else { closeDetail(); }
}

void WzResearchTreeCanvas::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
	const bool wasDrag = m_dragMoved;
	m_dragMoved = false;
	if (key != WKEY_PRIMARY || wasDrag)
	{
		return;		// the release that ends a pan must not pick something
	}
	const auto hit = pickAt(Vector2i(psContext->mx - x(), psContext->my - y()));
	m_selection = hit;
	if (hit)
	{
		openDetail(*hit, true);
		if (m_onUnitActivated)
		{
			m_onUnitActivated(hit->unit);
		}
	}
	else
	{
		closeDetail();
	}
}

void WzResearchTreeCanvas::highlightLost()
{
	m_hover.reset();
	if (!m_detailPinned && m_detailFollowsNode)
	{
		closeDetail();
	}
}

// The triggers zoom, left out and right in - as they do to the camera in-game
void WzResearchTreeCanvas::applyGamepadZoom()
{
	if (!isGamepadActiveInput())
	{
		return;
	}
	const float direction = gamepadAxis(GPAD_AXIS_RIGHT_TRIGGER) - gamepadAxis(GPAD_AXIS_LEFT_TRIGGER);
	if (direction == 0.f)
	{
		return;
	}

	const float before = m_zoom;
	m_viewMoved = true;
	m_zoom = std::clamp(m_zoom * std::pow(GAMEPAD_ZOOM_RATE, direction * realTimeAdjustedIncrement(1.f)), minimumZoom(), ZOOM_MAX);
	if (m_zoom == before)
	{
		return;
	}

	// Around the middle of the view (instead of the gamepad pointer location)
	const Vector2f center(static_cast<float>(width()) / 2.f, static_cast<float>(height()) / 2.f);
	m_pan += Vector2f(center.x / before - center.x / m_zoom, center.y / before - center.y / m_zoom);
	clampPan();
	rebuildLabels();
	m_lastCanvasMoveTime = realTime;
	if (m_detailPinned || !m_detailFollowsNode) { followDetail(); } else { closeDetail(); }
}

// Consume gesture input when the research tree is up, so that it (a) handles it and (b) doesn't leave it for the game world
void WzResearchTreeCanvas::applyGestureInput()
{
	bool moved = false;

	if (auto scale = consumePinchGestureScaleUpdate())
	{
		const float before = m_zoom;
		m_zoom = std::clamp(m_zoom * scale.value(), minimumZoom(), ZOOM_MAX);
		if (m_zoom != before)
		{
			// The middle of the view, since the gesture carries a scale but no center point
			const Vector2f center(static_cast<float>(width()) / 2.f, static_cast<float>(height()) / 2.f);
			m_pan += Vector2f(center.x / before - center.x / m_zoom, center.y / before - center.y / m_zoom);
			moved = true;
		}
	}

	if (auto delta = consumePanGestureDeltaUpdate())
	{
		if (delta.value().deltaX != 0.f || delta.value().deltaY != 0.f)
		{
			m_pan -= Vector2f(delta.value().deltaX / m_zoom, delta.value().deltaY / m_zoom);
			moved = true;
		}
	}

	if (!moved)
	{
		return;
	}
	m_viewMoved = true;
	clampPan();
	rebuildLabels();
	m_lastCanvasMoveTime = realTime;
	if (m_detailPinned || !m_detailFollowsNode) { followDetail(); } else { closeDetail(); }
}

void WzResearchTreeCanvas::run(W_CONTEXT *psContext)
{
	applyGamepadZoom();
	applyGestureInput();
	tickLabels();
	tickState();

	// The tree is moving under everything parked on it, the same as a pan does
	if (reflowPart() > 0.f)
	{
		m_lastCanvasMoveTime = realTime;
		followDetail();
	}

	if (m_labeled != m_labeledTarget)
	{
		const float part = std::min(1.f, static_cast<float>(realTime - m_pitchStart) / static_cast<float>(PITCH_TIME));
		m_labeled = m_labeledFrom + (m_labeledTarget - m_labeledFrom) * part;
		applyPresentation();
		m_lastCanvasMoveTime = realTime;
		rebuildLabels();
		if (m_labeled == m_labeledTarget && m_refitOnSettle)
		{
			m_refitOnSettle = false;
			fitToView();
		}
	}

	if (m_detailContents)
	{
		const bool canvasMoving = (realTime - m_lastCanvasMoveTime) < DETAIL_MOVING_LINGER;
		m_detailContents->setRestingAlpha(canvasMoving ? DETAIL_ALPHA_MOVING : DETAIL_ALPHA_RESTING);
	}

	// A panel hung off something outside the canvas is not the canvas's to take down
	if (!isMouseOverWidget())
	{
		m_hover.reset();
		if (!m_detailPinned && m_detailFollowsNode)
		{
			closeDetail();
		}
		return;
	}
	m_hover = pickAt(Vector2i(psContext->mx - x(), psContext->my - y()));

	if (m_detailPinned)
	{
		return;
	}
	if (!m_hover)
	{
		closeDetail();
	}
	else if (!m_detailPick || *m_detailPick != *m_hover)
	{
		openDetail(*m_hover, false);
	}
}

void WzResearchTreeCanvas::closeDetail()
{
	const bool wasParked = reservedTopPx() > 0;
	if (m_detailPopover)
	{
		m_detailPopover->close();
		m_detailPopover.reset();
	}
	if (!m_detailFollowsNode)
	{
		m_detailAnchor.reset();		// borrowed, thus not a detach
		m_detailFollowsNode = true;
	}
	if (m_detailAnchor)
	{
		detach(m_detailAnchor);
		m_detailAnchor.reset();
	}
	m_detailContents.reset();
	m_detailPick.reset();
	m_detailPinned = false;
	// The room it was standing in is the tree's to use again
	if (wasParked)
	{
		clampPan();
	}
}

void WzResearchTreeCanvas::followDetail()
{
	if (!m_detailFollowsNode)
	{
		// Beside something off the canvas, so nothing the canvas does moves it and losing
		// its own hover does not close it. (Whatever asked for it owns it.)
		return;
	}
	if (!m_detailPopover || !m_detailPick || !m_detailAnchor)
	{
		return;
	}
	if (m_detailPick->unit >= m_layout.unitCount())
	{
		closeDetail();
		return;
	}

	const WzRect node = detailAnchorRect(*m_detailPick);
	m_detailAnchor->setGeometry(node.x(), node.y(), node.width(), node.height());

	// A panel parked in a corner stays put. A panel beside its node waits out of sight while the node is off screen,
	// ready to return if the view moves back.
	const bool nodeOnCanvas = m_focusTarget || (node.right() >= 0 && node.x() <= width()
		&& node.bottom() >= 0 && node.y() <= height());
	if (nodeOnCanvas)
	{
		m_detailPopover->show();
		m_detailPopover->callCalcLayout();
	}
	else
	{
		m_detailPopover->hide();
	}
}

void WzResearchTreeCanvas::openDetailBeside(uint16_t subject, const std::shared_ptr<WIDGET>& anchor, bool pinned)
{
	auto pick = pickForResearch(subject);
	if (!pick.has_value() || anchor == nullptr)
	{
		return;
	}
	openDetail(pick.value(), pinned, anchor);
}

void WzResearchTreeCanvas::openDetail(TreePick pick, bool pinned, const std::shared_ptr<WIDGET>& besideInstead)
{
	closeDetail();
	// A popover puts itself on an overlay screen of its own, and is positioned based on (and has its lifetime bounded by) this one.
	if (screenPointer.expired())
	{
		return;
	}
	if (pick.unit >= m_layout.unitCount() || pick.member >= m_layout.units()[pick.unit].members.size())
	{
		return;
	}
	const uint16_t subject = m_layout.units()[pick.unit].members[pick.member];

	// A picked node is the subject of whatever the player does next, so its panel carries the action buttons.
	// A previewed node (i.e. hover) only describes.
	ResearchDetailActions actions;
	if (pinned && topicKnown(subject))
	{
		std::weak_ptr<WzResearchTreeCanvas> weakSelf = std::dynamic_pointer_cast<WzResearchTreeCanvas>(shared_from_this());
		actions.tracingPath = m_focusTarget.has_value();
		actions.onTracePath = [weakSelf]() {
			if (auto self = weakSelf.lock())
			{
				self->requestFocusToggle();
			}
		};

		if (m_context.allowAssignment && canStartResearchNow(m_context.player, subject))
		{
			actions.onResearch = [weakSelf, subject](const std::shared_ptr<WIDGET>& from) {
				if (auto self = weakSelf.lock())
				{
					self->requestResearch(subject, from);
				}
			};
		}
	}

	const int32_t widest = std::min<int32_t>(DETAIL_MAX_WIDTH, std::max(DETAIL_WIDTH, width() / 3));
	auto contents = makeResearchDetailContents(m_graph, m_layout.units()[pick.unit], m_context, DETAIL_WIDTH, widest, actions, subject, topicKnown(subject));
	if (!contents)
	{
		return;
	}
	// No more than half the canvas height
	contents->setMaxHeight(std::min<int32_t>(WzResearchDetailContents::MAX_PANEL_HEIGHT, height() / 2));

	// Park an empty widget over the node so the popover has something to anchor to,
	// since it aligns to a widget (and a node is only a rectangle).
	// Where the panel goes depends on how tall it came out.
	m_detailContents = contents;
	// If requested from outside the tree (ex. a lab in the toolbar), the panel hangs off
	// whatever asked rather than off a node, and the canvas moving must not move the panel.
	m_detailFollowsNode = (besideInstead == nullptr);
	if (!m_detailFollowsNode)
	{
		m_detailAnchor = besideInstead;
	}
	else
	{
		const WzRect node = detailAnchorRect(pick);
		m_detailAnchor = std::make_shared<WIDGET>();
		m_detailAnchor->setTransparentToMouse(true);
		m_detailAnchor->setGeometry(node.x(), node.y(), node.width(), node.height());
		attach(m_detailAnchor);
	}

	// A preview stays click-through.
	// A picked node's panel takes events on its own buttons and lets the rest fall through, so the canvas keeps its wheel and drag.
	// Neither panel dismisses itself - panning, zooming, picking elsewhere or closing the view all manage it from here.
	m_detailPopover = PopoverWidget::makePopover(m_detailAnchor, contents,
	                                             pinned ? PopoverWidget::Style::MouseInteractive : PopoverWidget::Style::NonInteractive,
	                                             PopoverWidget::Alignment::LeftOfParent, Vector2i(0, DETAIL_PANEL_DROP));
	m_detailPick = pick;
	m_detailPinned = pinned;
	if (m_detailFollowsNode)
	{
		// A panel parked over the tree takes room the tree was using
		clampPan();
		followDetail();
	}
}

void WzResearchTreeCanvas::display(int xOffset, int yOffset)
{
	const int originX = xOffset + x();
	const int originY = yOffset + y();
	const WzRect view(originX, originY, width(), height());

	const PIELIGHT ground = canvasBase();
	pie_UniTransBoxFill(static_cast<float>(originX), static_cast<float>(originY),
	                    static_cast<float>(originX + width()), static_cast<float>(originY + height()),
	                    ground);

	// Which units are on screen (recomputed here since pan and zoom both move it)
	m_visibleUnits.clear();
	for (uint32_t unit = 0; unit < m_layout.unitCount(); ++unit)
	{
		const WzRect box = unitOnScreen(unit, originX, originY);
		if (box.right() >= originX && box.x() <= originX + width() && box.bottom() >= originY && box.y() <= originY + height())
		{
			m_visibleUnits.push_back(unit);
		}
	}

	updateRevealedEdges();

	// Edges first, so nodes sit on top of them. Only those touching the subject.
	// (Since the instanced rect path takes no clip rect, each piece is cut to the widget.)
	m_rectBatch.clear();
	m_rectBatch.resizeRectGroups(1);
	const auto addClippedRect = [this, &view](int x0, int y0, int x1, int y1, PIELIGHT color) {
		// A reversed edge runs right to left, so the corners are put in order first
		const int left = std::max(std::min(x0, x1), view.x());
		const int top = std::max(std::min(y0, y1), view.y());
		const int right = std::min(std::max(x0, x1), view.x() + view.width());
		const int bottom = std::min(std::max(y0, y1), view.y() + view.height());
		if (right <= left || bottom <= top)
		{
			return;
		}
		m_rectBatch.addRect(PIERECT_DrawRequest(left, top, right, bottom, color));
	};
	// A long edge is drawn through points of its own between the tiers it crosses, and
	// those belong to the layout rather than any progression, so they have nowhere to
	// travel from while the tree moves. Lines are skipped until it settles rather than
	// drawn between one end that has moved and one that has not.
	const bool settling = reflowPart() > 0.f;
	size_t revealedSegments = 0;
	for (uint32_t s = 0; s < m_layout.segments().size() && !settling; ++s)
	{
		const auto& segment = m_layout.segments()[s];
		if (m_revealedEdges.count(segment.edge) == 0)
		{
			continue;
		}
		revealedSegments++;
		const Vector2i fromAt = logicalToScreen(m_layout.segmentFrom(s), originX, originY);
		const Vector2i toAt = logicalToScreen(m_layout.segmentTo(s), originX, originY);
		// A line into or out of a step the traced target does not rest on says nothing about how to reach it.
		// This batch draws opaque, so it is faded by mixing toward the canvas rather than by alpha.
		PIELIGHT color = laneColor(m_layout.edges()[segment.edge].colorLane);
		if (!m_layout.edges()[segment.edge].bothWanted)
		{
			color = blendToward(canvasBase(), color, OFF_PATH_FADE);
		}

		// Right out of the source, across in the gap between tiers, then into the target.
		// Axis-aligned so the whole set batches.
		const int midX = (fromAt.x + toAt.x) / 2;
		addClippedRect(fromAt.x, fromAt.y - 1, midX, fromAt.y + 1, color);
		addClippedRect(midX - 1, std::min(fromAt.y, toAt.y), midX + 1, std::max(fromAt.y, toAt.y), color);
		addClippedRect(midX, toAt.y - 1, toAt.x, toAt.y + 1, color);
	}
	if (revealedSegments > 0)
	{
		m_rectBatch.drawAllRects();
	}

	BatchedImageDrawRequests iconBatch(true);	// collect every icon, then draw them in one go
	// Half the strip's own height, so a strip is a rounded end whatever density it is drawn at.
	const float cornerRadius = static_cast<float>(m_layout.metrics().unitHeight) / 2.f * m_zoom;
	const bool showLabels = labelsVisible();
	const bool stepNamesVisible = showLabels && m_zoom >= ZOOM_STEP_NAMES;
	const bool showDetail = m_zoom >= ZOOM_DETAIL;

	// Names of strips that run off the left of the view, collected as they are met and drawn once every unit is down.
	struct PinnedName { uint32_t unit; int x; int room; int textY; int centerY; PIELIGHT color; };
	std::vector<PinnedName> pinnedNames;

	// A name is drawn over whatever the lines and the fill left behind it, which on a traced
	// path is a good deal, so laying it down once in black first helps keeps it legible
	// without a box. Cut to the room it has.
	const auto renderNamed = [&view](WzCachedText& text, int textX, int textY, PIELIGHT color, int room) {
		int shown = room;
		const bool truncated = text.getTextWidth() > room;
		if (truncated)
		{
			shown -= iV_GetEllipsisWidth(text.getFontID()) + 2;
		}
		if (shown <= 0)
		{
			return;
		}
		PIELIGHT shade = pal_RGBA(0, 0, 0, static_cast<uint8_t>(static_cast<float>(color.byte.a) * 0.7f));
		text->renderClipped(Vector2f(static_cast<float>(textX) + 1.f, static_cast<float>(textY) + 1.f), shade, view, shown);
		text->renderClipped(Vector2f(textX, textY), color, view, shown);
		if (truncated)
		{
			const Vector2f at(static_cast<float>(textX + shown + 2), static_cast<float>(textY));
			if (at.x >= static_cast<float>(view.x()) && at.x <= static_cast<float>(view.x() + view.width()))
			{
				iV_DrawEllipsis(text.getFontID(), at, color);
			}
		}
	};

	for (const auto unit : m_visibleUnits)
	{
		const auto& layoutUnit = m_layout.units()[unit];
		// The state of the step the player is on
		const auto current = actionableTopic(unit);
		const auto node = m_graph.nodeForResearchIndex(current ? *current : layoutUnit.members.front());
		const NodeState state = node ? m_graph.nodes()[*node].state : NodeState::Locked;
		const bool hovered = (m_hover && m_hover->unit == unit);
		const bool selected = (m_selection && m_selection->unit == unit);

		// Whether anything in the box can be started, which is not the state of the step
		// it has reached - a progression can be blocked there but still hold something
		// startable on another row
		bool active = false;
		for (const auto member : layoutUnit.members)
		{
			const auto memberNode = m_graph.nodeForResearchIndex(member);
			const NodeState memberState = memberNode ? m_graph.nodes()[*memberNode].state : NodeState::Locked;
			if (memberState == NodeState::Available || memberState == NodeState::InProgress)
			{
				active = true;
				break;
			}
		}

		// Drawn from the unrounded transform, to ensure the box and pips align properly
		const WzRect box = unitOnScreen(unit, originX, originY);
		const Vector2i unitSize = m_layout.unitSize(unit);
		const Vector2f corner = logicalToScreenF(unitOriginLogical(unit), originX, originY);
		const float x0 = corner.x;
		const float y0 = corner.y;
		const float x1 = x0 + static_cast<float>(unitSize.x) * m_zoom;
		const float y1 = y0 + static_cast<float>(unitSize.y) * m_zoom;

		const PIELIGHT lane = laneColor(layoutUnit.laneIndex);
		const StatePaint paint = statePaintFor(state, active, hovered);

		// A name pinned inside a strip takes room that the lines and the step names would otherwise use
		const bool namedOutside = layoutUnit.members.size() > 1 && m_labeled >= 1.f;
		const UnitLabelPlacement nameAt = showLabels
			? unitLabelPlacement(unit, box, view, namedOutside) : UnitLabelPlacement();

		// Only what the player is pointing at is outlined
		PIELIGHT border = pal_RGBA(0, 0, 0, 0);
		float borderWidth = 0.f;
		if (hovered || selected)
		{
			border = pal_Colour(235, 240, 245);
			borderWidth = 1.5f;
		}

		// A node just jumped to pulses to grab attention
		if (selected && realTime < m_revealedUntil)
		{
			const float phase = static_cast<float>((m_revealedUntil - realTime) % REVEAL_MARK_PERIOD) / static_cast<float>(REVEAL_MARK_PERIOD);
			const float pulse = 0.5f - 0.5f * std::cos(phase * 2.f * static_cast<float>(M_PI));
			border = pal_Colour(255, 235, 150);
			borderWidth = 2.f + 2.f * pulse;
		}

		PIELIGHT fill = blendToward(canvasBase(), lane, std::min(1.f, NODE_TINT / SOLIDITY_ACTIVE));
		fill.byte.a = static_cast<uint8_t>(255.f * paint.solidity);

		// Where a traced path stops inside this unit. The box carries on past it, so the
		// part the target does not rest on is drawn faded-out.
		const optional<float> pathEndsAt = tracedSplitX(unit, box, originX);
		if (!pathEndsAt)
		{
			// One call covers fill, outline, rounded ends and the cut at the viewport edge
			pie_DrawRoundedBoxClipped(x0, y0, x1, y1, fill, cornerRadius, border, borderWidth, view);
		}
		else
		{
			// The whole box shape is cut, so the two pieces meet cleanly and the outline stays continuous
			PIELIGHT faded = fill;
			faded.byte.a = static_cast<uint8_t>(static_cast<float>(fill.byte.a) * OFF_PATH_FADE);
			const int splitX = std::clamp(static_cast<int>(*pathEndsAt), view.x(), view.x() + view.width());
			const WzRect onPath(view.x(), view.y(), splitX - view.x(), view.height());
			const WzRect offPath(splitX, view.y(), view.x() + view.width() - splitX, view.height());
			pie_DrawRoundedBoxClipped(x0, y0, x1, y1, fill, cornerRadius, border, borderWidth, onPath);
			pie_DrawRoundedBoxClipped(x0, y0, x1, y1, faded, cornerRadius, border, borderWidth, offPath);
		}

		// Handle dependencies between this unit's own steps.
		// (Drawn here rather than with the rest, which would be covered by this nodes's fill.)
		if (m_revealAllInside || (m_revealedInsideUnit && *m_revealedInsideUnit == unit))
		{
			// Approximating what a line crossing behind the unit comes out at
			PIELIGHT thread = blendToward(lane, pal_Colour(255, 255, 255), 0.35f);
			thread.byte.a = 110;
			// Use a width the eye can follow when the view is zoomed out, which
			// is where a line inside a strip can be the hardest to pick out
			const float weight = std::max(2.f, 1.6f * m_zoom);

			// A step's name is written along the line leaving its pip, so the line breaks
			// where a name is drawn instead of running through it
			struct NameRun { float from; float to; int32_t row; };
			std::vector<NameRun> named;
			for (size_t m = 0; m < layoutUnit.members.size(); ++m)
			{
				const bool stepNamed = stepNamesVisible && layoutUnit.members.size() > 1
					&& unit < m_stepLabels.size();
				const int held = heldLabelRoomPx(unit, m);
				// What is drawn (not the whole name), since a truncated name leaves the rest of the line to draw
				const int nameWidth = stepNamed
					? std::min(m_stepLabels[unit][m].getTextWidth(), stepNameRoomPx(unit, m) - held) : 0;
				const int textWidth = held + std::max(0, nameWidth);
				if (textWidth <= 0)
				{
					continue;
				}
				const float from = pipScreenX(unit, m, box, originX) + stepReachPx(unit, m) + 2.f;
				named.push_back({from, from + static_cast<float>(textWidth) + unitPadPx(), layoutUnit.memberRows[m]});
			}
			// The unit's own name too, when it has been pinned inside the strip
			if (nameAt.pinned && nameAt.draw)
			{
				named.push_back({static_cast<float>(nameAt.x) - 2.f,
				                 static_cast<float>(nameAt.x + nameAt.room) + 2.f, 0});
			}
			std::sort(named.begin(), named.end(), [](const NameRun& a, const NameRun& b) { return a.from < b.from; });

			for (size_t e = 0; e < layoutUnit.internalEdges.size(); ++e)
			{
				if (!m_revealAllInside && m_revealedInside.count(e) == 0)
				{
					continue;
				}
				const auto& pair = layoutUnit.internalEdges[e];
				PIELIGHT threadHere = thread;
				if (!onTracedPath(layoutUnit.members[pair.first]) || !onTracedPath(layoutUnit.members[pair.second]))
				{
					threadHere.byte.a = static_cast<uint8_t>(static_cast<float>(thread.byte.a) * OFF_PATH_FADE);
				}
				const float fromX = pipScreenX(unit, pair.first, box, originX);
				const float toX = pipScreenX(unit, pair.second, box, originX);
				const float fromY = y0 + rowOffsetPx(unit, layoutUnit.memberRows[pair.first]);
				const float toY = y0 + rowOffsetPx(unit, layoutUnit.memberRows[pair.second]);
				const float midX = (fromX + toX) / 2.f;
				// A bar joining two pips runs through their middles
				const auto bar = [&](float ax, float ay, float bx, float by) {
					const float half = weight / 2.f;
					pie_DrawRoundedBoxClipped(std::min(ax, bx) - half, std::min(ay, by) - half, std::max(ax, bx) + half,
					                          std::max(ay, by) + half, threadHere, 0.f, pal_RGBA(0, 0, 0, 0), 0.f, view);
				};
				// A run along a row (minus whatever a name on that row has taken)
				const auto rowBar = [&](float ax, float bx, float y, int32_t row) {
					float at = std::min(ax, bx);
					const float end = std::max(ax, bx);
					for (const auto& run : named)
					{
						if (at >= end) { break; }
						if (run.row != row || run.to <= at || run.from >= end)
						{
							continue;
						}
						if (run.from > at) { bar(at, y, run.from, y); }
						at = run.to;
					}
					if (at < end) { bar(at, y, end, y); }
				};
				rowBar(fromX, midX, fromY, layoutUnit.memberRows[pair.first]);
				if (fromY != toY) { bar(midX, fromY, midX, toY); }
				rowBar(midX, toX, toY, layoutUnit.memberRows[pair.second]);
			}
		}

		// The topic's own icon, batched into one draw for the whole canvas.
		// Outside the unit, in the space every tier leaves clear (so the inside belongs to the steps).
		const uint16_t iconID = node ? m_graph.nodes()[*node].iconID : 0;
		const float iconSize = iconSizePx(box.height());
		const float drawnIcon = iconSize;
		if (iconID != 0 && drawnIcon >= ICON_MIN_PX)
		{
			const float iconX = x0 + unitPadPx();
			const float iconY = y0 + rowOffsetPx(unit, 0) - drawnIcon / 2.f;
			// No clipping support (currently), so skip when it would be clipped
			if (iconX >= static_cast<float>(view.x()) && iconY >= static_cast<float>(view.y())
			    && iconX + drawnIcon <= static_cast<float>(view.x() + view.width())
			    && iconY + drawnIcon <= static_cast<float>(view.y() + view.height()))
			{
				iV_DrawImageTint(IntImages, iconID, iconX, iconY,
				                 pal_RGBA(255, 255, 255, paint.contentAlpha), Vector2f(drawnIcon, drawnIcon),
				                 defaultProjectionMatrix(), &iconBatch);
			}
		}

		// A step at each tier it reached, communicating how long a progression is, how far
		// through it the player is, and where each step's lines leave from, without any text.
		// Every unit has these - a lone topic simply has one.
		const float radius = pipRadiusPx();
		for (size_t m = 0; m < layoutUnit.members.size(); ++m)
		{
			const auto memberNode = m_graph.nodeForResearchIndex(layoutUnit.members[m]);
			const NodeState memberState = memberNode ? m_graph.nodes()[*memberNode].state : NodeState::Locked;
			PipPaint pip = pipPaintFor(memberState, lane);
			const bool onPath = onTracedPath(layoutUnit.members[m]);
			if (!onPath)
			{
				pip.color.byte.a = static_cast<uint8_t>(static_cast<float>(pip.color.byte.a) * OFF_PATH_FADE);
			}
			const float cx = pipScreenX(unit, m, box, originX);
			const float cy = y0 + rowOffsetPx(unit, layoutUnit.memberRows[m]);

			// The two states worth acting on are drawn larger, colored differently and ringed.
			// Allowed to overshoot its row, which a pip already does at the zoom floor.
			const bool actionable = (memberState == NodeState::Available || memberState == NodeState::InProgress);
			// Everyone on the team holds it, which is worth telling apart from some of them holding it.
			// Said in the shape rather than the color: a full pip either way, and a band outside the
			// dark cut if the whole team is covered.
			const bool heldByAll = !m_context.aggregate.empty() && memberState == NodeState::Researched
				&& m_heldCount[layoutUnit.members[m]] >= m_context.aggregate.size();
			const float pipR = actionable ? radius + actionableExtraPx(radius) : radius;
			const auto allies = alliesResearching(m_context, layoutUnit.members[m]);

			// A ripple behind whatever can be started, growing outward and fading, and the
			// same ripple backwards on a topic being worked on. Nothing ripples off a traced
			// path, or on a topic a teammate has a lab on (in shared research mode), since
			// starting that is the doubling-up we warn about.
			//
			// For "available", only display ripples on a tree the viewer can act on.
			const bool ripplesOut = (memberState == NodeState::Available && allies.empty()
			                         && m_context.allowAssignment);
			const bool ripplesIn = (memberState == NodeState::InProgress);
			if (onPath && (ripplesOut || ripplesIn))
			{
				const float phase = static_cast<float>(realTime % PIP_PULSE_PERIOD) / static_cast<float>(PIP_PULSE_PERIOD);
				// Both radius and alpha hang off this, so running it the other way is the entire reversal
				const float travel = ripplesOut ? phase : 1.f - phase;
				// How far it travels and how heavy the line is both have a floor in screen pixels.
				// Grown purely off the pip it is few pixels of fading hairline once the view is
				// zoomed-out, which is where it most needs to catch the eye.
				const float edge = drawnPipReachPx(radius, true);
				const float reach = std::max(RIPPLE_LEAST_PX, pipR * 1.5f);
				// Going out, it leaves the pip's edge and fades out.
				// Coming in it ends under the pip (which is filled), and its brightest
				// part is swallowed instead of blinking out at the edge.
				const float shore = ripplesOut ? edge : pipR * 0.5f;
				const float ring = shore + (edge + reach - shore) * travel;
				PIELIGHT wash = pip.color;
				wash.byte.a = static_cast<uint8_t>(RIPPLE_ALPHA * (1.f - travel));
				pie_DrawRoundedBoxClipped(cx - ring, cy - ring, cx + ring, cy + ring,
				                          pal_RGBA(0, 0, 0, 0), ring, wash,
				                          std::max(1.5f, pipR * 0.4f), view);
			}

			// A teammate has a lab on this topic.
			if (!allies.empty() && onPath)
			{
				// More than one lab on it stops being a note about a teammate and
				// becomes something worth undoing, so it takes the interface's warning color
				const bool wasted = std::find(m_duplicated.begin(), m_duplicated.end(),
				                              layoutUnit.members[m]) != m_duplicated.end();
				// Outside whatever the state already drew there, so a ring about a teammate
				// is not mistaken for one about the state
				const float outside = drawnPipReachPx(radius, actionable, heldByAll);
				const float ring = outside + std::max(TEAM_RING_LEAST_PX, pipR * 0.7f);
				const PIELIGHT team = wasted ? WASTED_RING_COLOR
				                             : pal_GetTeamColour(getPlayerColour(allies.front()));
				pie_DrawRoundedBoxClipped(cx - ring, cy - ring, cx + ring, cy + ring,
				                          pal_RGBA(0, 0, 0, 0), ring, team,
				                          std::max(wasted ? 1.6f : 1.2f, radius * (wasted ? 0.55f : 0.4f)), view);
			}

			pie_DrawRoundedBoxClipped(cx - pipR, cy - pipR, cx + pipR, cy + pipR,
			                          pip.hollow ? pal_RGBA(0, 0, 0, 0) : pip.color, pipR,
			                          pip.hollow ? pip.color : pal_RGBA(0, 0, 0, 0),
			                          pip.hollow ? std::max(1.2f, pipR * 0.45f) : 0.f, view);

			// A step worth acting on is cut out of what it sits on by a dark hairline and ringed in its own color.
			if (actionable || heldByAll)
			{
				const float keyline = pipR + KEYLINE_WIDTH_PX;
				pie_DrawRoundedBoxClipped(cx - keyline, cy - keyline, cx + keyline, cy + keyline,
				                          pal_RGBA(0, 0, 0, 0), keyline, canvasBase(), KEYLINE_WIDTH_PX, view);
				// The band tells a topic the whole team holds from one only some of them do,
				// so it is drawn at any size if heldByAll.
				if (stateRingVisible(radius) || heldByAll)
				{
					const float halo = keyline + KEYLINE_WIDTH_PX;
					pie_DrawRoundedBoxClipped(cx - halo, cy - halo, cx + halo, cy + halo,
					                          pal_RGBA(0, 0, 0, 0), halo, pip.color, KEYLINE_WIDTH_PX, view);
				}
			}

			// How much of the team holds it.
			const int heldRoom = heldLabelRoomPx(unit, m);
			if (heldRoom > 0)
			{
				const uint32_t held = m_heldCount[layoutUnit.members[m]];
				// The color of a step's name, not the pip. A partly held step is drawn dim on
				// purpose, and text that dim is unlikely to be read.
				PIELIGHT heldLabelColor = WZCOL_TEXT_MEDIUM;
				heldLabelColor.byte.a = paint.contentAlpha;
				if (!onPath)
				{
					heldLabelColor.byte.a = static_cast<uint8_t>(static_cast<float>(heldLabelColor.byte.a) * OFF_PATH_FADE);
				}
				const int textY = static_cast<int>(cy) - static_cast<int>(m_heldLabels[held]->aboveBase() / 2);
				renderNamed(m_heldLabels[held], static_cast<int>(cx + drawnPipReachPx(radius, actionable, heldByAll)) + HELD_LABEL_GAP_PX,
				            textY, heldLabelColor, heldRoom);
			}

			// Which step is being described, since a strip may hold several and the panel beside it describes one
			const TreePick here{unit, m};
			if ((m_hover && *m_hover == here) || (m_selection && *m_selection == here))
			{
				const float ring = drawnPipReachPx(radius, actionable, heldByAll) + std::max(1.5f, 2.f * m_zoom);
				pie_DrawRoundedBoxClipped(cx - ring, cy - ring, cx + ring, cy + ring,
				                          pal_RGBA(0, 0, 0, 0), ring,
				                          pal_Colour(235, 240, 245), std::max(1.f, 1.2f * m_zoom), view);
			}
		}

		if (nameAt.draw)
		{
			PIELIGHT textColor = WZCOL_TEXT_BRIGHT;
			textColor.byte.a = paint.contentAlpha;
			const int textY = box.y() + static_cast<int>(rowOffsetPx(unit, 0))
				- static_cast<int>(m_unitLabels[unit]->aboveBase() / 2);
			if (nameAt.pinned)
			{
				// Held back to the end.
				// A name inside a strip sits where the strip's own contents are,
				// so it is drawn over all of them rather than among them.
				pinnedNames.push_back({unit, nameAt.x, nameAt.room, textY,
				                       box.y() + static_cast<int>(rowOffsetPx(unit, 0)), textColor});
			}
			else
			{
				renderNamed(m_unitLabels[unit], nameAt.x, textY, textColor, nameAt.room);
			}
		}

		// What each step is called, once there is room between one pip and the next.
		// A lone topic is left to its unit label, which says the same in the same place.
		if (stepNamesVisible && layoutUnit.members.size() > 1 && unit < m_stepLabels.size())
		{
			PIELIGHT stepColor = WZCOL_TEXT_MEDIUM;
			stepColor.byte.a = paint.contentAlpha;
			for (size_t m = 0; m < m_stepLabels[unit].size(); ++m)
			{
				// After the label saying how much of the team holds this step, so the two
				// read as one line instead of overlapping
				const int held = heldLabelRoomPx(unit, m);
				const int stepX = static_cast<int>(pipScreenX(unit, m, box, originX) + stepReachPx(unit, m) + unitPadPx()) + held;
				// A name pinned inside the strip sits where a step name would go, so avoid two
				// names on top of each other
				if (nameAt.pinned && nameAt.draw && layoutUnit.memberRows[m] == 0
				    && stepX < nameAt.x + nameAt.room && stepX + m_stepLabels[unit][m].getTextWidth() > nameAt.x)
				{
					continue;
				}
				PIELIGHT stepLabelColor = stepColor;
				if (!onTracedPath(layoutUnit.members[m]))
				{
					stepLabelColor.byte.a = static_cast<uint8_t>(static_cast<float>(stepLabelColor.byte.a) * OFF_PATH_FADE);
				}
				const int textY = box.y() + static_cast<int>(rowOffsetPx(unit, layoutUnit.memberRows[m]))
					- static_cast<int>(m_stepLabels[unit][m]->aboveBase() / 2);
				renderNamed(m_stepLabels[unit][m], stepX, textY, stepLabelColor, stepNameRoomPx(unit, m) - held);
			}
		}
	}

	iconBatch.draw(true);

	// Render the pinned names last, above the units / contents
	for (const auto& pinned : pinnedNames)
	{
		auto& label = m_unitLabels[pinned.unit];
		const int drawn = std::min(label.getTextWidth(), pinned.room);
		const float half = static_cast<float>(label.getTextLineSize()) / 2.f + 2.f;
		const float x0 = static_cast<float>(pinned.x) - PINNED_NAME_PAD_PX;
		const float x1 = static_cast<float>(pinned.x + drawn) + PINNED_NAME_PAD_PX;
		const float y0 = static_cast<float>(pinned.centerY) - half;
		const float y1 = static_cast<float>(pinned.centerY) + half;
		pie_DrawRoundedBoxClipped(x0, y0, x1, y1, pal_RGBA(16, 18, 24, 205), half,
		                          pal_RGBA(120, 132, 152, 150), 1.f, view);
		renderNamed(label, pinned.x, pinned.textY, pinned.color, pinned.room);
	}

	if (showDetail && m_selection && m_selection->unit < m_layout.unitCount())
	{
		// A plain readout when the detail popover does not exist
		const auto& members = m_layout.units()[m_selection->unit].members;
		const uint16_t picked = members[std::min(m_selection->member, members.size() - 1)];
		WzText detail;
		std::string text = getLocalizedStatsName(&asResearch[picked]);
		text += "  " + std::to_string(asResearch[picked].researchPoints) + " pts";
		detail.setText(WzString::fromUtf8(text), font_regular);
		detail.render(originX + 12, originY + height() - 12, WZCOL_TEXT_BRIGHT);
	}
}
