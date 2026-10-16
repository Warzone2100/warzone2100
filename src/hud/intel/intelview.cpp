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

#include "intelview.h"
#include "intelkit.h"
#include "objectivescard.h"

#include "../hudskin.h"
#include "../hudtext.h"
#include "../hudwidgets.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/sound/audio.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/paragraph.h"
#include "lib/widget/popovermenu.h"
#include "lib/widget/scrollablelist.h"

#include "../../intelmap.h"
#include "../../objectives.h"
#include "../../mapdisplay.h"
#include "../../messagedef.h"
#include "../../research.h"
#include "../../researchtree/researchtracks.h"
#include "../../seqdisp.h"
#include "../../screens/researchtreescreen.h"
#include "../../titleui/widgets/optionsform.h"
#include "../../widgets/detailpanel/research/researchdetail.h"

#include <algorithm>
#include <cctype>

using namespace intelkit;

namespace
{

constexpr int TOOLBAR_HEIGHT = 36;
constexpr int CHIP_HEIGHT = 22;
constexpr int SEARCH_WIDTH = 300;
constexpr int SEARCH_HEIGHT = 26;
constexpr int SCROLLBAR_WIDTH = 9;
constexpr int LIST_WIDTH = 330;
constexpr int LIST_WIDTH_COMPACT = 236;
constexpr int ROW_HEIGHT = 46;
constexpr int ROW_HEIGHT_COMPACT = 44;
constexpr int SECTION_HEIGHT = 20;
constexpr int PINNED_HEIGHT = 58;
constexpr int PINNED_HEIGHT_COMPACT = 40;
constexpr int BUTTON_HEIGHT_COMPACT = 32;
constexpr int THUMB_WIDTH = 40;
constexpr int THUMB_HEIGHT = 34;
constexpr int GAP = 8;

/// The filters, in the order they're offered
constexpr IntelFilter FILTER_ORDER[] = {IntelFilter::All, IntelFilter::Objectives, IntelFilter::Briefings, IntelFilter::Research, IntelFilter::Unread};

const char *filterName(IntelFilter filter)
{
	switch (filter)
	{
	case IntelFilter::All: return N_("All");
	case IntelFilter::Objectives: return N_("Objectives");
	case IntelFilter::Briefings: return N_("Briefings");
	case IntelFilter::Research: return N_("Research");
	case IntelFilter::Unread: return N_("Unread");
	}
	return "";
}

/// (Draws nothing: the search field draws the edit box's background, with its icon)
void displayNothing(WIDGET *, UDWORD, UDWORD) {}

/// The search field: the skin's input field with a search icon, and the edit box in it
class IntelSearchField : public WIDGET
{
public:
	std::shared_ptr<W_EDITBOX> box;

	void initialize(const std::shared_ptr<W_EDITBOX> &editBox)
	{
		box = editBox;
		attach(box);
	}

	void geometryChanged() override
	{
		const int iconWidth = iconSize() + 12;
		box->setGeometry(iconWidth, 0, std::max(0, width() - iconWidth - 6), height());
	}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		skin().drawInputField(rect, box->isEditing());
		const int s = iconSize();
		skin().drawNamedIcon("search", WzRect(rect.x() + 8, rect.y() + (rect.height() - s) / 2, s, s), skin().colors().textSecondary);
	}

private:
	int iconSize() const { return height() >= 30 ? 15 : 13; }
};

/// A section's heading in the list ("New", "Earlier · Transport Down"), with its count
class IntelSection : public WIDGET
{
public:
	IntelSection(const std::string &label, size_t count): labelText(label), countText(std::to_string(count)) {}

	void display(int xOffset, int yOffset) override
	{
		const PIELIGHT color = skin().colors().textSecondary;
		const int top = yOffset + y() + 6;
		drawLine(left, labelText, font_small, xOffset + x() + 6, top, width() - 40, color);
		drawLine(right, countText, font_small, xOffset + x() + width() - 6, top, 40, color, true);
	}

private:
	std::string labelText;
	std::string countText;
	WzText left;
	WzText right;
};

/// A row of the list: the item's picture, title, kind, time and unread mark
class IntelRow : public W_BUTTON
{
public:
	IntelRow(const std::shared_ptr<IntelView> &view, size_t index): view(view), index(index)
	{
		addOnClickHandler([](W_BUTTON &button) {
			auto &row = static_cast<IntelRow &>(button);
			auto strongView = row.view.lock();
			if (strongView)
			{
				strongView->select(row.index, true);
			}
		});
	}

	void display(int xOffset, int yOffset) override
	{
		auto strongView = view.lock();
		const HudIntelEntry *entry = strongView ? strongView->entry(index) : nullptr;
		if (entry == nullptr)
		{
			return;
		}
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool selected = strongView->isSelected(index);
		if (selected)
		{
			skin().drawCard(rect, HudCardState::Selected);
		}
		else if (isHighlighted())
		{
			skin().drawCard(rect, HudCardState::Highlighted);
		}
		drawThumb(*entry, WzRect(rect.x() + 5, rect.y() + (rect.height() - THUMB_HEIGHT) / 2, THUMB_WIDTH, THUMB_HEIGHT));
		const HudColors &colors = skin().colors();
		const int textX = rect.x() + THUMB_WIDTH + 13;
		const std::string time = timeText(*entry);
		timeLine.setText(WzString::fromUtf8(time), font_small);
		const int timeWidth = time.empty() ? 0 : timeLine.width() + 10;
		const int textWidth = rect.right() - 10 - textX - (strongView->isCompact() ? 8 : timeWidth);
		drawLine(title, entry->title, entry->unread && !selected ? font_regular_bold : font_regular, textX, rect.y() + 7, textWidth, colors.textPrimary);
		std::string sub = strongView->subtitle(*entry);
		if (strongView->isCompact() && !time.empty())
		{
			sub += " · " + time;
		}
		drawLine(subtitle, sub, font_small, textX, rect.y() + 25, rect.right() - 12 - textX, colors.textSecondary);
		if (!strongView->isCompact() && !time.empty())
		{
			drawLine(timeLine, time, font_small, rect.right() - 10, rect.y() + 8, timeWidth, colors.textSecondary, true);
		}
		if (entry->unread && !selected)
		{
			drawUnreadDot(rect.right() - 12, rect.y() + (strongView->isCompact() ? 12 : 32));
		}
	}

private:
	std::weak_ptr<IntelView> view;
	size_t index;
	WzText title;
	WzText subtitle;
	WzText timeLine;
};

/// The latest mission briefing, pinned above the list (until the scripts set objectives)
class IntelPinned : public W_BUTTON
{
public:
	IntelPinned(const std::shared_ptr<IntelView> &view, size_t index): view(view), index(index)
	{
		addOnClickHandler([](W_BUTTON &button) {
			auto &card = static_cast<IntelPinned &>(button);
			if (auto strongView = card.view.lock())
			{
				strongView->select(card.index, true);
			}
		});
	}

