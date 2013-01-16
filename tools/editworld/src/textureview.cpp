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
// TextureView.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "textureview.h"
#include "tiletypes.h"
#include "autoflagdialog.hpp"
#include "debugprint.hpp"

/////////////////////////////////////////////////////////////////////////////
// CTextureView

enum {
	ID_ELPOPUP_EDITITEM=1,
	ID_ELPOPUP_RESETITEM,
	ID_ELPOPUP_APPENDITEM,
	ID_ELPOPUP_INSERTITEM,
	ID_ELPOPUP_DELETEITEM,

	ID_ELPOPUP_MAKEINDEPENDANT,
	ID_ELPOPUP_MAKEINHERIT,
	ID_ELPOPUP_CHANGETEMPLATE,
	ID_ELPOPUP_USEDEFAULTS,
	ID_ELPOPUP_IMPORT,
	ID_ELPOPUP_EXPORT,
	ID_ELPOPUP_CENTER,
};

CTextureView *TextureView;


IMPLEMENT_DYNCREATE(CTextureView, CScrollView)


CTextureView::CTextureView()
{
	TextureView = this;
	m_ViewIsInitialised=FALSE;

//	m_EditList = NULL;
//	m_DepEditList = NULL;
//	m_SelectedEditList = NULL;
//	m_SelectedDeployment = NULL;
//	m_TableWidthsValid = FALSE;
}

CTextureView::~CTextureView()
{
	TextureView = NULL;
//	DeleteEditLists();
}

//void CTextureView::SetObjectStatsTemplate(CObjectDB *ObjectDB,int Instance)
//{
//// Delete the current template.
//	if(m_EditList) {
//// Ask the template class to dump it's defaults into the stats buffer.
//		if(!m_ObjectDB->IsInheriting(m_Instance)) {
//			m_EditList->Write(m_ObjectDB->GetStatsDataPointer(m_Instance));
//		}
//
//		delete m_EditList;
//	}
//
////	m_ObjectDB = ObjectDB;
////	m_Instance = Instance;
//
//// Create a new template using the objects template
//	m_EditList = new CEditList(ObjectDB->GetStatsTemplate(Instance));
//// and read the objects stats into it.
//	m_EditList->Read(ObjectDB->GetStatsData(Instance));
//
//	m_EditList->SetCanEdit((!ObjectDB->IsInheriting(Instance)) & ObjectDB->GetStatsEditable());
//
//	m_ObjectDB = ObjectDB;
//	m_Instance = Instance;
//}

//void CTextureView::SetDeploymentStatsTemplate(CObjectDB *ObjectDB,int Instance)
//{
//// Delete the current template.
//	if(m_DepEditList) {
//// Ask the template class to dump it's defaults into the stats buffer.
//		if(!m_ObjectDB->IsInheriting(m_Instance,TRUE)) {
//			m_DepEditList->Write(m_ObjectDB->GetStatsDataPointer(m_Instance,TRUE));
//		}
//
//		delete m_DepEditList;
//	}
//
//// Create a new template using the objects template
//	m_DepEditList = new CEditList(ObjectDB->GetStatsTemplate(Instance,TRUE));
//// and read the objects stats into it.
//	m_DepEditList->Read(ObjectDB->GetStatsData(Instance,TRUE));
//
//	m_DepEditList->SetCanEdit((!ObjectDB->IsInheriting(Instance,TRUE)) & ObjectDB->GetDeploymentStatsEditable());

// *** This is a potential problem. ***
//	m_ObjectDB = ObjectDB;
//	m_Instance = Instance;
//}


BEGIN_MESSAGE_MAP(CTextureView, CScrollView)
	//{{AFX_MSG_MAP(CTextureView)
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_VSCROLL()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
//	ON_COMMAND(ID_ELPOPUP_EDITITEM, CTextureView::OnELEditItem)
//	ON_COMMAND(ID_ELPOPUP_APPENDITEM, CTextureView::OnELAppendItem)
//	ON_COMMAND(ID_ELPOPUP_INSERTITEM, CTextureView::OnELInsertItem)
//	ON_COMMAND(ID_ELPOPUP_DELETEITEM, CTextureView::OnELDeleteItem)
//	ON_COMMAND(ID_ELPOPUP_MAKEINDEPENDANT, CTextureView::OnELMakeIndependant)
//	ON_COMMAND(ID_ELPOPUP_MAKEINHERIT, CTextureView::OnELMakeInherit)
//	ON_COMMAND(ID_ELPOPUP_CHANGETEMPLATE, CTextureView::OnELChangeTemplate)
//	ON_COMMAND(ID_ELPOPUP_USEDEFAULTS, CTextureView::OnELUseDefaults)
//	ON_COMMAND(ID_ELPOPUP_IMPORT, CTextureView::OnELImport)
//	ON_COMMAND(ID_ELPOPUP_EXPORT, CTextureView::OnELExport)
//	ON_COMMAND(ID_ELPOPUP_CENTER, CTextureView::OnELCenter)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextureView drawing

