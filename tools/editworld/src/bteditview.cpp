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
// BTEditView.cpp : implementation of the CBTEditView class
//

#include "stdafx.h"
#include "mainframe.h"
#include "btedit.h"

#include "bteditdoc.h"
#include "bteditview.h"
//#include "editstats.h"
#include "debugprint.hpp"
//#include "statsview.h"
#include "tiletypes.h"
#include "objectproperties.h"
#include "keyhandler.hpp"
#include "undoredo.h"

#define KEY_SEAUP			'U'
#define KEY_SEADOWN			'J'
#define KEY_ORBITLEFT		188	//','
#define KEY_ORBITRIGHT		190	//'.'
#define KEY_ROTATETILE		'R'
#define KEY_ROTATETILE2		'E'
#define KEY_XFLIPTILE		'X'
#define KEY_YFLIPTILE		'Y'
#define KEY_TRIFLIP			'V'
#define KEY_DECVIEWDIST		'Q'
#define KEY_INCVIEWDIST		'W'
#define KEY_SYNCPOSITION	'S'
#define KEY_YAWLIGHT		'L'
#define KEY_PITCHLIGHT		'K'
#define KEY_CAMERAUP		VK_PRIOR
#define KEY_CAMERADOWN		VK_NEXT
#define KEY_CAMERAROTLEFT	VK_HOME
#define KEY_CAMERAROTRIGHT	VK_END
#define KEY_CAMERAPITCHDOWN	VK_ADD
#define KEY_CAMERAPITCHUP	VK_SUBTRACT


#define ORBITRADIUS	(128*8)
#define ORBITSTEP	(2)

extern HCURSOR g_Pointer;
extern D3DVECTOR g_WorldRotation;
extern CUndoRedo *g_UndoRedo;

CBTEditView *BTEditView = NULL;

D3DVECTOR Center;
BOOL CenterValid = FALSE;
float OrbitAngle = 0.0F;
int SelectedObject = -1;

int Copy3dRectCorners=0;
int Copy3dRect0,Copy3dRect1;

int Mark3dRectCorners=0;
int Mark3dRect0,Mark3dRect1;

/////////////////////////////////////////////////////////////////////////////
// CBTEditView

IMPLEMENT_DYNCREATE(CBTEditView, CScrollView)

BEGIN_MESSAGE_MAP(CBTEditView, CScrollView)
	//{{AFX_MSG_MAP(CBTEditView)
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_SETCURSOR()
	ON_WM_CONTEXTMENU()
	ON_WM_KEYUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_3DPOPUP_PROPERTIES, On3dpopupProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBTEditView construction/destruction

CBTEditView::CBTEditView()
{
	BTEditView = this;
	m_ViewIsInitialised=FALSE;
//	m_Redraw = TRUE;
	m_HasChanged = FALSE;
	m_CurrentHeight = 0.0F;
	m_SelVerticies = 0;
	m_Selected = -1;
	m_DragMode = DM3D_NODRAG;
//	m_DirectMaths = NULL;
	m_KeyHandler = new KeyHandler();
	m_HeightsChanged = FALSE;

//	m_InfoDialog = new CInfoDialog(this);
}

CBTEditView::~CBTEditView()
{
	BTEditView = NULL;
	if(m_ViewIsInitialised) {
		DeleteData();
	}

	delete m_KeyHandler;

//	delete m_InfoDialog;
}

BOOL CBTEditView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CBTEditView drawing

void CBTEditView::OnDraw(CDC* pDC)
{
	CBTEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	UpdateView(pDoc);
}

void CBTEditView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CSize sizeTotal;
	// TODO: calculate the total size of this view
	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);


//	if(m_InfoDialog->GetSafeHwnd() == 0) {
//		m_InfoDialog->Create();
//	}
	
	DeleteData();

	InitialiseData();
}

CBTEditDoc* CBTEditView::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTEditDoc)));

	return (CBTEditDoc*)m_pDocument;
}


void CBTEditView::InitialiseData(void)
{
	m_ViewIsInitialised=TRUE;
}


void CBTEditView::DeleteData(void)
{
}

void CBTEditView::UpdateView(CBTEditDoc* pDoc)
{
	pDoc->Register3DWindow(this->m_hWnd);
	pDoc->Update3DView();
}

/////////////////////////////////////////////////////////////////////////////
// CBTEditView message handlers

void CBTEditView::OnDestroy() 
{
	CScrollView::OnDestroy();
	
	// TODO: Add your message handler code here

	DeleteData();	
}

BOOL	IsCaptured = FALSE;
BOOL	ShiftDown = FALSE;
D3DVECTOR LightDirection = {0.0F,0.0F,0.0F};