	void display(int xOffset, int yOffset) override
	{
		auto strongView = view.lock();
		const HudIntelEntry *entry = strongView ? strongView->entry(index) : nullptr;
		if (entry == nullptr)
		{
			return;
		}
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const HudColors &colors = skin().colors();
		const bool selected = strongView->isSelected(index);
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   alpha(colors.stateWarn, selected ? 46 : (isHighlighted() ? 36 : 26)), skin().metrics().cardRadius,
		                   alpha(colors.stateWarn, selected ? 230 : 150), 1.f);
		pie_BoxFill(rect.x() + 1, rect.y() + 6, rect.x() + 4, rect.bottom() - 6, colors.stateWarn);
		skin().drawNamedIcon("objective", WzRect(rect.x() + 11, rect.y() + (strongView->isCompact() ? 12 : 9), 16, 16), colors.stateWarn);
		const int textX = rect.x() + 34;
		const int textWidth = rect.right() - 12 - textX;
		drawLine(kindLine, _("Mission"), font_small, textX, rect.y() + 5, textWidth, colors.stateWarn);
		drawLine(titleLine, entry->title, font_regular_bold, textX, rect.y() + 19, textWidth - (strongView->isCompact() ? 18 : 0), colors.textPrimary);
		if (strongView->isCompact())
		{
			skin().drawNamedIcon("chevron-right", WzRect(rect.right() - 22, rect.y() + 13, 14, 14), colors.textSecondary);
			return;
		}
		if (entry->message != nullptr && entry->message->pViewData != nullptr)
		{
			drawLine(lastLine, lastLineOf(*entry), font_small, textX, rect.y() + 38, textWidth, colors.textSecondary);
		}
	}

private:
	static std::string lastLineOf(const HudIntelEntry &entry)
	{
		const VIEWDATA *viewData = entry.message->pViewData;
		if ((viewData->type == VIEW_RPL || viewData->type == VIEW_RPLX) && viewData->pData != nullptr)
		{
			const auto *replay = static_cast<const VIEW_REPLAY *>(viewData->pData);
			for (auto seq = replay->seqList.rbegin(); seq != replay->seqList.rend(); ++seq)
			{
				if (!seq->textMsg.empty())
				{
					return seq->textMsg.back().toUtf8();
				}
			}
		}
		return viewData->textMsg.empty() ? std::string() : viewData->textMsg.back().toUtf8();
	}

	std::weak_ptr<IntelView> view;
	size_t index;
	WzText kindLine;
	WzText titleLine;
	WzText lastLine;
};

/// Text centered in its rectangle ("No intelligence yet")
class IntelNote : public WIDGET
{
public:
	explicit IntelNote(const std::string &note): note(note) {}

	void display(int xOffset, int yOffset) override
	{
		text.setText(WzString::fromUtf8(note), font_regular);
		const float x0 = static_cast<float>(xOffset + x() + (width() - text.width()) / 2);
		text.render(x0, baselineAt(text, yOffset + y() + height() / 2 - text.lineSize() / 2), skin().colors().textSecondary);
	}

private:
	std::string note;
	WzText text;
};

/// The bottom bar of the item's pane: its buttons on the left, a note on the right
class IntelBar : public WIDGET
{
public:
	void add(const std::shared_ptr<WIDGET> &button)
	{
		buttons.push_back(button);
		attach(button);
	}

	void geometryChanged() override
	{
		int x0 = 0;
		for (auto &button : buttons)
		{
			button->setGeometry(x0, height() - button->height(), button->width(), button->height());
			x0 += button->width() + GAP;
		}
	}

	void display(int xOffset, int yOffset) override
	{
		if (divided)
		{
			drawDivider(xOffset + x(), yOffset + y(), xOffset + x() + width(), yOffset + y() + 1);
		}
	}

	bool divided = true;

private:
	std::vector<std::shared_ptr<WIDGET>> buttons;
};

DetailPanelStyle paneStyle()
{
	DetailPanelStyle style = DetailPanelStyle::researchTree();
	style.background = pal_RGBA(0, 0, 0, 0);
	style.border = pal_RGBA(0, 0, 0, 0);
	style.divider = alpha(skin().colors().panelBorder, 120);
	style.solidWhenPointedAt = false;
	style.drawBackground = [](const WzRect &) {};
	return style;
}

/// A briefing: its kind, title and time, a box to play its videos (when they're installed), its transcript
class IntelBriefingPane : public WIDGET
{
public:
	IntelBriefingPane(const HudIntelEntry &entry, bool compact): entry(entry), compact(compact) {}

	void initialize(const std::function<void ()> &play)
	{
		if (seq_hasVideos() && !compact)
		{
			auto box = std::make_shared<VideoBox>();
			box->addOnClickHandler([play](W_BUTTON &) {
				if (play)
				{
					play();
				}
			});
			box->setTip(_("Play the briefing"));
			videoBox = box;
			attach(box);
		}
		transcript = ScrollableListWidget::make();
		transcript->setItemSpacing(6);
		transcript->setBackgroundColor(pal_RGBA(0, 0, 0, 0));
		attach(transcript);
	}

	void geometryChanged() override
	{
		int top = 0;
		if (videoBox)
		{
			const int boxHeight = std::min(232, width() * 9 / 16);
			videoBox->setGeometry(0, 0, width(), boxHeight);
			top = boxHeight + 12;
		}
		headerTop = top;
		top += 18 + 24 + 12;
		transcript->setGeometry(0, top, width(), std::max(0, height() - top));
		if (transcript->numItems() == 0 && width() > 0)
		{
			fillTranscript();
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y() + headerTop;
		const std::string meta = metaText();
		metaLine.setText(WzString::fromUtf8(meta), font_small);
		const int metaWidth = metaLine.width();
		drawLine(headline, sentenceCase(entry.headlineKind), font_small, x0, y0, width(), colors.stateWarn);
		drawLine(title, entry.title, font_large, x0, y0 + 18, width() - metaWidth - 12, colors.textPrimary);
		drawLine(metaLine, meta, font_small, x0 + width(), y0 + 24, metaWidth + 2, colors.textSecondary, true);
	}

private:
	class VideoBox : public W_BUTTON
	{
	public:
		void display(int xOffset, int yOffset) override
		{
			const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
			const HudColors &colors = skin().colors();
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   pal_RGBA(10, 16, 26, 255), 5.f, isHighlighted() ? colors.accent : alpha(colors.panelBorder, 150), 1.f);
			const int r = std::min(rect.width(), rect.height()) / 6;
			const int cx = rect.x() + rect.width() / 2;
			const int cy = rect.y() + rect.height() / 2;
			pie_DrawRoundedBox(static_cast<float>(cx - r), static_cast<float>(cy - r), static_cast<float>(cx + r), static_cast<float>(cy + r),
			                   pal_RGBA(0, 0, 0, 140), static_cast<float>(r), alpha(colors.textPrimary, 200), 1.f);
			skin().drawNamedIcon("replay-play", WzRect(cx - r / 2 + 2, cy - r / 2, r, r), colors.textPrimary);
		}
	};

