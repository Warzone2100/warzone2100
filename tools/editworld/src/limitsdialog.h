/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
// LimitsDialog.h : header file
//

#include "directx.h"
#include "geometry.h"
#include "ddimage.h"
#include "heightmap.h"

/////////////////////////////////////////////////////////////////////////////
// CLimitsDialog dialog

class CLimitsDialog : public CDialog
{
	// Construction
	public:
		CLimitsDialog(CHeightMap *World,CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
		//{{AFX_DATA(CLimitsDialog)
		enum { IDD = IDD_SCROLLLIMITS };
		int     _MaxX;
		int     _MaxZ;
		int     _MinX;
		int     _MinZ;
		CString _ScriptName;
		//}}AFX_DATA

		CHeightMap* _World;
		int         _SelectedItemIndex;

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CLimitsDialog)
		public:
		virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	private:
		void RebuildList(void);
		bool GetEditFields(int& MinX, int& MinZ, int& MaxX, int& MaxZ, char* ScriptName);

		// Generated message map functions
		//{{AFX_MSG(CLimitsDialog)
		virtual BOOL OnInitDialog();
		afx_msg void OnGetdispinfoListlimits(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnKeydownListlimits(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnItemchangedListlimits(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnAddlimits();
		afx_msg void OnModify();
		//}}AFX_MSG

		CListCtrl* Limits_ListCtrl;
		CEdit* ScriptName_EditBox;
		CEdit* MaxX_EditBox;
		CEdit* MaxZ_EditBox;
		CEdit* MinX_EditBox;
		CEdit* MinZ_EditBox;

		DECLARE_MESSAGE_MAP()
};
