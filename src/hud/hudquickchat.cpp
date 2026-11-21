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

#include "hudquickchat.h"
#include "hudchatbar.h"
#include "intel/intelkit.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/button.h"
#include "lib/widget/widgint.h"

#include "../multiplay.h"

#include <algorithm>

using namespace intelkit;

namespace
{

constexpr int WIDTH = 640;
constexpr int HEADER = 40;
constexpr int FOOTER = 30;
constexpr int ROW = 30;
constexpr int RAIL = 170;
constexpr int RAIL_COMPACT = 150;
constexpr int ROWS = 5;
constexpr int CELL_GAP = 2;
constexpr int GRID_PAD = 8;
constexpr int SLOTS = 10;
constexpr int STRIP_PAD = 8;
constexpr int CHIP_HEIGHT = 24;
constexpr int CHIP_GAP = 6;
constexpr int TAB_GAP = 6;
constexpr int TRAY_PAD = 8;
constexpr int MESSAGE_HEIGHT = 26;
constexpr int MESSAGE_GAP = 6;
constexpr float MESSAGE_RADIUS = 5.f;
constexpr int STRIP_BOTTOM = 8;
const PIELIGHT TRAY_FILL = {{19, 34, 52, 255}};
/// Whether the strip shows its open section's messages, for the next time it opens
bool stripSectionOpen = true;
constexpr uint32_t FLASH_MS = 220;

// The section last chosen, by channel (Team, All), for the next time it opens
size_t lastSection[2] = {0, 0};

size_t channelIndex(WzChatMode mode)
{
	return mode == WzChatMode::Team ? 0 : 1;
}

nonstd::optional<size_t> numberKeyPressed()
{
	static const KEY_CODE keys[SLOTS][2] = {
		{KEY_1, KEY_KP_1}, {KEY_2, KEY_KP_2}, {KEY_3, KEY_KP_3}, {KEY_4, KEY_KP_4}, {KEY_5, KEY_KP_5},
		{KEY_6, KEY_KP_6}, {KEY_7, KEY_KP_7}, {KEY_8, KEY_KP_8}, {KEY_9, KEY_KP_9}, {KEY_0, KEY_KP_0},
	};
	for (size_t i = 0; i < SLOTS; ++i)
	{
		if (keyPressed(keys[i][0]) || keyPressed(keys[i][1]))
		{
			return i;
		}
	}
	return nonstd::nullopt;
}

std::string slotKey(size_t slot)
{
	return std::to_string((slot + 1) % 10);
}

/// A section in the rail: its key, title and count
class SectionRow : public W_BUTTON
{
public:
	SectionRow(HudQuickChat *owner, size_t index): owner(owner), index(index) {}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool current = owner->currentSection() == index;
		if (current)
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                    alpha(colors.accent, 24));
			pie_BoxFill(rect.x(), rect.y(), rect.x() + 3, rect.bottom(), colors.accent);
		}
		else if (isHighlighted())
		{
			pie_UniTransBoxFill(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                    pal_RGBA(255, 255, 255, 10));
		}
		KeyCapLook look = KeyCapLook::Dim;
		if (owner->hasKeys())
		{
			look = !owner->choosingMessage() ? KeyCapLook::Lit : (current ? KeyCapLook::Normal : KeyCapLook::Dim);
		}
		const int cy = rect.y() + rect.height() / 2;
		const int keyWidth = drawKeyCap(key, std::to_string(index + 1), rect.x() + 12, cy, look);
		const auto &section = owner->sectionAt(index);
		count.setText(WzString::fromUtf8(std::to_string(section.messages.size())), font_small);
		const int countX = rect.right() - 12 - count.width();
		count.render(static_cast<float>(countX), std::round(cy - count.lineSize() / 2.f - count.aboveBase()), colors.textSecondary);
		const int textX = rect.x() + 12 + keyWidth + 9;
		drawLine(title, section.title, font_regular, textX, cy - static_cast<int>(iV_GetTextLineSize(font_regular)) / 2, countX - 8 - textX,
		         current ? colors.textPrimary : colors.textSecondary);
	}

private:
	HudQuickChat *owner;
	size_t index;
	WzText key;
	WzText count;
	WzText title;
};

/// A message of the shown page: its key and text
class MessageCell : public W_BUTTON
{
public:
	MessageCell(HudQuickChat *owner, size_t slot): owner(owner), slot(slot) {}

	void display(int xOffset, int yOffset) override
	{
		const WzQuickChatMessage *message = owner->messageAt(slot);
		if (message == nullptr)
		{
			return;
		}
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (owner->flashingSlot() == static_cast<int>(slot))
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   alpha(colors.stateOk, 56), 4.f, alpha(colors.stateOk, 170), 1.f);
		}
		else if (isHighlighted())
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   alpha(colors.accent, 48), 4.f, pal_RGBA(0, 0, 0, 0), 0.f);
		}
		const KeyCapLook look = owner->hasKeys() && owner->choosingMessage() ? KeyCapLook::Lit : KeyCapLook::Dim;
		const int cy = rect.y() + rect.height() / 2;
		const int keyWidth = drawKeyCap(key, slotKey(slot), rect.x() + 7, cy, look);
		const int textX = rect.x() + 7 + keyWidth + 8;
		const std::string label = to_display_string(*message);
		const int availableWidth = rect.right() - 6 - textX;
		drawLine(text, label, font_regular, textX, cy - static_cast<int>(iV_GetTextLineSize(font_regular)) / 2, availableWidth, colors.textPrimary);
		truncated = text.width() > availableWidth;
	}

	std::string getTip() override
	{
		const WzQuickChatMessage *message = owner->messageAt(slot);
		return (truncated && message != nullptr) ? std::string(to_display_string(*message)) : std::string();
	}

