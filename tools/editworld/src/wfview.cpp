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
// WFView.cpp : implementation file
//

#include "stdafx.h"
#include "btedit.h"
#include "bteditdoc.h"
#include "wfview.h"
//#include "DBView.h"
#include "tiletypes.h"
//#include "EditStats.h"
#include "debugprint.hpp"
//#include "DebugWin.h"
#include "objectproperties.h"
#include "keyhandler.hpp"
#include "undoredo.h"

#define KEY_ROTATETILE	'R'
#define KEY_ROTATETILE2	'E'
#define KEY_XFLIPTILE	'X'
#define KEY_YFLIPTILE	'Y'


extern HCURSOR g_Pointer;
extern CUndoRedo *g_UndoRedo;

int CopyRectCorners=0;
int CopyRect0,CopyRect1;

int MarkRectCorners=0;
int MarkRect0,MarkRect1;

int GateX0,GateY0;		// Gateway start position;
int GateX1,GateY1;		// Gateway end position;
int GateMode = GATE_POSSTART;	// Current gateway positioning mode.
int GateIndex;



/////////////////////////////////////////////////////////////////////////////
// CWFView

IMPLEMENT_DYNCREATE(CWFView, CScrollView)


CWFView *WFView=NULL;

CWFView::CWFView()
{
	WFView = this;
	m_ViewIsInitialised=FALSE;
	m_DragMode=DM_NODRAG;
	m_KeyHandler = new KeyHandler();
	m_HeightsChanged = FALSE;
//	m_BrushDialog = new CBrushProp(this);
}

CWFView::~CWFView()
{
	delete m_KeyHandler;
	WFView = NULL;

//	delete m_BrushDialog;
}


BEGIN_MESSAGE_MAP(CWFView, CScrollView)
	//{{AFX_MSG_MAP(CWFView)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_COMMAND(ID_2DPOPUP_PROPERTIES, On2dpopupProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWFView drawing

void CWFView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

//	if(m_ViewIsInitialised) {
//		DeleteData();
//	}

	CSize sizeTotal;
	sizeTotal.cx = 1024;
	sizeTotal.cy = 1024;
	SetScrollSizes(MM_TEXT, sizeTotal);
	m_ScrollX = 0;
	m_ScrollY = 0;

//	CSize sizeTotal;
//	// TODO: calculate the total size of this view
//	sizeTotal.cx = sizeTotal.cy = 100;
//	SetScrollSizes(MM_TEXT, sizeTotal);

	InitialiseData();
}

void CWFView::OnDraw(CDC* pDC)
{
	CBTEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code here

	UpdateView(pDoc);
}

CBTEditDoc* CWFView::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTEditDoc)));

	return (CBTEditDoc*)m_pDocument;
}


void CWFView::InitialiseData(void)
{
	m_ViewIsInitialised=TRUE;
}


void CWFView::DeleteData(void)
{
}


void CWFView::UpdateView(CBTEditDoc* pDoc)
{
	CSize sizeTotal;
	sizeTotal.cx = pDoc->Get2DViewWidth() + pDoc->GetTextureWidth()*OVERSCAN*2;
	sizeTotal.cy = pDoc->Get2DViewHeight() + pDoc->GetTextureHeight()*OVERSCAN*2;
	SetScrollSizes(MM_TEXT, sizeTotal);

	CPoint ScrollPos = GetScrollPosition();
	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;

	pDoc->Register2DWindow(this->m_hWnd);
	pDoc->RegisterScrollPosition(ScrollPos.x,ScrollPos.y);
	pDoc->Update2DView();
}

/////////////////////////////////////////////////////////////////////////////
// CWFView message handlers

void CWFView::OnDestroy() 
{
	CScrollView::OnDestroy();
}

void CWFView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CBTEditDoc* pDoc = GetDocument();
	CPoint ScrollPos = GetScrollPosition();
	m_ScrollX = ScrollPos.x;
	m_ScrollY = ScrollPos.y;

	if(pDoc->ProcessButtonLapse()) {
		if(pDoc->Get2DMode() == M2D_WORLD) {
			if(!ApplyToolOnce(point)) {
				ApplyTool(point,TRUE);
			}
		} else {
			CEdgeBrush* EdgeBrush = pDoc->GetEdgeBrush();

			DWORD Command = EdgeBrush->DoButtons(m_MouseX,m_MouseY);

			switch(Command) {
				case EB_DECREMENT:
					EdgeBrush = pDoc->PrevEdgeBrush();
					UpdateAndValidate();
					pDoc->SyncBrushDialog();
					break;

				case EB_INCREMENT:
					EdgeBrush = pDoc->NextEdgeBrush();
					pDoc->SyncBrushDialog();
					UpdateAndValidate();
					break;

				case EB_SMALLBRUSH:
					EdgeBrush->SetButtonState(EB_SMALLBRUSH,1);
					EdgeBrush->SetButtonState(EB_LARGEBRUSH,0);
					EdgeBrush->SetLargeBrush(FALSE);
					pDoc->SyncBrushDialog();
					UpdateAndValidate();
					break;

				case EB_LARGEBRUSH:
					EdgeBrush->SetButtonState(EB_SMALLBRUSH,0);
					EdgeBrush->SetButtonState(EB_LARGEBRUSH,1);
					EdgeBrush->SetLargeBrush(TRUE);
					pDoc->SyncBrushDialog();
					UpdateAndValidate();
					break;

				default:
					switch(pDoc->GetEditTool()) {
						case ET_ROTATETILE:
							EdgeBrush->RotateTile(m_MouseX,m_MouseY,m_ScrollX,m_ScrollY-EBVIEW_YOFFSET);
							UpdateAndValidate();
							break;

						case ET_XFLIPTILE:
							EdgeBrush->XFlipTile(m_MouseX,m_MouseY,m_ScrollX,m_ScrollY-EBVIEW_YOFFSET);
							UpdateAndValidate();
							break;

						case ET_YFLIPTILE:
							EdgeBrush->YFlipTile(m_MouseX,m_MouseY,m_ScrollX,m_ScrollY-EBVIEW_YOFFSET);
							UpdateAndValidate();
							break;

						case ET_TRIDIR1:
							EdgeBrush->TriFlipTile(m_MouseX,m_MouseY,m_ScrollX,m_ScrollY-EBVIEW_YOFFSET);
							UpdateAndValidate();
							break;

						case ET_PAINT:
							EdgeBrush->SetBrushMap(pDoc->m_SelTexture,pDoc->m_SelFlags,
														m_MouseX,m_MouseY,m_ScrollX,m_ScrollY-EBVIEW_YOFFSET);
							UpdateAndValidate();
							break;
					}
			}
		}
	}

	CScrollView::OnLButtonDown(nFlags, point);
}

