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

#include "objectivescard.h"
#include "intelkit.h"
#include "intelmodel.h"
#include "intelpanel.h"
#include "intelview.h"

#include "../hud.h"
#include "../hudids.h"
#include "../hudrightcolumn.h"
#include "../hudsettings.h"
#include "../hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"

#include "../../display3d.h"
#include "../../hci.h"
#include "../../loop.h"
#include "../../map.h"
#include "../../message.h"
#include "../../messagedef.h"
#include "../../mission.h"

#include <algorithm>
#include <map>

using namespace intelkit;

namespace
{

constexpr int WIDTH = 300;
constexpr int WIDTH_COMPACT = 240;
constexpr int HEADER_HEIGHT = 34;
constexpr int ONE_LINE_HEIGHT = 36;
constexpr int CHIP_SIZE = 30;
constexpr int ICON_SIZE = 16;
constexpr int SMALL_BUTTON = 22;
constexpr int MARKER_BUTTON = 24;
constexpr int ROW_PAD = 6;
constexpr int TEXT_X = 34;
constexpr int PROGRESS_HEIGHT = 9;
constexpr int MORE_HEIGHT = 22;
constexpr int POP_UP_FOOTER = 32;
constexpr int POP_UP_GAP = 8;
constexpr uint32_t POP_UP_SETTLE_MS = 1000;   ///< its place clear this long before the pop-up shows
constexpr uint32_t POP_UP_STALE_MS = 120000;  ///< a pop-up waiting longer is dropped (the dot stays)
constexpr uint32_t HIGHLIGHT_MS = 5000;
constexpr uint32_t NEW_MS = 12000;
constexpr uint32_t LINE_KEEP_MS = 4000;       ///< one line keeps what changed this long past its highlight
constexpr uint32_t OPEN_NEW_MS = 8000;        ///< new objectives: the docked card opens this long (a little longer for many)
constexpr uint32_t OPEN_NEW_MAX_MS = 12000;
constexpr uint32_t OPEN_CHANGED_MS = 5000;    ///< a change: the docked card shows it on one line this long
constexpr uint32_t FOLD_MS = 180;         ///< the card unfolding or folding, the pop-up opening or closing
constexpr uint32_t CHIP_POP_MS = 140;     ///< the chip appearing
constexpr uint32_t HIGHLIGHT_FADE_MS = 600;
constexpr int COMPACT_BELOW_WIDTH = 960;
constexpr int COMPACT_BELOW_HEIGHT = 600;
constexpr uint32_t IDHUD_OBJECTIVES = IDHUD_ADDITION_FIRST + 0x90;
constexpr uint32_t IDHUD_OBJECTIVES_POP_UP = IDHUD_ADDITION_FIRST + 0x91;

struct Mark
{
	uint32_t highlightUntil = 0;
	uint32_t newUntil = 0;
};

struct Seen
{
	std::string text;
	ObjectiveState state = ObjectiveState::Active;
	nonstd::optional<std::pair<int32_t, int32_t>> progress;
};

/// What just changed: its highlight and "New" mark (in realTime)
std::map<std::string, Mark> marks;
/// (For the geometry dump) A state set the objectives or the card's state
bool setForTests = false;
/// The objectives as last noted, to see what changed
std::map<std::string, Seen> seen;
uint32_t seenRevision = 0;
uint32_t seenGameTime = 0;
bool seenValid = false;
/// The next notice is a save's objectives, not news
bool quiet = false;
/// Counts changes and reminders, for the minimized card's dot
uint32_t attentionSerial = 0;
/// New objectives, a change or a reminder: the docked card opens further than its resting state for a while (counted
/// while it's on screen, and not while the pointer is on it), then folds back
struct Opening
{
	ObjectivesDockState state = ObjectivesDockState::Minimized;
	uint32_t remainingMs = 0;
};
Opening opening;
/// The docked card's state as the player chose it, for the rest of the mission
nonstd::optional<ObjectivesDockState> chosenState;
/// A change for the pop-up, waiting for the place over the reticule to be clear: how long it shows, what to highlight
struct PendingPopUp
{
	bool waiting = false;
	uint32_t since = 0;
	uint32_t ms = 0;
	std::vector<std::string> keys;
};
PendingPopUp pendingPopUp;
/// Since when the pop-up's place has been clear
uint32_t popUpClearSince = 0;
bool popUpWasClear = false;
/// A change not seen yet (no card: a dot on the intelligence button until the pop-up has shown it, or the
/// intelligence screen is opened)
bool unseen = false;
/// (For the geometry dump) An opening that lasts
bool openingHeldForTests = false;
/// (For the geometry dump) The pop-up's time stands still
bool popUpHeldForTests = false;

bool compactLayout()
{
	return screenWidth < COMPACT_BELOW_WIDTH || screenHeight < COMPACT_BELOW_HEIGHT;
}

int cardWidth()
{
	return compactLayout() ? WIDTH_COMPACT : WIDTH;
}

bool highlighted(const Objective &objective)
{
	auto it = marks.find(hudObjectiveKey(objective));
	return it != marks.end() && realTime < it->second.highlightUntil;
}

bool animated()
{
	// (The geometry dump's states that set objectives: their end state, not a frame of the motion)
	return !hudReduceMotion() && !setForTests;
}

float easeOut(float t)
{
	const float u = 1.f - std::max(0.f, std::min(1.f, t));
	return 1.f - u * u * u;
}

/// How much of the highlight shows: it fades out at its end
float highlightStrength(const Objective &objective)
{
	auto it = marks.find(hudObjectiveKey(objective));
	if (it == marks.end() || realTime >= it->second.highlightUntil)
	{
		return 0.f;
	}
	if (!animated())
	{
		return 1.f;
	}
	return std::min(1.f, static_cast<float>(it->second.highlightUntil - realTime) / static_cast<float>(HIGHLIGHT_FADE_MS));
}

uint8_t scaled(uint8_t value, float strength)
{
	return static_cast<uint8_t>(std::round(static_cast<float>(value) * strength));
}

/// A height moving to its target: eased over FOLD_MS, or at once
class HeightMotion
{
public:
	bool moving() const { return now != to; }

	int update(int target)
	{
		if (now < 0 || !animated())
		{
			from = to = now = target;
			return now;
		}
		if (target != to)
		{
			from = now;
			to = target;
			startedAt = realTime;
		}
		const float t = static_cast<float>(realTime - startedAt) / static_cast<float>(FOLD_MS);
		now = t >= 1.f ? to : static_cast<int>(std::round(static_cast<float>(from) + static_cast<float>(to - from) * easeOut(t)));
		return now;
	}

	void reset(int value)
	{
		from = to = now = value;
	}

private:
	int from = -1;
	int to = -1;
	int now = -1;
	uint32_t startedAt = 0;
};

bool isNew(const Objective &objective)
{
	auto it = marks.find(hudObjectiveKey(objective));
	return it != marks.end() && realTime < it->second.newUntil && objective.options.state == ObjectiveState::Active;
}

std::vector<Objective> localObjectives()
{
	return selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer) : std::vector<Objective>();
}

const Objective *firstActive(const std::vector<Objective> &objectives)
{
	for (const Objective &objective : objectives)
	{
		if (objective.options.state == ObjectiveState::Active)
		{
			return &objective;
		}
	}
	return nullptr;
}

