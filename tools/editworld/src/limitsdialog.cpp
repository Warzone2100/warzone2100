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

#include "stdafx.h"
#include "limitsdialog.hpp"
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// LimitsDialog dialog


//    Limits_ListCtrl->InsertColumn (0, "Script Name", LVCFMT_LEFT, 128);

void LimitsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LimitsDialog)
	DDX_Text(pDX, IDC_SL_MAXX, _MaxX);
	DDX_Text(pDX, IDC_SL_MAXZ, _MaxZ);
	DDX_Text(pDX, IDC_SL_MINX, _MinX);
	DDX_Text(pDX, IDC_SL_MINZ, _MinZ);
	DDX_Text(pDX, IDC_SL_SCRIPTNAME, _ScriptName);
	DDV_MaxChars(pDX, _ScriptName, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(LimitsDialog, CDialog)
	//{{AFX_MSG_MAP(LimitsDialog)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LISTLIMITS, OnGetItemText)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LISTLIMITS, OnLimits_ListCtrlKeyDown)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTLIMITS, OnLimits_ListCtrlItemFocused)
	ON_BN_CLICKED(IDC_ADDLIMITS, OnAddLimits_ButtonClick)
	ON_BN_CLICKED(IDC_MODIFY, OnModify_ButtonClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

std::deque<CScrollLimits>::const_iterator LimitsDialog::firstLimit()
{
	return _limitsList.begin();
}

std::deque<CScrollLimits>::const_iterator LimitsDialog::lastLimit()
{
	return _limitsList.end();
}

/////////////////////////////////////////////////////////////////////////////
// LimitsDialog message handlers

BOOL LimitsDialog::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	
	return CDialog::Create(IDD, pParentWnd);
}


BOOL LimitsDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	Limits_ListCtrl = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);
	ScriptName_EditBox = (CEdit*)GetDlgItem(IDC_SL_SCRIPTNAME);
	MaxX_EditBox = (CEdit*)GetDlgItem(IDC_SL_MAXX);
	MaxZ_EditBox = (CEdit*)GetDlgItem(IDC_SL_MAXZ);
	MinX_EditBox = (CEdit*)GetDlgItem(IDC_SL_MINX);
	MinZ_EditBox = (CEdit*)GetDlgItem(IDC_SL_MINZ);

	Limits_ListCtrl->InsertColumn(0, _T("Name"),      LVCFMT_LEFT, 128);
	Limits_ListCtrl->InsertColumn(1, _T("Unique ID"), LVCFMT_LEFT, 64);
	Limits_ListCtrl->InsertColumn(2, _T("Min X"),     LVCFMT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(3, _T("Min Z"),     LVCFMT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(4, _T("Max X"),     LVCFMT_LEFT, 48);
	Limits_ListCtrl->InsertColumn(5, _T("Max Z"),     LVCFMT_LEFT, 48);

	RebuildList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void LimitsDialog::OnGetItemText(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LVITEMA& item = reinterpret_cast<LV_DISPINFO*>(pNMHDR)->item;

	if (item.iItem < 0
	 || item.iItem >= _limitsList.size())
		return;
	
	std::deque<CScrollLimits>::const_iterator ScrollLimits = _limitsList.begin();
	std::advance(ScrollLimits, item.iItem);

	switch (item.iSubItem)
	{
		case	0:
		    strcpy (item.pszText, ScrollLimits->ScriptName);
			break;
		case	1:
			sprintf(item.pszText, "%d", ScrollLimits->UniqueID);
			break;
		case	2:
			sprintf(item.pszText, "%d", ScrollLimits->MinX);
			break;
		case	3:
			sprintf(item.pszText, "%d", ScrollLimits->MinZ);
			break;
		case	4:
			sprintf(item.pszText, "%d", ScrollLimits->MaxX);
			break;
		case	5:
			sprintf(item.pszText, "%d", ScrollLimits->MaxZ);
			break;
	}

	*pResult = 0;
}


void LimitsDialog::OnLimits_ListCtrlKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;

	switch(pLVKeyDown->wVKey)
	{
		case	VK_DELETE:
			if (_SelectedItemIndex != -1)
			{
				std::deque<CScrollLimits>::iterator toErase = _limitsList.begin();
				std::advance(toErase, _SelectedItemIndex);
				_limitsList.erase(toErase);
				Limits_ListCtrl->DeleteItem(_SelectedItemIndex);
			}
			break;
	}
	
	*pResult = 0;
}


void LimitsDialog::OnLimits_ListCtrlItemFocused(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	char String[256];
	
	if(pNMListView->iItem != _SelectedItemIndex)
	{
		_SelectedItemIndex = pNMListView->iItem;

		if(_SelectedItemIndex < _limitsList.size())
		{
			ScriptName_EditBox->SetWindowText(_limitsList[_SelectedItemIndex].ScriptName);

			sprintf(String,"%d", _limitsList[_SelectedItemIndex].MinX);
			MinX_EditBox->SetWindowText(String);

			sprintf(String,"%d", _limitsList[_SelectedItemIndex].MinZ);
			MinZ_EditBox->SetWindowText(String);

			sprintf(String,"%d", _limitsList[_SelectedItemIndex].MaxX);
			MaxX_EditBox->SetWindowText(String);

			sprintf(String,"%d", _limitsList[_SelectedItemIndex].MaxZ);
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


void LimitsDialog::OnAddLimits_ButtonClick() 
{
	CScrollLimits limits;

	limits.UniqueID = _limitsList.back().UniqueID + 1;

	if(!GetEditFields(limits.MinX, limits.MinZ, limits.MaxX, limits.MaxZ, limits.ScriptName))
		return;

	Limits_ListCtrl->InsertItem(_limitsList.size(), limits.ScriptName);
	_limitsList.push_back(limits);

	Limits_ListCtrl->SetFocus();
}


void LimitsDialog::OnModify_ButtonClick() 
{
	if (_SelectedItemIndex < 0
	 || _SelectedItemIndex >= _limitsList.size())
		return;

	CScrollLimits& limits = _limitsList[_SelectedItemIndex];

	GetEditFields(limits.MinX, limits.MinZ, limits.MaxX, limits.MaxZ, limits.ScriptName);

	Limits_ListCtrl->SetItem(_SelectedItemIndex, 0, LVIF_TEXT, limits.ScriptName, 0, 0, 0, NULL);

	char String[256];
	sprintf(String, "%d", limits.MinX);
	Limits_ListCtrl->SetItem(_SelectedItemIndex, 2, LVIF_TEXT, String, 0, 0, 0, NULL);
	sprintf(String, "%d", limits.MinZ);
	Limits_ListCtrl->SetItem(_SelectedItemIndex, 3, LVIF_TEXT, String, 0, 0, 0, NULL);
	sprintf(String, "%d", limits.MaxX);
	Limits_ListCtrl->SetItem(_SelectedItemIndex, 4, LVIF_TEXT, String, 0, 0, 0, NULL);
	sprintf(String, "%d", limits.MaxZ);
	Limits_ListCtrl->SetItem(_SelectedItemIndex, 5, LVIF_TEXT, String, 0, 0, 0, NULL);

	Limits_ListCtrl->SetFocus();
}


void LimitsDialog::RebuildList()
{
	Limits_ListCtrl->DeleteAllItems();

	unsigned int Index = 0;
	for (std::deque<CScrollLimits>::const_iterator curNode = _limitsList.begin(); curNode != _limitsList.end(); ++curNode, ++Index)
	{
		Limits_ListCtrl->InsertItem(Index, curNode->ScriptName);
	}

	Limits_ListCtrl->UpdateWindow();
	Limits_ListCtrl->SetFocus();
}


bool LimitsDialog::GetEditFields(int& MinX, int& MinZ, int& MaxX, int& MaxZ, char* ScriptName)
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

	if (MinX > _MapWidth
	 || MinZ > _MapHeight
	 || MaxX > _MapWidth
	 || MaxZ > _MapHeight)
		return false;

	return true;
}
