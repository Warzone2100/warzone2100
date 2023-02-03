/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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
/**
 * @file init.c
 *
 * Game initialisation routines.
 *
 */

#include <nlohmann/json.hpp> // Must come before WZ includes
using json = nlohmann::json;

#include "lib/framework/frame.h"
#include "notifications.h"
#include "lib/gamelib/gtime.h"
#include "lib/widget/form.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pietypes.h"
#include "init.h"
#include "frend.h"
#include <algorithm>
#include <limits>

// MARK: - Notification Ignore List

#include <typeinfo>
#include <physfs.h>
#include "lib/framework/file.h"
#include <sstream>

class WZ_Notification_Preferences
{
public:
	WZ_Notification_Preferences(const std::string &fileName);

	void incrementNotificationRuns(const std::string& uniqueNotificationIdentifier);
	uint32_t getNotificationRuns(const std::string& uniqueNotificationIdentifier) const;

	void doNotShowNotificationAgain(const std::string& uniqueNotificationIdentifier);
	bool getDoNotShowNotificationValue(const std::string& uniqueNotificationIdentifier) const;
	bool canShowNotification(const WZ_Notification& notification) const;

	void clearAllNotificationPreferences();
	bool removeNotificationPreferencesIf(const std::function<bool (const std::string& uniqueNotificationIdentifier)>& matchIdentifierFunc);

	bool savePreferences();

private:
	void storeValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& value);
	json getJSONValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& defaultValue = json()) const;

	template<typename T>
	T getValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, T defaultValue) const
	{
		T result = defaultValue;
		try {
			result = getJSONValueForNotification(uniqueNotificationIdentifier, key, json(defaultValue)).get<T>();
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert json_variant to %s because of error: %s", typeid(T).name(), e.what());
			result = defaultValue;
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception encountered: json_variant::toType<%s>", typeid(T).name());
		}
		return result;
	}

private:
	json mRoot;
	std::string mFilename;
};

WZ_Notification_Preferences::WZ_Notification_Preferences(const std::string &fileName)
: mFilename(fileName)
{
	if (PHYSFS_exists(fileName.c_str()))
	{
		UDWORD size;
		char *data;
		if (loadFile(fileName.c_str(), &data, &size))
		{
			try {
				mRoot = json::parse(data, data + size);
			}
			catch (const std::exception &e) {
				ASSERT(false, "JSON document from %s is invalid: %s", fileName.c_str(), e.what());
			}
			catch (...) {
				debug(LOG_ERROR, "Unexpected exception parsing JSON %s", fileName.c_str());
			}
			ASSERT(!mRoot.is_null(), "JSON document from %s is null", fileName.c_str());
			ASSERT(mRoot.is_object(), "JSON document from %s is not an object. Read: \n%s", fileName.c_str(), data);
			free(data);
		}
		else
		{
			debug(LOG_ERROR, "Could not open \"%s\"", fileName.c_str());
			// treat as if no preferences exist yet
			mRoot = json::object();
		}
	}
	else
	{
		// no preferences exist yet
		mRoot = json::object();
	}

	// always ensure there's a "notifications" dictionary in the root object
	auto notificationsObject = mRoot.find("notifications");
	if (notificationsObject == mRoot.end() || !notificationsObject->is_object())
	{
		// create a dictionary object
		mRoot["notifications"] = json::object();
	}
}

void WZ_Notification_Preferences::storeValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& value)
{
	json& notificationsObj = mRoot["notifications"];
	auto notificationData = notificationsObj.find(uniqueNotificationIdentifier);
	if (notificationData == notificationsObj.end() || !notificationData->is_object())
	{
		notificationsObj[uniqueNotificationIdentifier] = json::object();
		notificationData = notificationsObj.find(uniqueNotificationIdentifier);
	}
	(*notificationData)[key] = value;
}

json WZ_Notification_Preferences::getJSONValueForNotification(const std::string& uniqueNotificationIdentifier, const std::string& key, const json& defaultValue /*= json()*/) const
{
	try {
		return mRoot.at("notifications").at(uniqueNotificationIdentifier).at(key);
	}
	catch (json::out_of_range&)
	{
		// some part of the path doesn't exist yet - return the default value
		return defaultValue;
	}
}

void WZ_Notification_Preferences::incrementNotificationRuns(const std::string& uniqueNotificationIdentifier)
{
	uint32_t seenCount = getNotificationRuns(uniqueNotificationIdentifier);
	storeValueForNotification(uniqueNotificationIdentifier, "seen", ++seenCount);
}

uint32_t WZ_Notification_Preferences::getNotificationRuns(const std::string& uniqueNotificationIdentifier) const
{
	return getValueForNotification(uniqueNotificationIdentifier, "seen", 0);
}

void WZ_Notification_Preferences::doNotShowNotificationAgain(const std::string& uniqueNotificationIdentifier)
{
	storeValueForNotification(uniqueNotificationIdentifier, "skip", true);
}

bool WZ_Notification_Preferences::getDoNotShowNotificationValue(const std::string& uniqueNotificationIdentifier) const
{
	return getValueForNotification(uniqueNotificationIdentifier, "skip", false);
}

void WZ_Notification_Preferences::clearAllNotificationPreferences()
{
	mRoot["notifications"] = json::object();
}

bool WZ_Notification_Preferences::removeNotificationPreferencesIf(const std::function<bool (const std::string& uniqueNotificationIdentifier)>& matchIdentifierFunc)
{
	ASSERT_OR_RETURN(false, mRoot.contains("notifications"), "root missing notifications object");
	json notificationsObjCopy = mRoot.at("notifications");
	std::vector<std::string> identifiersToRemove;
	for (auto it : notificationsObjCopy.items())
	{
		const auto& uniqueNotificationIdentifier = it.key();
		if (matchIdentifierFunc(uniqueNotificationIdentifier))
		{
			identifiersToRemove.push_back(uniqueNotificationIdentifier);
		}
	}
	if (identifiersToRemove.empty())
	{
		return false;
	}
	for (const auto& uniqueNotificationIdentifier : identifiersToRemove)
	{
		notificationsObjCopy.erase(uniqueNotificationIdentifier);
	}
	mRoot["notifications"] = notificationsObjCopy;
	return true;
}

bool WZ_Notification_Preferences::savePreferences()
{
	std::ostringstream stream;
	stream << mRoot.dump(4) << std::endl;
	std::string jsonString = stream.str();
	saveFile(mFilename.c_str(), jsonString.c_str(), jsonString.size());
	return true;
}

