/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/wzapp.h"
#include "joiningscreen.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/scrollablelist.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/netplay/netplay.h"
#include "lib/netplay/netsocket.h"

#include "../hci.h"
#include "../activity.h"
#include "../multistat.h"
#include "../modding.h"
#include "../warzoneconfig.h"
#include "../titleui/titleui.h"
#include "../titleui/multiplayer.h"
#include "../multiint.h"

#include <chrono>
#include <algorithm>

class WzJoiningGameScreen_HandlerRoot;
struct WzJoiningGameScreen;

// MARK: - Globals

static std::weak_ptr<WzJoiningGameScreen> psCurrentJoiningAttemptScreen;

void shutdownJoiningAttemptInternal(std::shared_ptr<W_SCREEN> expectedScreen);

constexpr int NET_READ_TIMEOUT = 0;
constexpr size_t NET_BUFFER_SIZE = MaxMsgSize;
constexpr uint32_t HOST_RESPONSE_TIMEOUT = 10000;

// MARK: - WzJoiningStatusForm

class WzJoiningActionButton : public W_BUTTON
{
protected:
	WzJoiningActionButton() {}
	void initialize(const WzString& text);
public:
	static std::shared_ptr<WzJoiningActionButton> make(const WzString& text)
	{
		class make_shared_enabler: public WzJoiningActionButton {};
		auto widget = std::make_shared<make_shared_enabler>();
		widget->initialize(text);
		return widget;
	}
protected:
	void display(int xOffset, int yOffset) override;
private:
	WzText wzText;
	const int InternalPadding = 12;
};

void WzJoiningActionButton::initialize(const WzString& text)
{
	setString(text);
	FontID = font_regular_bold;
	int minButtonWidthForText = iV_GetTextWidth(pText, FontID);
	setGeometry(0, 0, minButtonWidthForText + InternalPadding, iV_GetTextLineSize(FontID) + InternalPadding);
}

void WzJoiningActionButton::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	bool haveText = !pText.isEmpty();

	bool isDown = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
	bool isDisabled = (getState() & WBUT_DISABLE) != 0;
	bool isHighlight = !isDisabled && ((getState() & WBUT_HIGHLIGHT) != 0);

	PIELIGHT colour;
	if (isDisabled)								// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else										// available
	{
		if (isHighlight)						// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else									// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	if (isHighlight || isDown)
	{
		PIELIGHT fillClr = pal_RGBA(255, 255, 255, 30);
		pie_UniTransBoxFill(x0, y0, x1, y1, fillClr);
	}
	iV_Box(x0, y0, x1, y1, colour);

	if (haveText)
	{
		wzText.setText(pText, FontID);
		int fw = wzText.width();
		int fx = x0 + (width() - fw) / 2;
		int fy = y0 + (height() - wzText.lineSize()) / 2 - wzText.aboveBase();
		wzText.render(fx, fy, colour);
	}
}

class WzJoiningPasswordPrompt : public WIDGET
{
public:
	typedef std::function<void (WzString)> OnSubmitFunc;
protected:
	WzJoiningPasswordPrompt() { }
	void initialize(const OnSubmitFunc& onSubmit);
public:
	static std::shared_ptr<WzJoiningPasswordPrompt> make(const OnSubmitFunc& onSubmit);
	void setOnSubmit(const OnSubmitFunc& _onSubmit)
	{
		onSubmit = _onSubmit;
	}
	virtual int32_t idealHeight() override;
	void givePasswordBoxFocus();
	void clearAndSetCurrentEntryAsInvalid();
	void setDisabled(bool disabled);
protected:
	void geometryChanged() override
	{
		recalcLayout();
	}
private:
	void recalcLayout();
	void submitPassword(const WzString& str);
private:
	std::shared_ptr<W_EDITBOX> passwordBox;
	std::shared_ptr<WzJoiningActionButton> okayButton;
	OnSubmitFunc onSubmit;
	bool invalidPasswordPlaceholder = false;
};

int32_t WzJoiningPasswordPrompt::idealHeight()
{
	return std::max<int32_t>(passwordBox->height(), okayButton->height());
}

void WzJoiningPasswordPrompt::givePasswordBoxFocus()
{
	// give the password box the focus
	W_CONTEXT context = W_CONTEXT::ZeroContext();
	passwordBox->simulateClick(&context, true);
}

void WzJoiningPasswordPrompt::clearAndSetCurrentEntryAsInvalid()
{
	auto oldPass = passwordBox->getString();
	if (oldPass.isEmpty())
	{
		return;
	}

	passwordBox->setString("");
	passwordBox->setPlaceholder(oldPass);
	passwordBox->setPlaceholderTextColor(pal_RGBA(255,0,0,170));
	invalidPasswordPlaceholder = true;
}

std::shared_ptr<WzJoiningPasswordPrompt> WzJoiningPasswordPrompt::make(const OnSubmitFunc& onSubmit)
{
	class make_shared_enabler: public WzJoiningPasswordPrompt {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize(onSubmit);
	return result;
}

void WzJoiningPasswordPrompt::submitPassword(const WzString& str)
{
	if (str.isEmpty())
	{
		// don't send empty password
		auto weakSelf = std::weak_ptr<WzJoiningPasswordPrompt>(std::dynamic_pointer_cast<WzJoiningPasswordPrompt>(shared_from_this()));
		widgScheduleTask([weakSelf]() {
			// give it back focus
			auto strongParent = weakSelf.lock();
			ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
			strongParent->givePasswordBoxFocus();
		});
		return;
	}

	if (onSubmit)
	{
		onSubmit(str);
	}
}

void WzJoiningPasswordPrompt::setDisabled(bool disabled)
{
	if (disabled)
	{
		// ensure passwordBox does not have focus
		if (auto lockedScreen = screenPointer.lock())
		{
			if (lockedScreen->hasFocus(*passwordBox))
			{
				lockedScreen->setFocus(nullptr);
			}
		}
	}
	passwordBox->setState((disabled) ? WEDBS_DISABLE : 0);
	okayButton->setState((disabled) ? WBUT_DISABLE : 0);
}

void WzJoiningPasswordPrompt::initialize(const OnSubmitFunc& _onSubmit)
{
	onSubmit = _onSubmit;

	auto weakSelf = std::weak_ptr<WzJoiningPasswordPrompt>(std::dynamic_pointer_cast<WzJoiningPasswordPrompt>(shared_from_this()));

	passwordBox = std::make_shared<W_EDITBOX>();
	attach(passwordBox);
	passwordBox->setGeometry(0, 0, 280, 20);
	passwordBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
	passwordBox->setMaxStringSize(password_string_size);
	passwordBox->setPlaceholder(_("Enter password"));

	passwordBox->setOnReturnHandler([weakSelf](W_EDITBOX& widg) {
		WzString str = widg.getString();
		auto strongParent = weakSelf.lock();
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
		strongParent->submitPassword(str);
	});
	passwordBox->setOnEditingStoppedHandler([weakSelf](W_EDITBOX&){
		auto strongParent = weakSelf.lock();
		ASSERT_OR_RETURN(, strongParent != nullptr, "No parent?");
		if (strongParent->invalidPasswordPlaceholder)
		{
			strongParent->passwordBox->setPlaceholder(_("Enter password"));
			strongParent->passwordBox->setPlaceholderTextColor(nullopt);
			strongParent->invalidPasswordPlaceholder = false;
		}
	});

	okayButton = WzJoiningActionButton::make(_("OK"));
	attach(okayButton);
	okayButton->addOnClickHandler([weakSelf](W_BUTTON&) {
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent?");
		strongSelf->submitPassword(strongSelf->passwordBox->getString());
	});
}

void WzJoiningPasswordPrompt::recalcLayout()
{
	int w = width();
	int h = height();

	int buttonX0 = w - okayButton->width();
	int buttonY0 = (h - okayButton->height()) / 2;
	okayButton->setGeometry(buttonX0, buttonY0, okayButton->width(), h);

	int passwordBoxWidth = std::max<int>(buttonX0 - 5, 0);
	int passwordBoxY0 = (h - passwordBox->height()) / 2;
	passwordBox->setGeometry(0, passwordBoxY0, passwordBoxWidth, h);
}

class WzJoiningIndeterminateIndicatorWidget : public WIDGET
{
public:
	WzJoiningIndeterminateIndicatorWidget(iV_fonts fontID)
	: WIDGET()
	{
		periodText.setText(".", fontID);
	}

	void display(int xOffset, int yOffset) override;

	int32_t idealWidth() override
	{
		return (periodText.width() * InstanceCount) + (PaddingBetween * (InstanceCount - 1));
	}

	int32_t idealHeight() override
	{
		return periodText.lineSize();
	}
private:
	WzText periodText;
	static constexpr int PaddingBetween = 2;
	static constexpr int InstanceCount = 3;
	static constexpr int AnimTimePerInstance = 750;
	static constexpr uint32_t TotalAnimTime = AnimTimePerInstance * InstanceCount;
};

void WzJoiningIndeterminateIndicatorWidget::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	int timeFactor = static_cast<int>(realTime % TotalAnimTime);
	const int perInstanceAnim = AnimTimePerInstance;

	int textY0 = y0 + (height() - periodText.lineSize()) / 2 - periodText.aboveBase();

	int periodTextWidth = periodText.width();
	for (int i = 0; i < InstanceCount; i++)
	{
		int offsetX0 = i * (periodTextWidth + PaddingBetween);
		PIELIGHT bckColor = WZCOL_TEXT_MEDIUM;
		int a = 100 + (155 * clip<int>(timeFactor - (i * perInstanceAnim), 0, perInstanceAnim) / perInstanceAnim);
		bckColor.byte.a = static_cast<UBYTE>(a);
		periodText.render(x0 + offsetX0, textY0, bckColor);
	}
}