/// The objective that changed last, while it's highlighted (and `graceMs` past that)
const Objective *latestHighlighted(const std::vector<Objective> &objectives, uint32_t graceMs)
{
	const Objective *latest = nullptr;
	uint32_t latestUntil = 0;
	for (const Objective &objective : objectives)
	{
		auto it = marks.find(hudObjectiveKey(objective));
		if (it != marks.end() && it->second.highlightUntil + graceMs > realTime && it->second.highlightUntil >= latestUntil)
		{
			latest = &objective;
			latestUntil = it->second.highlightUntil;
		}
	}
	return latest;
}

void drawIcon(const std::string &name, int x, int y, int size, PIELIGHT color)
{
	skin().drawNamedIcon(name, WzRect(x, y, size, size), color);
}

/// A small icon button (minimize, fold, show on map): a card when pointed at, or always (`framed`)
class IconButton : public W_BUTTON
{
public:
	IconButton(const std::string &icon, bool framed): icon(icon), framed(framed) {}

	std::string icon;

protected:
	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (framed || isHighlighted())
		{
			skin().drawCard(rect, isHighlighted() ? HudCardState::Highlighted : HudCardState::Normal);
		}
		const int s = std::min(rect.width(), rect.height()) - (framed ? 8 : 6);
		drawIcon(icon, rect.x() + (rect.width() - s) / 2, rect.y() + (rect.height() - s) / 2, s,
		         isHighlighted() ? skin().colors().textPrimary : skin().colors().textSecondary);
	}

private:
	bool framed;
};

/// The header's area: a click folds or unfolds the card
class HeaderArea : public W_BUTTON
{
protected:
	void display(int, int) override {}
};

/// An objective in the card: its state's icon, its text (struck through once done or failed), its progress with a bar,
/// "New", and its place on the map
class ObjectiveRow : public W_BUTTON
{
public:
	ObjectiveRow(const Objective &objective, const std::shared_ptr<ObjectivesCard> &card): objective(objective), card(card) {}

	void initialize()
	{
		const ObjectivesCardMode mode = cardMode();
		if (objective.options.state == ObjectiveState::Active && hudObjectiveHasPlace(objective))
		{
			auto button = std::make_shared<IconButton>("show-on-map", true);
			button->setTip(_("Show on map"));
			const Objective target = objective;
			button->addOnClickHandler([target](W_BUTTON &) {
				widgScheduleTask([target]() { hudShowObjectiveOnMap(target); });
			});
			marker = button;
			attach(button);
		}
		addOnClickHandler([](W_BUTTON &button) {
			auto &row = static_cast<ObjectiveRow &>(button);
			auto strongCard = row.card.lock();
			if (strongCard && strongCard->onSelect)
			{
				strongCard->onSelect(row.objective);
			}
		});
		if (mode != ObjectivesCardMode::Pinned)
		{
			setTip(objective.text);
		}
	}

	static size_t maxLines(ObjectivesCardMode mode)
	{
		return mode == ObjectivesCardMode::Pinned ? 3 : 2;
	}

	static int textWidth(const Objective &objective, int width)
	{
		int w = width - TEXT_X - 10;
		if (objective.options.progress.has_value())
		{
			w -= 36;
		}
		if (objective.options.state == ObjectiveState::Active && hudObjectiveHasPlace(objective))
		{
			w -= MARKER_BUTTON + 6;
		}
		return std::max(40, w);
	}

	static int heightFor(const Objective &objective, int width, ObjectivesCardMode mode)
	{
		const size_t lines = std::max<size_t>(1, hudWrapText(objective.text, fontFor(objective), textWidth(objective, width), maxLines(mode)).size());
		const int textHeight = static_cast<int>(lines) * iV_GetTextLineSize(fontFor(objective));
		const int h = ROW_PAD * 2 + textHeight + (objective.options.progress.has_value() ? PROGRESS_HEIGHT : 0);
		return std::max(h, ROW_PAD * 2 + MARKER_BUTTON - 4);
	}

	void geometryChanged() override
	{
		lines = hudWrapText(objective.text, fontFor(objective), textWidth(objective, width()), maxLines(cardMode()));
		if (marker)
		{
			marker->setGeometry(width() - 8 - MARKER_BUTTON, (height() - MARKER_BUTTON) / 2, MARKER_BUTTON, MARKER_BUTTON);
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		auto strongCard = card.lock();
		const bool pinned = cardMode() == ObjectivesCardMode::Pinned;
		const bool selected = pinned && strongCard && strongCard->isSelected && strongCard->isSelected(objective);
		const bool active = objective.options.state == ObjectiveState::Active;
		const WzRect box(rect.x() + 4, rect.y() + 1, rect.width() - 8, rect.height() - 2);
		if (selected)
		{
			skin().drawCard(box, HudCardState::Selected);
		}
		else if (highlighted(objective))
		{
			const float strength = highlightStrength(objective);
			pie_DrawRoundedBox(static_cast<float>(box.x()), static_cast<float>(box.y()), static_cast<float>(box.right()), static_cast<float>(box.bottom()),
			                   alpha(colors.stateWarn, scaled(40, strength)), skin().metrics().cardRadius, alpha(colors.stateWarn, scaled(220, strength)), 1.f);
		}
		else if (pinned && isHighlighted())
		{
			skin().drawCard(box, HudCardState::Highlighted);
		}

		const iV_fonts font = fontFor(objective);
		const int lineHeight = iV_GetTextLineSize(font);
		const int top = rect.y() + ROW_PAD;
		const PIELIGHT stateColor = hudObjectiveColor(objective);
		drawIcon(hudObjectiveIcon(objective), rect.x() + 12, top + (lineHeight - ICON_SIZE) / 2, ICON_SIZE, stateColor);

		const int textX = rect.x() + TEXT_X;
		const int maxWidth = textWidth(objective, rect.width());
		const PIELIGHT textColor = active ? colors.textPrimary : alpha(colors.textSecondary, 200);
		texts.resize(lines.size());
		int lastRight = textX;
		for (size_t i = 0; i < lines.size(); ++i)
		{
			const int lineTop = top + static_cast<int>(i) * lineHeight;
			const int drawn = drawLine(texts[i], lines[i], font, textX, lineTop, maxWidth, textColor);
			if (!active)
			{
				const int strikeY = lineTop + lineHeight / 2 + 1;
				pie_BoxFill(textX, strikeY, textX + drawn, strikeY + 1, alpha(colors.textSecondary, 200));
			}
			lastRight = textX + drawn;
		}
		const int lastTop = top + static_cast<int>(std::max<size_t>(1, lines.size()) - 1) * lineHeight;
		if (isNew(objective))
		{
			const std::string label = _("New");
			if (lastRight + 8 + tagWidth(newTag, label) <= textX + maxWidth)
			{
				drawTag(newTag, label, lastRight + 8, lastTop + lineHeight / 2, colors.stateWarn, false);
			}
		}
		if (objective.options.progress.has_value())
		{
			const auto &progress = *objective.options.progress;
			const std::string count = astringf("%d/%d", progress.first, progress.second);
			const int right = marker ? rect.right() - 8 - MARKER_BUTTON - 8 : rect.right() - 12;
			drawLine(progressText, count, font_regular_bold, right, top, 40, active ? colors.textPrimary : colors.textSecondary, true);
			const int barTop = top + static_cast<int>(std::max<size_t>(1, lines.size())) * lineHeight + 4;
			const int barWidth = std::min(130, maxWidth);
			const float fraction = std::max(0.f, std::min(1.f, static_cast<float>(progress.first) / static_cast<float>(progress.second)));
			pie_DrawRoundedBox(static_cast<float>(textX), static_cast<float>(barTop), static_cast<float>(textX + barWidth), static_cast<float>(barTop + 3),
			                   alpha(colors.textSecondary, 70), 1.5f);
			if (fraction > 0.f)
			{
				pie_DrawRoundedBox(static_cast<float>(textX), static_cast<float>(barTop), static_cast<float>(textX) + static_cast<float>(barWidth) * fraction,
				                   static_cast<float>(barTop + 3), active ? colors.stateWarn : alpha(colors.textSecondary, 160), 1.5f);
			}
		}
	}

private:
	static iV_fonts fontFor(const Objective &objective)
	{
		return objective.options.primary && objective.options.state == ObjectiveState::Active ? font_regular_bold : font_regular;
	}

	ObjectivesCardMode cardMode() const
	{
		auto strongCard = card.lock();
		return strongCard ? strongCard->cardMode() : ObjectivesCardMode::Docked;
	}

	Objective objective;
	std::weak_ptr<ObjectivesCard> card;
	std::shared_ptr<W_BUTTON> marker;
	std::vector<std::string> lines;
	std::vector<WzText> texts;
	WzText progressText;
	WzText newTag;
};

/// (Pinned) The rows that don't fit, as "3 more"
class MoreButton : public W_BUTTON
{
public:
	std::string label;

protected:
	void display(int xOffset, int yOffset) override
	{
		const PIELIGHT color = isHighlighted() ? skin().colors().textPrimary : skin().colors().textSecondary;
		drawLine(text, label, font_small, xOffset + x() + TEXT_X, yOffset + y() + 4, width() - TEXT_X - 10, color);
	}

private:
	WzText text;
};

}