bool WZ_Notification_Preferences::canShowNotification(const WZ_Notification& notification) const
{
	if (notification.displayOptions.uniqueNotificationIdentifier().empty())
	{
		return true;
	}

	bool suppressNotification = false;
	if (notification.displayOptions.isOneTimeNotification())
	{
		suppressNotification = (getNotificationRuns(notification.displayOptions.uniqueNotificationIdentifier()) > 0);
	}
	else
	{
		suppressNotification = getDoNotShowNotificationValue(notification.displayOptions.uniqueNotificationIdentifier());
	}

	return !suppressNotification;
}

static WZ_Notification_Preferences* notificationPrefs = nullptr;

// MARK: - In-Game Notification System Types

class WZ_Notification_Status
{
public:
	WZ_Notification_Status(uint32_t queuedTime)
	: queuedTime(queuedTime)
	{ }
	enum NotificationState
	{
		waiting,
		opening,
		shown,
		closing,
		closed
	};
	NotificationState state = NotificationState::waiting;
	uint32_t stateStartTime = 0;
	uint32_t queuedTime = 0;
	float animationSpeed = 1.0f;
public:
	// normal speed is 1.0
	void setAnimationSpeed(float newSpeed)
	{
		animationSpeed = newSpeed;
	}
};

class WZ_Queued_Notification
{
public:
	WZ_Queued_Notification(const WZ_Notification& notification, const WZ_Notification_Status& status, const WZ_Notification_Trigger& trigger)
	: notification(notification)
	, status(status)
	, trigger(trigger)
	{ }

public:
	void setState(WZ_Notification_Status::NotificationState newState);
	bool wasProgrammaticallyDismissed() const { return dismissalCause == WZ_Notification_Dismissal_Reason::ACTION_BUTTON_CLICK || dismissalCause == WZ_Notification_Dismissal_Reason::PROGRAMMATIC; }
	WZ_Notification_Dismissal_Reason dismissReason() const { return dismissalCause; }

protected:
	void setWasProgrammaticallyDismissed() { dismissalCause = WZ_Notification_Dismissal_Reason::PROGRAMMATIC; }
	void setWasDismissedByActionButton() { dismissalCause = WZ_Notification_Dismissal_Reason::ACTION_BUTTON_CLICK; }

public:
	WZ_Notification notification;
	WZ_Notification_Status status;
	WZ_Notification_Trigger trigger;
private:
	bool bWasInitiallyShown = false;
	bool bWasFullyShown = false;
	WZ_Notification_Dismissal_Reason dismissalCause = WZ_Notification_Dismissal_Reason::USER_DISMISSED;
	friend class W_NOTIFICATION;
};

void WZ_Queued_Notification::setState(WZ_Notification_Status::NotificationState newState)
{
	status.state = newState;
	status.stateStartTime = realTime;

	if (newState == WZ_Notification_Status::NotificationState::closed)
	{
		if (notification.isIgnorable() && !bWasFullyShown && (dismissalCause != WZ_Notification_Dismissal_Reason::PROGRAMMATIC))
		{
			notificationPrefs->incrementNotificationRuns(notification.displayOptions.uniqueNotificationIdentifier());
		}
		bWasFullyShown = true;
	}
	else if (newState == WZ_Notification_Status::NotificationState::shown)
	{
		if (!bWasInitiallyShown)
		{
			bWasInitiallyShown = true;
			if (notification.onDisplay)
			{
				notification.onDisplay(notification);
			}
		}
	}
}

// MARK: - In-Game Notification Widgets

// Forward-declarations
void notificationsDidStartDragOnNotification(const Vector2i& dragStartPos);
void notificationDidStopDragOnNotification();
void finishedProcessingNotificationRequest(WZ_Queued_Notification* request, bool doNotShowAgain);
void removeInGameNotificationForm(WZ_Queued_Notification* request);

#include "lib/framework/input.h"

#define WZ_NOTIFICATION_WIDTH 500
#define WZ_NOTIFICATION_PADDING	15
#define WZ_NOTIFICATION_IMAGE_SIZE	36
#define WZ_NOTIFICATION_CONTENTS_LINE_SPACING	0
#define WZ_NOTIFICATION_CONTENTS_TOP_PADDING	5
#define WZ_NOTIFICATION_BUTTON_HEIGHT 20
#define WZ_NOTIFICATION_MODAL_BUTTON_HEIGHT 25
#define WZ_NOTIFICATION_BETWEEN_BUTTON_PADDING	10

#define WZ_NOTIFY_DONOTSHOWAGAINCB_ID 5

#include "hci.h"
#include "intfac.h"
#include "intdisplay.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/widget/checkbox.h"

static W_FORMINIT MakeNotificationFormInit()
{
	W_FORMINIT sFormInit;
	sFormInit.formID = 0;
	sFormInit.id = 0;
	sFormInit.x = 0; // always base of 0
	sFormInit.y = 0; // always base of 0
	sFormInit.width = WZ_NOTIFICATION_WIDTH; // fixed width
	sFormInit.height = 100; // starting height - real height is calculated later based on layout of contents
	return sFormInit;
}

class W_NOTIFICATION : public W_FORM
{
protected:
	W_NOTIFICATION(WZ_Queued_Notification* request, W_FORMINIT init = MakeNotificationFormInit());
public:
	~W_NOTIFICATION();
	static std::shared_ptr<W_NOTIFICATION> make(WZ_Queued_Notification* request, W_FORMINIT init = MakeNotificationFormInit());
	void run(W_CONTEXT *psContext) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void display(int xOffset, int yOffset) override;
public:
	Vector2i getDragOffset() const { return dragOffset; }
	bool isActivelyBeingDragged() const { return isInDragMode; }
	uint32_t getLastDragStartTime() const { return dragStartedTime; }
	uint32_t getLastDragEndTime() const { return dragEndedTime; }
	uint32_t getLastDragDuration() const { return dragEndedTime - dragStartedTime; }
	const std::string& notificationTag() const { return request->notification.tag; }
public:
	// to be called from the larger In-Game Notifications System
	bool dismissNotification(float animationSpeed = 1.0f);
	void deleteThisNotificationWidget();
private:
	bool calculateNotificationWidgetPos();
	gfx_api::texture* loadImage(const WZ_Notification_Image& image);
	void internalDismissNotification(float animationSpeed = 1.0f);
public:
	std::shared_ptr<WzCheckboxButton> pOnDoNotShowAgainCheckbox = nullptr;
private:
	WZ_Queued_Notification* request;
	bool isModal = false;
	bool isInDragMode = false;
	Vector2i dragOffset = {0, 0};
	Vector2i dragStartMousePos = {0, 0};
	Vector2i dragOffsetEnded = {0, 0};
	uint32_t dragStartedTime = 0;
	uint32_t dragEndedTime = 0;
	gfx_api::texture* pImageTexture = nullptr;
	std::weak_ptr<W_FORM> psModalParent;
};

