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
 *  A pannable research tree canvas
 */

#ifndef __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREECANVAS_H__
#define __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREECANVAS_H__

#include "lib/widget/widget.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/textdraw.h"

#include "lib/widget/popover.h"
#include "lib/widget/paragraph.h"	// for WzCachedText

#include "researchdetailpopover.h"
#include "researchassign.h"
#include "researchfocus.h"
#include "researchtreelayout.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

// What "a lab is working on this" looks like
inline PIELIGHT researchInProgressColor() { return pal_Colour(204, 235, 19); }

// The ground the tree is drawn on - neutral, so lane hues and state colors distinguish
PIELIGHT researchTreeCanvasGround();

// What color a tech category's finished steps are drawn as
PIELIGHT researchLaneSwatchColor(uint8_t lane);

// How far a panel parked in a corner sits in from the canvas edges.
// (Anything else laid over the canvas keeps the same inset, so what the view puts on the tree lines up.)
static constexpr int DETAIL_CORNER_INSET = 8;

struct TreePick
{
	uint32_t unit = 0;
	size_t member = 0;
	bool operator==(const TreePick& other) const { return unit == other.unit && member == other.member; }
	bool operator!=(const TreePick& other) const { return !(*this == other); }
};

struct UnitLabelPlacement
{
	int x = 0;
	int room = 0;
	bool pinned = false; // inside the strip rather than in the gutter beside it
	bool draw = false;
};

class WzResearchTreeCanvas : public WIDGET
{
public:
	~WzResearchTreeCanvas();

	static std::shared_ptr<WzResearchTreeCanvas> make(const ResearchTreeContext& context);

	void setContext(const ResearchTreeContext& context);
	// Every player has the same tree, so switching between them keeps the layout and only changes progress state
	void setPlayer(uint32_t player) { setPerspective(player, std::vector<uint32_t>(), m_context.allowAssignment); }
	// Whose tree, and whether it may be acted on
	void setPerspective(uint32_t player, std::vector<uint32_t> aggregate, bool allowAssignment);
	void refreshState();
	// Names or the whole tree.
	// Two tasks benefit from different densities: following what leads to what (where names add noise),
	// and finding / identifying a topic by name, where they are (of course) the point.
	void setLabeled(bool labeled);
	bool isLabeled() const { return m_labeledTarget > 0.5f; }

	// Center the view on a topic and mark it (ex. after a search). The mark animates a fade-out.
	bool revealResearch(uint16_t researchIndex);
	// The first topic in reading order the player could start right now
	optional<uint16_t> firstStartableTopic() const;
	void fitToView();
	// How far out the view is allowed to go, which is however far the whole tree needs to be displayed
	float minimumZoom() const;

	// Lay out only what the selected unit rests on. Nothing selected, or already focused, goes back to the whole tree.
	bool toggleFocusOnSelection();
	bool isFocused() const { return m_focusTarget.has_value(); }
	const ResearchFocus& focus() const { return m_focus; }
	optional<uint16_t> focusTarget() const { return m_focusTarget; }

	void setOnUnitActivated(std::function<void(uint32_t unit)> handler) { m_onUnitActivated = std::move(handler); }
	void setOnFocusRequested(std::function<void()> handler) { m_onFocusRequested = std::move(handler); }
	void requestFocusToggle();
	optional<uint32_t> hoveredUnit() const { return m_hover ? optional<uint32_t>(m_hover->unit) : nullopt; }
	optional<uint32_t> selectedUnit() const { return m_selection ? optional<uint32_t>(m_selection->unit) : nullopt; }
	void clearSelection();
	// Move the pick along the tree as it is drawn (for non-pointer interaction)
	void stepSelection(int delta);

	const ResearchTreeLayout& layout() const { return m_layout; }
	// Topics the team has more than one lab on
	const std::vector<uint16_t>& duplicatedTopics() const { return m_duplicated; }

	void closePopovers();
	bool detailIsOpen() const { return m_detailPopover != nullptr; }
	// Describe a topic from somewhere that is not the tree (ex. a lab in the toolbar)
	void openDetailBeside(uint16_t subject, const std::shared_ptr<WIDGET>& anchor, bool pinned);

	// The color bands anything placed on the canvas falls into - for the legend.
	// (Grows as a campaign reveals topics.)
	const std::vector<uint8_t>& lanesShown() const { return m_lanesShown; }

protected:
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void highlightLost() override;