private:
	HudQuickChat *owner;
	size_t slot;
	bool truncated = false;
	WzText key;
	WzText text;
};

/// The strip's section: its key (while the sections have the keys) and title (the strip draws the open one as a tab)
class SectionChip : public W_BUTTON
{
public:
	SectionChip(HudQuickChat *owner, size_t index): owner(owner), index(index) {}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool open = owner->sectionShown() && owner->currentSection() == index;
		const bool keys = owner->hasKeys() && !owner->choosingMessage();
		if (!open)
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   isHighlighted() ? pal_RGBA(255, 255, 255, 18) : pal_RGBA(255, 255, 255, 8), 4.f,
			                   pal_RGBA(120, 150, 190, keys || isHighlighted() ? 120 : 70), 1.f);
		}
		const auto &section = owner->sectionAt(index);
		const int cy = rect.y() + rect.height() / 2;
		const int keyWidth = keys ? keyCapWidth(key, std::to_string(index + 1)) + 6 : 0;
		const int availableWidth = rect.width() - 12 - keyWidth;
		title.setText(WzString::fromUtf8(section.title), font_regular);
		const int textWidth = std::min(availableWidth, title.width());
		int x0 = rect.x() + (rect.width() - keyWidth - textWidth) / 2;
		if (keys)
		{
			drawKeyCap(key, std::to_string(index + 1), x0, cy, KeyCapLook::Lit);
			x0 += keyWidth;
		}
		drawLine(title, section.title, font_regular, x0, cy - static_cast<int>(iV_GetTextLineSize(font_regular)) / 2, availableWidth,
		         (open || isHighlighted()) ? colors.textPrimary : colors.textSecondary);
	}

private:
	HudQuickChat *owner;
	size_t index;
	WzText key;
	WzText title;
};

/// The strip's message: a button with its key and text
class MessageButton : public W_BUTTON
{
public:
	MessageButton(HudQuickChat *owner, size_t slot): owner(owner), slot(slot) {}

	void display(int xOffset, int yOffset) override
	{
		const WzQuickChatMessage *message = owner->messageAt(slot);
		if (message == nullptr)
		{
			return;
		}
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		PIELIGHT fill = pal_RGBA(255, 255, 255, 13);
		PIELIGHT border = pal_RGBA(120, 150, 190, 72);
		if (owner->flashingSlot() == static_cast<int>(slot))
		{
			fill = alpha(colors.stateOk, 56);
			border = alpha(colors.stateOk, 170);
		}
		else if (isHighlighted())
		{
			fill = alpha(colors.accent, 48);
		}
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   fill, MESSAGE_RADIUS, border, 1.f);
		const KeyCapLook look = owner->hasKeys() && owner->choosingMessage() ? KeyCapLook::Lit : KeyCapLook::Dim;
		const int cy = rect.y() + rect.height() / 2;
		const int keyWidth = drawKeyCap(key, slotKey(slot), rect.x() + 7, cy, look);
		const int textX = rect.x() + 7 + keyWidth + 7;
		const std::string label = to_display_string(*message);
		const int availableWidth = rect.right() - 10 - textX;
		drawLine(text, label, font_regular, textX, cy - static_cast<int>(iV_GetTextLineSize(font_regular)) / 2, availableWidth, colors.textPrimary);
		truncated = text.width() > availableWidth;
	}

	std::string getTip() override
	{
		const WzQuickChatMessage *message = owner->messageAt(slot);
		return (truncated && message != nullptr) ? std::string(to_display_string(*message)) : std::string();
	}

private:
	HudQuickChat *owner;
	size_t slot;
	bool truncated = false;
	WzText key;
	WzText text;
};

/// The strip's pager: a page key to click (− or +, and the = key and the keypad's + also page on)
class PageButton : public W_BUTTON
{
public:
	explicit PageButton(std::string keyName): keyName(std::move(keyName)) {}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		drawKeyCap(key, keyName, rect.x() + (rect.width() - keyCapWidth(key, keyName)) / 2, rect.y() + rect.height() / 2,
		           isHighlighted() ? KeyCapLook::Lit : KeyCapLook::Normal);
	}

private:
	std::string keyName;
	WzText key;
};

}

bool hudQuickChatEnabled()
{
	return currentHudSkin().drawsQuickChat();
}

std::shared_ptr<HudQuickChat> HudQuickChat::make(bool team, bool compact)
{
	auto quickChat = std::make_shared<HudQuickChat>();
	quickChat->initialize(team, compact);
	return quickChat;
}