// MARK: - The card

std::shared_ptr<ObjectivesCard> ObjectivesCard::make(ObjectivesCardMode mode)
{
	auto card = std::make_shared<ObjectivesCard>();
	card->initialize(mode);
	return card;
}

void ObjectivesCard::initialize(ObjectivesCardMode cardMode)
{
	mode = cardMode;
	std::weak_ptr<ObjectivesCard> weakSelf = std::static_pointer_cast<ObjectivesCard>(shared_from_this());
	if (mode == ObjectivesCardMode::Docked)
	{
		auto area = std::make_shared<HeaderArea>();
		area->addOnClickHandler([weakSelf](W_BUTTON &) {
			auto self = weakSelf.lock();
			if (self && self->onToggle)
			{
				widgScheduleTask(self->onToggle);
			}
		});
		header = area;
		attach(area);
		auto minimize = std::make_shared<IconButton>("minimize", false);
		minimize->setTip(_("Minimize"));
		minimize->addOnClickHandler([weakSelf](W_BUTTON &) {
			auto self = weakSelf.lock();
			if (self && self->onMinimize)
			{
				widgScheduleTask(self->onMinimize);
			}
		});
		minimizeButton = minimize;
		attach(minimize);
		auto toggle = std::make_shared<IconButton>("chevron-up", false);
		toggle->addOnClickHandler([weakSelf](W_BUTTON &) {
			auto self = weakSelf.lock();
			if (self && self->onToggle)
			{
				widgScheduleTask(self->onToggle);
			}
		});
		toggleButton = toggle;
		attach(toggle);
	}
	if (mode == ObjectivesCardMode::Pinned)
	{
		auto more = std::make_shared<MoreButton>();
		more->addOnClickHandler([weakSelf](W_BUTTON &) {
			auto self = weakSelf.lock();
			if (self && self->onMore)
			{
				widgScheduleTask(self->onMore);
			}
		});
		moreButton = more;
		attach(more);
		more->hide();
	}
	reload();
}

void ObjectivesCard::setOneLine(bool newOneLine)
{
	if (oneLine == newOneLine)
	{
		return;
	}
	oneLine = newOneLine;
	layoutRows();
}

int ObjectivesCard::fullHeight()
{
	return oneLine ? ONE_LINE_HEIGHT : expandedHeight();
}

int ObjectivesCard::expandedHeight()
{
	if (cachedWidth != width())
	{
		cachedWidth = width();
		cachedHeight = HEADER_HEIGHT + 4;
		for (const Objective &objective : objectives)
		{
			cachedHeight += ObjectiveRow::heightFor(objective, width(), mode);
		}
		cachedHeight += 6;
	}
	return cachedHeight;
}

void ObjectivesCard::reload()
{
	revision = objectivesRevision();
	objectives = localObjectives();
	cachedWidth = -1;
	for (auto &row : rows)
	{
		detach(row);
	}
	rows.clear();
	auto self = std::static_pointer_cast<ObjectivesCard>(shared_from_this());
	for (const Objective &objective : objectives)
	{
		auto row = std::make_shared<ObjectiveRow>(objective, self);
		row->initialize();
		rows.push_back(row);
		attach(row);
	}
	layoutRows();
}

void ObjectivesCard::layoutRows()
{
	const int w = width();
	if (header)
	{
		header->setGeometry(0, 0, w, oneLine ? ONE_LINE_HEIGHT : HEADER_HEIGHT);
		const int buttonTop = ((oneLine ? ONE_LINE_HEIGHT : HEADER_HEIGHT) - SMALL_BUTTON) / 2;
		toggleButton->setGeometry(w - 8 - SMALL_BUTTON, buttonTop, SMALL_BUTTON, SMALL_BUTTON);
		minimizeButton->setGeometry(w - 8 - SMALL_BUTTON * 2 - 2, buttonTop, SMALL_BUTTON, SMALL_BUTTON);
		auto toggle = std::static_pointer_cast<IconButton>(toggleButton);
		toggle->icon = oneLine ? "chevron-down" : "chevron-up";
		toggle->setTip(oneLine ? _("Show all objectives") : _("Fold to one line"));
	}
	int top = HEADER_HEIGHT + 4;
	const int bottom = height() - 6;
	// (Pinned, when they don't all fit: those that do, and "3 more" below them)
	const bool allFit = fullHeight() <= height();
	const int limit = allFit || mode != ObjectivesCardMode::Pinned ? bottom : bottom - MORE_HEIGHT;
	size_t hidden = 0;
	for (size_t i = 0; i < rows.size(); ++i)
	{
		const int h = ObjectiveRow::heightFor(objectives[i], w, mode);
		if (oneLine || hidden > 0 || top + h > limit)
		{
			rows[i]->hide();
			++hidden;
			continue;
		}
		rows[i]->setGeometry(0, top, w, h);
		rows[i]->show();
		top += h;
	}
	if (moreButton)
	{
		auto more = std::static_pointer_cast<MoreButton>(moreButton);
		more->label = astringf(ngettext("%u more", "%u more", static_cast<unsigned>(hidden)), static_cast<unsigned>(hidden));
		more->setGeometry(0, top, w, MORE_HEIGHT);
		more->show(hidden > 0 && !oneLine);
	}
}

void ObjectivesCard::geometryChanged()
{
	layoutRows();
}

void ObjectivesCard::run(W_CONTEXT *)
{
	if (revision != objectivesRevision())
	{
		reload();
	}
}