	bool capturesMouseDrag(WIDGET_KEY key) override { return key == WKEY_PRIMARY; }
	void mouseDragged(WIDGET_KEY key, W_CONTEXT *start, W_CONTEXT *current) override;
	bool canConsumeWheelScroll() override { return true; }
	std::shared_ptr<WIDGET> findMouseTargetRecursive(W_CONTEXT *psContext, WIDGET_KEY key, bool wasPressed) override;

private:
	void rebuild();
	void rebuildLayout();
	void rebuildLabelText();
	void rebuildLabels();
	void refreshHeldCounts();
	// Whether the whole tree fits at a zoom its names can be read at
	bool labeledPitchFits();
	// Clear space between the things sitting along a unit (sized off the row)
	float unitPadPx() const;
	int heldLabelRoomPx(uint32_t unit, size_t member);
	// Re-read what the game has done behind the view, and act on it only if it has moved
	void tickState();
	// Place the tree around a topic the player has just been given, keeping where they were
	// and reflowing everything that shifted
	void reflowForDiscovery(std::vector<bool> discovered);
	// How much of the reflow movement is left to travel - 1 at the start and 0 once the tree has settled
	float reflowPart() const;
	// Where a unit is drawn (which is where it was packed once nothing is moving)
	Vector2f unitOriginLogical(uint32_t unit) const;
	optional<TreePick> pickForResearch(uint16_t researchIndex) const;
	// Whether a topic's name, cost and effects may be shown
	bool topicKnown(uint16_t researchIndex) const
	{
		return m_known.empty() || (researchIndex < m_known.size() && m_known[researchIndex]);
	}
	// Release cached textures for labels that have been off screen for a bit
	void tickLabels();
	// The popover positions itself against a widget and nodes are not widgets, so a small
	// invisible one is parked over whichever node it describes. (Given something else to
	// sit beside, it hangs off that instead.)
	void openDetail(TreePick pick, bool pinned, const std::shared_ptr<WIDGET>& besideInstead = nullptr);
	void closeDetail();
	// Keep a pinned popover beside its node while the canvas moves under it
	void followDetail();
	// Where the label starts, and how much room it has before whatever follows it
	float labelOffsetPx(uint32_t unit) const;
	int labelRoomPx(uint32_t unit) const;
	UnitLabelPlacement unitLabelPlacement(uint32_t unit, const WzRect& box, const WzRect& view, bool namedOutside) const;
	int stepNameRoomPx(uint32_t unit, size_t member) const;
	// How far a step is drawn out to, which is past its pip where the state also rings it.
	// (What is written beside a step starts from this point.)
	float stepReachPx(uint32_t unit, size_t member) const;
	// Re-space the tree for wherever the switch between densities has reached
	LayoutMetrics presentationMetrics() const;
	void applyPresentation();
	float iconSizePx(int boxHeight) const;
	// A progression's steps, drawn as pips along it at the tier each one reached (and, where two share a tier, on a row of their own)
	float rowOffsetPx(uint32_t unit, int32_t rowWithinUnit) const;
	float pipRadiusPx() const;
	float pipScreenX(uint32_t unit, size_t member, const WzRect& box, int originX) const;
	float bulletOffsetPx(int boxHeight) const;
	bool labelsVisible() const;
	float pipOffsetPx(uint32_t unit, int32_t tier) const;
	optional<int32_t> secondTierOf(uint32_t unit) const;
	// The box a unit occupies on screen at the current pan and zoom, and the part of it a panel describing that unit should sit beside
	WzRect unitOnScreen(uint32_t unit, int originX, int originY) const;
	WzRect stepOnScreen(TreePick pick, int originX, int originY) const;
	WzRect detailAnchorRect(TreePick pick) const;
	int detailPanelHeight() const;
	// The middle of one step's pip, in layout coordinates
	Vector2f pipCenter(uint32_t unit, size_t member) const;
	void applyGamepadZoom();
	void applyGestureInput();
	// The step a progression has reached, which is what its fill summarizes.
	// (What a panel describes and acts on is whichever step is being pointed at.)
	optional<uint16_t> actionableTopic(uint32_t unit) const;
	// Whether a topic is one a traced target rests on, rather than a step of the same progression that came along with it
	bool onTracedPath(uint16_t researchIndex) const;
	// Where a unit's box stops being part of a traced path, for drawing the rest of it back
	optional<float> tracedSplitX(uint32_t unit, const WzRect& box, int originX) const;
	void requestResearch(uint16_t researchIndex, const std::shared_ptr<WIDGET>& from);
	void openLabPicker(uint16_t researchIndex, const std::vector<ResearchLabOption>& options, const std::shared_ptr<WIDGET>& from);
	// How much of the top of the canvas a parked panel is standing in
	int reservedTopPx() const;
	void clampPan();
	Vector2i logicalToScreen(Vector2i logical, int originX, int originY) const;
	Vector2f logicalToScreenF(Vector2f logical, int originX, int originY) const;
	Vector2f screenToLogical(Vector2i screen, int originX, int originY) const;
	optional<TreePick> pickAt(Vector2i localPoint) const;
	void updateRevealedEdges();
	uint32_t unitOfResearch(uint16_t researchIndex) const;