void CTextureView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	SetViewScrollSize();
}

void CTextureView::OnDraw(CDC* pDC)
{
	CBTEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	UpdateView(pDoc,pDC);
}

CBTEditDoc* CTextureView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTEditDoc)));
	return (CBTEditDoc*)m_pDocument;
}

//#define STATSINFOHEIGHT	112
//#define DEPSTATSINFOHEIGHT 16

void CTextureView::SetViewScrollSize(void)
{
	CBTEditDoc* pDoc = GetDocument();
	CDC *pDC = GetDC();

	CSize sizeTotal;
	CSize sizeTotal2;

	switch(pDoc->GetTVMode()) {
		case MTV_TEXTURES:
			sizeTotal.cx = pDoc->GetTextureViewWidth(this->m_hWnd);
			sizeTotal.cy = pDoc->GetTextureViewHeight(this->m_hWnd);
			SetScrollSizes(MM_TEXT, sizeTotal);
			m_ScrollSize = sizeTotal;
			break;
	}

	ReleaseDC(pDC);
}

void CTextureView::UpdateView(CBTEditDoc* pDoc,CDC* pDC)
{
	CPoint ScrollPos;

	SetViewScrollSize();

	switch(pDoc->GetTVMode()) {
		case MTV_TEXTURES:
			ScrollPos = GetScrollPosition();
			m_ScrollX = ScrollPos.x;
			m_ScrollY = ScrollPos.y;

			pDoc->UpdateTextureView(this->m_hWnd,this->GetParentOwner()->m_hWnd,m_ScrollX,m_ScrollY);
			m_TableWidthsValid = FALSE;
			break;
	}
}

//extern int TableWidths[256];
//
//void CTextureView::BuildTableWidths(CDC *pDC)
//{
//	if(!m_TableWidthsValid) {
//		CBTEditDoc* pDoc = GetDocument();
//		CEditList *EditList;
//		CForceList *Deployments;
//		int TmpWidths[256];
//
//		for(int i=0; i<256; i++) {
//			TableWidths[i] = 0;
//		}
//
//		Deployments = pDoc->GetDeploymentList();
//
//		for(i=0; i<Deployments->GetNumItems(); i++) {
//			EditList = Deployments->GetEditList(i);
//			if(EditList == NULL) break;
//
//			EditList->GetTableWidths(pDC,TmpWidths);
//
//			for(int j=0; j<256; j++) {
//				if(TmpWidths[j] > TableWidths[j]) {
//					TableWidths[j] = TmpWidths[j];
//				}
//			}
//		}
//		m_TableWidthsValid = TRUE;
//	}
//}

/////////////////////////////////////////////////////////////////////////////
// CTextureView message handlers

void CTextureView::OnDestroy() 
{
	CScrollView::OnDestroy();
}


// Overide OnEraseBkgnd() to clear area outside view but not bits
// were going to repaint.
//
BOOL CTextureView::OnEraseBkgnd(CDC* pDC) 
{
	CBTEditDoc* pDoc = GetDocument();
	CBrush br(RGB(0,0,0));

	switch(pDoc->GetTVMode()) {
		case MTV_TEXTURES:
			FillOutsideRect( pDC, &br );

			return TRUE;	// Erased
		default:
			return CWnd::OnEraseBkgnd(pDC);
	}
}

void CTextureView::OnMouseMove(UINT nFlags, CPoint point) 
{
//·	MK_CONTROL   Set if the CTRL key is down.
//·	MK_LBUTTON   Set if the left mouse button is down.
//·	MK_MBUTTON   Set if the middle mouse button is down.
//·	MK_RBUTTON   Set if the right mouse button is down.
//·	MK_SHIFT   Set if the SHIFT

//	TRACE3("MM %d %d %d\n",point.x,point.y,nFlags);

	CScrollView::OnMouseMove(nFlags, point);
}