struct DisplayNotificationButtonCache
{
	WzText wzText;
};

// MARK: - In-game Notification Display

static std::list<std::unique_ptr<WZ_Queued_Notification>> notificationQueue;
static std::shared_ptr<W_SCREEN> psNotificationOverlayScreen = nullptr;
static std::unique_ptr<WZ_Queued_Notification> currentNotification;
static std::shared_ptr<W_NOTIFICATION> currentInGameNotification = nullptr;
static uint32_t lastNotificationClosed = 0;
static Vector2i lastDragOnNotificationStartPos(-1,-1);

std::unique_ptr<WZ_Queued_Notification> popNextQueuedNotification()
{
	ASSERT(notificationPrefs, "Notification preferences not loaded!");
	auto it = notificationQueue.begin();
	while (it != notificationQueue.end())
	{
		auto & request = *it;

		if (!notificationPrefs->canShowNotification(request->notification))
		{
			// ignore this notification - remove from the list
			debug(LOG_GUI, "Ignoring notification: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
			if (request->notification.onIgnored)
			{
				request->notification.onIgnored(request->notification);
			}
			it = notificationQueue.erase(it);
			continue;
		}

		uint32_t num = std::min<uint32_t>(realTime - request->status.queuedTime, request->trigger.timeInterval);
		if (num == request->trigger.timeInterval)
		{
			std::unique_ptr<WZ_Queued_Notification> retVal(std::move(request));
			it = notificationQueue.erase(it);
			return retVal;
		}

		++it;
	}
	return nullptr;
}

// MARK: - W_NOTIFICATION

// ////////////////////////////////////////////////////////////////////////////
// display a notification action button
void displayNotificationAction(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	SDWORD			fx, fy, fw;
	W_BUTTON		*psBut = (W_BUTTON *)psWidget;
	bool			hilight = false;
	bool			greyOut = /*psWidget->UserData ||*/ (psBut->getState() & WBUT_DISABLE); // if option is unavailable.
	bool			isActionButton = (psBut->UserData == 1);

	// Any widget using displayTextOption must have its pUserData initialized to a (DisplayTextOptionCache*)
	assert(psWidget->pUserData != nullptr);
	DisplayNotificationButtonCache& cache = *static_cast<DisplayNotificationButtonCache*>(psWidget->pUserData);

	cache.wzText.setText(psBut->pText, psBut->FontID);

	if (psBut->isHighlighted())					// if mouse is over text then hilight.
	{
		hilight = true;
	}

	PIELIGHT colour;

	if (greyOut)								// unavailable
	{
		colour = WZCOL_TEXT_DARK;
	}
	else										// available
	{
		if (hilight || isActionButton)			// hilight
		{
			colour = WZCOL_TEXT_BRIGHT;
		}
		else									// don't highlight
		{
			colour = WZCOL_TEXT_MEDIUM;
		}
	}

	if (isActionButton)
	{
		// "Action" buttons have a bordering box
		int x0 = psBut->x() + xOffset;
		int y0 = psBut->y() + yOffset;
		int x1 = x0 + psBut->width();
		int y1 = y0 + psBut->height();
		if (hilight)
		{
			PIELIGHT fillClr = pal_RGBA(255, 255, 255, 30);
			pie_UniTransBoxFill(x0, y0, x1, y1, fillClr);
		}
		iV_Box(x0, y0, x1, y1, colour);
	}

	fw = cache.wzText.width();
	fy = yOffset + psWidget->y() + (psWidget->height() - cache.wzText.lineSize()) / 2 - cache.wzText.aboveBase();

	if (psWidget->style & WBUT_TXTCENTRE)							//check for centering, calculate offset.
	{
		fx = xOffset + psWidget->x() + ((psWidget->width() - fw) / 2);
	}
	else
	{
		fx = xOffset + psWidget->x();
	}

	if (!greyOut)
	{
		cache.wzText.render(fx + 1, fy + 1, pal_RGBA(0, 0, 0, 80));
	}
	cache.wzText.render(fx, fy, colour);

	return;
}

bool W_NOTIFICATION::dismissNotification(float animationSpeed /*= 1.0f*/)
{
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::closing:
		case WZ_Notification_Status::NotificationState::closed:
			// do nothing
			break;
		default:
			request->setWasProgrammaticallyDismissed();
			request->status.setAnimationSpeed(animationSpeed);
			request->setState(WZ_Notification_Status::NotificationState::closing);
			return true;
			break;
	}

	return false;
}

void W_NOTIFICATION::deleteThisNotificationWidget()
{
	if (auto psStrongModalParent = psModalParent.lock())
	{
		// if modal and has a modal parent, actually submit the *parent* for removal
		psStrongModalParent->deleteLater();
	}
	else
	{
		deleteLater();
	}
}

void W_NOTIFICATION::internalDismissNotification(float animationSpeed /*= 1.0f*/)
{
	// if notification is the one being displayed, animate it away by setting its state to closing
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::waiting:
			// should not happen
			break;
////		case WZ_Notification_Status::NotificationState::opening:
//			request->setState(WZ_Notification_Status::NotificationState::shown);
//			break;
		case WZ_Notification_Status::NotificationState::shown:
			request->status.setAnimationSpeed(animationSpeed);
			request->setState(WZ_Notification_Status::NotificationState::closing);
			break;
		default:
			// do nothing
			break;
	}
}

W_NOTIFICATION::W_NOTIFICATION(WZ_Queued_Notification* request, W_FORMINIT init /*= MakeNotificationFormInit()*/)
: W_FORM(&init)
, request(request)
{
	if (request)
	{
		isModal = request->notification.isModal;
	}
}