	ResearchTreeContext m_context;
	// Which topics this context may show. Empty outside campaign, where the whole tree is the player's to see.
	std::vector<bool> m_visible;
	// Which topics this context may name. A campaign can choose to show the shape of what the player has not found,
	// while still hiding what it is.
	std::vector<bool> m_known;
	// Worked out on the same tick that re-reads state, since it walks every topic
	std::vector<uint16_t> m_duplicated;
	// How many of an aggregate context's seats hold each topic, by research index. Empty outside one.
	std::vector<uint8_t> m_heldCount;
	// Recomputed wherever the visible set moves (not per frame)
	std::vector<uint8_t> m_lanesShown;
	ResearchGraph m_graph;
	std::vector<ResearchTrack> m_tracks;
	ResearchTreeLayout m_layout;
	// The topic a focus is on, and what it rests on. Empty when the whole tree is shown.
	// Topics in reading order, left to right and down each tier, which is the only order in which stepping between them makes sense
	std::vector<TreePick> m_picksInReadingOrder;
	std::vector<uint32_t> m_unitOfResearch;		// by research index (or UINT32_MAX when left out)
	std::set<uint32_t> m_revealedEdges;		// indices into the layout's edges, for this frame
	std::set<size_t> m_revealedInside;		// indices into one unit's own internal edges
	optional<uint32_t> m_revealedInsideUnit;
	bool m_revealAllInside = false;		// while a path is traced, every line in it
	optional<uint16_t> m_focusTarget;
	ResearchFocus m_focus;

	Vector2f m_pan = Vector2f(0.f, 0.f);	// logical point drawn at the widget's top left
	float m_zoom = 1.0f;
	optional<TreePick> m_hover;
	optional<TreePick> m_selection;
	bool m_dragMoved = false;
	Vector2f m_dragStartPan;
	uint32_t m_lastWheelFrame = 0;
	uint32_t m_revealedUntil = 0;	// the `realTime` at which the reveal mark stops being drawn
	// Whether the player has panned, zoomed or jumped somewhere
	// Until they have, the opening view is the widget's to decide (and is redone on every resize).
	bool m_viewMoved = false;

	std::vector<WzCachedText> m_unitLabels;
	std::vector<std::string> m_unitLabelText;
	// One per step, for the names shown inside a unit once there is room
	std::vector<std::vector<WzCachedText>> m_stepLabels;
	// How much of the team holds a topic (i.e. "3 of 5")
	// One per count rather than one per topic - empty outside an aggregate context
	std::vector<WzCachedText> m_heldLabels;
	// 0 compact, 1 labeled - anything between while it is moving
	float m_labeled = 0.f;
	float m_labeledTarget = 0.f;
	// Set once the player has explicitly indicated which density they want
	bool m_densityAsked = false;
	float m_labeledFrom = 0.f;
	uint32_t m_pitchStart = 0;
	std::vector<uint32_t> m_visibleUnits;
	BatchedMultiRectRenderer m_rectBatch;
	std::shared_ptr<WIDGET> m_detailAnchor;
	std::shared_ptr<PopoverWidget> m_detailPopover;
	std::shared_ptr<WzResearchDetailContents> m_detailContents;
	std::shared_ptr<PopoverWidget> m_labPicker;
	uint32_t m_lastCanvasMoveTime = 0;
	uint32_t m_lastStateRefresh = 0;
	// Set when the density goes back to compact, so the view goes back out to the whole tree once the pitch has finished changing
	bool m_refitOnSettle = false;
	// How far each unit still has to travel after the tree was placed again, in layout coordinates, decaying to nothing over the reflow
	std::vector<Vector2f> m_reflowOffset;
	uint32_t m_reflowStart = 0;
	optional<TreePick> m_detailPick;
	bool m_detailPinned = false;
	bool m_detailFollowsNode = true;	// false while it is beside something off the canvas
	std::function<void(uint32_t)> m_onUnitActivated;
	std::function<void()> m_onFocusRequested;
};

#endif // __INCLUDED_SRC_RESEARCHTREE_RESEARCHTREECANVAS_H__
