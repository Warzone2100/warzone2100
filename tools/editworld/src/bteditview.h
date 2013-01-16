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

#ifndef __INCLUDED_BTEDITVIEW_H__
#define __INCLUDED_BTEDITVIEW_H__

// BTEditView.h : interface of the CBTEditView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "directx.h"
#include "geometry.h"

//#include "DDView.h"
#include "ddimage.h"
#include "heightmap.h"
#include "pcxhandler.h"

//#include "InfoDialog.h"

#define	SCREEN_XRES	640
#define	SCREEN_YRES	480
#define	SCREEN_BPP	16

enum {
	DM3D_NODRAG,
	DM3D_DRAGOBJECT,
};

// Forward declarations for pointers
class CBTEditDoc;
class KeyHandler;

class CBTEditView : public CScrollView
{
protected: // create from serialization only
	int m_CurrentObjectID;
	SLONG m_Selected;
	BOOL m_HeightsChanged;
	float m_CurrentHeight;
	DWORD m_SelVerticies;

//	CInfoDialog *m_InfoDialog;

	CBTEditView();
	DECLARE_DYNCREATE(CBTEditView)

// Attributes
public:
	CBTEditDoc* GetDocument();
	void SyncPosition(void);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBTEditView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBTEditView();

protected:
	void InitialiseData(void);
	void DeleteData(void);
	void UpdateView(CBTEditDoc* pDoc);
	BOOL ApplyToolOnce(CPoint point);
	void ApplyRandomness(int Selected);
	void ApplyTool(CPoint point,int XVel,int YVel,BOOL JustPressed = FALSE);
	void PositionSelectedObjects(CPoint point,int XVel,int YVel);
	void RotateSelectedObjects(CPoint point,int XVel,int YVel);

	void SelectObject(CPoint &point);
	void DeSelectObject(CPoint &point);
	void GetObjectHeight(CPoint &point);
//	CObjectDB* GetObjectVertex(CPoint &point,DWORD *Instance,D3DVECTOR *Vector);
	void EditStats(void);

	BOOL	m_HasChanged;
	BOOL	m_Active;
	BOOL	m_Redraw;
	BOOL	m_ViewIsInitialised;
//	CDirectDraw* m_DirectDrawView;
//	CGeometry* m_DirectMaths;
//	CHeightMap* m_HeightMap;
//	D3DVECTOR m_CameraPosition;
//	D3DVECTOR m_CameraRotation;
	PCXHandler *m_PCXBitmap;
	PCXHandler *m_PCXTexBitmap;
//	LPDIRECTDRAWSURFACE m_TextureSurface;
//	LPDIRECTDRAWSURFACE m_SpriteSurface;
//	LPDIRECTDRAWSURFACE m_TextSurface;
//	DDImage* m_SpriteImage;
//	DDImage* m_TextImage;
	DWORD m_MouseX;
	DWORD m_MouseY;
	DWORD m_DragMode;
	KeyHandler *m_KeyHandler;
// Generated message map functions
protected:
	//{{AFX_MSG(CBTEditView)
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void On3dpopupProperties();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CBTEditView *BTEditView;

#endif // __INCLUDED_BTEDITVIEW_H__