std::shared_ptr<HudQuickChat> HudQuickChat::makeStrip(bool team, bool compact)
{
	auto quickChat = std::make_shared<HudQuickChat>();
	quickChat->strip = true;
	quickChat->initialize(team, compact);
	return quickChat;
}

void HudQuickChat::initialize(bool team, bool compactLayout)
{
	compact = compactLayout;
	const std::vector<uint32_t> teammates = quickChatTeammates();
	teamAvailable = !teammates.empty();
	chatMode = (team && teamAvailable) ? WzChatMode::Team : WzChatMode::Glob;
	noTeammates = strip && team && !teamAvailable;

	// The recipients of Team, as the Classic panel's "To" choice: human and AI teammates, by position
	for (uint32_t player : teammates)
	{
		if (NetPlay.players[player].isSpectator)
		{
			continue;
		}
		if (NetPlay.players[player].allocated)
		{
			humans.push_back(player);
		}
		else if (NetPlay.players[player].ai >= 0)
		{
			bots.push_back(player);
		}
	}
	const auto byPosition = [](uint32_t a, uint32_t b) { return NetPlay.players[a].position < NetPlay.players[b].position; };
	std::sort(humans.begin(), humans.end(), byPosition);
	std::sort(bots.begin(), bots.end(), byPosition);
	const WzQuickChatTargeting initial = quickChatDefaultTeamTargeting();
	if (initial.humanTeammates)
	{
		chosen.insert(humans.begin(), humans.end());
	}
	if (initial.aiTeammates)
	{
		chosen.insert(bots.begin(), bots.end());
	}
	chosen.insert(initial.specificPlayers.begin(), initial.specificPlayers.end());

	std::weak_ptr<HudQuickChat> weakSelf = std::static_pointer_cast<HudQuickChat>(shared_from_this());
	if (!strip)
	{
		channelSwitch = HudChannelSwitch::make(compact, [weakSelf]() {
			auto self = weakSelf.lock();
			return self ? self->chatMode : WzChatMode::Glob;
		}, [weakSelf](WzChatMode chosenMode) {
			if (auto self = weakSelf.lock())
			{
				self->setMode(chosenMode);
				self->takeFocus();
				if (self->onModeChanged)
				{
					self->onModeChanged(chosenMode);
				}
			}
		}, teamAvailable);
		attach(channelSwitch);
	}

	if (humans.size() + bots.size() > 1)
	{
		toButton = IntelButton::make("", "", false, 24);
		toButton->trailingIcon = "chevron-down";
		toButton->addOnClickHandler([weakSelf](W_BUTTON &) {
			widgScheduleTask([weakSelf]() {
				if (auto self = weakSelf.lock())
				{
					self->openRecipients();
				}
			});
		});
		attach(toButton);
		updateRecipientsLabel();
	}

	for (size_t slot = 0; slot < SLOTS; ++slot)
	{
		std::shared_ptr<W_BUTTON> cell;
		if (strip)
		{
			cell = std::make_shared<MessageButton>(this, slot);
		}
		else
		{
			cell = std::make_shared<MessageCell>(this, slot);
		}
		cell->addOnClickHandler([weakSelf, slot](W_BUTTON &) {
			widgScheduleTask([weakSelf, slot]() {
				if (auto self = weakSelf.lock())
				{
					self->send(slot);
				}
			});
		});
		messageCells[slot] = cell;
		attach(cell);
	}
	if (strip)
	{
		const auto pageButton = [weakSelf](const std::string &keyName, int step) {
			auto button = std::make_shared<PageButton>(keyName);
			button->addOnClickHandler([weakSelf, step](W_BUTTON &) {
				widgScheduleTask([weakSelf, step]() {
					if (auto self = weakSelf.lock())
					{
						self->setPage(step < 0 ? (self->page > 0 ? self->page - 1 : 0) : self->page + 1);
					}
				});
			});
			return button;
		};
		pagePrevious = pageButton("−", -1);
		pageNext = pageButton("+", 1);
		attach(pagePrevious);
		attach(pageNext);
	}
	rebuildSections();
}

const HudQuickChat::Section &HudQuickChat::sectionAt(size_t index) const
{
	return sections[std::min(index, sections.size() - 1)];
}

void HudQuickChat::rebuildSections()
{
	sections.clear();
	const bool team = chatMode == WzChatMode::Team;
	for (const auto &info : quickChatInGameSections(team ? WzQuickChatMode::Team : WzQuickChatMode::Global))
	{
		sections.push_back({info.title, info.messages, false});
	}
	if (!team)
	{
		for (const auto &info : quickChatInGameSections(WzQuickChatMode::EndGame))
		{
			sections.push_back({_("End of game"), info.messages, true});
		}
	}
	section = std::min(lastSection[channelIndex(chatMode)], sections.size() - 1);
	page = 0;
	messageLevel = false;

	for (auto &row : sectionRows)
	{
		detach(row);
	}
	sectionRows.clear();
	std::weak_ptr<HudQuickChat> weakSelf = std::static_pointer_cast<HudQuickChat>(shared_from_this());
	for (size_t i = 0; i < sections.size(); ++i)
	{
		std::shared_ptr<W_BUTTON> row;
		if (strip)
		{
			row = std::make_shared<SectionChip>(this, i);
		}
		else
		{
			row = std::make_shared<SectionRow>(this, i);
		}
		row->addOnClickHandler([weakSelf, i](W_BUTTON &) {
			widgScheduleTask([weakSelf, i]() {
				if (auto self = weakSelf.lock())
				{
					self->clickSection(i);
				}
			});
		});
		sectionRows.push_back(row);
		attach(row);
	}
	if (toButton)
	{
		toButton->show(team);
	}
	geometryChanged();
	notifyResized();
}