	std::string metaText() const
	{
		std::string meta;
		if (!entry.headline.empty())
		{
			meta = entry.kind == HudIntelKind::Update ? _("Update") : _("Mission briefing");
		}
		const std::string time = timeText(entry);
		if (!time.empty())
		{
			meta += meta.empty() ? time : " · " + time;
		}
		return meta;
	}

	void fillTranscript()
	{
		if (entry.message == nullptr || entry.message->pViewData == nullptr)
		{
			return;
		}
		std::vector<WzString> lines;
		const VIEWDATA *viewData = entry.message->pViewData;
		if ((viewData->type == VIEW_RPL || viewData->type == VIEW_RPLX) && viewData->pData != nullptr)
		{
			for (const SEQ_DISPLAY &seq : static_cast<const VIEW_REPLAY *>(viewData->pData)->seqList)
			{
				lines.insert(lines.end(), seq.textMsg.begin(), seq.textMsg.end());
			}
		}
		else
		{
			lines = viewData->textMsg;
		}
		for (const WzString &line : lines)
		{
			auto paragraph = std::make_shared<Paragraph>();
			paragraph->setFont(compact ? font_small : font_regular);
			paragraph->setFontColour(skin().colors().textPrimary);
			paragraph->addText(line);
			paragraph->setGeometry(0, 0, width() - 14, 0);
			transcript->addItem(paragraph);
		}
	}

	HudIntelEntry entry;
	bool compact;
	int headerTop = 0;
	std::shared_ptr<W_BUTTON> videoBox;
	std::shared_ptr<ScrollableListWidget> transcript;
	WzText headline;
	WzText title;
	WzText metaLine;
};

/// An objective: its state and text, its tags (primary or secondary, its state, when it was added), its progress, the
/// mission's objectives with when each last changed, and the briefing they came with
class IntelObjectivePane : public WIDGET
{
public:
	IntelObjectivePane(const Objective &objective, bool compact): objective(objective), compact(compact) {}

	void initialize(const HudIntelEntry *briefing, std::function<void ()> openBriefing)
	{
		all = objectivesFor(selectedPlayer);
		if (briefing != nullptr)
		{
			auto link = std::make_shared<BriefingLink>(*briefing);
			link->addOnClickHandler([openBriefing](W_BUTTON &) {
				if (openBriefing)
				{
					widgScheduleTask(openBriefing);
				}
			});
			briefingLink = link;
			attach(link);
		}
	}

	void geometryChanged() override
	{
		titleLines = hudWrapText(objective.text, titleFont(), std::max(80, width() - TITLE_X), 3);
		if (briefingLink)
		{
			const int top = briefingTop();
			briefingLink->setGeometry(0, top + SECTION_LABEL, width(), compact ? 40 : 58);
			briefingLink->show(top + SECTION_LABEL + briefingLink->height() <= height());
		}
	}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const iV_fonts font = titleFont();
		const int lineHeight = iV_GetTextLineSize(font);
		const PIELIGHT stateColor = hudObjectiveColor(objective);
		const int iconSize = compact ? 18 : 22;
		skin().drawNamedIcon(hudObjectiveIcon(objective), WzRect(x0, y0 + (lineHeight - iconSize) / 2, iconSize, iconSize), stateColor);
		titleTexts.resize(titleLines.size());
		for (size_t i = 0; i < titleLines.size(); ++i)
		{
			drawLine(titleTexts[i], titleLines[i], font, x0 + TITLE_X, y0 + static_cast<int>(i) * lineHeight, width() - TITLE_X, colors.textPrimary);
		}
		int y = y0 + static_cast<int>(titleLines.size()) * lineHeight + 8;

		const int tagCenter = y + 10;
		int tagX = x0 + TITLE_X;
		const std::string kind = objective.options.primary ? _("Primary objective") : _("Secondary objective");
		tagX += drawTag(tags[0], kind, tagX, tagCenter, objective.options.primary ? colors.stateWarn : colors.textSecondary, false) + 6;
		tagX += drawTag(tags[1], stateName(objective.options.state), tagX, tagCenter, objective.options.state == ObjectiveState::Active ? colors.textSecondary : stateColor, false) + 6;
		const std::string added = astringf(_("Added at %s"), hudFormatDuration(objective.addedAt / GAME_TICKS_PER_SEC).c_str());
		if (tagX + tagWidth(tags[2], added) <= x0 + width())
		{
			drawTag(tags[2], added, tagX, tagCenter, colors.textSecondary, false);
		}
		y += 28;

		if (objective.options.progress.has_value())
		{
			const auto &progress = *objective.options.progress;
			const std::string count = astringf(_("%d of %d"), progress.first, progress.second);
			const int barX = x0 + TITLE_X + drawLine(progressLine, count, font_regular_bold, x0 + TITLE_X, y, 120, colors.textPrimary) + 12;
			const int barWidth = std::min(200, x0 + width() - barX);
			const int barY = y + iV_GetTextLineSize(font_regular_bold) / 2 - 2;
			const float fraction = std::max(0.f, std::min(1.f, static_cast<float>(progress.first) / static_cast<float>(progress.second)));
			if (barWidth > 20)
			{
				pie_DrawRoundedBox(static_cast<float>(barX), static_cast<float>(barY), static_cast<float>(barX + barWidth), static_cast<float>(barY + 4),
				                   alpha(colors.textSecondary, 70), 2.f);
				pie_DrawRoundedBox(static_cast<float>(barX), static_cast<float>(barY), static_cast<float>(barX) + static_cast<float>(barWidth) * fraction,
				                   static_cast<float>(barY + 4), stateColor, 2.f);
			}
			y += 26;
		}
		drawDivider(x0, y, x0 + width(), y + 1);
		y += 12;

		drawLine(sectionText, _("This mission"), font_regular_bold, x0, y, width(), colors.stateWarn);
		y += SECTION_LABEL;
		const int rowHeight = compact ? 24 : 28;
		listTexts.resize(all.size());
		timeTexts.resize(all.size());
		for (size_t i = 0; i < all.size() && y + rowHeight <= y0 + briefingTop(); ++i)
		{
			const Objective &item = all[i];
			const bool current = item.id == objective.id && item.options.player == objective.options.player;
			skin().drawNamedIcon(hudObjectiveIcon(item), WzRect(x0 + 2, y + (rowHeight - 16) / 2, 16, 16), hudObjectiveColor(item));
			const std::string when = timeOf(item);
			timeTexts[i].setText(WzString::fromUtf8(when), font_small);
			const int whenWidth = when.empty() ? 0 : timeTexts[i].width() + 12;
			std::string text = item.text;
			if (item.options.progress.has_value())
			{
				text += astringf("  %d/%d", item.options.progress->first, item.options.progress->second);
			}
			const iV_fonts rowFont = current ? font_regular_bold : font_regular;
			const int textTop = y + (rowHeight - iV_GetTextLineSize(rowFont)) / 2;
			drawLine(listTexts[i], text, rowFont, x0 + 26, textTop, width() - 26 - whenWidth,
			         item.options.state == ObjectiveState::Active ? colors.textPrimary : colors.textSecondary);
			if (!when.empty())
			{
				drawLine(timeTexts[i], when, font_small, x0 + width(), y + (rowHeight - iV_GetTextLineSize(font_small)) / 2, whenWidth, colors.textSecondary, true);
			}
			y += rowHeight;
		}

