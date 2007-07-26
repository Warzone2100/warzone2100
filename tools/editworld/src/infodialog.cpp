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
// InfoDialog.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "infodialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoDialog dialog


CInfoDialog::CInfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInfoDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInfoDialog)
	m_Info = _T("");
	//}}AFX_DATA_INIT
	m_pView = NULL;
}


CInfoDialog::CInfoDialog(CView *pView)
{
	m_pView = pView;
}


BOOL CInfoDialog::Create(void)
{
	return CDialog::Create(CInfoDialog::IDD);	//_DEBUGINFO);
}


void CInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInfoDialog)
	DDX_Text(pDX, IDC_DEBUGINFO, m_Info);
	DDV_MaxChars(pDX, m_Info, 65535);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInfoDialog, CDialog)
	//{{AFX_MSG_MAP(CInfoDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfoDialog message handlers

void CInfoDialog::OnCancel() 
{
	if(m_pView != NULL) {
		m_pView->PostMessage(WM_GOODBYE, IDCANCEL);
	}
}

void CInfoDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_pView != NULL) {
		m_pView->PostMessage(WM_GOODBYE, IDOK);
	}
}
