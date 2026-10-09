/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

#include "replay_controller.h"
#include "replay_timeline.h"

#include "lib/framework/frame.h"
#include "lib/framework/i18n.h"
#include "lib/framework/string_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/widget.h"

#include "../gamestate_replay_seek.h"
#include "../gamestate_snapshot_ring.h"
#include "../hci.h"
#include "../hud/hudskin.h"
#include "../loop.h"
#include "../notifications.h"

#include <algorithm>
#include <memory>

namespace replayplayback
{

uint32_t playheadMs()
{
	return gameTime;
}

uint32_t durationMs()
{
	return std::max(gamestate::replayEndGameTime(), gameTime);
}

bool isBusy()
{
	return gamestate::replaySeekStatus().busy;
}

uint32_t seekTargetMs()
{
	return gamestate::replaySeekStatus().target;
}

std::vector<uint32_t> snapshotTimes()
{
	return gamestate::snapshotRingStoredTimes();
}

void seekTo(uint32_t timeMs)
{
	const uint32_t target = (timeMs / REPLAY_SEEK_QUANTUM_MS) * REPLAY_SEEK_QUANTUM_MS;
	std::string error;
	if (gamestate::requestReplaySeek(target, &error))
	{
		debug(LOG_INFO, "Replay seek to gameTime %u requested from the replay controls at gameTime %u", target, gameTime);
	}
	else
	{
		debug(LOG_ERROR, "Replay seek to gameTime %u can't be done: %s", target, error.c_str());
	}
}

std::string formatTime(uint32_t timeMs, bool useHours)
{
	const uint32_t totalSec = timeMs / 1000;
	const uint32_t s = totalSec % 60;
	const uint32_t m = (totalSec / 60) % 60;
	const uint32_t h = totalSec / 3600;
	if (useHours)
	{
		return astringf("%u:%02u:%02u", h, m, s);
	}
	return astringf("%u:%02u", totalSec / 60, s);
}

std::string formatClockLabel(uint32_t playheadMs, uint32_t durationMs)
{
	const uint32_t totalMs = std::max(durationMs, playheadMs);
	if (totalMs == 0)
	{
		return "--:-- / --:--";
	}
	const bool useHours = totalMs >= 3600u * 1000u;
	return astringf("%s / %s", formatTime(playheadMs, useHours).c_str(), formatTime(totalMs, useHours).c_str());
}

} // namespace replayplayback

// ---------------------------------------------------------------------------------------------------------------------
// The transport bar

constexpr int REPLAY_TRANSPORT_PADDING = 5;
constexpr int REPLAY_TRANSPORT_MARGIN = 6;
constexpr int REPLAY_TRANSPORT_MIN_WIDTH = 420;
constexpr float REPLAY_TRANSPORT_WIDTH_FRAC = 0.6f;
constexpr int REPLAY_ACTION_BUTTONS_PADDING = 5;
constexpr int REPLAY_ACTION_BUTTONS_SPACING = 5;
constexpr int REPLAY_ACTION_BUTTONS_IMAGE_SIZE = 16;
constexpr int REPLAY_ACTION_BUTTONS_WIDTH = REPLAY_ACTION_BUTTONS_IMAGE_SIZE + (REPLAY_ACTION_BUTTONS_PADDING * 2);
constexpr int REPLAY_ACTION_BUTTONS_HEIGHT = REPLAY_ACTION_BUTTONS_WIDTH;
constexpr int REPLAY_TIMELINE_SPACING = 10;
constexpr size_t WZ_MAX_REPLAY_FASTFORWARD_TICKS = 3;

class W_REPLAY_CONTROL_BUTTON : public W_BUTTON
{
public:
	W_REPLAY_CONTROL_BUTTON() : W_BUTTON() {}

