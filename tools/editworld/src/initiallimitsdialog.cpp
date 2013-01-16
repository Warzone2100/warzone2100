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
// InitialLimitsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "initiallimitsdialog.hpp"

/////////////////////////////////////////////////////////////////////////////
// InitialLimitsDlg dialog


void InitialLimitsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(InitialLimitsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(InitialLimitsDlg, CDialog)
	//{{AFX_MSG_MAP(InitialLimitsDlg)
	ON_CBN_SELCHANGE(IDC_INITIALLIMITS, OnSelchangeInitiallimits)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// InitialLimitsDlg message handlers

BOOL InitialLimitsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	InitialLimits_Choice = (CComboBox*)GetDlgItem(IDC_INITIALLIMITS);

	// Add the strings to the list box.
	for (std::deque<std::string>::const_iterator curString = _stringList.begin(); curString != _stringList.end(); ++curString)
	{
		InitialLimits_Choice->AddString(curString->c_str());
	}

	// Set the default selection.
	if(!_stringList.empty())
	{
		InitialLimits_Choice->SelectString(-1, _stringList.front().c_str());
		_selected = 0;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void InitialLimitsDlg::OnSelchangeInitiallimits() 
{
	_selected = InitialLimits_Choice->GetCurSel();
}

int InitialLimitsDlg::Selected() const
{
	return _selected;
}

std::string InitialLimitsDlg::SelectedString() const
{
	if (_selected == -1)
		return std::string();

	return _stringList.at(_selected);
}
