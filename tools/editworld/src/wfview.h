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

#ifndef __INCLUDED_WFVIEW_H__
#define __INCLUDED_WFVIEW_H__

/////////////////////////////////////////////////////////////////////////////
// CWFView view

#include "directx.h"
#include "geometry.h"

//#include "ddview.h"
#include "ddimage.h"
#include "heightmap.h"
#include "pcxhandler.h"
//#include "brushprop.h"

#define	SCREEN_XRES	640
#define	SCREEN_YRES	480
#define	SCREEN_BPP	16

enum {
	DM_NODRAG,
	DM_DRAGOBJECT,
};

// Forward declarations for pointers
class KeyHandler;

class CWFView : public CScrollView
{
protected:
	int m_CurrentObjectID;
	BOOL m_GotFocus;
	int m_ObjectID;
	BOOL m_HeightsChanged;
	CFont m_Font;
	CWFView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWFView)

// Attributes
public:
	CBTEditDoc* GetDocument();

// Operations
public:
	void SyncPosition(D3DVECTOR &CameraPos);
	CFont *GetFont(void) { return &m_Font; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWFView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWFView();

	void InitialiseData(void);
	void DeleteData(void);
	void UpdateView(CBTEditDoc* pDoc);
	void ApplyRandomness(int Selected);
	void ApplyTool(CPoint point,BOOL JustPressed = FALSE);
	BOOL ApplyToolOnce(CPoint point);

	void PlaceObject(CPoint &point);
	void PositionSelectedObjects(CPoint &point);
	void SelectObject(CPoint &Point);
	void DeSelectObject(CPoint &Point);
//	void GetObjectExtents(CObjectDB* ObjectDB,DWORD Instance,D3DVECTOR &Position,D3DVECTOR &Smallest,D3DVECTOR &Largest);
//	void EditStats(void);

	void UpdateAndValidate(void);
	void UpdateScrollPosition(void);
	void AutoScroll(CPoint &point);

	BOOL m_HasChanged;
	BOOL m_ViewIsInitialised;
	CDirectDraw* m_DirectDrawView;
	CGeometry* m_DirectMaths;
	CHeightMap* m_HeightMap;
//	D3DVECTOR m_CameraPosition;
//	D3DVECTOR m_CameraRotation;
	PCXHandler *m_PCXBitmap;
	PCXHandler *m_PCXTexBitmap;
	LPDIRECTDRAWSURFACE m_TextureSurface;
	LPDIRECTDRAWSURFACE m_SpriteSurface;
	LPDIRECTDRAWSURFACE m_TextSurface;
	DDImage* m_SpriteImage;
	DDImage* m_TextImage;
	SLONG	m_ScrollX;
	SLONG	m_ScrollY;
	SLONG	m_MouseX;
	SLONG	m_MouseY;
	DWORD	m_DragMode;
//	CBrushProp *m_BrushDialog;

	KeyHandler *m_KeyHandler;

	// Generated message map functions
	//{{AFX_MSG(CWFView)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void On2dpopupProperties();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CWFView* WFView;

#endif // __INCLUDED_WFVIEW_H__
