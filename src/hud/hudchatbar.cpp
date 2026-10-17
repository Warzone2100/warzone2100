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

#include "hudchatbar.h"
#include "intel/intelkit.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/editbox.h"
#include "lib/widget/form.h"
#include "lib/widget/virtualkeyboard.h"
#include "lib/widget/widgint.h"

#include "../hci/quickchat.h"
#include "../multiplay.h"

#include <algorithm>

using namespace intelkit;

namespace
{

constexpr int HEIGHT = 38;
constexpr int HEIGHT_COMPACT = 36;
constexpr int PAD = 6;
constexpr int SEGMENT_HEIGHT = 24;
constexpr int QUICK_CHAT_MAX_WIDTH = 1200;

void displayNothing(WIDGET *, UDWORD, UDWORD) {}

/// One side of the Team / All switch
class ChannelButton : public W_BUTTON
{
public:
	ChannelButton(std::string label, std::function<bool ()> selected): label(std::move(label)), selected(std::move(selected)) {}

	void display(int xOffset, int yOffset) override
	{
		const HudColors &colors = skin().colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		const bool on = selected && selected();
		if (on)
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x() + 1), static_cast<float>(rect.y() + 1), static_cast<float>(rect.right() - 1),
			                   static_cast<float>(rect.bottom() - 1), alpha(colors.accent, 48), 3.f, pal_RGBA(0, 0, 0, 0), 0.f);
		}
		text.setText(WzString::fromUtf8(label), font_small);
		const PIELIGHT color = (on || isHighlighted()) ? colors.textPrimary : colors.textSecondary;
		text.render(static_cast<float>(rect.x() + (rect.width() - text.width()) / 2),
		            std::round(rect.y() + rect.height() / 2.f - text.lineSize() / 2.f - text.aboveBase()), color);
	}

private:
	std::string label;
	std::function<bool ()> selected;
	WzText text;
};

/// The field: a chat icon and the edit box, on the bar itself
class ChatField : public WIDGET
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
		const int iconWidth = ICON + 14;
		box->setGeometry(iconWidth, 0, std::max(0, width() - iconWidth - 6), height());
	}

	void display(int xOffset, int yOffset) override
	{
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		skin().drawNamedIcon("chat", WzRect(rect.x() + 8, rect.y() + (rect.height() - ICON) / 2, ICON, ICON),
		                     box->isEditing() ? skin().colors().accent : skin().colors().textSecondary);
	}

private:
	static constexpr int ICON = 15;
};

}

std::shared_ptr<HudChatBar> HudChatBar::make(bool compact, WzChatMode mode)
{
	auto bar = std::make_shared<HudChatBar>();
	bar->initialize(compact, mode);
	return bar;
}

HudChatBar::~HudChatBar()
{
	closeQuickChat();
}