void HudQuickChat::setMode(WzChatMode newMode)
{
	if (strip)
	{
		const bool had = noTeammates;
		noTeammates = newMode == WzChatMode::Team && !teamAvailable;
		if (noTeammates != had)
		{
			geometryChanged();
			notifyResized();
		}
	}
	if (newMode == chatMode || (newMode == WzChatMode::Team && !teamAvailable))
	{
		return;
	}
	chatMode = newMode;
	closeMenu();
	rebuildSections();
}

void HudQuickChat::takeFocus()
{
	if (auto screen = screenPointer.lock())
	{
		screen->setFocus(shared_from_this());
	}
}

bool HudQuickChat::hasKeys() const
{
	auto screen = screenPointer.lock();
	if (!screen)
	{
		return false;
	}
	auto focused = screen->getWidgetWithFocus();
	if (focused == nullptr)
	{
		// (The strip shares its screen with the chat bar, which types without the focus at times)
		return !strip;
	}
	return focused.get() == this || focused->hasAncestor(const_cast<HudQuickChat *>(this));
}

size_t HudQuickChat::pageCount() const
{
	return std::max<size_t>(1, (sections[section].messages.size() + SLOTS - 1) / SLOTS);
}

const WzQuickChatMessage *HudQuickChat::messageAt(size_t slot) const
{
	const auto &messages = sections[section].messages;
	const size_t index = page * SLOTS + slot;
	return index < messages.size() ? &messages[index] : nullptr;
}

int HudQuickChat::flashingSlot() const
{
	return sentSlot;
}

void HudQuickChat::chooseSection(size_t index)
{
	if (index >= sections.size() || sentSlot >= 0)
	{
		return;
	}
	section = index;
	lastSection[channelIndex(chatMode)] = index;
	page = 0;
	messageLevel = true;
	if (strip)
	{
		stripSectionOpen = true;
		geometryChanged();
		notifyResized();
	}
}

void HudQuickChat::clickSection(size_t index)
{
	if (strip && stripSectionOpen && index == section)
	{
		// Its tab: closes it
		stripSectionOpen = false;
		messageLevel = false;
		takeFocus();
		geometryChanged();
		notifyResized();
		return;
	}
	chooseSection(index);
	takeFocus();
}

bool HudQuickChat::sectionShown() const
{
	return !strip || stripSectionOpen;
}

void HudQuickChat::takeFocusFromTyping()
{
	if (strip)
	{
		messageLevel = false;
	}
	takeFocus();
}

void HudQuickChat::setPage(size_t newPage)
{
	const size_t previous = page;
	page = std::min(newPage, pageCount() - 1);
	if (strip && page != previous)
	{
		geometryChanged();
		notifyResized();
	}
}

void HudQuickChat::notifyResized()
{
	if (strip && onResized)
	{
		onResized();
	}
}

WzQuickChatTargeting HudQuickChat::teamTargeting() const
{
	if (!toButton)
	{
		return quickChatDefaultTeamTargeting();
	}
	// As the Classic panel's "To" choice: a whole group as the group, the rest one by one
	WzQuickChatTargeting targeting;
	std::unordered_set<uint32_t> rest = chosen;
	const auto allChosen = [this](const std::vector<uint32_t> &group) {
		return !group.empty() && std::all_of(group.begin(), group.end(), [this](uint32_t player) { return chosen.count(player) > 0; });
	};
	if (allChosen(humans))
	{
		targeting.humanTeammates = true;
		for (uint32_t player : humans)
		{
			rest.erase(player);
		}
	}
	if (allChosen(bots))
	{
		targeting.aiTeammates = true;
		for (uint32_t player : bots)
		{
			rest.erase(player);
		}
	}
	targeting.specificPlayers = rest;
	return targeting;
}

void HudQuickChat::send(size_t slot)
{
	const WzQuickChatMessage *message = messageAt(slot);
	if (message == nullptr || sentSlot >= 0 || noTeammates)
	{
		return;
	}
	const WzQuickChatTargeting targeting = (chatMode == WzChatMode::Glob || sections[section].everyone) ? WzQuickChatTargeting::targetAll() : teamTargeting();
	if (targeting.noTargets())
	{
		openRecipients();
		return;
	}
	sendQuickChat(*message, selectedPlayer, targeting);
	sentSlot = static_cast<int>(slot);
	sentAt = realTime;
}

