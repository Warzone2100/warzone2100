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
// ExpandLimitsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "debugprint.hpp"
#include "expandlimitsdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CExpandLimitsDlg dialog


CExpandLimitsDlg::CExpandLimitsDlg(CHeightMap *World,CWnd* pParent /*=NULL*/)
	: CDialog(CExpandLimitsDlg::IDD, pParent)
{
	m_World = World;
	m_ExcludeSelected = -1;
	m_IncludeSelected = -1;

	//{{AFX_DATA_INIT(CExpandLimitsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CExpandLimitsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpandLimitsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExpandLimitsDlg, CDialog)
	//{{AFX_MSG_MAP(CExpandLimitsDlg)
	ON_CBN_SELCHANGE(IDC_EXCLUDELIST, OnSelchangeExcludelist)
	ON_CBN_SELCHANGE(IDC_INCLUDELIST, OnSelchangeIncludelist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpandLimitsDlg message handlers

BOOL CExpandLimitsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_EXCLUDELIST);

	size_t ListSize = 0;
	const char* FirstString;

	// Add the strings to the list box.
	std::list<CScrollLimits>::const_iterator curNode;
	for (curNode = m_World->GetScrollLimits().begin(); curNode != m_World->GetScrollLimits().end(); ++curNode)
	{
		List->AddString(curNode->ScriptName);
		if(ListSize == 0)
			FirstString = curNode->ScriptName;

		++ListSize;
	}

	// Set the default selection.
	if(ListSize)
	{
		List->SelectString(-1, FirstString);
		m_ExcludeSelected = 0;
	}


	List = (CComboBox*)GetDlgItem(IDC_INCLUDELIST);

	ListSize = 0;

	// Add the strings to the list box.
	for (curNode = m_World->GetScrollLimits().begin(); curNode != m_World->GetScrollLimits().end(); ++curNode)
	{
		List->AddString(curNode->ScriptName);
		if(ListSize == 0)
			FirstString = curNode->ScriptName;

		++ListSize;
	}

	// Set the default selection.
	if(ListSize)
	{
		List->SelectString(-1, FirstString);
		m_IncludeSelected = 0;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpandLimitsDlg::OnSelchangeExcludelist() 
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_EXCLUDELIST);
	m_ExcludeSelected = List->GetCurSel();
}

void CExpandLimitsDlg::OnSelchangeIncludelist() 
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_INCLUDELIST);
	m_IncludeSelected = List->GetCurSel();
}