void ObjectivesCard::display(int xOffset, int yOffset)
{
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const HudColors &colors = skin().colors();
	const PIELIGHT amber = colors.stateWarn;
	if (mode == ObjectivesCardMode::Pinned)
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   alpha(amber, 26), skin().metrics().cardRadius, alpha(amber, 150), 1.f);
	}
	else if (mode == ObjectivesCardMode::Docked)
	{
		PIELIGHT fill = colors.panelBackground;
		fill.byte.a = 238;
		// (One line: what changed, or the reminder, shows on the whole card)
		float strength = 0.f;
		if (oneLine)
		{
			for (const Objective &objective : objectives)
			{
				strength = std::max(strength, highlightStrength(objective));
			}
		}
		if (strength > 0.f)
		{
			auto mix = [strength](uint8_t base, uint8_t tint) {
				return static_cast<uint8_t>(std::round(static_cast<float>(base) + (static_cast<float>(tint) - static_cast<float>(base)) * 0.25f * strength));
			};
			fill = pal_RGBA(mix(fill.byte.r, amber.byte.r), mix(fill.byte.g, amber.byte.g), mix(fill.byte.b, amber.byte.b), 245);
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   fill, skin().metrics().cardRadius, alpha(amber, 190), 1.f);
	}
	const int headerHeight = oneLine ? ONE_LINE_HEIGHT : HEADER_HEIGHT;
	pie_BoxFill(rect.x() + 1, rect.y() + 8, rect.x() + 4, rect.y() + headerHeight - 8, amber);
	const int buttonsWidth = header ? SMALL_BUTTON * 2 + 14 : 10;

	if (oneLine)
	{
		const Objective *first = latestHighlighted(objectives, LINE_KEEP_MS);
		if (first == nullptr)
		{
			first = firstActive(objectives);
		}
		const int centerY = rect.y() + ONE_LINE_HEIGHT / 2;
		const std::string text = first ? first->text : _("All objectives done");
		const PIELIGHT iconColor = first ? hudObjectiveColor(*first) : colors.stateOk;
		drawIcon(first ? hudObjectiveIcon(*first) : "objective-done", rect.x() + 12, centerY - ICON_SIZE / 2, ICON_SIZE, iconColor);
		int right = rect.right() - buttonsWidth;
		size_t others = 0;
		for (const Objective &objective : objectives)
		{
			others += objective.options.state == ObjectiveState::Active ? 1 : 0;
		}
		others = others > 0 ? others - 1 : 0;
		if (others > 0)
		{
			plusText.setText(WzString::fromUtf8(astringf("+%u", static_cast<unsigned>(others))), font_small);
			const int pw = plusText.width() + 16;
			const int ph = 20;
			const int px = right - pw;
			pie_DrawRoundedBox(static_cast<float>(px), static_cast<float>(centerY - ph / 2), static_cast<float>(px + pw), static_cast<float>(centerY + ph / 2),
			                   alpha(colors.textSecondary, 30), ph / 2.f, alpha(colors.textSecondary, 150), 1.f);
			plusText.render(static_cast<float>(px + 8), std::round(static_cast<float>(centerY) - plusText.lineSize() / 2.f - plusText.aboveBase()), colors.textSecondary);
			right = px - 8;
		}
		const int lineHeight = iV_GetTextLineSize(font_regular_bold);
		drawLine(lineText, text, font_regular_bold, rect.x() + TEXT_X, centerY - lineHeight / 2, right - rect.x() - TEXT_X, colors.textPrimary);
		return;
	}

	const int titleHeight = iV_GetTextLineSize(font_regular_bold);
	drawIcon("objective", rect.x() + 12, rect.y() + (HEADER_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, amber);
	const std::string title = hudObjectivesTitle();
	const std::string done = hudObjectivesDoneText(objectives);
	const int textX = rect.x() + TEXT_X;
	const int availableWidth = rect.right() - buttonsWidth - textX;
	countText.setText(WzString::fromUtf8(done), font_small);
	titleText.setText(WzString::fromUtf8(title), font_regular_bold);
	const int doneWidth = done.empty() ? 0 : countText.width() + 8;
	const int titleWidth = std::min(titleText.width(), std::max(40, availableWidth - doneWidth));
	drawLine(titleText, title, font_regular_bold, textX, rect.y() + (HEADER_HEIGHT - titleHeight) / 2, titleWidth, colors.textPrimary);
	if (!done.empty() && titleWidth + doneWidth <= availableWidth)
	{
		const int smallHeight = iV_GetTextLineSize(font_small);
		drawLine(countText, done, font_small, textX + titleWidth + 8, rect.y() + (HEADER_HEIGHT - smallHeight) / 2 + 1, doneWidth, colors.textSecondary);
	}
	drawDivider(rect.x() + 10, rect.y() + HEADER_HEIGHT, rect.right() - 10, rect.y() + HEADER_HEIGHT + 1);
}

// MARK: - Shared

std::string hudObjectivesTitle()
{
	const std::string &title = objectivesTitle();
	return title.empty() ? std::string(_("Objectives")) : title;
}

std::string hudObjectivesDoneText(const std::vector<Objective> &objectives)
{
	unsigned total = 0;
	unsigned done = 0;
	for (const Objective &objective : objectives)
	{
		if (objective.options.primary)
		{
			++total;
			done += objective.options.state == ObjectiveState::Done ? 1 : 0;
		}
	}
	return total > 1 ? astringf(_("%u of %u done"), done, total) : std::string();
}

std::string hudObjectiveIcon(const Objective &objective)
{
	switch (objective.options.state)
	{
	case ObjectiveState::Done: return "objective-done";
	case ObjectiveState::Failed: return "objective-failed";
	case ObjectiveState::Active: break;
	}
	return objective.options.primary ? "objective-active" : "objective-secondary";
}

PIELIGHT hudObjectiveColor(const Objective &objective)
{
	const HudColors &colors = skin().colors();
	switch (objective.options.state)
	{
	case ObjectiveState::Done: return colors.stateOk;
	case ObjectiveState::Failed: return colors.stateBad;
	case ObjectiveState::Active: break;
	}
	return objective.options.primary ? colors.stateWarn : colors.textSecondary;
}

const VIEW_PROXIMITY *placeOf(const Objective &objective)
{
	if (objective.options.marker.empty())
	{
		return nullptr;
	}
	const VIEWDATA *viewData = getViewData(WzString::fromUtf8(objective.options.marker));
	return viewData != nullptr && viewData->type == VIEW_PROX ? static_cast<const VIEW_PROXIMITY *>(viewData->pData) : nullptr;
}

bool hudObjectiveHasPlace(const Objective &objective)
{
	return placeOf(objective) != nullptr;
}

void hudShowObjectiveOnMap(const Objective &objective)
{
	const VIEW_PROXIMITY *place = placeOf(objective);
	if (place == nullptr)
	{
		return;
	}
	if (hudIntelScreenIsOpen())
	{
		intResetScreen(false);
	}
	setViewPos(map_coord(place->x), map_coord(place->y), true);
}

std::string hudObjectiveKey(const Objective &objective)
{
	return objective.id + "/" + std::to_string(objective.options.player);
}