void CWFView::OnMouseMove(UINT nFlags, CPoint point) 
{
	static BOOL OldPosValid = FALSE;
	static CPoint OldPos(0,0);
	int XVel=0;
	int YVel=0;
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();

	if(OldPosValid) {
		XVel = point.x - OldPos.x;
		YVel = point.y - OldPos.y;
	}
	OldPos = point;
	OldPosValid = TRUE;

	m_MouseX = point.x;
	m_MouseY = point.y;

	if(pDoc->Get2DMode() == M2D_WORLD) {
		BOOL UpdateStatus = TRUE;
		CPoint ScrollPos = GetScrollPosition();
		ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
		ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;

		if(pDoc->ProcessButtonLapse()) {
			if((pDoc->GetEditTool() == ET_GATEWAY) && m_GotFocus) {
				if(GateMode == GATE_POSEND) {
					int Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);

					GateX1 = Selected%pDoc->GetMapWidth();
					GateY1 = Selected/pDoc->GetMapWidth();
					pDoc->GetHeightMap()->SetGateway(GateIndex,GateX0,GateY0,GateX1,GateY1);

					UpdateAndValidate();
				}
			} else if( nFlags & MK_LBUTTON ) {
				if(m_DragMode == DM_DRAGOBJECT) {
					if((m_MouseX > (SLONG)pDoc->GetMapWidth()) ||
						(m_MouseY > (SLONG)pDoc->GetMapHeight())) {
						switch(pDoc->GetEditTool()) {
							case ET_MOVE:
								UpdateStatus = FALSE;
								AutoScroll(point);
								PositionSelectedObjects(point);
								break;

							case ET_ROTATEOBJECT:
								if(m_ObjectID >= 0) {
									D3DVECTOR Rotation;

									UpdateStatus = FALSE;
									HeightMap->Select3DObject(m_ObjectID);

									HeightMap->Get3DObjectRotation(m_ObjectID,Rotation);
									Rotation.y += XVel;
									if(Rotation.y >= 360.0F) {
										Rotation.y -= 360.0F;
									}
									if(Rotation.y < 0.0F) {
										Rotation.y += 360.0F;
									}
									HeightMap->Set3DObjectRotation(m_ObjectID,Rotation);

									UpdateAndValidate();
									pDoc->Invalidate3D();
								}
								break;
						}
					} else {
						UpdateStatus = FALSE;
						ApplyTool(point);
					}
				} else {
					UpdateStatus = FALSE;
					ApplyTool(point);
				}
			} else {
				if( (pDoc->GetEditTool() == ET_PASTERECT) && pDoc->ClipboardIsValid() ) {
   					int Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
					if(Selected >= 0) {
						HeightMap->SetSelectionBox(Selected,pDoc->m_TileBufferWidth-1,pDoc->m_TileBufferHeight-1);
						HeightMap->ClipSelectionBox();
						UpdateAndValidate();
					}
				}
			}
		}

		if(UpdateStatus) {
			// Update the status bar.
			int Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
			if(Selected >= 0) {
				g_CursorTileX = Selected % pDoc->GetMapWidth();
				g_CursorTileZ = Selected / pDoc->GetMapWidth();
			} else {
				g_CursorTileX = -1;
				g_CursorTileZ = -1;
			}
			pDoc->UpdateStatusBar();
		}

	}

	CScrollView::OnMouseMove(nFlags, point);
}