		if (briefingLink && briefingLink->visible())
		{
			drawLine(briefingText, _("From the briefing"), font_regular_bold, x0, y0 + briefingTop(), width(), colors.stateWarn);
		}
	}

private:
	static constexpr int TITLE_X = 32;
	static constexpr int SECTION_LABEL = 26;

	/// The briefing, as a card: its picture, title and time, opened with a click
	class BriefingLink : public W_BUTTON
	{
	public:
		explicit BriefingLink(const HudIntelEntry &entry): entry(entry) {}

	protected:
		void display(int xOffset, int yOffset) override
		{
			const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
			const HudColors &colors = skin().colors();
			skin().drawCard(rect, isHighlighted() ? HudCardState::Highlighted : HudCardState::Normal);
			const int thumb = rect.height() - 12;
			drawThumb(entry, WzRect(rect.x() + 6, rect.y() + 6, thumb * 3 / 2, thumb));
			const int textX = rect.x() + 6 + thumb * 3 / 2 + 12;
			const int textWidth = rect.right() - 30 - textX;
			const bool small = rect.height() < 50;
			drawLine(title, entry.title, small ? font_regular : font_regular_bold, textX, rect.y() + (small ? 4 : 10), textWidth, colors.textPrimary);
			std::string meta = _("Mission briefing");
			const std::string time = timeText(entry);
			if (!time.empty())
			{
				meta += " · " + time;
			}
			drawLine(metaLine, meta, font_small, textX, rect.y() + (small ? 22 : 31), textWidth, colors.textSecondary);
			skin().drawNamedIcon("chevron-right", WzRect(rect.right() - 24, rect.y() + (rect.height() - 14) / 2, 14, 14), colors.textSecondary);
		}

	private:
		HudIntelEntry entry;
		WzText title;
		WzText metaLine;
	};

	iV_fonts titleFont() const
	{
		return compact ? font_regular_bold : font_large;
	}

	int briefingTop() const
	{
		return briefingLink ? std::max(0, height() - (compact ? 40 : 58) - SECTION_LABEL) : height();
	}

	static std::string stateName(ObjectiveState state)
	{
		switch (state)
		{
		case ObjectiveState::Done: return _("Done");
		case ObjectiveState::Failed: return _("Failed");
		case ObjectiveState::Active: break;
		}
		return _("Active");
	}

	static std::string timeOf(const Objective &item)
	{
		const std::string time = hudFormatDuration(item.stateAt / GAME_TICKS_PER_SEC);
		switch (item.options.state)
		{
		case ObjectiveState::Done: return astringf(_("done at %s"), time.c_str());
		case ObjectiveState::Failed: return astringf(_("failed at %s"), time.c_str());
		case ObjectiveState::Active: break;
		}
		return astringf(_("since %s"), time.c_str());
	}

	Objective objective;
	bool compact;
	std::vector<Objective> all;
	std::shared_ptr<W_BUTTON> briefingLink;
	std::vector<std::string> titleLines;
	std::vector<WzText> titleTexts;
	WzText tags[3];
	WzText progressLine;
	WzText sectionText;
	std::vector<WzText> listTexts;
	std::vector<WzText> timeTexts;
	WzText briefingText;
};

/// The item's pane: its contents above, its buttons below
class IntelDetail : public WIDGET
{
public:
	std::shared_ptr<WIDGET> contents;
	std::shared_ptr<IntelBar> bar;
	int barHeight = BUTTON_HEIGHT;

	void geometryChanged() override
	{
		const int barTop = bar ? height() - barHeight - 12 : height();
		if (contents)
		{
			contents->setGeometry(0, 0, width(), std::min(barTop - 8, std::max(contents->height(), barTop - 8)));
		}
		if (bar)
		{
			bar->setGeometry(0, barTop, width(), barHeight + 12);
		}
	}
};

}

std::shared_ptr<IntelView> IntelView::make(bool compact)
{
	auto view = std::make_shared<IntelView>();
	view->initialize(compact);
	return view;
}

void IntelView::initialize(bool compactLayout)
{
	compact = compactLayout;
	std::weak_ptr<IntelView> weakSelf = std::static_pointer_cast<IntelView>(shared_from_this());

	W_EDBINIT init;
	init.FontID = font_small;
	init.pBoxDisplay = displayNothing;
	searchBox = std::make_shared<W_EDITBOX>(&init);
	searchBox->setPlaceholder(WzString::fromUtf8(compact ? _("Search") : _("Search reports and briefings")));
	PIELIGHT muted = skin().colors().textSecondary;
	muted.byte.a = 150;
	searchBox->setPlaceholderTextColor(muted);
	searchBox->setMaxStringSize(40);
	searchBox->setOnEscapeHandler([weakSelf](W_EDITBOX &box) {
		box.setString(WzString());
		box.stopEditing();
		if (auto self = weakSelf.lock())
		{
			self->searchShown = false;
			self->layoutChildren();
		}
	});

	auto field = std::make_shared<IntelSearchField>();
	field->initialize(searchBox);
	searchField = field;
	accessory = std::make_shared<WIDGET>();

	if (compact)
	{
		auto menuButton = IntelButton::make("", "filter", false, BUTTON_HEIGHT_COMPACT);
		menuButton->trailingIcon = "chevron-down";
		menuButton->addOnClickHandler([weakSelf](W_BUTTON &button) {
			if (auto self = weakSelf.lock())
			{
				self->openFilterMenu(button.shared_from_this(), true);
			}
		});
		filterButton = menuButton;
		accessory->attach(menuButton);
		auto find = IntelButton::make("", "search", false, BUTTON_HEIGHT_COMPACT);
		find->setTip(_("Search"));
		find->addOnClickHandler([weakSelf](W_BUTTON &) {
			if (auto self = weakSelf.lock())
			{
				self->focusSearch();
			}
		});
		searchButton = find;
		accessory->attach(find);
		accessory->attach(searchField);
	}
	else
	{
		for (IntelFilter which : FILTER_ORDER)
		{
			auto chip = HudChip::make(WzString());
			chip->onPressed = [weakSelf, which](HudChip &, bool) {
				if (auto self = weakSelf.lock())
				{
					self->setFilter(which);
				}
			};
			chips.push_back(chip);
			attach(chip);
		}
		auto orderButton = IntelButton::make(_("Newest first"), "", false, CHIP_HEIGHT, true);
		orderButton->trailingIcon = "chevron-down";
		orderButton->addOnClickHandler([weakSelf](W_BUTTON &button) {
			if (auto self = weakSelf.lock())
			{
				self->openFilterMenu(button.shared_from_this(), false);
			}
		});
		orderChip = orderButton;
		attach(orderButton);
		// (In the header, beside the close button)
		accessory->attach(searchField);
	}

	list = ScrollableListWidget::make();
	list->setItemSpacing(2);
	list->setBackgroundColor(pal_RGBA(0, 0, 0, 0));
	list->setScrollbarWidth(SCROLLBAR_WIDTH);
	list->setScrollbarDisplay(displayScrollbar);
	list->setExpandWhenScrollbarInvisible(false);
	attach(list);
	emptyLabel = std::make_shared<IntelNote>(_("No intelligence yet"));
	attach(emptyLabel);
	reload();
}

