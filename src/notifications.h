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
/** @file
 *  Interface to the initialisation routines.
 */

#ifndef __INCLUDED_SRC_NOTIFICATIONS_H__
#define __INCLUDED_SRC_NOTIFICATIONS_H__

#include <list>
#include <memory>
#include <functional>

class WZ_Notification_Display_Options
{
public:
	// The default WZ_Notification_Display_Options is useful for notifications
	// that should _always_ be displayed.
	WZ_Notification_Display_Options() {}

public:
	// Sets a specific notification identifier to "display once".
	//
	// Once a notification with this identifier is seen once, future calls to `addNotification` with a
	// matching "uniqueNotificationIdentifier" will be ignored.
	// The "seen" status is persisted across application runs.
	//
	// Specify a uniqueNotificationIdentifier that uniquely identifies this notification, and is used to
	// match against future notifications.
	static WZ_Notification_Display_Options makeOneTime(const std::string& uniqueNotificationIdentifier)
	{
		WZ_Notification_Display_Options options;
		options._uniqueNotificationIdentifier = uniqueNotificationIdentifier;
		options._isOneTimeNotification = true;
		return options;
	}

	// Sets a specific notification identifier as "ignorable".
	//
	// This will cause a "Do Not Show Again" checkbox / option to be displayed after this
	// uniquely identified notification is displayed `numTimesSeenBeforeDoNotShowAgainOption`
	// times. (Prior instances of this notification will offer a "Dismiss" or "Not Now" option.)
	//
	// If the user selects / checks the "Do Not Show Again" option, future calls to
	// `addNotification` with a matching "uniqueNotificationIdentifier" will be ignored.
	// The display count / ignore state is persisted across application runs.
	//
	// Specify a uniqueNotificationIdentifier that uniquely identifies this notification, and is used to
	// match against future notifications.
	static WZ_Notification_Display_Options makeIgnorable(const std::string& uniqueNotificationIdentifier, uint8_t numTimesSeenBeforeDoNotShowAgainOption = 0)
	{
		WZ_Notification_Display_Options options;
		options._uniqueNotificationIdentifier = uniqueNotificationIdentifier;
		options._numTimesSeenBeforeDoNotShowAgainOption = numTimesSeenBeforeDoNotShowAgainOption;
		return options;
	}
public:
	const std::string& uniqueNotificationIdentifier() const { return _uniqueNotificationIdentifier; }
	uint8_t numTimesSeenBeforeDoNotShowAgainOption() const { return _numTimesSeenBeforeDoNotShowAgainOption; }
	bool isOneTimeNotification() const { return _isOneTimeNotification; }

private:
	// A string that uniquely identifies the notification
	// (to match against future notifications)
	std::string _uniqueNotificationIdentifier;

	// The number of times this identified notification can be displayed
	// before the "Do Not Show Again" option is offered
	uint8_t _numTimesSeenBeforeDoNotShowAgainOption = 0;

	bool _isOneTimeNotification = false;
};

class WZ_Notification; // forward-declare

class WZ_Notification_Action
{
public:
	WZ_Notification_Action() {}
	WZ_Notification_Action(std::string actionTitle, const std::function<void (const WZ_Notification&)>& onAction)
	: title(actionTitle)
	, onAction(onAction)
	{ }
public:
	std::string title;
	std::function<void (WZ_Notification&)> onAction;
};

class WZ_Notification
{
public:
	// [Required properties]:

	// A title for the top of the notification - REQUIRED
	std::string contentTitle;
	// Content text for the notification - REQUIRED
	std::string contentText;
	// A duration, in game ticks (see: GAME_TICKS_PER_SEC),
	// for the notification to be displayed before it auto-dismisses.
	//
	// Set to 0 for "until dismissed by user".
	//
	// Do not set this interval too short or users may not be able to
	// read the text before the notification disappears!
	uint32_t duration = 0;

	// [Optional properties]:

	// The path to a PNG to be displayed on the right side of the notification.
	// (Suggestion: Make sure the PNG is *at least* 72x72 pixels to ensure sharper display on higher resolution screens.)
	std::string largeIconPath;
	// Displays an Action button on the notification which calls the handler when clicked.
	WZ_Notification_Action action;
	// See: WZ_Notification_Display_Options
	WZ_Notification_Display_Options displayOptions;
public:
	bool isIgnorable() const { return !displayOptions.uniqueNotificationIdentifier().empty(); }
};

class WZ_Notification_Trigger
{
public:
	// A time interval, in game ticks (see: GAME_TICKS_PER_SEC),
	// from the time the notification is submitted before it is displayed.
	WZ_Notification_Trigger(uint32_t timeInterval)
	: timeInterval(timeInterval)
	{ }

	// Can be displayed immediately.
	static WZ_Notification_Trigger Immediate()
	{
		return WZ_Notification_Trigger(0);
	}
public:
	uint32_t timeInterval = 0;
};

// Add a notification
void addNotification(const WZ_Notification& notification, const WZ_Notification_Trigger& trigger);

// In-Game Notifications System
bool notificationsInitialize();
void notificationsShutDown();
void runNotifications();
bool isDraggingInGameNotification();
bool isMouseOverInGameNotification();

#endif // __INCLUDED_SRC_NOTIFICATIONS_H__
