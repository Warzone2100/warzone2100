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
// PlayerMap.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "playermap.h"

/////////////////////////////////////////////////////////////////////////////
// CPlayerMap dialog


CPlayerMap::CPlayerMap(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayerMap::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlayerMap)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPlayerMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlayerMap)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlayerMap, CDialog)
	//{{AFX_MSG_MAP(CPlayerMap)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlayerMap message handlers


#define NUM_CLANTYPES	8

char *ClanNames[NUM_CLANTYPES]={
	"Clan 1","Clan 2","Clan 3","Clan 4","Clan 5","Clan 6","Clan 7","Barbarian"
};

BOOL CPlayerMap::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_PLAYER1CLAN);
	int ListSize = 0;
	char *FirstString;

	// Add the strings to the list box.
	for(int i=0; i<NUM_CLANTYPES; i++) {
		List->AddString(ClanNames[i]);
		if(ListSize == 0) {
			FirstString = ClanNames[i];
		}
		ListSize++;
	}

	// Set the default selection.
	List->SelectString(-1, FirstString);
	m_Selected = 0;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