class WzJoiningStatusForm : public W_FORM
{
protected:
	WzJoiningStatusForm() { }
	void initialize();
public:
	static std::shared_ptr<WzJoiningStatusForm> make();

	void displayProgressStatus(const WzString& statusDescription);
	void displaySuccessStatus(const WzString& statusDescription);
	void displayUnableToJoinError(const WzString& errorDetails);
	void displayRejectionMessage(const WzString& rejectionMessageFromHost);

	typedef std::function<void (WzString)> PasswordSubmitFunc;
	void displayPasswordPrompt(const PasswordSubmitFunc& submitFunc);

	int32_t idealWidth() override;
	int32_t idealHeight() override;
	int32_t maximumHeight();
protected:
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}
	void display(int xOffset, int yOffset) override;
private:
	void recalcLayout();
	int32_t calculateNeededHeight(bool withDetailsParagraph);
	void updateTitle(const WzString& title);
	void updateStatusTitle(const WzString& title);
	void displayStatus(const WzString& statusDescription);
	void displayDetailsParagraph(const WzString& messageContents);
private:
	std::shared_ptr<W_LABEL> titleLabel;
	std::shared_ptr<WzJoiningIndeterminateIndicatorWidget> progressIndicator;
	std::shared_ptr<W_LABEL> statusTitle;
	std::shared_ptr<W_LABEL> statusDetails;

	// details paragraph
	std::shared_ptr<Paragraph> detailsParagraph;
	std::shared_ptr<ScrollableListWidget> scrollableParagraphContainer;

	std::shared_ptr<WzJoiningPasswordPrompt> passwordPrompt;
	std::shared_ptr<WzJoiningActionButton> actionButton;
	std::chrono::steady_clock::time_point startTime;
	static constexpr int TitleContentsPadding = 20;
	static constexpr int32_t InternalPadding = 20;
	static constexpr int32_t DetailsLabelParagraphPadding = 5;
	static constexpr int32_t MaxParagraphHeight = 185;
};

int32_t WzJoiningStatusForm::idealWidth()
{
	return 540;
}

int32_t WzJoiningStatusForm::idealHeight()
{
	return calculateNeededHeight((detailsParagraph != nullptr));
}

int32_t WzJoiningStatusForm::maximumHeight()
{
	return calculateNeededHeight(true);
}

int32_t WzJoiningStatusForm::calculateNeededHeight(bool withDetailsParagraph)
{
	int32_t result = (InternalPadding * 2);
	result += titleLabel->height();
	result += TitleContentsPadding;
	result += statusTitle->height();
	result += DetailsLabelParagraphPadding;
	if (withDetailsParagraph)
	{
		result += MaxParagraphHeight;
	}
	else
	{
		result += statusDetails->idealHeight();
	}
	result += InternalPadding;
	if (passwordPrompt && passwordPrompt->visible())
	{
		result += passwordPrompt->idealHeight() + InternalPadding;
	}
	result += actionButton->height();
	return result;
}

std::shared_ptr<WzJoiningStatusForm> WzJoiningStatusForm::make()
{
	class make_shared_enabler: public WzJoiningStatusForm {};
	auto result = std::make_shared<make_shared_enabler>();
	result->initialize();
	return result;
}

