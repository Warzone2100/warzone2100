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
// BrushProp.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "wfview.h"
#include "debugprint.hpp"

#include "brushprop.h"

/////////////////////////////////////////////////////////////////////////////
// CBrushProp dialog


static CBTEditDoc *m_pDoc;

CBrushProp::CBrushProp(CWnd* parent) :
	CDialog(CBrushProp::IDD, parent),
	_View(NULL),
	_Height(0),
	_RandomRange(0),
	_BrushID(0),
	_BrushSize(-1),
	_HeightSetting(-1)
{
}


CBrushProp::CBrushProp(CView* view) :
	_View(view),
	_OldHeight(255),
	_Height(0),
	_RandomRange(0),
	_BrushID(0),
	_BrushSize(1),
	_HeightSetting(0)
{

	m_pDoc = ((CWFView*)_View)->GetDocument();
	ASSERT_VALID(m_pDoc);
}


BOOL CBrushProp::Create()
{
	BOOL Ok = CDialog::Create(CBrushProp::IDD);

	if(Ok) {
		_HeightSlider.SetRange( 0, 255, TRUE );
		_HeightSlider.SetTicFreq( 16 );
		m_RandomSlider.SetRange( 0, 64, TRUE );
		m_RandomSlider.SetTicFreq( 8 );
		GetBrushData();
	}

	return Ok;
}


void CBrushProp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrushProp)
	DDX_Control(pDX, IDC_RANDOMRANGE, m_RandomSlider);
	DDX_Control(pDX, IDC_HEIGHTSLIDER, _HeightSlider);
	DDX_Text(pDX, IDC_HEIGHTEDIT, _Height);
	DDV_MinMaxInt(pDX, _Height, 0, 255);
	DDX_Text(pDX, IDC_RANDEDIT, _RandomRange);
	DDV_MinMaxInt(pDX, _RandomRange, 0, 64);
	DDX_Text(pDX, IDC_BRUSHID, _BrushID);
	DDV_MinMaxInt(pDX, _BrushID, 0, 15);
	DDX_Radio(pDX, IDC_BRUSHSIZE, _BrushSize);
	DDX_Radio(pDX, IDC_SETHEIGHT, _HeightSetting);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrushProp, CDialog)
	//{{AFX_MSG_MAP(CBrushProp)
	ON_EN_CHANGE(IDC_HEIGHTEDIT, OnChangeHeightedit)
	ON_EN_CHANGE(IDC_RANDEDIT, OnChangeRandedit)
	ON_EN_CHANGE(IDC_BRUSHID, OnChangeBrushid)
	ON_BN_CLICKED(IDC_BRUSHIDNEXT, OnBrushidnext)
	ON_BN_CLICKED(IDC_BRUSHIDPREV, OnBrushidprev)
	ON_BN_CLICKED(IDC_BRUSHLARGE, OnBrushlarge)
	ON_BN_CLICKED(IDC_BRUSHSIZE, OnBrushsize)
	ON_BN_CLICKED(IDC_ABHEIGHT, OnAbheight)
	ON_BN_CLICKED(IDC_SETHEIGHT, OnSetheight)
	ON_BN_CLICKED(IDC_WATERHEIGHT, OnWaterheight)
	ON_WM_KILLFOCUS()
	ON_WM_KEYUP()
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrushProp message handlers

void CBrushProp::OnCancel() 
{
	DestroyWindow();
//	CDialog::OnCancel();
}


void CBrushProp::GetBrushData()
{
	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();

	_BrushID = m_pDoc->GetEdgeBrushID();

	_Height = EdgeBrush->_Height;
	_OldHeight = (255-_Height);
	_HeightSlider.SetPos(_OldHeight);

	_RandomRange = EdgeBrush->_RandomRange;
	m_RandomSlider.SetPos(_RandomRange);

	if(EdgeBrush->GetLargeBrush()) {
		_BrushSize = EBP_BS_LARGE;
	} else {
		_BrushSize = EBP_BS_SMALL;
	}

	_HeightSetting = EdgeBrush->_HeightMode;

	// Set dialog's controls from brush data.
	UpdateData(FALSE);

	GetDlgItem(IDC_TILEBUT1)->InvalidateRect(NULL,FALSE);
}


void CBrushProp::SetBrushData()
{
	// Get data from the dialog's controls.
	UpdateData(TRUE);

	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();

	EdgeBrush->_Height = _Height;
	EdgeBrush->_RandomRange = _RandomRange;

	if(_BrushSize == EBP_BS_SMALL) {
		EdgeBrush->SetLargeBrush(FALSE);
	} else {
		EdgeBrush->SetLargeBrush(TRUE);
	}

	EdgeBrush->_HeightMode = _HeightSetting;
}