void CTextureView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CBTEditDoc* pDoc = GetDocument();
	int SelX = point.x + m_ScrollX;
	int SelY = point.y + m_ScrollY;
	CTextureSelector *Selector = pDoc->GetTextureSelector();
	int Selected;

	switch(pDoc->GetTVMode()) {
		case MTV_TEXTURES:
			if( pDoc->SelectTexture(SelX,SelY) ) {
				if(pDoc->m_SelIsTexture) {
					DWORD EditTool = pDoc->GetEditTool();
					if((EditTool >= ET_TYPESFIRST) && (EditTool <= ET_TYPESLAST)) {
						Selector->SetTextureType(SelX,SelY,pDoc->GetSelectedType());
						pDoc->ReInitEdgeBrushes();
					}

					Selected = ((int)pDoc->m_SelTexture)-1;
					if(Selected >= 0) {

//						switch(EditTool) {
//							case ET_ROTATETILE:
//								Rotate = Selector->GetTextureRotate(Selected);
//								Rotate++;
//								Rotate &= 3;
//								Selector->SetTextureRotate(Selected,Rotate);
//								break;
//
//							case ET_XFLIPTILE:
//								FlipX = Selector->GetTextureFlipX(Selected);
//								FlipY = Selector->GetTextureFlipY(Selected);
//								Rotate = Selector->GetTextureRotate(Selected);
//								if(Rotate & 1) {
//									Selector->SetTextureFlip(Selected,FlipX,!FlipY);
//								} else {
//									Selector->SetTextureFlip(Selected,!FlipX,FlipY);
//								}
//								break;
//
//							case ET_YFLIPTILE:
//								FlipX = Selector->GetTextureFlipX(Selected);
//								FlipY = Selector->GetTextureFlipY(Selected);
//								Rotate = Selector->GetTextureRotate(Selected);
//								if(Rotate & 1) {
//									Selector->SetTextureFlip(Selected,!FlipX,FlipY);
//								} else {
//									Selector->SetTextureFlip(Selected,FlipX,!FlipY);
//								}
//								break;
//						}
					}
				}
			}

			// Do it again incase the type or flags changed.
			pDoc->SelectTexture(SelX,SelY);
			InvalidateRect(NULL,NULL);
			break;
	}

	CScrollView::OnLButtonDown(nFlags, point);
}

void CTextureView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
}

