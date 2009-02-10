/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#ifndef __INCLUDED_LIMITSDIALOG_HPP__
#define __INCLUDED_LIMITSDIALOG_HPP__

#include "heightmap.h"
#include "resource.h"
#include <deque>

/////////////////////////////////////////////////////////////////////////////
// LimitsDialog dialog

class LimitsDialog : public CDialog
{
	// Construction
	public:
		template<typename InputIterator>
		LimitsDialog(InputIterator first, InputIterator last, unsigned int MapWidth, unsigned int MapHeight, CWnd* parent = NULL) :
			CDialog(LimitsDialog::IDD, parent),
			_MaxX(0),
			_MaxZ(0),
			_MinX(0),
			_MinZ(0),
			_ScriptName(_T("")),
			_MapWidth(MapWidth),
			_MapHeight(MapHeight),
			_SelectedItemIndex(-1),
			_limitsList(first, last)
		{
		}

	public:
		std::deque<CScrollLimits>::const_iterator firstLimit();
		std::deque<CScrollLimits>::const_iterator lastLimit();

	private:
	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(LimitsDialog)
		public:
		virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	private:
		void RebuildList();
		bool GetEditFields(int& MinX, int& MinZ, int& MaxX, int& MaxZ, char* ScriptName);

		// Generated message map functions
		//{{AFX_MSG(LimitsDialog)
		virtual BOOL OnInitDialog();
		afx_msg void OnGetItemText(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnLimits_ListCtrlKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnLimits_ListCtrlItemFocused(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnAddLimits_ButtonClick();
		afx_msg void OnModify_ButtonClick();
		//}}AFX_MSG

		CListCtrl* Limits_ListCtrl;
		CEdit* ScriptName_EditBox;
		CEdit* MaxX_EditBox;
		CEdit* MaxZ_EditBox;
		CEdit* MinX_EditBox;
		CEdit* MinZ_EditBox;

	private:
		DECLARE_MESSAGE_MAP()

	private:
	// Dialog Data
		//{{AFX_DATA(LimitsDialog)
		enum { IDD = IDD_SCROLLLIMITS };
		int     _MaxX;
		int     _MaxZ;
		int     _MinX;
		int     _MinZ;
		CString _ScriptName;
		//}}AFX_DATA

		const unsigned int _MapWidth;
		const unsigned int _MapHeight;

		int _SelectedItemIndex;

		std::deque<CScrollLimits> _limitsList;
};

#endif // __INCLUDED_LIMITSDIALOG_HPP__
