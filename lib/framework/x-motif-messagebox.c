/*
	This file is part of Warzone 2100.
	Copyright (C) 2010  Giel van Schijndel
	Copyright (C) 2010  Warzone 2100 Project

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

#define WZ_NO_LCAPS_BOOL
#include "x-motif-messagebox.h"
#include <Xm/MessageB.h>
#include <Xm/Xm.h>
#include <stdlib.h>
#include "string_ext.h"
#include "wzglobal.h"

static void XmCloseMsgBox(Widget w, XtPointer clientData, XtPointer WZ_DECL_UNUSED callData)
{
	volatile bool* const keep_running = (volatile bool*)clientData;

	*keep_running = false;
	XtUnrealizeWidget(w);
}

void XmMessageBox(code_part part, const char* part_name, const char* msg)
{
	XtAppContext XtApp;
	Widget topWidget;

	char* trim_msg;
	volatile bool keep_running;

	// Create the X application shell object and toplevel widget
	{
		int argc = 0;
		char* argv[] = { NULL };

		topWidget = XtVaOpenApplication(&XtApp, "D2X-XL", NULL, 0, &argc, argv, NULL, applicationShellWidgetClass, NULL);
	}

	// Trim leading and trailing whitespace from message
	{
		size_t len = strlen(msg);

		// Trim leading whitespace
		while (len
		    && (msg[0] == '\r'
		     || msg[0] == '\n'
		     || msg[0] == '\t'
		     || msg[0] == ' '))
		{
			++msg;
			--len;
		}

		// Trim trailing whitespace
		while (len
		    && (msg[len - 1] == '\r'
		     || msg[len - 1] == '\n'
		     || msg[len - 1] == '\t'
		     || msg[len - 1] == ' '))
		{
			--len;
		}

		trim_msg = alloca(len + 1);
		strlcpy(trim_msg, msg, len + 1);
	}

	// Set up the dialog box
	{
		Widget xMsgBox;

		// setup message box text
		XmString str = XmStringCreateLocalized(trim_msg);
		XmString title = XmStringCreateLocalized((char*)part_name);

		Arg args[2];
		XtSetArg(args[0], XmNmessageString, str);
		XtSetArg(args[1], XmNdialogTitle, title);

		// use the built-in message box
		switch (part)
		{
			case LOG_ERROR:
			case LOG_FATAL:
				xMsgBox = XmCreateErrorDialog(topWidget, (char*)part_name, args, ARRAY_SIZE(args));
				break;

			case LOG_POPUP:
			case LOG_WARNING:
				xMsgBox = XmCreateWarningDialog(topWidget, (char*)part_name, args, ARRAY_SIZE(args));
				break;

			default:
				xMsgBox = XmCreateInformationDialog(topWidget, (char*)part_name, args, ARRAY_SIZE(args));
				break;
		}

		// remove text resource
		XmStringFree(str);
		XmStringFree(title);

		// remove help and cancel buttons
		XtUnmanageChild(XmMessageBoxGetChild(xMsgBox, XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(xMsgBox, XmDIALOG_HELP_BUTTON));

		// add callback to the "close" button that signals closing of the message box
		XtAddCallback(xMsgBox, XmNokCallback, XmCloseMsgBox, (bool*)&keep_running);
		XtManageChild(xMsgBox);
		XtRealizeWidget(xMsgBox);
	}

	keep_running = true;
	while (keep_running
	 || XtAppPending(XtApp))
	{
		XEvent event;
		XtAppNextEvent(XtApp, &event);
		XtDispatchEvent(&event);
	}

	XtDestroyWidget(topWidget);
	XtDestroyApplicationContext(XtApp);
}
