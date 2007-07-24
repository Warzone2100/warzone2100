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
#include "debugprint.h"
#include "limitsdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLimitsDialog dialog


CLimitsDialog::CLimitsDialog(CHeightMap *World,CWnd* pParent /*=NULL*/)
	: CDialog(CLimitsDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLimitsDialog)
	m_MaxX = 0;
	m_MaxZ = 0;
	m_MinX = 0;
	m_MinZ = 0;
	m_ScriptName = _T("");
	//}}AFX_DATA_INIT

	m_World = World;
	m_SelectedItemIndex = -1;
}

//	CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);
//    List->InsertColumn (0, "Script Name", LVCFMT_LEFT, 128);

void CLimitsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLimitsDialog)
	DDX_Text(pDX, IDC_SL_MAXX, m_MaxX);
	DDX_Text(pDX, IDC_SL_MAXZ, m_MaxZ);
	DDX_Text(pDX, IDC_SL_MINX, m_MinX);
	DDX_Text(pDX, IDC_SL_MINZ, m_MinZ);
	DDX_Text(pDX, IDC_SL_SCRIPTNAME, m_ScriptName);
	DDV_MaxChars(pDX, m_ScriptName, 32);
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

	CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);

    List->InsertColumn (0, "Name", LVCFMT_LEFT, 128);
    List->InsertColumn (1, "Unique ID", LVCFMT_LEFT, 64);
    List->InsertColumn (2, "Min X", LVCFMT_LEFT, 48);
    List->InsertColumn (3, "Min Z", LVCFMT_LEFT, 48);
    List->InsertColumn (4, "Max X", LVCFMT_LEFT, 48);
    List->InsertColumn (5, "Max Z", LVCFMT_LEFT, 48);

	RebuildList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CLimitsDialog::OnGetdispinfoListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LVITEMA item = pDispInfo->item;
	
	char String[256];

	ListNode<CScrollLimits> *ScrollLimits = m_World->GetScrollLimits();
	CScrollLimits *Data = ScrollLimits->GetNthNode(item.iItem)->GetData();

	switch (pDispInfo->item.iSubItem) {
		case	0:
		    strcpy (pDispInfo->item.pszText, Data->ScriptName);
			break;
		case	1:
			sprintf(String,"%d",Data->UniqueID);
			strcpy (pDispInfo->item.pszText, String);
			break;
		case	2:
			sprintf(String,"%d",Data->MinX);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	3:
			sprintf(String,"%d",Data->MinZ);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	4:
			sprintf(String,"%d",Data->MaxX);
		    strcpy (pDispInfo->item.pszText, String);
			break;
		case	5:
			sprintf(String,"%d",Data->MaxZ);
		    strcpy (pDispInfo->item.pszText, String);
			break;
	}

	*pResult = 0;
}


void CLimitsDialog::OnKeydownListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDown = (LV_KEYDOWN*)pNMHDR;

	CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);

	switch(pLVKeyDown->wVKey) {
		case	VK_DELETE:
			if(m_SelectedItemIndex != -1) {
				m_World->DeleteScrollLimit(m_SelectedItemIndex);
				List->DeleteItem(m_SelectedItemIndex);
			}
			break;
	}
	
	*pResult = 0;
}


void CLimitsDialog::OnItemchangedListlimits(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	char String[256];
	
	if(pNMListView->iItem != m_SelectedItemIndex) {
		m_SelectedItemIndex = pNMListView->iItem;

		ListNode<CScrollLimits> *ScrollLimits = m_World->GetScrollLimits();
		ScrollLimits = ScrollLimits->GetNthNode(m_SelectedItemIndex);
		if(ScrollLimits) {
			CScrollLimits *Data = ScrollLimits->GetData();

			GetDlgItem(IDC_SL_SCRIPTNAME)->SetWindowText(Data->ScriptName);

			sprintf(String,"%d",Data->MinX);
			GetDlgItem(IDC_SL_MINX)->SetWindowText(String);

			sprintf(String,"%d",Data->MinZ);
			GetDlgItem(IDC_SL_MINZ)->SetWindowText(String);

			sprintf(String,"%d",Data->MaxX);
			GetDlgItem(IDC_SL_MAXX)->SetWindowText(String);

			sprintf(String,"%d",Data->MaxZ);
			GetDlgItem(IDC_SL_MAXZ)->SetWindowText(String);
		} else {
			m_SelectedItemIndex = -1;
			GetDlgItem(IDC_SL_SCRIPTNAME)->SetWindowText("");
			GetDlgItem(IDC_SL_MINX)->SetWindowText("");
			GetDlgItem(IDC_SL_MINZ)->SetWindowText("");
			GetDlgItem(IDC_SL_MAXX)->SetWindowText("");
			GetDlgItem(IDC_SL_MAXZ)->SetWindowText("");
		}
	}

	*pResult = 0;
}