	~W_REPLAY_CONTROL_BUTTON()
	{
		delete pButtonImage;
		pButtonImage = nullptr;
		delete pAltImage;
		pAltImage = nullptr;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = x() + xOffset;
		const int y0 = y() + yOffset;
		const int x1 = x0 + width();
		const int y1 = y0 + height();

		const bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		const bool isDisabled = (getState() & WBUT_DISABLE) != 0;
		const bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

		HudReplayButtonLook look;
		look.rect = WzRect(x0, y0, width(), height());
		look.button = showAltImage ? altRole : role;
		look.down = isDown;
		look.highlighted = isHighlight;
		look.disabled = isDisabled;
		look.fastForward = (outline == OutlineState::Double) ? 4 : ((outline == OutlineState::Single) ? 2 : 0);
		if (currentHudSkin().drawReplayButton(look))
		{
			return;
		}

		const auto light_border = pal_RGBA(255, 255, 255, 100);
		const auto fill_color = isDown || isDisabled ? pal_RGBA(10, 0, 70, 200) : pal_RGBA(25, 0, 110, 175);
		iV_ShadowBox(x0, y0, x1, y1, 0, isDown ? pal_RGBA(0, 0, 0, 0) : light_border, isDisabled ? light_border : WZCOL_FORM_DARK, fill_color);

		if (isHighlight)
		{
			iV_Box(x0 + 2, y0 + 2, x1 - 2, y1 - 2, WZCOL_FORM_HILITE);
		}

		switch (outline)
		{
			case OutlineState::None:
				break;
			case OutlineState::Double:
				iV_Box(x0 + 3, y0 + 3, x1 - 3, y1 - 3, pal_RGBA(218, 207, 255, 200));
				// fall-through
			case OutlineState::Single:
				iV_Box(x0 + 1, y0 + 1, x1 - 1, y1 - 1, pal_RGBA(218, 207, 255, 200));
				break;
		}

		gfx_api::texture *image = (showAltImage && pAltImage) ? pAltImage : pButtonImage;
		if (image)
		{
			const int imageLeft = x1 - REPLAY_ACTION_BUTTONS_PADDING - REPLAY_ACTION_BUTTONS_IMAGE_SIZE;
			const int imageTop = y0 + REPLAY_ACTION_BUTTONS_PADDING;
			iV_DrawImageAnisotropic(*image, Vector2i(imageLeft, imageTop), Vector2f(0, 0),
				Vector2f(REPLAY_ACTION_BUTTONS_IMAGE_SIZE, REPLAY_ACTION_BUTTONS_IMAGE_SIZE), 0.f, isDisabled ? pal_RGBA(255, 255, 255, 110) : WZCOL_WHITE);
		}
	}

	gfx_api::texture* pButtonImage = nullptr;
	gfx_api::texture* pAltImage = nullptr; ///< (shown instead while showAltImage: the play/pause button's play)
	bool showAltImage = false;
	HudReplayButton role = HudReplayButton::Play;
	HudReplayButton altRole = HudReplayButton::Play;
	enum class OutlineState
	{
		None,
		Single,
		Double
	};
	OutlineState outline = OutlineState::None;
};

class ReplayTransportBar : public W_FORM
{
public:
	static std::shared_ptr<ReplayTransportBar> make();

	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	void run(W_CONTEXT *psContext) override;

private:
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> makeReplayActionButton(const char *imagePath, HudReplayButton role);
	void refreshButtonStates();
	void layoutContent();

	static void setPaused(bool pause);
	static void cycleFastForward(bool reverseDirection);
	static void seekRelative(int32_t deltaMs);

	std::shared_ptr<ReplayTimelineWidget> timeline;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> startButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> skipBackButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> playPauseButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> skipForwardButton;
	std::shared_ptr<W_REPLAY_CONTROL_BUTTON> fastForwardButton;
	std::shared_ptr<W_LABEL> timeLabel;
	UDWORD lastUpdateTime = 0;
};

void ReplayTransportBar::setPaused(bool pause)
{
	if (pause)
	{
		if (!gameTimeIsStopped())
		{
			gameTimeStop();
		}
		setAudioPause(true);
		setConsolePause(true);
	}
	else
	{
		setAudioPause(false);
		setConsolePause(false);
		if (gameTimeIsStopped())
		{
			gameTimeStart();
		}
	}
}

void ReplayTransportBar::cycleFastForward(bool reverseDirection)
{
	const auto currFastForward = getMaxFastForwardTicks();
	size_t newFastForward = 0;
	if (currFastForward == 0)
	{
		newFastForward = (!reverseDirection) ? 1 : WZ_MAX_REPLAY_FASTFORWARD_TICKS;
	}
	else if (currFastForward == 1)
	{
		newFastForward = (!reverseDirection) ? WZ_MAX_REPLAY_FASTFORWARD_TICKS : 0;
	}
	else if (reverseDirection && currFastForward == WZ_MAX_REPLAY_FASTFORWARD_TICKS)
	{
		newFastForward = 1;
	}
	setMaxFastForwardTicks(newFastForward, true);
}