std::shared_ptr<W_NOTIFICATION> W_NOTIFICATION::make(WZ_Queued_Notification* request, W_FORMINIT init /*= MakeNotificationFormInit()*/)
{
	ASSERT(request != nullptr, "Request is null");

	class make_shared_enabler : public W_NOTIFICATION {
	public:
		make_shared_enabler(WZ_Queued_Notification* request, W_FORMINIT init): W_NOTIFICATION(request, init) {}
	};
	auto psNewNotificationForm = std::make_shared<make_shared_enabler>(request, init);

	bool isModal = request->notification.isModal;
	std::shared_ptr<W_FORM> psRootFrm = psNotificationOverlayScreen->psForm;
	if (isModal)
	{
		auto modalRootFrm = W_FULLSCREENOVERLAY_CLICKFORM::make();
		psRootFrm->attach(modalRootFrm);
		psRootFrm = modalRootFrm;
		psNewNotificationForm->psModalParent = modalRootFrm;
	}

	psRootFrm->attach(psNewNotificationForm);

	// Load the image, if specified
	if (!request->notification.largeIcon.empty())
	{
		psNewNotificationForm->pImageTexture = psNewNotificationForm->loadImage(request->notification.largeIcon);
	}

	const int notificationWidth = psNewNotificationForm->width();

//		/* Add the close button */
//		W_BUTINIT sCloseButInit;
//		sCloseButInit.formID = 0;
//		sCloseButInit.id = 1;
//		sCloseButInit.pTip = _("Close");
//		sCloseButInit.pDisplay = intDisplayImageHilight;
//		sCloseButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
//		W_BUTTON* psCloseButton = new W_BUTTON(&sCloseButInit);
//		psCloseButton->addOnClickHandler([psNewNotificationForm](W_BUTTON& button) {
//			psNewNotificationForm->internalDismissNotification();
//		});
//		attach(psCloseButton);
//		psCloseButton->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
//			int parentWidth = psWidget->parent()->width();
//			psWidget->setGeometry(parentWidth - CLOSE_WIDTH, 0, CLOSE_WIDTH, CLOSE_HEIGHT);
//		}));

	// Calculate dimensions for text area
	const int imageSize = (psNewNotificationForm->pImageTexture) ? WZ_NOTIFICATION_IMAGE_SIZE : 0;
	int maxTextWidth = notificationWidth - (WZ_NOTIFICATION_PADDING * 2) - imageSize - ((imageSize > 0) ? WZ_NOTIFICATION_PADDING : 0);
	int maxTitleWidth = maxTextWidth;
	int maxContentsWidth = maxTextWidth;
	if (isModal)
	{
		maxContentsWidth = notificationWidth - (WZ_NOTIFICATION_PADDING * 2);
	}

	// Add title
	auto label_title = std::make_shared<W_LABEL>();
	psNewNotificationForm->attach(label_title);
	int titleStartXPos = WZ_NOTIFICATION_PADDING;
	int titleStartYPos = WZ_NOTIFICATION_PADDING;
	if (isModal)
	{
		titleStartXPos = WZ_NOTIFICATION_PADDING + imageSize + ((imageSize > 0) ? WZ_NOTIFICATION_PADDING : 0);
	}
	label_title->setGeometry(titleStartXPos, titleStartYPos, maxTitleWidth, 12);
	label_title->setFontColour(WZCOL_TEXT_BRIGHT);
	int heightOfTitleLabel = label_title->setFormattedString(WzString::fromUtf8(request->notification.contentTitle), maxTitleWidth, (isModal) ? font_medium_bold : font_regular_bold, WZ_NOTIFICATION_CONTENTS_LINE_SPACING);
	if (isModal && (heightOfTitleLabel < imageSize))
	{
		// vertically center title text in available space
		titleStartYPos += ((imageSize - heightOfTitleLabel) / 2);
	}
	label_title->setGeometry(titleStartXPos, titleStartYPos, maxTitleWidth, heightOfTitleLabel);
	label_title->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	label_title->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });
	int titleBottom = label_title->y() + label_title->height();
	if (isModal)
	{
		titleBottom = label_title->y() + std::max<int>(label_title->height(), imageSize);
	}

	// Add contents
	auto label_contents = std::make_shared<W_LABEL>();
	psNewNotificationForm->attach(label_contents);