void WzJoiningStatusForm::initialize()
{
	startTime = std::chrono::steady_clock::now();

	titleLabel = std::make_shared<W_LABEL>();
	attach(titleLabel);
	titleLabel->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
	titleLabel->setString(_("Connecting to Game"));
	titleLabel->setGeometry(0, 0, titleLabel->getMaxLineWidth(), iV_GetTextLineSize(font_medium_bold));
	titleLabel->setCanTruncate(true);

	progressIndicator = std::make_shared<WzJoiningIndeterminateIndicatorWidget>(font_medium_bold);
	attach(progressIndicator);
	progressIndicator->setGeometry(0, 0, progressIndicator->idealWidth(), progressIndicator->idealHeight());

	statusTitle = std::make_shared<W_LABEL>();
	attach(statusTitle);
	statusTitle->setFont(font_regular_bold, WZCOL_TEXT_MEDIUM);
	statusTitle->setString(_("Status:"));
	statusTitle->setGeometry(0, 0, statusTitle->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
	statusTitle->setCanTruncate(true);

	statusDetails = std::make_shared<W_LABEL>();
	attach(statusDetails);
	statusDetails->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	statusDetails->setString("");
	statusDetails->setGeometry(0, 0, statusDetails->getMaxLineWidth(), iV_GetTextLineSize(font_regular));
	statusDetails->setCanTruncate(true);

	actionButton = WzJoiningActionButton::make(_("Cancel"));
	attach(actionButton);
	actionButton->addOnClickHandler([](W_BUTTON& but) {
		shutdownJoiningAttemptInternal(but.screenPointer.lock());
	});

	scrollableParagraphContainer = ScrollableListWidget::make();
	scrollableParagraphContainer->setGeometry(0, 0, 400, 45);
	attach(scrollableParagraphContainer);
}

void WzJoiningStatusForm::recalcLayout()
{
	int w = width();
	int h = height();

	if (w <= 0 && h <= 0)
	{
		return;
	}

	int usableWidth = w - (InternalPadding * 2);
	int usableHeight = h - (InternalPadding * 2);

	int indicatorWidth = progressIndicator->idealWidth();
	int indicatorX0 = (w - InternalPadding - indicatorWidth);
	progressIndicator->setGeometry(indicatorX0, InternalPadding, indicatorWidth, progressIndicator->idealHeight());

	int titleUsableWidth = usableWidth - indicatorWidth - InternalPadding;
	titleLabel->setGeometry(InternalPadding, InternalPadding, titleUsableWidth, iV_GetTextLineSize(font_medium_bold));
	int lastLineY1 = titleLabel->y() + titleLabel->height();
	usableHeight -= titleLabel->height();

	statusTitle->setGeometry(InternalPadding, lastLineY1 + TitleContentsPadding, usableWidth, statusTitle->height());
	lastLineY1 = statusTitle->y() + statusTitle->height();
	usableHeight -= TitleContentsPadding + titleLabel->height();

	// position button at bottom
	int actionButtonX0 = (width() - actionButton->width()) / 2;
	int actionButtonY0 = height() - InternalPadding - actionButton->height();
	actionButton->setGeometry(actionButtonX0, actionButtonY0, actionButton->width(), actionButton->height());
	usableHeight -= actionButton->height() + InternalPadding;

	if (passwordPrompt && passwordPrompt->visible())
	{
		int passwordPromptY0 = actionButton->y() - InternalPadding - passwordPrompt->idealHeight();
		passwordPrompt->setGeometry(InternalPadding, passwordPromptY0, usableWidth, passwordPrompt->idealHeight());
		usableHeight -= passwordPrompt->height() + InternalPadding;
	}

	// both of these start at the same place, taking up the remaining vertical space, but only one is displayed at a time

	int statusDetailsX0 = InternalPadding;
	int statusDetailsWidth = usableWidth;
	statusDetails->setGeometry(statusDetailsX0, lastLineY1 + DetailsLabelParagraphPadding, statusDetailsWidth, statusDetails->idealHeight());

	int paragraphHeight = usableHeight - DetailsLabelParagraphPadding;
	scrollableParagraphContainer->setGeometry(InternalPadding, lastLineY1 + DetailsLabelParagraphPadding, usableWidth, paragraphHeight);
}

void WzJoiningStatusForm::updateTitle(const WzString& title)
{
	titleLabel->setString(title);
}

void WzJoiningStatusForm::updateStatusTitle(const WzString& title)
{
	statusTitle->setString(title);
}

void WzJoiningStatusForm::displayStatus(const WzString &statusDescription)
{
	if (detailsParagraph)
	{
		scrollableParagraphContainer->clear();
		detailsParagraph.reset();
	}
	scrollableParagraphContainer->hide();

	statusDetails->setFormattedString(statusDescription, std::numeric_limits<uint32_t>::max(), font_regular);
	statusDetails->show();
}

void WzJoiningStatusForm::displayDetailsParagraph(const WzString& messageContents)
{
	if (detailsParagraph)
	{
		scrollableParagraphContainer->clear();
	}
	detailsParagraph = std::make_shared<Paragraph>();
	int usableWidth = width() - (InternalPadding * 2);
	detailsParagraph->setGeometry(InternalPadding, InternalPadding, usableWidth, MaxParagraphHeight);
	detailsParagraph->setFontColour(WZCOL_TEXT_BRIGHT);
	detailsParagraph->setLineSpacing(5);
	detailsParagraph->setFont(font_regular);

	std::string messageContentsStr = messageContents.toUtf8();
	size_t maxLinePos = nthOccurrenceOfChar(messageContentsStr, '\n', 10);
	if (maxLinePos != std::string::npos)
	{
		messageContentsStr = messageContentsStr.substr(0, maxLinePos);
	}
	detailsParagraph->addText(WzString::fromUtf8(messageContentsStr));

	scrollableParagraphContainer->addItem(detailsParagraph);

	scrollableParagraphContainer->show();
	statusDetails->hide();
}

void WzJoiningStatusForm::displayProgressStatus(const WzString &statusDescription)
{
	displayStatus(statusDescription);
	progressIndicator->show();
}

void WzJoiningStatusForm::displaySuccessStatus(const WzString& statusDescription)
{
	displayStatus(statusDescription);
	actionButton->setState(WBUT_DISABLE);
	actionButton->hide();
}

void WzJoiningStatusForm::displayPasswordPrompt(const PasswordSubmitFunc& submitFunc)
{
	auto weakSelf = std::weak_ptr<WzJoiningStatusForm>(std::dynamic_pointer_cast<WzJoiningStatusForm>(shared_from_this()));
	auto wrappedSubmitFunc = [weakSelf, submitFunc](WzString str) {
		submitFunc(str);
		auto strongSelf = weakSelf.lock();
		if (strongSelf)
		{
			strongSelf->passwordPrompt->setDisabled(true);
		}
	};
	if (!passwordPrompt)
	{
		passwordPrompt = WzJoiningPasswordPrompt::make(wrappedSubmitFunc);
		attach(passwordPrompt);
	}
	else
	{
		passwordPrompt->setOnSubmit(wrappedSubmitFunc);
		passwordPrompt->clearAndSetCurrentEntryAsInvalid();
	}
	passwordPrompt->show();
	passwordPrompt->setDisabled(false);
	displayStatus(_("Game requires a password to join"));
	passwordPrompt->givePasswordBoxFocus();
	recalcLayout();
}

void WzJoiningStatusForm::displayUnableToJoinError(const WzString& errorDetails)
{
	if (passwordPrompt)
	{
		passwordPrompt->hide();
	}
	progressIndicator->hide();
	titleLabel->setString(_("Unable to Join Game"));
	statusTitle->setString(_("Error:"));
	displayStatus(errorDetails);
	actionButton->setString(_("Close"));
}

void WzJoiningStatusForm::displayRejectionMessage(const WzString& rejectionMessageFromHost)
{
	if (passwordPrompt)
	{
		passwordPrompt->hide();
	}
	progressIndicator->hide();
	titleLabel->setString(_("Join Attempt Rejected"));
	statusTitle->setString(_("Message from Host:"));
	displayDetailsParagraph(rejectionMessageFromHost);
	actionButton->setString(_("Close"));
}

void WzJoiningStatusForm::run(W_CONTEXT *)
{
	// currently, no-op
}

void WzJoiningStatusForm::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();

	// draw "drop-shadow"
	int dropShadowX0 = std::max<int>(x0 - 6, 0);
	int dropShadowY0 = std::max<int>(y0 - 6, 0);
	int dropShadowX1 = std::min<int>(x1 + 6, pie_GetVideoBufferWidth());
	int dropShadowY1 = std::min<int>(y1 + 6, pie_GetVideoBufferHeight());
	pie_UniTransBoxFill((float)dropShadowX0, (float)dropShadowY0, (float)dropShadowX1, (float)dropShadowY1, pal_RGBA(0, 0, 0, 40));

	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 50));
	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(0, 0, 0, 70));

	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_NOTIFICATION_BOX);
	iV_Box2(x0, y0, x1, y1, WZCOL_FORM_DARK, WZCOL_FORM_DARK);
	iV_Box2(x0 - 1, y0 - 1, x1 + 1, y1 + 1, pal_RGBA(255, 255, 255, 50), pal_RGBA(255, 255, 255, 50));
}

// MARK: - WzJoiningGameScreen definition

typedef std::function<void (const JoinConnectionDescription& connection)> JoinSuccessHandler;
typedef std::function<void ()> JoinFailureHandler;

struct WzJoiningGameScreen: public W_SCREEN
{
protected:
	WzJoiningGameScreen(): W_SCREEN() {}
	~WzJoiningGameScreen()
	{
		setFocus(nullptr);
	}

public:
	static std::shared_ptr<WzJoiningGameScreen> make(const std::vector<JoinConnectionDescription>& connectionList, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, const JoinSuccessHandler& onSuccessFunc, const JoinFailureHandler& onFailureFunc);

public:
	void closeScreen();

private:
	WzJoiningGameScreen(WzJoiningGameScreen const &) = delete;
	WzJoiningGameScreen &operator =(WzJoiningGameScreen const &) = delete;
};

// MARK: - WzJoiningGameScreen_HandlerRoot