std::vector<std::string> hudWrapText(const std::string &text, iV_fonts font, int width, size_t maxLines)
{
	std::vector<std::string> lines;
	std::string line;
	size_t start = 0;
	while (start < text.size())
	{
		size_t end = text.find(' ', start);
		if (end == std::string::npos)
		{
			end = text.size();
		}
		const std::string word = text.substr(start, end - start);
		start = end + 1;
		if (word.empty())
		{
			continue;
		}
		const std::string candidate = line.empty() ? word : line + " " + word;
		if (line.empty() || static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(candidate), font)) <= width || lines.size() + 1 >= maxLines)
		{
			line = candidate;
			continue;
		}
		lines.push_back(line);
		line = word;
	}
	if (!line.empty() || lines.empty())
	{
		lines.push_back(line);
	}
	return lines;
}

// MARK: - The docked card (the Objectives module)

namespace
{

class ObjectivesDock;
class ObjectivesModule;
std::weak_ptr<ObjectivesDock> currentDock;
ObjectivesModule *currentModule = nullptr;
void noteChanges();

/// The minimized card: a chip with the objectives' icon, and a dot while something changed unseen
class ObjectivesChip : public W_BUTTON
{
public:
	bool dot = false;
	uint32_t appearedAt = 0;

protected:
	void display(int xOffset, int yOffset) override
	{
		WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		// (It pops in, from the card's corner)
		if (animated() && realTime - appearedAt < CHIP_POP_MS)
		{
			const float scale = 0.6f + 0.4f * easeOut(static_cast<float>(realTime - appearedAt) / static_cast<float>(CHIP_POP_MS));
			const int side = static_cast<int>(std::round(static_cast<float>(width()) * scale));
			rect = WzRect(rect.right() - side, rect.y(), side, side);
		}
		const HudColors &colors = skin().colors();
		PIELIGHT fill = colors.panelBackground;
		fill.byte.a = 238;
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   isHighlighted() ? alpha(colors.stateWarn, 60) : fill, skin().metrics().cardRadius, alpha(colors.stateWarn, 220), 1.f);
		const int iconSize = std::min(ICON_SIZE + 2, rect.width() - 8);
		drawIcon("objective", rect.x() + (rect.width() - iconSize) / 2, rect.y() + (rect.height() - iconSize) / 2, iconSize, colors.stateWarn);
		if (dot)
		{
			const float cx = static_cast<float>(rect.right() - 2);
			const float cy = static_cast<float>(rect.y() + 2);
			pie_DrawRoundedBox(cx - 5.f, cy - 5.f, cx + 5.f, cy + 5.f, pal_RGBA(10, 14, 20, 255), 5.f);
			pie_DrawRoundedBox(cx - 4.f, cy - 4.f, cx + 4.f, cy + 4.f, colors.stateWarn, 4.f);
		}
	}
};

/// It rests minimized, and opens when the objectives change
ObjectivesDockState defaultState()
{
	return ObjectivesDockState::Minimized;
}

/// The state a click on the chip opens to when the card hasn't been open yet: the whole card (one line at the compact
/// size)
ObjectivesDockState defaultOpenState()
{
	return compactLayout() ? ObjectivesDockState::OneLine : ObjectivesDockState::Expanded;
}

/// How open a state is: minimized 0, one line 1, expanded 2
int openness(ObjectivesDockState state)
{
	return state == ObjectivesDockState::Expanded ? 2 : (state == ObjectivesDockState::OneLine ? 1 : 0);
}

/// Opens the docked card at least to `state` for `ms`
void openFor(ObjectivesDockState state, uint32_t ms)
{
	if (opening.remainingMs == 0 || openness(state) > openness(opening.state))
	{
		opening.state = state;
		opening.remainingMs = ms;
		return;
	}
	opening.remainingMs = std::max(opening.remainingMs, ms);
}

ObjectivesDockState rememberedState()
{
	return chosenState.value_or(defaultState());
}

class ObjectivesDock : public HudZoneForm
{
public:
	void initialize()
	{
		// (So it takes the clicks over it)
		style &= ~WFORM_INVISIBLE;
		setTransparentToMouse(false);
		std::weak_ptr<ObjectivesDock> weakSelf = std::static_pointer_cast<ObjectivesDock>(shared_from_this());
		card = ObjectivesCard::make(ObjectivesCardMode::Docked);
		card->onToggle = [weakSelf]() {
			if (auto self = weakSelf.lock())
			{
				self->toggle();
			}
		};
		card->onMinimize = [weakSelf]() {
			if (auto self = weakSelf.lock())
			{
				self->setDockState(ObjectivesDockState::Minimized);
			}
		};
		attach(card);
		chip = std::make_shared<ObjectivesChip>();
		chip->setTip(_("Objectives"));
		chip->addOnClickHandler([weakSelf](W_BUTTON &) {
			widgScheduleTask([weakSelf]() {
				if (auto self = weakSelf.lock())
				{
					self->setDockState(self->beforeMinimized);
				}
			});
		});
		attach(chip);
		chip->hide();
		// (It unfolds from one line when the first objectives come)
		motion.reset(ONE_LINE_HEIGHT);
		seenAttention = attentionSerial;
		lastTick = realTime;
		// (Something changed while there was no card, ex. the intelligence screen was up: an opening still to come)
		const auto objectives = localObjectives();
		chip->dot = rememberedState() == ObjectivesDockState::Minimized && opening.remainingMs == 0
			&& std::any_of(objectives.begin(), objectives.end(), highlighted);
		refresh();
	}

	void toggle()
	{
		setDockState(shownState == ObjectivesDockState::Expanded ? ObjectivesDockState::OneLine : ObjectivesDockState::Expanded);
	}

	void setDockState(ObjectivesDockState state)
	{
		const ObjectivesDockState current = rememberedState();
		if (state == ObjectivesDockState::Minimized && current != ObjectivesDockState::Minimized)
		{
			beforeMinimized = current;
		}
		if (state != ObjectivesDockState::Minimized)
		{
			chip->dot = false;
		}
		// (The player's choice: the card stays in it)
		opening.remainingMs = 0;
		chosenState = state;
		refresh();
	}

	void display(int, int) override
	{
		// (The card and the chip draw themselves. The form's own background would show around a chip popping in)
	}

	/// For the geometry dump: a state, without an opening's or the fold's history
	void setStateForTests(ObjectivesDockState state)
	{
		beforeMinimized = defaultOpenState();
		setDockState(state);
	}

	void run(W_CONTEXT *) override
	{
		if (seenAttention != attentionSerial)
		{
			seenAttention = attentionSerial;
			if (rememberedState() == ObjectivesDockState::Minimized && opening.remainingMs == 0)
			{
				chip->dot = true;
			}
		}
		// (An opening's time counts while it's on screen, and not while the game is paused (ex. the guide is up) or
		// the pointer is on it)
		const uint32_t now = realTime;
		const uint32_t elapsed = std::min<uint32_t>(now - lastTick, 1000);
		lastTick = now;
		if (opening.remainingMs > 0 && !openingHeldForTests && !gamePaused())
		{
			const WzRect rect(screenPosX(), screenPosY(), width(), height());
			if (!rect.contains(mouseX(), mouseY()))
			{
				opening.remainingMs = opening.remainingMs > elapsed ? opening.remainingMs - elapsed : 0;
			}
		}
		refresh();
	}