void CBTEditView::OnMouseMove(UINT nFlags, CPoint point) 
{
	static BOOL OldPosValid = FALSE;
	static CPoint OldPos(0,0);
	int XVel=0;
	int YVel=0;
	RECT FrameRect;
	D3DVECTOR CameraPosition,CameraRotation;
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();
	BOOL UpdateStatus = TRUE;

	pDoc->GetCamera(CameraRotation,CameraPosition);

	m_MouseX = point.x;
	m_MouseY = point.y;

	if(OldPosValid) {
		XVel = point.x - OldPos.x;
		YVel = point.y - OldPos.y;
	}
	OldPos = point;
	OldPosValid = TRUE;

	if(IsCaptured) {
		BOOL ChangedPos = FALSE;

		GetWindowRect(&FrameRect);

		int NewCX = point.x+FrameRect.left;
		int NewCY = point.y+FrameRect.top;

		if(point.x+FrameRect.left > FrameRect.right) {
			NewCX = FrameRect.left;
			ChangedPos = TRUE;
		} else {
			if(point.x+FrameRect.left < FrameRect.left) {
				NewCX = FrameRect.right;
				ChangedPos = TRUE;
			}
		}

		if(point.y+FrameRect.top > FrameRect.bottom) {
			NewCY = FrameRect.top;
			ChangedPos = TRUE;
		} else {
			if(point.y+FrameRect.top < FrameRect.top) {
				NewCY = FrameRect.bottom;
				ChangedPos = TRUE;
			}
		}

		if(ChangedPos) {
			OldPosValid = FALSE;
			SetCursorPos(NewCX,NewCY);
		}
	}


//	pDoc->GetCamera(CameraRotation,CameraPosition);

	if(pDoc->ProcessButtonLapse()) {
		if( (nFlags & (MK_LBUTTON | MK_SHIFT)) == (MK_LBUTTON | MK_SHIFT) ) {
			UpdateStatus = FALSE;
	#if(1)
	// Rotate view.
			if(pDoc->GetCameraMode() == MCAM_FREE) {
				CameraRotation.x+=YVel;
			}
			CameraRotation.y+=XVel;
	#else
	// Rotate light source.
			D3DVECTOR LightRotation;
			pDoc->GetLightRotation(LightRotation);
			LightRotation.x -= YVel;
			LightRotation.y += XVel;
			pDoc->SetLightRotation(LightRotation);
	#endif

			CenterValid = FALSE;
			pDoc->RegisterCamera(CameraRotation,CameraPosition);
			InvalidateRect(NULL,NULL);
		} else if( (nFlags & (MK_RBUTTON | MK_SHIFT)) == (MK_RBUTTON | MK_SHIFT) ) {
			UpdateStatus = FALSE;
	// Fly.
			if(pDoc->GetCameraMode() == MCAM_FREE) {
				pDoc->GetDirectMaths()->Motion(CameraPosition,CameraRotation,(float)-YVel);
				CenterValid = FALSE;
				pDoc->RegisterCamera(CameraRotation,CameraPosition);
				InvalidateRect(NULL,NULL);
			} else {
				D3DVECTOR CameraRot = {0.0F,0.0F,0.0F};
				CameraRot.y = CameraRotation.y;
				pDoc->GetDirectMaths()->Motion(CameraPosition,CameraRot,(float)-YVel);
				CameraPosition.y = pDoc->GetHeightAt(CameraPosition)+CAM_HEIGHT;
				CenterValid = FALSE;
				pDoc->RegisterCamera(CameraRotation,CameraPosition);
				InvalidateRect(NULL,NULL);
			}
		} else if( nFlags & MK_LBUTTON ) {

			// Process any drag commands.

			if(m_DragMode == DM3D_DRAGOBJECT) {
				if((m_MouseX > pDoc->GetMapWidth()) ||
					(m_MouseY > pDoc->GetMapHeight())) {
					switch(pDoc->GetEditTool()) {
						case ET_MOVE:
							if(SelectedObject >= 0) {
								UpdateStatus = FALSE;
								HeightMap->Select3DObject(SelectedObject);

								if( (!HeightMap->GetObjectInstanceSnap(SelectedObject)) || 
									HeightMap->m_NoObjectSnap) {
									D3DVECTOR Position;
									HeightMap->Get3DObjectPosition(SelectedObject,Position);
									VECTOR2D Vel,TVel;
									Vel.x = (float)XVel;
									Vel.y = (float)-YVel;
									pDoc->GetDirectMaths()->RotateVector2D(&Vel,&TVel,NULL,CameraRotation.y,1);
									Position.x += TVel.x;
									Position.z += TVel.y;
									HeightMap->Set3DObjectPosition(SelectedObject,Position);

									InvalidateRect(NULL,NULL);
									pDoc->Invalidate2D();
								} else {
									int Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
									if(Selected >= 0) {
										CTile *Tile = HeightMap->GetTile(Selected);

										HeightMap->Set3DObjectPosition(SelectedObject,Tile->Position);
										HeightMap->SnapObject(SelectedObject);

										InvalidateRect(NULL,NULL);
										pDoc->Invalidate2D();
									}
								}
							}
							break;
						case ET_ROTATEOBJECT:
							if(SelectedObject >= 0) {
								D3DVECTOR Rotation;

								UpdateStatus = FALSE;
								HeightMap->Select3DObject(SelectedObject);

								HeightMap->Get3DObjectRotation(SelectedObject,Rotation);
								Rotation.y += XVel;
								if(Rotation.y >= 360.0F) {
									Rotation.y -= 360.0F;
								}
								if(Rotation.y < 0.0F) {
									Rotation.y += 360.0F;
								}
								HeightMap->Set3DObjectRotation(SelectedObject,Rotation);

								InvalidateRect(NULL,NULL);
								pDoc->Invalidate2D();
							}
							break;
					}
				} else {
					UpdateStatus = FALSE;
					ApplyTool(point,0,0);
				}
			} else {
				UpdateStatus = FALSE;
				// Paint textures and other things.
				ApplyTool(point,XVel,YVel);
			}
		} else {
			if( (pDoc->GetEditTool() == ET_PASTERECT) && pDoc->ClipboardIsValid() ) {
				int Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
				if(Selected >= 0) {
					HeightMap->SetSelectionBox(Selected,pDoc->m_TileBufferWidth-1,pDoc->m_TileBufferHeight-1);
					HeightMap->ClipSelectionBox();
					pDoc->Update3DView();
				}
			}
		}
	}

	if(UpdateStatus) {
		// Update the status bar.
		int Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
		if(Selected >= 0) {
			g_CursorTileX = Selected % pDoc->GetMapWidth();
			g_CursorTileZ = Selected / pDoc->GetMapWidth();
		} else {
			g_CursorTileX = -1;
			g_CursorTileZ = -1;
		}
		pDoc->UpdateStatusBar();
	}
	
	CScrollView::OnMouseMove(nFlags, point);
}


void CBTEditView::OnRButtonDown(UINT nFlags, CPoint point) 
{
// If button down and shift key pressed then this view captures all
// mouse input.
	if((nFlags & MK_SHIFT) && (!IsCaptured)) {
		IsCaptured = TRUE;
		SetCapture();
	} else {
//		CBTEditDoc* pDoc = GetDocument();
//
//		pDoc->DeSelectAllObjects();
//		SelectObject(point);
	}

	CScrollView::OnRButtonDown(nFlags, point);
}

void CBTEditView::OnRButtonUp(UINT nFlags, CPoint point) 
{
// If button up and mouse input was captured by this view then release
// capture.
	if(IsCaptured) {
		ReleaseCapture();
		IsCaptured = FALSE;
	}

	CScrollView::OnRButtonUp(nFlags, point);
}

void CBTEditView::OnLButtonDown(UINT nFlags, CPoint point) 
{
//	CBTEditDoc* pDoc = GetDocument();
//	D3DVECTOR CameraPosition,CameraRotation;
//	pDoc->GetCamera(CameraRotation,CameraPosition);
//	pDoc->GetHeightMap()->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);

	D3DVECTOR CameraPosition,CameraRotation;
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();
	BOOL Applied;

// If button down and shift key pressed then this view captures all
// mouse input.
	if(pDoc->ProcessButtonLapse()) {
		if((nFlags & MK_SHIFT) && (!IsCaptured)) {
			IsCaptured = TRUE;
			SetCapture();
		} else {
	// Otherwise were painting textures or something.
			if(!(Applied=ApplyToolOnce(point))) {
				ApplyTool(point,0,0,TRUE);
			}

			if(!Applied) {
				switch(pDoc->GetEditTool()) {
		//			case ET_SELECTOBJECT:
		//				SelectObject(point);
		//				break;
		//
		//			case ET_DESELECTOBJECT:
		//				DeSelectObject(point);
		//				break;
		//
		//			case ET_GETOBJECTHEIGHT:
		//				GetObjectHeight(point);
		//				break;
		//
		//			case ET_DRAGOBJECT:
		//				if(m_DragMode!=DM3D_DRAGOBJECT) {
		//					pDoc->DeSelectAllObjects();
		//					SelectObject(point);
		//					m_DragMode = DM3D_DRAGOBJECT;
		//					IsCaptured = TRUE;
		//					SetCapture();
		////					DebugPrint("StartDrag\n");
		//				}
		//

					case ET_MOVE:
						if(m_DragMode!=DM3D_DRAGOBJECT) {
							HeightMap->DeSelectAll3DObjects();
							pDoc->GetCamera(CameraRotation,CameraPosition);
							SelectedObject = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);
							if(SelectedObject >= 0) {
								HeightMap->Select3DObject(SelectedObject);
								HeightMap->SetObjectTileFlags(TF_SHOW);
								m_DragMode = DM3D_DRAGOBJECT;
								IsCaptured = TRUE;
								SetCapture();
								DebugPrint("StartDrag\n");
							}
						}
					break;

					case ET_ROTATEOBJECT:
						if(m_DragMode!=DM3D_DRAGOBJECT) {
							HeightMap->DeSelectAll3DObjects();
							pDoc->GetCamera(CameraRotation,CameraPosition);
							SelectedObject = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);
							if(SelectedObject >= 0) {
								if(!HeightMap->GetObjectInstanceFlanged(SelectedObject)) {
									HeightMap->Select3DObject(SelectedObject);
									HeightMap->SetObjectTileFlags(TF_SHOW);
									m_DragMode = DM3D_DRAGOBJECT;
									IsCaptured = TRUE;
									SetCapture();
									DebugPrint("StartDrag\n");
								}
							}
						}
						break;
				}
			}
		}
	}

	CScrollView::OnLButtonDown(nFlags, point);
}