void HudQuickChat::updateRecipientsLabel()
{
	if (!toButton)
	{
		return;
	}
	const size_t all = humans.size() + bots.size();
	const auto exactly = [this](const std::vector<uint32_t> &group) {
		return chosen.size() == group.size() && std::all_of(group.begin(), group.end(), [this](uint32_t player) { return chosen.count(player) > 0; });
	};
	std::string who;
	if (chosen.empty())
	{
		who = _("Nobody");
	}
	else if (chosen.size() == all)
	{
		who = _("Team");
	}
	else if (chosen.size() == 1)
	{
		who = getPlayerName(*chosen.begin());
	}
	else if (!bots.empty() && exactly(humans))
	{
		who = _("Human teammates");
	}
	else if (!humans.empty() && exactly(bots))
	{
		who = _("Bot teammates");
	}
	else
	{
		who = astringf(_("%u players"), static_cast<unsigned>(chosen.size()));
	}
	toButton->setLabel(astringf(_("To: %s"), who.c_str()));
	toButton->danger = chosen.empty();
	geometryChanged();
}

void HudQuickChat::openRecipients()
{
	if (!toButton || chatMode != WzChatMode::Team)
	{
		return;
	}
	std::weak_ptr<HudQuickChat> weakSelf = std::static_pointer_cast<HudQuickChat>(shared_from_this());
	// (Opened again after each choice, so a group's switch and its players' agree)
	const auto choose = [weakSelf](std::vector<uint32_t> players, bool on) {
		return [weakSelf, players, on]() {
			auto self = weakSelf.lock();
			if (!self)
			{
				return;
			}
			for (uint32_t player : players)
			{
				if (on)
				{
					self->chosen.insert(player);
				}
				else
				{
					self->chosen.erase(player);
				}
			}
			self->updateRecipientsLabel();
			closeMenu();
			widgScheduleTask([weakSelf]() {
				if (auto again = weakSelf.lock())
				{
					again->openRecipients();
				}
			});
		};
	};
	const auto allChosen = [this](const std::vector<uint32_t> &group) {
		return std::all_of(group.begin(), group.end(), [this](uint32_t player) { return chosen.count(player) > 0; });
	};
	auto menu = IntelMenu::make(30, 220);
	auto heading = std::make_shared<WzText>(WzString::fromUtf8(_("SEND TO")), font_small);
	menu->setHeading(26, [heading](const WzRect &rect) {
		heading->render(static_cast<float>(rect.x() + 12), std::round(rect.y() + rect.height() / 2.f - heading->lineSize() / 2.f - heading->aboveBase()),
		                skin().colors().textSecondary);
	});
	if (!humans.empty() && !bots.empty())
	{
		IntelMenu::Item humansItem;
		humansItem.label = _("Human teammates");
		humansItem.toggle = allChosen(humans);
		humansItem.onChosen = choose(humans, !allChosen(humans));
		menu->addItem(humansItem);
		IntelMenu::Item botsItem;
		botsItem.label = _("Bot teammates");
		botsItem.toggle = allChosen(bots);
		botsItem.onChosen = choose(bots, !allChosen(bots));
		menu->addItem(botsItem);
		menu->addSeparator();
	}
	std::vector<uint32_t> everyone = humans;
	everyone.insert(everyone.end(), bots.begin(), bots.end());
	std::sort(everyone.begin(), everyone.end(), [](uint32_t a, uint32_t b) { return NetPlay.players[a].position < NetPlay.players[b].position; });
	for (uint32_t player : everyone)
	{
		IntelMenu::Item item;
		item.label = getPlayerName(player);
		item.player = player;
		item.detail = NetPlay.players[player].allocated ? std::string() : std::string(_("AI"));
		const bool on = chosen.count(player) > 0;
		item.toggle = on;
		item.onChosen = choose({player}, !on);
		menu->addItem(item);
	}
	menu->open(toButton, true);
}

int32_t HudQuickChat::idealWidth()
{
	return WIDTH;
}

int32_t HudQuickChat::idealHeight()
{
	if (strip)
	{
		return stripHeight(width());
	}
	const int rail = 12 + static_cast<int>(sections.size()) * ROW;
	const int grid = 2 * GRID_PAD + ROWS * ROW + (ROWS - 1) * CELL_GAP;
	return HEADER + 1 + std::max(rail, grid) + 1 + FOOTER;
}

void HudQuickChat::geometryChanged()
{
	if (strip)
	{
		if (messageCells.back() && pageNext)
		{
			layoutStrip(width(), true);
		}
		return;
	}
	if (!channelSwitch || !messageCells.back())
	{
		return;
	}
	titleText.setText(WzString::fromUtf8(_("Quick chat")), font_regular_bold);
	const int switchX = 12 + 16 + 8 + titleText.width() + 14;
	channelSwitch->setGeometry(switchX, (HEADER - channelSwitch->idealHeight()) / 2, channelSwitch->idealWidth(), channelSwitch->idealHeight());
	if (toButton)
	{
		const int availableWidth = width() - 10 - (switchX + channelSwitch->idealWidth() + 12);
		const int w = std::min(toButton->idealWidth(), availableWidth);
		toButton->setGeometry(width() - 10 - w, (HEADER - 24) / 2, w, 24);
	}
	const int rail = (compact || width() < WIDTH) ? RAIL_COMPACT : RAIL;
	for (size_t i = 0; i < sectionRows.size(); ++i)
	{
		sectionRows[i]->setGeometry(0, HEADER + 1 + 6 + static_cast<int>(i) * ROW, rail, ROW);
	}
	const int gridX = rail + 1 + 10;
	const int cellWidth = (width() - gridX - 10 - 6) / 2;
	for (size_t slot = 0; slot < SLOTS; ++slot)
	{
		const int column = static_cast<int>(slot) / ROWS;
		const int row = static_cast<int>(slot) % ROWS;
		messageCells[slot]->setGeometry(gridX + column * (cellWidth + 6), HEADER + 1 + GRID_PAD + row * (ROW + CELL_GAP), cellWidth, ROW);
	}
}