class WzJoiningGameScreen_HandlerRoot : public W_CLICKFORM
{
protected:
	static constexpr int32_t INTERNAL_PADDING = 15;
protected:
	WzJoiningGameScreen_HandlerRoot(const W_FORMINIT *init);
	~WzJoiningGameScreen_HandlerRoot();
	void initialize();
	void recalcLayout();
public:
	static std::shared_ptr<WzJoiningGameScreen_HandlerRoot> make(const std::vector<JoinConnectionDescription>& connectionList, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, const JoinSuccessHandler& onSuccessFunc, const JoinFailureHandler& onFailureFunc);
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
	void run(W_CONTEXT *psContext) override;
	void geometryChanged() override
	{
		recalcLayout();
	}

private:
	void attemptToOpenConnection(size_t connectionIdx);

	// Process a connection result (to be called from non-main threads)
	void processOpenConnectionResultOnMainThread(size_t connectionIdx, OpenConnectionResult&& result);

	// Process a connection result - **Only to be called on the main thread**
	void processOpenConnectionResult(size_t connectionIdx, OpenConnectionResult&& result);

	// called from run()
	void processJoining();

	// internal helpers
	void closeConnectionAttempt();
	bool joiningSocketNETsend();
	void handleSuccess();

	// displaying status info / state
	void promptForPassword();
	void updateJoiningStatus(const WzString& statusDescription);
	void handleJoinTimeoutError();

	struct FailureDetails
	{
	protected:
		FailureDetails() {}
	public:
		static FailureDetails makeFromLobbyError(LOBBY_ERROR_TYPES resultCode);
		static FailureDetails makeFromInternalError(const WzString& details);
		static FailureDetails makeFromHostRejectionMessage(const WzString& message);
	public:
		WzString resultMessage;
		bool hostProvidedMessage = false;
	};
	void handleFailure(const FailureDetails& failureDetails);

public:
	PIELIGHT backgroundColor = pal_RGBA(0, 0, 0, 80);
	std::function<void ()> onClickedFunc;
	std::function<void ()> onCancelPressed;
private:
	std::vector<JoinConnectionDescription> connectionList;
	WzString playerName;
	EcKey playerIdentity;
	bool asSpectator = false;
	char gamePassword[password_string_size] = {};
	size_t currentConnectionIdx = 0;

	JoinSuccessHandler onSuccessFunc;
	JoinFailureHandler onFailureFunc;

	enum class JoiningState
	{
		NeedsPassword,								// Waiting for user to enter a password
		AwaitingConnection,							// Waiting for background thread to (hopefully) yield an open connection (socket)
		AwaitingInitialNetcodeHandshakeAck,			// Waiting for response to initial netcode version handshake
		ProcessingJoinMessages,						// Waiting for initial join-related net messages, and responding as needed
		Failure,									// Join attempt failed
		SuccessPendingClose,						// Join attempt was successful - handed off to onSuccessFunc, but still waiting to close display
		Success										// Success
	};
	JoiningState currentJoiningState = JoiningState::AwaitingConnection;

	const char* to_string(JoiningState s);
	const char* to_display_str(JoiningState s);
	const char* to_localized_state_fail_desc(JoiningState s);

	// state when handling initial connection join
	uint32_t startTime = 0;
	Socket* client_transient_socket = nullptr;
	SocketSet* tmp_joining_socket_set = nullptr;
	NETQUEUE tmpJoiningQUEUE = {};
	NetQueuePair *tmpJoiningQueuePair = nullptr;
	char initialAckBuffer[10] = {'\0'};
	size_t usedInitialAckBuffer = 0;
	const size_t expectedInitialAckSize = sizeof(uint32_t);

	std::chrono::steady_clock::time_point timeStarted;
	const std::chrono::milliseconds minimumTimeBeforeAutoClose = std::chrono::milliseconds(300);

	std::shared_ptr<WzJoiningStatusForm> joiningProgressForm;
};

WzJoiningGameScreen_HandlerRoot::FailureDetails WzJoiningGameScreen_HandlerRoot::FailureDetails::makeFromLobbyError(LOBBY_ERROR_TYPES resultCode)
{
	const char* txt = nullptr;
	switch (resultCode)
	{
	case ERROR_NOERROR:
		break;
	case ERROR_FULL:
		txt = _("Game is full");
		break;
	case ERROR_KICKED:
		txt = _("You were kicked!");
		break;
	case ERROR_WRONGVERSION:
		txt = _("Your game version does not match the host");
		break;
	case ERROR_WRONGDATA:
		txt = _("The host rejected your connection due to invalid data");
		break;
	case ERROR_HOSTDROPPED:
		txt = _("Host has dropped connection!");
		break;
	// error codes not currently used by the host on join:
	case ERROR_UNKNOWNFILEISSUE:
	case ERROR_WRONGPASSWORD:
		ASSERT(false, "Unexpected lobby error from host join: %d", (int)resultCode);
		// fall-through
	case ERROR_CONNECTION:
	default:
		txt = _("Connection Error");
		break;
	}
	auto result = FailureDetails();
	result.resultMessage = txt;
	result.hostProvidedMessage = false;
	return result;
}

WzJoiningGameScreen_HandlerRoot::FailureDetails WzJoiningGameScreen_HandlerRoot::FailureDetails::makeFromInternalError(const WzString &details)
{
	auto result = FailureDetails();
	result.resultMessage = details;
	result.hostProvidedMessage = false;
	return result;
}

WzJoiningGameScreen_HandlerRoot::FailureDetails WzJoiningGameScreen_HandlerRoot::FailureDetails::makeFromHostRejectionMessage(const WzString &message)
{
	auto result = FailureDetails();
	result.resultMessage = message;
	result.hostProvidedMessage = true;
	return result;
}

WzJoiningGameScreen_HandlerRoot::WzJoiningGameScreen_HandlerRoot(W_FORMINIT const *init)
: W_CLICKFORM(init)
{ }

WzJoiningGameScreen_HandlerRoot::~WzJoiningGameScreen_HandlerRoot()
{
	// close any in-progress connection attempt
	closeConnectionAttempt();
	currentJoiningState = JoiningState::Failure;
}

std::shared_ptr<WzJoiningGameScreen_HandlerRoot> WzJoiningGameScreen_HandlerRoot::make(const std::vector<JoinConnectionDescription>& connectionList, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, const JoinSuccessHandler& onSuccessFunc, const JoinFailureHandler& onFailureFunc)
{
	W_FORMINIT sInit;
	sInit.id = 0;
	sInit.style = WFORM_PLAIN | WFORM_CLICKABLE;
	sInit.x = 0;
	sInit.y = 0;
	sInit.width = screenWidth - 1;
	sInit.height = screenHeight - 1;
	sInit.calcLayout = LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	});

	class make_shared_enabler: public WzJoiningGameScreen_HandlerRoot
	{
	public:
		make_shared_enabler(const W_FORMINIT *init) : WzJoiningGameScreen_HandlerRoot(init) {}
	};
	auto widget = std::make_shared<make_shared_enabler>(&sInit);
	widget->connectionList = connectionList;
	widget->playerName = playerName;
	widget->playerIdentity = playerIdentity;
	widget->asSpectator = asSpectator;
	widget->onSuccessFunc = onSuccessFunc;
	widget->onFailureFunc = onFailureFunc;

	widget->initialize();
	return widget;
}