//	debug(LOG_GUI, "label_title.height=%d", label_title->height());
	int contentsStartYPos = titleBottom + WZ_NOTIFICATION_CONTENTS_TOP_PADDING;
	if (isModal)
	{
		contentsStartYPos = titleBottom + ((imageSize > 0) ? WZ_NOTIFICATION_PADDING : (WZ_NOTIFICATION_CONTENTS_TOP_PADDING + 3));
	}
	label_contents->setGeometry(WZ_NOTIFICATION_PADDING, contentsStartYPos, maxContentsWidth, 12);
	label_contents->setFontColour(WZCOL_TEXT_BRIGHT);
	int heightOfContentsLabel = label_contents->setFormattedString(WzString::fromUtf8(request->notification.contentText), maxContentsWidth, font_regular, WZ_NOTIFICATION_CONTENTS_LINE_SPACING);
	label_contents->setGeometry(label_contents->x(), label_contents->y(), maxContentsWidth, heightOfContentsLabel);
	label_contents->setTextAlignment(WLAB_ALIGNTOPLEFT);
	// set a custom hit-testing function that ignores all mouse input / clicks
	label_contents->setCustomHitTest([](WIDGET *psWidget, int x, int y) -> bool { return false; });

	// Add action buttons
	std::string dismissLabel = _("Dismiss");
	std::string actionLabel = request->notification.action.title;

	std::shared_ptr<W_BUTTON> psActionButton = nullptr;
	std::shared_ptr<W_BUTTON> psDismissButton = nullptr;

	// Position the buttons below the text contents area
	int buttonsTop = label_contents->y() + label_contents->height() + WZ_NOTIFICATION_PADDING;
	if (isModal)
	{
		buttonsTop += (WZ_NOTIFICATION_PADDING / 2);
	}

	W_BUTINIT sButInit;
	sButInit.formID = 0;
	sButInit.height = (isModal) ? WZ_NOTIFICATION_MODAL_BUTTON_HEIGHT : WZ_NOTIFICATION_BUTTON_HEIGHT;
	sButInit.y = buttonsTop;
	sButInit.style |= WBUT_TXTCENTRE;
	sButInit.UserData = 0; // store whether "Action" button or not
	sButInit.initPUserDataFunc = []() -> void * { return new DisplayNotificationButtonCache(); };
	sButInit.onDelete = [](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayNotificationButtonCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	};
	sButInit.pDisplay = displayNotificationAction;

	if (!actionLabel.empty())
	{
		// Display both an "Action" button and a "Dismiss" button

		// 1.) "Action" button
		sButInit.id = 2;
		sButInit.width = iV_GetTextWidth(actionLabel.c_str(), font_regular_bold) + 18;
		sButInit.x = (short)(psNewNotificationForm->width() - WZ_NOTIFICATION_PADDING - sButInit.width);
		sButInit.UserData = 1; // store "Action" state
		sButInit.FontID = font_regular_bold;
		sButInit.pText = actionLabel.c_str();
		psActionButton = std::make_shared<W_BUTTON>(&sButInit);
		psNewNotificationForm->attach(psActionButton);
		psActionButton->addOnClickHandler([](W_BUTTON& button) {
			auto psNewNotificationForm = std::dynamic_pointer_cast<W_NOTIFICATION>(button.parent());
			ASSERT_OR_RETURN(, psNewNotificationForm != nullptr, "null parent");
			if (psNewNotificationForm->request->notification.action.onAction)
			{
				psNewNotificationForm->request->notification.action.onAction(psNewNotificationForm->request->notification);
			}
			else
			{
				debug(LOG_ERROR, "Action defined (\"%s\"), but no action handler!", psNewNotificationForm->request->notification.action.title.c_str());
			}
			psNewNotificationForm->request->setWasDismissedByActionButton();
			psNewNotificationForm->internalDismissNotification();
		});
	}

	if (psActionButton != nullptr || request->notification.duration == 0)
	{
		// 2.) "Dismiss" button
		dismissLabel = u8"▴ " + dismissLabel;
		sButInit.id = 3;
		sButInit.FontID = font_regular;
		sButInit.width = iV_GetTextWidth(dismissLabel.c_str(), font_regular) + 18;
		sButInit.x = (short)(((psActionButton) ? (psActionButton->x()) - WZ_NOTIFICATION_BETWEEN_BUTTON_PADDING : psNewNotificationForm->width() - WZ_NOTIFICATION_PADDING) - sButInit.width);
		sButInit.pText = dismissLabel.c_str();
		sButInit.UserData = 0; // store regular state
		psDismissButton = std::make_shared<W_BUTTON>(&sButInit);
		psDismissButton->addOnClickHandler([](W_BUTTON& button) {
			auto psNewNotificationForm = std::dynamic_pointer_cast<W_NOTIFICATION>(button.parent());
			ASSERT_OR_RETURN(, psNewNotificationForm != nullptr, "null parent");
			psNewNotificationForm->internalDismissNotification();
		});
		psNewNotificationForm->attach(psDismissButton);
	}

	if (request->notification.isIgnorable() && !request->notification.displayOptions.isOneTimeNotification())
	{
		ASSERT(notificationPrefs, "Notification preferences not loaded!");
		auto numTimesShown = notificationPrefs->getNotificationRuns(request->notification.displayOptions.uniqueNotificationIdentifier());
		if (numTimesShown >= request->notification.displayOptions.numTimesSeenBeforeDoNotShowAgainOption())
		{
			// Display "do not show again" button with checkbox
			auto pDoNotShowAgainButton = std::make_shared<WzCheckboxButton>();
			psNewNotificationForm->attach(pDoNotShowAgainButton);
			pDoNotShowAgainButton->id = WZ_NOTIFY_DONOTSHOWAGAINCB_ID;
			pDoNotShowAgainButton->pText = _("Do not show again");
			pDoNotShowAgainButton->FontID = font_small;
			Vector2i minimumDimensions = pDoNotShowAgainButton->calculateDesiredDimensions();
			pDoNotShowAgainButton->setGeometry(WZ_NOTIFICATION_PADDING, buttonsTop, minimumDimensions.x, std::max(minimumDimensions.y, (isModal) ? WZ_NOTIFICATION_MODAL_BUTTON_HEIGHT : WZ_NOTIFICATION_BUTTON_HEIGHT));
			psNewNotificationForm->pOnDoNotShowAgainCheckbox = pDoNotShowAgainButton;
		}
	}

	// calculate the required height for the notification
	int bottom_labelContents = label_contents->y() + label_contents->height();
	uint16_t calculatedHeight = bottom_labelContents + WZ_NOTIFICATION_PADDING;
	if (psActionButton || psDismissButton)
	{
		int maxButtonBottom = std::max<int>((psActionButton) ? (psActionButton->y() + psActionButton->height()) : 0, (psDismissButton) ? (psDismissButton->y() + psDismissButton->height()) : 0);
		calculatedHeight = std::max<int>(calculatedHeight, maxButtonBottom + WZ_NOTIFICATION_PADDING);
	}
	// Also factor in the image, if one is present
	if (imageSize > 0)
	{
		calculatedHeight = std::max<int>(calculatedHeight, imageSize + (WZ_NOTIFICATION_PADDING * 2));
	}
	psNewNotificationForm->setGeometry(psNewNotificationForm->x(), psNewNotificationForm->y(), psNewNotificationForm->width(), calculatedHeight);

	return psNewNotificationForm;
}

W_NOTIFICATION::~W_NOTIFICATION()
{
	if (pImageTexture)
	{
		delete pImageTexture;
		pImageTexture = nullptr;
	}
}

#include "lib/ivis_opengl/piestate.h"

gfx_api::texture* W_NOTIFICATION::loadImage(const WZ_Notification_Image& image)
{
	return image.loadImageToTexture();
}

gfx_api::texture* WZ_Notification_Image::loadImageToTexture() const
{
	const WZ_Notification_Image& image = *this;

	gfx_api::texture* pTexture = nullptr;
	if (!image.imagePath().empty())
	{
		pTexture = gfx_api::context::get().loadTextureFromFile(image.imagePath().c_str(), gfx_api::texture_type::user_interface);
	}
	else if (!image.memoryBuffer().empty())
	{
		iV_Image ivImage;
		auto result = iV_loadImage_PNG(image.memoryBuffer(), &ivImage);
		if (!result.noError())
		{
			debug(LOG_ERROR, "Failed to load image from memory buffer: %s", result.text.c_str());
			return nullptr;
		}
		pTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(ivImage), gfx_api::texture_type::user_interface, "mem::notify_image");
	}
	else
	{
		// empty or unhandled case
		return nullptr;
	}
	return pTexture;
}

#ifndef M_PI_2
# define M_PI_2		1.57079632679489661923132169163975144	/* pi/2 */
#endif

float EaseOutElastic(float p)
{
	return static_cast<float>(sin(-13 * M_PI_2 * (p + 1)) * pow(2, -10 * p) + 1);
}

float EaseOutQuint(float p)
{
	float f = (p - 1);
	return f * f * f * f * f + 1;
}

float EaseInCubic(float p)
{
	return p * p * p;
}

