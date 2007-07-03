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
*/
// AutoFlagDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAutoFlagDialog dialog

class CAutoFlagDialog : public CDialog
{
// Construction
public:
	CAutoFlagDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAutoFlagDialog)
	enum { IDD = IDD_AUTOFLAGDIALOG };
	//}}AFX_DATA
	BOOL m_IncRotate;
	BOOL m_TogXFlip;
	BOOL m_TogYFlip;
	BOOL m_RandRotate;
	BOOL m_RandXFlip;
	BOOL m_RandYFlip;
	BOOL m_XFlip;
	BOOL m_YFlip;
	int m_Rotate;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoFlagDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAutoFlagDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChkrandrotate();
	afx_msg void OnChkrandxflip();
	afx_msg void OnChkrandyflip();
	afx_msg void OnRot0();
	afx_msg void OnRot180();
	afx_msg void OnRot270();
	afx_msg void OnRot90();
	afx_msg void OnXflip();
	afx_msg void OnYflip();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