void WzJoiningGameScreen_HandlerRoot::initialize()
{
	timeStarted = std::chrono::steady_clock::now();

	auto weakSelf = std::weak_ptr<WzJoiningGameScreen_HandlerRoot>(std::dynamic_pointer_cast<WzJoiningGameScreen_HandlerRoot>(shared_from_this()));

	// Create progress display form
	joiningProgressForm = WzJoiningStatusForm::make();
	attach(joiningProgressForm);
	joiningProgressForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psProgressForm = dynamic_cast<WzJoiningStatusForm*>(psWidget);
		auto psParent = psProgressForm->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int desiredWidth = psProgressForm->idealWidth();
		int desiredHeight = psProgressForm->idealHeight();
		int x0 = (psParent->width() - desiredWidth) / 2;
		int y0 = (psParent->height() - psProgressForm->maximumHeight()) / 2;
		psWidget->setGeometry(x0, y0, desiredWidth, desiredHeight);
	}));

	recalcLayout();

	// Kick-off the initial connection attempt
	if (connectionList.empty())
	{
		debug(LOG_ERROR, "No connections to try");
		handleFailure(FailureDetails::makeFromInternalError(_("No connections available")));
		return;
	}
	attemptToOpenConnection(0);
}

void WzJoiningGameScreen_HandlerRoot::handleSuccess()
{
	currentJoiningState = JoiningState::SuccessPendingClose;
	joiningProgressForm->displaySuccessStatus(_("Synchronizing data with host ..."));
	ASSERT_OR_RETURN(, onSuccessFunc != nullptr, "Missing success handler!");
	const auto& connDesc = connectionList[currentConnectionIdx];
	onSuccessFunc(connDesc);
}

void WzJoiningGameScreen_HandlerRoot::promptForPassword()
{
	currentJoiningState = JoiningState::NeedsPassword;

	auto weakSelf = std::weak_ptr<WzJoiningGameScreen_HandlerRoot>(std::dynamic_pointer_cast<WzJoiningGameScreen_HandlerRoot>(shared_from_this()));
	joiningProgressForm->displayPasswordPrompt([weakSelf](WzString newPassword) {
		widgScheduleTask([weakSelf, newPassword]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent form?");
			ASSERT_OR_RETURN(, strongSelf->currentJoiningState == JoiningState::NeedsPassword, "Unexpected state");
			sstrcpy(strongSelf->gamePassword, newPassword.toUtf8().c_str());
			// re-attempt join with last used connection
			strongSelf->attemptToOpenConnection(strongSelf->currentConnectionIdx);
		});
	});
	joiningProgressForm->callCalcLayout();
}

void WzJoiningGameScreen_HandlerRoot::updateJoiningStatus(const WzString& statusDescription)
{
	debug(LOG_NET, "%s", statusDescription.toUtf8().c_str());
	joiningProgressForm->displayProgressStatus(statusDescription);
}

const char* WzJoiningGameScreen_HandlerRoot::to_string(JoiningState s)
{
	switch (s) {
		case JoiningState::NeedsPassword: return "NeedsPassword";
		case JoiningState::AwaitingConnection: return "AwaitingConnection";
		case JoiningState::AwaitingInitialNetcodeHandshakeAck: return "AwaitingInitialNetcodeHandshakeAck";
		case JoiningState::ProcessingJoinMessages: return "ProcessingJoinMessages";
		case JoiningState::Failure: return "Failure";
		case JoiningState::SuccessPendingClose: return "SuccessPendingClose";
		case JoiningState::Success: return "Success";
	}
	return ""; // silence compiler warning
}

const char* WzJoiningGameScreen_HandlerRoot::to_display_str(JoiningState s)
{
	switch (s) {
		case JoiningState::NeedsPassword: return "NeedsPassword";
		case JoiningState::AwaitingConnection: return "PendingConnect";
		case JoiningState::AwaitingInitialNetcodeHandshakeAck: return "NetcodeHandshake";
		case JoiningState::ProcessingJoinMessages: return "ProcessingJoin";
		case JoiningState::Failure: return "Failure";
		case JoiningState::SuccessPendingClose: return "SuccessPendingClose";
		case JoiningState::Success: return "Success";
	}
	return ""; // silence compiler warning
}

const char* WzJoiningGameScreen_HandlerRoot::to_localized_state_fail_desc(JoiningState s)
{
	switch (s) {
		case JoiningState::NeedsPassword:
			return _("Waiting for correct join password");
		case JoiningState::AwaitingConnection:
			return _("Attempting to connect");
		case JoiningState::AwaitingInitialNetcodeHandshakeAck:
			return _("Establishing connection handshake");
		case JoiningState::ProcessingJoinMessages:
			return _("Coordinating join with host");
		case JoiningState::Failure:
			return _("Join attempt failed");
		case JoiningState::SuccessPendingClose:
		case JoiningState::Success:
			return "";
	}
	return ""; // silence compiler warning
}

void WzJoiningGameScreen_HandlerRoot::handleJoinTimeoutError()
{
	debug(LOG_INFO, "Failed to join with timeout, state: %s", to_string(currentJoiningState));

	WzString timeoutErrorDetails = _("Host did not respond before timeout");
	timeoutErrorDetails += "\n";
	WzString localizedJoinStateDesc = to_localized_state_fail_desc(currentJoiningState);
	if (!localizedJoinStateDesc.isEmpty())
	{
		timeoutErrorDetails += WzString::fromUtf8(astringf(_("Failed at: [%s] - %s"), to_display_str(currentJoiningState), localizedJoinStateDesc.toUtf8().c_str()));
	}
	else
	{
		timeoutErrorDetails += WzString::fromUtf8(astringf(_("Failed at: [%s]"), to_display_str(currentJoiningState)));
	}

	currentJoiningState = JoiningState::Failure;

	joiningProgressForm->displayUnableToJoinError(timeoutErrorDetails);
	joiningProgressForm->callCalcLayout();

	if (onFailureFunc)
	{
		onFailureFunc();
	}

	ActivityManager::instance().joinGameFailed(connectionList);
}

void WzJoiningGameScreen_HandlerRoot::handleFailure(const FailureDetails& failureDetails)
{
	currentJoiningState = JoiningState::Failure;

	debug(LOG_INFO, "Failed to join with error: %s", failureDetails.resultMessage.toUtf8().c_str());
	if (!failureDetails.hostProvidedMessage)
	{
		joiningProgressForm->displayUnableToJoinError(failureDetails.resultMessage);
	}
	else
	{
		joiningProgressForm->displayRejectionMessage(failureDetails.resultMessage);
	}
	joiningProgressForm->callCalcLayout();

	if (onFailureFunc)
	{
		onFailureFunc();
	}

	ActivityManager::instance().joinGameFailed(connectionList);
}

void WzJoiningGameScreen_HandlerRoot::recalcLayout()
{
	joiningProgressForm->callCalcLayout();
}

void WzJoiningGameScreen_HandlerRoot::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	if (onClickedFunc)
	{
		onClickedFunc();
	}
}

void WzJoiningGameScreen_HandlerRoot::display(int xOffset, int yOffset)
{
	if (!visible())
	{
		return; // skip if hidden
	}

	if (backgroundColor.rgba == 0)
	{
		return;
	}

	int x0 = x() + xOffset;
	int y0 = y() + yOffset;

	// draw background over everything
	pie_UniTransBoxFill(x0, y0, x0 + width(), y0 + height(), backgroundColor);
}

void WzJoiningGameScreen_HandlerRoot::run(W_CONTEXT *psContext)
{
	if (keyPressed(KEY_ESC))
	{
		// while this is displayed, clear the input buffer
		// (ensuring that subsequent screens / the main screen do not get the input to handle)
		inputLoseFocus();

		if (onCancelPressed)
		{
			onCancelPressed();
		}
		return;
	}

	// while this is displayed, clear the input buffer
	// (ensuring that subsequent screens / the main screen do not get the input to handle)
	inputLoseFocus();

	processJoining();
}