void HudChatBar::initialize(bool compactLayout, WzChatMode initialMode)
{
	compact = compactLayout;
	spectator = selectedPlayer >= MAX_PLAYERS || NetPlay.players[selectedPlayer].isSpectator;
	chatMode = spectator ? WzChatMode::Glob : initialMode;
	std::weak_ptr<HudChatBar> weakSelf = std::static_pointer_cast<HudChatBar>(shared_from_this());

	if (!spectator)
	{
		auto team = std::make_shared<ChannelButton>(_("Team"), [weakSelf]() {
			auto self = weakSelf.lock();
			return self && self->chatMode == WzChatMode::Team;
		});
		team->setTip(_("Your team (Tab)"));
		team->addOnClickHandler([weakSelf](W_BUTTON &) {
			if (auto self = weakSelf.lock())
			{
				self->setMode(WzChatMode::Team);
			}
		});
		auto all = std::make_shared<ChannelButton>(_("All"), [weakSelf]() {
			auto self = weakSelf.lock();
			return self && self->chatMode == WzChatMode::Glob;
		});
		all->setTip(_("Everyone (Tab)"));
		all->addOnClickHandler([weakSelf](W_BUTTON &) {
			if (auto self = weakSelf.lock())
			{
				self->setMode(WzChatMode::Glob);
			}
		});
		teamButton = team;
		allButton = all;
		attach(teamButton);
		attach(allButton);
	}

	W_EDBINIT init;
	init.FontID = font_regular;
	init.pBoxDisplay = displayNothing;
	box = std::make_shared<W_EDITBOX>(&init);
	PIELIGHT muted = skin().colors().textSecondary;
	muted.byte.a = 170;
	box->setPlaceholderTextColor(muted);
	if (chatFreeChatEnabled())
	{
		box->setPlaceholder(WzString::fromUtf8(spectator ? _("Press Enter to chat with spectators") : (compact ? _("Chat") : _("Press Enter to chat"))));
	}
	else if (chatQuickChatAvailable())
	{
		box->setPlaceholder(WzString::fromUtf8(_("Use Quick Chat to chat with other players.")));
		box->setTip(chatFreeChatDisabledByHost() ? _("The host has disabled free chat. Please use Quick Chat.")
		                                         : _("Free chat is set to \"Quick Chat Only\" in your Options. Please use Quick Chat."));
	}
	else
	{
		box->setPlaceholder(WzString::fromUtf8(chatFreeChatDisabledByHost() ? _("The host has disabled free chat.")
		                                                                    : _("Free chat is set to \"Quick Chat Only\" in your Options.")));
	}
	box->setOnReturnHandler([weakSelf](W_EDITBOX &widg) {
		const WzString text = widg.getString();
		widg.setString(WzString());
		widgScheduleTask([weakSelf, text]() {
			auto self = weakSelf.lock();
			if (!self)
			{
				return;
			}
			chatSendTypedText(text, self->chatMode);
			self->stopTyping();
			if (self->onFinished)
			{
				self->onFinished();
			}
		});
	});
	box->setOnEscapeHandler([weakSelf](W_EDITBOX &) {
		widgScheduleTask([weakSelf]() {
			auto self = weakSelf.lock();
			if (!self)
			{
				return;
			}
			self->stopTyping();
			if (self->onFinished)
			{
				self->onFinished();
			}
		});
	});
	box->setOnTabHandler([weakSelf](W_EDITBOX &) -> bool {
		if (auto self = weakSelf.lock())
		{
			if (!self->spectator)
			{
				self->setMode(self->chatMode == WzChatMode::Team ? WzChatMode::Glob : WzChatMode::Team);
			}
		}
		return true;
	});
	auto chatField = std::make_shared<ChatField>();
	chatField->initialize(box);
	field = chatField;
	attach(field);

	if (chatQuickChatAvailable())
	{
		quickButton = IntelButton::make(compact ? std::string() : std::string(_("Quick chat")), "quick-chat", false, compact ? HEIGHT_COMPACT - 2 * PAD : HEIGHT - 2 * PAD);
		quickButton->setTip(_("Quick Chat"));
		quickButton->addOnClickHandler([weakSelf](W_BUTTON &) {
			widgScheduleTask([weakSelf]() {
				if (auto self = weakSelf.lock())
				{
					self->toggleQuickChat();
				}
			});
		});
		attach(quickButton);
	}
}

int32_t HudChatBar::idealHeight()
{
	return compact ? HEIGHT_COMPACT : HEIGHT;
}

void HudChatBar::geometryChanged()
{
	const int h = height();
	int x0 = PAD;
	if (spectator)
	{
		spectatorsText.setText(WzString::fromUtf8(_("Spectators")), font_small);
		x0 += spectatorsText.width() + 16;
	}
	else
	{
		const int segment = compact ? 44 : 52;
		teamButton->setGeometry(x0, (h - SEGMENT_HEIGHT) / 2, segment, SEGMENT_HEIGHT);
		allButton->setGeometry(x0 + segment, (h - SEGMENT_HEIGHT) / 2, segment - 8, SEGMENT_HEIGHT);
		x0 += 2 * segment - 8 + 8;
	}
	int x1 = width() - PAD;
	if (quickButton)
	{
		const int bw = compact ? quickButton->idealHeight() + 8 : quickButton->idealWidth();
		quickButton->setGeometry(x1 - bw, (h - quickButton->idealHeight()) / 2, bw, quickButton->idealHeight());
		x1 -= bw + 8;
	}
	if (!compact && !spectator)
	{
		hintKey.setText(WzString::fromUtf8(_("Tab")), font_small);
		hintText.setText(WzString::fromUtf8(_("channel")), font_small);
		x1 -= hintKey.width() + 8 + 5 + hintText.width() + 10;
	}
	field->setGeometry(x0, 4, std::max(40, x1 - x0), h - 8);
}

