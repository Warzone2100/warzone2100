/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#include "event.h"

#include "../../widget.h"
#include "../../window.h"

#include "../../../../framework/utf.h"

/**
 * Converts an SDLkey to an eventKeycode (as used by the widget system).
 *
 * @param key   The SDL key to convert from.
 * @return The equivalent eventKeycode, or EVT_KEYCODE_UNKNOWN if there is no
 *         equivalent.
 */
static eventKeycode SDLKeyToEventKeycode(SDLKey key)
{
	switch (key)
	{
		case SDLK_BACKSPACE:
			return EVT_KEYCODE_BACKSPACE;
		case SDLK_TAB:
			return EVT_KEYCODE_TAB;
		case SDLK_RETURN:
			return EVT_KEYCODE_RETURN;
		case SDLK_ESCAPE:
			return EVT_KEYCODE_ESCAPE;
		case SDLK_KP_ENTER:
			return EVT_KEYCODE_ENTER;
		case SDLK_LEFT:
			return EVT_KEYCODE_LEFT;
		case SDLK_RIGHT:
			return EVT_KEYCODE_RIGHT;
		case SDLK_UP:
			return EVT_KEYCODE_UP;
		case SDLK_DOWN:
			return EVT_KEYCODE_DOWN;
		case SDLK_INSERT:
			return EVT_KEYCODE_INSERT;
		case SDLK_DELETE:
			return EVT_KEYCODE_DELETE;
		case SDLK_HOME:
			return EVT_KEYCODE_HOME;
		case SDLK_END:
			return EVT_KEYCODE_END;
		case SDLK_PAGEUP:
			return EVT_KEYCODE_PAGEUP;
		case SDLK_PAGEDOWN:
			return EVT_KEYCODE_PAGEDOWN;
		default:
			return EVT_KEYCODE_UNKNOWN;
	}
}

/**
 * Converts SDL mouse button ids to their corresponding mouseButton enumeration.
 *
 * @param buttonId  The button which was pressed.
 * @return The corresponding mouseButton enum.
 */
static mouseButton SDLButtonToMouseButton(int buttonId)
{
	switch (buttonId)
	{
		case 1:
			return BUTTON_LEFT;
		case 2:
			return BUTTON_RIGHT;
		case 4:
			return BUTTON_WHEEL_UP;
		case 5:
			return BUTTON_WHEEL_DOWN;
		default:
			return BUTTON_OTHER;
	}
}

void widgetHandleSDLEvent(const SDL_Event *sdlEvt)
{
	// Last known location of the mouse
	static point previousMouseLoc = { -1, -1 };

	switch (sdlEvt->type)
	{
		case SDL_MOUSEMOTION:
		{
			eventMouse evtMouse;
			evtMouse.event = widgetCreateEvent(EVT_MOUSE_MOVE);

			// Location
			evtMouse.loc.x = sdlEvt->motion.x;
			evtMouse.loc.y = sdlEvt->motion.y;

			// Previous location
			evtMouse.previousLoc = previousMouseLoc;

			// Update the previous location
			previousMouseLoc = evtMouse.loc;

			windowHandleEventForWindowVector((event *) &evtMouse);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			eventMouseBtn evtMouseBtn;
			evtMouseBtn.event = widgetCreateEvent(sdlEvt->button.state == SDL_PRESSED ?
			                                      EVT_MOUSE_DOWN : EVT_MOUSE_UP);

			// Location
			evtMouseBtn.loc.x = sdlEvt->button.x;
			evtMouseBtn.loc.y = sdlEvt->button.y;

			// Update the previous location
			previousMouseLoc = evtMouseBtn.loc;

			// Button pressed/released
			evtMouseBtn.button = SDLButtonToMouseButton(sdlEvt->button.button);

			windowHandleEventForWindowVector((event *) &evtMouseBtn);
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			eventKey evtKey;
			evtKey.event = widgetCreateEvent(sdlEvt->key.type == SDL_KEYDOWN ?
			                                 EVT_KEY_DOWN : EVT_KEY_UP);

			// Key pressed/released
			evtKey.keycode = SDLKeyToEventKeycode(sdlEvt->key.keysym.sym);

			// Modifier keys
			if (sdlEvt->key.keysym.mod & KMOD_CTRL)
			{
				evtKey.ctrl = true;
			}
			if (sdlEvt->key.keysym.mod & KMOD_ALT)
			{
				evtKey.alt = true;
			}
			if (sdlEvt->key.keysym.mod & KMOD_SHIFT)
			{
				evtKey.shift = true;
			}

			// Only needed/works on SDL 1.2!
			if (sdlEvt->key.keysym.unicode)
			{
				const uint16_t utf16[2] = { sdlEvt->key.keysym.unicode, 0 };
				char *utf8;

				// Create the text event
				eventText evtText;
				evtText.event = widgetCreateEvent(EVT_TEXT);

				// Convert the UTF-16 string to UTF-8
				utf8 = UTF16toUTF8(utf16, NULL);

				// Set the text of the event to UTF-8 string
				evtText.utf8 = utf8;

				// Dispatch the event
				windowHandleEventForWindowVector((event *) &evtText);

				// Free the memory allocated by UTF16toUTF8
				free(utf8);
			}

			windowHandleEventForWindowVector((event *) &evtKey);
			break;
		}
	}
}

void widgetFireTimers()
{
	// Create the generic timer event
	eventTimer evtTimer;
	evtTimer.event = widgetCreateEvent(EVT_TIMER);

	// Dispatch
	windowHandleEventForWindowVector((event *) &evtTimer);
}