void IntelView::reload()
{
	entries = hudIntelEntries();
	newAtOpen.clear();
	for (const HudIntelEntry &entry : entries)
	{
		if (entry.unread)
		{
			newAtOpen.insert(entry.key);
		}
	}
	rebuildList();
}

const HudIntelEntry *IntelView::entry(size_t index) const
{
	return index < entries.size() ? &entries[index] : nullptr;
}

bool IntelView::isSelected(size_t index) const
{
	return selected.has_value() && *selected == index;
}

std::string IntelView::subtitle(const HudIntelEntry &item) const
{
	return entrySubtitle(item);
}

size_t IntelView::unreadCount() const
{
	return static_cast<size_t>(std::count_if(entries.begin(), entries.end(), [](const HudIntelEntry &item) { return item.unread; }));
}

bool IntelView::passes(const HudIntelEntry &item) const
{
	switch (filter)
	{
	case IntelFilter::All: break;
	case IntelFilter::Briefings: if (item.kind == HudIntelKind::Research) { return false; } break;
	case IntelFilter::Research: if (item.kind != HudIntelKind::Research) { return false; } break;
	case IntelFilter::Unread: if (!item.unread) { return false; } break;
	case IntelFilter::Objectives: return false;
	}
	return search.empty() || item.searchText.find(search) != std::string::npos;
}

void IntelView::rebuildList()
{
	list->clear();
	shown.clear();
	std::weak_ptr<IntelView> weakSelf = std::static_pointer_cast<IntelView>(shared_from_this());
	const int rowHeight = compact ? ROW_HEIGHT_COMPACT : ROW_HEIGHT;
	const int rowWidth = (compact ? LIST_WIDTH_COMPACT : LIST_WIDTH);

	objectivesSeen = objectivesRevision();
	const bool withObjectives = selectedPlayer < MAX_PLAYERS && objectivesAny(selectedPlayer);
	if (!withObjectives && filter == IntelFilter::Objectives)
	{
		filter = IntelFilter::All;
	}
	// The objectives, pinned (with the filters that show them, and no search)
	if (objectivesCard)
	{
		detach(objectivesCard);
		objectivesCard.reset();
	}
	if (withObjectives && (filter == IntelFilter::All || filter == IntelFilter::Objectives) && search.empty())
	{
		objectivesCard = ObjectivesCard::make(ObjectivesCardMode::Pinned);
		std::weak_ptr<IntelView> weakView = std::static_pointer_cast<IntelView>(shared_from_this());
		objectivesCard->isSelected = [weakView](const Objective &objective) {
			auto view = weakView.lock();
			return view && view->selectedObjective.has_value() && *view->selectedObjective == hudObjectiveKey(objective);
		};
		objectivesCard->onSelect = [weakView](const Objective &objective) {
			if (auto view = weakView.lock())
			{
				view->selectObjective(hudObjectiveKey(objective));
			}
		};
		objectivesCard->onMore = [weakView]() {
			if (auto view = weakView.lock())
			{
				view->setFilter(IntelFilter::Objectives);
			}
		};
		attach(objectivesCard);
	}
	// Or, when there are none, the latest mission briefing (with the filters that show briefings, and no search)
	pinned.reset();
	if (!withObjectives && (filter == IntelFilter::All || filter == IntelFilter::Briefings) && search.empty())
	{
		for (size_t i = 0; i < entries.size(); ++i)
		{
			if (entries[i].kind == HudIntelKind::Briefing && entries[i].message != nullptr)
			{
				pinned = i;
				break;
			}
		}
	}
	if (pinnedCard)
	{
		detach(pinnedCard);
		pinnedCard.reset();
	}
	if (pinned.has_value())
	{
		pinnedCard = std::make_shared<IntelPinned>(std::static_pointer_cast<IntelView>(shared_from_this()), *pinned);
		attach(pinnedCard);
	}

	std::vector<size_t> matching;
	for (size_t i = 0; i < entries.size(); ++i)
	{
		if (passes(entries[i]))
		{
			matching.push_back(i);
		}
	}

	std::vector<std::pair<std::string, std::vector<size_t>>> sections;
	if (order == IntelOrder::ByKind)
	{
		std::vector<size_t> briefings;
		std::vector<size_t> reports;
		for (size_t i : matching)
		{
			(entries[i].kind == HudIntelKind::Research ? reports : briefings).push_back(i);
		}
		sections.emplace_back(_("Briefings"), briefings);
		sections.emplace_back(_("Research"), reports);
	}
	else
	{
		std::vector<size_t> fresh;
		std::vector<size_t> earlier;
		for (size_t i : matching)
		{
			(newAtOpen.count(entries[i].key) != 0 ? fresh : earlier).push_back(i);
		}
		sections.emplace_back(_("New"), fresh);
		// Earlier: by the mission it came in, when there's more than one
		std::set<std::string> levels;
		for (size_t i : earlier)
		{
			levels.insert(entries[i].level);
		}
		if (levels.size() <= 1)
		{
			sections.emplace_back(_("Earlier"), earlier);
		}
		else
		{
			for (size_t i : earlier)
			{
				const std::string &level = entries[i].level;
				std::string label = _("Earlier");
				const std::string title = level.empty() ? std::string() : hudIntelLevelTitle(level);
				if (!title.empty())
				{
					label = astringf(_("Earlier · %s"), title.c_str());
				}
				if (sections.size() <= 1 || sections.back().first != label)
				{
					sections.emplace_back(label, std::vector<size_t>());
				}
				sections.back().second.push_back(i);
			}
		}
	}

	for (const auto &section : sections)
	{
		if (section.second.empty())
		{
			continue;
		}
		auto heading = std::make_shared<IntelSection>(section.first, section.second.size());
		heading->setGeometry(0, 0, rowWidth, SECTION_HEIGHT);
		list->addItem(heading);
		for (size_t i : section.second)
		{
			auto row = std::make_shared<IntelRow>(std::static_pointer_cast<IntelView>(shared_from_this()), i);
			row->setGeometry(0, 0, rowWidth, rowHeight);
			list->addItem(row);
			shown.push_back(i);
		}
	}

	detach(emptyLabel);
	emptyLabel = std::make_shared<IntelNote>(entries.empty() ? _("No intelligence yet") : _("Nothing matches"));
	attach(emptyLabel);
	emptyLabel->show(shown.empty() && !pinned.has_value() && !objectivesCard);
	updateChips();
	layoutChildren();
	// What the filters or the search hide doesn't stay shown beside the list
	if (selectedObjective.has_value())
	{
		const auto objectives = objectivesCard ? objectivesFor(selectedPlayer) : std::vector<Objective>();
		const bool stillThere = std::any_of(objectives.begin(), objectives.end(), [this](const Objective &objective) {
			return hudObjectiveKey(objective) == *selectedObjective;
		});
		if (!stillThere)
		{
			selectedObjective.reset();
			if (!shown.empty())
			{
				select(shown.front(), false);
			}
			else
			{
				rebuildDetail();
			}
		}
	}
	else if (selected.has_value() && std::find(shown.begin(), shown.end(), *selected) == shown.end() && (!pinned.has_value() || *pinned != *selected))
	{
		if (!shown.empty())
		{
			select(shown.front(), false);
		}
		else
		{
			selected.reset();
			rebuildDetail();
		}
	}
}