#define WZ_NOTIFICATION_OPEN_DURATION (GAME_TICKS_PER_SEC*1) // Time duration for notification open animation, in game ticks
#define WZ_NOTIFICATION_CLOSE_DURATION (WZ_NOTIFICATION_OPEN_DURATION / 2)
#define WZ_NOTIFICATION_TOP_PADDING 5
#define WZ_NOTIFICATION_TOP_MODAL_PADDING 20
#define WZ_NOTIFICATION_MIN_DELAY_BETWEEN (GAME_TICKS_PER_SEC*1) // Minimum delay between display of notifications, in game ticks

bool W_NOTIFICATION::calculateNotificationWidgetPos()
{
	if (request == nullptr)
	{
		return false;
	}

	// center horizontally in window
	int x = std::max<int>((screenWidth - width()) / 2, 0);
	int y = 0; // set below
	const int endingYPosition = (!isModal) ? WZ_NOTIFICATION_TOP_PADDING : (WZ_NOTIFICATION_TOP_MODAL_PADDING);

	// calculate positioning based on state and time
	switch (request->status.state)
	{
		case WZ_Notification_Status::NotificationState::waiting:
			// first chance to display
			request->setState(WZ_Notification_Status::NotificationState::opening);
			// fallthrough
		case WZ_Notification_Status::NotificationState::opening:
		{
			// calculate opening progress based on the stateStartTime
			const uint32_t startTime = request->status.stateStartTime;
			float openAnimationDuration = float(WZ_NOTIFICATION_OPEN_DURATION) * request->status.animationSpeed;
			uint32_t endTime = startTime + uint32_t(openAnimationDuration);
			if (realTime < endTime)
			{
				y = static_cast<int>((-height()) + (EaseOutQuint((float(realTime) - float(startTime)) / float(openAnimationDuration)) * (endingYPosition + height())) + 1);
				if (!(y + getDragOffset().y >= endingYPosition))
				{
					break;
				}
				else
				{
					// factoring in the drag, the notification is already at (or past) fully open
					// so fall through to immediately transition to the "shown" state
				}
			}
			request->setState(WZ_Notification_Status::NotificationState::shown);
		} // fallthrough
		case WZ_Notification_Status::NotificationState::shown:
		{
			const auto& duration = request->notification.duration;
			if (duration > 0 && !isActivelyBeingDragged())
			{
				if (getDragOffset().y > 0)
				{
					// when dragging a notification more *open* (/down),
					// ensure that the notification remains displayed for at least one additional second
					// beyond the bounce-back from the drag release
					request->status.stateStartTime = std::max<uint32_t>(request->status.stateStartTime, realTime - duration + GAME_TICKS_PER_SEC);
				}
			}
			if (duration == 0 || (realTime < (request->status.stateStartTime + duration)) || isActivelyBeingDragged())
			{
				y = endingYPosition;
				if ((y + getDragOffset().y) > ((-height() / 3) * 2))
				{
					break;
				}
			}
			request->setState(WZ_Notification_Status::NotificationState::closing);
		} // fallthrough
		case WZ_Notification_Status::NotificationState::closing:
		{
			// calculate closing progress based on the stateStartTime
			const uint32_t startTime = request->status.stateStartTime;
			float closeAnimationDuration = float(WZ_NOTIFICATION_CLOSE_DURATION) * request->status.animationSpeed;
			uint32_t endTime = startTime + uint32_t(closeAnimationDuration);
			if (realTime < endTime)
			{
				float percentComplete = (float(realTime) - float(startTime)) / float(closeAnimationDuration);
				if (getDragOffset().y >= 0)
				{
					percentComplete = EaseInCubic(percentComplete);
				}
				y = static_cast<int>(endingYPosition - (percentComplete * (endingYPosition + height())));
				if ((y + getDragOffset().y) > -height())
				{
					break;
				}
				else
				{
					// closed early (because of drag offset)
					// drop through and signal "closed" state
				}
			}
			request->setState(WZ_Notification_Status::NotificationState::closed);

		} // fallthrough
		case WZ_Notification_Status::NotificationState::closed:
			// widget is now off-screen - get checkbox state (if present)
			bool bDoNotShowAgain = false;
			if (pOnDoNotShowAgainCheckbox)
			{
				bDoNotShowAgain = pOnDoNotShowAgainCheckbox->getIsChecked();
			}
			// then destroy the widget, and finalize the notification request
			removeInGameNotificationForm(request);
			finishedProcessingNotificationRequest(request, bDoNotShowAgain); // after this, request is invalid!
			request = nullptr; // TEMP
			return true; // processed notification
	}

	x += getDragOffset().x;
	y += getDragOffset().y;

	move(x, y);

	return false;
}

/* Run a notification widget */
void W_NOTIFICATION::run(W_CONTEXT *psContext)
{
	if (isInDragMode && mouseDown(MOUSE_LMB))
	{
		int dragStartY = dragStartMousePos.y;
		int currMouseY = mouseY();

		// calculate how much to respond to the drag by comparing the start to the current position
		if (dragStartY > currMouseY)
		{
			// dragging up (to close) - respond 1 to 1
			int distanceY = dragStartY - currMouseY;
			dragOffset.y = (distanceY > 0) ? -(distanceY) : 0;
//			debug(LOG_GUI, "dragging up, dragOffset.y: (%d)", dragOffset.y);
		}
		else if (currMouseY > dragStartY)
		{
			// dragging down
			const int verticalLimit = 10;
			int distanceY = currMouseY - dragStartY;
			dragOffset.y = static_cast<int>(verticalLimit * (1 + log10(float(distanceY) / float(verticalLimit))));
//			debug(LOG_GUI, "dragging down, dragOffset.y: (%d)", dragOffset.y);
		}
		else
		{
			dragOffset.y = 0;
		}
	}
	else
	{
		if (isInDragMode && !mouseDown(MOUSE_LMB))
		{
//			debug(LOG_GUI, "No longer in drag mode");
			isInDragMode = false;
			dragEndedTime = realTime;
			dragOffsetEnded = dragOffset;
			notificationDidStopDragOnNotification();
		}
		if (request && (request->status.state != WZ_Notification_Status::NotificationState::closing))
		{
			// decay drag offset
			const uint32_t dragDecayDuration = GAME_TICKS_PER_SEC * 1;
			if (dragOffset.y != 0)
			{
				dragOffset.y = dragOffsetEnded.y - (int)(float(dragOffsetEnded.y) * EaseOutElastic((float(realTime) - float(dragEndedTime)) / float(dragDecayDuration)));
			}
		}
	}

	calculateNotificationWidgetPos();
}