void HudQuickChat::clicked(W_CONTEXT *, WIDGET_KEY)
{
	if (strip)
	{
		takeFocusFromTyping();
		return;
	}
	takeFocus();
}

void HudQuickChat::run(W_CONTEXT *psContext)
{
	if (sentSlot >= 0)
	{
		if (!closing && realTime - sentAt >= FLASH_MS)
		{
			closing = true;
			std::weak_ptr<HudQuickChat> weakSelf = std::static_pointer_cast<HudQuickChat>(shared_from_this());
			widgScheduleTask([weakSelf]() {
				auto self = weakSelf.lock();
				if (self && self->onSent)
				{
					self->onSent();
				}
			});
		}
		return;
	}
	if (menuIsOpen())
	{
		return;
	}
	// The wheel over it pages the section
	const int x0 = screenPosX();
	const int y0 = screenPosY();
	if (psContext->mx >= x0 && psContext->mx < x0 + width() && psContext->my >= y0 && psContext->my < y0 + height())
	{
		if (mousePressed(MOUSE_WUP))
		{
			setPage(page > 0 ? page - 1 : 0);
		}
		else if (mousePressed(MOUSE_WDN))
		{
			setPage(page + 1);
		}
	}
	if (!hasKeys() || keyPressed(KEY_ESC))
	{
		// (Escape belongs to the screen, which closes it)
		return;
	}
	handleKeys();
	// (Its keys don't go to the game)
	inputLoseFocus();
}

void HudQuickChat::handleKeys()
{
	if (keyPressed(KEY_TAB) && !keyDown(KEY_LCTRL) && !keyDown(KEY_RCTRL))
	{
		// Back to typing
		if (onTypeRequested)
		{
			auto request = onTypeRequested;
			widgScheduleTask(request);
		}
		return;
	}
	if (keyPressed(KEY_TAB))
	{
		if (teamAvailable)
		{
			const WzChatMode other = chatMode == WzChatMode::Team ? WzChatMode::Glob : WzChatMode::Team;
			setMode(other);
			if (onModeChanged)
			{
				onModeChanged(other);
			}
		}
		return;
	}
	if (keyPressed(KEY_RETURN) || keyPressed(KEY_KPENTER))
	{
		if (onTypeRequested)
		{
			auto request = onTypeRequested;
			widgScheduleTask(request);
		}
		return;
	}
	if (noTeammates)
	{
		return;
	}
	if (keyPressed(KEY_BACKSPACE))
	{
		if (messageLevel)
		{
			// Back to the sections, the open one still shown
			messageLevel = false;
		}
		else if (strip && stripSectionOpen)
		{
			stripSectionOpen = false;
			geometryChanged();
			notifyResized();
		}
		return;
	}
	if (keyPressed(KEY_MINUS) || keyPressed(KEY_KP_MINUS))
	{
		setPage(page > 0 ? page - 1 : 0);
		return;
	}
	if (keyPressed(KEY_EQUALS) || keyPressed(KEY_KP_PLUS))
	{
		setPage(page + 1);
		return;
	}
	const auto number = numberKeyPressed();
	if (!number.has_value())
	{
		return;
	}
	if (!messageLevel)
	{
		chooseSection(*number);
	}
	else
	{
		send(*number);
	}
}

