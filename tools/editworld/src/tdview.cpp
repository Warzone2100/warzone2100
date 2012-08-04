/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
// TDView.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "tdview.h"

/////////////////////////////////////////////////////////////////////////////
// CTDView

IMPLEMENT_DYNCREATE(CTDView, CView)

CTDView::CTDView()
{
}

CTDView::~CTDView()
{
}


BEGIN_MESSAGE_MAP(CTDView, CView)
	//{{AFX_MSG_MAP(CTDView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDView drawing

void CTDView::OnDraw(CDC* pDC)
{
	CBTEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code here

//	pDC->TextOut(16,16,"Splitter View 2 (CTDView)");


//	CListCtrl* List=(CListCtrl*)GetDlgItem(IDC_LISTVIEW);
	CListCtrl List;

	RECT ClientRect;
	GetClientRect(&ClientRect);
	List.Create(LVS_LIST,ClientRect,this,1);
}

CBTEditDoc* CTDView::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTEditDoc)));
	return (CBTEditDoc*)m_pDocument;
}