BOOL CBTEditView::ApplyToolOnce(CPoint point)
{
	CBTEditDoc* pDoc = GetDocument();
	D3DVECTOR CameraPosition,CameraRotation;
	pDoc->GetCamera(CameraRotation,CameraPosition);
	int Selected;

	CHeightMap *HeightMap = pDoc->GetHeightMap();

	if(pDoc->InLocator(m_MouseX,m_MouseY)) {
		CameraPosition.x = (float)(( ((SLONG)(m_MouseX-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapWidth()/2)) * (SLONG)pDoc->GetTileWidth());
		CameraPosition.z = -(float)(( ((SLONG)(m_MouseY-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapHeight()/2)) * (SLONG)pDoc->GetTileHeight());
		if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//			CameraPosition.y = pDoc->GetHeightAt(CameraPosition)+CAM_HEIGHT;
			CameraPosition.y = CAM_HEIGHT;
		}
		CenterValid = FALSE;
		pDoc->RegisterCamera(CameraRotation,CameraPosition);
		InvalidateRect(NULL,NULL);

		if(pDoc->Get2DMode() == M2D_WORLD) {
			if(pDoc->GetAutoSync()) {
				pDoc->Set2DViewPos();		// Bind views.
				pDoc->Invalidate2D();
			}
		}
		return FALSE;
	}

	if(pDoc->InLocatorBorder(m_MouseX,m_MouseY)) {
		return FALSE;
	}

	BOOL DoneSomething = FALSE;

	switch(pDoc->GetEditTool()) {
		case ET_ROTATETILE:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

				DWORD Rotate = HeightMap->GetTextureRotate(Selected);
				Rotate++;
				Rotate &= 3;
				HeightMap->SetTextureRotate(Selected,Rotate);
				InvalidateRect(NULL,NULL);
				pDoc->Invalidate2D();
				DoneSomething = TRUE;
  			}
			break;

		case ET_XFLIPTILE:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

				BOOL FlipX = HeightMap->GetTextureFlipX(Selected);
				BOOL FlipY = HeightMap->GetTextureFlipY(Selected);
				DWORD Rotate = HeightMap->GetTextureRotate(Selected);
				if(Rotate & 1) {
					HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
				} else {
					HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
				}
				InvalidateRect(NULL,NULL);
				pDoc->Invalidate2D();
				DoneSomething = TRUE;
  			}
			break;

		case ET_YFLIPTILE:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

				BOOL FlipX = HeightMap->GetTextureFlipX(Selected);
				BOOL FlipY = HeightMap->GetTextureFlipY(Selected);
				DWORD Rotate = HeightMap->GetTextureRotate(Selected);
				if(Rotate & 1) {
					HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
				} else {
					HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
				}
				InvalidateRect(NULL,NULL);
				pDoc->Invalidate2D();
				DoneSomething = TRUE;
  			}
			break;

		case ET_POINT:
			{
				HeightMap->DeSelectAll3DObjects();
				D3DVECTOR CameraPosition,CameraRotation;
				pDoc->GetCamera(CameraRotation,CameraPosition);
				int ObjID = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);
				if(ObjID >= 0) {
					HeightMap->Select3DObject(ObjID);
				}
				InvalidateRect(NULL,NULL);
				pDoc->Invalidate2D();
				DoneSomething = TRUE;
			}
			break;

//		case ET_MOVE:
//			{
//				HeightMap->DeSelectAll3DObjects();
//				D3DVECTOR CameraPosition,CameraRotation;
//				pDoc->GetCamera(CameraRotation,CameraPosition);
//				int ObjID = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);
//				if(ObjID >= 0) {
//					HeightMap->Select3DObject(ObjID);
//				}
//				InvalidateRect(NULL,NULL);
//				pDoc->Invalidate2D();
//				DoneSomething = TRUE;
//			}
//			break;

		case ET_PAINT:
			if(!pDoc->m_SelIsTexture) {
				Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
				if(Selected >= 0) {
					D3DVECTOR Rotation = {0.0F, 0.0F, 0.0F};
					D3DVECTOR Position;
					CTile *Tile = HeightMap->GetTile(Selected);

					Position = Tile->Position;
					Position.y = HeightMap->GetHeight(Position.x,-Position.z);

					DWORD ObjToAdd = pDoc->m_SelObject;
//					if(HeightMap->GetObjectType(ObjToAdd) == IMD_STRUCTURE) {
//						ObjToAdd += pDoc->GetCurrentPlayer();
//					}

					int ObjID = HeightMap->AddObject(ObjToAdd,Rotation,Position,pDoc->GetCurrentPlayer());
					HeightMap->SnapObject(ObjID);
					HeightMap->SetObjectTileFlags(ObjID,TF_HIDE);
					HeightMap->SetObjectTileHeights(ObjID);
					HeightMap->InitialiseSectors();

					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
					DoneSomething = TRUE;
				}
			}
			break;

		case ET_PASTERECT:
			if( pDoc->ClipboardIsValid() ) {
				pDoc->PasteTileRect(HeightMap->GetSelectionBox0());
				pDoc->BuildRadarMap();
				pDoc->GetHeightMap()->InitialiseSectors();
				pDoc->Invalidate2D();
				pDoc->Invalidate3D();
			}
			break;

		case ET_ROTATEOBJECT:
			HeightMap->DeSelectAll3DObjects();
			pDoc->GetCamera(CameraRotation,CameraPosition);
			SelectedObject = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,point.x,point.y);
			if(SelectedObject >= 0) {
				if(HeightMap->GetObjectInstanceFlanged(SelectedObject)) {
					HeightMap->Select3DObject(SelectedObject);
					HeightMap->SetObjectTileFlags(TF_SHOW);

					D3DVECTOR Rotation;
					HeightMap->Get3DObjectRotation(SelectedObject,Rotation);
					Rotation.y += 90;
					if(Rotation.y >= 360.0F) {
						Rotation.y -= 360.0F;
					}
					HeightMap->Set3DObjectRotation(SelectedObject,Rotation);
					HeightMap->SetObjectTileFlags(TF_HIDE);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
					DoneSomething = TRUE;
				}
			}
			break;

		case ET_TRIDIR1:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

				DWORD Dir = pDoc->GetHeightMap()->GetVertexFlip(Selected);
				Dir = !Dir;
				pDoc->GetHeightMap()->SetVertexFlip(Selected,Dir);
				m_HeightsChanged = TRUE;
				InvalidateRect(NULL,NULL);
			}
			break;
	}

	return DoneSomething;
}


