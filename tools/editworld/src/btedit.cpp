/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

	$Revision$
	$Id$
	$HeadURL$
*/
// BTEdit.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "btedit.h"
#include "aboutdialog.hpp"

#include "mainframe.h"
#include "bteditdoc.h"
#include "bteditview.h"

#include <string.h>
#include <string>
#include <boost/scoped_array.hpp>
#include "winapi.hpp"

using std::string;
using boost::scoped_array;

CBTEditCommandLineInfo g_cmdInfo;

void CBTEditCommandLineInfo::ParseParam(const char* param, BOOL flag, BOOL bLast)
{
	if (flag)
	{
		if (strcmp(param, "emulation") == 0)
		{
			m_bForceEmulation = TRUE;
			return;
		}
		if (strcmp(param, "hardware") == 0)
		{
			m_bForceHardware = TRUE;
			return;
		}
		if (strcmp(param, "primary") == 0)
		{
			m_bForcePrimary = TRUE;
			return;
		}
		if (strcmp(param, "secondary") == 0)
		{
			m_bForceSecondary = TRUE;
			return;
		}
		if (strcmp(param, "ramp") == 0)
		{
			m_bForceRamp = TRUE;
			return;
		}
		if (strcmp(param, "rgb") == 0)
		{
			m_bForceRGB = TRUE;
			return;
		}
//		if (strcmp(param, "nosizecheck") == 0)
//		{
//			m_bMapSizePower2 = FALSE;
//			return;
//		}
		m_bMapSizePower2 = FALSE;
	}
	CCommandLineInfo::ParseParam(param, flag, bLast);
}

/////////////////////////////////////////////////////////////////////////////
// CBTEditApp initialization

string g_HomeDirectory, g_WorkDirectory;

HCURSOR g_Wait;
HCURSOR g_Pointer;
HCURSOR g_PointerPaint;
HCURSOR g_PointerFill;
HCURSOR g_PointerDrag;
HCURSOR g_PointerSelRect;
HCURSOR g_PointerSelPoint;
HCURSOR g_PointerPipet;
HCURSOR g_PointerPliers;

HICON g_IconIncrement;
HICON g_IconDecrement;
HICON g_IconSmallBrush;
HICON g_IconSmallBrush2;
HICON g_IconLargeBrush;
HICON g_IconLargeBrush2;

/////////////////////////////////////////////////////////////////////////////
// The one and only CBTEditApp object

/////////////////////////////////////////////////////////////////////////////
// CBTEditApp

class CBTEditApp : public CWinApp
{
	// Overrides
	public:
		virtual BOOL InitInstance()
		{
			// Standard initialization
			// If you are not using these features and wish to reduce the size
			//  of your final executable, you should remove from the following
			//  the specific initialization routines you do not need.

		//	pDebugThread = AfxBeginThread(RUNTIME_CLASS(DebugThread));


		#ifdef _AFXDLL
			Enable3dControls();			// Call this when using MFC in a shared DLL
		#else
			Enable3dControlsStatic();	// Call this when linking to MFC statically
		#endif

			SetRegistryKey("Pumpkin Studios");
			LoadStdProfileSettings();  // Load standard INI file options (including MRU)

			// Register the application's document templates.  Document templates
			//  serve as the connection between documents, frame windows and views.

			CSingleDocTemplate* pDocTemplate;
			pDocTemplate = new CSingleDocTemplate(
				IDR_MAINFRAME,
				RUNTIME_CLASS(CBTEditDoc),
				RUNTIME_CLASS(CMainFrame),       // main SDI frame window
				RUNTIME_CLASS(CBTEditView));
			AddDocTemplate(pDocTemplate);

			g_HomeDirectory = Win::GetCurrentDirectory();

			g_Wait = ::LoadCursor(NULL,IDC_WAIT);
			g_Pointer = LoadCursor(IDC_POINTER);
			g_PointerPaint = LoadCursor(IDC_POINTER_PAINT);
			g_PointerFill = LoadCursor(IDC_POINTER_FILL);
			g_PointerDrag = LoadCursor(IDC_POINTER_HGTDRAG);
			g_PointerSelRect = LoadCursor(IDC_POINTER_SELRECT);
			g_PointerSelPoint = LoadCursor(IDC_POINTER);
			g_PointerPipet = LoadCursor(IDC_PIPET);
			g_PointerPliers = LoadCursor(IDC_PLIERS);

			g_IconIncrement = LoadIcon(IDI_INCREMENT);
			g_IconDecrement = LoadIcon(IDI_DECREMENT);
			g_IconSmallBrush = LoadIcon(IDI_SMALLBRUSH);
			g_IconSmallBrush2 = LoadIcon(IDI_SMALLBRUSH2);
			g_IconLargeBrush = LoadIcon(IDI_LARGEBRUSH);
			g_IconLargeBrush2 = LoadIcon(IDI_LARGEBRUSH2);

			// Parse command line for standard shell commands, DDE, file open
		//	CCommandLineInfo cmdInfo;
			ParseCommandLine(g_cmdInfo);

			// Dispatch commands specified on the command line
			if (!ProcessShellCommand(g_cmdInfo))
				return FALSE;

			return TRUE;
		}

	// Implementation
	private:
		void OnAppAbout()
		{
			AboutDialog().DoModal();
		}

	private:
		DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CBTEditApp, CWinApp)
	//{{AFX_MSG_MAP(CBTEditApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

CBTEditApp theApp;
