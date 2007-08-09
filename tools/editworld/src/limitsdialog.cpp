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
// LimitsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "debugprint.hpp"
#include "limitsdialog.h"
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// CLimitsDialog dialog


CLimitsDialog::CLimitsDialog(CHeightMap* World, CWnd* pParent) :
	CDialog(CLimitsDialog::IDD, pParent),
	_MaxX(0),
	_MaxZ(0),
	_MinX(0),
	_MinZ(0),
	_ScriptName(_T("")),
	_World(World),
	_SelectedItemIndex(-1)
{
}

//    Limits_ListCtrl->InsertColumn (0, "Script Name", LVCFMT_LEFT, 128);

void CLimitsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLimitsDialog)
	DDX_Text(pDX, IDC_SL_MAXX, _MaxX);
	DDX_Text(pDX, IDC_SL_MAXZ, _MaxZ);
	DDX_Text(pDX, IDC_SL_MINX, _MinX);
	DDX_Text(pDX, IDC_SL_MINZ, _MinZ);
	DDX_Text(pDX, IDC_SL_SCRIPTNAME, _ScriptName);
	DDV_MaxChars(pDX, _ScriptName, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLimitsDialog, CDialog)
	//{{AFX_MSG_MAP(CLimitsDialog)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LISTLIMITS, OnGetdispinfoListlimits)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LISTLIMITS, OnKeydownListlimits)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTLIMITS, OnItemchangedListlimits)
	ON_BN_CLICKED(IDC_ADDLIMITS, OnAddlimits)
	ON_BN_CLICKED(IDC_MODIFY, OnModify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLimitsDialog message handlers

BOOL CLimitsDialog::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	
	return CDialog::Create(IDD, pParentWnd);
}


BOOL CLimitsDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	Limits_ListCtrl = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);
	ScriptName_EditBox = (CEdit*)GetDlgItem(IDC_SL_SCRIPTNAME);
	MaxX_EditBox = (CEdit*)GetDlgItem(IDC_SL_MAXX);
	MaxZ_EditBox = (CEdit*)GetDlgItem(IDC_SL_MAXZ);
	MinX_EditBox = (CEdit*)GetDlgItem(IDC_SL_MINX);
	MinZ_EditBox = (CEdit*)GetDlgItem(IDC_SL_MINZ);

    Limits_ListCtrl->InsertColumn (0, "Name",      LVCFMT_LEFT, 128);
    Limits_ListCtrl->InsertColumn (1, "Unique ID", LVCFMT_LEFT, 64);
    Limits_ListCtrl->InsertColumn (2, "Min X",     LVCFMT_LEFT, 48);
    Limits_ListCtrl->InsertColumn (3, "Min Z",     LVCFMT_LEFT, 48);
    Limits_ListCtrl->InsertColumn (4, "Max X",     LVCFMT_LEFT, 48);
    Limits_ListCtrl->InsertColumn (5, "Max Z",     LVCFMT_LEFT, 48);

	RebuildList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CLimitsDialog::OnGetdispinfoListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LVITEMA item = pDispInfo->item;
	
	char String[256];

	std::list<CScrollLimits>::const_iterator ScrollLimits = _World->GetScrollLimits().begin();
	std::advance(ScrollLimits, item.iItem);

	switch (pDispInfo->item.iSubItem) {
		case	0:
		    strcpy (pDispInfo->item.pszText, ScrollLimits->ScriptName);
			break;
		case	1:
			sprintf(String,"%d",ScrollLimits->UniqueID);
			strcpy (pDispInfo->item.pszText, String);
			break;
		case	2:
			sprintf(String,"%d",ScrollLimits->MinX);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	3:
			sprintf(String,"%d",ScrollLimits->MinZ);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	4:
			sprintf(String,"%d",ScrollLimits->MaxX);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	5:
			sprintf(String,"%d",ScrollLimits->MaxZ);
		    strcpy (pDispInfo->item.pszText, String);
			break;
	}

	*pResult = 0;
}


void CLimitsDialog::OnKeydownListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;

	switch(pLVKeyDown->wVKey)
	{
		case	VK_DELETE:
			if (_SelectedItemIndex != -1)
			{
				_World->DeleteScrollLimit(_SelectedItemIndex);
				Limits_ListCtrl->DeleteItem(_SelectedItemIndex);
			}
			break;
	}
	
	*pResult = 0;
}