BOOL CWFView::ApplyToolOnce(CPoint point)
{
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();
	D3DVECTOR CameraPosition,CameraRotation;
	CPoint ScrollPos = GetScrollPosition();
	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;
	
	int Selected;


	if(pDoc->InLocator(m_MouseX,m_MouseY)) {
		pDoc->GetCamera(CameraRotation,CameraPosition);
		CameraPosition.x = (float)(( ((SLONG)(m_MouseX-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapWidth()/2)) * (SLONG)pDoc->GetTileWidth());
		CameraPosition.z = -(float)(( ((SLONG)(m_MouseY-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapHeight()/2)) * (SLONG)pDoc->GetTileHeight());
		if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//			CameraPosition.y = pDoc->GetHeightAt(CameraPosition)+CAM_HEIGHT;
			CameraPosition.y = (float)CAM_HEIGHT;
		}

		SyncPosition(CameraPosition);
		UpdateAndValidate();

		if(pDoc->GetAutoSync()) {
			pDoc->RegisterCamera(CameraRotation,CameraPosition);	// Bind views.
			pDoc->Invalidate3D();
		}

		return FALSE;
	}

	if(pDoc->InLocatorBorder(m_MouseX,m_MouseY)) {
		return FALSE;
	}

	BOOL DoneSomething = FALSE;

	switch(pDoc->GetEditTool()) {
		case ET_ROTATETILE:
   			Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
				
				DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
				Rotate++;
				Rotate &= 3;
				pDoc->GetHeightMap()->SetTextureRotate(Selected,Rotate);
				UpdateAndValidate();
				DoneSomething = TRUE;
  			}
			break;

		case ET_XFLIPTILE:
   			Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
				
				BOOL FlipX = pDoc->GetHeightMap()->GetTextureFlipX(Selected);
				BOOL FlipY = pDoc->GetHeightMap()->GetTextureFlipY(Selected);
				DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
				if(Rotate & 1) {
					pDoc->GetHeightMap()->SetTextureFlip(Selected,FlipX,!FlipY);
				} else {
					pDoc->GetHeightMap()->SetTextureFlip(Selected,!FlipX,FlipY);
				}
				UpdateAndValidate();
				DoneSomething = TRUE;
  			}
			break;

		case ET_YFLIPTILE:
   			Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
   			if(Selected >= 0) {

				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
				
				BOOL FlipX = pDoc->GetHeightMap()->GetTextureFlipX(Selected);
				BOOL FlipY = pDoc->GetHeightMap()->GetTextureFlipY(Selected);
				DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
				if(Rotate & 1) {
					pDoc->GetHeightMap()->SetTextureFlip(Selected,!FlipX,FlipY);
				} else {
					pDoc->GetHeightMap()->SetTextureFlip(Selected,FlipX,!FlipY);
				}
				UpdateAndValidate();
				DoneSomething = TRUE;
  			}
			break;

		case ET_GATEWAY:
   			Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
 			if(Selected >= 0) {
//				DebugPrint("%d,(%d %d)\n",Selected,Selected%pDoc->GetMapWidth(),Selected/pDoc->GetMapHeight());
				if(GateMode == GATE_POSEND) {
					GateX1 = Selected%pDoc->GetMapWidth();
					GateY1 = Selected/pDoc->GetMapWidth();

					if(pDoc->GetHeightMap()->CheckGatewayOverlap(GateIndex,GateX0,GateY0,GateX1,GateY1) == FALSE) {
						pDoc->GetHeightMap()->DeleteGateway(GateIndex);
						MessageBox( "Can't overlap gateways", "Error.", MB_OK );
					} else if((GateX0 == GateX1) && (GateY0 == GateY1)) {
						pDoc->GetHeightMap()->DeleteGateway(GateIndex);
						MessageBox( "Can't have gateways of length 1", "Error.", MB_OK );
					} else if(pDoc->GetHeightMap()->CheckGatewayBlockingTiles(GateIndex) == FALSE) {
						pDoc->GetHeightMap()->DeleteGateway(GateIndex);
						MessageBox( "Can't put it there", "Error.", MB_OK );
					} else {
						pDoc->GetHeightMap()->SetGateway(GateIndex,GateX0,GateY0,GateX1,GateY1);
					}

					GateMode = GATE_POSSTART;
					DoneSomething = TRUE;
					DebugPrint("End %d,(%d %d)\n",Selected,GateX1,GateY1);
					UpdateAndValidate();
				} else if(GateMode == GATE_POSSTART) {
					GateX0 = Selected%pDoc->GetMapWidth();
					GateY0 = Selected/pDoc->GetMapWidth();

					GateIndex = pDoc->GetHeightMap()->FindGateway(GateX0,GateY0);
					if(GateIndex >= 0) {
						pDoc->GetHeightMap()->DeleteGateway(GateIndex);
						UpdateAndValidate();
					} else {
						GateIndex = pDoc->GetHeightMap()->AddGateway(GateX0,GateY0,GateX0,GateY0);
						GateMode = GATE_POSEND;
						DoneSomething = TRUE;
						DebugPrint("Start %d,(%d %d)\n",Selected,GateX0,GateY0);
					}
				}
			}
			break;

		case ET_PAINT:
			if(!pDoc->m_SelIsTexture) {
				SLONG	wx = (SLONG)point.x;
				SLONG	wz = (SLONG)point.y;
				CPoint ScrollPos = GetScrollPosition();
				// Convert 2d view cursor position into world coordinate.
				ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
				ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;
				pDoc->ConvertCoord2d3d(wx,wz,(SLONG)ScrollPos.x,(SLONG)ScrollPos.y);

				D3DVECTOR Rotation = {0.0F, 0.0F, 0.0F};
				D3DVECTOR Position = {0.0F, 0.0F, 0.0F};
				Position.x = (float)wx;
				Position.z = (float)-wz;
				Position.y = HeightMap->GetHeight(Position.x,-Position.z);

				DWORD ObjToAdd = pDoc->m_SelObject;
//				if(HeightMap->GetObjectType(ObjToAdd) == IMD_STRUCTURE) {
//					ObjToAdd += pDoc->GetCurrentPlayer();
//				}

				int ObjID = HeightMap->AddObject(ObjToAdd,Rotation,Position,pDoc->GetCurrentPlayer());
				HeightMap->SnapObject(ObjID);
				HeightMap->SetObjectTileFlags(ObjID,TF_HIDE);
				HeightMap->SetObjectTileHeights(ObjID);
				HeightMap->InitialiseSectors();

				pDoc->Invalidate3D();
				UpdateAndValidate();
				DoneSomething = TRUE;
			}
			break;

		case ET_PASTERECT:
			if( pDoc->ClipboardIsValid() ) {
				pDoc->PasteTileRect(HeightMap->GetSelectionBox0());
				pDoc->BuildRadarMap();
				pDoc->GetHeightMap()->InitialiseSectors();
				UpdateAndValidate();
				pDoc->Invalidate3D();
			}
			break;

		case ET_POINT:
			{
				HeightMap->DeSelectAll3DObjects();
				CPoint SPos = GetScrollPosition();
				int ObjID = HeightMap->FindObjectHit2D(SPos.x,SPos.y,point.x,point.y);
				if(ObjID >= 0) {
					HeightMap->Select3DObject(ObjID);
				}
				pDoc->Invalidate3D();
				UpdateAndValidate();
			}
			break;

		case ET_MOVE:
			{
				if(m_DragMode!=DM_DRAGOBJECT) {
					HeightMap->DeSelectAll3DObjects();
					CPoint SPos = GetScrollPosition();
					m_ObjectID = HeightMap->FindObjectHit2D(SPos.x,SPos.y,point.x,point.y);
					if(m_ObjectID >= 0) {
						HeightMap->Select3DObject(m_ObjectID);
						m_DragMode = DM_DRAGOBJECT;
						DebugPrint("StartDrag\n");
					}
					HeightMap->SetObjectTileFlags(TF_SHOW);
					pDoc->Invalidate3D();
					UpdateAndValidate();
				}
			}
			break;

		case ET_ROTATEOBJECT:
			{
				HeightMap->DeSelectAll3DObjects();
				CPoint SPos = GetScrollPosition();
				m_ObjectID = HeightMap->FindObjectHit2D(SPos.x,SPos.y,point.x,point.y);
				if(m_ObjectID >= 0) {
					if(HeightMap->GetObjectInstanceFlanged(m_ObjectID)) {

						HeightMap->Select3DObject(m_ObjectID);

						D3DVECTOR Rotation;
						HeightMap->Get3DObjectRotation(m_ObjectID,Rotation);
						Rotation.y += 90;
						if(Rotation.y >= 360.0F) {
							Rotation.y -= 360.0F;
						}
						HeightMap->Set3DObjectRotation(m_ObjectID,Rotation);
						pDoc->Invalidate3D();
						UpdateAndValidate();
					} else {
						if(m_DragMode!=DM_DRAGOBJECT) {
							HeightMap->Select3DObject(m_ObjectID);
							HeightMap->SetObjectTileFlags(TF_SHOW);
							m_DragMode = DM_DRAGOBJECT;
							pDoc->Invalidate3D();
							UpdateAndValidate();
							DebugPrint("StartDrag\n");
						}
					}
				}
			}
			break;
	}

	return DoneSomething;
}


