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

#ifndef __INCLUDE_AUTOFLAGDIALOG_HPP__
#define __INCLUDE_AUTOFLAGDIALOG_HPP__

/////////////////////////////////////////////////////////////////////////////
// AutoFlagDialog dialog

class AutoFlagDialog : public CDialog
{
	public:
		// Construction
		AutoFlagDialog(CWnd* parent = NULL,
		               bool RandRotate = false,
		               bool RandXFlip = false,
		               bool RandYFlip = false,
		               bool XFlip = false,
		               bool YFlip = false,
		               unsigned int Rotate = 0,
		               bool IncRotate = false,
		               bool ToggleXFlip = false,
		               bool ToggleYFlip = false);

		bool RandRotate() const;
		bool RandXFlip() const;
		bool RandYFlip() const;
		bool XFlip() const;
		bool YFlip() const;
		unsigned int Rotate() const;
		bool IncRotate() const;
		bool ToggleXFlip() const;
		bool ToggleYFlip() const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoFlagDialog)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	private:

		// Generated message map functions
		//{{AFX_MSG(CAutoFlagDialog)
		virtual BOOL OnInitDialog();
		virtual void OnOK();
		afx_msg void OnRandRotate();
		afx_msg void OnRandXFlip();
		afx_msg void OnRandYFlip();
		afx_msg void OnRotate();
		afx_msg void OnXFlip();
		afx_msg void OnYFlip();
		//}}AFX_MSG

		CButton* Degree0_RadioButton;
		CButton* Degree90_RadioButton;
		CButton* Degree180_RadioButton;
		CButton* Degree270_RadioButton;
		CButton* XFlip_CheckBox;
		CButton* YFlip_CheckBox;
		CButton* RandRotate_CheckBox;
		CButton* RandXFlip_CheckBox;
		CButton* RandYFlip_CheckBox;
//		CButton* IncrementRotate_CheckBox;
//		CButton* ToggleXFlip_CheckBox;
//		CButton* ToggleYFlip_CheckBox;

	private:
		DECLARE_MESSAGE_MAP();

	private:
		// Dialog Data
		//{{AFX_DATA(CAutoFlagDialog)
		enum { IDD = IDD_AUTOFLAGDIALOG };
		//}}AFX_DATA

		bool _RandRotate;
		bool _RandXFlip;
		bool _RandYFlip;
		bool _XFlip;
		bool _YFlip;
		unsigned int _Rotate;
		bool _IncRotate;
		bool _ToggleXFlip;
		bool _ToggleYFlip;
};

#endif // __INCLUDE_AUTOFLAGDIALOG_HPP__