//void CBTEditView::ApplyRandomness(int Selected)
//{
//	CBTEditDoc* pDoc = GetDocument();
//	CHeightMap *HeightMap = pDoc->GetHeightMap();
//
//	if(pDoc->m_SelFlags & (SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY | SF_RANDTEXTUREROTATE |
//							SF_TEXTUREFLIPX | SF_TEXTUREFLIPY | SF_TEXTUREROTMASK)) {
//		BOOL FlipX = HeightMap->GetTextureFlipX(Selected);
//		BOOL FlipY = HeightMap->GetTextureFlipY(Selected);
//		DWORD Rotate = HeightMap->GetTextureRotate(Selected);
//		BOOL OFlipX = FlipX;
//		BOOL OFlipY = FlipY;
//		DWORD ORotate = Rotate;
//
//		if(pDoc->m_SelFlags & (SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY | SF_RANDTEXTUREROTATE)) {
//			do {
//				if(pDoc->m_SelFlags & SF_RANDTEXTUREFLIPX) {
//					FlipX = rand()%2 ? FALSE : TRUE;
//				}
//				if(pDoc->m_SelFlags & SF_RANDTEXTUREFLIPY) {
//					FlipY = rand()%2 ? FALSE : TRUE;
//				}
//				if(pDoc->m_SelFlags & SF_RANDTEXTUREROTATE) {
//					Rotate = rand()%3;
//				}
//
//				if(pDoc->m_SelFlags & SF_TOGTEXTUREFLIPX) {
//					FlipX = FlipX ? FALSE : TRUE;
//				}
//				if(pDoc->m_SelFlags & SF_TOGTEXTUREFLIPY) {
//					FlipY = FlipY ? FALSE : TRUE;
//				}
//				if(pDoc->m_SelFlags & SF_INCTEXTUREROTATE) {
//					Rotate = (Rotate + 1)%3;
//				}
//				// Make sure something changed.
//			} while((FlipX == OFlipX) && (FlipY == OFlipY) && (Rotate == ORotate));
//		}
//
//		if( !(pDoc->m_SelFlags & SF_RANDTEXTUREFLIPX) ) {
//			FlipX = FALSE;
//		}
//		if( !(pDoc->m_SelFlags & SF_RANDTEXTUREFLIPY) ) {
//			FlipY = FALSE;
//		}
//		if( !(pDoc->m_SelFlags & SF_RANDTEXTUREROTATE) ) {
//			Rotate = 0;
//		}
//
//		if(pDoc->m_SelFlags & SF_TEXTUREFLIPX) {
//			FlipX = TRUE;
//		}
//
//		if(pDoc->m_SelFlags & SF_TEXTUREFLIPY) {
//			FlipY = TRUE;
//		}
//
//		if(pDoc->m_SelFlags & SF_TEXTUREROTMASK) {
//			Rotate = (pDoc->m_SelFlags & SF_TEXTUREROTMASK)>>SF_TEXTUREROTSHIFT;
//		}
//
//		HeightMap->SetTextureFlip(Selected,FlipX,FlipY);
//		HeightMap->SetTextureRotate(Selected,Rotate);
//	} else {
//		HeightMap->SetTextureFlip(Selected,FALSE,FALSE);
//		HeightMap->SetTextureRotate(Selected,0);
//	}
//}