void HudChatBar::display(int xOffset, int yOffset)
{
	const HudColors &colors = skin().colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const bool typing = isTyping();
	pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
	                   alpha(colors.panelBackground, 245), 6.f, typing ? alpha(colors.accent, 190) : pal_RGBA(120, 150, 190, 64), 1.f);
	if (spectator)
	{
		spectatorsText.render(static_cast<float>(rect.x() + PAD + 8), std::round(rect.y() + rect.height() / 2.f - spectatorsText.lineSize() / 2.f - spectatorsText.aboveBase()),
		                      colors.textSecondary);
	}
	else
	{
		// The switch's frame
		const int sx0 = rect.x() + teamButton->x();
		const int sx1 = rect.x() + allButton->x() + allButton->width();
		const int sy0 = rect.y() + teamButton->y();
		pie_DrawRoundedBox(static_cast<float>(sx0), static_cast<float>(sy0), static_cast<float>(sx1), static_cast<float>(sy0 + SEGMENT_HEIGHT),
		                   pal_RGBA(0, 0, 0, 40), 4.f, pal_RGBA(120, 150, 190, 72), 1.f);
	}
	if (!compact && !spectator && typing)
	{
		const int x1 = quickButton ? rect.x() + quickButton->x() - 10 : rect.right() - PAD - 4;
		const float baseline = std::round(rect.y() + rect.height() / 2.f - hintText.lineSize() / 2.f - hintText.aboveBase());
		const int textX = x1 - hintText.width();
		hintText.render(static_cast<float>(textX), baseline, colors.textSecondary);
		const int keyX = textX - 5 - (hintKey.width() + 8);
		const int cy = rect.y() + rect.height() / 2;
		pie_DrawRoundedBox(static_cast<float>(keyX), static_cast<float>(cy - 8), static_cast<float>(keyX + hintKey.width() + 8), static_cast<float>(cy + 8),
		                   pal_RGBA(0, 0, 0, 0), 3.f, alpha(colors.textSecondary, 150), 1.f);
		hintKey.render(static_cast<float>(keyX + 4), baseline, colors.textPrimary);
	}
}

void HudChatBar::startTyping()
{
	if (box->isEditing())
	{
		return;
	}
	virtualKeyboardRequestForNextTextEntry();
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	box->clicked(&context);
}

void HudChatBar::stopTyping()
{
	if (box->isEditing())
	{
		box->stopEditing();
	}
	if (auto screen = screenPointer.lock())
	{
		screen->setFocus(nullptr);
	}
}

bool HudChatBar::isTyping() const
{
	return box && box->isEditing();
}

void HudChatBar::setMode(WzChatMode newMode)
{
	if (spectator || newMode == chatMode)
	{
		return;
	}
	chatMode = newMode;
	if (quickChat)
	{
		// (Opened again on its tab for the channel)
		closeQuickChat();
		toggleQuickChat();
	}
}

void HudChatBar::toggleQuickChat()
{
	if (quickChat)
	{
		closeQuickChat();
		return;
	}
	auto screen = screenPointer.lock();
	if (!screen || !chatQuickChatAvailable())
	{
		return;
	}
	stopTyping();
	std::weak_ptr<HudChatBar> weakSelf = std::static_pointer_cast<HudChatBar>(shared_from_this());
	quickChat = createQuickChatForm(WzQuickChatContext::InGame, [weakSelf]() {
		widgScheduleTask([weakSelf]() {
			if (auto self = weakSelf.lock())
			{
				self->closeQuickChat();
				if (self->onFinished)
				{
					self->onFinished();
				}
			}
		});
	}, chatMode == WzChatMode::Team ? WzQuickChatMode::Team : WzQuickChatMode::Global);
	screen->psForm->attach(quickChat);
	quickChat->setCalcLayout([weakSelf](WIDGET *widget) {
		auto self = weakSelf.lock();
		if (!self)
		{
			return;
		}
		const int w = std::min<int>(static_cast<int>(screenWidth) - 40, QUICK_CHAT_MAX_WIDTH);
		const int availableHeight = std::max(40, self->screenPosY() - 18);
		const int h = std::min(widget->idealHeight(), availableHeight);
		widget->setGeometry(std::max<int>(20, (static_cast<int>(screenWidth) - w) / 2), std::max(8, self->screenPosY() - 10 - h), w, h);
	});
	quickChat->callCalcLayout();
	if (quickButton)
	{
		quickButton->active = true;
	}
}

void HudChatBar::closeQuickChat()
{
	if (!quickChat)
	{
		return;
	}
	if (auto parent = quickChat->parent())
	{
		parent->detach(quickChat);
	}
	quickChat.reset();
	if (quickButton)
	{
		quickButton->active = false;
	}
}
