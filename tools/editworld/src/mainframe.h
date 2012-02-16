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

#ifndef __INCLUDED_MAINFRAME_HPP__
#define __INCLUDED_MAINFRAME_HPP__

class CMainFrame : public CFrameWnd
{
	protected: // create from serialization only
		CMainFrame();
		DECLARE_DYNCREATE(CMainFrame)

	// Attributes
	protected:
		CSplitterWnd m_wndSplitter;
		CSplitterWnd m_wndSplitter3;
		CSplitterWnd m_wndSplitter2;

		CSplitterWnd m_wndSplitter4;
	public:

	// Operations
	public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMainFrame)
		public:
		virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		//}}AFX_VIRTUAL

	// Implementation
	public:
		virtual ~CMainFrame();

	public:	//protected:  // control bar embedded members
		CStatusBar  m_wndStatusBar;
		CToolBar    m_wndToolBar;
		CToolBar    m_wndToolBar2;
		CToolBar    m_wndToolBar3;

	// Generated message map functions
	protected:
		//{{AFX_MSG(CMainFrame)
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnInitMenu(CMenu* pMenu);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

#endif // __INCLUDED_MAINFRAME_HPP__