void WzJoiningGameScreen_HandlerRoot::processOpenConnectionResultOnMainThread(size_t connectionIdx, OpenConnectionResult&& result)
{
	auto weakSelf = std::weak_ptr<WzJoiningGameScreen_HandlerRoot>(std::dynamic_pointer_cast<WzJoiningGameScreen_HandlerRoot>(shared_from_this()));

	// Work around the fact that we need to copy a move-only type into the lambda (which is stored as a std::function)
	auto pResult = std::make_unique<OpenConnectionResult>(std::move(result));
	auto ppResult = std::make_shared<std::unique_ptr<OpenConnectionResult>>(std::move(pResult));

	wzAsyncExecOnMainThread([weakSelf, connectionIdx, ppResult]() mutable {
		auto strongSelf = weakSelf.lock();
		if (!strongSelf)
		{
			// background thread ultimately returned after the requester has gone away - just return
			return;
		}
		strongSelf->processOpenConnectionResult(connectionIdx, std::move(**ppResult));
	});
}

void WzJoiningGameScreen_HandlerRoot::processOpenConnectionResult(size_t connectionIdx, OpenConnectionResult&& result)
{
	ASSERT_OR_RETURN(, currentJoiningState == JoiningState::AwaitingConnection, "Not awaiting connection? (Ignoring)");
	ASSERT_OR_RETURN(, connectionIdx == currentConnectionIdx, "Received result for connection %zu, but should be processing: %zu", connectionIdx, currentConnectionIdx);

	if (result.hasError())
	{
		if ((connectionIdx+1) < connectionList.size())
		{
			// try the next connection
			attemptToOpenConnection(++connectionIdx);
		}
		else
		{
			debug(LOG_ERROR, "%s", result.errorString.c_str());
			// Done trying connections - all failed
			const auto errCode = result.errorCode.value();
			const auto sockErrorMsg = errCode.message();
			auto localizedError = astringf(_("Failed to open connection: [%d] %s"), errCode.value(), sockErrorMsg.c_str());
			handleFailure(FailureDetails::makeFromInternalError(WzString::fromUtf8(localizedError)));
		}
		return;
	}

	// take ownership of the socket
	client_transient_socket = result.open_socket.release();

	if (NETgetEnableTCPNoDelay())
	{
		// Enable TCP_NODELAY
		socketSetTCPNoDelay(*client_transient_socket, true);
	}

	// Send initial connection data: NETCODE_VERSION_MAJOR and NETCODE_VERSION_MINOR
	char buffer[sizeof(int32_t) * 2] = { 0 };
	char *p_buffer = buffer;
	auto pushu32 = [&](uint32_t value) {
		uint32_t swapped = htonl(value);
		memcpy(p_buffer, &swapped, sizeof(swapped));
		p_buffer += sizeof(swapped);
	};
	pushu32(NETGetMajorVersion());
	pushu32(NETGetMinorVersion());

	const auto writeResult = writeAll(*client_transient_socket, buffer, sizeof(buffer));
	if (!writeResult.has_value())
	{
		const auto writeErrMsg = writeResult.error().message();
		debug(LOG_ERROR, "Couldn't send my version: %s", writeErrMsg.c_str());
		closeConnectionAttempt();
		return;
	}

	tmp_joining_socket_set = allocSocketSet();
	if (tmp_joining_socket_set == nullptr)
	{
		debug(LOG_ERROR, "Cannot create socket set - out of memory?");
		closeConnectionAttempt();
		return;
	}
	debug(LOG_NET, "Created socket_set %p", static_cast<void *>(tmp_joining_socket_set));

	// `client_transient_socket` is used to talk to host machine
	SocketSet_AddSocket(*tmp_joining_socket_set, client_transient_socket);

	// Create temporary NETQUEUE
	auto NETnetJoinTmpQueue = [&]()
	{
		NETQUEUE ret;
		NetQueuePair **queue = &tmpJoiningQueuePair;
		ret.queue = queue;
		ret.isPair = true;
		ret.index = NET_HOST_ONLY;
		ret.queueType = QUEUE_TRANSIENT_JOIN;
		ret.exclude = NET_NO_EXCLUDE;
		return ret;
	};
	tmpJoiningQUEUE = NETnetJoinTmpQueue();
	NETinitQueue(tmpJoiningQUEUE);

	// transition to next internal state, handling responses in processJoining()
	currentJoiningState = JoiningState::AwaitingInitialNetcodeHandshakeAck;
	startTime = wzGetTicks();
}

void WzJoiningGameScreen_HandlerRoot::attemptToOpenConnection(size_t connectionIdx)
{
	ASSERT_OR_RETURN(, connectionIdx < connectionList.size(), "Invalid connectionIdx: %zu", connectionIdx);
	ASSERT_OR_RETURN(, currentJoiningState == JoiningState::AwaitingConnection || currentJoiningState == JoiningState::NeedsPassword, "Unexpected joining state: %d", (int)currentJoiningState);
	currentJoiningState = JoiningState::AwaitingConnection;
	currentConnectionIdx = connectionIdx;
	JoinConnectionDescription& description = connectionList[connectionIdx];
	switch (description.type)
	{
		case JoinConnectionDescription::JoinConnectionType::TCP_DIRECT:
			if (description.port == 0)
			{
				description.port = NETgetGameserverPort(); // use default configured port
			}
			auto weakSelf = std::weak_ptr<WzJoiningGameScreen_HandlerRoot>(std::dynamic_pointer_cast<WzJoiningGameScreen_HandlerRoot>(shared_from_this()));
			socketOpenTCPConnectionAsync(description.host, description.port, [weakSelf, connectionIdx](OpenConnectionResult&& result) {
				auto strongSelf = weakSelf.lock();
				if (!strongSelf)
				{
					// background thread ultimately returned after the requester has gone away (join was cancelled?) - just return
					return;
				}
				strongSelf->processOpenConnectionResultOnMainThread(connectionIdx, std::move(result));
			});
			break;
	}
	updateJoiningStatus(_("Establishing connection with host"));
}

bool WzJoiningGameScreen_HandlerRoot::joiningSocketNETsend()
{
	NetQueue *queue = &tmpJoiningQueuePair->send;
	NetMessage const *message = &queue->getMessageForNet();
	uint8_t *rawData = message->rawDataDup();
	ssize_t rawLen   = message->rawLen();
	size_t compressedRawLen = 0;
	const auto writeResult = writeAll(*client_transient_socket, rawData, rawLen, &compressedRawLen);
	delete[] rawData;  // Done with the data.
	queue->popMessageForNet();
	if (writeResult.has_value())
	{
		// success writing to socket
		debug(LOG_NET, "Wrote initial message to socket to host");
	}
	else
	{
		const auto writeErrMsg = writeResult.error().message();
		// Write error, most likely host disconnect.
		debug(LOG_ERROR, "Failed to send message (type: %" PRIu8 ", rawLen: %zu, compressedRawLen: %zu) to host: %s", message->type, message->rawLen(), compressedRawLen, writeErrMsg.c_str());
		return false;
	}
	socketFlush(*client_transient_socket, NET_HOST_ONLY);  // Make sure the message was completely sent.
	ASSERT(queue->numMessagesForNet() == 0, "Queue not empty (%u messages remaining).", queue->numMessagesForNet());
	return true;
}

void WzJoiningGameScreen_HandlerRoot::closeConnectionAttempt()
{
	if (client_transient_socket)
	{
		if (tmp_joining_socket_set)
		{
			SocketSet_DelSocket(*tmp_joining_socket_set, client_transient_socket);
		}
		socketClose(client_transient_socket);
		client_transient_socket = nullptr;
	}
	if (tmp_joining_socket_set)
	{
		deleteSocketSet(tmp_joining_socket_set);
		tmp_joining_socket_set = nullptr;
	}
	if (tmpJoiningQueuePair)
	{
		delete tmpJoiningQueuePair;
		tmpJoiningQueuePair = nullptr;
	}
	initialAckBuffer[0] = '\0';
	usedInitialAckBuffer = 0;
}

