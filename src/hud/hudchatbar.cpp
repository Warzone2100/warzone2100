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
#include "hudquickchat.h"
#include "intel/intelkit.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
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
		const bool disabled = (getState() & WBUT_DISABLE) != 0;
		const PIELIGHT color = disabled ? colors.stateDisabled : ((on || isHighlighted()) ? colors.textPrimary : colors.textSecondary);
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
	/// (While idle) A click asks for typing instead of starting it
	std::function<void ()> onTypeRequested;
	bool framedWhileTyping = false;

	void clicked(W_CONTEXT *, WIDGET_KEY key) override
	{
		if (key == WKEY_PRIMARY && onTypeRequested)
		{
			onTypeRequested();
		}
	}

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
		if (framedWhileTyping && box->isEditing())
		{
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
			                   pal_RGBA(10, 14, 20, 230), 4.f, skin().colors().accent, 1.f);
		}
		skin().drawNamedIcon("chat", WzRect(rect.x() + 8, rect.y() + (rect.height() - ICON) / 2, ICON, ICON),
		                     box->isEditing() ? skin().colors().accent : skin().colors().textSecondary);
	}

private:
	static constexpr int ICON = 15;
};

/// The key the bar's hint names: Tab for quick chat, else Ctrl+Tab for the channel
std::string tabHintKey()
{
	return chatQuickChatAvailable() ? _("Tab") : _("Ctrl+Tab");
}

}

std::shared_ptr<HudChannelSwitch> HudChannelSwitch::make(bool compact, std::function<WzChatMode ()> mode, std::function<void (WzChatMode)> onChosen,
                                                         bool teamEnabled)
{
	auto channelSwitch = std::make_shared<HudChannelSwitch>();
	channelSwitch->compact = compact;
	auto team = std::make_shared<ChannelButton>(_("Team"), [mode]() { return mode() == WzChatMode::Team; });
	team->setTip(_("Your team (Ctrl+Tab)"));
	if (teamEnabled)
	{
		team->addOnClickHandler([onChosen](W_BUTTON &) { onChosen(WzChatMode::Team); });
	}
	else
	{
		team->setState(WBUT_DISABLE);
		team->setTip(_("No teammates"));
	}
	auto all = std::make_shared<ChannelButton>(_("All"), [mode]() { return mode() == WzChatMode::Glob; });
	all->setTip(_("Everyone (Ctrl+Tab)"));
	all->addOnClickHandler([onChosen](W_BUTTON &) { onChosen(WzChatMode::Glob); });
	channelSwitch->teamButton = team;
	channelSwitch->allButton = all;
	channelSwitch->attach(team);
	channelSwitch->attach(all);
	return channelSwitch;
}

int32_t HudChannelSwitch::idealWidth()
{
	return compact ? 2 * 44 - 8 : 2 * 52 - 8;
}

int32_t HudChannelSwitch::idealHeight()
{
	return SEGMENT_HEIGHT;
}

void HudChannelSwitch::geometryChanged()
{
	const int segment = compact ? 44 : 52;
	teamButton->setGeometry(0, 0, segment, height());
	allButton->setGeometry(segment, 0, width() - segment, height());
}

void HudChannelSwitch::display(int xOffset, int yOffset)
{
	const float x0 = static_cast<float>(xOffset + x());
	const float y0 = static_cast<float>(yOffset + y());
	pie_DrawRoundedBox(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 40), 4.f, pal_RGBA(120, 150, 190, 72), 1.f);
}

int HudChatBar::barHeight(bool compact)
{
	return compact ? HEIGHT_COMPACT : HEIGHT;
}

void HudChatBar::setEmbedded(bool value)
{
	embedded = value;
	if (auto chatField = std::dynamic_pointer_cast<ChatField>(field))
	{
		chatField->framedWhileTyping = value;
	}
}

void HudChatBar::setIdle(std::function<void ()> onTypeRequested, std::function<void ()> onQuickChatRequested)
{
	if (auto chatField = std::dynamic_pointer_cast<ChatField>(field))
	{
		chatField->onTypeRequested = std::move(onTypeRequested);
		box->setTransparentToMouse(true);
	}
	quickChatRequested = std::move(onQuickChatRequested);
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
		channelSwitch = HudChannelSwitch::make(compact, [weakSelf]() {
			auto self = weakSelf.lock();
			return self ? self->chatMode : WzChatMode::Glob;
		}, [weakSelf](WzChatMode chosen) {
			if (auto self = weakSelf.lock())
			{
				self->setMode(chosen);
			}
		});
		attach(channelSwitch);
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
	// Tab: quick chat, Ctrl+Tab: Team / All
	box->setOnTabHandler([weakSelf](W_EDITBOX &) -> bool {
		const bool switchChannel = keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL);
		widgScheduleTask([weakSelf, switchChannel]() {
			auto self = weakSelf.lock();
			if (!self)
			{
				return;
			}
			if (switchChannel)
			{
				self->setMode(self->chatMode == WzChatMode::Team ? WzChatMode::Glob : WzChatMode::Team);
			}
			else if (chatQuickChatAvailable() && (!self->quickChat || self->stripShown))
			{
				self->toggleQuickChat();
			}
		});
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
					if (self->quickChatRequested)
					{
						self->quickChatRequested();
					}
					else
					{
						self->toggleQuickChat();
					}
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
		channelSwitch->setGeometry(x0, (h - SEGMENT_HEIGHT) / 2, channelSwitch->idealWidth(), SEGMENT_HEIGHT);
		x0 += channelSwitch->idealWidth() + 8;
	}
	int x1 = width() - PAD;
	if (quickButton && quickButton->visible())
	{
		const int bw = compact ? quickButton->idealHeight() + 8 : quickButton->idealWidth();
		quickButton->setGeometry(x1 - bw, (h - quickButton->idealHeight()) / 2, bw, quickButton->idealHeight());
		x1 -= bw + 8;
	}
	if (!compact && !spectator)
	{
		hintText.setText(WzString::fromUtf8(chatQuickChatAvailable() ? _("quick chat") : _("Team / All")), font_small);
		x1 -= keyCapWidth(hintKey, tabHintKey()) + 5 + hintText.width() + 10;
	}
	field->setGeometry(x0, 4, std::max(40, x1 - x0), h - 8);
	if (stripShown && quickChat)
	{
		quickChat->callCalcLayout();
	}
}