void W_NOTIFICATION::clicked(W_CONTEXT *psContext, WIDGET_KEY key)
{
	ASSERT_OR_RETURN(, request != nullptr, "request is null");

	if (request->status.state == WZ_Notification_Status::NotificationState::closing)
	{
		// if clicked while closing, set state to shown
//		debug(LOG_GUI, "Click while closing - set to shown");
		request->status.state = WZ_Notification_Status::NotificationState::shown;
		request->status.stateStartTime = realTime;
	}

	if (geometry().contains(psContext->mx, psContext->my))
	{
//		debug(LOG_GUI, "Enabling drag mode");
		isInDragMode = true;
		dragStartMousePos.x = psContext->mx;
		dragStartMousePos.y = psContext->my;
//		debug(LOG_GUI, "dragStartMousePos: (%d x %d)", dragStartMousePos.x, dragStartMousePos.y);
		dragStartedTime = realTime;
		notificationsDidStartDragOnNotification(dragStartMousePos);
	}
}

#define WZ_NOTIFICATION_DOWN_DRAG_DISCARD_CLICK_THRESHOLD	5

void W_NOTIFICATION::released(W_CONTEXT *psContext, WIDGET_KEY key)
{
//	debug(LOG_GUI, "released");

	if (request)
	{
		if (isInDragMode && dragOffset.y < WZ_NOTIFICATION_DOWN_DRAG_DISCARD_CLICK_THRESHOLD)
		{
			internalDismissNotification();
		}
	}
}

void W_NOTIFICATION::display(int xOffset, int yOffset)
{
	int x0 = x() + xOffset;
	int y0 = y() + yOffset;
	int x1 = x0 + width();
	int y1 = y0 + height();
	// Need to do a little trick here - ensure the bounds are always positive, adjust the height
	if (y() < 0)
	{
		y0 += -y();
	}

	auto videoBuffWidth = pie_GetVideoBufferWidth();
	auto videoBuffHeight = pie_GetVideoBufferHeight();
	if (videoBuffWidth == 0 || videoBuffHeight == 0 || screenWidth == 0 || screenHeight == 0)
	{
		return;
	}

	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(255, 255, 255, 50));
	pie_UniTransBoxFill(x0, y0, x1, y1, pal_RGBA(0, 0, 0, 50));

	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_NOTIFICATION_BOX);
	iV_Box2(x0, y0, x1, y1, WZCOL_FORM_DARK, WZCOL_FORM_DARK);
	iV_Box2(x0 - 1, y0 - 1, x1 + 1, y1 + 1, pal_RGBA(255, 255, 255, 50), pal_RGBA(255, 255, 255, 50));

	// Display the image, if present
	int imageLeft = x1 - WZ_NOTIFICATION_PADDING - WZ_NOTIFICATION_IMAGE_SIZE;
	int imageTop = (y() + yOffset) + WZ_NOTIFICATION_PADDING;
	if (isModal)
	{
		// Place the icon on the left side, before the title
		imageLeft = x0 + WZ_NOTIFICATION_PADDING;
	}

	if (pImageTexture)
	{
		int image_x0 = imageLeft;
		int image_y0 = imageTop;

		iV_DrawImageAnisotropic(*pImageTexture, Vector2i(image_x0, image_y0), Vector2f(0,0), Vector2f(WZ_NOTIFICATION_IMAGE_SIZE, WZ_NOTIFICATION_IMAGE_SIZE), 0.f, WZCOL_WHITE);
	}
}

// MARK: - In-Game Notification System Functions

void notificationsDidStartDragOnNotification(const Vector2i& dragStartPos)
{
	lastDragOnNotificationStartPos = dragStartPos;
}

void notificationDidStopDragOnNotification()
{
	lastDragOnNotificationStartPos = Vector2i(-1, -1);
}

bool notificationsInitialize()
{
	notificationPrefs = new WZ_Notification_Preferences("notifications.json");

	// Initialize the notifications overlay screen
	psNotificationOverlayScreen = W_SCREEN::make();
	psNotificationOverlayScreen->psForm->hide(); // hiding the root form does not stop display of children, but *does* prevent it from accepting mouse over itself - i.e. basically makes it transparent
	widgRegisterOverlayScreen(psNotificationOverlayScreen, std::numeric_limits<uint16_t>::max());

	return true;
}

void notificationsShutDown()
{
	if (notificationPrefs)
	{
		notificationPrefs->savePreferences();
		delete notificationPrefs;
		notificationPrefs = nullptr;
	}

	if (currentInGameNotification)
	{
		widgDelete(currentInGameNotification.get());
		currentInGameNotification = nullptr;
	}

	if (psNotificationOverlayScreen)
	{
		widgRemoveOverlayScreen(psNotificationOverlayScreen);
		psNotificationOverlayScreen = nullptr;
	}
}

bool isDraggingInGameNotification()
{
	// right now we only support a single concurrent notification
	if (currentInGameNotification)
	{
		if (currentInGameNotification->isActivelyBeingDragged())
		{
			return true;
		}
	}

	// also handle the case where a drag starts, the notification dismisses itself, but the user has not released the mouse button
	// in this case, we want to consider it an in-game notification drag *until* the mouse button is released

	return (lastDragOnNotificationStartPos.x >= 0 && lastDragOnNotificationStartPos.y >= 0);
}

std::shared_ptr<W_NOTIFICATION> getOrCreateInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return nullptr;

	// right now we only support a single concurrent notification
	if (!currentInGameNotification)
	{
		currentInGameNotification = W_NOTIFICATION::make(request);
	}
	return currentInGameNotification;
}


void displayNotificationInGame(WZ_Queued_Notification* request)
{
	ASSERT(request, "request is null");
	getOrCreateInGameNotificationForm(request);
	// NOTE: Can ignore the result of the above, because it automatically attaches it to the root notification overlay screen
}

void displayNotification(WZ_Queued_Notification* request)
{
	ASSERT(request, "request is null");

	// By default, display the notification using the in-game notification system
	displayNotificationInGame(request);
}