void WzJoiningGameScreen_HandlerRoot::processJoining()
{
	if (currentJoiningState == JoiningState::Success)
	{
		return;
	}
	if (currentJoiningState == JoiningState::SuccessPendingClose)
	{
		// close the joining screen after a minimum duration has passed from when it opened (to prevent a "flickering" effect)
		auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timeStarted);
		if (timePassed >= minimumTimeBeforeAutoClose)
		{
			currentJoiningState = JoiningState::Success;
			shutdownJoiningAttemptInternal(screenPointer.lock());
		}
		return;
	}
	if (currentJoiningState == JoiningState::NeedsPassword)
	{
		// waiting for user to enter + submit password
		return;
	}
	if (currentJoiningState == JoiningState::AwaitingConnection || currentJoiningState == JoiningState::Failure)
	{
		// nothing much to do until background thread hands over a connection
		return;
	}

	if (client_transient_socket == nullptr)
	{
		// internal error - client_transient_socket should be valid
		closeConnectionAttempt();
		handleFailure(FailureDetails::makeFromInternalError(_("An internal error occurred"))); // internal error
		return;
	}

	if ((unsigned)wzGetTicks() - startTime > HOST_RESPONSE_TIMEOUT)
	{
		// exceeded timeout
		closeConnectionAttempt();
		handleJoinTimeoutError();
		return;
	}

	// Handling JoiningStatus::AwaitingInitialNetcodeHandshakeAck
	if (currentJoiningState == JoiningState::AwaitingInitialNetcodeHandshakeAck)
	{
		// read in data, if we have it
		if (checkSockets(*tmp_joining_socket_set, NET_READ_TIMEOUT) > 0)
		{
			if (!socketReadReady(*client_transient_socket))
			{
				return; // wait for next check
			}

			char *p_buffer = initialAckBuffer;
			const auto readResult = readNoInt(*client_transient_socket, p_buffer + usedInitialAckBuffer, expectedInitialAckSize - usedInitialAckBuffer);
			if (readResult.has_value())
			{
				usedInitialAckBuffer += static_cast<size_t>(readResult.value());
			}

			if (usedInitialAckBuffer >= expectedInitialAckSize)
			{
				uint32_t result = ERROR_CONNECTION;
				memcpy(&result, initialAckBuffer, sizeof(result));
				result = ntohl(result);
				if (result != ERROR_NOERROR)
				{
					debug(LOG_ERROR, "Received error %d", result);

					closeConnectionAttempt();

					if (result == ERROR_WRONGPASSWORD)
					{
						// We didn't send a password yet, so this is an invalid response - just treat as a generic failure
						result = ERROR_CONNECTION;
					}
					if (result == ERROR_INVALID)
					{
						// Not expected to be sent by hosts on join, map to generic failure
						result = ERROR_CONNECTION;
					}
					handleFailure(FailureDetails::makeFromLobbyError((LOBBY_ERROR_TYPES)result));
					return;
				}

				// transition to net message mode (enable compression, wait for messages)
				socketBeginCompression(*client_transient_socket);
				currentJoiningState = JoiningState::ProcessingJoinMessages;
				// permit fall-through to currentJoiningState == JoiningState::ProcessingJoinMessage case below
			}
		}
	}

	// Handling JoiningStatus::ProcessingJoinMessages
	if (currentJoiningState == JoiningState::ProcessingJoinMessages)
	{
		// read in data, if we have it
		if (checkSockets(*tmp_joining_socket_set, NET_READ_TIMEOUT) > 0)
		{
			if (!socketReadReady(*client_transient_socket))
			{
				return; // wait for next check
			}

			uint8_t readBuffer[NET_BUFFER_SIZE];
			const auto readResult = readNoInt(*client_transient_socket, readBuffer, sizeof(readBuffer));
			if (!readResult.has_value())
			{
				// disconnect or programmer error
				if (readResult.error() == std::errc::timed_out || readResult.error() == std::errc::connection_reset)
				{
					debug(LOG_NET, "Client socket disconnected.");
				}
				else
				{
					const auto readErrMsg = readResult.error().message();
					debug(LOG_NET, "Client socket encountered error: %s", readErrMsg.c_str());
				}
				NETlogEntry("Client socket disconnected (allowJoining)", SYNC_FLAG, startTime);
				debug(LOG_NET, "freeing temp socket %p (%d)", static_cast<void *>(client_transient_socket), __LINE__);
				closeConnectionAttempt();
				handleFailure(FailureDetails::makeFromInternalError(_("Invalid host - disconnected")));
				return;
			}
			else
			{
				NETinsertRawData(tmpJoiningQUEUE, readBuffer, static_cast<size_t>(readResult.value()));
			}
		}

		if (!NETisMessageReady(tmpJoiningQUEUE))
		{
			// need to wait for a full and complete join message
			// sanity check
			if (NETincompleteMessageDataBuffered(tmpJoiningQUEUE) > (NET_BUFFER_SIZE * 16))	// something definitely big enough to encompass the expected message(s) at this point
			{
				// host is sending data that doesn't appear to be a properly formatted message - cut it off
				closeConnectionAttempt();
				handleFailure(FailureDetails::makeFromInternalError(_("Invalid host response")));
				return;
			}
			return; // nothing to do until more data comes in
		}

		uint8_t msgType = NETgetMessage(tmpJoiningQUEUE)->type;

		if (msgType == NET_ACCEPTED)
		{
			// :)
			uint8_t index;
			uint32_t hostPlayer = MAX_CONNECTED_PLAYERS + 1; // invalid host index

			NETbeginDecode(tmpJoiningQUEUE, NET_ACCEPTED);
			// Retrieve the player ID the game host arranged for us
			NETuint8_t(&index);
			NETuint32_t(&hostPlayer); // and the host player idx
			NETend();
			NETpop(tmpJoiningQUEUE);

			if (hostPlayer >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad host player number (%" PRIu32 ") received from host!", NetPlay.hostPlayer);
				closeConnectionAttempt();
				handleFailure(FailureDetails::makeFromInternalError(_("Invalid host response")));
				return;
			}

			if (index >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad player number (%u) received from host!", index);
				closeConnectionAttempt();
				handleFailure(FailureDetails::makeFromInternalError(_("Invalid host response")));
				return;
			}

			// On success, promote the temporary socket / socketset / queuepair to their permanent (stable) locations, owned by netplay
			// (Function consumes the socket-related inputs)
			if (!NETpromoteJoinAttemptToEstablishedConnectionToHost(hostPlayer, index, playerName.toUtf8().c_str(), tmpJoiningQUEUE, &client_transient_socket, &tmp_joining_socket_set))
			{
				debug(LOG_ERROR, "Failed to promote to established host connection");
				closeConnectionAttempt();
				handleFailure(FailureDetails::makeFromInternalError(_("An internal error occurred")));
				return;
			}
			// tmpJoiningQueuePair is now "owned" by NetPlay.hostPlayer's netQueue - do not delete it here!
			tmpJoiningQueuePair = nullptr;

			handleSuccess();
			return;
		}
		else if (msgType == NET_REJECTED)
		{
			uint8_t rejection = 0;
			char reason[MAX_JOIN_REJECT_REASON] = {};

			NETbeginDecode(tmpJoiningQUEUE, NET_REJECTED);
			NETuint8_t(&rejection);
			NETstring(reason, MAX_JOIN_REJECT_REASON);
			NETend();
			NETpop(tmpJoiningQUEUE);

			debug(LOG_NET, "NET_REJECTED received. Error code: %u", (unsigned int) rejection);

			closeConnectionAttempt();

			if (rejection == ERROR_WRONGPASSWORD)
			{
				currentJoiningState = JoiningState::NeedsPassword;
				promptForPassword();
			}
			else
			{
				size_t reasonStrLength = strnlen(reason, MAX_JOIN_REJECT_REASON);
				if (reasonStrLength > 0)
				{
					handleFailure(FailureDetails::makeFromHostRejectionMessage(reason));
				}
				else
				{
					handleFailure(FailureDetails::makeFromLobbyError((LOBBY_ERROR_TYPES)rejection));
				}
			}
			return;
		}
		else if (msgType == NET_PING)
		{
			updateJoiningStatus(_("Requesting to join game"));

			std::vector<uint8_t> challenge(NETgetJoinConnectionNETPINGChallengeSize(), 0);
			NETbeginDecode(tmpJoiningQUEUE, NET_PING);
			NETbytes(&challenge, NETgetJoinConnectionNETPINGChallengeSize() * 4);
			NETend();
			NETpop(tmpJoiningQUEUE);

			EcKey::Sig challengeResponse = playerIdentity.sign(challenge.data(), challenge.size());
			EcKey::Key identity = playerIdentity.toBytes(EcKey::Public);
			uint8_t playerType = (!asSpectator) ? NET_JOIN_PLAYER : NET_JOIN_SPECTATOR;
			const auto& modListStr = getModList();

			NETbeginEncode(tmpJoiningQUEUE, NET_JOIN);
			NETstring(playerName.toUtf8().c_str(), std::min<uint16_t>(StringSize, playerName.toUtf8().size() + 1));
			NETstring(modListStr.c_str(), std::min<uint16_t>(modlist_string_size, modListStr.size() + 1));
			NETstring(gamePassword, sizeof(gamePassword));
			NETuint8_t(&playerType);
			NETbytes(&identity);
			NETbytes(&challengeResponse);
			NETend(); // because of QUEUE_TRANSIENT_JOIN type, this won't trigger a NETsend() - we must write ourselves
			joiningSocketNETsend();
			// and now we wait for the host to respond with a further message
			return;
		}
		else
		{
			debug(LOG_ERROR, "Unexpected %s.", messageTypeToString(msgType));
			NETpop(tmpJoiningQUEUE);
			// Disconnect and treat as a failure
			closeConnectionAttempt();
			handleFailure(FailureDetails::makeFromInternalError(_("Invalid host response")));
			return;
		}
	}
}