void CBTEditView::ApplyTool(CPoint point,int XVel,int YVel,BOOL JustPressed)
{
	static SDWORD LastSelected = -1;
	static SDWORD LastSelTexture = -1;
	static SDWORD LastSelType = -1;

	CBTEditDoc* pDoc = GetDocument();
	D3DVECTOR CameraPosition,CameraRotation;
	pDoc->GetCamera(CameraRotation,CameraPosition);
	int Selected;

	CHeightMap *HeightMap = pDoc->GetHeightMap();

	if(pDoc->InLocator(m_MouseX,m_MouseY)) {
		CameraPosition.x = (float)(( ((SLONG)(m_MouseX-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapWidth()/2)) * (SLONG)pDoc->GetTileWidth());
		CameraPosition.z = -(float)(( ((SLONG)(m_MouseY-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapHeight()/2)) * (SLONG)pDoc->GetTileHeight());
		if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//			CameraPosition.y = pDoc->GetHeightAt(CameraPosition)+CAM_HEIGHT;
			CameraPosition.y = CAM_HEIGHT;
		}
		CenterValid = FALSE;
		pDoc->RegisterCamera(CameraRotation,CameraPosition);
		InvalidateRect(NULL,NULL);

		if(pDoc->Get2DMode() == M2D_WORLD) {
			if(pDoc->GetAutoSync()) {
				pDoc->Set2DViewPos();		// Bind views.
				pDoc->Invalidate2D();
			}
		}
		return;
	}

	if(pDoc->InLocatorBorder(m_MouseX,m_MouseY)) {
		return;
	}

	switch(pDoc->GetEditTool()) {
   		case ET_MARKRECT:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected > 0) {
				switch(Mark3dRectCorners) {
   					case	0:
   					case	2:
   	   					Mark3dRect0 = Selected;
						HeightMap->SetSelectionBox0(Selected);
   						Mark3dRectCorners=1;
   						break;
   					case	1:
   	   					Mark3dRect1 = Selected;
						HeightMap->SetSelectionBox1(Selected);
						pDoc->Update3DView();
//						InvalidateRect(NULL,NULL);
   						break;
   				}
			}
   			break;

		case ET_GET:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected >= 0) {
				pDoc->m_SelTexture = HeightMap->GetTextureID(Selected);
				pDoc->GetTextureSelector()->SetSelectedTexture(pDoc->m_SelTexture);
				pDoc->UpdateTextureView();
			}
			break;

		case ET_HIDETILE:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected >= 0) {
				if(Selected != LastSelected) {
					LastSelected = Selected;

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					pDoc->GetHeightMap()->SetTileVisible(Selected,TF_HIDE);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
				}
			}
			break;

		case ET_SHOWTILE:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected >= 0) {
				if(Selected != LastSelected) {
					LastSelected = Selected;

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

	   				pDoc->GetHeightMap()->SetTileVisible(Selected,TF_SHOW);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
				}
			}
			break;

		case ET_PAINT:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
			if(Selected >= 0) {
				if(pDoc->m_SelIsTexture) {
					if( (Selected != LastSelected) || (JustPressed) ||
						((SDWORD)pDoc->m_SelTexture != LastSelTexture) ||
						((SDWORD)pDoc->m_SelType  != LastSelType) ) {

						LastSelected = Selected;
						LastSelTexture = pDoc->m_SelTexture;
						LastSelType = pDoc->m_SelType;

						if(pDoc->m_SelType == TF_TYPEWATER) {
							if(pDoc->GetAutoHeight()) {
								g_UndoRedo->BeginGroup();
							}
							g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
							pDoc->SetTextureID(Selected,pDoc->m_SelTexture);
							HeightMap->SetTileType(Selected,pDoc->m_SelType);

							HeightMap->ApplyRandomness(Selected,pDoc->m_SelFlags);

//							DWORD Flags = HeightMap->GetTileFlags(Selected);
//							Flags &= TF_GEOMETRYMASK | TF_TYPEMASK;
//							Flags |= pDoc->m_SelFlags;
//							HeightMap->SetTileFlags(Selected,Flags);

							if(pDoc->GetAutoHeight()) {
								HeightMap->SetTileHeightUndo(Selected,(float)HeightMap->GetSeaLevel());
								g_UndoRedo->EndGroup();
								m_HeightsChanged = TRUE;
							}
							InvalidateRect(NULL,NULL);
						} else {
							g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
							pDoc->SetTextureID(Selected,pDoc->m_SelTexture);
							HeightMap->SetTileType(Selected,pDoc->m_SelType);

							HeightMap->ApplyRandomness(Selected,pDoc->m_SelFlags);
							
//							DWORD Flags = HeightMap->GetTileFlags(Selected);
//							Flags &= TF_GEOMETRYMASK | TF_TYPEMASK;
//							Flags |= pDoc->m_SelFlags;
//							HeightMap->SetTileFlags(Selected,Flags);

							pDoc->RedrawFace(Selected);
						}


						pDoc->Invalidate2D();
					}
				}
			}
			break;

		case ET_EDGEPAINT:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {
				if(Selected != LastSelected) {
					LastSelected = Selected;

					int mw = pDoc->GetMapWidth();
					int y = Selected / mw;
					int x = Selected - (y*mw);
					if(pDoc->GetEdgeBrush()->Paint(x,y,pDoc->GetAutoHeight())) {
						m_HeightsChanged = TRUE;
					}
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
//					g_UndoRedo->DumpStack();
				}
			}
			break;

		case ET_EDGEFILL:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {
//   				if(pDoc->m_SelIsTexture) {
  					pDoc->GetEdgeBrush()->FillMap(Selected,pDoc->m_SelTexture,pDoc->m_SelType,pDoc->m_SelFlags);
   					m_HeightsChanged = TRUE;
   					InvalidateRect(NULL,NULL);
   					pDoc->Invalidate2D();
//   				}
   			}
			break;

		case ET_BRUSHPAINT:
			break;

		case ET_FILL:
			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
   			if(Selected >= 0) {
				if(pDoc->m_SelIsTexture) {
   					pDoc->FillMap(Selected,pDoc->m_SelTexture,pDoc->m_SelType,pDoc->m_SelFlags);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
				}
   			}
			break;

		case ET_GETVERTEX:
			if(pDoc->m_TileMode) {
				Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
				if(Selected >= 0) {
					m_CurrentHeight = HeightMap->GetTileHeight(Selected);
				}
			} else {
				m_SelVerticies = HeightMap->FindVerticies(CameraRotation,CameraPosition,point.x,point.y);
				if(m_SelVerticies) {
					m_CurrentHeight = HeightMap->GetVertexHeight(SelVerticies[0].TileIndex,SelVerticies[0].VertIndex);
				}
			}
			break;

		case ET_PAINTVERTEX:
			if(pDoc->m_TileMode) {
				Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
				if(Selected >= 0) {
					HeightMap->SetTileHeight(Selected,m_CurrentHeight);
					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			} else {
				m_SelVerticies = HeightMap->FindVerticies(CameraRotation,CameraPosition,point.x,point.y);

				if(m_SelVerticies == 4) {
					DWORD i;

					for(i=0; i<m_SelVerticies; i++) {
						HeightMap->SetVertexHeight(SelVerticies[i].TileIndex,SelVerticies[i].VertIndex,m_CurrentHeight);
					}
					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			}
			break;

		case ET_PAINTVERTEXSEALEVEL:
			if(pDoc->m_TileMode) {
				Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
				if(Selected >= 0) {
					HeightMap->SetTileHeight(Selected,(float)HeightMap->GetSeaLevel());
					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			} else {
				m_SelVerticies = HeightMap->FindVerticies(CameraRotation,CameraPosition,point.x,point.y);

				if(m_SelVerticies == 4) {
					DWORD i;

					for(i=0; i<m_SelVerticies; i++) {
						HeightMap->SetVertexHeight(SelVerticies[i].TileIndex,SelVerticies[i].VertIndex,(float)HeightMap->GetSeaLevel());
					}
					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			}
			break;

		case ET_DRAGVERTEX:
			if(!IsCaptured) {
				IsCaptured = TRUE;
				SetCapture();
			}

			if(pDoc->m_TileMode) {
				if(m_Selected < 0) {
					m_Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
					if(m_Selected >= 0) {
						m_CurrentHeight = HeightMap->GetTileHeight(m_Selected);
					}
				} else {
//					DWORD i;

					m_CurrentHeight -= YVel;

					if(m_CurrentHeight < 0) {
						m_CurrentHeight = 0.0F;
					} else {
						if(m_CurrentHeight > (float)255) {
							m_CurrentHeight = (float)255;
						}
					}
					HeightMap->RaiseTile(m_Selected,(float)-YVel);

					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			} else {
				if(m_SelVerticies == 0) {
					m_SelVerticies = HeightMap->FindVerticies(CameraRotation,CameraPosition,point.x,point.y);
					if(m_SelVerticies == 4) {
						m_CurrentHeight = HeightMap->GetVertexHeight(SelVerticies[0].TileIndex,SelVerticies[0].VertIndex);
					}
				}

				if(m_SelVerticies == 4) {
					DWORD i;

					m_CurrentHeight -= YVel;

					if(m_CurrentHeight < 0) {
						m_CurrentHeight = 0.0F;
					} else {
						if(m_CurrentHeight > (float)255) {
							m_CurrentHeight = (float)255;
						}
					}

					for(i=0; i<m_SelVerticies; i++) {
						HeightMap->SetVertexHeight(SelVerticies[i].TileIndex,SelVerticies[i].VertIndex,m_CurrentHeight);
					}
					m_HeightsChanged = TRUE;
					InvalidateRect(NULL,NULL);
				}
			}
			break;

//		case ET_TRIDIR1:
//			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
//			if(Selected >= 0) {
//				pDoc->GetHeightMap()->SetVertexFlip(Selected,0);
//				InvalidateRect(NULL,NULL);
//			}
//			break;
//
//		case ET_TRIDIR2:
//			Selected = pDoc->SelectFace(CameraRotation,CameraPosition,point.x,point.y);
//			if(Selected >= 0) {
//				pDoc->GetHeightMap()->SetVertexFlip(Selected,1);
//				InvalidateRect(NULL,NULL);
//			}
//			break;
	}
}


void CBTEditView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();

// If button up and mouse input was captured by this view then release
// capture.
	if(IsCaptured) {
		ReleaseCapture();
		IsCaptured = FALSE;
	}

	m_SelVerticies = 0;
	m_Selected = -1;

	if(m_DragMode == DM3D_DRAGOBJECT) {
		m_DragMode = DM3D_NODRAG;
		HeightMap->SetObjectTileFlags(TF_HIDE);
		InvalidateRect(NULL,NULL);
		DebugPrint("StopDrag\n");
	} else {
		switch(pDoc->GetEditTool()) {
			case ET_MARKRECT:
				if(Mark3dRectCorners) {
//					pDoc->YFlipTileRect(Mark3dRect0,Mark3dRect1);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
					Mark3dRectCorners=0;
				}
				break;
		}

		if(m_HeightsChanged) {
			CBTEditDoc* pDoc = GetDocument();
			pDoc->BuildRadarMap();
			pDoc->GetHeightMap()->InitialiseSectors();
			InvalidateRect(NULL,NULL);
			m_HeightsChanged = FALSE;
		}
//		CBTEditDoc* pDoc = GetDocument();
//		switch(pDoc->GetEditTool()) {
//			case ET_PAINTVERTEX:
//			case ET_DRAGVERTEX:
//				pDoc->BuildRadarMap();
//				pDoc->GetHeightMap()->InitialiseSectors();
//				InvalidateRect(NULL,NULL);
//				break;
//		}
	}

	CScrollView::OnLButtonUp(nFlags, point);
}


void CBTEditView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
// If this view just got the focus then make sure it gets redrawn.
	if(bActivate) {
		InvalidateRect(NULL,NULL);
//		m_Redraw = TRUE;
	}

	m_Active = bActivate;
	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CBTEditView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CBTEditDoc* pDoc = GetDocument();
	CGeometry *Maths = pDoc->GetDirectMaths(); 
	int Selected;
	D3DVECTOR LightDir;

	m_KeyHandler->HandleKeyDown(nChar);

//	DebugPrint("%d %d\n",nChar,nFlags);

	BOOL Changed = FALSE;	
	D3DVECTOR	CameraPos,CameraRot,TmpRot;
	pDoc->GetCamera(CameraRot,CameraPos);

// Adjust camera y position ( adjusted for heading )
	TmpRot.x = TmpRot.z = 0.0F;
	if(m_KeyHandler->GetKeyState(VK_LEFT)) {
		TmpRot.y = CameraRot.y-90.0F;
		pDoc->GetDirectMaths()->Motion(CameraPos,TmpRot,(float)pDoc->GetTileWidth());
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(VK_RIGHT)) {
			TmpRot.y = CameraRot.y+90.0F;
			pDoc->GetDirectMaths()->Motion(CameraPos,TmpRot,(float)pDoc->GetTileWidth());
			Changed = TRUE;
		}
	}

// Adjust camera z position ( adjusted for heading )
	if(m_KeyHandler->GetKeyState(VK_UP)) {
		TmpRot.y = CameraRot.y;
		pDoc->GetDirectMaths()->Motion(CameraPos,TmpRot,(float)pDoc->GetTileWidth());
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(VK_DOWN)) {
			TmpRot.y = CameraRot.y;
			pDoc->GetDirectMaths()->Motion(CameraPos,TmpRot,-(float)pDoc->GetTileWidth());
			Changed = TRUE;
		}
	}