	/// Its state (folded to fit above the radar), and its size: the card unfolds and folds (from and to one line, and
	/// to its chip before the chip pops in)
	void refresh()
	{
		ObjectivesDockState state = rememberedState();
		if (opening.remainingMs > 0 && openness(opening.state) > openness(state))
		{
			state = opening.state;
		}
		const int w = cardWidth();
		const int availableHeight = hudRightColumnAvailableHeight(HudRightColumnSlot::Objectives);
		card->setGeometry(0, 0, w, std::max(1, card->height()));
		if (state == ObjectivesDockState::Expanded && card->expandedHeight() > availableHeight)
		{
			state = ObjectivesDockState::OneLine;
		}
		if (state == ObjectivesDockState::OneLine && ONE_LINE_HEIGHT > availableHeight)
		{
			state = ObjectivesDockState::Minimized;
		}
		shownState = state;

		int newWidth = CHIP_SIZE;
		int newHeight = CHIP_SIZE;
		if (state != ObjectivesDockState::Minimized && !card->visible())
		{
			// (From the chip: the card starts as one line)
			chip->hide();
			card->show();
			card->setOneLine(true);
			motion.reset(ONE_LINE_HEIGHT);
		}
		if (card->visible())
		{
			const int target = state == ObjectivesDockState::Expanded ? card->expandedHeight() : ONE_LINE_HEIGHT;
			if (state == ObjectivesDockState::Expanded)
			{
				card->setOneLine(false);
			}
			const int h = motion.update(target);
			if (!motion.moving() && state == ObjectivesDockState::OneLine)
			{
				card->setOneLine(true);
			}
			card->setGeometry(0, 0, w, h);
			newWidth = w;
			newHeight = h;
			if (state == ObjectivesDockState::Minimized && !motion.moving())
			{
				card->hide();
				chip->show();
				chip->appearedAt = realTime;
			}
		}
		if (chip->visible())
		{
			chip->setGeometry(0, 0, CHIP_SIZE, CHIP_SIZE);
			newWidth = CHIP_SIZE;
			newHeight = CHIP_SIZE;
		}
		if (width() != newWidth || height() != newHeight)
		{
			setGeometry(x() + width() - newWidth, y(), newWidth, newHeight);
			hudLayoutRightColumn();
		}
	}

private:
	std::shared_ptr<ObjectivesCard> card;
	std::shared_ptr<ObjectivesChip> chip;
	ObjectivesDockState shownState = ObjectivesDockState::Expanded;
	ObjectivesDockState beforeMinimized = defaultOpenState();
	HeightMotion motion;
	uint32_t lastTick = 0;
	uint32_t seenAttention = 0;
};

class ObjectivesModule : public HudModule
{
public:
	HudModuleId moduleId() const override { return HudModuleId::Objectives; }

	~ObjectivesModule() override
	{
		if (currentModule == this)
		{
			currentModule = nullptr;
		}
	}

	void create(const std::shared_ptr<W_FORM> &newRoot) override
	{
		root = newRoot;
		currentModule = this;
		update();
	}

	void destroy() override
	{
		remove();
		root.reset();
		if (currentModule == this)
		{
			currentModule = nullptr;
		}
	}

	void setVisible(bool visible) override
	{
		shown = visible;
		update();
	}

	void update() override
	{
		auto strongRoot = root.lock();
		// (Not while the intelligence screen, which pins them, is up)
		// (Nor once the mission is over: its results are up)
		const bool wanted = strongRoot && shown && hudIntelScreenEnabled() && !hudIntelScreenIsOpen() && !MissionResUp && selectedPlayer < MAX_PLAYERS
			&& objectivesAny(selectedPlayer);
		if (!wanted)
		{
			remove();
			return;
		}
		if (!dock)
		{
			// (Created when there are objectives: most games have none, and then nothing of it exists)
			dock = std::make_shared<ObjectivesDock>();
			dock->id = IDHUD_OBJECTIVES;
			dock->setHelp(WidgetHelp()
				.setTitle(_("Objectives"))
				.setDescription(_("What the mission asks of you. Click the header to fold it to one line, or minimize it to a chip.")));
			strongRoot->attach(dock);
			dock->initialize();
			hudRightColumnSet(HudRightColumnSlot::Objectives, dock);
			currentDock = dock;
		}
		hudLayoutRightColumn();
	}

private:
	void remove()
	{
		if (!dock)
		{
			return;
		}
		if (auto parent = dock->parent())
		{
			parent->detach(dock);
		}
		hudRightColumnSet(HudRightColumnSlot::Objectives, nullptr);
		dock.reset();
		currentDock.reset();
	}

	std::weak_ptr<W_FORM> root;
	std::shared_ptr<ObjectivesDock> dock;
	bool shown = true;
};

// MARK: - The pop-up

/// The pop-up's bottom left: over the reticule (or the screen's bottom left)
glm::ivec2 popUpCorner()
{
	if (WIDGET *reticule = psWScreen ? widgGetFromID(psWScreen, IDRET_FORM) : nullptr)
	{
		if (reticule->visible())
		{
			return glm::ivec2(reticule->screenPosX() + 10, reticule->screenPosY() - POP_UP_GAP);
		}
	}
	return glm::ivec2(12, static_cast<int>(screenHeight) - 12);
}

/// Where the pop-up can show, and a little around it
WzRect popUpArea()
{
	const glm::ivec2 corner = popUpCorner();
	const int top = std::max(0, corner.y - 400);
	return WzRect(std::max(0, corner.x - 16), top, cardWidth() + 32, corner.y - top);
}

/// Nothing could be under the pop-up: no screen over the game, and no panel there (the build, research and
/// manufacture lists and the orders panel open there, and the row of factories, labs or trucks can reach it)
bool popUpPlaceFree()
{
	if (hudIntelScreenIsOpen() || MissionResUp)
	{
		return false;
	}
	if (intMode == INT_NORMAL)
	{
		return true;
	}
	if (intMode != INT_OBJECT)
	{
		return false;
	}
	WIDGET *objects = psWScreen ? widgGetFromID(psWScreen, IDOBJ_FORM) : nullptr;
	return objects == nullptr || !objects->visible() || !objects->screenGeometry().intersects(popUpArea());
}

/// The pop-up can show: its place free, and the player not about to click there or dragging
bool popUpAreaClear()
{
	if (!popUpPlaceFree() || mouseDown(MOUSE_LMB) || mouseDown(MOUSE_RMB) || mouseDown(MOUSE_MMB))
	{
		return false;
	}
	return !popUpArea().contains(mouseX(), mouseY());
}

class PopUpLink : public W_BUTTON
{
protected:
	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const PIELIGHT color = isHighlighted() ? skin().colors().textPrimary : skin().colors().accent;
		drawIcon("intelligence", rect.x(), rect.y() + (rect.height() - 14) / 2, 14, color);
		drawLine(text, _("Open intelligence"), font_small, rect.x() + 20, rect.y() + (rect.height() - iV_GetTextLineSize(font_small)) / 2, rect.width() - 20, color);
	}

private:
	WzText text;
};

class ObjectivesPopUp : public HudZoneForm
{
public:
	void initialize(uint32_t ms)
	{
		durationMs = ms;
		style &= ~WFORM_INVISIBLE;
		setTransparentToMouse(false);
		card = ObjectivesCard::make(ObjectivesCardMode::PopUp);
		attach(card);
		link = std::make_shared<PopUpLink>();
		link->addOnClickHandler([](W_BUTTON &) {
			widgScheduleTask([]() {
				widgDelete(psWScreen, IDHUD_OBJECTIVES_POP_UP);
				hudIntelScreenShowEntry(INTEL_OBJECTIVES_KIND, std::string());
			});
		});
		attach(link);
		// (It unfolds up out of the intelligence button)
		motion.reset(HEADER_HEIGHT);
		restart(durationMs);
	}