void HudQuickChat::display(int xOffset, int yOffset)
{
	HudSkin &hudSkin = currentHudSkin();
	const HudColors &colors = hudSkin.colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	if (strip)
	{
		displayStrip(rect);
		return;
	}
	// (Opaque: it opens over panels)
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   alpha(colors.panelBackground, 255), 6.f, pal_RGBA(0, 0, 0, 0), 0.f);
	hudSkin.drawPanelFrame(rect, rect);

	// The header: the title, then the switch (and the recipients, at the right)
	const int hy = rect.y() + HEADER / 2;
	hudSkin.drawNamedIcon("quick-chat", WzRect(rect.x() + 12, hy - 8, 16, 16), colors.accent);
	titleText.render(static_cast<float>(rect.x() + 12 + 16 + 8), std::round(hy - titleText.lineSize() / 2.f - titleText.aboveBase()), colors.textPrimary);
	drawDivider(rect.x() + 1, rect.y() + HEADER, rect.right() - 1, rect.y() + HEADER + 1);

	const int rail = sectionRows.empty() ? RAIL : sectionRows.front()->width();
	const int footerY = rect.bottom() - FOOTER;
	drawDivider(rect.x() + rail, rect.y() + HEADER + 1, rect.x() + rail + 1, footerY - 1);
	drawDivider(rect.x() + 1, footerY - 1, rect.right() - 1, footerY);

	// The footer: the keys that apply now, with the pages and Escape at the right
	std::vector<std::pair<std::vector<std::string>, std::string>> hints;
	if (!hasKeys())
	{
		hints.push_back({{_("Tab")}, _("number keys")});
	}
	else if (!messageLevel)
	{
		hints.push_back({{sections.size() > 1 ? astringf("1–%u", static_cast<unsigned>(sections.size())) : std::string("1")}, _("section")});
	}
	else
	{
		size_t shown = 0;
		while (shown < SLOTS && messageAt(shown) != nullptr)
		{
			++shown;
		}
		hints.push_back({{shown > 1 ? "1–" + slotKey(shown - 1) : std::string("1")}, _("send")});
		if (pageCount() > 1)
		{
			hints.push_back({{"-", "+"}, _("more")});
		}
		hints.push_back({{_("Backspace")}, _("sections")});
	}
	if (hasKeys() && !compact)
	{
		if (onTypeRequested && chatFreeChatEnabled())
		{
			hints.push_back({{_("Tab")}, _("typing")});
		}
		if (teamAvailable)
		{
			hints.push_back({{_("Ctrl+Tab")}, _("Team / All")});
		}
	}
	size_t needed = 1;
	for (const auto &hint : hints)
	{
		needed += hint.first.size() + 1;
	}
	hintTexts.resize(std::max(hintTexts.size(), needed + 1));
	const int fy = footerY + FOOTER / 2;
	size_t t = 0;
	int fx = rect.x() + 12;
	for (const auto &hint : hints)
	{
		for (const auto &keyName : hint.first)
		{
			fx += drawKeyCap(hintTexts[t++], keyName, fx, fy) + 4;
		}
		WzText &label = hintTexts[t++];
		label.setText(WzString::fromUtf8(hint.second), font_small);
		label.render(static_cast<float>(fx + 2), std::round(fy - label.lineSize() / 2.f - label.aboveBase()), colors.textSecondary);
		fx += 2 + label.width() + 14;
	}
	endText.setText(WzString::fromUtf8(_("close")), font_small);
	int rx = rect.right() - 12 - endText.width();
	endText.render(static_cast<float>(rx), std::round(fy - endText.lineSize() / 2.f - endText.aboveBase()), colors.textSecondary);
	rx -= 6 + keyCapWidth(hintTexts[t], _("Esc"));
	drawKeyCap(hintTexts[t], _("Esc"), rx, fy);
	if (pageCount() > 1)
	{
		const int pages = static_cast<int>(pageCount());
		int dx = rx - 14 - pages * 10 + 4;
		for (int p = 0; p < pages; ++p)
		{
			pie_DrawRoundedBox(static_cast<float>(dx), static_cast<float>(fy - 3), static_cast<float>(dx + 6), static_cast<float>(fy + 3),
			                   p == static_cast<int>(page) ? colors.accent : pal_RGBA(120, 150, 190, 80), 3.f, pal_RGBA(0, 0, 0, 0), 0.f);
			dx += 10;
		}
	}
}

int HudQuickChat::stripHeight(int w)
{
	return layoutStrip(w, false);
}

