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
// BrushProp.h : header file
//

#ifndef __INCLUDED_BRUSHPROP_H__
#define __INCLUDED_BRUSHPROP_H__


/////////////////////////////////////////////////////////////////////////////
// CBrushProp dialog

// Valid values for _BrushSize.
enum {
	EBP_BS_SMALL,	// Small brush
	EBP_BS_LARGE,	// Large brush
};

// Valid values for _HeightSetting.
enum {
	EBP_HS_NOSET,	// Don't set heights
	EBP_HS_SETABB,	// Set absolute height
	EBP_HS_SEALEVEL,// Use sea level
};


#define WM_CLOSE_BRUSHPROP	(WM_USER+10)

class CBrushProp : public CDialog
{
	// Construction
	public:
		CBrushProp(CWnd* parent = NULL);   // standard constructor
		CBrushProp(CView* view);
		BOOL Create();
		void GetBrushData();
		void SetBrushData();

		CView *_View;
		int _OldHeight;

	// Dialog Data
		//{{AFX_DATA(CBrushProp)
		enum { IDD = IDD_EDGEBRUSH };
		CSliderCtrl	m_RandomSlider;
		CSliderCtrl	_HeightSlider;
		int		_Height;
		int		_RandomRange;
		int		_BrushID;
		int		_BrushSize;
		int		_HeightSetting;
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CBrushProp)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
		//}}AFX_VIRTUAL

	// Implementation
	private:
		// Generated message map functions
		//{{AFX_MSG(CBrushProp)
		virtual void OnCancel();
		afx_msg void OnChangeHeightedit();
		afx_msg void OnChangeRandedit();
		afx_msg void OnChangeBrushid();
		afx_msg void OnBrushidnext();
		afx_msg void OnBrushidprev();
		afx_msg void OnBrushlarge();
		afx_msg void OnBrushsize();
		afx_msg void OnAbheight();
		afx_msg void OnSetheight();
		afx_msg void OnWaterheight();
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
		//}}AFX_MSG

	private:
		DECLARE_MESSAGE_MAP()
};

#endif // __INCLUDED_BRUSHPROP_H__