// MARK: - WzJoiningGameScreen

std::shared_ptr<WzJoiningGameScreen> WzJoiningGameScreen::make(const std::vector<JoinConnectionDescription>& connectionList, const WzString& playerName, const EcKey& playerIdentity, bool asSpectator, const JoinSuccessHandler& onSuccessFunc, const JoinFailureHandler& onFailureFunc)
{
	class make_shared_enabler: public WzJoiningGameScreen {};
	auto newRootFrm = WzJoiningGameScreen_HandlerRoot::make(connectionList, playerName, playerIdentity, asSpectator, onSuccessFunc, onFailureFunc);
	auto screen = std::make_shared<make_shared_enabler>();
	screen->initialize(newRootFrm);
	std::weak_ptr<WzJoiningGameScreen> psWeakHelpOverlayScreen(screen);
	newRootFrm->onCancelPressed = [psWeakHelpOverlayScreen]() {
		auto psOverlayScreen = psWeakHelpOverlayScreen.lock();
		if (psOverlayScreen)
		{
			psOverlayScreen->closeScreen();
		}
	};

	return screen;
}

void WzJoiningGameScreen::closeScreen()
{
	shutdownJoiningAttemptInternal(shared_from_this());
}

// Internal helpers

static void handleJoinSuccess(const JoinConnectionDescription& connection, const PLAYERSTATS& playerStats)
{
	ingame.localJoiningInProgress = true;

	// send initial messages of player data (stats, color request, etc)
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);

	if (selectedPlayer < MAX_PLAYERS && war_getMPcolour() >= 0)
	{
		SendColourRequest(selectedPlayer, war_getMPcolour());
	}

	// switch the TitleUI to the multiplayer options (lobby), which will take over handling messages from the host
	changeTitleUI(std::make_shared<WzMultiplayerOptionsTitleUI>(wzTitleUICurrent));

	ActivityManager::instance().joinGameSucceeded(connection.host.c_str(), connection.port);

	// NOTE: Do *NOT* close the overlay screen here - this will be handled automatically after a minimum duration has passed
}

static void handleJoinFailure()
{
	// NOTE: Do *NOT* close the overlay screen here - let the user dismiss it when they are done reading the failure message
}

void shutdownJoiningAttemptInternal(std::shared_ptr<W_SCREEN> expectedScreen)
{
	widgRemoveOverlayScreen(expectedScreen);
	if (auto strongJoiningAttemptScreen = psCurrentJoiningAttemptScreen.lock())
	{
		ASSERT_OR_RETURN(, strongJoiningAttemptScreen == expectedScreen, "Current joining attempt screen does not match - ignoring");
		strongJoiningAttemptScreen.reset();
		psCurrentJoiningAttemptScreen.reset();
	}
}

// MARK: - Public API

bool startJoiningAttempt(char* playerName, std::vector<JoinConnectionDescription> connection_list, bool asSpectator /*= false*/)
{
	ASSERT_OR_RETURN(false, !connection_list.empty(), "Empty connection_list?");

	const auto currentGameMode = ActivityManager::instance().getCurrentGameMode();
	if (currentGameMode != ActivitySink::GameMode::MENUS)
	{
		// Can't join a game while already in a game
		debug(LOG_ERROR, "Can't join a game while already in a game / lobby. (Current mode: %s)", to_string(currentGameMode).c_str());
		return false;
	}

	if (ingame.localJoiningInProgress)
	{
		// Can't join a game while already in a game
		debug(LOG_ERROR, "Can't join a game while already in a game / lobby.");
		return false;
	}

	shutdownJoiningAttempt();

	// sort the connection list
	if (connection_list.size() > 1)
	{
		// sort the list, based on NETgetJoinPreferenceIPv6
		// preserve the original relative order amongst each class of IPv4/IPv6 addresses
		bool bSortIPv6First = NETgetJoinPreferenceIPv6();
		std::stable_sort(connection_list.begin(), connection_list.end(), [bSortIPv6First](const JoinConnectionDescription& a, const JoinConnectionDescription& b) -> bool {
			bool a_isIPv6 = a.host.find(":") != std::string::npos; // this is a very simplistic test - if the host contains ":" we treat it as IPv6
			bool b_isIPv6 = b.host.find(":") != std::string::npos;
			return (bSortIPv6First) ? (a_isIPv6 && !b_isIPv6) : (!a_isIPv6 && b_isIPv6);
		});
	}

	// network communication preparation
	NetPlay.bComms = true; // use network = true
	NETinit(true);

	PLAYERSTATS	playerStats;
	loadMultiStats(playerName, &playerStats);

	auto screen = WzJoiningGameScreen::make(connection_list, playerName, playerStats.identity, asSpectator,
		// onSuccessFunc
		[playerStats](const JoinConnectionDescription& connection) {
			handleJoinSuccess(connection, playerStats);
		},
		// onFailureFunc
		[]() {
			handleJoinFailure();
		}
	);
	// Use widgScheduleTask to ensure we never modify the registered overlays while they are being enumerated
	widgScheduleTask([screen]() {
		widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	});
	psCurrentJoiningAttemptScreen = screen;
	return true;
}

void shutdownJoiningAttempt()
{
	// Closes the overlay screen, cancelling whater join attempt is in progress (if one is in progress)
	if (auto strongJoiningAttemptScreen = psCurrentJoiningAttemptScreen.lock())
	{
		widgRemoveOverlayScreen(strongJoiningAttemptScreen);
		strongJoiningAttemptScreen.reset();
		psCurrentJoiningAttemptScreen.reset();
	}
}