void ReplayTransportBar::seekRelative(int32_t deltaMs)
{
	const uint32_t playhead = replayplayback::playheadMs();
	uint32_t target = playhead;
	if (deltaMs < 0)
	{
		const uint32_t back = static_cast<uint32_t>(-deltaMs);
		target = (playhead < back) ? 0 : playhead - back;
	}
	else
	{
		target = playhead + static_cast<uint32_t>(deltaMs);
	}
	replayplayback::seekTo(target);
}

std::shared_ptr<W_REPLAY_CONTROL_BUTTON> ReplayTransportBar::makeReplayActionButton(const char *imagePath, HudReplayButton role)
{
	auto button = std::make_shared<W_REPLAY_CONTROL_BUTTON>();
	button->role = role;
	button->altRole = role;
	button->style |= WBUT_SECONDARY;
	button->FontID = font_regular;
	button->pButtonImage = WZ_Notification_Image(imagePath).loadImageToTexture();
	button->setGeometry(0, 0, REPLAY_ACTION_BUTTONS_WIDTH, REPLAY_ACTION_BUTTONS_HEIGHT);
	return button;
}

std::shared_ptr<ReplayTransportBar> ReplayTransportBar::make()
{
	auto result = std::make_shared<ReplayTransportBar>();
	std::weak_ptr<ReplayTransportBar> weakBar = result;
	auto refreshSoon = [weakBar]() {
		widgScheduleTask([weakBar]() {
			if (auto bar = weakBar.lock())
			{
				bar->refreshButtonStates();
			}
		});
	};

	result->timeline = ReplayTimelineWidget::make();
	result->attach(result->timeline);

	result->startButton = result->makeReplayActionButton("images/replay/skip_start.png", HudReplayButton::Start);
	result->startButton->setTip(_("Back to the start"));
	result->attach(result->startButton);
	result->startButton->addOnClickHandler([](W_BUTTON&) {
		replayplayback::seekTo(0);
	});

	result->skipBackButton = result->makeReplayActionButton("images/replay/skip_backward.png", HudReplayButton::Back);
	result->skipBackButton->setTip(_("Back 15 seconds"));
	result->attach(result->skipBackButton);
	result->skipBackButton->addOnClickHandler([](W_BUTTON&) {
		seekRelative(-static_cast<int32_t>(REPLAY_SKIP_STEP_MS));
	});

	// One button: pause while playing, resume while paused
	result->playPauseButton = result->makeReplayActionButton("images/replay/pause.png", HudReplayButton::Pause);
	result->playPauseButton->pAltImage = WZ_Notification_Image("images/replay/play.png").loadImageToTexture();
	result->playPauseButton->altRole = HudReplayButton::Play;
	result->playPauseButton->setTip(_("Pause"));
	result->attach(result->playPauseButton);
	result->playPauseButton->addOnClickHandler([refreshSoon](W_BUTTON&) {
		setPaused(!gameTimeIsStopped());
		refreshSoon();
	});

	result->skipForwardButton = result->makeReplayActionButton("images/replay/skip_forward.png", HudReplayButton::Forward);
	result->skipForwardButton->setTip(_("Forward 15 seconds"));
	result->attach(result->skipForwardButton);
	result->skipForwardButton->addOnClickHandler([](W_BUTTON&) {
		seekRelative(static_cast<int32_t>(REPLAY_SKIP_STEP_MS));
	});

	result->fastForwardButton = result->makeReplayActionButton("images/replay/fast_forward.png", HudReplayButton::FastForward);
	result->fastForwardButton->setTip(_("Fast-Forward"));
	result->attach(result->fastForwardButton);
	result->fastForwardButton->addOnClickHandler([refreshSoon](W_BUTTON& button) {
		cycleFastForward(button.getOnClickButtonPressed() == WKEY_SECONDARY);
		refreshSoon();
	});

	result->timeLabel = std::make_shared<W_LABEL>();
	result->timeLabel->setFont(font_regular, WZCOL_FORM_TEXT);
	result->timeLabel->setTextAlignment(WLAB_ALIGNRIGHT);
	result->timeLabel->setCacheNeverExpires(true);
	result->attach(result->timeLabel);

	// One row: the buttons, the timeline, the time
	const int barHeight = REPLAY_TRANSPORT_PADDING + REPLAY_ACTION_BUTTONS_HEIGHT + REPLAY_TRANSPORT_PADDING;
	result->setGeometry(0, 0, REPLAY_TRANSPORT_MIN_WIDTH, barHeight);
	result->refreshButtonStates();
	return result;
}

void ReplayTransportBar::display(int xOffset, int yOffset)
{
	const int x0 = x() + xOffset;
	const int y0 = y() + yOffset;
	if (!currentHudSkin().drawReplayBar(WzRect(x0, y0, width(), height())))
	{
		pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 160));
	}
}