int HudQuickChat::layoutStrip(int w, bool apply)
{
	if (apply)
	{
		for (auto &chip : sectionRows)
		{
			chip->show(!noTeammates);
		}
	}
	if (noTeammates)
	{
		if (apply)
		{
			for (auto &cell : messageCells)
			{
				cell->hide();
			}
			pagePrevious->hide();
			pageNext->hide();
		}
		trayRect = WzRect();
		return CHIP_HEIGHT + STRIP_BOTTOM;
	}
	const int x0 = STRIP_PAD;
	const int inner = std::max(0, w - 2 * STRIP_PAD);
	int chipsRight = w - STRIP_PAD;
	if (toButton && toButton->visible())
	{
		const int toWidth = std::min(toButton->idealWidth(), inner / 3);
		if (apply)
		{
			toButton->setGeometry(chipsRight - toWidth, 0, toWidth, CHIP_HEIGHT);
		}
		chipsRight -= toWidth + CHIP_GAP;
	}
	const int count = std::max<int>(1, static_cast<int>(sectionRows.size()));
	const int chipWidth = std::max(0, (chipsRight - x0 - CHIP_GAP * (count - 1)) / count);
	if (apply)
	{
		for (size_t i = 0; i < sectionRows.size(); ++i)
		{
			sectionRows[i]->setGeometry(x0 + static_cast<int>(i) * (chipWidth + CHIP_GAP), 0, chipWidth, CHIP_HEIGHT);
		}
	}
	const bool open = stripSectionOpen && !sections.empty();
	if (apply)
	{
		for (auto &cell : messageCells)
		{
			cell->show(open);
		}
		pagePrevious->show(open && pageCount() > 1);
		pageNext->show(open && pageCount() > 1);
	}
	if (!open)
	{
		trayRect = WzRect();
		return CHIP_HEIGHT + STRIP_BOTTOM;
	}

	// The open section's messages, wrapping, and the pager at the end
	const int trayTop = CHIP_HEIGHT + TAB_GAP;
	const int left = x0 + TRAY_PAD;
	const int right = x0 + inner - TRAY_PAD;
	const int keyWidth = keyCapWidth(measureKey, "0");
	const auto &messages = sections[section].messages;
	// Lays out a page's messages from the tray's top, and returns where the last row's next message would go
	const auto flow = [&](size_t onPage, bool place, int &px, int &py) {
		px = left;
		py = trayTop + TRAY_PAD;
		for (size_t slot = 0; slot < SLOTS; ++slot)
		{
			const size_t index = onPage * SLOTS + slot;
			if (index >= messages.size())
			{
				if (place)
				{
					messageCells[slot]->hide();
				}
				continue;
			}
			const int textWidth = static_cast<int>(iV_GetTextWidth(WzString::fromUtf8(to_display_string(messages[index])), font_regular));
			const int messageWidth = std::min(right - left, 7 + keyWidth + 7 + textWidth + 11);
			if (px > left && px + messageWidth > right)
			{
				px = left;
				py += MESSAGE_HEIGHT + MESSAGE_GAP;
			}
			if (place)
			{
				messageCells[slot]->setGeometry(px, py, messageWidth, MESSAGE_HEIGHT);
			}
			px += messageWidth + MESSAGE_GAP;
		}
	};
	int px = left;
	int py = 0;
	flow(page, apply, px, py);
	if (page > 0)
	{
		// (As tall as the first page, so paging doesn't move the card's edge)
		int firstX = left;
		int firstY = 0;
		flow(0, false, firstX, firstY);
		if (firstY > py)
		{
			py = firstY;
			px = firstX;
		}
	}
	if (pageCount() > 1)
	{
		pageText.setText(WzString::fromUtf8(astringf("%u / %u", static_cast<unsigned>(page + 1), static_cast<unsigned>(pageCount()))), font_small);
		const int button = keyCapWidth(measureKey, "+") + 8;
		const int pagerWidth = 2 * button + 8 + pageText.width();
		if (px > left && px + pagerWidth > right)
		{
			py += MESSAGE_HEIGHT + MESSAGE_GAP;
		}
		pagerRect = WzRect(right - pagerWidth, py, pagerWidth, MESSAGE_HEIGHT);
		if (apply)
		{
			pagePrevious->setGeometry(pagerRect.x(), py, button, MESSAGE_HEIGHT);
			pageNext->setGeometry(pagerRect.right() - button, py, button, MESSAGE_HEIGHT);
		}
	}
	const int trayBottom = py + MESSAGE_HEIGHT + TRAY_PAD;
	trayRect = WzRect(x0, trayTop, inner, trayBottom - trayTop);
	return trayBottom + STRIP_BOTTOM;
}

void HudQuickChat::displayStrip(const WzRect &rect)
{
	const HudColors &colors = skin().colors();
	if (noTeammates)
	{
		noTeammatesText.setText(WzString::fromUtf8(_("Quick chat: no teammates to send to")), font_regular);
		noTeammatesText.render(static_cast<float>(rect.x() + STRIP_PAD + 4),
		                       std::round(rect.y() + CHIP_HEIGHT / 2.f - noTeammatesText.lineSize() / 2.f - noTeammatesText.aboveBase()), colors.textSecondary);
		return;
	}
	const bool keys = hasKeys();
	if (keys && !messageLevel && !sectionRows.empty())
	{
		// The sections have the keys: a ring around them
		const int left = rect.x() + sectionRows.front()->x() - 3;
		const int right = rect.x() + sectionRows.back()->x() + sectionRows.back()->width() + 3;
		pie_DrawRoundedBox(static_cast<float>(left), static_cast<float>(rect.y() - 3), static_cast<float>(right), static_cast<float>(rect.y() + CHIP_HEIGHT + 3),
		                   pal_RGBA(0, 0, 0, 0), 6.f, alpha(colors.accent, 200), 1.f);
	}
	if (trayRect.width() <= 0 || section >= sectionRows.size())
	{
		return;
	}
	// The messages' tray, and the open section's chip as its tab
	const PIELIGHT border = (keys && messageLevel) ? colors.accent : pal_RGBA(120, 150, 190, 100);
	const float tx0 = static_cast<float>(rect.x() + trayRect.x());
	const float ty0 = static_cast<float>(rect.y() + trayRect.y());
	pie_DrawRoundedBox(tx0, ty0, tx0 + trayRect.width(), ty0 + trayRect.height(), TRAY_FILL, 6.f, border, 1.f);
	const auto &chip = sectionRows[section];
	const float cx0 = static_cast<float>(rect.x() + chip->x());
	const float cx1 = cx0 + chip->width();
	const float cy0 = static_cast<float>(rect.y() + chip->y());
	pie_DrawRoundedBox(cx0, cy0, cx1, ty0 + 6.f, TRAY_FILL, 4.f, border, 1.f);
	pie_UniTransBoxFill(cx0 + 1.f, ty0 - 1.f, cx1 - 1.f, ty0 + 7.f, TRAY_FILL);
	if (pageCount() > 1)
	{
		pageText.render(static_cast<float>(rect.x() + pagerRect.x() + (pagerRect.width() - pageText.width()) / 2),
		                std::round(rect.y() + pagerRect.y() + pagerRect.height() / 2.f - pageText.lineSize() / 2.f - pageText.aboveBase()), colors.textSecondary);
	}
}