void CLimitsDialog::OnItemchangedListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	char String[256];
	
	if(pNMListView->iItem != _SelectedItemIndex)
	{
		_SelectedItemIndex = pNMListView->iItem;

		if(_SelectedItemIndex < _World->GetScrollLimits().size())
		{
			std::list<CScrollLimits>::const_iterator ScrollLimits = _World->GetScrollLimits().begin();
			std::advance(ScrollLimits, _SelectedItemIndex);

			ScriptName_EditBox->SetWindowText(ScrollLimits->ScriptName);

			sprintf(String,"%d",ScrollLimits->MinX);
			MinX_EditBox->SetWindowText(String);

			sprintf(String,"%d",ScrollLimits->MinZ);
			MinZ_EditBox->SetWindowText(String);

			sprintf(String,"%d",ScrollLimits->MaxX);
			MaxX_EditBox->SetWindowText(String);

			sprintf(String,"%d",ScrollLimits->MaxZ);
			MaxZ_EditBox->SetWindowText(String);
		}
		else
		{
			_SelectedItemIndex = -1;
			ScriptName_EditBox->SetWindowText("");
			MinX_EditBox->SetWindowText("");
			MinZ_EditBox->SetWindowText("");
			MaxX_EditBox->SetWindowText("");
			MaxZ_EditBox->SetWindowText("");
		}
	}

	*pResult = 0;
}


void CLimitsDialog::OnAddlimits() 
{
	int MinX,MinZ,MaxX,MaxZ;
	char ScriptName[MAX_SCRIPTNAME];

	if(! GetEditFields(MinX,MinZ,MaxX,MaxZ,ScriptName) ) {
		return;
	}

	Limits_ListCtrl->InsertItem(_World->GetNumScrollLimits(), ScriptName);
	_World->AddScrollLimit(MinX, MinZ, MaxX, MaxZ, ScriptName);

	Limits_ListCtrl->SetFocus();
}


void CLimitsDialog::OnModify() 
{
	int MinX,MinZ,MaxX,MaxZ;
	char ScriptName[MAX_SCRIPTNAME];
	char String[256];

	if(! GetEditFields(MinX,MinZ,MaxX,MaxZ,ScriptName) ) {
		return;
	}

	if(_SelectedItemIndex >= 0)
	{
		_World->SetScrollLimit(_SelectedItemIndex, MinX, MinZ, MaxX, MaxZ, ScriptName);
		Limits_ListCtrl->SetItem(_SelectedItemIndex, 0, LVIF_TEXT, ScriptName, 0, 0, 0, NULL);

   		sprintf(String, "%d", MinX);
		Limits_ListCtrl->SetItem(_SelectedItemIndex, 2, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String, "%d", MinZ);
		Limits_ListCtrl->SetItem(_SelectedItemIndex, 3, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String, "%d", MaxX);
		Limits_ListCtrl->SetItem(_SelectedItemIndex, 4, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String, "%d", MaxZ);
		Limits_ListCtrl->SetItem(_SelectedItemIndex, 5, LVIF_TEXT, String, 0, 0, 0, NULL);
	}

	Limits_ListCtrl->SetFocus();
}


void CLimitsDialog::RebuildList(void)
{
	Limits_ListCtrl->DeleteAllItems();

	unsigned int Index = 0;
	for (std::list<CScrollLimits>::const_iterator curNode = _World->GetScrollLimits().begin(); curNode != _World->GetScrollLimits().end(); ++curNode, ++Index)
	{
		Limits_ListCtrl->InsertItem(Index, curNode->ScriptName);
	}

	Limits_ListCtrl->UpdateWindow();
	Limits_ListCtrl->SetFocus();
}


bool CLimitsDialog::GetEditFields(int& MinX, int& MinZ, int& MaxX, int& MaxZ, char* ScriptName)
{
	char String[MAX_SCRIPTNAME];

	MinX_EditBox->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MinX) != 1) {
		return false;
	}

	MinZ_EditBox->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MinZ) != 1) {
		return false;
	}

	MaxX_EditBox->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MaxX) != 1) {
		return false;
	}

	MaxZ_EditBox->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MaxZ) != 1) {
		return false;
	}

	ScriptName_EditBox->GetWindowText(String,MAX_SCRIPTNAME);
	strcpy(ScriptName,String);

	if( (MaxX <= MinX) || ( MaxZ <= MinZ) ) {
		return false;
	}

	if( (MinX < 0) || (MinZ < 0) ||
		(MaxX < 0) || (MaxZ < 0) ) {
		return false;
	}

	DWORD MapWidth,MapHeight;
	_World->GetMapSize(&MapWidth,&MapHeight);

	if (MinX > MapWidth
	 || MinZ > MapHeight
	 || MaxX > MapWidth
	 || MaxZ > MapHeight)
		return false;

	return true;
}