// Adjust camera Height.
	if(m_KeyHandler->GetKeyState(KEY_CAMERAUP)) {
		CameraPos.y+=64;
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(KEY_CAMERADOWN)) {
			CameraPos.y-=64;
			Changed = TRUE;
		}
	}
	
// Adjust camera Heading.
	if(m_KeyHandler->GetKeyState(KEY_CAMERAROTLEFT)) {
		CameraRot.y-=10;
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(KEY_CAMERAROTRIGHT)) {
			CameraRot.y+=10;
			Changed = TRUE;
		}
	}
	

// Adjust camera Pitch.
	if(pDoc->GetCameraMode() != MCAM_ISOMETRIC) {
		if(m_KeyHandler->GetKeyState(KEY_CAMERAPITCHDOWN)) {
			CameraRot.x-=20;
			Changed = TRUE;
		} else {
			if(m_KeyHandler->GetKeyState(KEY_CAMERAPITCHUP)) {
				CameraRot.x+=20;
				Changed = TRUE;
			}
		}
	}

// Adjust sea level.
	if(m_KeyHandler->GetKeyState(KEY_SEADOWN)) {
		CHeightMap *HeightMap = pDoc->GetHeightMap();
		int SeaLevel = HeightMap->GetSeaLevel() - 1;
		if(SeaLevel < 0) {
			SeaLevel = 0;
		}
		HeightMap->SetSeaLevel(SeaLevel);
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(KEY_SEAUP)) {
			CHeightMap *HeightMap = pDoc->GetHeightMap();
			int SeaLevel = HeightMap->GetSeaLevel() + 1;
			if(SeaLevel > 255) {
				SeaLevel = 255;
			}
			HeightMap->SetSeaLevel(SeaLevel);
			Changed = TRUE;
		}
	}

// Adjust draw radius.
	if(m_KeyHandler->GetKeyState(KEY_DECVIEWDIST)) {
		CHeightMap *HeightMap = pDoc->GetHeightMap();
		int DrawRadius = HeightMap->GetDrawRadius() - 1;
		if(DrawRadius < MINDRAWRADIUS) {
			DrawRadius = MINDRAWRADIUS;
		}
		HeightMap->SetDrawRadius(DrawRadius);
		Changed = TRUE;
	} else {
		if(m_KeyHandler->GetKeyState(KEY_INCVIEWDIST)) {
			CHeightMap *HeightMap = pDoc->GetHeightMap();
			int DrawRadius = HeightMap->GetDrawRadius() + 1;
			if(DrawRadius > MAXDRAWRADIUS) {
				DrawRadius = MAXDRAWRADIUS;
			}
			HeightMap->SetDrawRadius(DrawRadius);
			Changed = TRUE;
		}
	}

	CHeightMap *HeightMap = pDoc->GetHeightMap();