BOOL CBrushProp::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *Message = (NMHDR*)lParam;

	if(wParam == IDC_HEIGHTSLIDER) {
//		DebugPrint("%d %d\n",Message->idFrom,Message->code);
   		if(_OldHeight != _HeightSlider.GetPos()) {
   			_OldHeight = _HeightSlider.GetPos();
   			_Height = 255-_HeightSlider.GetPos();
   			UpdateData(FALSE);
   		}
		if(Message->code != NM_CUSTOMDRAW) {
			SetBrushData();
		}
	}

	if(wParam == IDC_RANDOMRANGE) {
//		DebugPrint("%d %d\n",Message->idFrom,Message->code);
		if(_RandomRange != m_RandomSlider.GetPos()) {
			_RandomRange = m_RandomSlider.GetPos();
			UpdateData(FALSE);
		}
		if(Message->code != NM_CUSTOMDRAW) {
			SetBrushData();
		}
	}

	return CDialog::OnNotify(wParam, lParam, pResult);
}


void CBrushProp::OnChangeHeightedit() 
{
	UpdateData();
	_OldHeight = (255-_Height);
	_HeightSlider.SetPos(255-_Height);
	SetBrushData();
}



void CBrushProp::OnChangeRandedit() 
{
	UpdateData();
	m_RandomSlider.SetPos(_RandomRange);
	SetBrushData();
}



void CBrushProp::OnChangeBrushid() 
{
	GetBrushData();
}

void CBrushProp::OnBrushidnext() 
{
	SetBrushData();

	_BrushID ++;
	if(_BrushID > 15) {
		_BrushID = 0;
	}

	m_pDoc->SetEdgeBrush(_BrushID);
	GetBrushData();
}

void CBrushProp::OnBrushidprev() 
{
	SetBrushData();

	_BrushID --;
	if(_BrushID < 0) {
		_BrushID = 15;
	}

	m_pDoc->SetEdgeBrush(_BrushID);
	GetBrushData();
}




void CBrushProp::OnBrushlarge() 
{
	SetBrushData();
}

void CBrushProp::OnBrushsize() 
{
	SetBrushData();
}




void CBrushProp::OnAbheight() 
{
	SetBrushData();
}


void CBrushProp::OnSetheight() 
{
	SetBrushData();
}
 
void CBrushProp::OnWaterheight() 
{
	SetBrushData();
}



void CBrushProp::OnKillFocus(CWnd* pNewWnd) 
{
	CDialog::OnKillFocus(pNewWnd);
}

void CBrushProp::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

/*
typedef struct tagDRAWITEMSTRUCT {
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemAction;
    UINT        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem;
    DWORD       itemData;
} DRAWITEMSTRUCT, NEAR *PDRAWITEMSTRUCT, FAR *LPDRAWITEMSTRUCT;
*/

void CBrushProp::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	int ButWidth = lpDrawItemStruct->rcItem.right&0xfffc;
	int ButHeight = lpDrawItemStruct->rcItem.bottom;
	
	CDIBDraw *Dib = new CDIBDraw(lpDrawItemStruct->hwndItem,
								ButWidth,ButHeight,
								g_WindowsIs555);
	Dib->ClearDIB();

	CEdgeBrush *EdgeBrush = m_pDoc->GetEdgeBrush();
	CTextureSelector *Textures = m_pDoc->GetTextureSelector();

	int x = (ButWidth-m_pDoc->GetTextureWidth()*3)/2;
	int y = (ButHeight-m_pDoc->GetTextureHeight())/2;

	EdgeBrush->DrawEdgeBrushIcon(Dib,Textures->GetImages(),x,y);

	Dib->SwitchBuffers();

//	CDC	*dc=dc->FromHandle(lpDrawItemStruct->hDC);
//	CPen Pen;
//	Pen.CreatePen( PS_SOLID, 2, RGB(255,0,255) );
//	dc->SelectObject( &Pen );
//
//  int x0 = lpDrawItemStruct->rcItem.left;
//  int y0 = lpDrawItemStruct->rcItem.top;
//  int x1 = lpDrawItemStruct->rcItem.right;
//  int y1 = lpDrawItemStruct->rcItem.bottom;
//
//	dc->MoveTo(x0,y0);
//	dc->LineTo(x1,y0);
//	dc->LineTo(x1,y1);
//	dc->LineTo(x0,y1);
//	dc->LineTo(x0,y0);
//
	delete Dib;

	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}
