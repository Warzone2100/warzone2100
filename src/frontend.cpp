/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*
 * FrontEnd.c
 *
 * front end title and options screens.
 * Alex Lee. Pumpkin Studios. Eidos PLC 98,
 */

#include "lib/framework/wzapp.h"

#include "lib/framework/input.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/sound/mixer.h"
#include "lib/sound/tracklib.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/slider.h"
#include "lib/widget/dropdown.h"
#include "lib/widget/gridlayout.h"
#include "lib/widget/margin.h"
#include "lib/widget/alignment.h"

#include <limits>

#include "advvis.h"
#include "challenge.h"
#include "component.h"
#include "configuration.h"
#include "difficulty.h"
#include "display.h"
#include "droid.h"
#include "frend.h"
#include "frontend.h"
#include "group.h"
#include "hci.h"
#include "init.h"
#include "levels.h"
#include "intdisplay.h"
#include "keybind.h"
#include "keyedit.h"
#include "loadsave.h"
#include "main.h"
#include "mission.h"
#include "modding.h"
#include "multiint.h"
#include "multilimit.h"
#include "multiplay.h"
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"
#include "titleui/titleui.h"
#include "urlhelpers.h"
#include "game.h"
#include "map.h" //for builtInMap
#include "notifications.h"

#include "uiscreens/titlemenu.h"
#include "uiscreens/common.h"
#include "uiscreens/options/graphicsoptions.h"
#include "uiscreens/gamemodes/gmcommon.h"



// ////////////////////////////////////////////////////////////////////////////
// Handling Screen Size Changes

#define DISPLAY_SCALE_PROMPT_TAG_PREFIX		"displayscale::prompt::"
#define DISPLAY_SCALE_PROMPT_INCREASE_TAG	DISPLAY_SCALE_PROMPT_TAG_PREFIX "increase"
#define WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION 1440
static unsigned int lastProcessedDisplayScale = 0;
static int lastProcessedScreenWidth = 0;
static int lastProcessedScreenHeight = 0;

/* Tell the frontend when the screen has been resized */
void frontendScreenSizeDidChange(int oldWidth, int oldHeight, int newWidth, int newHeight)
{
	// NOTE:
	// By setting the appropriate calcLayout functions on all interface elements,
	// they should automatically recalculate their layout on screen resize.
	if (wzTitleUICurrent)
	{
		std::shared_ptr<WzTitleUI> current = wzTitleUICurrent;
		current->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
	}

	// Determine if there should be a prompt to increase display scaling
	const unsigned int currentDisplayScale = wzGetCurrentDisplayScale();
	if ((newWidth >= oldWidth && newHeight >= oldHeight)
		&& (lastProcessedScreenWidth != newWidth || lastProcessedScreenHeight != newHeight) // filter out duplicate events
		&& (lastProcessedDisplayScale == 0 || lastProcessedDisplayScale == currentDisplayScale)	// filter out duplicate events & display-scale-only changes
		&& (newWidth >= WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION || newHeight >= WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION))
	{
		unsigned int suggestedDisplayScale = wzGetSuggestedDisplayScaleForCurrentWindowSize(WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION);
		if ((suggestedDisplayScale > currentDisplayScale)
			&& !hasNotificationsWithTag(DISPLAY_SCALE_PROMPT_INCREASE_TAG, NotificationScope::DISPLAYED_ONLY))
		{
			cancelOrDismissNotificationIfTag([](const std::string& tag) {
				return (tag.rfind(DISPLAY_SCALE_PROMPT_TAG_PREFIX, 0) == 0);
			});

			WZ_Notification notification;
			notification.duration = 0;
			notification.contentTitle = _("Increase Game Display Scale?");
			std::string contextText = "\n";
			contextText += _("With your current resolution & display scale settings, the game's user interface may appear small, and the game perspective may appear distorted.");
			contextText += "\n\n";
			contextText += _("You can fix this by increasing the game's Display Scale setting.");
			contextText += "\n\n";
			contextText += astringf(_("Would you like to increase the game's Display Scale to: %u%%?"), suggestedDisplayScale);
			notification.contentText = contextText;
			notification.action = WZ_Notification_Action(_("Increase Display Scale"), [](const WZ_Notification&) {
				// Determine maximum display scale for current window size
				unsigned int desiredDisplayScale = wzGetSuggestedDisplayScaleForCurrentWindowSize(WZ_SUGGESTED_MAX_LOGICAL_SCREEN_SIZE_DIMENSION);
				if (desiredDisplayScale == wzGetCurrentDisplayScale())
				{
					// nothing to do now
					return;
				}

				// Store the new display scale
				auto priorDisplayScale = war_GetDisplayScale();
				war_SetDisplayScale(desiredDisplayScale);

				if (wzChangeDisplayScale(desiredDisplayScale))
				{
					WZ_Notification completed_notification;
					completed_notification.duration = 6 * GAME_TICKS_PER_SEC;
					completed_notification.contentTitle = astringf(_("Display Scale Increased to: %u%%"), desiredDisplayScale);
					std::string contextText = _("You can adjust the Display Scale at any time in the Video Options menu.");
					completed_notification.contentText = contextText;
					completed_notification.tag = DISPLAY_SCALE_PROMPT_TAG_PREFIX "completed";
					addNotification(completed_notification, WZ_Notification_Trigger::Immediate());
				}
				else
				{
					// failed to change display scale - restore the prior setting
					war_SetDisplayScale(priorDisplayScale);
				}
			});
			notification.onDismissed = [](const WZ_Notification&, WZ_Notification_Dismissal_Reason reason) {
				if (reason != WZ_Notification_Dismissal_Reason::USER_DISMISSED) { return; }
				WZ_Notification dismissed_notification;
				dismissed_notification.duration = 0;
				dismissed_notification.contentTitle = _("Tip: Adjusting Display Scale");
				std::string contextText = _("You can adjust the Display Scale at any time in the Video Options menu.");
				dismissed_notification.contentText = contextText;
				dismissed_notification.tag = DISPLAY_SCALE_PROMPT_TAG_PREFIX "dismissed_info";
				dismissed_notification.displayOptions = WZ_Notification_Display_Options::makeOneTime("displayscale::tip");
				addNotification(dismissed_notification, WZ_Notification_Trigger::Immediate());
			};
			notification.tag = DISPLAY_SCALE_PROMPT_INCREASE_TAG;
			notification.displayOptions = WZ_Notification_Display_Options::makeIgnorable("displayscale::prompt_increase", 4);

			addNotification(notification, WZ_Notification_Trigger(GAME_TICKS_PER_SEC * 2));
		}
	}
	lastProcessedDisplayScale = currentDisplayScale;
	lastProcessedScreenWidth = newWidth;
	lastProcessedScreenHeight = newHeight;
}