void CTextureView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
//	CBTEditDoc* pDoc = GetDocument();
//	CMenu Menu;
//
//	RECT Rect;
//	RECT Rect2;
//	GetWindowRect(&Rect);
//	int HScroll = this->GetScrollPos(SB_HORZ);
//	int VScroll = this->GetScrollPos(SB_VERT);
//	int Start,End;
//	int i;
//	CEditList *EditList;
//	CForceList *Deployments;
//
//	point.x -= Rect.left;
//	point.y -= Rect.top;
//
//	switch(pDoc->GetTVMode()) {
//		case MTV_STATS:
//			if(m_EditList) {
//				m_EditList->DeSelectAll();
//				if(m_EditList->Select(point.x,point.y,HScroll,VScroll)) {
//		// Do context menu for object stats with selection.
//					InvalidateRect(NULL,TRUE);
//
//					Menu.CreatePopupMenu();
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_EDITITEM,"Edit value");
//					Menu.AppendMenu(MF_SEPARATOR);
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_APPENDITEM,"Append value");
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_INSERTITEM,"Insert value");
//					Menu.AppendMenu(MF_SEPARATOR);
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_DELETEITEM,"Delete value");
//
////					m_IsDeployment = FALSE;
//					m_SelectedEditList = m_EditList;
//					Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x+Rect.left,point.y+Rect.top,this);
//					return;
//				}
//			}
//
//			if((point.y+VScroll > m_StatsInfoY) && (point.y+VScroll < m_StatsY)) {
//		// Do context menu for object stats with no selection.
//				DebugPrint("DefObj\n");
//				Menu.CreatePopupMenu();
//				if(m_ObjectDB->IsInheriting(m_Instance)) {
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_MAKEINDEPENDANT,"Make independant");
//				} else {
//					if(m_Instance >= 0) {
//						Menu.AppendMenu(MF_STRING,ID_ELPOPUP_MAKEINHERIT,"Inherit from parent");
//						Menu.AppendMenu(MF_SEPARATOR);
//					}
//					if(m_ObjectDB->GetCanChangeTemplate()) {
//						Menu.AppendMenu(MF_STRING,ID_ELPOPUP_CHANGETEMPLATE,"Change template");
//						Menu.AppendMenu(MF_SEPARATOR);
//					}
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_USEDEFAULTS,"Use default values");
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_IMPORT,"Import values");
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_EXPORT,"Export values");
//				}
//
//		//		if(m_Instance >= 0) {
//		//			Menu.AppendMenu(MF_SEPARATOR);
//		//			Menu.AppendMenu(MF_STRING,ID_ELPOPUP_CENTER,"Center on object");
//		//		}
//
////				m_IsDeployment = FALSE;
//				m_SelectedEditList = m_EditList;
//				Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x+Rect.left,point.y+Rect.top,this);
//				return;
//			}
//
//			m_SelectedEditList = NULL;
//			break;
//
//		case MTV_DEPLOYMENTS:
//			Deployments = pDoc->GetDeploymentList();
//
//			GetClientRect(&Rect2);
//			Start = VScroll/16;
//			End = Start + Rect2.bottom/16;
//
//			if(End > Deployments->GetNumItems()) {
//				End = Deployments->GetNumItems();
//			}
//
//			for(i=0; i<Deployments->GetNumItems(); i++) {
//				EditList = Deployments->GetEditList(i);
//				EditList->DeSelectAll();
//			}
//
//			for(i=Start; i<End; i++) {
//				EditList = Deployments->GetEditList(i);
//				if( EditList->Select(point.x,point.y,HScroll,VScroll) ) {
//		// Do context menu for object stats with selection.
//					InvalidateRect(NULL,TRUE);
//
//					Menu.CreatePopupMenu();
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_EDITITEM,"Edit value");
//					Menu.AppendMenu(MF_SEPARATOR);
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_APPENDITEM,"Append value");
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_INSERTITEM,"Insert value");
//					Menu.AppendMenu(MF_SEPARATOR);
//					Menu.AppendMenu(MF_STRING,ID_ELPOPUP_DELETEITEM,"Delete value");
//
//					m_SelectedDeployment = EditList;
//					Menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x+Rect.left,point.y+Rect.top,this);
//					return;
//				}
//			}
//			m_SelectedDeployment = NULL;
//			InvalidateRect(NULL,TRUE);
//			break;
//	}
}

//void CTextureView::OnELEditItem(void)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	if(pDoc->GetTVMode() == MTV_STATS) {
//		m_SelectedEditList->EditSelectedValue(this);
//		InvalidateRect(NULL,TRUE);
//
//	// Always write values back to object database when they've been edited so
//	// that the database is always up to date.
//
//		if(m_EditList) {
//	// Ask the template class to dump it's defaults into the stats buffer.
//			if(!m_ObjectDB->IsInheriting(m_Instance)) {
//				m_EditList->Write(m_ObjectDB->GetStatsDataPointer(m_Instance));
//			}
//		}
//
////		if(m_DepEditList) {
////	// Ask the template class to dump it's defaults into the deployment stats buffer.
////			if(!m_ObjectDB->IsInheriting(m_Instance,TRUE)) {
////				m_DepEditList->Write(m_ObjectDB->GetStatsDataPointer(m_Instance,TRUE));
////			}
////		}
//	} else {
//		m_SelectedDeployment->EditSelectedValue(this);
//		m_TableWidthsValid = FALSE;
//		g_SortDeployments=TRUE;
//		InvalidateRect(NULL,TRUE);
//	}
//}

//void CTextureView::OnELAppendItem(void)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	if(pDoc->GetTVMode() == MTV_STATS) {
//		m_SelectedEditList->AppendSelectedValue(this);
//		InvalidateRect(NULL,TRUE);
//	} else {
//		m_SelectedDeployment->AppendSelectedValue(this);
//		m_TableWidthsValid = FALSE;
//		g_SortDeployments=TRUE;
//		InvalidateRect(NULL,TRUE);
//	}
//}

//void CTextureView::OnELInsertItem(void)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	if(pDoc->GetTVMode() == MTV_STATS) {
//		m_SelectedEditList->InsertSelectedValue(this);
//		InvalidateRect(NULL,TRUE);
//	} else {
//		m_SelectedDeployment->InsertSelectedValue(this);
//		m_TableWidthsValid = FALSE;
//		g_SortDeployments=TRUE;
//		InvalidateRect(NULL,TRUE);
//	}
//}

