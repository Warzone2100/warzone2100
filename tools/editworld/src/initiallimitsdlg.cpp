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
// InitialLimitsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "debugprint.hpp"
#include "initiallimitsdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInitialLimitsDlg dialog


CInitialLimitsDlg::CInitialLimitsDlg(CHeightMap *World,CWnd* pParent /*=NULL*/)
	: CDialog(CInitialLimitsDlg::IDD, pParent)
{
	m_World = World;
	m_Selected = -1;

	//{{AFX_DATA_INIT(CInitialLimitsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInitialLimitsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInitialLimitsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInitialLimitsDlg, CDialog)
	//{{AFX_MSG_MAP(CInitialLimitsDlg)
	ON_CBN_SELCHANGE(IDC_INITIALLIMITS, OnSelchangeInitiallimits)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInitialLimitsDlg message handlers

BOOL CInitialLimitsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CComboBox *List = (CComboBox*)GetDlgItem(IDC_INITIALLIMITS);

	ListNode<CScrollLimits> *TmpNode = m_World->GetScrollLimits();
	int ListSize = 0;
	char *FirstString;

	// Add the strings to the list box.
	while(TmpNode!=NULL) {
		List->AddString(TmpNode->GetData()->ScriptName);
		if(ListSize == 0) {
			FirstString = TmpNode->GetData()->ScriptName;
		}
		ListSize++;
		TmpNode = TmpNode->GetNextNode();
	}

	// Set the default selection.
	if(ListSize) {
		List->SelectString(-1, FirstString);
		m_Selected = 0;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInitialLimitsDlg::OnSelchangeInitiallimits() 
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_INITIALLIMITS);
	m_Selected = List->GetCurSel();
}