// If things have changed then update the views.
	if(Changed) {
		if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//			CameraPos.y = pDoc->GetHeightAt(CameraPos)+CAM_HEIGHT;
			CameraPos.y = CAM_HEIGHT;
		}
		CenterValid = FALSE;
		pDoc->RegisterCamera(CameraRot,CameraPos);
		InvalidateRect(NULL,NULL);

		if(pDoc->Get2DMode() == M2D_WORLD) {
			if(pDoc->GetAutoSync()) {
				pDoc->Set2DViewPos();		// Bind views.
  				pDoc->Invalidate2D();
			}
		}
	} else {
		
		switch(nChar) {
			case	VK_SHIFT:
				ShiftDown = TRUE;
				break;

			case	KEY_SYNCPOSITION:
				SyncPosition();
				InvalidateRect(NULL,NULL);
				break;

			case	KEY_YAWLIGHT:
				pDoc->GetLightDirection(&LightDir);
				LightDir.y += 10;
				pDoc->SetLightDirection(&LightDir);
				InvalidateRect(NULL,NULL);
				break;

			case	KEY_PITCHLIGHT:
				pDoc->GetLightDirection(&LightDir);
				LightDir.x += 10;
				pDoc->SetLightDirection(&LightDir);
				InvalidateRect(NULL,NULL);
				break;

			case	VK_DELETE:
				HeightMap->SetObjectTileFlags(TF_SHOW);
				HeightMap->DeleteSelected3DObjects();
				HeightMap->SetObjectTileFlags(TF_HIDE);
				InvalidateRect(NULL,NULL);
	  			pDoc->Invalidate2D();
				break;

			case	KEY_ORBITLEFT:
					if(!CenterValid) {
						TmpRot.x = TmpRot.z = 0.0F;
						TmpRot.y = CameraRot.y;
						Center = CameraPos;
						Center.y = 0.0F;
						Maths->Motion(Center,TmpRot,(float)(ORBITRADIUS));
						CenterValid = TRUE;
						OrbitAngle = TmpRot.y;
					}
					// Now orbit the camera around the point.
					TmpRot.x = CameraRot.x;
					OrbitAngle = Maths->Orbit(CameraPos,CameraRot,Center,OrbitAngle,(float)ORBITRADIUS,(float)-ORBITSTEP);
					if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
						CameraRot.x = TmpRot.x;
					}
					pDoc->RegisterCamera(CameraRot,CameraPos);
					InvalidateRect(NULL,NULL);
				break;

			case	KEY_ORBITRIGHT:
					if(!CenterValid) {
						TmpRot.x = TmpRot.z = 0.0F;
						TmpRot.y = CameraRot.y;
						Center = CameraPos;
						Center.y = 0.0F;
						Maths->Motion(Center,TmpRot,(float)(ORBITRADIUS));
						OrbitAngle = TmpRot.y;
						CenterValid = TRUE;
					}
					// Now orbit the camera around the point.
					TmpRot.x = CameraRot.x;
					OrbitAngle = Maths->Orbit(CameraPos,CameraRot,Center,OrbitAngle,(float)ORBITRADIUS,(float)ORBITSTEP);
					if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
						CameraRot.x = TmpRot.x;
					}
					pDoc->RegisterCamera(CameraRot,CameraPos);
					InvalidateRect(NULL,NULL);
				break;

			case	KEY_ROTATETILE:
				Selected = pDoc->SelectFace(CameraRot,CameraPos,m_MouseX,m_MouseY);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					DWORD Rotate = HeightMap->GetTextureRotate(Selected);
					Rotate++;
					Rotate &= 3;
					HeightMap->SetTextureRotate(Selected,Rotate);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
  				}
				break;

			case	KEY_ROTATETILE2:
				Selected = pDoc->SelectFace(CameraRot,CameraPos,m_MouseX,m_MouseY);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					DWORD Rotate = HeightMap->GetTextureRotate(Selected);
					Rotate--;
					Rotate &= 3;
					HeightMap->SetTextureRotate(Selected,Rotate);
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
  				}
				break;

			case	KEY_XFLIPTILE:
				Selected = pDoc->SelectFace(CameraRot,CameraPos,m_MouseX,m_MouseY);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					BOOL FlipX = HeightMap->GetTextureFlipX(Selected);
					BOOL FlipY = HeightMap->GetTextureFlipY(Selected);
					DWORD Rotate = HeightMap->GetTextureRotate(Selected);
					if(Rotate & 1) {
						HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
					} else {
						HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
					}
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
  				}
				break;

			case	KEY_YFLIPTILE:
				Selected = pDoc->SelectFace(CameraRot,CameraPos,m_MouseX,m_MouseY);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					BOOL FlipX = HeightMap->GetTextureFlipX(Selected);
					BOOL FlipY = HeightMap->GetTextureFlipY(Selected);
					DWORD Rotate = HeightMap->GetTextureRotate(Selected);
					if(Rotate & 1) {
						HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
					} else {
						HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
					}
					InvalidateRect(NULL,NULL);
					pDoc->Invalidate2D();
  				}
				break;

			case	KEY_TRIFLIP:
				Selected = pDoc->SelectFace(CameraRot,CameraPos,m_MouseX,m_MouseY);
				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

					DWORD Dir = HeightMap->GetVertexFlip(Selected);
					Dir = !Dir;
					HeightMap->SetVertexFlip(Selected,Dir);
					InvalidateRect(NULL,NULL);
				}
				break;


//			case	'O':
//				{
//					C3DObjectInstance *Object = HeightMap->GetObjectPointer(0);
//					DWORD NumIMD = HeightMap->GetNumIMD();
//					Object->ObjectID++;
//					if(Object->ObjectID >= HeightMap->GetNumIMD()) {
//						Object->ObjectID =0;
//					}
//					InvalidateRect(NULL,NULL);
//		  			pDoc->Invalidate2D();
//				}
//				break;
//
//			case	'R':
//				{
//					C3DObjectInstance *Object = HeightMap->GetObjectPointer(0);
//					Object->Rotation.y += 1;
//					InvalidateRect(NULL,NULL);
//		  			pDoc->Invalidate2D();
//				}
//				break;
		}

	}
	
	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CBTEditView::SyncPosition(void)
{
	D3DVECTOR	CameraPos,CameraRot;
	RECT ClientRect;
	CBTEditDoc* pDoc = GetDocument();

	GetClientRect(&ClientRect);
	pDoc->GetCamera(CameraRot,CameraPos);
	SLONG ScrollX,ScrollY;
	pDoc->GetScrollPosition((SLONG*)&ScrollX,(SLONG*)&ScrollY);
	SLONG TextureWidth = (SLONG)pDoc->GetTextureWidth();
	SLONG TextureHeight = (SLONG)pDoc->GetTextureHeight();
	SLONG TileWidth = (SLONG)pDoc->GetTileWidth();
	SLONG TileHeight = (SLONG)pDoc->GetTileHeight();
	SLONG MapWidthO2 = (SLONG)pDoc->GetMapWidth()/2;
	SLONG MapHeightO2 = (SLONG)pDoc->GetMapHeight()/2;

	CameraPos.x = (float)(((ScrollX+(ClientRect.right/2))/TextureWidth)*TileWidth) - (MapWidthO2*TileWidth);
	CameraPos.z = -((float)(((ScrollY+(ClientRect.bottom/2))/TextureHeight)*TileHeight) - (MapHeightO2*TileHeight));
	if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//		CameraPos.y = pDoc->GetHeightAt(CameraPos)+CAM_HEIGHT;
			CameraPos.y = CAM_HEIGHT;
	}

	CenterValid = FALSE;
	pDoc->RegisterCamera(CameraRot,CameraPos);
}

BOOL CBTEditView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if(nHitTest == HTCLIENT) {
		CBTEditDoc* pDoc = GetDocument();

		if(!pDoc->InLocatorBorder(m_MouseX,m_MouseY)) {
			pDoc->SetEditToolCursor();
		} else {
			::SetCursor(g_Pointer);
		}
		return TRUE;
	}

	return CScrollView::OnSetCursor(pWnd, nHitTest, message);
}