//void CWFView::ApplyRandomness(int Selected)
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


void CWFView::ApplyTool(CPoint point,BOOL JustPressed)
{
	static SDWORD LastSelected = -1;
	static SDWORD LastSelTexture = -1;
	static SDWORD LastSelType = -1;

	CBTEditDoc* pDoc = GetDocument();
	D3DVECTOR CameraPosition,CameraRotation;
	CPoint ScrollPos = GetScrollPosition();
	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;

	int Selected;

	CHeightMap *HeightMap = pDoc->GetHeightMap();

	if(pDoc->InLocator(m_MouseX,m_MouseY)) {
		pDoc->GetCamera(CameraRotation,CameraPosition);
		CameraPosition.x = (float)(( ((SLONG)(m_MouseX-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapWidth()/2)) * (SLONG)pDoc->GetTileWidth());
		CameraPosition.z = -(float)(( ((SLONG)(m_MouseY-OVERSCAN)/pDoc->GetRadarScale()) -((SLONG)pDoc->GetMapHeight()/2)) * (SLONG)pDoc->GetTileHeight());
		if(pDoc->GetCameraMode() == MCAM_ISOMETRIC) {
//			CameraPosition.y = pDoc->GetHeightAt(CameraPosition)+CAM_HEIGHT;
			CameraPosition.y = (float)CAM_HEIGHT;
		}
		SyncPosition(CameraPosition);
		UpdateAndValidate();

		if(pDoc->GetAutoSync()) {
			pDoc->RegisterCamera(CameraRotation,CameraPosition);	// Bind views.
			pDoc->Invalidate3D();
		}

		return;
	}

	if(pDoc->InLocatorBorder(m_MouseX,m_MouseY)) {
		return;
	}

	Selected = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);

	DWORD EditTool = pDoc->GetEditTool();

	if((EditTool >= ET_TYPESFIRST) && (EditTool <= ET_TYPESLAST)) {
		if(Selected >= 0) {
			if( (Selected != LastSelected) ||
				((SDWORD)pDoc->m_SelType  != LastSelType) ) {

				LastSelected = Selected;
				LastSelTexture = -1;
				LastSelType = pDoc->m_SelType;

  				g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));

				HeightMap->SetTileType(Selected,pDoc->GetSelectedType());
				UpdateAndValidate();
			}
		}
	} else {
		switch(EditTool) {
			case ET_GET:
				if(Selected >= 0) {
					pDoc->m_SelTexture = HeightMap->GetTextureID(Selected);
					pDoc->GetTextureSelector()->SetSelectedTexture(pDoc->m_SelTexture);
					pDoc->UpdateTextureView();
				}
				break;

			case ET_PAINT:
   				if(Selected >= 0) {
					AutoScroll(point);
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
								HeightMap->SetTextureID(Selected,pDoc->m_SelTexture);
								HeightMap->SetTileType(Selected,pDoc->m_SelType);

								HeightMap->ApplyRandomness(Selected,pDoc->m_SelFlags);

//								DWORD Flags = HeightMap->GetTileFlags(Selected);
//								Flags &= TF_GEOMETRYMASK | TF_TYPEMASK;
//								Flags |= pDoc->m_SelFlags;
//								HeightMap->SetTileFlags(Selected,Flags);

								if(pDoc->GetAutoHeight()) {
									HeightMap->SetTileHeightUndo(Selected,(float)HeightMap->GetSeaLevel());
									g_UndoRedo->EndGroup();
									m_HeightsChanged = TRUE;
								}
								pDoc->Invalidate3D();
							} else {
								g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
								HeightMap->SetTextureID(Selected,pDoc->m_SelTexture);
								HeightMap->SetTileType(Selected,pDoc->m_SelType);

								HeightMap->ApplyRandomness(Selected,pDoc->m_SelFlags);

//								DWORD Flags = HeightMap->GetTileFlags(Selected);
//								Flags &= TF_GEOMETRYMASK | TF_TYPEMASK;
//								Flags |= pDoc->m_SelFlags;
//								HeightMap->SetTileFlags(Selected,Flags);

								pDoc->RedrawFace(Selected);
							}
							UpdateAndValidate();
						}
					}
  				}
				break;

			case ET_EDGEPAINT:
				if(Selected >= 0) {
					AutoScroll(point);
					if(Selected != LastSelected) {
						LastSelected = Selected;

						int mw = pDoc->GetMapWidth();
						int y = Selected / mw;
						int x = Selected - (y*mw);
						if(pDoc->GetEdgeBrush()->Paint(x,y,pDoc->GetAutoHeight())) {
							m_HeightsChanged = TRUE;
						}

						UpdateAndValidate();
						pDoc->Invalidate3D();
					}
				}
				break;

			case ET_EDGEFILL:
   				if(Selected >= 0) {
					pDoc->GetEdgeBrush()->FillMap(Selected,pDoc->m_SelTexture,pDoc->m_SelType,pDoc->m_SelFlags);
					m_HeightsChanged = TRUE;
					UpdateAndValidate();
					pDoc->Invalidate3D();
				}
				break;

			case ET_BRUSHPAINT:
				break;

			case ET_HIDETILE:
				if(Selected >= 0) {
					if(Selected != LastSelected) {
						LastSelected = Selected;

						g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
						AutoScroll(point);
   						pDoc->GetHeightMap()->SetTileVisible(Selected,TF_HIDE);
						UpdateAndValidate();
						pDoc->Invalidate3D();
					}
  				}
				break;

			case ET_SHOWTILE:
   				if(Selected >= 0) {
					if(Selected != LastSelected) {
						LastSelected = Selected;

						g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
						AutoScroll(point);
   						pDoc->GetHeightMap()->SetTileVisible(Selected,TF_SHOW);
						UpdateAndValidate();
						pDoc->Invalidate3D();
					}
  				}
				break;

			case ET_FILL:
   				if(Selected >= 0) {
					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
					if(pDoc->m_SelIsTexture) {
   						pDoc->FillMap(Selected,pDoc->m_SelTexture,pDoc->m_SelType,pDoc->m_SelFlags);
						UpdateAndValidate();
						pDoc->Invalidate3D();
					}
   				}
				break;

			case ET_COPYRECT:
				switch(CopyRectCorners) {
					case	0:
					case	2:
		   				CopyRect0 = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
						CopyRectCorners=1;
						break;
					case	1:
						AutoScroll(point);
		   				CopyRect1 = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
						pDoc->Update2DView(-1,CopyRect0,CopyRect1);
						ValidateRect(NULL);
						break;
				}
				break;

			case ET_MARKRECT:
				switch(MarkRectCorners) {
					case	0:
					case	2:
		   				MarkRect0 = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
						HeightMap->SetSelectionBox0(MarkRect0);
						MarkRectCorners=1;
						break;
					case	1:
						AutoScroll(point);
		   				MarkRect1 = pDoc->Select2DFace(point.x,point.y,ScrollPos.x,ScrollPos.y);
						HeightMap->SetSelectionBox1(MarkRect1);
						UpdateAndValidate();
//						pDoc->Update2DView(-1,MarkRect0,MarkRect1);
//						ValidateRect(NULL);
						break;
				}
				break;