size_t IntelView::filterCount(IntelFilter which) const
{
	if (which == IntelFilter::Objectives)
	{
		return selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer).size() : 0;
	}
	size_t n = 0;
	for (const HudIntelEntry &item : entries)
	{
		switch (which)
		{
		case IntelFilter::All: ++n; break;
		case IntelFilter::Briefings: n += item.kind != HudIntelKind::Research ? 1 : 0; break;
		case IntelFilter::Research: n += item.kind == HudIntelKind::Research ? 1 : 0; break;
		case IntelFilter::Unread: n += item.unread ? 1 : 0; break;
		case IntelFilter::Objectives: break;
		}
	}
	return n;
}

bool IntelView::filterShown(IntelFilter which) const
{
	// (Most games have no objectives)
	return which != IntelFilter::Objectives || (selectedPlayer < MAX_PLAYERS && objectivesAny(selectedPlayer));
}

void IntelView::updateChips()
{
	for (size_t i = 0; i < chips.size(); ++i)
	{
		const IntelFilter which = FILTER_ORDER[i];
		chips[i]->setText(WzString::fromUtf8(astringf("%s %zu", gettext(filterName(which)), filterCount(which))));
		chips[i]->setActive(filter == which);
		chips[i]->show(filterShown(which));
	}
	if (auto button = std::dynamic_pointer_cast<IntelButton>(orderChip))
	{
		button->setLabel(order == IntelOrder::Newest ? _("Newest first") : _("By kind"));
	}
	if (auto button = std::dynamic_pointer_cast<IntelButton>(filterButton))
	{
		button->setLabel(astringf("%s %zu", gettext(filterName(filter)), filterCount(filter)));
	}
}

void IntelView::setFilter(IntelFilter newFilter)
{
	filter = newFilter;
	rebuildList();
}

void IntelView::setOrder(IntelOrder newOrder)
{
	order = newOrder;
	rebuildList();
}

void IntelView::openFilterMenu(const std::shared_ptr<WIDGET> &anchor, bool withFilters)
{
	auto menu = PopoverMenuWidget::make();
	std::weak_ptr<IntelView> weakSelf = std::static_pointer_cast<IntelView>(shared_from_this());
	const auto addItem = [&menu](const std::string &text, bool current, std::function<void ()> onChosen) {
		auto item = WzClickableOptionsChoiceWidget::make(font_small);
		item->setString(WzString::fromUtf8(current ? "✓ " + text : text));
		item->addOnClickHandler([onChosen](WzClickableOptionsChoiceWidget &, WIDGET_KEY) { onChosen(); });
		item->setGeometry(0, 0, item->idealWidth(), item->idealHeight());
		menu->addMenuItem(item, true);
	};
	if (withFilters)
	{
		for (IntelFilter which : FILTER_ORDER)
		{
			if (!filterShown(which))
			{
				continue;
			}
			addItem(gettext(filterName(which)), filter == which, [weakSelf, which]() {
				if (auto self = weakSelf.lock())
				{
					self->setFilter(which);
				}
			});
		}
	}
	addItem(_("Newest first"), order == IntelOrder::Newest, [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			self->setOrder(IntelOrder::Newest);
		}
	});
	addItem(_("By kind"), order == IntelOrder::ByKind, [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			self->setOrder(IntelOrder::ByKind);
		}
	});
	hudOpenMenu(menu, anchor);
}

void IntelView::select(size_t index, bool byPointer)
{
	if (index >= entries.size())
	{
		return;
	}
	const bool changed = !selected.has_value() || *selected != index || selectedObjective.has_value();
	selected = index;
	selectedObjective.reset();
	HudIntelEntry &item = entries[index];
	if (item.message != nullptr && !item.message->read)
	{
		item.message->read = true;
		item.unread = false;
		updateChips();
		if (onUnreadChanged)
		{
			onUnreadChanged();
		}
	}
	if (changed)
	{
		rebuildDetail();
		if (byPointer && item.kind == HudIntelKind::Research)
		{
			intPlayResearchReportAudio(item.research);
		}
	}
	// Keep the selected row in view
	size_t rowPosition = 0;
	for (size_t row = 0; row < list->numItems(); ++row)
	{
		if (auto widget = std::dynamic_pointer_cast<IntelRow>(list->getItemAtIdx(row)))
		{
			if (shown[rowPosition] == index)
			{
				list->scrollEnsureItemVisible(row);
				break;
			}
			++rowPosition;
		}
	}
}

bool IntelView::selectObjective(const std::string &key)
{
	const auto objectives = selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer) : std::vector<Objective>();
	const Objective *chosen = nullptr;
	for (const Objective &objective : objectives)
	{
		if (key.empty() ? objective.options.state == ObjectiveState::Active : hudObjectiveKey(objective) == key)
		{
			chosen = &objective;
			break;
		}
	}
	if (chosen == nullptr && key.empty() && !objectives.empty())
	{
		chosen = &objectives.front();
	}
	if (chosen == nullptr)
	{
		return false;
	}
	if (!objectivesCard)
	{
		filter = IntelFilter::All;
		search.clear();
		searchBox->setString(WzString());
		rebuildList();
	}
	const std::string chosenKey = hudObjectiveKey(*chosen);
	const bool changed = !selectedObjective.has_value() || *selectedObjective != chosenKey;
	selected.reset();
	selectedObjective = chosenKey;
	if (changed)
	{
		rebuildDetail();
	}
	return true;
}

bool IntelView::selectEntry(HudIntelKind kind, const std::string &key)
{
	if (static_cast<int>(kind) == INTEL_OBJECTIVES_KIND)
	{
		return selectObjective(key);
	}
	for (size_t i = 0; i < entries.size(); ++i)
	{
		if (entries[i].kind == kind && entries[i].key == key)
		{
			select(i, false);
			return true;
		}
	}
	return false;
}