	void restart(uint32_t ms)
	{
		durationMs = ms;
		shownTime = 0;
		lastTick = realTime;
		leaving = false;
		place();
	}

	void place()
	{
		const int w = cardWidth();
		const glm::ivec2 corner = popUpCorner();
		const int left = corner.x;
		const int bottom = corner.y;
		card->setGeometry(0, 0, w, std::max(1, card->height()));
		const int maxCard = std::max(HEADER_HEIGHT, bottom - 10 - POP_UP_FOOTER);
		const int cardHeight = motion.update(leaving ? HEADER_HEIGHT : std::min(card->fullHeight(), maxCard));
		card->setGeometry(0, 0, w, cardHeight);
		link->setGeometry(12, cardHeight + 6, 150, POP_UP_FOOTER - 10);
		const int h = cardHeight + POP_UP_FOOTER;
		setGeometry(std::max(0, std::min(left, static_cast<int>(screenWidth) - w)), std::max(0, bottom - h), w, h);
	}

	void run(W_CONTEXT *) override
	{
		const uint32_t now = realTime;
		const WzRect rect(screenPosX(), screenPosY(), width(), height());
		// (Not while the pointer is on it, nor while the game is paused, ex. the guide is up)
		if (!rect.contains(mouseX(), mouseY()) && !popUpHeldForTests && !gamePaused())
		{
			shownTime += now - lastTick;
		}
		lastTick = now;
		if (MissionResUp)
		{
			widgScheduleTask([]() { widgDelete(psWScreen, IDHUD_OBJECTIVES_POP_UP); });
			return;
		}
		if (!leaving && !popUpHeldForTests && !popUpPlaceFree())
		{
			// (A panel opened where it is: it leaves at once, and the button's dot stays)
			leaving = true;
		}
		if (shownTime >= durationMs && !leaving)
		{
			// (Shown in full, so it's been seen. It folds back down, then goes)
			leaving = true;
			unseen = false;
		}
		place();
		if (leaving && !motion.moving())
		{
			widgScheduleTask([]() { widgDelete(psWScreen, IDHUD_OBJECTIVES_POP_UP); });
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		PIELIGHT fill = colors.panelBackground;
		fill.byte.a = 245;
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   fill, skin().metrics().cardRadius, alpha(colors.stateWarn, 200), 1.f);
		const int footerTop = rect.y() + card->height();
		drawDivider(rect.x() + 1, footerTop, rect.right() - 1, footerTop + 1);
		const uint32_t left = durationMs > shownTime ? durationMs - shownTime : 0;
		const unsigned seconds = static_cast<unsigned>((left + 999) / 1000);
		drawLine(foldText, astringf(_("folds away in %u s"), seconds), font_small, rect.right() - 12, footerTop + 9, 130, colors.textSecondary, true);
		const float fraction = static_cast<float>(left) / static_cast<float>(std::max<uint32_t>(durationMs, 1));
		pie_BoxFill(rect.x() + 8, rect.bottom() - 4, rect.x() + 8 + static_cast<int>(static_cast<float>(rect.width() - 16) * fraction), rect.bottom() - 2,
		            alpha(colors.stateWarn, 200));
		// (Pointing down at the intelligence button)
		const int ax = rect.x() + 30;
		for (int i = 0; i < 6; ++i)
		{
			pie_BoxFill(ax - 6 + i, rect.bottom() + i, ax + 6 - i, rect.bottom() + i + 1, alpha(colors.stateWarn, 200));
		}
	}

private:
	std::shared_ptr<ObjectivesCard> card;
	std::shared_ptr<PopUpLink> link;
	HeightMotion motion;
	bool leaving = false;
	uint32_t durationMs = OPEN_CHANGED_MS;
	uint32_t shownTime = 0;
	uint32_t lastTick = 0;
	WzText foldText;
};

void showPopUp(uint32_t ms)
{
	if (psWScreen == nullptr || selectedPlayer >= MAX_PLAYERS || !objectivesAny(selectedPlayer) || !hudIntelScreenEnabled() || hudIntelScreenIsOpen()
	    || MissionResUp)
	{
		return;
	}
	if (auto *popUp = dynamic_cast<ObjectivesPopUp *>(widgGetFromID(psWScreen, IDHUD_OBJECTIVES_POP_UP)))
	{
		popUp->restart(ms);
		return;
	}
	auto popUp = std::make_shared<ObjectivesPopUp>();
	popUp->id = IDHUD_OBJECTIVES_POP_UP;
	popUp->setHelp(WidgetHelp()
		.setTitle(_("Objectives"))
		.setDescription(_("The objectives changed. To keep them on screen, choose the card in the HUD's options.")));
	psWScreen->psForm->attach(popUp);
	popUp->initialize(ms);
}

/// A change or a reminder (`keys`: what changed) for the Pop-up and Off choices of the Objectives option: queues the
/// pop-up until its place is clear, or only shows the intelligence button's dot (with Card, openFor() opens the card)
void popUpFor(uint32_t ms, const std::vector<std::string> &keys)
{
	const HudObjectivesShow show = hudObjectivesShow();
	if (show == HudObjectivesShow::Card)
	{
		return;
	}
	unseen = true;
	if (show == HudObjectivesShow::Off)
	{
		return;
	}
	if (!pendingPopUp.waiting)
	{
		pendingPopUp = PendingPopUp();
		pendingPopUp.waiting = true;
		pendingPopUp.since = realTime;
	}
	// (Changes while it waits: one pop-up with all of them)
	pendingPopUp.ms = std::max(pendingPopUp.ms, ms);
	for (const std::string &key : keys)
	{
		if (std::find(pendingPopUp.keys.begin(), pendingPopUp.keys.end(), key) == pendingPopUp.keys.end())
		{
			pendingPopUp.keys.push_back(key);
		}
	}
}

/// A waiting pop-up shows once its place has been clear for a moment (its changes highlighted again from then), or is
/// dropped when stale. Opening the intelligence screen counts as seeing the changes
void runPopUp()
{
	if (hudIntelScreenIsOpen())
	{
		unseen = false;
		pendingPopUp = PendingPopUp();
		return;
	}
	if (!pendingPopUp.waiting)
	{
		return;
	}
	const uint32_t now = realTime;
	if (hudObjectivesShow() != HudObjectivesShow::PopUp || now - pendingPopUp.since > POP_UP_STALE_MS)
	{
		pendingPopUp = PendingPopUp();
		return;
	}
	const bool clear = popUpAreaClear();
	if (clear && !popUpWasClear)
	{
		popUpClearSince = now;
	}
	popUpWasClear = clear;
	if (!clear || now - popUpClearSince < POP_UP_SETTLE_MS)
	{
		return;
	}
	for (const std::string &key : pendingPopUp.keys)
	{
		auto it = marks.find(key);
		if (it != marks.end())
		{
			it->second.highlightUntil = std::max(it->second.highlightUntil, now + HIGHLIGHT_MS);
			if (it->second.newUntil > pendingPopUp.since)
			{
				it->second.newUntil = std::max(it->second.newUntil, now + NEW_MS);
			}
		}
	}
	const uint32_t ms = pendingPopUp.ms;
	pendingPopUp = PendingPopUp();
	showPopUp(ms);
}

