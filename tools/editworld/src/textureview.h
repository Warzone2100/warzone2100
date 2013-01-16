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

#ifndef __INCLUDED_TEXTUREVIEW_H__
#define __INCLUDED_TEXTUREVIEW_H__

/////////////////////////////////////////////////////////////////////////////
// CTextureView view


#include "directx.h"
#include "geometry.h"

//#include "DDView.h"
#include "ddimage.h"
#include "heightmap.h"
#include "pcxhandler.h"
#include "bmphandler.h"
//#include "editlist.h"
//#include "textsel.h"


class CTextureView : public CScrollView
{
protected:
	int m_TextDY;
	void BuildTableWidths(CDC *pDC);
//	CEditList* m_SelectedDeployment;

//	BOOL m_IsDeployment;
//	int m_DepStatsInfoY;
	int m_StatsInfoY;
	int m_StatsY;
//	int m_DepStatsY;
	int m_Instance;
	int m_ScrollY;
	int m_ScrollX;
	BOOL m_TableWidthsValid;
	CFont m_Font;
//	CObjectDB *m_ObjectDB;
	CSize m_ScrollSize;
	CTextureView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTextureView)

//	CEditList *m_EditList;
//	CEditList *m_DepEditList;
//	CEditList *m_SelectedEditList;

// Attributes
public:
	void PurgeView(void);
	CBTEditDoc* GetDocument();

// Operations
public:
	void SetViewScrollSize(void);
	void GetViewScrollSize(CSize &Size) { Size = m_ScrollSize; }
//	void SetObjectStatsTemplate(CObjectDB *ObjectDB,int Instance=-1);
//	void SetDeploymentStatsTemplate(CObjectDB *ObjectDB,int Instance=-1);
//	void DeleteEditLists(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextureView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTextureView();
	void UpdateView(CBTEditDoc* pDoc,CDC* pDC);
	void OnELEditItem(void);
	void OnELAppendItem(void);
	void OnELInsertItem(void);
	void OnELDeleteItem(void);
	void OnELMakeIndependant(void);
	void OnELMakeInherit(void);
	void OnELChangeTemplate(void);
	void OnELUseDefaults(void);
	void OnELImport(void);
	void OnELExport(void);
	void OnELCenter(void);

	BOOL	m_ViewIsInitialised;

	// Generated message map functions
	//{{AFX_MSG(CTextureView)
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CTextureView *TextureView;

#endif // __INCLUDED_TEXTUREVIEW_H__