//			case ET_PASTERECT:
//				if(Selected != LastSelected) {
//					LastSelected = Selected;
//
//					AutoScroll(point);
//					pDoc->PasteTileRect(Selected);
//					m_HeightsChanged = TRUE;
//					UpdateAndValidate();
//  					pDoc->Invalidate3D();
//				}
//				break;
		}
	}
}




void CWFView::AutoScroll(CPoint &point)
{
	CBTEditDoc* pDoc = GetDocument();
	RECT ClientRect;
	GetClientRect(&ClientRect);

	SWORD TWidth = (SWORD)pDoc->GetTextureWidth();
	SWORD THeight = (SWORD)pDoc->GetTextureHeight();

	if(point.x > ClientRect.right - TWidth) {
		RECT WindowRect;
		GetWindowRect(&WindowRect);
		m_ScrollX += pDoc->GetTextureWidth();
		UpdateScrollPosition();
		SetCursorPos(WindowRect.left+point.x-TWidth,WindowRect.top+point.y);
	}

	if(point.y > ClientRect.bottom - THeight) {
		RECT WindowRect;
		GetWindowRect(&WindowRect);
		m_ScrollY += pDoc->GetTextureHeight();
		UpdateScrollPosition();
		SetCursorPos(WindowRect.left+point.x,WindowRect.top+point.y-THeight);
	}

	if(point.x < TWidth) {
		RECT WindowRect;
		GetWindowRect(&WindowRect);
		m_ScrollX -= pDoc->GetTextureWidth();
		UpdateScrollPosition();
		SetCursorPos(WindowRect.left+point.x+TWidth,WindowRect.top+point.y);
	}

	if(point.y < THeight) {
		RECT WindowRect;
		GetWindowRect(&WindowRect);
		m_ScrollY -= pDoc->GetTextureHeight();
		UpdateScrollPosition();
		SetCursorPos(WindowRect.left+point.x,WindowRect.top+point.y+THeight);
	}
}


void CWFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CBTEditDoc* pDoc = GetDocument();

	if(m_DragMode == DM_DRAGOBJECT) {
		m_DragMode = DM_NODRAG;
		pDoc->GetHeightMap()->SetObjectTileFlags(TF_HIDE);
		pDoc->Invalidate3D();
		UpdateAndValidate();
		DebugPrint("StopDrag\n");
	} else {
		if(m_HeightsChanged) {
			pDoc->BuildRadarMap();
			pDoc->GetHeightMap()->InitialiseSectors();
			UpdateAndValidate();
			pDoc->Invalidate3D();
		}

		switch(pDoc->GetEditTool()) {
			case ET_COPYRECT:
				if(CopyRectCorners) {
					pDoc->CopyTileRect(CopyRect0,CopyRect1);
					UpdateAndValidate();
					CopyRectCorners=2;
				}
				break;

			case ET_MARKRECT:
				if(MarkRectCorners) {
//					pDoc->YFlipTileRect(MarkRect0,MarkRect1);
					UpdateAndValidate();
					pDoc->Invalidate3D();
					MarkRectCorners=0;
				}
				break;

//			case ET_PASTERECT:
//				pDoc->BuildRadarMap();
//				pDoc->GetHeightMap()->InitialiseSectors();
//				UpdateAndValidate();
//				pDoc->Invalidate3D();
//				break;
		}
	}

	CScrollView::OnLButtonUp(nFlags, point);
}


void CWFView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CBTEditDoc* pDoc = GetDocument();
	int	MinPos,MaxPos;
	SCROLLINFO ScrollInfo;

	CPoint ScrollPos = GetScrollPosition();
	m_ScrollX = ScrollPos.x;
	m_ScrollY = ScrollPos.y;
//	pDoc->GetScrollPosition((SLONG*)&m_ScrollX,(SLONG*)&m_ScrollY);

	GetScrollInfo( SB_HORZ, &ScrollInfo, SIF_ALL );
	MinPos = ScrollInfo.nMin;
	MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;

	switch(nSBCode) {
		case	SB_THUMBPOSITION:
		case	SB_THUMBTRACK:
			m_ScrollX = nPos;
			break;
		case	SB_LINELEFT:
			m_ScrollX -= pDoc->GetTextureWidth();
			if(m_ScrollX < MinPos) m_ScrollX = MinPos;
			break;
		case	SB_LINERIGHT:
			m_ScrollX += pDoc->GetTextureWidth();
			if(m_ScrollX > MaxPos) m_ScrollX = MaxPos;
			break;
		case	SB_PAGELEFT:
			m_ScrollX -= ScrollInfo.nPage;
			if(m_ScrollX < MinPos) m_ScrollX = MinPos;
			break;
		case	SB_PAGERIGHT:
			m_ScrollX += ScrollInfo.nPage;
			if(m_ScrollX > MaxPos) m_ScrollX = MaxPos;
			break;
	}

	m_ScrollX /= pDoc->GetTextureWidth();
	m_ScrollX *= pDoc->GetTextureWidth();

	pDoc->RegisterScrollPosition(m_ScrollX-pDoc->GetTextureWidth()*OVERSCAN,m_ScrollY-pDoc->GetTextureHeight()*OVERSCAN);
  	UpdateAndValidate();

	if(pDoc->Get2DMode() == M2D_WORLD) {
		if(pDoc->GetAutoSync()) {
			pDoc->Set3DViewPos();	// Bind views.
  			pDoc->Invalidate3D();
		}
	}


	SetScrollPos( SB_HORZ, m_ScrollX, TRUE );
}

void CWFView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CBTEditDoc* pDoc = GetDocument();
	int	MinPos,MaxPos;
	SCROLLINFO ScrollInfo;

	CPoint ScrollPos = GetScrollPosition();
	m_ScrollX = ScrollPos.x;
	m_ScrollY = ScrollPos.y;