void ReplayTransportBar::geometryChanged()
{
	layoutContent();
}

void ReplayTransportBar::run(W_CONTEXT *)
{
	if (realTime - lastUpdateTime >= GAME_TICKS_PER_UPDATE)
	{
		refreshButtonStates();
	}
}

void ReplayTransportBar::refreshButtonStates()
{
	const bool busy = replayplayback::isBusy();
	const bool paused = gameTimeIsStopped();
	playPauseButton->setState(busy ? WBUT_DISABLE : 0);
	playPauseButton->showAltImage = paused;
	playPauseButton->setTip(paused ? _("Resume") : _("Pause"));

	const auto maxFastForwardTicks = getMaxFastForwardTicks();
	if (maxFastForwardTicks > 0)
	{
		fastForwardButton->outline = (maxFastForwardTicks >= WZ_MAX_REPLAY_FASTFORWARD_TICKS)
			? W_REPLAY_CONTROL_BUTTON::OutlineState::Double
			: W_REPLAY_CONTROL_BUTTON::OutlineState::Single;
	}
	else
	{
		fastForwardButton->outline = W_REPLAY_CONTROL_BUTTON::OutlineState::None;
	}

	const uint32_t playhead = replayplayback::playheadMs();
	const uint32_t duration = replayplayback::durationMs();
	const bool atStart = playhead < REPLAY_SEEK_QUANTUM_MS;
	startButton->setState((busy || atStart) ? WBUT_DISABLE : 0);
	skipBackButton->setState((busy || atStart) ? WBUT_DISABLE : 0);
	skipForwardButton->setState((busy || playhead + REPLAY_SEEK_QUANTUM_MS >= duration) ? WBUT_DISABLE : 0);

	timeLabel->setString(WzString::fromUtf8(replayplayback::formatClockLabel(playhead, duration)));
	lastUpdateTime = realTime;
	layoutContent();
}

void ReplayTransportBar::layoutContent()
{
	if (!timeline)
	{
		return;
	}
	int x = REPLAY_TRANSPORT_PADDING;
	const int buttonsY = (height() - REPLAY_ACTION_BUTTONS_HEIGHT) / 2;
	for (const auto &button : {startButton, skipBackButton, playPauseButton, skipForwardButton, fastForwardButton})
	{
		button->setGeometry(x, buttonsY, button->width(), button->height());
		x += button->width() + REPLAY_ACTION_BUTTONS_SPACING;
	}

	// The time at the right, as wide as the total time both sides (the digits are all as wide), so the timeline keeps
	// its length while it plays
	const uint32_t duration = replayplayback::durationMs();
	const std::string widest = replayplayback::formatClockLabel(duration, duration);
	const int labelWidth = std::max(static_cast<int>(iV_GetTextWidth(widest.c_str(), font_regular)), static_cast<int>(timeLabel->getMaxLineWidth()));
	const int labelHeight = iV_GetTextLineSize(font_regular);
	const int labelX = width() - REPLAY_TRANSPORT_PADDING - 3 - labelWidth;
	timeLabel->setGeometry(labelX, (height() - labelHeight) / 2, labelWidth, labelHeight);

	const int timelineX = x - REPLAY_ACTION_BUTTONS_SPACING + REPLAY_TIMELINE_SPACING;
	const int timelineW = std::max(1, labelX - REPLAY_TIMELINE_SPACING - timelineX);
	timeline->setGeometry(timelineX, (height() - REPLAY_PLAYHEAD_SIZE) / 2, timelineW, REPLAY_PLAYHEAD_SIZE);
}

// ---------------------------------------------------------------------------------------------------------------------
// The overlay while a seek is under way: the world dimmed, where the seek goes and how far it is, Esc to stop it

class ReplaySeekOverlay : public W_FULLSCREENOVERLAY_CLICKFORM
{
public:
	static std::shared_ptr<ReplaySeekOverlay> make();

	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;

	ReplaySeekOverlay() : W_FULLSCREENOVERLAY_CLICKFORM() {}
};

std::shared_ptr<ReplaySeekOverlay> ReplaySeekOverlay::make()
{
	auto overlay = std::make_shared<ReplaySeekOverlay>();
	overlay->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	}));
	overlay->backgroundColor = pal_RGBA(0, 0, 0, 0);
	overlay->setTransparentToMouse(true);
	overlay->swallowsInput = false;
	overlay->onCancelPressed = []() {
		gamestate::replaySeekStop();
	};
	return overlay;
}