void IntelView::selectMessage(const MESSAGE *message)
{
	for (size_t i = 0; message != nullptr && i < entries.size(); ++i)
	{
		if (entries[i].message == message)
		{
			select(i, false);
			return;
		}
	}
	for (size_t i : shown)
	{
		if (entries[i].unread)
		{
			select(i, false);
			return;
		}
	}
	if (!shown.empty())
	{
		select(shown.front(), false);
	}
	else if (pinned.has_value())
	{
		select(*pinned, false);
	}
	else if (objectivesCard)
	{
		selectObjective(std::string());
	}
}

void IntelView::moveSelection(int delta)
{
	if (shown.empty())
	{
		return;
	}
	auto it = selected.has_value() ? std::find(shown.begin(), shown.end(), *selected) : shown.end();
	if (it == shown.end())
	{
		select(shown.front(), false);
		return;
	}
	const auto at = static_cast<int>(it - shown.begin()) + delta;
	select(shown[static_cast<size_t>(std::max(0, std::min(at, static_cast<int>(shown.size()) - 1)))], false);
}

void IntelView::playSelected()
{
	if (!selected.has_value())
	{
		return;
	}
	const HudIntelEntry &item = entries[*selected];
	if (item.kind != HudIntelKind::Research && item.message != nullptr)
	{
		intPlayMessageSequences(item.message);
	}
}

void IntelView::markUnread()
{
	if (!selected.has_value())
	{
		return;
	}
	HudIntelEntry &item = entries[*selected];
	if (item.message != nullptr)
	{
		item.message->read = false;
		item.unread = true;
		updateChips();
		if (onUnreadChanged)
		{
			onUnreadChanged();
		}
	}
}

void IntelView::showInResearchTree()
{
	if (!selected.has_value() || entries[*selected].research == nullptr || selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	ResearchTreeContext context;
	context.player = selectedPlayer;
	context.viewer = selectedPlayer;
	context.allowAssignment = true;
	showResearchTreeScreen(context, screen ? screen() : nullptr);
	// (Once the tree is laid out)
	const auto topic = static_cast<uint16_t>(entries[*selected].research->index);
	widgScheduleTask([topic]() { researchTreeScreenRevealResearch(topic); });
}

void IntelView::rebuildDetail()
{
	if (detail)
	{
		detach(detail);
		detail.reset();
	}
	std::weak_ptr<IntelView> weakSelf = std::static_pointer_cast<IntelView>(shared_from_this());
	const WzRect rect = detailRect();
	const int buttonHeight = compact ? BUTTON_HEIGHT_COMPACT : BUTTON_HEIGHT;
	auto pane = std::make_shared<IntelDetail>();
	pane->barHeight = buttonHeight;
	auto bar = std::make_shared<IntelBar>();
	bar->divided = !compact;
	if (selectedObjective.has_value())
	{
		const auto objectives = selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer) : std::vector<Objective>();
		auto it = std::find_if(objectives.begin(), objectives.end(), [this](const Objective &objective) {
			return hudObjectiveKey(objective) == *selectedObjective;
		});
		if (it == objectives.end())
		{
			return;
		}
		// (The latest mission briefing, which set them)
		nonstd::optional<size_t> briefing;
		for (size_t i = 0; i < entries.size(); ++i)
		{
			if (entries[i].kind == HudIntelKind::Briefing && entries[i].message != nullptr)
			{
				briefing = i;
				break;
			}
		}
		auto contents = std::make_shared<IntelObjectivePane>(*it, compact);
		contents->initialize(briefing.has_value() ? &entries[*briefing] : nullptr, [weakSelf, briefing]() {
			auto self = weakSelf.lock();
			if (self && briefing.has_value())
			{
				self->select(*briefing, true);
			}
		});
		contents->setGeometry(0, 0, rect.width(), rect.height() - buttonHeight - 20);
		pane->contents = contents;
		pane->attach(contents);
		if (it->options.state == ObjectiveState::Active && hudObjectiveHasPlace(*it))
		{
			const Objective target = *it;
			auto show = IntelButton::make(_("Show on map"), "show-on-map", true, buttonHeight);
			show->addOnClickHandler([target](W_BUTTON &) {
				widgScheduleTask([target]() { hudShowObjectiveOnMap(target); });
			});
			bar->add(show);
		}
		pane->bar = bar;
		pane->attach(bar);
		detail = pane;
		attach(pane);
		layoutChildren();
		return;
	}
	if (!selected.has_value())
	{
		return;
	}
	const HudIntelEntry &item = entries[*selected];

	if (item.kind == HudIntelKind::Research)
	{
		if (item.research != nullptr)
		{
			ResearchTreeContext context;
			context.player = selectedPlayer;
			context.viewer = selectedPlayer;
			ResearchDetailOptions options;
			options.style = paneStyle();
			// (The full panel, with the report's own text, wherever it fits)
			options.compact = rect.width() < 340;
			options.maxListEntries = compact ? 3 : 6;
			auto contents = makeResearchTopicDetailContents(researchDetailSubjectForPlayer(static_cast<uint16_t>(item.research->index), selectedPlayer),
			                                                context, rect.width(), rect.width(), ResearchDetailActions(), options);
			if (contents != nullptr)
			{
				contents->setMaxHeight(rect.height() - buttonHeight - 20);
				pane->contents = contents;
				pane->attach(contents);
			}
			auto tree = IntelButton::make(compact ? _("Research tree") : _("Show in the research tree"), "research-tree", true, buttonHeight);
			tree->addOnClickHandler([weakSelf](W_BUTTON &) {
				widgScheduleTask([weakSelf]() {
					if (auto self = weakSelf.lock())
					{
						self->showInResearchTree();
					}
				});
			});
			bar->add(tree);
		}
	}
	else
	{
		auto play = [weakSelf]() {
			widgScheduleTask([weakSelf]() {
				if (auto self = weakSelf.lock())
				{
					self->playSelected();
				}
			});
		};
		auto briefing = std::make_shared<IntelBriefingPane>(item, compact);
		briefing->initialize(play);
		briefing->setGeometry(0, 0, rect.width(), rect.height() - buttonHeight - 20);
		pane->contents = briefing;
		pane->attach(briefing);
		if (seq_hasVideos())
		{
			auto button = IntelButton::make(_("Play the briefing"), "replay-play", true, buttonHeight);
			button->addOnClickHandler([play](W_BUTTON &) { play(); });
			bar->add(button);
		}
	}
	if (item.message != nullptr)
	{
		auto unread = IntelButton::make(_("Mark as unread"), "mark-unread", false, buttonHeight);
		unread->addOnClickHandler([weakSelf](W_BUTTON &) {
			if (auto self = weakSelf.lock())
			{
				self->markUnread();
			}
		});
		bar->add(unread);
	}
	pane->bar = bar;
	pane->attach(bar);
	detail = pane;
	attach(pane);
	layoutChildren();
}