void HudChatBar::display(int xOffset, int yOffset)
{
	const HudColors &colors = skin().colors();
	const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
	const bool typing = isTyping();
	if (!embedded)
	{
		pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()),
		                   alpha(colors.panelBackground, 245), 6.f, typing ? alpha(colors.accent, 190) : pal_RGBA(120, 150, 190, 64), 1.f);
	}
	if (spectator)
	{
		spectatorsText.render(static_cast<float>(rect.x() + PAD + 8), std::round(rect.y() + rect.height() / 2.f - spectatorsText.lineSize() / 2.f - spectatorsText.aboveBase()),
		                      colors.textSecondary);
	}
	if (!compact && !spectator && typing)
	{
		const int x1 = (quickButton && quickButton->visible()) ? rect.x() + quickButton->x() - 10 : rect.right() - PAD - 4;
		const float baseline = std::round(rect.y() + rect.height() / 2.f - hintText.lineSize() / 2.f - hintText.aboveBase());
		const int textX = x1 - hintText.width();
		hintText.render(static_cast<float>(textX), baseline, colors.textSecondary);
		const int keyX = textX - 5 - keyCapWidth(hintKey, tabHintKey());
		drawKeyCap(hintKey, tabHintKey(), keyX, rect.y() + rect.height() / 2);
	}
}

void HudChatBar::startTyping()
{
	if (box->isEditing())
	{
		return;
	}
	if (!stripShown)
	{
		closeQuickChat();
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
		quickChat->setMode(newMode);
	}
	if (onModeChanged)
	{
		onModeChanged(newMode);
	}
}

void HudChatBar::showQuickChatStrip()
{
	auto host = parent();
	if (stripShown || !host || !chatQuickChatAvailable())
	{
		return;
	}
	stripShown = true;
	if (quickButton)
	{
		quickButton->hide();
	}
	std::weak_ptr<HudChatBar> weakSelf = std::static_pointer_cast<HudChatBar>(shared_from_this());
	quickChat = HudQuickChat::makeStrip(chatMode == WzChatMode::Team, compact);
	quickChat->onSent = [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			if (self->onFinished)
			{
				self->onFinished();
			}
		}
	};
	quickChat->onTypeRequested = [weakSelf]() {
		auto self = weakSelf.lock();
		if (self && chatFreeChatEnabled())
		{
			self->startTyping();
		}
	};
	quickChat->onResized = [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			if (self->quickChat)
			{
				self->quickChat->callCalcLayout();
			}
		}
	};
	// (Attached beside the bar, shown under it)
	host->attach(quickChat);
	quickChat->setCalcLayout([weakSelf](WIDGET *widget) {
		auto self = weakSelf.lock();
		if (!self)
		{
			return;
		}
		auto strip = std::static_pointer_cast<HudQuickChat>(widget->shared_from_this());
		const int h = strip->stripHeight(self->width());
		widget->setGeometry(self->x(), self->y() + self->height(), self->width(), h);
		if (self->onStripResized)
		{
			self->onStripResized(h);
		}
	});
	quickChat->callCalcLayout();
	geometryChanged();
}

void HudChatBar::toggleQuickChat()
{
	if (stripShown)
	{
		// Tab or the button: the strip takes the keys
		if (quickChat)
		{
			if (box->isEditing())
			{
				box->stopEditing();
			}
			quickChat->takeFocusFromTyping();
		}
		return;
	}
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
	quickChat = HudQuickChat::make(chatMode == WzChatMode::Team, compact);
	quickChat->onSent = [weakSelf]() {
		if (auto self = weakSelf.lock())
		{
			self->closeQuickChat();
			if (self->onFinished)
			{
				self->onFinished();
			}
		}
	};
	quickChat->onTypeRequested = [weakSelf]() {
		auto self = weakSelf.lock();
		if (self && chatFreeChatEnabled())
		{
			self->startTyping();
		}
	};
	quickChat->onModeChanged = [weakSelf](WzChatMode chosen) {
		if (auto self = weakSelf.lock())
		{
			self->setMode(chosen);
		}
	};
	// Attached beside the bar, so it runs before the screen's form takes the keys. Shown above the bar, at its right, at
	// most its width
	auto host = parent();
	ASSERT_OR_RETURN(, host != nullptr, "The chat bar has no parent");
	host->attach(quickChat);
	quickChat->setCalcLayout([weakSelf](WIDGET *widget) {
		auto self = weakSelf.lock();
		if (!self)
		{
			return;
		}
		const int w = std::min(widget->idealWidth(), self->width());
		const int h = widget->idealHeight();
		widget->setGeometry(self->x() + self->width() - w, std::max(8, self->y() - 6 - h), w, h);
	});
	quickChat->callCalcLayout();
	quickChat->takeFocus();
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
	stripShown = false;
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