void ReplaySeekOverlay::display(int xOffset, int yOffset)
{
	const gamestate::ReplaySeekStatus status = gamestate::replaySeekStatus();
	if (!status.busy)
	{
		return;
	}
	pie_UniTransBoxFill(xOffset, yOffset, xOffset + width(), yOffset + height(), pal_RGBA(0, 0, 0, 100));

	const uint32_t duration = replayplayback::durationMs();
	const bool useHours = duration >= 3600u * 1000u;
	const std::string targetText = replayplayback::formatTime(status.target, useHours);
	const std::string title = status.restarting
		? astringf(_("Playing the replay again from the start, to %s"), targetText.c_str())
		: astringf(_("Seeking to %s"), targetText.c_str());
	const char *hint = _("(Press ESC to stop here)");

	const int titleW = iV_GetTextWidth(title.c_str(), font_large);
	const int titleH = iV_GetTextLineSize(font_large);
	const int hintW = iV_GetTextWidth(hint, font_regular);
	const int hintH = iV_GetTextLineSize(font_regular);
	const int gap = 10;
	const int barW = std::min(360, static_cast<int>(width()) - 40);
	const int barH = 6;
	const int blockH = titleH + gap + barH + gap + hintH;
	const int top = yOffset + (height() - blockH) / 2;

	iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	iV_DrawText(title.c_str(), xOffset + (width() - titleW) / 2, top + titleH, font_large);

	// How far the fast-forward is
	const int barX = xOffset + (width() - barW) / 2;
	const int barY = top + titleH + gap;
	pie_UniTransBoxFill(barX, barY, barX + barW, barY + barH, pal_RGBA(255, 255, 255, 50));
	if (!status.restarting && status.target > status.from)
	{
		const uint32_t done = std::min(gameTime, status.target) - std::min(status.from, gameTime);
		const int doneW = static_cast<int>((static_cast<uint64_t>(done) * barW) / (status.target - status.from));
		pie_UniTransBoxFill(barX, barY, barX + std::min(doneW, barW), barY + barH, pal_RGBA(218, 207, 255, 200));
	}

	iV_SetTextColour(WZCOL_TEXT_MEDIUM);
	iV_DrawText(hint, xOffset + (width() - hintW) / 2, barY + barH + gap + hintH, font_regular);
}

void ReplaySeekOverlay::run(W_CONTEXT *psContext)
{
	const bool busy = replayplayback::isBusy();
	setTransparentToMouse(!busy);
	swallowsInput = busy;
	if (busy)
	{
		W_FULLSCREENOVERLAY_CLICKFORM::run(psContext);
	}
}

static std::shared_ptr<W_SCREEN> replayOverlayScreen;

bool createReplayControllerOverlay()
{
	ASSERT(psWScreen != nullptr, "psWScreen is not initialized yet?");
	ASSERT_OR_RETURN(false, replayOverlayScreen == nullptr, "Already initialized");

	replayOverlayScreen = W_SCREEN::make();
	replayOverlayScreen->psForm->hide();

	// The overlay first, so the bar is drawn over it
	replayOverlayScreen->psForm->attach(ReplaySeekOverlay::make());

	auto bar = ReplayTransportBar::make();
	replayOverlayScreen->psForm->attach(bar);
	bar->setCalcLayout([](WIDGET *psWidget) {
		// Centered at the bottom, clear of the radar in the bottom-left corner on most screens
		const int maxW = std::max(1, static_cast<int>(screenWidth) - 2 * REPLAY_TRANSPORT_MARGIN);
		const int w = std::min(maxW, std::max(REPLAY_TRANSPORT_MIN_WIDTH, static_cast<int>(static_cast<float>(screenWidth) * REPLAY_TRANSPORT_WIDTH_FRAC)));
		psWidget->setGeometry((static_cast<int>(screenWidth) - w) / 2, static_cast<int>(screenHeight) - psWidget->height() - REPLAY_TRANSPORT_MARGIN, w, psWidget->height());
	});

	widgRegisterOverlayScreenOnTopOfScreen(replayOverlayScreen, psWScreen);
	return true;
}

void destroyReplayControllerOverlay()
{
	if (replayOverlayScreen)
	{
		widgRemoveOverlayScreen(replayOverlayScreen);
		replayOverlayScreen = nullptr;
	}
}

void replayOverlayScreenSizeDidChange()
{
	if (replayOverlayScreen)
	{
		replayOverlayScreen->screenSizeDidChange(screenWidth, screenHeight, screenWidth, screenHeight);
	}
}