//void CTextureView::OnELDeleteItem(void)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	if(pDoc->GetTVMode() == MTV_STATS) {
//		m_SelectedEditList->DeleteSelectedValue(this);
//		InvalidateRect(NULL,TRUE);
//	} else {
//		m_SelectedDeployment->DeleteSelectedValue(this);
//		m_TableWidthsValid = FALSE;
//		g_SortDeployments=TRUE;
//		InvalidateRect(NULL,TRUE);
//	}
//}
//
//void CTextureView::OnELMakeIndependant(void)
//{
////	if(m_IsDeployment) {
////		m_ObjectDB->SetIsInheriting(m_Instance,FALSE,m_IsDeployment);
////		m_DepEditList->SetCanEdit(TRUE & m_ObjectDB->GetDeploymentStatsEditable());
////		m_ObjectDB->SetDeploymentStatsTemplate(m_ObjectDB->GetStatsTemplate(-1,m_IsDeployment),m_Instance);
////		InvalidateRect(NULL,TRUE);
////	} else {
//		m_ObjectDB->SetIsInheriting(m_Instance,FALSE);
//		m_EditList->SetCanEdit(TRUE & m_ObjectDB->GetStatsEditable());
//		m_ObjectDB->SetStatsTemplate(m_ObjectDB->GetStatsTemplate(),m_Instance);
//		InvalidateRect(NULL,TRUE);
////	}
//}
//
//void CTextureView::OnELMakeInherit(void)
//{
////	if(m_IsDeployment) {
////		m_ObjectDB->SetIsInheriting(m_Instance,TRUE,m_IsDeployment);
////		m_DepEditList->SetCanEdit(FALSE);
////		SetDeploymentStatsTemplate(m_ObjectDB,m_Instance);
////		InvalidateRect(NULL,TRUE);
////	} else {
//		m_ObjectDB->SetIsInheriting(m_Instance,TRUE);
//		m_EditList->SetCanEdit(FALSE);
//		SetObjectStatsTemplate(m_ObjectDB,m_Instance);
//		InvalidateRect(NULL,TRUE);
////	}
//}
//
//const char TemplateFilters[]="Stat Template Files (*.stt)|*.stt|All Files (*.*)|*.*||";
//const char StatsFilters[]="Stat Files (*.std)|*.std|All Files (*.*)|*.*||";
//
//void CTextureView::OnELChangeTemplate(void)
//{
//	CFileDialog FileDlg(TRUE,"stt","*.stt",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,TemplateFilters);
//	if(FileDlg.DoModal() == IDOK) {
////		if(m_IsDeployment) {
////			m_ObjectDB->SetDeploymentStatsTemplate(FileDlg.GetFileName().GetBuffer(0),m_Instance);
////
////// Delete the current template.
////			if(m_DepEditList) {
////				delete m_DepEditList;
////			}
////// Create a new template using the objects template
////			m_DepEditList = new CEditList(m_ObjectDB->GetStatsTemplate(m_Instance,TRUE));
////// and read the default stats into it.
////			m_DepEditList->Read(m_ObjectDB->GetStatsData(m_Instance,TRUE));
////			InvalidateRect(NULL,TRUE);
////		} else {
//			m_ObjectDB->SetStatsTemplate(FileDlg.GetFileName().GetBuffer(0),m_Instance);
//
//// Delete the current template.
//			if(m_EditList) {
//				delete m_EditList;
//			}
//// Create a new template using the objects template
//			m_EditList = new CEditList(m_ObjectDB->GetStatsTemplate(m_Instance));
//// and read the default stats into it.
//			m_EditList->Read(m_ObjectDB->GetStatsData(m_Instance));
//			InvalidateRect(NULL,TRUE);
////		}
//	}
//}
//
//void CTextureView::OnELUseDefaults(void)
//{
//// Read the default stats.
////	m_SelectedEditList->Read(m_ObjectDB->GetStatsData(m_Instance,m_IsDeployment));
//	m_SelectedEditList->Read(m_ObjectDB->GetStatsData(m_Instance));
//	InvalidateRect(NULL,TRUE);
//}
//
//void CTextureView::OnELImport(void)
//{
//	CFileDialog FileDlg(TRUE,"std","*.std",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,StatsFilters);
//	if(FileDlg.DoModal() == IDOK) {
//		FILE *Stream = fopen(FileDlg.GetPathName().GetBuffer(0),"rb");
//		if(Stream!=NULL) {
//			m_SelectedEditList->Read(Stream);
//			fclose(Stream);
//			InvalidateRect(NULL,TRUE);
//		} else {
//			MessageBox(FileDlg.GetPathName().GetBuffer(0), "Error opening file for read.", MB_OK );
//		}
//	}
//}
//
//void CTextureView::OnELExport(void)
//{
//	CFileDialog FileDlg(FALSE,"std","*.std",OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,StatsFilters);
//	if(FileDlg.DoModal() == IDOK) {
//		FILE *Stream = fopen(FileDlg.GetPathName().GetBuffer(0),"wb");
//		if(Stream!=NULL) {
//			m_SelectedEditList->Write(Stream);
//			fclose(Stream);
//		} else {
//			MessageBox(FileDlg.GetPathName().GetBuffer(0), "Error opening file for write.", MB_OK );
//		}
//	}
//}
//
//void CTextureView::OnELCenter(void)
//{
//}
//
//
//void CTextureView::DeleteEditLists(void)
//{
//	if(m_EditList) {
//		delete m_EditList;
//		m_EditList = NULL;
//	}
////	if(m_DepEditList) {
////		delete m_DepEditList;
////		m_DepEditList = NULL;
////	}
//}
//
void CTextureView::PurgeView(void)
{
//	DeleteEditLists();

	CBTEditDoc* pDoc = GetDocument();

	if(pDoc->GetTVMode() != MTV_TEXTURES) {
		InvalidateRect(NULL,TRUE);
	}
//	switch(pDoc->GetTVMode()) {
//		case MTV_TEXTURES:
//			InvalidateRect(NULL,TRUE);
//			break;
//	}
}