void CLimitsDialog::OnAddlimits() 
{
	int MinX,MinZ,MaxX,MaxZ;
	char ScriptName[MAX_SCRIPTNAME];

	if(! GetEditFields(TRUE,MinX,MinZ,MaxX,MaxZ,ScriptName) ) {
		return;
	}

	CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);

	List->InsertItem(m_World->GetNumScrollLimits(),ScriptName);
	m_World->AddScrollLimit(MinX,MinZ,MaxX,MaxZ,ScriptName);

	GetDlgItem(IDC_LISTLIMITS)->SetFocus();
}


void CLimitsDialog::OnModify() 
{
	int MinX,MinZ,MaxX,MaxZ;
	char ScriptName[MAX_SCRIPTNAME];
	char String[256];

	if(! GetEditFields(TRUE,MinX,MinZ,MaxX,MaxZ,ScriptName) ) {
		return;
	}

	if(m_SelectedItemIndex >= 0) {
		m_World->SetScrollLimit(m_SelectedItemIndex,MinX,MinZ,MaxX,MaxZ,ScriptName);
		CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);
		List->SetItem( m_SelectedItemIndex, 0, LVIF_TEXT, ScriptName, 0, 0, 0, NULL);

   		sprintf(String,"%d",MinX);
		List->SetItem( m_SelectedItemIndex, 2, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String,"%d",MinZ);
		List->SetItem( m_SelectedItemIndex, 3, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String,"%d",MaxX);
		List->SetItem( m_SelectedItemIndex, 4, LVIF_TEXT, String, 0, 0, 0, NULL);
   		sprintf(String,"%d",MaxZ);
		List->SetItem( m_SelectedItemIndex, 5, LVIF_TEXT, String, 0, 0, 0, NULL);
	}

	GetDlgItem(IDC_LISTLIMITS)->SetFocus();
}


void CLimitsDialog::RebuildList(void)
{
	CListCtrl *List = (CListCtrl*)GetDlgItem(IDC_LISTLIMITS);

	List->DeleteAllItems();

	ListNode<CScrollLimits> *ScrollLimits = m_World->GetScrollLimits();
	ListNode<CScrollLimits> *TmpNode;
	CScrollLimits *Data;

	int Index = 0;
	TmpNode = ScrollLimits;
	while(TmpNode!=NULL) {
		Data = TmpNode->GetData();
		TmpNode = TmpNode->GetNextNode();
		List->InsertItem(Index,Data->ScriptName);
		Index++;
	}

	GetDlgItem(IDC_LISTLIMITS)->UpdateWindow();
	GetDlgItem(IDC_LISTLIMITS)->SetFocus();
}


BOOL CLimitsDialog::GetEditFields(BOOL CheckDup,int &MinX,int &MinZ,int &MaxX,int &MaxZ,char *ScriptName)
{
	char String[MAX_SCRIPTNAME];

	GetDlgItem(IDC_SL_MINX)->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MinX) != 1) {
		return FALSE;
	}

	GetDlgItem(IDC_SL_MINZ)->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MinZ) != 1) {
		return FALSE;
	}

	GetDlgItem(IDC_SL_MAXX)->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MaxX) != 1) {
		return FALSE;
	}

	GetDlgItem(IDC_SL_MAXZ)->GetWindowText(String,MAX_SCRIPTNAME);
	if( sscanf(String,"%d",&MaxZ) != 1) {
		return FALSE;
	}

	GetDlgItem(IDC_SL_SCRIPTNAME)->GetWindowText(String,MAX_SCRIPTNAME);
	strcpy(ScriptName,String);

	if( (MaxX <= MinX) || ( MaxZ <= MinZ) ) {
		return FALSE;
	}

	if( (MinX < 0) || (MinZ < 0) ||
		(MaxX < 0) || (MaxZ < 0) ) {
		return FALSE;
	}

	DWORD MapWidth,MapHeight;
	m_World->GetMapSize(&MapWidth,&MapHeight);

	if( (MinX > MapWidth) || (MinZ > MapHeight) ||
		(MaxX > MapWidth) || (MaxZ > MapHeight) ) {
		return FALSE;
	}

	// Check for duplicate script name.
	if(CheckDup) {
	}

	return TRUE;
}