//	pDoc->GetScrollPosition((SLONG*)&m_ScrollX,(SLONG*)&m_ScrollY);

	GetScrollInfo( SB_VERT, &ScrollInfo, SIF_ALL );
	MinPos = ScrollInfo.nMin;
	MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;

	switch(nSBCode) {
		case	SB_THUMBPOSITION:
		case	SB_THUMBTRACK:
			m_ScrollY = nPos;
			break;
		case	SB_LINELEFT:
			m_ScrollY -= pDoc->GetTextureHeight();
			if(m_ScrollY < MinPos) m_ScrollY = MinPos;
			break;
		case	SB_LINERIGHT:
			m_ScrollY += pDoc->GetTextureHeight();
			if(m_ScrollY > MaxPos) m_ScrollY = MaxPos;
			break;
		case	SB_PAGELEFT:
			m_ScrollY -= ScrollInfo.nPage;
			if(m_ScrollY < MinPos) m_ScrollY = MinPos;
			break;
		case	SB_PAGERIGHT:
			m_ScrollY += ScrollInfo.nPage;
			if(m_ScrollY > MaxPos) m_ScrollY = MaxPos;
			break;
	}
	
	m_ScrollY /= pDoc->GetTextureHeight();
	m_ScrollY *= pDoc->GetTextureHeight();

	pDoc->RegisterScrollPosition(m_ScrollX-pDoc->GetTextureWidth()*OVERSCAN,m_ScrollY-pDoc->GetTextureHeight()*OVERSCAN);
  	UpdateAndValidate();

	if(pDoc->Get2DMode() == M2D_WORLD) {
		if(pDoc->GetAutoSync()) {
			pDoc->Set3DViewPos();	// Bind views.
  			pDoc->Invalidate3D();
		}
	}

	SetScrollPos( SB_VERT, m_ScrollY, TRUE );
}


void CWFView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_KeyHandler->HandleKeyDown(nChar);

	CBTEditDoc* pDoc = GetDocument();

	if(pDoc->Get2DMode() == M2D_WORLD) {
		BOOL Changed = FALSE;	
// Adjust x position.
		if(m_KeyHandler->GetKeyState(VK_LEFT)) {
			m_ScrollX-=pDoc->GetTextureWidth();
			Changed = TRUE;
		} else {
			if(m_KeyHandler->GetKeyState(VK_RIGHT)) {
				m_ScrollX+=pDoc->GetTextureWidth();
				Changed = TRUE;
			}
		}

// Adjust y position
		if(m_KeyHandler->GetKeyState(VK_UP)) {
			m_ScrollY-=pDoc->GetTextureHeight();
			Changed = TRUE;
		} else {
			if(m_KeyHandler->GetKeyState(VK_DOWN)) {
				m_ScrollY+=pDoc->GetTextureHeight();
				Changed = TRUE;
			}
		}

// If things have changed then update views.
		if(Changed) {
			UpdateScrollPosition();
		}

		D3DVECTOR	CameraPos,CameraRot;
		CBTEditDoc* pDoc = GetDocument();
		CHeightMap* HeightMap = pDoc->GetHeightMap();
		int Selected;
		CPoint ScrollPos = GetScrollPosition();
		ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
		ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;


		switch(nChar) {
			case	'S':		// Sync view positions.
				pDoc->GetCamera(CameraRot,CameraPos);
				SyncPosition(CameraPos);
				break;

			case	VK_DELETE:
				HeightMap->SetObjectTileFlags(TF_SHOW);
				HeightMap->DeleteSelected3DObjects();
				HeightMap->SetObjectTileFlags(TF_HIDE);
  				UpdateAndValidate();
  				pDoc->Invalidate3D();
				break;

			case	KEY_ROTATETILE:
   				Selected = pDoc->Select2DFace(m_MouseX,m_MouseY,ScrollPos.x,ScrollPos.y);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
					
					DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
					Rotate++;
					Rotate &= 3;
					pDoc->GetHeightMap()->SetTextureRotate(Selected,Rotate);
					UpdateAndValidate();
  				}
				break;

			case	KEY_ROTATETILE2:
   				Selected = pDoc->Select2DFace(m_MouseX,m_MouseY,ScrollPos.x,ScrollPos.y);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
					
					DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
					Rotate--;
					Rotate &= 3;
					pDoc->GetHeightMap()->SetTextureRotate(Selected,Rotate);
					UpdateAndValidate();
  				}
				break;

			case	KEY_XFLIPTILE:
   				Selected = pDoc->Select2DFace(m_MouseX,m_MouseY,ScrollPos.x,ScrollPos.y);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
					
					BOOL FlipX = pDoc->GetHeightMap()->GetTextureFlipX(Selected);
					BOOL FlipY = pDoc->GetHeightMap()->GetTextureFlipY(Selected);
					DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
					if(Rotate & 1) {
						pDoc->GetHeightMap()->SetTextureFlip(Selected,FlipX,!FlipY);
					} else {
						pDoc->GetHeightMap()->SetTextureFlip(Selected,!FlipX,FlipY);
					}
					UpdateAndValidate();
  				}
				break;

			case	KEY_YFLIPTILE:
   				Selected = pDoc->Select2DFace(m_MouseX,m_MouseY,ScrollPos.x,ScrollPos.y);
   				if(Selected >= 0) {

					g_UndoRedo->AddUndo(&(HeightMap->GetMapTiles()[Selected]));
					
					BOOL FlipX = pDoc->GetHeightMap()->GetTextureFlipX(Selected);
					BOOL FlipY = pDoc->GetHeightMap()->GetTextureFlipY(Selected);
					DWORD Rotate = pDoc->GetHeightMap()->GetTextureRotate(Selected);
					if(Rotate & 1) {
						pDoc->GetHeightMap()->SetTextureFlip(Selected,!FlipX,FlipY);
					} else {
						pDoc->GetHeightMap()->SetTextureFlip(Selected,FlipX,!FlipY);
					}
					UpdateAndValidate();
  				}
				break;
		}
	}


	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CWFView::UpdateScrollPosition(void)
{
	CBTEditDoc* pDoc = GetDocument();

	SCROLLINFO ScrollInfo;
	GetScrollInfo( SB_HORZ, &ScrollInfo, SIF_ALL );
	int MinPos = ScrollInfo.nMin;
	int MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;
	if(m_ScrollX < MinPos) m_ScrollX = MinPos;
	if(m_ScrollX > MaxPos) m_ScrollX = MaxPos;

	GetScrollInfo( SB_VERT, &ScrollInfo, SIF_ALL );
	MinPos = ScrollInfo.nMin;
	MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;
	if(m_ScrollY < MinPos) m_ScrollY = MinPos;
	if(m_ScrollY > MaxPos) m_ScrollY = MaxPos;

	SetScrollPos( SB_HORZ, m_ScrollX, TRUE );
	SetScrollPos( SB_VERT, m_ScrollY, TRUE );
	pDoc->RegisterScrollPosition(m_ScrollX-pDoc->GetTextureWidth()*OVERSCAN,m_ScrollY-pDoc->GetTextureHeight()*OVERSCAN);

	UpdateAndValidate();

	if(pDoc->GetAutoSync()) {
		pDoc->Set3DViewPos();
		pDoc->Invalidate3D();
	}
}

