/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "btedit.h"

#include "mainframe.h"
#include "bteditdoc.h"
#include "bteditview.h"
//#include "dbview.h"
#include "tdview.h"
#include "textureview.h"
#include "wfview.h"

extern int g_MaxXRes;	// 3d view size.
extern int g_MaxYRes;
int g_DispXRes;
int g_DispYRes;

// Splitter layout, 0,1 or 2.
#define LAYOUT	1

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_INITMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_SEPARATOR,           // status line indicator
	ID_SEPARATOR,           // status line indicator
//	ID_INDICATOR_CAPS,
//	ID_INDICATOR_NUM,
//	ID_INDICATOR_SCRL,
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndToolBar2.Create(this) ||
		!m_wndToolBar2.LoadToolBar(IDR_TILEFLAGS))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndToolBar3.Create(this) ||
		!m_wndToolBar3.LoadToolBar(IDR_OBJECTSTATS))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}




	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	// TODO: Remove this if you don't want tool tips or a resizeable toolbar

	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	m_wndToolBar2.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar2.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar2);	//,AFX_IDW_DOCKBAR_LEFT);

	m_wndToolBar3.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar3.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar3,AFX_IDW_DOCKBAR_LEFT);

	return 0;
}

#if LAYOUT == 0
BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
// Split the window in two vertically.
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}
	m_wndSplitter.SetColumnInfo(0,640, 0);
	m_wndSplitter.SetRowInfo(0,480, 0);
	
// Now split the first pane in two horizontaly for the 3d and 2d views.
	if (!m_wndSplitter3.CreateStatic(
		&m_wndSplitter,     // our parent window is the first splitter
		2, 1,               // the new splitter is 2 rows, 1 column
		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
		m_wndSplitter.IdFromRowCol(0, 0) 
			// new splitter is in the first row, 2nd column of first splitter
	   ))
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}

// Attach the 3d edit view class to the top pane.
	if (!m_wndSplitter3.CreateView(0, 0,
		RUNTIME_CLASS(CBTEditView), CSize(640, 480), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

// And attach the 2d edit view to the bottom pane.
	if (!m_wndSplitter3.CreateView(1, 0,
		RUNTIME_CLASS(CWFView), CSize(640, 64), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	if (!m_wndSplitter.CreateView(0, 1,
		RUNTIME_CLASS(CTextureView), CSize(20, 20), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

//// The second horizontal split.
//	if (!m_wndSplitter2.CreateStatic(
//		&m_wndSplitter,     // our parent window is the first splitter
//		2, 1,               // the new splitter is 2 rows, 1 column
//		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
//		m_wndSplitter.IdFromRowCol(0, 1) 
//			// new splitter is in the first row, 2nd column of first splitter
//	   ))
//	{
//		TRACE0("Failed to create nested splitter\n");
//		return FALSE;
//	}
//
//	if (!m_wndSplitter2.CreateView(0, 0,
//		RUNTIME_CLASS(CDBView), CSize(20, 128), pContext))
//	{
//		TRACE0("Failed to create second pane\n");
//		return FALSE;
//	}

//// The second horizontal split.
//	if (!m_wndSplitter2.CreateStatic(
//		&m_wndSplitter,     // our parent window is the first splitter
//		2, 1,               // the new splitter is 2 rows, 1 column
//		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
//		m_wndSplitter.IdFromRowCol(0, 1) 
//			// new splitter is in the first row, 2nd column of first splitter
//	   ))
//	{
//		TRACE0("Failed to create nested splitter\n");
//		return FALSE;
//	}
//
//	if (!m_wndSplitter2.CreateView(0, 0,
//		RUNTIME_CLASS(CDBView), CSize(20, 128), pContext))
//	{
//		TRACE0("Failed to create second pane\n");
//		return FALSE;
//	}
//
//	if (!m_wndSplitter2.CreateView(1, 0,
//		RUNTIME_CLASS(CTextureView), CSize(20, 20), pContext))
//	{
//		TRACE0("Failed to create second pane\n");
//		return FALSE;
//	}

	return TRUE;
}

#elif LAYOUT == 1

BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// Split the window in two vertically.
	if (!m_wndSplitter.CreateStatic(this, 1, 2))	// One row, 2 columns.
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}
	m_wndSplitter.SetColumnInfo(0,640, 0);
	m_wndSplitter.SetRowInfo(0,480, 0);
	
	// Now split the first pane in two horizontaly for the 3d and 2d views.
	if (!m_wndSplitter3.CreateStatic(
		&m_wndSplitter,     // our parent window is the first splitter
		2, 1,               // the new splitter is 2 rows, 1 column
		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
		m_wndSplitter.IdFromRowCol(0, 1) 
			// new splitter is in the first row, 2nd column of first splitter
	   ))
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}

// Attach the 3d edit view class to the top pane.
	if (!m_wndSplitter3.CreateView(0, 0,
		RUNTIME_CLASS(CBTEditView), CSize(g_MaxXRes,g_MaxYRes), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

// And attach the 2d edit view to the bottom pane.
	if (!m_wndSplitter3.CreateView(1, 0,
		RUNTIME_CLASS(CWFView), CSize(g_MaxXRes, 64), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	if (!m_wndSplitter.CreateView(0, 0,
		RUNTIME_CLASS(CTextureView), CSize(g_DispXRes-g_MaxXRes, 20), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	return TRUE;
}

#elif LAYOUT == 2

BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// Split the window in two vertically.
	if (!m_wndSplitter.CreateStatic(this, 1, 2))	// One row, 2 columns.
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}
	m_wndSplitter.SetColumnInfo(0,640, 0);
	m_wndSplitter.SetRowInfo(0,480, 0);
	
	// Now split the first pane in two horizontaly for the 3d and 2d views.
	if (!m_wndSplitter3.CreateStatic(
		&m_wndSplitter,     // our parent window is the first splitter
		1, 2,               // the new splitter is 2 rows, 1 column
		WS_CHILD | WS_VISIBLE | WS_BORDER,  // style, WS_BORDER is needed
		m_wndSplitter.IdFromRowCol(0, 1) 
			// new splitter is in the first row, 2nd column of first splitter
	   ))
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}

// Attach the 3d edit view class to the top pane.
	if (!m_wndSplitter3.CreateView(0, 0,
		RUNTIME_CLASS(CBTEditView), CSize(800, 800), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

// And attach the 2d edit view to the bottom pane.
	if (!m_wndSplitter3.CreateView(0, 1,
		RUNTIME_CLASS(CWFView), CSize(640, 64), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	if (!m_wndSplitter.CreateView(0, 0,
		RUNTIME_CLASS(CTextureView), CSize(128, 20), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	return TRUE;
}

#endif


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_MAXIMIZE;

    // Set the window screen size and center it 
    cs.cy = ::GetSystemMetrics(SM_CYMAXIMIZED); 
    cs.cx = ::GetSystemMetrics(SM_CXMAXIMIZED); 
    cs.y = ((cs.cy) - cs.cy) / 2; 
    cs.x = ((cs.cx) - cs.cx) / 2;

	g_DispXRes = cs.cx;
	g_DispYRes = cs.cy;

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnInitMenu(CMenu* pMenu) 
{
	CFrameWnd::OnInitMenu(pMenu);
	
	int Ret = pMenu->EnableMenuItem(ID_FILE_EXPORTWARZONESCENARIOEXPAND,MF_GRAYED);
}