void CTextureView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
}


int CTextureView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_Font.CreateFont(10,0,0,0,FW_NORMAL,0,0,0,
				DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif");
	SetFont( &m_Font,FALSE );
	
	return 0;
}


void CTextureView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int SelX = point.x + m_ScrollX;
	int SelY = point.y + m_ScrollY;
	CBTEditDoc* pDoc = GetDocument();
	CTextureSelector *Selector = pDoc->GetTextureSelector();
	int Selected;

	switch(pDoc->GetTVMode()) {
		case MTV_TEXTURES:
			if( pDoc->SelectTexture(SelX,SelY) ) {
				if(pDoc->m_SelIsTexture) {
					Selected = ((int)pDoc->m_SelTexture)-1;

					DebugPrint("Double clicked on tile %d\n",Selected);

					AutoFlagDialog Dlg(NULL,
					                   Selector->GetTextureFlags(Selected) & SF_RANDTEXTUREROTATE,
					                   Selector->GetTextureFlags(Selected) & SF_RANDTEXTUREFLIPX,
					                   Selector->GetTextureFlags(Selected) & SF_RANDTEXTUREFLIPY,
					                   Selector->GetTextureFlags(Selected) & SF_TEXTUREFLIPX,
					                   Selector->GetTextureFlags(Selected) & SF_TEXTUREFLIPY,
					                   Selector->GetTextureRotate(Selected),
					                   Selector->GetTextureFlags(Selected) & SF_INCTEXTUREROTATE,
					                   Selector->GetTextureFlags(Selected) & SF_TOGTEXTUREFLIPX,
					                   Selector->GetTextureFlags(Selected) & SF_TOGTEXTUREFLIPY);

					if(Dlg.DoModal() == IDOK)
					{
						Selector->SetTextureRotate(Selected, Dlg.Rotate());
						Selector->SetTextureFlip(Selected, Dlg.XFlip(), Dlg.YFlip());
						Selector->SetTextureRandomFlip(Selected, Dlg.RandXFlip(), Dlg.RandYFlip());
						Selector->SetTextureToggleFlip(Selected, Dlg.ToggleXFlip(), Dlg.ToggleYFlip());
						Selector->SetTextureRandomRotate(Selected, Dlg.RandRotate());
						Selector->SetTextureIncrementRotate(Selected, Dlg.IncRotate());
						pDoc->SelectTexture(SelX,SelY);
						InvalidateRect(NULL,NULL);
					}
				}
			}
			break;
	}
	
	CScrollView::OnLButtonDblClk(nFlags, point);
}

void CTextureView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);
}