// run in-game notifications queue
void runNotifications()
{
	// at the moment, we only support displaying a single notification at a time

	if (!currentNotification)
	{
		if ((realTime - lastNotificationClosed) < WZ_NOTIFICATION_MIN_DELAY_BETWEEN)
		{
			// wait to fetch a new notification till a future cycle
			return;
		}
		// check for a new notification to display
		currentNotification = popNextQueuedNotification();
		if (currentNotification)
		{
			// display the new notification
			displayNotification(currentNotification.get());
		}
	}

	if (!currentInGameNotification || !currentInGameNotification->isActivelyBeingDragged())
	{
		if (lastDragOnNotificationStartPos.x >= 0 && lastDragOnNotificationStartPos.y >= 0)
		{
			UDWORD currDragStartX, currDragStartY;
			if (mouseDrag(MOUSE_LMB, &currDragStartX, &currDragStartY))
			{
				if (currDragStartX != lastDragOnNotificationStartPos.x || currDragStartY != lastDragOnNotificationStartPos.y)
				{
					notificationDidStopDragOnNotification(); // ensure last notification drag position is cleared
				}
			}
			else
			{
				notificationDidStopDragOnNotification(); // ensure last notification drag position is cleared
			}
		}
	}
}

void removeInGameNotificationForm(WZ_Queued_Notification* request)
{
	if (!request) return;

	// right now we only support a single concurrent notification
	currentInGameNotification->deleteThisNotificationWidget();
	currentInGameNotification = nullptr;
}

void finishedProcessingNotificationRequest(WZ_Queued_Notification* request, bool doNotShowAgain)
{
	// at the moment, we only support processing a single notification at a time

	if (doNotShowAgain && !request->wasProgrammaticallyDismissed())
	{
		ASSERT(!request->notification.displayOptions.uniqueNotificationIdentifier().empty(), "Do Not Show Again was selected, but notification has no ignore key");
		debug(LOG_GUI, "Do Not Show Notification Again: %s", request->notification.displayOptions.uniqueNotificationIdentifier().c_str());
		notificationPrefs->doNotShowNotificationAgain(request->notification.displayOptions.uniqueNotificationIdentifier());
	}

	if (request->notification.onDismissed)
	{
		request->notification.onDismissed(request->notification, request->dismissReason());
	}

	// finished with this notification
	currentNotification.reset(); // at this point request is no longer valid!
	lastNotificationClosed = realTime;
}

// MARK: - Base functions for handling Notifications

void addNotification(const WZ_Notification& notification, const WZ_Notification_Trigger& trigger)
{
	// Verify notification properties
	if (notification.contentTitle.empty())
	{
		debug(LOG_ERROR, "addNotification called with empty notification.contentTitle");
		return;
	}
	if (notification.contentText.empty())
	{
		debug(LOG_ERROR, "addNotification called with empty notification.contentText");
		return;
	}

	// Add the notification to the notification system's queue
	notificationQueue.push_back(std::unique_ptr<WZ_Queued_Notification>(new WZ_Queued_Notification(notification, WZ_Notification_Status(realTime), trigger)));
}

bool removeNotificationPreferencesIf(const std::function<bool (const std::string& uniqueNotificationIdentifier)>& matchIdentifierFunc)
{
	ASSERT_OR_RETURN(false, notificationPrefs, "notificationPrefs is null");
	return notificationPrefs->removeNotificationPreferencesIf(matchIdentifierFunc);
}

// Whether one or more notifications with the specified tag (exact match) are currently-displayed or queued
// If `scope` is `DISPLAYED_ONLY`, only currently-displayed notifications will be processed
// If `scope` is `QUEUED_ONLY`, only queued notifications will be processed
bool hasNotificationsWithTag(const std::string& tag, NotificationScope scope /*= NotificationScope::DISPLAYED_AND_QUEUED*/)
{
	if ((scope == NotificationScope::DISPLAYED_AND_QUEUED || scope == NotificationScope::DISPLAYED_ONLY) && currentInGameNotification)
	{
		if (currentInGameNotification->notificationTag() == tag)
		{
			return true;
		}
	}

	if ((scope == NotificationScope::DISPLAYED_AND_QUEUED || scope == NotificationScope::QUEUED_ONLY))
	{
		for (auto& queuedNotification : notificationQueue)
		{
			if (queuedNotification->notification.tag == tag)
			{
				return true;
			}
		}
	}
	return false;
}

// Cancel or dismiss existing notifications by tag (exact match)
// If `scope` is `DISPLAYED_ONLY`, only currently-displayed notifications will be processed
// If `scope` is `QUEUED_ONLY`, only queued notifications will be processed
//
// Returns: `true` if one or more notifications were cancelled or dismissed
bool cancelOrDismissNotificationsWithTag(const std::string& desiredTag, NotificationScope scope /*= NotificationScope::DISPLAYED_AND_QUEUED*/)
{
	return cancelOrDismissNotificationIfTag([desiredTag](const std::string& tag) {
		return desiredTag == tag;
	}, scope);
}

// Cancel or dismiss existing notifications by tag
// Accepts a `matchTagFunc` that receives each (queued / currently-displayed) notification's tag,
// and returns "true" if it should be cancelled or dismissed (if queued / currently-displayed)
// If `scope` is `DISPLAYED_ONLY`, only currently-displayed notifications will be processed
// If `scope` is `QUEUED_ONLY`, only queued notifications will be processed
//
// Returns: `true` if one or more notifications were cancelled or dismissed
bool cancelOrDismissNotificationIfTag(const std::function<bool (const std::string& tag)>& matchTagFunc, NotificationScope scope /*= NotificationScope::DISPLAYED_AND_QUEUED*/)
{
	size_t numCancelledNotifications = 0;

	if ((scope == NotificationScope::DISPLAYED_AND_QUEUED || scope == NotificationScope::DISPLAYED_ONLY) && currentInGameNotification)
	{
		if (matchTagFunc(currentInGameNotification->notificationTag()))
		{
			if (currentInGameNotification->dismissNotification())
			{
				debug(LOG_GUI, "Dismissing currently-displayed notification with tag: [%s]", currentInGameNotification->notificationTag().c_str());
				++numCancelledNotifications;
			}
		}
	}

	if ((scope == NotificationScope::DISPLAYED_AND_QUEUED || scope == NotificationScope::QUEUED_ONLY))
	{
		auto it = notificationQueue.begin();
		while (it != notificationQueue.end())
		{
			auto & request = *it;

			if (matchTagFunc(request->notification.tag))
			{
				// cancel this notification - remove from the queue
				debug(LOG_GUI, "Cancelling queued notification: [%s]; with tag: [%s]", request->notification.contentTitle.c_str(), request->notification.tag.c_str());
				it = notificationQueue.erase(it);
				++numCancelledNotifications;
				continue;
			}

			++it;
		}
	}

	return numCancelledNotifications > 0;
}