void CWFView::SyncPosition(D3DVECTOR &CameraPos)
{
	CBTEditDoc* pDoc = GetDocument();

	SLONG TextureWidth = (SLONG)pDoc->GetTextureWidth();
	SLONG TextureHeight = (SLONG)pDoc->GetTextureHeight();
	SLONG TileWidth = (SLONG)pDoc->GetTileWidth();
	SLONG TileHeight = (SLONG)pDoc->GetTileHeight();
	SLONG MapWidthO2 = (SLONG)pDoc->GetMapWidth()/2;
	SLONG MapHeightO2 = (SLONG)pDoc->GetMapHeight()/2;

	RECT ClientRect;
	GetClientRect(&ClientRect);

	m_ScrollX = ( ( ((SLONG)CameraPos.x)/TileWidth)+MapWidthO2 -
				((SLONG)ClientRect.right/2/TextureWidth) )*TextureWidth;

	m_ScrollY = ( ( (-(SLONG)CameraPos.z)/TileHeight)+MapHeightO2 -
				((SLONG)ClientRect.bottom/2/TextureHeight) )*TextureHeight;

	SCROLLINFO ScrollInfo;
	GetScrollInfo( SB_HORZ, &ScrollInfo, SIF_ALL );
	int MinPos = ScrollInfo.nMin;
	int MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;
	if(m_ScrollX < MinPos) m_ScrollX = MinPos;
	if(m_ScrollX > MaxPos) m_ScrollX = MaxPos;

	GetScrollInfo( SB_VERT, &ScrollInfo, SIF_ALL );
	MinPos = ScrollInfo.nMin;
	MaxPos = ScrollInfo.nMax-ScrollInfo.nPage;
	if(m_ScrollY < MinPos) m_ScrollY = MinPos;
	if(m_ScrollY > MaxPos) m_ScrollY = MaxPos;

	SetScrollPos( SB_HORZ, m_ScrollX, TRUE );
	SetScrollPos( SB_VERT, m_ScrollY, TRUE );
	pDoc->RegisterScrollPosition(m_ScrollX-pDoc->GetTextureWidth()*OVERSCAN,m_ScrollY-pDoc->GetTextureHeight()*OVERSCAN);
	InvalidateRect(NULL,NULL);
}

BOOL CWFView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
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

//// Place an object on landscape.
////
//void CWFView::PlaceObject(CPoint &point)
//{
//	SLONG	wx = (SLONG)point.x;
//	SLONG	wz = (SLONG)point.y;
//
//	CBTEditDoc* pDoc = GetDocument();
//
//	CPoint ScrollPos = GetScrollPosition();
//	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
//	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;
//
//// Get currently selected object.
//	CObjectDB* ObjectDB=pDoc->GetSelectedObject();
//// Convert 2d view cursor position into world coordinate.
//	pDoc->ConvertCoord2d3d(wx,wz,(SLONG)ScrollPos.x,(SLONG)ScrollPos.y);
//// and postion it.
//	if(ObjectDB != NULL) {
//		D3DVECTOR Position;
//		D3DVECTOR Rotation;
//
//		Position.x = (float)wx;
//		Position.z = (float)-wz;
//
//		if(ObjectDB->GetSnapFlags() == SNAP_NONE) {
//			pDoc->DoGridSnap(Position);
//		} else {
//			pDoc->DoGridSnap(Position,ObjectDB->GetSnapFlags());
//		}
//		
//		Position.y = pDoc->GetHeightAt(Position);
//
//		Rotation.x = Rotation.y = Rotation.z = 0.0F;
//		ObjectDB->Add3DObject(Position,Rotation);
//		pDoc->BuildIDList();
//  		pDoc->Invalidate3D();
//  		UpdateAndValidate();
////		DBView->Refresh();
//	}
//}

// Position any selected objects.
//
void CWFView::PositionSelectedObjects(CPoint &point)
{
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap* HeightMap = pDoc->GetHeightMap();

	SLONG	wx = (SLONG)point.x;
	SLONG	wz = (SLONG)point.y;
	CPoint ScrollPos = GetScrollPosition();

// Convert 2d view cursor position into world coordinate.
	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;
	pDoc->ConvertCoord2d3d(wx,wz,(SLONG)ScrollPos.x,(SLONG)ScrollPos.y);

	D3DVECTOR Position = {0.0F, 0.0F, 0.0F};
	Position.x = (float)wx;
	Position.y = 0.0F;
	Position.z = (float)-wz;

	HeightMap->Set3DObjectPosition(m_ObjectID,Position);
	HeightMap->SnapObject(m_ObjectID);

  	UpdateAndValidate();
  	pDoc->Invalidate3D();
}

// Select an object on the landscape
//
//void CWFView::SelectObject(CPoint &Point)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	ListNode<CObjectDB>* TmpNode = pDoc->GetObjectDatabase();
//	while(TmpNode!=NULL) {
//		CObjectDB* ObjectDB = TmpNode->GetData();
//
//		DWORD Instances = ObjectDB->GetInstances();
//		for(DWORD i=0; i<Instances; i++) {
//			D3DVECTOR Position;
//			D3DVECTOR Smallest,Largest;
//
//			GetObjectExtents(ObjectDB,i,Position,Smallest,Largest);
//
//			if( (Point.x > Smallest.x) &&
//				(Point.x < Largest.x) &&
//				(Point.y > Smallest.z) &&
//				(Point.y < Largest.z) ) {
//
//				ObjectDB->Set3DObjectSelected(i);
//
//				CBTEditDoc* pDoc = GetDocument();
//				pDoc->SetStatsView(ObjectDB,i);
//				
//				
//				pDoc->Invalidate3D();
//				UpdateAndValidate();
//				return;
//			}
//		
//		}
//
//		TmpNode = TmpNode->GetNextNode();
//	}
//}