//void CBTEditView::SelectObject(CPoint &point)
//{
//	DWORD Instance;
//
//	CObjectDB* ObjectDB = GetObjectVertex(point,&Instance,NULL);
//	if(ObjectDB) {
//		CBTEditDoc* pDoc = GetDocument();
//		ObjectDB->Set3DObjectSelected(Instance);
//
//		pDoc->SetStatsView(ObjectDB,Instance);
//
//		InvalidateRect(NULL,NULL);
//  		pDoc->Invalidate2D();
//	}
//}
//
//void CBTEditView::DeSelectObject(CPoint &point)
//{
//	DWORD Instance;
//
//	CObjectDB* ObjectDB = GetObjectVertex(point,&Instance,NULL);
//	if(ObjectDB) {
//		CBTEditDoc* pDoc = GetDocument();
//		ObjectDB->Clear3DObjectSelected(Instance);
//		InvalidateRect(NULL,NULL);
//  		pDoc->Invalidate2D();
//	}
//}
//
//void CBTEditView::GetObjectHeight(CPoint &point)
//{
//
//	D3DVECTOR Vector;
//	CObjectDB* ObjectDB = GetObjectVertex(point,NULL,&Vector);
//	m_CurrentHeight = Vector.y;
//}
//
//
//CObjectDB* CBTEditView::GetObjectVertex(CPoint &point,DWORD *Instance,D3DVECTOR *Vector)
//{
//	CBTEditDoc* pDoc = GetDocument();
//	D3DVECTOR CameraPosition,CameraRotation;
//	pDoc->GetCamera(CameraRotation,CameraPosition);
//
// 	ListNode<CObjectDB>* TmpNode = pDoc->GetObjectDatabase();
//
//	while(TmpNode!=NULL) {
//		CObjectDB* ObjectDB = TmpNode->GetData();
//		D3DVECTOR Position,Rotation;
//		DWORD Instances = ObjectDB->GetInstances();
//		for(DWORD i=0; i<Instances; i++) {
//			ObjectDB->Get3DObjectPosition(i,Position);
//			ObjectDB->Get3DObjectRotation(i,Rotation);
//			int Selected = ObjectDB->Get3DObject()->FindVertices(&Rotation,&Position,
//										&CameraRotation,&CameraPosition,
//										point.x,point.y);
////			DebugPrint("Selected %d\n\n",Selected);
//			if(Selected) {
//				if(Instance) {
//					*Instance = i;
//				}
//				if(Vector) {
//					*Vector = SelObjectVerticies[0].Vertex;
//				}
//				return ObjectDB;
//			}
//		}
//
//		TmpNode = TmpNode->GetNextNode();
//	}
//
//	return NULL;
//}

void CBTEditView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if((!IsCaptured) && (!ShiftDown)) {
		CBTEditDoc* pDoc = GetDocument();
		CHeightMap *HeightMap = pDoc->GetHeightMap();
		D3DVECTOR CameraPosition,CameraRotation;

		HeightMap->DeSelectAll3DObjects();
		pDoc->GetCamera(CameraRotation,CameraPosition);

		int ObjID = HeightMap->FindObjectHit3D(CameraRotation,CameraPosition,m_MouseX,m_MouseY);

		if(ObjID >= 0) {
			HeightMap->Select3DObject(ObjID);
			InvalidateRect(NULL,NULL);
			pDoc->Invalidate2D();

			CMenu Menu;
			Menu.LoadMenu(IDR_3DPOPUP);
			Menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								point.x,point.y,this);
			m_CurrentObjectID = ObjID;
		}
	}
}


void CBTEditView::On3dpopupProperties() 
{
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();
	CObjectProperties Dlg;
	D3DVECTOR Position;
	D3DVECTOR Rotation;

	HeightMap->Get3DObjectPosition(m_CurrentObjectID,Position);
	HeightMap->Get3DObjectRotation(m_CurrentObjectID,Rotation);

	Dlg.m_Name = HeightMap->GetObjectInstanceDescription(m_CurrentObjectID);
	Dlg.m_Class = HeightMap->GetObjectInstanceTypeName(m_CurrentObjectID);
//	Dlg.m_ScriptName = HeightMap->GetObjectInstanceScriptName(m_CurrentObjectID);
	Dlg.m_UniqueID = HeightMap->GetObjectInstanceUniqueID(m_CurrentObjectID);
	Dlg.m_PlayerID = HeightMap->GetObjectInstancePlayerID(m_CurrentObjectID);
	Dlg.m_PosX = Position.x;
	Dlg.m_PosY = Position.y;
	Dlg.m_PosZ = Position.z;
	Dlg.m_RotX = Rotation.x;
	Dlg.m_RotY = Rotation.y;
	Dlg.m_RotZ = Rotation.z;
	Dlg.m_GPosX = (DWORD)(Position.x + (pDoc->GetMapWidth()*pDoc->GetTileWidth()/2));
	Dlg.m_GPosY = (DWORD)(-Position.z + (pDoc->GetMapHeight()*pDoc->GetTileHeight()/2));
	Dlg.m_GPosZ = (DWORD)(Position.y);

	if(Dlg.DoModal()==IDOK) {
		Position.x = Dlg.m_PosX;
		Position.y = Dlg.m_PosY;
		Position.z = Dlg.m_PosZ;
		Rotation.x = Dlg.m_RotX;
		Rotation.y = Dlg.m_RotY;
		Rotation.z = Dlg.m_RotZ;
		HeightMap->Set3DObjectPosition(m_CurrentObjectID,Position);
		HeightMap->Set3DObjectRotation(m_CurrentObjectID,Rotation);
		HeightMap->SetObjectInstancePlayerID(m_CurrentObjectID,Dlg.m_PlayerID);

//		char *String = Dlg.m_ScriptName.GetBuffer(0);
//		BOOL FoundSpace = FALSE;
//
//		for(int i=0; i<strlen(String); i++) {
//			if(String[i] == ' ') {
//				FoundSpace = TRUE;
//			}
//		}
//
//		if(FoundSpace) {
//			int Index = 0;
//			char NewName[MAX_SCRIPTNAME];
//
//			for(int i=0; i<strlen(String); i++) {
//				if(String[i] != ' ') {
//					NewName[Index] = String[i];
//					Index++;
//				}
//			}
//			NewName[Index] = 0;
//
////			HeightMap->SetObjectInstanceScriptName(m_CurrentObjectID,NewName);
//
//			MessageBox("Spaces removed from script name","Warning",MB_OK);
//		} else {
//			HeightMap->SetObjectInstanceScriptName(m_CurrentObjectID,Dlg.m_ScriptName.GetBuffer(0));
//		}
	}
}


// Edit selected objects stats.
//
void CBTEditView::EditStats(void)
{
//	CBTEditDoc* pDoc = GetDocument();
//
//	ListNode<CObjectDB>* TmpNode = pDoc->GetObjectDatabase();
//	while(TmpNode!=NULL) {
//		CObjectDB* ObjectDB = TmpNode->GetData();
//
//		DWORD Instances = ObjectDB->GetInstances();
//		for(DWORD i=0; i<Instances; i++) {
//			if(ObjectDB->Get3DObjectSelected(i)) {
//				CEditStats StatsDlg;
//				StatsDlg.SetStats(ObjectDB->GetInstanceStats(i));
//				StatsDlg.SetName(ObjectDB->GetFileName());
//				char Ins[16];
//				sprintf(Ins,"%d",i);
//				StatsDlg.SetInstance(Ins);
//				StatsDlg.DoModal();
//				char *Stats = StatsDlg.GetStats();
//				ObjectDB->SetInstanceStats(i,Stats);
//			}
//		}
//
//		TmpNode = TmpNode->GetNextNode();
//	}
}


void CBTEditView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_KeyHandler->HandleKeyUp(nChar);

	switch(nChar) {
		case	VK_SHIFT:
			ShiftDown = FALSE;
			break;
	};
	
	CScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CBTEditView::OnSetFocus(CWnd* pOldWnd) 
{
	CScrollView::OnSetFocus(pOldWnd);
	
	CBTEditDoc* pDoc = GetDocument();
	pDoc->Set3DFocus(TRUE);
	pDoc->Invalidate2D();
	InvalidateRect(NULL,FALSE);
}

void CBTEditView::OnKillFocus(CWnd* pNewWnd) 
{
	CScrollView::OnKillFocus(pNewWnd);
	
	CBTEditDoc* pDoc = GetDocument();
	pDoc->Set3DFocus(FALSE);
	InvalidateRect(NULL,FALSE);
}

