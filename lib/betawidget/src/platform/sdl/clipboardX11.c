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

/*
 * Something wicked this way comes...
 * Documentation/reference:
 *  http://svr-www.eng.cam.ac.uk/~er258/code/dist/x_clipboard/paste.cc
 */

#include <stdbool.h>
#include <assert.h>

#include <SDL_syswm.h>
#include <SDL.h>

static SDL_SysWMinfo info;

// Atoms
static Atom XA_CLIPBOARD;
static Atom XA_COMPOUND_TEXT;
static Atom XA_UTF8_STRING;
static Atom XA_TARGETS;

/**
 * Filters through SDL_Events searching for clipboard requests from the X
 * server.
 *
 * @param evt   The event to filter.
 */
static int widgetClipboardFilterX11(const SDL_Event *evt)
{
	// We are only interested in window manager events
	if (evt->type == SDL_SYSWMEVENT)
	{
		XEvent xevent = evt->syswm.msg->event.xevent;

		// See if the event is a selection/clipboard request
		if (xevent.type == SelectionRequest)
		{
			// Get the request in question
			XSelectionRequestEvent *request = &xevent.xselectionrequest;

			// Generate a reply to the selection request
			XSelectionEvent reply;

			reply.type = SelectionNotify;
			reply.serial = xevent.xany.send_event;
			reply.send_event = True;
			reply.display = info.info.x11.display;
			reply.requestor = request->requestor;
			reply.selection = request->selection;
			reply.property = request->property;
			reply.target = None;
			reply.time = request->time;

			// They want to know what we can provide/offer
			if (request->target == XA_TARGETS)
			{
				Atom possibleTargets[] =
				{
					XA_STRING,
					XA_UTF8_STRING,
					XA_COMPOUND_TEXT
				};

				XChangeProperty(info.info.x11.display, request->requestor,
				                request->property, XA_ATOM, 32, PropModeReplace,
				                (unsigned char *) possibleTargets, 3);
			}
			// They want a string (all we can provide)
			else if (request->target == XA_STRING
			      || request->target == XA_UTF8_STRING
			      || request->target == XA_COMPOUND_TEXT)
			{
				int len;
				char *xdata = XFetchBytes(info.info.x11.display, &len);

				XChangeProperty(info.info.x11.display, request->requestor,
				                request->property, request->target, 8,
				                PropModeReplace, (unsigned char *) xdata,
				                len);
				XFree(xdata);
			}
			else
			{
				// Did not have what they wanted, so no property set
				reply.property = None;
			}

			// Dispatch the event
			XSendEvent(request->display, request->requestor, 0, NoEventMask,
			           (XEvent *) &reply);
			XSync(info.info.x11.display, False);
		}
	}

	return 1;
}

static void widgetInitialiseClipboardX11()
{
	static bool initialised = false;

	if (!initialised)
	{
		// Get the window manager information
		SDL_GetWMInfo(&info);

		// Ensure we're running under X11
		assert(info.subsystem == SDL_SYSWM_X11);

		// Register the event filter
		SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
		SDL_SetEventFilter(widgetClipboardFilterX11);

		// Lock the connection to the X server
		info.info.x11.lock_func();

		// Get the clipboard atom (it is not defined by default)
		XA_CLIPBOARD = XInternAtom(info.info.x11.display, "CLIPBOARD", True);

		// Get the compound text type atom
		XA_COMPOUND_TEXT = XInternAtom(info.info.x11.display, "COMPOUND_TEXT",
		                               True);

		// UTF-8 string atom
		XA_UTF8_STRING = XInternAtom(info.info.x11.display, "UTF8_STRING",
		                             True);

		// TARGETS atom
		XA_TARGETS = XInternAtom(info.info.x11.display, "TARGETS", True);

		// Unlock the connection
		info.info.x11.unlock_func();

		// We are initialised
		initialised = true;
	}
}

char *widgetGetClipboardText()
{
	char *text = NULL;
	unsigned char *data = NULL;
	Atom type;
	int format, result;
	unsigned long len, bytesLeft, dummy;
	Window selectionOwner;

	// Make sure we are initialised
	widgetInitialiseClipboardX11();

	// Lock the connection
	info.info.x11.lock_func();

	// Get the owner of the clipboard selection
	selectionOwner = XGetSelectionOwner(info.info.x11.display, XA_CLIPBOARD);

	// If there is a selection (and therefore owner) fetch it
	if (selectionOwner != None)
	{
		SDL_Event event;
		bool response = false;

		/*
		 * Ask the window whom current owns the clipboard to convert it to an
		 * XA_UTF8_STRING and place it into the XA_CLIPBOARD property of our
		 * window.
		 */
		XConvertSelection(info.info.x11.display, XA_CLIPBOARD, XA_UTF8_STRING,
		                  XA_CLIPBOARD, info.info.x11.window, CurrentTime);
		XFlush(info.info.x11.display);

		/*
		 * We now need to wait for a response from the window that owns the
		 * clipboard.
		 */

		// Unlock the connection so that the SDL event loop may function
		info.info.x11.unlock_func();

		while (!response)
		{
			// Wait for an event
			SDL_WaitEvent(&event);

			// If the event is a window manager event
			if (event.type == SDL_SYSWMEVENT)
			{
				XEvent xevent = event.syswm.msg->event.xevent;

				// See if it is a response to our request
				if (xevent.type == SelectionNotify
				 && xevent.xselection.requestor == info.info.x11.window)
				{
					response = true;
				}
			}
		}

		// Lock the connection once again
		info.info.x11.lock_func();

		// See how much data is there
		XGetWindowProperty(info.info.x11.display, info.info.x11.window,
		                   XA_CLIPBOARD, 0, 0, False, AnyPropertyType, &type,
		                   &format, &len, &bytesLeft, &data);

		// If any 0-length data was returned, free it
		if (data)
		{
			XFree(data);
			data = NULL;
		}

		// If there is any data
		if (bytesLeft)
		{
			// Fetch the data
			result = XGetWindowProperty(info.info.x11.display,
			                            info.info.x11.window, XA_CLIPBOARD, 0,
			                            bytesLeft, False, AnyPropertyType,
			                            &type, &format, &len, &dummy, &data);

			// If we got some data, duplicate it
			if (result == Success)
			{
				text = strdup((char *) data);
				XFree(data);
			}
		}

		// Delete the property now that we are finished with it
		XDeleteProperty(info.info.x11.display, info.info.x11.window,
		                XA_CLIPBOARD);
	}

	// Unlock the connection
	info.info.x11.unlock_func();

	return text;
}

bool widgetSetClipboardText(const char *text)
{
	Window selectionOwner;

	// Make sure we are initialised
	widgetInitialiseClipboardX11();

	// Lock the connection
	info.info.x11.lock_func();

	// Copy the text into the root windows cut buffer (for Xterm compatibility)
	XStoreBytes(info.info.x11.display, text, strlen(text) + 1);

	// Set ourself as the owner of the CLIPBOARD atom
	XSetSelectionOwner(info.info.x11.display, XA_CLIPBOARD,
	                   info.info.x11.window, CurrentTime);

	// Check if we acquired ownership or not
	selectionOwner = XGetSelectionOwner(info.info.x11.display, XA_CLIPBOARD);

	// We got ownership
	if (selectionOwner == info.info.x11.window)
	{
		info.info.x11.unlock_func();
		return true;
	}
	// We did not get ownership
	else
	{
		info.info.x11.unlock_func();
		return false;
	}
}