// De-Select an object on the landscape
//
//void CWFView::DeSelectObject(CPoint &Point)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	ListNode<CObjectDB>* TmpNode = pDoc->GetObjectDatabase();
//	while(TmpNode!=NULL) {
//		CObjectDB* ObjectDB = TmpNode->GetData();
//
//		DWORD Instances = ObjectDB->GetInstances();
//		for(DWORD i=0; i<Instances; i++) {
//			D3DVECTOR Position;
//			D3DVECTOR Smallest,Largest;
//
//			GetObjectExtents(ObjectDB,i,Position,Smallest,Largest);
//
//			if( (Point.x > Smallest.x) &&
//				(Point.x < Largest.x) &&
//				(Point.y > Smallest.z) &&
//				(Point.y < Largest.z) ) {
//
//				ObjectDB->Clear3DObjectSelected(i);
//				pDoc->Invalidate3D();
//				UpdateAndValidate();
//				return;
//			}
//		
//		}
//
//		TmpNode = TmpNode->GetNextNode();
//	}
//}

// Get an objects position and bounding box.
//
//void CWFView::GetObjectExtents(CObjectDB* ObjectDB,DWORD Instance,D3DVECTOR &Position,D3DVECTOR &Smallest,D3DVECTOR &Largest)
//{
//	CBTEditDoc* pDoc = GetDocument();
//
//	SLONG TextureWidth = (SLONG)pDoc->GetTextureWidth();
//	SLONG TextureHeight = (SLONG)pDoc->GetTextureHeight();
//	SLONG TileWidth = (SLONG)pDoc->GetTileWidth();
//	SLONG TileHeight = (SLONG)pDoc->GetTileHeight();
//	SLONG MapWidth = (SLONG)pDoc->GetMapWidth();
//	SLONG MapHeight = (SLONG)pDoc->GetMapHeight();
//
//	CPoint ScrollPos = GetScrollPosition();
//	ScrollPos.x -= pDoc->GetTextureWidth()*OVERSCAN;
//	ScrollPos.y -= pDoc->GetTextureHeight()*OVERSCAN;
//
//
//	ScrollPos.x/=TextureWidth;
//	ScrollPos.x*=TextureWidth;
//	ScrollPos.y/=TextureHeight;
//	ScrollPos.y*=TextureHeight;
//
//	ObjectDB->Get3DObjectPosition(Instance,Position);
//	Position.z = -Position.z;
//	Position.x += (float)(MapWidth*TileWidth)/2;
//	Position.x /= (float)(TileWidth/TextureWidth);
//	Position.z += (float)(MapHeight*TileHeight)/2;
//	Position.z /= (float)(TileHeight/TextureHeight);
//	Position.x -= (float)ScrollPos.x;
//	Position.z -= (float)ScrollPos.y;
//
//	ObjectDB->Get3DObject()->GetBoundingBox(Smallest,Largest);
//
//	Smallest.x /= (float)(TileWidth/TextureWidth);
//	Smallest.z /= (float)(TileHeight/TextureHeight);
//	Smallest.x = Smallest.x + Position.x;
//	Smallest.z = Smallest.z + Position.z;
//	Largest.x /= (float)(TileWidth/TextureWidth);
//	Largest.z /= (float)(TileHeight/TextureHeight);
//	Largest.x = Largest.x + Position.x;
//	Largest.z = Largest.z + Position.z;
//}


void CWFView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CBTEditDoc* pDoc = GetDocument();
	CHeightMap *HeightMap = pDoc->GetHeightMap();
	RECT WindowRect;

	CPoint SPos = GetScrollPosition();

	// Make click point relative to window top left.
	GetWindowRect(&WindowRect);
	int ObjID = HeightMap->FindObjectHit2D(SPos.x,SPos.y,
											point.x-WindowRect.left,
											point.y-WindowRect.top);

	// If found an object then
   	if(ObjID >= 0) {
		// Select it
		HeightMap->DeSelectAll3DObjects();
   		HeightMap->Select3DObject(ObjID);
   		pDoc->Invalidate3D();
		UpdateAndValidate();

		// And bring up it's context menu.
   		CMenu Menu;
   		Menu.LoadMenu(IDR_2DPOPUP);
   		Menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
   							point.x,point.y,this);
   		m_CurrentObjectID = ObjID;
   	}
}

//void CWFView::On2dpopupEditstats() 
//{
//	DebugPrint("Popup\n");
//	//	EditStats();
//}

void CWFView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CBTEditDoc* pDoc = GetDocument();

//	pDoc->DeSelectAllObjects();
//	SelectObject(point);
	
	CScrollView::OnRButtonDown(nFlags, point);
}

// Edit selected objects stats.
//
//void CWFView::EditStats(void)
//{
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
//}


void CWFView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	m_KeyHandler->HandleKeyUp(nChar);
	
	CScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CWFView::UpdateAndValidate(void)
{
	CBTEditDoc* pDoc = GetDocument();
	pDoc->Update2DView();
	ValidateRect(NULL);
}

void CWFView::OnSetFocus(CWnd* pOldWnd) 
{
	CScrollView::OnSetFocus(pOldWnd);

	CBTEditDoc* pDoc = GetDocument();
	pDoc->Set2DFocus(TRUE);
	pDoc->Invalidate3D();
	InvalidateRect(NULL,FALSE);
	m_GotFocus = TRUE;
}

void CWFView::OnKillFocus(CWnd* pNewWnd) 
{
	CScrollView::OnKillFocus(pNewWnd);

	CBTEditDoc* pDoc = GetDocument();
	pDoc->Set2DFocus(FALSE);
	InvalidateRect(NULL,FALSE);
	m_GotFocus = FALSE;
}

int CWFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_Font.CreateFont(10,0,0,0,FW_NORMAL,0,0,0,
				DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif");
	SetFont( &m_Font,FALSE );
	
	return 0;
}

void CWFView::On2dpopupProperties() 
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
//			HeightMap->SetObjectInstanceScriptName(m_CurrentObjectID,NewName);
//
//			MessageBox("Spaces removed from script name","Warning",MB_OK);
//		} else {
//			HeightMap->SetObjectInstanceScriptName(m_CurrentObjectID,Dlg.m_ScriptName.GetBuffer(0));
//		}
	}
}