void remind()
{
	const uint32_t now = realTime;
	for (const Objective &objective : localObjectives())
	{
		if (objective.options.state == ObjectiveState::Active)
		{
			marks[hudObjectiveKey(objective)].highlightUntil = now + HIGHLIGHT_MS;
		}
	}
	++attentionSerial;
	openFor(ObjectivesDockState::Expanded, OPEN_NEW_MS);
	std::vector<std::string> keys;
	for (const Objective &objective : localObjectives())
	{
		if (objective.options.state == ObjectiveState::Active)
		{
			keys.push_back(hudObjectiveKey(objective));
		}
	}
	popUpFor(OPEN_NEW_MS, keys);
}

/// What the HUD has seen of the objectives (no mark: they aren't news)
void noteSeen(const std::vector<Objective> &objectives)
{
	seen.clear();
	for (const Objective &objective : objectives)
	{
		seen[hudObjectiveKey(objective)] = Seen{objective.text, objective.options.state, objective.options.progress};
	}
	seenRevision = objectivesRevision();
	seenGameTime = gameTime;
	seenValid = true;
}

}

std::unique_ptr<HudModule> hudMakeObjectivesCard()
{
	return std::make_unique<ObjectivesModule>();
}

void hudObjectivesInitialize()
{
	ObjectivesDisplay display;
	display.shows = []() { return hudIntelScreenEnabled(); };
	display.bringToAttention = []() { remind(); };
	objectivesSetDisplay(std::move(display));
}

void hudObjectivesShutdown()
{
	objectivesSetDisplay(ObjectivesDisplay());
	marks.clear();
}

void hudObjectivesLevelStarts(bool fromSave)
{
	seen.clear();
	marks.clear();
	opening = Opening();
	chosenState.reset();
	pendingPopUp = PendingPopUp();
	unseen = false;
	seenValid = false;
	quiet = fromSave;
}

bool hudObjectivesUnseen()
{
	// (Not while the pop-up shows them)
	return unseen && hudObjectivesShow() != HudObjectivesShow::Card
	       && (psWScreen == nullptr || widgGetFromID(psWScreen, IDHUD_OBJECTIVES_POP_UP) == nullptr);
}

void hudObjectivesFrame()
{
	noteChanges();
	runPopUp();
	if (currentModule != nullptr)
	{
		currentModule->update();
	}
}

namespace
{

void noteChanges()
{
	if (seenValid && seenRevision == objectivesRevision())
	{
		return;
	}
	const std::vector<Objective> objectives = localObjectives();
	// (A save's objectives, or a restored snapshot's: time went back)
	if (quiet || !hudIntelScreenEnabled() || (seenValid && gameTime < seenGameTime))
	{
		quiet = false;
		noteSeen(objectives);
		return;
	}
	const uint32_t now = realTime;
	bool changed = false;
	size_t added = 0;
	std::map<std::string, Mark> kept;
	std::vector<std::string> changedKeys;
	for (const Objective &objective : objectives)
	{
		const std::string key = hudObjectiveKey(objective);
		Mark mark = marks.count(key) != 0 ? marks[key] : Mark();
		auto it = seen.find(key);
		if (it == seen.end())
		{
			mark.highlightUntil = now + HIGHLIGHT_MS;
			mark.newUntil = now + NEW_MS;
			changed = true;
			changedKeys.push_back(key);
			++added;
		}
		else if (it->second.text != objective.text || it->second.state != objective.options.state || it->second.progress != objective.options.progress)
		{
			mark.highlightUntil = now + HIGHLIGHT_MS;
			changed = true;
			changedKeys.push_back(key);
		}
		kept[key] = mark;
	}
	marks = std::move(kept);
	noteSeen(objectives);
	if (changed)
	{
		if (added > 0)
		{
			// (A second longer for each row past four)
			const uint32_t extra = static_cast<uint32_t>(objectives.size() > 4 ? objectives.size() - 4 : 0) * 1000;
			const uint32_t ms = std::min(OPEN_NEW_MS + extra, OPEN_NEW_MAX_MS);
			openFor(ObjectivesDockState::Expanded, ms);
			popUpFor(ms, changedKeys);
		}
		else
		{
			openFor(ObjectivesDockState::OneLine, OPEN_CHANGED_MS);
			popUpFor(OPEN_CHANGED_MS, changedKeys);
		}
		++attentionSerial;
	}
}

}

bool hudObjectivesApplyForTests(const std::string &setting)
{
	const size_t equals = setting.find('=');
	const std::string key = setting.substr(0, equals);
	const std::string value = equals == std::string::npos ? std::string() : setting.substr(equals + 1);
	setForTests = true;
	if (key == "sample")
	{
		// (Campaign 2's Transport Down, as a script would set it: one done, one just added with a place, a count, and a
		// secondary one)
		ObjectiveOptions options;
		options.state = ObjectiveState::Done;
		objectivesSet("rescue", _("Locate and rescue your units from the shot down transporter"), options);
		options = ObjectiveOptions();
		options.marker = value;
		objectivesSet("lz", _("Return to LZ"), options);
		options = ObjectiveOptions();
		options.progress = std::make_pair(1, 3);
		objectivesSet("bases", _("Bases destroyed"), options);
		options = ObjectiveOptions();
		options.primary = false;
		objectivesSet("artifact", _("Recover the artifact"), options);
		noteSeen(localObjectives());
		marks.clear();
		return true;
	}
	if (key == "card")
	{
		const ObjectivesDockState state = value == "line" ? ObjectivesDockState::OneLine
			: (value == "minimized" ? ObjectivesDockState::Minimized : ObjectivesDockState::Expanded);
		chosenState = state;
		if (auto dock = currentDock.lock())
		{
			dock->setStateForTests(state);
		}
		return true;
	}
	if (key == "changed")
	{
		const uint32_t now = realTime;
		for (const Objective &objective : localObjectives())
		{
			if (objective.id == value)
			{
				marks[hudObjectiveKey(objective)] = Mark{now + 60000, now + 60000};
				++attentionSerial;
				return true;
			}
		}
		return false;
	}
	if (key == "remind")
	{
		openingHeldForTests = true;
		objectivesShow(static_cast<int32_t>(selectedPlayer));
		for (auto &mark : marks)
		{
			mark.second.highlightUntil = std::max(mark.second.highlightUntil, mark.second.highlightUntil != 0 ? realTime + 60000 : 0u);
		}
		return true;
	}
	if (key == "popup")
	{
		popUpHeldForTests = true;
		showPopUp(OPEN_CHANGED_MS);
		return psWScreen != nullptr && widgGetFromID(psWScreen, IDHUD_OBJECTIVES_POP_UP) != nullptr;
	}
	return false;
}

void hudObjectivesResetForTests()
{
	if (psWScreen != nullptr && widgGetFromID(psWScreen, IDHUD_OBJECTIVES_POP_UP) != nullptr)
	{
		widgDelete(psWScreen, IDHUD_OBJECTIVES_POP_UP);
	}
	marks.clear();
	opening = Opening();
	pendingPopUp = PendingPopUp();
	unseen = false;
	openingHeldForTests = false;
	popUpHeldForTests = false;
	// (What the game's scripts set stays)
	if (setForTests)
	{
		setForTests = false;
		objectivesClear(nonstd::nullopt);
		chosenState.reset();
	}
	noteSeen(localObjectives());
}