WzRect IntelView::listRect() const
{
	const int top = compact ? 8 : TOOLBAR_HEIGHT + 10;
	// (With the scrollbar's room on the right)
	return WzRect(compact ? 4 : 10, top, (compact ? LIST_WIDTH_COMPACT : LIST_WIDTH) + SCROLLBAR_WIDTH, std::max(0, height() - top - (compact ? 6 : 10)));
}

WzRect IntelView::detailRect() const
{
	const WzRect left = listRect();
	const int x0 = left.right() + (compact ? 12 : 20);
	const int top = compact ? 10 : TOOLBAR_HEIGHT + 14;
	return WzRect(x0, top, std::max(0, width() - x0 - (compact ? 8 : 18)), std::max(0, height() - top - (compact ? 6 : 16)));
}

void IntelView::layoutChildren()
{
	if (compact)
	{
		const int fw = filterButton->width();
		// (Growing to the left: its right edge stays by the close button)
		// (The search field, once opened, takes more room over the header)
		const int newWidth = searchShown ? std::max(220, fw + 6 + BUTTON_HEIGHT_COMPACT) : fw + 6 + BUTTON_HEIGHT_COMPACT;
		const int right = accessory->x() + accessory->width();
		accessory->setGeometry(accessory->width() > 0 ? right - newWidth : accessory->x(), accessory->y(), newWidth, BUTTON_HEIGHT_COMPACT);
		filterButton->setGeometry(0, 0, fw, BUTTON_HEIGHT_COMPACT);
		searchButton->setGeometry(fw + 6, 0, BUTTON_HEIGHT_COMPACT, BUTTON_HEIGHT_COMPACT);
		filterButton->show(!searchShown);
		searchButton->show(!searchShown);
		searchField->show(searchShown);
		searchField->setGeometry(0, 0, accessory->width(), BUTTON_HEIGHT_COMPACT);
	}
	else
	{
		int x0 = 16;
		for (auto &chip : chips)
		{
			if (!chip->visible())
			{
				continue;
			}
			const int w = chip->idealWidth() + 8;
			chip->setGeometry(x0, 4, w, CHIP_HEIGHT);
			x0 += w + 6;
		}
		const int orderWidth = orderChip->idealWidth();
		orderChip->setGeometry(width() - 18 - orderWidth, 4, orderWidth, CHIP_HEIGHT);
		accessory->setGeometry(accessory->x(), accessory->y(), SEARCH_WIDTH, SEARCH_HEIGHT);
		searchField->setGeometry(0, 0, SEARCH_WIDTH, SEARCH_HEIGHT);
	}

	WzRect rect = listRect();
	int listTop = rect.y();
	if (objectivesCard)
	{
		// (With the Objectives filter, the whole list, otherwise up to about half of it, then "3 more")
		objectivesCard->setGeometry(rect.x(), listTop, rect.width() - SCROLLBAR_WIDTH, std::max(1, objectivesCard->height()));
		const int limit = filter == IntelFilter::Objectives ? rect.height() : std::max(120, rect.height() * 45 / 100);
		const int h = std::min(objectivesCard->fullHeight(), limit);
		objectivesCard->setGeometry(rect.x(), listTop, rect.width() - SCROLLBAR_WIDTH, h);
		listTop += h + 6;
	}
	if (pinnedCard)
	{
		const int h = compact ? PINNED_HEIGHT_COMPACT : PINNED_HEIGHT;
		pinnedCard->setGeometry(rect.x(), listTop, rect.width(), h);
		listTop += h + 6;
	}
	list->setGeometry(rect.x(), listTop, rect.width(), std::max(0, rect.bottom() - listTop));
	emptyLabel->setGeometry(rect.x(), listTop, rect.width(), std::max(0, std::min(120, rect.bottom() - listTop)));
	if (detail)
	{
		const WzRect area = detailRect();
		detail->setGeometry(area.x(), area.y(), area.width(), area.height());
	}
}

void IntelView::geometryChanged()
{
	layoutChildren();
	if (detail && hasSelection())
	{
		rebuildDetail();
	}
}

void IntelView::focusSearch()
{
	if (compact)
	{
		searchShown = true;
		layoutChildren();
	}
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	searchBox->simulateClick(&context, true);
}

bool IntelView::searchIsEditing() const
{
	return searchBox && searchBox->isEditing();
}

bool IntelView::backOut()
{
	if (!search.empty() || searchShown)
	{
		searchBox->setString(WzString());
		searchBox->stopEditing();
		searchShown = false;
		search.clear();
		rebuildList();
		return true;
	}
	return false;
}

bool IntelView::applyForTests(const std::string &key, const std::string &value)
{
	if (key == "select")
	{
		const size_t row = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
		if (row >= shown.size())
		{
			return false;
		}
		select(shown[row], false);
		return true;
	}
	if (key == "objective")
	{
		const size_t row = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
		const auto objectives = selectedPlayer < MAX_PLAYERS ? objectivesFor(selectedPlayer) : std::vector<Objective>();
		return row < objectives.size() && selectObjective(hudObjectiveKey(objectives[row]));
	}
	if (key == "pinned")
	{
		if (!pinned.has_value())
		{
			return false;
		}
		select(*pinned, false);
		return true;
	}
	if (key == "filter")
	{
		const char *names[] = {"all", "briefings", "research", "unread", "objectives"};
		for (size_t i = 0; i < 5; ++i)
		{
			if (value == names[i])
			{
				setFilter(static_cast<IntelFilter>(i));
				return filterShown(filter);
			}
		}
		return false;
	}
	if (key == "tree")
	{
		showInResearchTree();
		return true;
	}
	if (key == "order")
	{
		setOrder(value == "kind" ? IntelOrder::ByKind : IntelOrder::Newest);
		return true;
	}
	if (key == "search")
	{
		if (compact)
		{
			searchShown = true;
			layoutChildren();
		}
		searchBox->setString(WzString::fromUtf8(value));
		return true;
	}
	return false;
}

void IntelView::run(W_CONTEXT *)
{
	std::string text = searchBox->getString().toUtf8();
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if (text != search)
	{
		search = text;
		rebuildList();
	}
	if (objectivesSeen != objectivesRevision())
	{
		rebuildList();
		if (selectedObjective.has_value())
		{
			rebuildDetail();
		}
	}
}

void IntelView::display(int xOffset, int yOffset)
{
	const int x0 = xOffset + x();
	const int y0 = yOffset + y();
	if (!compact)
	{
		drawDivider(x0 + 1, y0 + TOOLBAR_HEIGHT, x0 + width() - 1, y0 + TOOLBAR_HEIGHT + 1);
	}
	const WzRect left = listRect();
	const int divider = left.right() + (compact ? 6 : 10);
	drawDivider(x0 + divider, y0 + (compact ? 1 : TOOLBAR_HEIGHT + 1), x0 + divider + 1, y0 + height() - 1);
}
