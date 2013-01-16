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
// BTEditDoc.cpp : implementation of the CBTEditDoc class
//

#include "stdafx.h"
#include "winapi.hpp"
#include "btedit.h"
#include "grdland.h"
#include "mainframe.h"
#include "bteditdoc.h"
#include "mapprefs.h"
#include "textureprefs.h"
#include "dibdraw.h"
#include "debugprint.hpp"
#include "wfview.h"
#include "textureview.h"
#include "bteditview.h"
#include "savesegmentdialog.hpp"
#include "limitsdialog.hpp"
#include "initiallimitsdialog.hpp"
#include "expandlimitsdlg.h"
#include "infodialog.hpp"
#include "playermap.h"
#include "gateway.hpp"
#include "pasteprefs.h"
#include "undoredo.h"

#include <string>

using std::string;

#define MAX_FILESTRING 512

extern string g_HomeDirectory;
extern string g_WorkDirectory;
extern BOOL g_OverlayTypes;
extern BOOL g_OverlayZoneIDs;

extern HCURSOR g_Wait;
extern HCURSOR g_Pointer;
extern HCURSOR g_PointerPaint;
extern HCURSOR g_PointerFill;
extern HCURSOR g_PointerDrag;
extern HCURSOR g_PointerSelRect;
extern HCURSOR g_PointerSelPoint;
extern HCURSOR g_PointerPipet;
extern HCURSOR g_PointerPliers;

CUndoRedo *g_UndoRedo = NULL;

CBTEditDoc *BTEditDoc = NULL;

Profile DesiredProfile = {
	FALSE,		// Fullscreen ?
//	640,480,	// Resolution.
	800,800,	// Resolution.
	16,			// Bit depth.
	FALSE,		// Triple buffer ?
	FALSE,		// Use hardware ?
	TRUE,		// Use z buffer ?
	RGBCOLOR,	// Colour model.
};

int g_MaxXRes = 800;	// 3d view size.
int g_MaxYRes = 600;

int g_CursorTileX;
int g_CursorTileZ;

const char ClipboardFilters[]="Clipboard Files (*.clp)|*.clp|All Files (*.*)|*.*||";
const char TileTypeFilters[]="Tile Types (*.ett)|*.ett|All Files (*.*)|*.*||";
const char EdgeBrushFilters[]="Edge Brushes (*.ebr)|*.ebr|All Files (*.*)|*.*||";
const char FeatureSetFilters[]="Data Sets (*.eds)|*.eds|All Files (*.*)|*.*||";
const char BitmapFilters[]="PCX Files (*.pcx)|*.pcx|BMP Files (*.bmp)|*.bmp|All Files (*.*)|*.*||";
const char BMPFilters[]="BMP Files (*.bmp)|*.bmp|All Files (*.*)|*.*||";
const char ObjectFilters[]="Object Files (*.mof)|*.mof|All Files (*.*)|*.*||";
const char LandscapeFilters[]="Landscape Files (*.lnd)|*.lnd|All Files (*.*)|*.*||";
const char TemplateFilters[]="Template Files (*.stt)|*.stt|All Files (*.*)|*.*||";
const char MapFilters[]="Map Files (*.map)|*.map|All Files (*.*)|*.*||";
const char FeatureFilters[]="Feature Files (*.bjo)|*.bjo|All Files (*.*)|*.*||";
const char WarzoneFilters[]="Warzone Scenario Files (*.gam)|*.gam|All Files (*.*)|*.*||";

void ReadStatsString(FILE* Stream,char** String);
void WriteStatsString(FILE* Stream,char* String);
void UpperCase(char *String);
void LowerCase(char *String);

/////////////////////////////////////////////////////////////////////////////
// CBTEditDoc

IMPLEMENT_DYNCREATE(CBTEditDoc, CDocument)

BEGIN_MESSAGE_MAP(CBTEditDoc, CDocument)
	//{{AFX_MSG_MAP(CBTEditDoc)
	ON_COMMAND(ID_MAP_SETMAPSIZE, OnMapSetmapsize)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_MAP_LOADHEIGHTMAP, OnMapLoadheightmap)
	ON_COMMAND(ID_DRAW_FILL, OnDrawFill)
	ON_COMMAND(ID_DRAW_PAINT, OnDrawPaint)
	ON_COMMAND(ID_SELECT_AREA, OnSelectArea)
	ON_COMMAND(ID_SELECT_POINT, OnSelectPoint)
	ON_COMMAND(ID_HEIGHT_DRAG, OnHeightDrag)
	ON_UPDATE_COMMAND_UI(ID_DRAW_PAINT, OnUpdateDrawPaint)
	ON_UPDATE_COMMAND_UI(ID_HEIGHT_DRAG, OnUpdateHeightDrag)
	ON_UPDATE_COMMAND_UI(ID_SELECT_AREA, OnUpdateSelectArea)
	ON_UPDATE_COMMAND_UI(ID_SELECT_POINT, OnUpdateSelectPoint)
	ON_UPDATE_COMMAND_UI(ID_DRAW_FILL, OnUpdateDrawFill)
	ON_COMMAND(ID_DRAW_GET, OnDrawGet)
	ON_UPDATE_COMMAND_UI(ID_DRAW_GET, OnUpdateDrawGet)
	ON_COMMAND(ID_HEIGHT_PAINT, OnHeightPaint)
	ON_UPDATE_COMMAND_UI(ID_HEIGHT_PAINT, OnUpdateHeightPaint)
	ON_COMMAND(ID_HEIGHT_PICK, OnHeightPick)
	ON_UPDATE_COMMAND_UI(ID_HEIGHT_PICK, OnUpdateHeightPick)
	ON_COMMAND(ID_TRIDIR1, OnTridir1)
	ON_UPDATE_COMMAND_UI(ID_TRIDIR1, OnUpdateTridir1)
	ON_COMMAND(ID_DRAW_EDGEPAINT, OnDrawEdgepaint)
	ON_UPDATE_COMMAND_UI(ID_DRAW_EDGEPAINT, OnUpdateDrawEdgepaint)
	ON_COMMAND(ID_VIEW_EDGEBRUSHES, OnViewEdgebrushes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_EDGEBRUSHES, OnUpdateViewEdgebrushes)
	ON_COMMAND(ID_VIEW_WORLD, OnViewWorld)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WORLD, OnUpdateViewWorld)
	ON_COMMAND(ID_VIEW_LOCATORMAPS, OnViewLocatormaps)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LOCATORMAPS, OnUpdateViewLocatormaps)
	ON_COMMAND(ID_VIEW_SEALEVEL, OnViewSealevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SEALEVEL, OnUpdateViewSealevel)
	ON_COMMAND(ID_PAINTATSEALEVEL, OnPaintatsealevel)
	ON_UPDATE_COMMAND_UI(ID_PAINTATSEALEVEL, OnUpdatePaintatsealevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TEXTURES, OnUpdateViewTextures)
	ON_COMMAND(ID_VIEW_ZEROCAMERA, OnViewZerocamera)
	ON_COMMAND(ID_VIEW_ZEROCAMERAPOSITION, OnViewZerocameraposition)
	ON_COMMAND(ID_VIEW_FREECAMERA, OnViewFreecamera)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FREECAMERA, OnUpdateViewFreecamera)
	ON_COMMAND(ID_VIEW_ISOMETRIC, OnViewIsometric)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ISOMETRIC, OnUpdateViewIsometric)
	ON_COMMAND(ID_VIEW_WIREFRAME, OnViewWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WIREFRAME, OnUpdateViewWireframe)
	ON_COMMAND(ID_VIEW_TEXTURED, OnViewTextured)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TEXTURED, OnUpdateViewTextured)
	ON_COMMAND(ID_FLIPX, OnFlipx)
	ON_UPDATE_COMMAND_UI(ID_FLIPX, OnUpdateFlipx)
	ON_COMMAND(ID_FLIPY, OnFlipy)
	ON_UPDATE_COMMAND_UI(ID_FLIPY, OnUpdateFlipy)
	ON_COMMAND(ID_ROTATE90, OnRotate90)
	ON_UPDATE_COMMAND_UI(ID_ROTATE90, OnUpdateRotate90)
	ON_COMMAND(ID_VIEW_TERRAINTYPES, OnViewTerraintypes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TERRAINTYPES, OnUpdateViewTerraintypes)
	ON_COMMAND(ID_HEIGHT_TILEMODE, OnHeightTilemode)
	ON_UPDATE_COMMAND_UI(ID_HEIGHT_TILEMODE, OnUpdateHeightTilemode)
	ON_COMMAND(ID_HEIGHT_VERTEXMODE, OnHeightVertexmode)
	ON_UPDATE_COMMAND_UI(ID_HEIGHT_VERTEXMODE, OnUpdateHeightVertexmode)
	ON_COMMAND(ID_POINT, OnPoint)
	ON_UPDATE_COMMAND_UI(ID_POINT, OnUpdatePoint)
	ON_COMMAND(ID_MOVE, OnMove)
	ON_UPDATE_COMMAND_UI(ID_MOVE, OnUpdateMove)
	ON_COMMAND(ID_OBJECTROTATE, OnObjectrotate)
	ON_UPDATE_COMMAND_UI(ID_OBJECTROTATE, OnUpdateObjectrotate)
	ON_COMMAND(ID_VIEW_FEATURES, OnViewFeatures)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FEATURES, OnUpdateViewFeatures)
	ON_COMMAND(ID_VIEW_SHOWHEIGHTS, OnViewShowheights)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWHEIGHTS, OnUpdateViewShowheights)
	ON_COMMAND(ID_VIEW_SHOWTEXTURES, OnViewShowtextures)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWTEXTURES, OnUpdateViewShowtextures)
	ON_COMMAND(ID_FILE_LOADFEATURESET, OnFileLoadfeatureset)
	ON_COMMAND(ID_FILE_SAVEEDGEBRUSHES, OnFileSaveedgebrushes)
	ON_COMMAND(ID_FILE_LOADEDGEBRUSHES, OnFileLoadedgebrushes)
	ON_COMMAND(ID_FILE_SAVETILETYPES, OnFileSavetiletypes)
	ON_COMMAND(ID_FILE_LOADTILETYPES, OnFileLoadtiletypes)
	ON_COMMAND(ID_UNDO, OnUndo)
	ON_COMMAND(ID_REDO, OnRedo)
	ON_COMMAND(ID_VIEW_GOURAUDSHADING, OnViewGouraudshading)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GOURAUDSHADING, OnUpdateViewGouraudshading)
	ON_COMMAND(ID_VIEW_BOUNDINGSPHERES, OnViewBoundingspheres)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BOUNDINGSPHERES, OnUpdateViewBoundingspheres)
	ON_COMMAND(ID_FILE_EXPORTWARZONESCENARIO, OnFileExportwarzonescenario)
	ON_COMMAND(ID_FILE_IMPORTWARZONESCENARIO, OnFileImportwarzonescenario)
	ON_COMMAND(ID_MAP_SAVEHEIGHTMAP, OnMapSaveheightmap)
	ON_COMMAND(ID_VIEW_AUTOHEIGHTSET, OnViewAutoheightset)
	ON_UPDATE_COMMAND_UI(ID_VIEW_AUTOHEIGHTSET, OnUpdateViewAutoheightset)
	ON_COMMAND(ID_OPTIONS_RESETTEXTUREFLAGS, OnOptionsResettextureflags)
	ON_COMMAND(ID_PLAYER1, OnPlayer1)
	ON_UPDATE_COMMAND_UI(ID_PLAYER1, OnUpdatePlayer1)
	ON_COMMAND(ID_PLAYER2, OnPlayer2)
	ON_UPDATE_COMMAND_UI(ID_PLAYER2, OnUpdatePlayer2)
	ON_COMMAND(ID_PLAYER3, OnPlayer3)
	ON_UPDATE_COMMAND_UI(ID_PLAYER3, OnUpdatePlayer3)
	ON_COMMAND(ID_PLAYER4, OnPlayer4)
	ON_UPDATE_COMMAND_UI(ID_PLAYER4, OnUpdatePlayer4)
	ON_COMMAND(ID_PLAYER5, OnPlayer5)
	ON_UPDATE_COMMAND_UI(ID_PLAYER5, OnUpdatePlayer5)
	ON_COMMAND(ID_PLAYER6, OnPlayer6)
	ON_UPDATE_COMMAND_UI(ID_PLAYER6, OnUpdatePlayer6)
	ON_COMMAND(ID_PLAYER7, OnPlayer7)
	ON_UPDATE_COMMAND_UI(ID_PLAYER7, OnUpdatePlayer7)
	ON_COMMAND(ID_PLAYER0, OnPlayer0)
	ON_UPDATE_COMMAND_UI(ID_PLAYER0, OnUpdatePlayer0)
	ON_COMMAND(ID_ENABLEAUTOSNAP, OnEnableautosnap)
	ON_UPDATE_COMMAND_UI(ID_ENABLEAUTOSNAP, OnUpdateEnableautosnap)
	ON_COMMAND(ID_FILE_SAVEMAPSEGMENT, OnFileSavemapsegment)
	ON_COMMAND(ID_TYPEBACKEDEARTH, OnTypebackedearth)
	ON_UPDATE_COMMAND_UI(ID_TYPEBACKEDEARTH, OnUpdateTypebackedearth)
	ON_COMMAND(ID_TYPECLIFFFACE, OnTypecliffface)
	ON_UPDATE_COMMAND_UI(ID_TYPECLIFFFACE, OnUpdateTypecliffface)
	ON_COMMAND(ID_TYPEGREENMUD, OnTypegreenmud)
	ON_UPDATE_COMMAND_UI(ID_TYPEGREENMUD, OnUpdateTypegreenmud)
	ON_COMMAND(ID_TYPEPINKROCK, OnTypepinkrock)
	ON_UPDATE_COMMAND_UI(ID_TYPEPINKROCK, OnUpdateTypepinkrock)
	ON_COMMAND(ID_TYPEREDBRUSH, OnTyperedbrush)
	ON_UPDATE_COMMAND_UI(ID_TYPEREDBRUSH, OnUpdateTyperedbrush)
	ON_COMMAND(ID_TYPEROAD, OnTyperoad)
	ON_UPDATE_COMMAND_UI(ID_TYPEROAD, OnUpdateTyperoad)
	ON_COMMAND(ID_TYPERUBBLE, OnTyperubble)
	ON_UPDATE_COMMAND_UI(ID_TYPERUBBLE, OnUpdateTyperubble)
	ON_COMMAND(ID_TYPESAND, OnTypesand)
	ON_UPDATE_COMMAND_UI(ID_TYPESAND, OnUpdateTypesand)
	ON_COMMAND(ID_TYPESANDYBRUSH, OnTypesandybrush)
	ON_UPDATE_COMMAND_UI(ID_TYPESANDYBRUSH, OnUpdateTypesandybrush)
	ON_COMMAND(ID_TYPESHEETICE, OnTypesheetice)
	ON_UPDATE_COMMAND_UI(ID_TYPESHEETICE, OnUpdateTypesheetice)
	ON_COMMAND(ID_TYPESLUSH, OnTypeslush)
	ON_UPDATE_COMMAND_UI(ID_TYPESLUSH, OnUpdateTypeslush)
	ON_COMMAND(ID_TYPEWATER, OnTypewater)
	ON_UPDATE_COMMAND_UI(ID_TYPEWATER, OnUpdateTypewater)
	ON_COMMAND(ID_FILE_IMPORTCLIPBOARD, OnFileImportclipboard)
	ON_COMMAND(ID_FILE_EXPORTCLIPBOARD, OnFileExportclipboard)
	ON_COMMAND(ID_MAP_EDITSCROLLLIMITS, OnMapEditscrolllimits)
	ON_COMMAND(ID_FILE_EXPORTWARZONESCENARIOEXPAND, OnFileExportwarzonescenarioexpand)
	ON_COMMAND(ID_FILE_EXPORTWARZONEMISSION, OnFileExportwarzonemission)
	ON_COMMAND(ID_MAP_SETPLAYERCLANMAPPINGS, OnMapSetplayerclanmappings)
	ON_COMMAND(ID_GATEWAY, OnGateway)
	ON_UPDATE_COMMAND_UI(ID_GATEWAY, OnUpdateGateway)
	ON_COMMAND(ID_MAP_EXPORTMAPASBITMAP, OnMapExportmapasbitmap)
	ON_COMMAND(ID_MAP_REFRESHZONES, OnMapRefreshzones)
	ON_COMMAND(ID_OPTIONS_ZOOMEDIN, OnOptionsZoomedin)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_ZOOMEDIN, OnUpdateOptionsZoomedin)
	ON_COMMAND(ID_MAP_SAVETILEMAP, OnMapSavetilemap)
	ON_COMMAND(ID_MAP_LOADTILEMAP, OnMapLoadtilemap)
	ON_COMMAND(ID_PLAYER0EN, OnPlayer0en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER0EN, OnUpdatePlayer0en)
	ON_COMMAND(ID_PLAYER1EN, OnPlayer1en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER1EN, OnUpdatePlayer1en)
	ON_COMMAND(ID_PLAYER2EN, OnPlayer2en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER2EN, OnUpdatePlayer2en)
	ON_COMMAND(ID_PLAYER3EN, OnPlayer3en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER3EN, OnUpdatePlayer3en)
	ON_COMMAND(ID_PLAYER4EN, OnPlayer4en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER4EN, OnUpdatePlayer4en)
	ON_COMMAND(ID_PLAYER5EN, OnPlayer5en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER5EN, OnUpdatePlayer5en)
	ON_COMMAND(ID_PLAYER6EN, OnPlayer6en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER6EN, OnUpdatePlayer6en)
	ON_COMMAND(ID_PLAYER7EN, OnPlayer7en)
	ON_UPDATE_COMMAND_UI(ID_PLAYER7EN, OnUpdatePlayer7en)
	ON_COMMAND(ID_OPTIONS_USENAMES, OnOptionsUsenames)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_USENAMES, OnUpdateOptionsUsenames)
	ON_COMMAND(ID_MARKRECT, OnMarkrect)
	ON_UPDATE_COMMAND_UI(ID_MARKRECT, OnUpdateMarkrect)
	ON_COMMAND(ID_PASTE, OnPaste)
	ON_UPDATE_COMMAND_UI(ID_PASTE, OnUpdatePaste)
	ON_COMMAND(ID_XFLIPMARKED, OnXflipmarked)
	ON_UPDATE_COMMAND_UI(ID_XFLIPMARKED, OnUpdateXflipmarked)
	ON_COMMAND(ID_YFLIPMARKED, OnYflipmarked)
	ON_UPDATE_COMMAND_UI(ID_YFLIPMARKED, OnUpdateYflipmarked)
	ON_COMMAND(ID_COPYMARKED, OnCopymarked)
	ON_UPDATE_COMMAND_UI(ID_COPYMARKED, OnUpdateCopymarked)
	ON_COMMAND(ID_PASTEPREFS, OnPasteprefs)
	ON_UPDATE_COMMAND_UI(ID_PASTEPREFS, OnUpdatePasteprefs)
	ON_UPDATE_COMMAND_UI(ID_FILE_EXPORTCLIPBOARD, OnUpdateFileExportclipboard)
	ON_COMMAND(ID_DRAWBRUSHFILL, OnDrawbrushfill)
	ON_UPDATE_COMMAND_UI(ID_DRAWBRUSHFILL, OnUpdateDrawbrushfill)
	//}}AFX_MSG_MAP
//	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBTEditDoc construction/destruction

CBTEditDoc::CBTEditDoc() :
	m_EnableRefresh(FALSE),
	m_PathName(NULL),
	m_FileName(NULL),
	m_FullPath(NULL),
	m_RadarMap(NULL),
	m_RadarMode(RADAR_TEXTURES),
	m_TileBuffer(NULL),
	m_ObjectBufferSize(0),
	m_ObjectBuffer(NULL),
	m_SnapX(16), m_SnapZ(m_SnapX),
	m_EnableGravity(TRUE),
	m_RotSnap(45),
	m_2DMode(M2D_WORLD),
	m_TVMode(MTV_TEXTURES),
	m_DrawMode(DM_TEXTURED),
	m_ShadeMode(SM_GOURAUD),
	m_ShowLocators(TRUE),
	m_ScaleLocators(FALSE),
	m_ShowSeaLevel(FALSE),
	m_ShowVerticies(FALSE),
	m_2DGotFocus(FALSE),
	m_3DGotFocus(FALSE),
	m_CameraMode(MCAM_FREE),
	m_TileMode(FALSE),
	m_AutoSync(FALSE),
	m_ViewFeatures(TRUE),
	m_ShowBoxes(FALSE),
	m_AutoHeight(TRUE),
	m_CurrentPlayer(0),
	m_ButtonLapse(0),
	m_Captureing(FALSE),
	m_PasteFlags(PF_PASTEALL),
	m_PasteWithPlayerID(TRUE),
	m_BrushProp(NULL /* new CBrushProp(NULL) */),
	m_CurrentEdgeBrush(0),
	m_EnableAutoSnap(TRUE)
{
	BTEditDoc = this;

	g_UndoRedo = NULL;

	DebugOpen("\\bteditlog.txt");
//	SetCurrentDirectory("Data");

	TRACE0("CBTEditDoc\n");

	for(int i = 0; i < MAXEDGEBRUSHES; ++i)
	{
		m_EdgeBrush[i] = NULL;
	}
}

CBTEditDoc::~CBTEditDoc()
{
	DebugClose();
}

void CBTEditDoc::DeleteProjectName(void)
{
	if(m_PathName) delete m_PathName;
	if(m_FileName) delete m_FileName;
	if(m_FullPath) delete m_FullPath;
	m_PathName = NULL;
	m_FileName = NULL;
	m_FullPath = NULL;
}


BOOL CBTEditDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_BrushScrollX = m_BrushScrollY = 0;
	m_WorldScrollX = m_WorldScrollY	= 0;
	m_TexVScrollX = m_TexVScrollY = 0;

	m_EnableRefresh = FALSE;
	m_SelTexture = 0;
	m_SelType = 0;
	m_CurrentType = 0;	//ET_TYPEFIRST;
	m_TextureWidth = DEFAULT_TEXTURESIZE;
	m_TextureHeight = DEFAULT_TEXTURESIZE;
	m_TileWidth = DEFAULT_TILESIZE;
	m_TileHeight = DEFAULT_TILESIZE;
	m_MapWidth = DEFAULT_MAPSIZE;
	m_MapHeight = DEFAULT_MAPSIZE;
	m_RadarScale = DEFAULT_RADARSCALE;
	m_HeightScale = DEFAULT_HEIGHTSCALE;
	CenterCamera();

//	m_UnitIDs = NULL;
//	m_DatabaseTypes = NULL;
//	m_Deployments = NULL;

	InitialiseData();
	m_HeightMap->AddScrollLimit(0,0,m_MapWidth,m_MapHeight,"Entire Map");

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBTEditDoc serialization

void CBTEditDoc::Serialize(CArchive& ar)
{
//	CFile	*File;

	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBTEditDoc commands

void CBTEditDoc::OnMapSetmapsize() 
{
	CMapPrefs MapPrefsDlg;
	MapPrefsDlg.SetMapWidth(m_MapWidth);
	MapPrefsDlg.SetMapHeight(m_MapHeight);
	MapPrefsDlg.SetMapSpacing(m_TileWidth);
	MapPrefsDlg.SetTextureSize(m_TextureWidth);
	MapPrefsDlg.SetSeaLevel(m_HeightMap->GetSeaLevel());
	MapPrefsDlg.SetHeightScale(m_HeightMap->GetHeightScale());
	MapPrefsDlg.DoModal();

	if( (MapPrefsDlg.GetMapWidth() > MAX_MAPWIDTH) || (MapPrefsDlg.GetMapHeight() > MAX_MAPHEIGHT) ) {
		MessageBox( NULL, "The maximum map width and height is 250.", "Illegal map size.", MB_OK );
		return;
	}
	
//	if(g_cmdInfo.m_bMapSizePower2) {
//		if( !IsPower2(MapPrefsDlg.GetMapWidth()) || !IsPower2(MapPrefsDlg.GetMapHeight()) ) {
//			MessageBox( NULL, "Map width and height must be a value 2 to the power n\nie. 64x64 , 64x128 128x128 , 256x128 etc.", "Illegal map size.", MB_OK );
//			return;
//		}
//	}

	if( !IsPower2(MapPrefsDlg.GetTextureSize()) ) {
		MessageBox( NULL, "Tile texture size must be a value 2 to the power n\nie. 32,64 or 128", "Illegal texture size.", MB_OK );
		return;
	}

	if( (m_TileWidth != MapPrefsDlg.GetMapSpacing()) ||
		(m_TileHeight != MapPrefsDlg.GetMapSpacing()) ) {
		m_TileWidth = MapPrefsDlg.GetMapSpacing();
		m_TileHeight = MapPrefsDlg.GetMapSpacing();
		m_HeightMap->SetTileSize(m_TileWidth,m_TileHeight);
		CenterCamera();
	}

	if( (m_MapWidth != MapPrefsDlg.GetMapWidth()) ||
		(m_MapHeight != MapPrefsDlg.GetMapHeight()) ||
		(m_TextureWidth != MapPrefsDlg.GetTextureSize()) ) {


		if(MessageBox( NULL,
						"This operation will destroy the current map.\nDo you want to continue.",
						"Warning.", MB_YESNO | MB_DEFBUTTON2 ) == IDNO) {
			return;
		}

		m_MapWidth = MapPrefsDlg.GetMapWidth();
		m_MapHeight = MapPrefsDlg.GetMapHeight();
		m_TextureWidth = MapPrefsDlg.GetTextureSize();
		m_TextureHeight = m_TextureWidth;

		::ValidateRect(NULL,NULL);
		DeleteData();
		InitialiseData();
		m_HeightMap->AddScrollLimit(0,0,m_MapWidth,m_MapHeight,"Entire Map");
		CenterCamera();
	}

	m_HeightMap->SetSeaLevel(MapPrefsDlg.GetSeaLevel());
	m_HeightMap->SetHeightScale(MapPrefsDlg.GetHeightScale());

	BuildRadarMap();
	InvalidateRect(NULL,NULL,NULL);
}

//void CBTEditDoc::OnObjectsSnappreferences() 
//{
//	CSnapPrefs SnapPrefsDlg;
//	SnapPrefsDlg.SetGravity(m_EnableGravity);
//	SnapPrefsDlg.SetSnapMode(m_SnapMode);
//	SnapPrefsDlg.SetSnapX(m_SnapX);
//	SnapPrefsDlg.SetSnapZ(m_SnapZ);
//	SnapPrefsDlg.SetRotSnap(m_RotSnap);
//	SnapPrefsDlg.DoModal();
//	m_EnableGravity = SnapPrefsDlg.GetGravity();
//	m_SnapMode = SnapPrefsDlg.GetSnapMode();
//	m_SnapX = SnapPrefsDlg.GetSnapX();
//	m_SnapZ = SnapPrefsDlg.GetSnapZ();
//	m_RotSnap = SnapPrefsDlg.GetRotSnap();
//}

void CBTEditDoc::CenterCamera(void)
{
   	m_CameraPosition.x=D3DVAL(0);
   	m_CameraPosition.y=D3DVAL(512);	//1024);
   	m_CameraPosition.z=D3DVAL(0);
   	m_CameraRotation.x=D3DVAL(25);
   	m_CameraRotation.y=D3DVAL(0);
   	m_CameraRotation.z=D3DVAL(0);
}


void CBTEditDoc::LoadTextures(char *FileName)
{
	if(!m_HeightMap->AddTexture(FileName)) {
		return;
	}
	delete m_Textures;
	m_Textures = new CTextureSelector(m_DirectDrawView);
	m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),
						m_TextureWidth,m_TextureHeight,
						m_HeightMap->GetNum3DObjects(),m_HeightMap);
	GetRadarColours();
	InvalidateRect(NULL,NULL,NULL);

	m_EdgeBrush[0]->SetNumImages(m_Textures->GetNumImages());
}


//void CBTEditDoc::OnTexturesAddtexture() 
//{
//	char	FullPath[256];
//	char	FileName[256];
//
//	if(GetFilePath((char*)BitmapFilters,"pcx","*.pcx",TRUE,NULL,FileName,FullPath)) {
//		LoadTextures(FileName);
////		if(!m_HeightMap->AddTexture(FileName)) {
////			return;
////		}
//////		m_HeightMap->SetTextureSize(m_TextureWidth,m_TextureHeight);
////		delete m_Textures;
////		m_Textures = new CTextureSelector(m_DirectDrawView);
////		m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),
////							m_TextureWidth,m_TextureHeight,
////							m_HeightMap->GetNum3DObjects(),m_HeightMap);
////		GetRadarColours();
////		InvalidateRect(NULL,NULL,NULL);
////
////		m_EdgeBrush[0]->SetNumImages(m_Textures->GetNumImages());
//	}
//}


//void CBTEditDoc::OnTexturesTexturepreferences() 
//{
//	CTexturePrefs TexturePrefsDlg;
//	TexturePrefsDlg.SetTextureSize(m_TextureWidth,m_TextureHeight);
//	if(TexturePrefsDlg.DoModal()==IDOK) {
//		TexturePrefsDlg.GetTextureSize(&m_TextureWidth,&m_TextureHeight);
//		m_HeightMap->SetTextureSize(m_TextureWidth,m_TextureHeight);
//		delete m_Textures;
//		m_Textures = new CTextureSelector(m_DirectDrawView);
//		m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),m_TextureWidth,m_TextureHeight);
//		InvalidateRect(NULL,NULL,NULL);
//
//		m_EdgeBrush[0]->SetNumImages(m_Textures->GetNumImages());
//	}
//
//	for(int i=0; i<MAXEDGEBRUSHES; i++) {
//		m_EdgeBrush[i]->SetTextureSize(m_TextureWidth,m_TextureHeight);
//	}
//
//}


//BOOL CBTEditDoc::LoadDataTypes(void)
//{
//	char FullPath[256];
//	
//	if(GetFilePath((char*)LandscapeFilters,"dbt","*.dbt",TRUE,NULL,NULL,FullPath)) {
//		if(m_DatabaseTypes) {
//			delete m_DatabaseTypes;
//			m_DatabaseTypes = NULL;
//		}
//
//		m_DatabaseTypes = new CDatabaseTypes();
//		return m_DatabaseTypes->Create(FullPath);
//	}
//
//	return FALSE;
//}


void CBTEditDoc::InitialiseData(void)
{
	ASSERT(BTEditView!=NULL);

	DesiredProfile.XRes = g_MaxXRes;
	DesiredProfile.YRes = g_MaxYRes;
	
	if(g_cmdInfo.m_bForceEmulation) {
		DesiredProfile.UseHardware = FALSE;
	} else if(g_cmdInfo.m_bForceHardware) {
		DesiredProfile.UseHardware = TRUE;
	}

	if(g_cmdInfo.m_bForceRamp) {
		DesiredProfile.ColourModel = RAMPCOLOR;
	} else if(g_cmdInfo.m_bForceRGB) {
		DesiredProfile.ColourModel = RGBCOLOR;
	}

	m_DirectDrawView = new CDirectDraw();

	//	if(!Render->Create(hWndMain,Devices->List[DDDeviceIndex],
//		Devices->D3DDevices[DDDeviceIndex].List[D3DDeviceIndex].Name,
//		&DesiredProfile)) {
//		exit(-1);
//	}

	if(!m_DirectDrawView->Create((HWND)NULL,NULL,NULL,&DesiredProfile)) {
		exit(-1);
	}

//	m_DirectDrawView->SetFilter(TRUE);
	
	g_WorkDirectory = Win::GetCurrentDirectory();

//	m_DirectDrawView = new CDirectDraw(NULL,NULL,FALSE,
//				&DesiredProfile);
//
//
//	if( m_DirectDrawView->GetResult() != DDINITERR_OK) {
//		DebugPrint("\n\n* %s \n",m_DirectDrawView->GetResultString());
//		DebugPrint("* Switching to emulation!\n\n");
//		delete m_DirectDrawView;
//		DesiredProfile.UseHardware = USEEMULATION;
//		DesiredProfile.ColorModel = RAMPCOLOR;
//		m_DirectDrawView = new CDirectDraw(NULL,NULL,FALSE,
//					&DesiredProfile);
//
//
//		if( m_DirectDrawView->GetResult() != DDINITERR_OK) {
//			DebugPrint("\n\n* %s \n",m_DirectDrawView->GetResultString());
//			DebugPrint("* Aborting!\n\n");
//			delete m_DirectDrawView;
//			exit(-1);
//		}
//	} else {
////		m_DirectDrawView->SetBiLinearFilter(TRUE);
//	}

	m_FillMode = FM_SOLID;

	m_DirectMaths = new CGeometry(m_DirectDrawView->GetDirect3D(),
								m_DirectDrawView->Get3DDevice(),
								m_DirectDrawView->GetViewport());

	m_MatManager = new CMatManager(m_DirectDrawView);

	m_LightID = m_DirectMaths->AddLight(D3DLIGHT_DIRECTIONAL);
	D3DVECTOR LightPosition = {0.0F,1000.0F,0.0F};
	D3DVECTOR LightDirection = {0.0F,-0.86F,0.51F};

	m_LightRotation.x =  45.0F;	//28.0F;
	m_LightRotation.y =  -90.0F;	//-6.0F;
	m_LightRotation.z =  0.0F;
	m_DirectMaths->DirectionVector(m_LightRotation,LightDirection);
	m_DirectMaths->SetLightDirection(m_LightID,&LightDirection);

	D3DCOLORVALUE LightColour = {D3DVAL(2.0), D3DVAL(2.0), D3DVAL(2.0), D3DVAL(2.0)};
	m_DirectMaths->SetLightPosition(m_LightID,&LightPosition);
	m_DirectMaths->SetLightColour(m_LightID,&LightColour);

//	m_DirectDrawView->SetFog(TRUE,SKYRED,SKYGREEN,SKYBLUE,D3DVAL(1000),D3DVAL(2000));

	m_HeightMap=new CHeightMap(m_DirectDrawView,m_DirectMaths,m_MatManager,
					m_MapWidth,m_MapHeight,m_TileWidth,m_TileHeight,m_TextureWidth);

	m_HeightMap->m_NoObjectSnap = !m_EnableAutoSnap;

//	m_HeightMap->AddScrollLimit(0,0,m_MapWidth,m_MapHeight,"Entire Map");

	BuildRadarMap();

// Read in all the objects.
//	char Name[256];
//	strcpy(Name,g_HomeDirectory.c_str());
//	strcat(Name,"\\Data\\Startup.txt");
//	m_HeightMap->ReadObjects(Name);

	m_HeightMap->GetTextureSize(&m_TextureHeight,&m_TextureWidth);

//	char Name[256];
//	strcpy(Name,g_HomeDirectory.c_str());
//	strcat(Name,"\\Data\\Structures.txt");
//	if(!m_HeightMap->ReadStructureStats(Name)) {		// Temp to test structure file parser.
//		MessageBox(NULL,"Error parsing file.","Structures.txt",MB_OK);
//	}

	m_Textures = new CTextureSelector(m_DirectDrawView);
	m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),
						m_TextureWidth,m_TextureHeight,
						m_HeightMap->GetNum3DObjects(),m_HeightMap);
//						m_HeightMap->GetNum3DObjects(),m_HeightMap->GetObjectNames());
	GetRadarColours();

	m_ViewIsInitialised=TRUE;

	for(int i=0; i<MAXEDGEBRUSHES; i++) {
		m_EdgeBrush[i] = new CEdgeBrush(m_HeightMap,m_TextureWidth,m_TextureHeight,m_Textures->GetNumImages());
	}

	SetTitle("Noname");

	m_SelType = 0;
	m_SelTexture = 0;

	m_EnableRefresh = TRUE;

	g_UndoRedo = new CUndoRedo(m_HeightMap,UNDO_STACKSIZE);

	m_BrushProp = new CBrushProp(WFView);

	DebugPrint("Completed Doc initialisation\n");
}

void CBTEditDoc::GetLightRotation(D3DVECTOR &Rotation)
{
	Rotation = m_LightRotation;
}


void CBTEditDoc::SetLightRotation(D3DVECTOR &Rotation)
{
	D3DVECTOR LightDirection;

	m_LightRotation = Rotation;
	m_DirectMaths->DirectionVector(m_LightRotation,LightDirection);
	m_DirectMaths->SetLightDirection(m_LightID,&LightDirection);
}

DWORD	CBTEditDoc::GetTextureViewWidth(HWND hWnd)
{
	return m_Textures->GetWidth(hWnd);
}

DWORD	CBTEditDoc::GetTextureViewHeight(HWND hWnd)
{
	return m_Textures->GetHeight(hWnd);
}

void CBTEditDoc::DeleteData(void)
{
	m_EnableRefresh = FALSE;

	DELOBJ(g_UndoRedo);
	DELOBJ(m_HeightMap);
	DELOBJ(m_Textures);
	DELOBJ(m_MatManager);
	DELOBJ(m_DirectMaths);
	DELOBJ(m_DirectDrawView);

	for(int i=0; i<MAXEDGEBRUSHES; i++) {
		DELOBJ(m_EdgeBrush[i]);
	}

	DELOBJ(m_TileBuffer);

	m_ViewIsInitialised=FALSE;

	if(m_BrushProp != NULL) {
   		if(m_BrushProp->GetSafeHwnd() != 0) {
			m_BrushProp->DestroyWindow();
		}
		delete m_BrushProp;
	}
}


void CBTEditDoc::UpdateStatusBar(void)
{
	// Update the status bar contents.
   	CString str;
   	CMainFrame* Frame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
   	CStatusBar* Status = &Frame->m_wndStatusBar;
   	if(Status) {
   		str.Format("Position (%d,%d,%d)   Angle (%d,%d,%d)",
   						(SLONG)m_CameraPosition.x,
   						(SLONG)m_CameraPosition.y,
   						(SLONG)m_CameraPosition.z,
   						(DWORD)(m_CameraRotation.x)%360,
   						(DWORD)(m_CameraRotation.y)%360,
   						(DWORD)(m_CameraRotation.z)%360);
   		Status->SetPaneText(1,str);

		if((g_CursorTileX >= 0) && (g_CursorTileZ >= 0)) {
			str.Format("Tile Position T(%d,%d) W(%d %d)",
   							g_CursorTileX,g_CursorTileZ,
   							g_CursorTileX*m_TileWidth,g_CursorTileZ*m_TileHeight);
		} else {
			str.Format("Tile Position ?");
		}
   		Status->SetPaneText(2,str);
   	}
}


void CBTEditDoc::Update3DView(void)
{
	if(m_EnableRefresh) {
//		DebugPrint("Update3D\n");
		m_DirectDrawView->SetClipper(m_3DWnd);
 		m_DirectDrawView->OnResize(m_3DWnd);

 		m_DirectDrawView->ClearBackBuffer();

		m_DirectMaths->TransformLights(&m_CameraRotation,&m_CameraPosition);

		m_MatManager->BeginScene();

		if(m_ShowSeaLevel) {
			m_HeightMap->DrawSea(m_CameraRotation,m_CameraPosition);
		}

		m_HeightMap->DrawHeightMap(m_CameraRotation,m_CameraPosition);
		if(m_ShowVerticies) {
			m_HeightMap->Draw3DVerticies(m_CameraRotation,m_CameraPosition);
		}

//		if((m_EditTool==ET_POINT) || (m_EditTool==ET_MOVE) || (m_EditTool==ET_ROTATEOBJECT)) {
//			m_ShowBoxes = TRUE;
//		}  else {
//			m_ShowBoxes = FALSE;
//		}

		if(m_ViewFeatures) {
			m_HeightMap->DrawObjects(m_CameraRotation,m_CameraPosition,m_ShowBoxes);
		}

		m_HeightMap->DisplayGateways3D(m_CameraRotation,m_CameraPosition);

//		m_HeightMap->DrawScrollLimits(m_CameraRotation,m_CameraPosition);

		m_MatManager->EndScene();
  

		if(m_ShowLocators) {
			CDIBDraw *DIBDraw = new CDIBDraw(m_3DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
			DIBDraw->ClearDIB();
			DrawRadarMap3D(DIBDraw,m_CameraPosition);

			HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
			DIBDraw->SwitchBuffers(hdc);
			m_DirectDrawView->ReleaseBackBufferDC(hdc);
			delete DIBDraw;
		}

		if((m_3DGotFocus) && (!m_Captureing)) {
 			HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
   			CDC	*dc=dc->FromHandle(hdc);

   			CPen Pen;
   			Pen.CreatePen( PS_SOLID, 2, RGB(255,255,255) );
   			CPen* pOldPen = dc->SelectObject( &Pen );

   			RECT Rect;
   			GetClientRect(m_3DWnd,&Rect);
   			dc->MoveTo(1,1);
   			dc->LineTo(Rect.right-1,1);
   			dc->LineTo(Rect.right-1,Rect.bottom-1);
   			dc->LineTo(1,Rect.bottom-1);
   			dc->LineTo(1,1);

   			dc->SelectObject( pOldPen );
			m_DirectDrawView->ReleaseBackBufferDC(hdc);
		}

 		m_DirectDrawView->SwitchBuffers();

		UpdateStatusBar();
	}
}



int CBTEditDoc::SelectFace(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD XPos,DWORD YPos)
{
	return m_HeightMap->SelectFace(CameraRotation,CameraPosition,XPos,YPos);
}

int CBTEditDoc::Select2DFace(DWORD XPos,DWORD YPos,int ScrollX,int ScrollY)
{
	return m_HeightMap->Select2DFace(XPos,YPos,ScrollX,ScrollY);
}

// Convert a coord in the 2d top down window into a world coordinate.
//
void CBTEditDoc::ConvertCoord2d3d(SLONG &XPos,SLONG &YPos,SLONG ScrollX,SLONG ScrollY)
{
	XPos = (XPos+ScrollX)*(m_TileWidth/m_TextureWidth) - (m_MapWidth*m_TileWidth/2);
	YPos = (YPos+ScrollY)*(m_TileHeight/m_TextureHeight) - (m_MapHeight*m_TileHeight/2);
}

float CBTEditDoc::GetHeightAt(D3DVECTOR &Position)
{
	float Height = (float)m_HeightMap->GetHeight(Position.x,-Position.z);
	DebugPrint("%.2f\n",Height);

	return Height;
}

void CBTEditDoc::DoGridSnap(D3DVECTOR &Position,int SnapMode)
{
//	SLONG x,z;
//
//	if(SnapMode < 0) {
//		SnapMode = m_SnapMode;
//	}
//
//	switch(SnapMode) {
//		case	SNAP_CENTER:
//			x = (SLONG)(Position.x)/m_TileWidth;
//			x = (x*m_TileWidth)+m_TileWidth/2;
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_TileHeight;
//			z = (z*m_TileHeight)+m_TileHeight/2;
//			Position.z = (float)z;
//			break;
//		case	SNAP_TOPLEFT:
//			x = (SLONG)(Position.x)/m_TileWidth;
//			x = (x*m_TileWidth);
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_TileHeight;
//			z = (z*m_TileHeight)+m_TileHeight-1;
//			Position.z = (float)z;
//			break;
//		case	SNAP_TOPRIGHT:
//			x = (SLONG)(Position.x)/m_TileWidth;
//			x = (x*m_TileWidth)+m_TileWidth-1;
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_TileHeight;
//			z = (z*m_TileHeight)+m_TileHeight-1;
//			Position.z = (float)z;
//			break;
//		case	SNAP_BOTTOMLEFT:
//			x = (SLONG)(Position.x)/m_TileWidth;
//			x = (x*m_TileWidth);
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_TileHeight;
//			z = (z*m_TileHeight);
//			Position.z = (float)z;
//			break;
//		case	SNAP_BOTTOMRIGHT:
//			x = (SLONG)(Position.x)/m_TileWidth;
//			x = (x*m_TileWidth)+m_TileWidth-1;
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_TileHeight;
//			z = (z*m_TileHeight);
//			Position.z = (float)z;
//			break;
//		case	SNAP_CUSTOM:
//			x = (SLONG)(Position.x)/m_SnapX;
//			x = (x*m_SnapX)+m_SnapX/2;
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/m_SnapZ;
//			z = (z*m_SnapZ)+m_SnapZ/2;
//			Position.z = (float)z;
//			break;
//		case	SNAP_QUAD:
//			x = (SLONG)(Position.x)/(m_TileWidth/2);
//			x = (x*(m_TileWidth/2))+(m_TileWidth/2)/2;
//			Position.x = (float)x;
//			z = (SLONG)(Position.z)/(m_TileHeight/2);
//			z = (z*(m_TileHeight/2))+(m_TileHeight/2)/2;
//			Position.z = (float)z;
//			break;
//	}
}

void CBTEditDoc::SetTextureID(DWORD Selected,DWORD TextureID)
{
	m_HeightMap->SetTextureID(Selected,TextureID);
}

void CBTEditDoc::FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags)
{
	m_HeightMap->FillMap(Selected,TextureID,Type,Flags);
}


void CBTEditDoc::RedrawFace(HWND hWnd,DWORD Selected,D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	m_DirectDrawView->SetClipper(hWnd);
 	m_DirectDrawView->OnResize(hWnd);

   	m_MatManager->BeginScene();
	m_HeightMap->DrawTile(Selected,CameraRotation,CameraPosition);
 	m_MatManager->EndScene();

	if(m_ShowLocators) {
		CDIBDraw *DIBDraw = new CDIBDraw(m_3DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
		DIBDraw->ClearDIB();
		DrawRadarMap3D(DIBDraw,m_CameraPosition);
		HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
		DIBDraw->SwitchBuffers(hdc);
		m_DirectDrawView->ReleaseBackBufferDC(hdc);
		delete DIBDraw;
	}

   	m_DirectDrawView->SwitchBuffers();
}

void CBTEditDoc::RedrawFace(DWORD Selected)
{
	m_DirectDrawView->SetClipper(m_3DWnd);
 	m_DirectDrawView->OnResize(m_3DWnd);

   	m_MatManager->BeginScene();
	m_HeightMap->DrawTile(Selected,m_CameraRotation,m_CameraPosition);
 	m_MatManager->EndScene();

	if(m_ShowLocators) {
		CDIBDraw *DIBDraw = new CDIBDraw(m_3DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
		DIBDraw->ClearDIB();
		DrawRadarMap3D(DIBDraw,m_CameraPosition);
		HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
		DIBDraw->SwitchBuffers(hdc);
		m_DirectDrawView->ReleaseBackBufferDC(hdc);
		delete DIBDraw;
	}

   	m_DirectDrawView->SwitchBuffers();
}

			
BOOL CBTEditDoc::SelectTexture(SLONG XPos,SLONG YPos)
{
	if( m_Textures->SelectTexture(XPos,YPos) ) {
		m_SelTexture = m_Textures->GetSelectedID();

		if(m_SelTexture <= m_Textures->GetNumImages()) {
			m_SelIsTexture = TRUE;
			m_SelType = m_Textures->GetTextureType(m_SelTexture);
			m_SelFlags = m_Textures->GetTextureFlags(m_SelTexture-1);
		} else {
			m_SelIsTexture = FALSE;
			m_SelObject = m_SelTexture - m_Textures->GetNumImages() - 1;
			m_SelTexture = m_Textures->GetNumImages();
			m_SelFlags = 0;
		}

		DebugPrint("%d %d %d %d\n",m_SelTexture,m_SelType,m_SelIsTexture,m_SelObject);

		return TRUE;
	}

	return FALSE;
}


void CBTEditDoc::UpdateTextureView(HWND hWnd,HWND OwnerhWnd,
								   SLONG ScrollX,SLONG ScrollY)
{
	if(m_EnableRefresh) {
		m_TVhWnd = hWnd;
		m_TVScrollX = ScrollX;
		m_TVScrollY = ScrollY;
		UpdateTextureView();
	}
}

void CBTEditDoc::UpdateTextureView(void)
{
	if(m_EnableRefresh) {
// Get the size of the client window.
		RECT ClientRect;
		GetClientRect(m_TVhWnd,&ClientRect);
// Round it up to the nearest tile.
		ClientRect.right = ((ClientRect.right+m_TextureWidth-1)/m_TextureWidth) * m_TextureWidth;
		ClientRect.bottom = ((ClientRect.bottom+m_TextureHeight*3-1)/m_TextureHeight) * m_TextureHeight;
		CDIBDraw *DIBDraw = new CDIBDraw(m_TVhWnd,ClientRect.right,ClientRect.bottom,g_WindowsIs555);

//		CDIBDraw *DIBDraw = new CDIBDraw(m_TVhWnd);

		DIBDraw->ClearDIB();
		m_Textures->Draw(DIBDraw,-m_TVScrollX,-m_TVScrollY);
		DIBDraw->SwitchBuffers(0,m_TextureHeight);

		delete DIBDraw;
	}
}



DWORD CBTEditDoc::Get2DViewWidth(void)
{
	return m_HeightMap->GetMapWidth() * m_TextureWidth;
}

DWORD CBTEditDoc::Get2DViewHeight(void)
{
	return m_HeightMap->GetMapHeight() * m_TextureHeight;
}


void CBTEditDoc::Set2DViewPos(void)
{
	WFView->SyncPosition(m_CameraPosition);
}

void CBTEditDoc::Set3DViewPos(void)
{
	BTEditView->SyncPosition();
}


void CBTEditDoc::Invalidate2D(void)
{
	WFView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::Invalidate3D(void)
{
	BTEditView->InvalidateRect(NULL,NULL);
}




void CBTEditDoc::Update2DView(HWND hWnd,HWND OwnerhWnd,
								   SLONG ScrollX,SLONG ScrollY)
{
}


void CBTEditDoc::Update2DView(void)
{
	if(m_EnableRefresh) {
// Get the size of the client window.
		RECT ClientRect;
		GetClientRect(m_2DWnd,&ClientRect);
// Round it up to the nearest tile.
		ClientRect.right = ((ClientRect.right+m_TextureWidth-1)/m_TextureWidth) * m_TextureWidth;
		ClientRect.bottom = ((ClientRect.bottom+m_TextureHeight-1)/m_TextureHeight) * m_TextureHeight;
		CDIBDraw *DIBDraw = new CDIBDraw(m_2DWnd,ClientRect.right,ClientRect.bottom,g_WindowsIs555);

		if(m_2DMode == M2D_WORLD) {
			DIBDraw->ClearDIB();
			m_HeightMap->Draw2DMap(DIBDraw,m_Textures->GetImages(),m_Textures->GetNumImages(),m_ScrollX,m_ScrollY);

			if(m_ViewFeatures) {
				m_HeightMap->DrawObjects2D(DIBDraw,m_ScrollX,m_ScrollY,&ClientRect);
			}

			m_HeightMap->DisplayGateways2D(DIBDraw,m_ScrollX, m_ScrollY,&ClientRect);

			if(m_2DGotFocus) {
				CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

				CPen Pen;
				Pen.CreatePen( PS_SOLID, 2, RGB(255,255,255) );
				CPen* pOldPen = dc->SelectObject( &Pen );

				RECT Rect;
				GetClientRect(m_2DWnd,&Rect);
				dc->MoveTo(1,1);
				dc->LineTo(Rect.right-1,1);
				dc->LineTo(Rect.right-1,Rect.bottom-1);
				dc->LineTo(1,Rect.bottom-1);
	   			dc->LineTo(1,1);

				dc->SelectObject( pOldPen );
			}

			if(m_ShowLocators) {
				CDIBDraw *MapDIB = new CDIBDraw(m_2DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
				MapDIB->ClearDIB();
				DrawRadarMap2D(MapDIB,0,0);
				MapDIB->SwitchBuffers(DIBDraw->GetDIBDC());
				delete MapDIB;
			}

			DIBDraw->SwitchBuffers();

		} else {
			DIBDraw->ClearDIB();

			int XPos = m_ScrollX + GetTextureWidth()*OVERSCAN;
			int YPos = m_ScrollY + GetTextureHeight()*OVERSCAN-EBVIEW_YOFFSET;

			m_EdgeBrush[m_CurrentEdgeBrush]->DrawEdgeBrush(DIBDraw,m_Textures->GetImages(),XPos,YPos);

			DIBDraw->SwitchBuffers();
		}

		delete DIBDraw;
	}
}



void CBTEditDoc::Update2DView(int Dest)
{
	Update2DView(Dest,0,(m_TileBufferWidth-1)+(m_TileBufferHeight-1)*m_MapWidth);
}


void CBTEditDoc::Update2DView(int Dest,int Corner0,int Corner1)
{
	if(m_EnableRefresh) {
// Get the size of the client window.
		RECT ClientRect;
		GetClientRect(m_2DWnd,&ClientRect);
// Round it up to the nearest tile.
		ClientRect.right = ((ClientRect.right+m_TextureWidth-1)/m_TextureWidth) * m_TextureWidth;
		ClientRect.bottom = ((ClientRect.bottom+m_TextureHeight-1)/m_TextureHeight) * m_TextureHeight;
		CDIBDraw *DIBDraw = new CDIBDraw(m_2DWnd,ClientRect.right,ClientRect.bottom,g_WindowsIs555);

		if(m_2DMode == M2D_WORLD) {
			DIBDraw->ClearDIB();
			m_HeightMap->Draw2DMap(DIBDraw,m_Textures->GetImages(),m_Textures->GetNumImages(),m_ScrollX,m_ScrollY);

			m_HeightMap->DrawObjects2D(DIBDraw,m_ScrollX,m_ScrollY,&ClientRect);

			m_HeightMap->DisplayGateways2D(DIBDraw,m_ScrollX, m_ScrollY,&ClientRect);

			CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

   			CPen Pen;
   			Pen.CreatePen( PS_SOLID, 2, RGB(255,255,255) );
			CPen* pOldPen = dc->SelectObject( &Pen );

			int y0 = Corner0 / m_MapWidth;
			int x0 = Corner0 % m_MapWidth;
			int y1 = Corner1 / m_MapWidth;
			int x1 = Corner1 % m_MapWidth;

  			if(x1 < x0) {
				int tmp = x1;
				x1 = x0;
				x0 = tmp;
			}

			if(y1 < y0) {
				int tmp = y1;
				y1 = y0;
				y0 = tmp;
			}

			if(Dest >= 0) {
				int yd = Dest / m_MapWidth;
				int xd = Dest % m_MapWidth;
				x1 = xd + (x1-x0);
				y1 = yd + (y1-y0);
				x0 = xd;
				y0 = yd;
			}

			ClipCoordToMap(x0,y0);
			ClipCoordToMap(x1,y1);

			x0 = x0 * m_TextureWidth - (m_ScrollX/m_TextureWidth)*m_TextureWidth;
			y0 = y0 * m_TextureHeight - (m_ScrollY/m_TextureWidth)*m_TextureHeight;
			x1 = (x1+1) * m_TextureWidth - (m_ScrollX/m_TextureWidth)*m_TextureWidth;
			y1 = (y1+1) * m_TextureHeight - (m_ScrollY/m_TextureWidth)*m_TextureHeight;

 			dc->MoveTo(x0,y0);
   			dc->LineTo(x1,y0);
   			dc->LineTo(x1,y1);
   			dc->LineTo(x0,y1);
   			dc->LineTo(x0,y0);

			dc->SelectObject( pOldPen );

			if(m_ShowLocators) {
				CDIBDraw *MapDIB = new CDIBDraw(m_2DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
				MapDIB->ClearDIB();
				DrawRadarMap2D(MapDIB,0,0);
				MapDIB->SwitchBuffers(DIBDraw->GetDIBDC());
				delete MapDIB;
			}

//			if(m_ShowLocators) {
//				DrawRadarMap2D(DIBDraw,0,0);
//			}

			DIBDraw->SwitchBuffers();
		} else {
			DIBDraw->ClearDIB();
			m_EdgeBrush[m_CurrentEdgeBrush]->DrawEdgeBrush(DIBDraw,m_Textures->GetImages(),m_ScrollX,m_ScrollY);
			DIBDraw->SwitchBuffers();
		}

		delete DIBDraw;
	}
}


CEdgeBrush *CBTEditDoc::SetEdgeBrush(int ID)
{
	if((ID < MAXEDGEBRUSHES) && (ID >= 0)) {
		m_CurrentEdgeBrush = ID;
		Update2DView();
	}

	return m_EdgeBrush[m_CurrentEdgeBrush];
}


CEdgeBrush *CBTEditDoc::NextEdgeBrush(void)
{
	if(m_CurrentEdgeBrush < MAXEDGEBRUSHES-1) {
		m_CurrentEdgeBrush++;
	}

	return m_EdgeBrush[m_CurrentEdgeBrush];
}

CEdgeBrush *CBTEditDoc::PrevEdgeBrush(void)
{
	if(m_CurrentEdgeBrush > 0) {
		m_CurrentEdgeBrush--;
	}

	return m_EdgeBrush[m_CurrentEdgeBrush];
}


// Register the 3d views window with the document.
//
void CBTEditDoc::Register3DWindow(HWND hWnd)
{
	m_3DWnd = hWnd;
}

// Register the 3d views camera with the document.
//
void CBTEditDoc::RegisterCamera(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	m_CameraRotation = CameraRotation;
	m_CameraPosition = CameraPosition;
}

void CBTEditDoc::GetCamera(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	CameraRotation = m_CameraRotation;
	CameraPosition = m_CameraPosition;
}


// Register the 2d views window with the document.
//
void CBTEditDoc::Register2DWindow(HWND hWnd)
{
	m_2DWnd = hWnd;
}

// Register the 2d views scroll values with the document.
//
void CBTEditDoc::RegisterScrollPosition(SLONG ScrollX,SLONG ScrollY)
{
	m_ScrollX = ScrollX;
	m_ScrollY = ScrollY;
}

void CBTEditDoc::GetScrollPosition(SLONG *ScrollX,SLONG *ScrollY)
{
	*ScrollX = m_ScrollX;
	*ScrollY = m_ScrollY;
}


void CBTEditDoc::OnCloseDocument() 
{
	if(m_RadarMap) delete m_RadarMap;
	m_RadarMap = NULL;

	if(m_TileBuffer) delete m_TileBuffer;
	m_TileBuffer = NULL;

	m_ObjectBuffer.clear();
	m_ObjectBufferSize = 0;

	DeleteData();
	DeleteProjectName();

	CDocument::OnCloseDocument();
}


BOOL CBTEditDoc::WriteProject(char *FileName)
{
	return WriteProject(FileName, 0, 0, m_HeightMap->GetMapWidth(), m_HeightMap->GetMapHeight());
}


// WriteProject
//
// Creates an instance of the IO class CGrdLandIO, copies data from the
// project into it and calls it's Write method.
//
// Returns TRUE if successful.
//
BOOL CBTEditDoc::WriteProject(char *FileName,UWORD StartX,UWORD StartY,UWORD Width,UWORD Height)
{
	CGrdLandIO Project;
	DWORD	i;
	FILE *Stream;
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	char	BackupName[512];

	_splitpath(FileName,Drive,Dir,FName,Ext);
	sprintf(BackupName,"%s%s%s%s",Drive,Dir,FName,".bak");
	remove( BackupName );
	rename( FileName, BackupName );

	DWORD ScrollX = GetScrollPos(m_2DWnd,SB_HORZ);
	DWORD ScrollY = GetScrollPos(m_2DWnd,SB_VERT);
 
	Project.SetCameraPosition(m_CameraPosition.x,m_CameraPosition.y,m_CameraPosition.z);
	Project.SetCameraRotation(m_CameraRotation.x,m_CameraRotation.y,m_CameraRotation.z);
	Project.Set2DPosition(ScrollX,ScrollY);
	Project.SetHeightScale(m_HeightMap->GetHeightScale());
	Project.SetSnapMode(m_SnapMode);
	Project.SetSnapX(m_SnapX);
	Project.SetSnapZ(m_SnapZ);
	Project.SetGravity(m_EnableGravity);
	Project.SetSeaLevel(m_HeightMap->GetSeaLevel());

	Project.SetMapSize(Width,Height);
 	DWORD TileWidth,TileHeight;
	m_HeightMap->GetTileSize(&TileWidth,&TileHeight);
	Project.SetTileSize(TileWidth,TileHeight);

	Project.SetTextureSize(m_TextureWidth,m_TextureHeight);
	Project.SetNumTextures(m_Textures->GetNumTextures());
	for(i=0; i<m_Textures->GetNumTextures(); i++) {
		Project.SetTextureName(i,m_Textures->GetTextureName(i));
	}

	DWORD	NumTiles=Width*Height;

	CGrdTileIO *GrdTile;


	int GIndex = 0;
	for(int y=StartY; y<StartY+Height; y++)
	{
		for(int x=StartX; x<StartX+Width; x++)
		{
			int Index = x + y * m_HeightMap->GetMapWidth();

			GrdTile = Project.GetTile(GIndex);
   			GrdTile->SetTextureID(m_HeightMap->GetTextureID(Index));
   			GrdTile->SetTextureFlip(m_HeightMap->GetTextureFlipX(Index));
   			GrdTile->SetVertexFlip(m_HeightMap->GetVertexFlip(Index));
   			GrdTile->SetVertexHeight(0,m_HeightMap->GetVertexHeight(Index,0));
   			GrdTile->SetVertexHeight(1,m_HeightMap->GetVertexHeight(Index,1));
   			GrdTile->SetVertexHeight(2,m_HeightMap->GetVertexHeight(Index,2));
   			GrdTile->SetVertexHeight(3,m_HeightMap->GetVertexHeight(Index,3));
			GrdTile->SetFlags(m_HeightMap->GetTileFlags(Index));

			GIndex++;
		}
	}

	if( (Stream = fopen(FileName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full of write protected.",FileName,MB_OK);
		return FALSE;
	}

	fprintf(Stream,"DataSet %s\n",m_DataSetName);

	Project.Write(Stream);

	m_HeightMap->WriteObjectList(Stream,StartX,StartY,Width,Height);
	m_HeightMap->WriteScrollLimits(Stream,StartX,StartY,Width,Height);
	m_HeightMap->WriteGateways(Stream,StartX,StartY,Width,Height);

	WriteTileTypes(Stream);
	WriteTileFlags(Stream);

	WriteBrushes(Stream);

	fclose(Stream);

	return TRUE;
}


/*
// WriteProject
//
// Creates an instance of the IO class CGrdLandIO, copies data from the
// project into it and calls it's Write method.
//
// Returns TRUE if successful.
//
BOOL CBTEditDoc::WriteProject(char *FileName)
{
	CGrdLandIO Project;
	DWORD	i;
	FILE *Stream;

	DWORD ScrollX = GetScrollPos(m_2DWnd,SB_HORZ);
	DWORD ScrollY = GetScrollPos(m_2DWnd,SB_VERT);
 
	Project.SetCameraPosition(m_CameraPosition.x,m_CameraPosition.y,m_CameraPosition.z);
	Project.SetCameraRotation(m_CameraRotation.x,m_CameraRotation.y,m_CameraRotation.z);
	Project.Set2DPosition(ScrollX,ScrollY);
	Project.SetHeightScale(m_HeightMap->GetHeightScale());
	Project.SetSnapMode(m_SnapMode);
	Project.SetSnapX(m_SnapX);
	Project.SetSnapZ(m_SnapZ);
	Project.SetGravity(m_EnableGravity);
	Project.SetSeaLevel(m_HeightMap->GetSeaLevel());

	DWORD MapWidth,MapHeight;
	m_HeightMap->GetMapSize(&MapWidth,&MapHeight);
	Project.SetMapSize(MapWidth,MapHeight);
 	DWORD TileWidth,TileHeight;
	m_HeightMap->GetTileSize(&TileWidth,&TileHeight);
	Project.SetTileSize(TileWidth,TileHeight);

	Project.SetTextureSize(m_TextureWidth,m_TextureHeight);
	Project.SetNumTextures(m_Textures->GetNumTextures());
	for(i=0; i<m_Textures->GetNumTextures(); i++) {
		Project.SetTextureName(i,m_Textures->GetTextureName(i));
	}

	DWORD	NumTiles=MapWidth*MapHeight;
	CGrdTileIO *GrdTile;

	for(i=0; i<NumTiles; i++) {
   		GrdTile = Project.GetTile(i);
   		GrdTile->SetTextureID(m_HeightMap->GetTextureID(i));
   		GrdTile->SetTextureFlip(m_HeightMap->GetTextureFlipX(i));
   		GrdTile->SetVertexFlip(m_HeightMap->GetVertexFlip(i));
   		GrdTile->SetVertexHeight(0,m_HeightMap->GetVertexHeight(i,0));
   		GrdTile->SetVertexHeight(1,m_HeightMap->GetVertexHeight(i,1));
   		GrdTile->SetVertexHeight(2,m_HeightMap->GetVertexHeight(i,2));
   		GrdTile->SetVertexHeight(3,m_HeightMap->GetVertexHeight(i,3));
		GrdTile->SetFlags(m_HeightMap->GetTileFlags(i));
	}
	
	if( (Stream = fopen(FileName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full of write protected.",FileName,MB_OK);
		return FALSE;
	}

	Project.Write(Stream);

	m_HeightMap->WriteObjectList(Stream);

	WriteTileTypes(Stream);

	WriteBrushes(Stream);

	fclose(Stream);

	return TRUE;
}
*/


void CBTEditDoc::DisplayExportSummary(void)
{
	CWorldInfo *Info = m_HeightMap->GetWorldInfo();
	char String[1024];
	char *Tmp = String;
	DWORD TotalStructures = 0;
	DWORD TotalWalls = 0;
	DWORD TotalDroids = 0;

	for(int i=0; i<MAX_PLAYERS; i++) {
		if( (Info->NumStructures[i]) || (Info->NumWalls[i]) || (Info->NumDroids[i]) ) {
			sprintf(Tmp,"Player %d : Structures %d , Walls %d , Units %d\r\n",
					i,
					Info->NumStructures[i],
					Info->NumWalls[i],
					Info->NumDroids[i]);
			Tmp += strlen(Tmp);

			TotalStructures += Info->NumStructures[i];
			TotalWalls += Info->NumWalls[i];
			TotalDroids += Info->NumDroids[i];
		}
	}

	sprintf(Tmp,"\r\nTotal Structures %d\r\n",TotalStructures);
	Tmp += strlen(Tmp);

	sprintf(Tmp,"Total Walls %d\r\n",TotalWalls);
	Tmp += strlen(Tmp);

	sprintf(Tmp,"Total Units %d\r\n",TotalDroids);
	Tmp += strlen(Tmp);

	sprintf(Tmp,"\r\nFeatures %d\r\n",Info->NumFeatures);
	Tmp += strlen(Tmp);

	InfoDialog(String).DoModal();
}

// Proxy class that serves as InputIterator (as defined by the C++ standard in
// section 24.1.1) for the initialisation of InitialLimitsDlg
class scrollLimitStringIterator : public std::list<CScrollLimits>::const_iterator
{
	public:
		inline scrollLimitStringIterator(const std::list<CScrollLimits>::const_iterator& rhs) :
			std::list<CScrollLimits>::const_iterator(rhs)
		{}

		inline std::string operator*() const
		{
			return std::string(const_iterator::operator*().ScriptName);
		}
};

	
// Write out the map in Deliverance format ( Scenario Start ).
//
BOOL CBTEditDoc::WriteDeliveranceStart(char *FileName)
{
	FILE *Stream;
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	_splitpath(FileName,Drive,Dir,FName,Ext);

	// Need to specify scroll limits to use.
	InitialLimitsDlg Dlg(scrollLimitStringIterator(m_HeightMap->GetScrollLimits().begin()), scrollLimitStringIterator(m_HeightMap->GetScrollLimits().end()));
	if(Dlg.DoModal() != IDOK) {
		return TRUE;
	}

	int LimitsIndex = Dlg.Selected();
	if (LimitsIndex < 0)
	{
		MessageBox(NULL,"You must select some scroll limits", "Error", MB_OK);
	}

// Create the save game directory.
	char	SaveName[256];
	strcpy(SaveName,Drive);
	strcat(SaveName,Dir);
	strcat(SaveName,FName);
	CreateDirectory(SaveName,NULL);

// Write out the game.
	strcpy(SaveName,FName);
	strcat(SaveName,".gam");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Game.map",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceGame(Stream,GTYPE_SCENARIO_START,LimitsIndex);
	fclose(Stream);

// Write out the tile types.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\TTypes.ttp");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","TTypes.ttp",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceTileTypes(Stream);
	fclose(Stream);

// Write out the game data.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Game.map");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Game.map",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceMap(Stream);
	fclose(Stream);

// Write out the feature file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Feat.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Feat.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceFeatures(Stream,GTYPE_SCENARIO_START,-1,LimitsIndex);
	fclose(Stream);

// Write out the structure file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Struct.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Struct.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceStructures(Stream,GTYPE_SCENARIO_START,-1,LimitsIndex);
	fclose(Stream);

// Write out the droid initialisation file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\DInit.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Droid.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceDroidInit(Stream,GTYPE_SCENARIO_START,-1,LimitsIndex);
	fclose(Stream);

// Write out the gateway data file.
//	strcpy(SaveName,FName);
//	strcat(SaveName,"\\Gates.txt");
//	if( (Stream = fopen(SaveName,"wb")) == NULL) {
//		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Gates.txt",MB_OK);
//		return FALSE;
//	}
//	m_HeightMap->WriteDeliveranceGateways(Stream);
//	fclose(Stream);

	m_HeightMap->CountObjects(-1,LimitsIndex);
	DisplayExportSummary();
	
	return TRUE;
}


// Write out the map in Deliverance format ( Scenario Expand ).
//
BOOL CBTEditDoc::WriteDeliveranceExpand(char *FileName)
{
	FILE *Stream;
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	_splitpath(FileName,Drive,Dir,FName,Ext);

	int ExcludeIndex;
	int IncludeIndex;

	BOOL Error = FALSE;
	do {
		Error = FALSE;
		// Need to specify scroll limits to use.
		CExpandLimitsDlg Dlg(m_HeightMap);
		if(Dlg.DoModal() != IDOK) {
			return TRUE;
		}

		if( (ExcludeIndex = Dlg.GetExcludeSelected()) < 0) {
			MessageBox(NULL,"You must select some scroll limits to exclude","Error",MB_OK);
			Error = TRUE;
		}

		if( (IncludeIndex = Dlg.GetIncludeSelected()) < 0) {
			MessageBox(NULL,"You must select some scroll limits to exclude","Error",MB_OK);
			Error = TRUE;
		}

//		if(ExcludeIndex == IncludeIndex) {
//			MessageBox(NULL,"Exclude and include limits must be different","Error",MB_OK);
//			Error = TRUE;
//		}

		if(!m_HeightMap->CheckLimitsWithin(ExcludeIndex,IncludeIndex)) {
			MessageBox(NULL,"Exclude limits must fit inside include limits","Error",MB_OK);
			Error = TRUE;
		}
	} while(Error);

// Create the save game directory.
	char	SaveName[256];
	strcpy(SaveName,Drive);
	strcat(SaveName,Dir);
	strcat(SaveName,FName);
	CreateDirectory(SaveName,NULL);

// Write out the game.
	strcpy(SaveName,FName);
	strcat(SaveName,".gam");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Game.map",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceGame(Stream,GTYPE_SCENARIO_EXPAND,IncludeIndex);
	fclose(Stream);

// Write out the feature file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Feat.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Feat.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceFeatures(Stream,GTYPE_SCENARIO_EXPAND,ExcludeIndex,IncludeIndex);
	fclose(Stream);

// Write out the structure file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Struct.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Struct.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceStructures(Stream,GTYPE_SCENARIO_EXPAND,ExcludeIndex,IncludeIndex);
	fclose(Stream);

// Write out the droid initialisation file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\DInit.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Droid.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceDroidInit(Stream,GTYPE_SCENARIO_EXPAND,ExcludeIndex,IncludeIndex);
	fclose(Stream);

	m_HeightMap->CountObjects(ExcludeIndex,IncludeIndex);
	DisplayExportSummary();

	return TRUE;
}


// Write out the map in Deliverance format ( Mission ).
//
BOOL CBTEditDoc::WriteDeliveranceMission(char *FileName)
{
	FILE *Stream;
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	_splitpath(FileName,Drive,Dir,FName,Ext);

// Create the save game directory.
	char	SaveName[256];
	strcpy(SaveName,Drive);
	strcat(SaveName,Dir);
	strcat(SaveName,FName);
	CreateDirectory(SaveName,NULL);

// Write out the game.
	strcpy(SaveName,FName);
	strcat(SaveName,".gam");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Game.map",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceGame(Stream,GTYPE_MISSION,-1);
	fclose(Stream);

// Write out the tile types.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\TTypes.ttp");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","TTypes.ttp",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceTileTypes(Stream);
	fclose(Stream);

// Write out the game data.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Game.map");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Game.map",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceMap(Stream);
	fclose(Stream);

// Write out the feature file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Feat.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Feat.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceFeatures(Stream,GTYPE_MISSION,-1,-1);
	fclose(Stream);

// Write out the structure file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\Struct.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Struct.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceStructures(Stream,GTYPE_MISSION,-1,-1);
	fclose(Stream);

// Write out the droid initialisation file.
	strcpy(SaveName,FName);
	strcat(SaveName,"\\DInit.bjo");
	if( (Stream = fopen(SaveName,"wb")) == NULL) {
		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Droid.bjo",MB_OK);
		return FALSE;
	}
	m_HeightMap->WriteDeliveranceDroidInit(Stream,GTYPE_MISSION,-1,-1);
	fclose(Stream);

//// Write out the droid file.
//	strcpy(SaveName,FName);
//	strcat(SaveName,"\\Droid.bjo");
//// Temporary messure. Writes a dummy file if dos'nt already exist.
//	if( (Stream = fopen(SaveName,"rb")) == NULL) {
//		if( (Stream = fopen(SaveName,"wb")) == NULL) {
//			MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Droid.bjo",MB_OK);
//			return FALSE;
//		}
//		m_HeightMap->WriteDeliveranceDroids(Stream);
//		fclose(Stream);
//	} else {
//		fclose(Stream);
//	}

//// Write out the template file.
//	strcpy(SaveName,FName);
//	strcat(SaveName,"\\Templ.bjo");
//// Temporary messure. Writes a dummy file if dos'nt already exist.
//	if( (Stream = fopen(SaveName,"rb")) == NULL) {
//		if( (Stream = fopen(SaveName,"wb")) == NULL) {
//			MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.","Templ.bjo",MB_OK);
//			return FALSE;
//		}
//		m_HeightMap->WriteDeliveranceTemplates(Stream);
//		fclose(Stream);
//	} else {
//		fclose(Stream);
//	}
//
	m_HeightMap->CountObjects(-1,-1);
	DisplayExportSummary();

	return TRUE;
}


//// Write out the map in Deliverance format.
////
//BOOL CBTEditDoc::WriteProjectNecromancer(char *FileName)
//{
//	FILE *Stream;
//	char	Drive[256];
//	char	Dir[256];
//	char	FName[256];
//	char	Ext[256];
//	char	SaveName[256];
//	_splitpath(FileName,Drive,Dir,FName,Ext);
//
//// Write out the game data.
//	strcpy(SaveName,FName);
//	strcat(SaveName,".map");
//	if( (Stream = fopen(SaveName,"wb")) == NULL) {
//		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.",SaveName,MB_OK);
//		return FALSE;
//	}
//	m_HeightMap->WriteNecromancerMap(Stream);
//	fclose(Stream);
//
//	strcpy(SaveName,FName);
//	strcat(SaveName,".nob");
//	if( (Stream = fopen(SaveName,"wb")) == NULL) {
//		MessageBox(NULL,"Error opening file for write.\nThe disk may be full or write protected.",SaveName,MB_OK);
//		return FALSE;
//	}
//	m_HeightMap->WriteNecromancerObjects(Stream);
//	fclose(Stream);
//
//	return TRUE;
//}



void StrExtractFileName(char *Dest,char *Source)
{
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];

	_splitpath(Source,Drive,Dir,FName,Ext);
	sprintf(Dest,"%s%s",FName,Ext);
}


BOOL CBTEditDoc::LoadDataSet(char *FullPath)
{
   	m_HeightMap->Delete3DObjectInstances();
   	m_HeightMap->Delete3DObjects();

   	if(m_HeightMap->ReadIMDObjects(FullPath) == FALSE) {
		MessageBox(NULL,"Error Processing Data Set",FullPath,MB_OK);
		return FALSE;
	}

	StrExtractFileName(m_DataSetName,FullPath);

   	LoadTextures(m_HeightMap->m_TileTextureName);

   	for(int i=0; i<MAXEDGEBRUSHES; i++) {
   		for(int j=0; j<16; j++) {
   			m_EdgeBrush[i]->SetBrushDirect(j,
   										m_HeightMap->m_BrushHeightMode[i],
   										m_HeightMap->m_BrushHeight[i],
   										m_HeightMap->m_BrushRandomRange[i],
   										m_HeightMap->m_BrushTiles[i][j],
   										m_HeightMap->m_BrushFlags[i][j]);
   		}
   		m_EdgeBrush[i]->SetBrushTextureFromMap();
   	}

   	for(i=0; i<128; i++) {
   		int Type = m_HeightMap->m_TileTypes[i];

   		if(Type >= TF_NUMTYPES) {
   			Type = TF_DEFAULTTYPE;
   		}
   		m_Textures->SetTextureType(i,Type);
   	}
   	
   	ReInitEdgeBrushes();

	return TRUE;
}


// ReadProject
//
// Creates an instance of the IO class CGrdLandIO, calls it's Read
// method and copies data from it.
//
// Returns TRUE if successful.
//
BOOL CBTEditDoc::ReadProject(char *FileName)
{
	FILE *Stream;

	SetCursor(g_Wait);

// Cleanup.
	DeleteData();
	InitialiseData();

	m_EnableRefresh = FALSE;

	CGrdLandIO *Project;

	if( (Stream = fopen(FileName,"rb")) == NULL) {
		MessageBox(NULL,"Error opening file",FileName,MB_OK);
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}


	fpos_t fpos;
	fgetpos(Stream, &fpos);

	// Get the data set name
	fscanf(Stream,"%s",m_DataSetName);
	if(strcmp(m_DataSetName,"DataSet") == 0) {
		if(fscanf(Stream,"%s",m_DataSetName) != 1) {
			fclose(Stream);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
			SetCursor(g_Pointer);
			return FALSE;
		}
		// and load it.
		if( LoadDataSet(m_DataSetName) == FALSE) {
			fclose(Stream);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
			InvalidateRect(NULL,NULL,NULL);
			SetCursor(g_Pointer);
			return FALSE;
		}
	} else {
		MessageBox(NULL,"Loading old project\nPlease specify the data set file to load in the\nfile selector that follows this message.",FileName,MB_OK);
		fsetpos(Stream, &fpos);
		if(SelectAndLoadDataSet() == FALSE) {
			fclose(Stream);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
		}
	}

	if( (Project = m_HeightMap->Read(Stream)) == NULL) {
		fclose(Stream);
		OnFileNew() ;
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}


	Project->GetCameraPosition(&m_CameraPosition.x,&m_CameraPosition.y,&m_CameraPosition.z);
	Project->GetCameraRotation(&m_CameraRotation.x,&m_CameraRotation.y,&m_CameraRotation.z);
	Project->Get2DPosition(&m_ScrollX,&m_ScrollY);
	m_SnapMode = Project->GetSnapMode();
	m_SnapX = Project->GetSnapX();
	m_SnapZ = Project->GetSnapZ();
	m_EnableGravity = Project->GetGravity();

	m_HeightScale = Project->GetHeightScale();
	SetScrollPos(m_2DWnd,SB_HORZ,m_ScrollX,TRUE);
	SetScrollPos(m_2DWnd,SB_VERT,m_ScrollY,TRUE);
	m_HeightMap->GetTextureSize(&m_TextureWidth,&m_TextureHeight);

//	m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),
//							m_TextureWidth,m_TextureHeight,
//							m_HeightMap->GetNum3DObjects(),m_HeightMap);
//	GetRadarColours();

	m_HeightMap->SetHeightScale(m_HeightScale);

	m_EdgeBrush[0]->SetNumImages(m_Textures->GetNumImages());

	Project->GetMapSize(&m_MapWidth,&m_MapHeight);
	Project->GetTileSize(&m_TileWidth,&m_TileHeight);
	m_HeightMap->SetSeaLevel(Project->GetSeaLevel());

	DWORD Version = Project->GetVersion();

	delete Project;

	if(!m_HeightMap->ReadObjectList(Stream)) {
		fclose(Stream);
		MessageBox(NULL,"Error reading object list","Read Error!",MB_OK);
		OnFileNew() ;
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}

	if(Version >= 3) {
		if(!m_HeightMap->ReadScrollLimits(Stream)) {
			fclose(Stream);
			MessageBox(NULL,"Error reading scroll limits","Read Error!",MB_OK);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
			InvalidateRect(NULL,NULL,NULL);
			SetCursor(g_Pointer);
			return FALSE;
		}
	}

	if(Version >= 4) {
		if(!m_HeightMap->ReadGateways(Stream)) {
			fclose(Stream);
			MessageBox(NULL,"Error reading gateways limits","Read Error!",MB_OK);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
			InvalidateRect(NULL,NULL,NULL);
			SetCursor(g_Pointer);
			return FALSE;
		}
	}

	if(!m_Textures->Read(m_HeightMap->GetNumTextures(),m_HeightMap->GetTextureList(),
							m_TextureWidth,m_TextureHeight,
							m_HeightMap->GetNum3DObjects(),m_HeightMap)) {
		fclose(Stream);
		MessageBox(NULL,"An error occured reading terrain textures\nfor the texture selector.\n","Error",MB_OK);
		OnFileNew() ;
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}

	GetRadarColours();

	if(!ReadTileTypes(Stream)) {
		fclose(Stream);
		MessageBox(NULL,"Error reading tile types","Read Error!",MB_OK);
		OnFileNew() ;
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}

	if(!ReadTileFlags(Stream)) {
		fclose(Stream);
		MessageBox(NULL,"Error reading tile flags","Read Error!",MB_OK);
		OnFileNew() ;
		m_EnableRefresh = TRUE;
		InvalidateRect(NULL,NULL,NULL);
		SetCursor(g_Pointer);
		return FALSE;
	}

	m_SelTexture = 1;
	m_SelType = m_Textures->GetTextureType(m_SelTexture);

	if(Version >= 2) {
		if(!ReadBrushes(Stream)) {
			fclose(Stream);
			MessageBox(NULL,"An error occured reading the brush definitions.\n","Error",MB_OK);
			OnFileNew() ;
			m_EnableRefresh = TRUE;
			InvalidateRect(NULL,NULL,NULL);
			SetCursor(g_Pointer);
			return FALSE;
		}
	}

	fclose(Stream);

	BuildRadarMap();

	m_HeightMap->SetObjectTileFlags(TF_HIDE);

	m_EnableRefresh = TRUE;
	InvalidateRect(NULL,NULL,NULL);

	SetPathName( FileName,TRUE );
//	AfxGetApp()->AddToRecentFileList(FileName);

	SetCursor(g_Pointer);
	return TRUE;
}

void CBTEditDoc::OnFileNew() 
{
	DeleteData();
	InitialiseData();
	m_HeightMap->AddScrollLimit(0,0,m_MapWidth,m_MapHeight,"Entire Map");

	InvalidateRect(NULL,NULL,NULL);
}

void CBTEditDoc::OnFileOpen() 
{
	DeleteProjectName();
	m_FileName = new char[MAX_FILESTRING];
	m_PathName = new char[MAX_FILESTRING];
	m_FullPath = new char[MAX_FILESTRING];

	if(GetFilePath((char*)LandscapeFilters,"lnd","*.lnd",TRUE,m_PathName,m_FileName,m_FullPath)) {
		if(ReadProject(m_FullPath)) {
			SetTitle(m_FileName);
		}
	}
}

//void CBTEditDoc::OnFileImportmap() 
//{
//	char FileName[256];
//	char PathName[256];
//	char FullPath[256];
//
//	m_EnableRefresh = FALSE;
//
//	if(GetFilePath((char*)MapFilters,"map","*.map",TRUE,PathName,FileName,FullPath)) {
//		FILE *Stream;
//		Stream = fopen(FullPath,"rb");
//
//		if(m_HeightMap->ReadDeliveranceMap(Stream)) {
//			BuildRadarMap();
//		}
//
//		fclose(Stream);
//	}
//
//	InvalidateRect(NULL,NULL,NULL);
//	m_EnableRefresh = TRUE;
//}
//
//
//void CBTEditDoc::OnFileImportfeatures() 
//{
//	char FileName[256];
//	char PathName[256];
//	char FullPath[256];
//
//	m_EnableRefresh = FALSE;
//
//	if(GetFilePath((char*)FeatureFilters,"bjo","*.bjo",TRUE,PathName,FileName,FullPath)) {
//		FILE *Stream;
//		Stream = fopen(FullPath,"rb");
//
//		m_HeightMap->SetObjectTileFlags(TF_SHOW);
//		m_HeightMap->Delete3DObjectInstances();
//		m_HeightMap->ReadDeliveranceFeatures(Stream);
//		m_HeightMap->SetObjectTileFlags(TF_HIDE);
//
//		fclose(Stream);
//	}
//
//	InvalidateRect(NULL,NULL,NULL);
//	m_EnableRefresh = TRUE;
//}


void CBTEditDoc::OnFileSave() 
{
	if(m_FullPath) {
		WriteProject(m_FullPath);
	} else {
		DeleteProjectName();
		m_FileName = new char[MAX_FILESTRING];
		m_PathName = new char[MAX_FILESTRING];
		m_FullPath = new char[MAX_FILESTRING];

		if(GetFilePath((char*)LandscapeFilters,"lnd","*.lnd",FALSE,m_PathName,m_FileName,m_FullPath)) {
			WriteProject(m_FullPath);
			SetTitle(m_FileName);
  			SetPathName( m_FullPath,TRUE );
//			AfxGetApp()->AddToRecentFileList(m_FullPath);
		}
	}
}

void CBTEditDoc::OnFileSaveAs() 
{
	DeleteProjectName();
	m_FileName = new char[MAX_FILESTRING];
	m_PathName = new char[MAX_FILESTRING];
	m_FullPath = new char[MAX_FILESTRING];
	if(GetFilePath((char*)LandscapeFilters,"lnd","*.lnd",FALSE,m_PathName,m_FileName,m_FullPath)) {
		WriteProject(m_FullPath);
		SetTitle(m_FileName);
		SetPathName( m_FullPath,TRUE );
//		AfxGetApp()->AddToRecentFileList(m_FullPath);
	}
}

// GetFilePath(BOOL Open)
//
// Display file selector and get file path in m_FileName,m_PathName and m_FullName.
// returns TRUE if OK , FALSE if cancel was pressed.
//
// Parameters:
//
// BOOL Open	TRUE for Open, FALSE for Save As.
//
// Returns:
//
// TRUE if ok , FALSE if Cancel.
//
BOOL CBTEditDoc::GetFilePath(char *FilterList,char *ExtType,char *Filter,BOOL Open,
							 char *PathName,char *FileName,char *FullPath)
{
	CString	Tmp;
	DWORD	Flags;

	if(Open) {
		Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	} else {
		Flags = OFN_HIDEREADONLY;
	}

	CFileDialog dlg(Open,ExtType,Filter,Flags,
					FilterList,NULL);

	if(dlg.DoModal() == IDOK) {
		Tmp = dlg.GetPathName();
		char	Drive[256];
		char	Dir[256];
		char	FName[256];
		char	Ext[256];
		_splitpath(Tmp.GetBuffer(0),Drive,Dir,FName,Ext);

		if(FileName) {
			strcpy(FileName,FName);
			strcat(FileName,Ext);
		}

		if(PathName) {
			strcpy(PathName,Drive);
			strcat(PathName,Dir);
		}

		if(FullPath) {
			strcpy(FullPath,Tmp.GetBuffer(0));
		}

		g_WorkDirectory = Win::GetCurrentDirectory();
		DebugPrint("New working directory %s\n", g_WorkDirectory.c_str());

		SetButtonLapse();

		return TRUE;
	}

	SetButtonLapse();

	return FALSE;
}



void CBTEditDoc::OnDrawFill()
{
	m_EditTool = ET_FILL;
//	EnableTextures();
	InitTextureTool();
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnDrawPaint() 
{
	m_EditTool = ET_PAINT;
//	EnableTextures();
	InitTextureTool();
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnGateway() 
{
	m_EditTool = ET_GATEWAY;
	g_OverlayZoneIDs = TRUE;
	InitTextureTool();
//	DisplayTextures(TRUE);
	OnMapRefreshzones();
	Update2DView();
}

void CBTEditDoc::OnSelectArea() 
{
	m_EditTool = ET_SELECTRECT;	
	InitTextureTool();
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnSelectPoint() 
{
	m_EditTool = ET_SELECTPOINT;	
}

void CBTEditDoc::OnHeightDrag() 
{
	m_EditTool = ET_DRAGVERTEX;	
	InitVertexTool();
//	EnableWireFrame();
}



void CBTEditDoc::SetEditToolCursor(void)
{
	switch(m_EditTool) {
		case	ET_FILL:
			::SetCursor(g_PointerFill);
			break;
		case	ET_GET:
		case	ET_GETVERTEX:
		case	ET_GETOBJECTHEIGHT:
			::SetCursor(g_PointerPipet);
			break;
		case	ET_DRAGVERTEX:
		case	ET_DRAGOBJECT:
			::SetCursor(g_PointerPliers);
			break;
		case	ET_PAINT:
		case	ET_EDGEFILL:
		case	ET_EDGEPAINT:
		case	ET_BRUSHPAINT:
		case	ET_PAINTVERTEX:
		case	ET_PAINTVERTEXSEALEVEL:
		case	ET_TRIDIR1:
		case	ET_TRIDIR2:
		case	ET_PLACEOBJECT:
		case	ET_SHOWTILE:
		case	ET_HIDETILE:
//		case	ET_TYPEGRASS:
//		case	ET_TYPEROCK:
		case	ET_TYPESAND:
		case	ET_TYPEWATER:
		case	ET_ROTATETILE:
		case	ET_XFLIPTILE:
		case	ET_YFLIPTILE:
		case	ET_ROTATEOBJECT:
			::SetCursor(g_PointerPaint);
			break;
		case	ET_SELECTRECT:
			::SetCursor(g_PointerSelRect);
			break;
		case	ET_SELECTPOINT:
		case	ET_SELECTOBJECT:
		case	ET_DESELECTOBJECT:
			::SetCursor(g_PointerSelPoint);
			break;
		case	ET_POINT:
		case	ET_MOVE:
			::SetCursor(g_Pointer);
			break;
		default:
			::SetCursor(g_Pointer);
	}
}

void CBTEditDoc::OnUpdateDrawFill(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_FILL ? TRUE : FALSE);
}

void CBTEditDoc::OnUpdateDrawPaint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_PAINT ? TRUE : FALSE);
}

void CBTEditDoc::OnUpdateGateway(CCmdUI* pCmdUI) 
{
	static IsGateway = FALSE;

	if(m_EditTool == ET_GATEWAY) {
		IsGateway = TRUE;
		pCmdUI->SetCheck(TRUE);
	} else {
		if(IsGateway == TRUE) {
			InvalidateRect(NULL,NULL,NULL);
			IsGateway = FALSE;
			g_OverlayZoneIDs = FALSE;
			pCmdUI->SetCheck(FALSE);
		}
	}
}

void CBTEditDoc::OnUpdateHeightDrag(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_DRAGVERTEX ? TRUE : FALSE);
}

void CBTEditDoc::OnUpdateSelectArea(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_SELECTRECT ? TRUE : FALSE);
}

void CBTEditDoc::OnUpdateSelectPoint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_SELECTPOINT ? TRUE : FALSE);
}


void CBTEditDoc::OnTridir1() 
{
	m_EditTool = ET_TRIDIR1;
//	EnableWireFrame();
}

void CBTEditDoc::OnUpdateTridir1(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TRIDIR1 ? TRUE : FALSE);
}

//void CBTEditDoc::OnTridir2() 
//{
//	m_EditTool = ET_TRIDIR2;
////	EnableWireFrame();
//}
//
//void CBTEditDoc::OnUpdateTridir2(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_TRIDIR2 ? TRUE : FALSE);
//}

void CBTEditDoc::OnDrawGet() 
{
	m_EditTool = ET_GET;	
//	EnableTextures();
	InitTextureTool();
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnUpdateDrawGet(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_GET ? TRUE : FALSE);
}

void CBTEditDoc::OnHeightPaint() 
{
	m_EditTool = ET_PAINTVERTEX;	
	InitVertexTool();
//	EnableWireFrame();
}

void CBTEditDoc::OnUpdateHeightPaint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_PAINTVERTEX ? TRUE : FALSE);
}

void CBTEditDoc::OnHeightPick() 
{
	m_EditTool = ET_GETVERTEX;	
	InitVertexTool();
//	EnableWireFrame();
}

void CBTEditDoc::OnUpdateHeightPick(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_GETVERTEX ? TRUE : FALSE);
}


void CBTEditDoc::OnPoint() 
{
	if(m_EditTool != ET_POINT) {
		m_EditTool = ET_POINT;
		Update3DView();
	}
}

void CBTEditDoc::OnUpdatePoint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_POINT ? TRUE : FALSE);
//	if(m_ShowBoxes) {
//		Update3DView();
//	}
}

void CBTEditDoc::OnMove() 
{
	if(m_EditTool != ET_MOVE) {
		m_EditTool = ET_MOVE;
		Update3DView();
	}
//	m_EditTool = ET_MOVE;
}

void CBTEditDoc::OnUpdateMove(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_MOVE ? TRUE : FALSE);
//	if(m_ShowBoxes) {
//		Update3DView();
//	}
}


void CBTEditDoc::BuildRadarMap(void)
{
//	if( (m_MapWidth <= 80) && (m_MapHeight <= 80) ) {
//		m_RadarScale = 2;
//	} else {
//		m_RadarScale = 1;
//	}

	if(m_ScaleLocators) {
		m_RadarScale = 2;
	} else {
		m_RadarScale = 1;
	}

	DWORD i;
	PixFormatExt Format;

//	m_DirectDrawView->GetWindowsPixelFormat(Format);
	m_DirectDrawView->GetDisplayPixelFormat(Format);
	UWORD RedMul = 8-Format.RedBPP;
	UWORD GreenMul = 8-Format.GreenBPP;
	UWORD BlueMul = 8-Format.BlueBPP;
	UWORD Height;

	if(m_RadarMap) delete m_RadarMap;
//	m_RadarMap = new UBYTE[m_MapWidth*m_MapHeight];
	m_RadarMap = new UWORD[m_MapWidth*m_MapHeight];

	for(i=0; i<m_MapWidth*m_MapHeight; i++) {
//		m_RadarMap[i] = (UBYTE)(m_HeightMap->GetVertexHeight(i,0) );
		Height = (UWORD)m_HeightMap->GetVertexHeight(i,0);
//		m_RadarMap[i] =  (Height>>BlueMul)<<Format.BlueShift |	   	// Blue 
//						  (Height>>GreenMul)<<Format.GreenShift |  	// Green
//						  (Height>>RedMul)<<Format.RedShift;		// Red  
		m_RadarMap[i] =  (Height>>BlueMul)<<Format.BlueShift |	   	// Blue 
						  (0>>GreenMul)<<Format.GreenShift |  	// Green
						  (0>>RedMul)<<Format.RedShift;		// Red  
	}
}


void CBTEditDoc::DrawRadarMap2D(CDIBDraw *DIBDraw,DWORD XPos,DWORD YPos)
{
	SLONG Scale;

	if(m_RadarMap) {
		if(m_EditTool == ET_GATEWAY) {
			Scale = DrawRadarBlocking(DIBDraw);
		} else if(m_RadarMode == RADAR_HEIGHTS) {
			Scale = DrawRadarHeightMap(DIBDraw);
		} else {
			Scale = DrawRadarTextureMap(DIBDraw);
		}

		if(Scale == 0) return;

		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		if(m_EditTool != ET_GATEWAY) {

			if(m_HeightMap->GetNumScrollLimits()) {
				HDC	dc=(HDC)DIBDraw->GetDIBDC();
				HPEN NormalPen = CreatePen(PS_SOLID,1,RGB(255,0,255));
				HPEN OldPen = (HPEN)SelectObject(dc,NormalPen);
				
				for(std::list<CScrollLimits>::const_iterator curNode = m_HeightMap->GetScrollLimits().begin(); curNode != m_HeightMap->GetScrollLimits().end(); ++curNode)
				{
					int x0 = (curNode->MinX + OVERSCAN)*m_RadarScale;
					int y0 = (curNode->MinZ + OVERSCAN)*m_RadarScale;
					int x1 = (curNode->MaxX + OVERSCAN)*m_RadarScale;
					int y1 = (curNode->MaxZ + OVERSCAN)*m_RadarScale;

					MoveToEx(dc,x0,y0,NULL);
					LineTo(dc,x1,y0);
					LineTo(dc,x1,y1);
					LineTo(dc,x0,y1);
					LineTo(dc,x0,y0);
				}

				SelectObject(dc,OldPen);
				DeleteObject(NormalPen);
			}
		}

		if(m_EditTool == ET_GATEWAY)
		{
			HDC	dc = (HDC)DIBDraw->GetDIBDC();
			HPEN NormalPen = CreatePen(PS_SOLID,1,RGB(255,255,0));
			HPEN OldPen = (HPEN)SelectObject(dc,NormalPen);

			for (std::list<GateWay>::const_iterator curGateway = m_HeightMap->GetGateWays().begin(); curGateway != m_HeightMap->GetGateWays().end(); ++curGateway)
			{
				int x0 = (static_cast<int>(curGateway->x0) + OVERSCAN) * m_RadarScale;
				int y0 = (static_cast<int>(curGateway->y0) + OVERSCAN) * m_RadarScale;
				int x1 = (static_cast<int>(curGateway->x1) + OVERSCAN) * m_RadarScale;
				int y1 = (static_cast<int>(curGateway->y1) + OVERSCAN) * m_RadarScale;

				if(x0 == x1)
				{
					for(; y0 < y1+1; y0++)
					{
						MoveToEx(dc, x0, y0, NULL);
						LineTo(dc, x0 + 1, y0);
					}
				}
				else
				{
					for (; x0 < x1+1; x0++)
					{
						MoveToEx(dc, x0, y0, NULL);
						LineTo(dc, x0 + 1, y0);
					}
				}
			}

			SelectObject(dc, OldPen);
			DeleteObject(NormalPen);
		}


		CPen Pen;
		Pen.CreatePen( PS_SOLID, 1, RGB(255,255,0) );
        CPen* pOldPen = dc->SelectObject( &Pen );

		SLONG XPos = (m_ScrollX/m_TextureWidth) / Scale;
		SLONG YPos = (m_ScrollY/m_TextureHeight) / Scale;
		XPos +=OVERSCAN;
		YPos +=OVERSCAN;

// Get the size of the client window.
		RECT ClientRect;
		GetClientRect((HWND)DIBDraw->GetDIBWindow(),&ClientRect);

		SLONG Width = (ClientRect.right/m_TextureWidth)*m_RadarScale;
		SLONG Height = (ClientRect.bottom/m_TextureHeight)*m_RadarScale;
		XPos *= m_RadarScale;
		YPos *= m_RadarScale;

		dc->MoveTo(XPos,YPos);
		dc->LineTo(XPos+Width,YPos);
		dc->LineTo(XPos+Width,YPos+Height);
		dc->LineTo(XPos,YPos+Height);
		dc->LineTo(XPos,YPos);

        dc->SelectObject( pOldPen );

		if((m_ViewFeatures) && (m_EditTool != ET_GATEWAY)) {
			m_HeightMap->DrawRadarObjects(DIBDraw,m_RadarScale);
		}
	}
}

void CBTEditDoc::DrawRadarMap3D(CDIBDraw *DIBDraw,D3DVECTOR &CamPos)
{
	if(m_RadarMap) {
		SLONG Scale;

		if(m_RadarMode == RADAR_HEIGHTS) {
			Scale = DrawRadarHeightMap(DIBDraw);
		} else {
			Scale = DrawRadarTextureMap(DIBDraw);
		}

		if(Scale == 0) return;

		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		if(m_HeightMap->GetNumScrollLimits()) {
			HDC	dc=(HDC)DIBDraw->GetDIBDC();
			HPEN NormalPen = CreatePen(PS_SOLID,1,RGB(255,0,255));
			HPEN OldPen = (HPEN)SelectObject(dc,NormalPen);
			
			for(std::list<CScrollLimits>::const_iterator curNode = m_HeightMap->GetScrollLimits().begin(); curNode != m_HeightMap->GetScrollLimits().end(); ++curNode)
			{
				int x0 = (curNode->MinX + OVERSCAN)*m_RadarScale;
				int y0 = (curNode->MinZ + OVERSCAN)*m_RadarScale;
				int x1 = (curNode->MaxX + OVERSCAN)*m_RadarScale;
				int y1 = (curNode->MaxZ + OVERSCAN)*m_RadarScale;

				MoveToEx(dc,x0,y0,NULL);
				LineTo(dc,x1,y0);
				LineTo(dc,x1,y1);
				LineTo(dc,x0,y1);
				LineTo(dc,x0,y0);
			}

			SelectObject(dc,OldPen);
			DeleteObject(NormalPen);
		}

		CPen Pen;
		Pen.CreatePen( PS_SOLID, 1, RGB(255,255,0) );
        CPen* pOldPen = dc->SelectObject( &Pen );

		SLONG XPos = ((((SLONG)CamPos.x)/m_TileWidth) + (m_MapWidth/2) ) * m_RadarScale;
		SLONG YPos = ((-((SLONG)CamPos.z)/m_TileWidth) + (m_MapHeight/2) ) * m_RadarScale;
		SLONG Radius = 8;

		XPos += OVERSCAN*m_RadarScale;
		YPos += OVERSCAN*m_RadarScale;

		dc->MoveTo(XPos,YPos-Radius);
		dc->LineTo(XPos,YPos+Radius+1);
		dc->MoveTo(XPos-Radius,YPos);
		dc->LineTo(XPos+Radius+1,YPos);

        dc->SelectObject( pOldPen );

		if(m_ViewFeatures) {
			m_HeightMap->DrawRadarObjects(DIBDraw,m_RadarScale);
		}
	}
}


SLONG CBTEditDoc::DrawRadarHeightMap(CDIBDraw *DIBDraw)
{
	UWORD	Step = 0;

	if(m_RadarMap) {
		DWORD	DibWidth = DIBDraw->GetDIBWidth();
		DWORD	DibHeight = DIBDraw->GetDIBHeight();
		UWORD	*DibBase = (UWORD*)DIBDraw->GetDIBBits() + OVERSCAN*m_RadarScale + OVERSCAN*m_RadarScale*DibWidth;
//		UBYTE	*MapBase = m_RadarMap;
		UWORD	*MapBase = m_RadarMap;

		DWORD	x,y;
		UWORD	*DibCurrent;
//		UBYTE	*MapCurrent;
		UWORD	*MapCurrent;

		Step = 1;
		if((DibWidth < m_MapWidth*m_RadarScale) || (DibHeight < m_MapHeight*m_RadarScale)) {
			return 0;
		}

 		PixFormatExt Format;
		m_DirectDrawView->GetDisplayPixelFormat(Format);
		UWORD RedMul = 8-Format.RedBPP;
		UWORD GreenMul = 8-Format.GreenBPP;
		UWORD BlueMul = 8-Format.BlueBPP;

		for(y=0; y<m_MapHeight; y+=Step) {
			if(DibBase) {
				DibCurrent = DibBase;
				MapCurrent = MapBase;
				for(x=0; x<m_MapWidth; x+=Step) {
//					rgb = (((UWORD)*MapCurrent>>BlueMul))<<Format.BlueShift |	   	// Blue 
//								  ((((UWORD)*MapCurrent>>GreenMul))<<Format.GreenShift) |  	// Green
//								  ((((UWORD)*MapCurrent>>RedMul))<<Format.RedShift);		// Red  
					*DibCurrent = *MapCurrent;
					if(m_RadarScale == 2) {
						*(DibCurrent+1) =  *MapCurrent;
						*(DibCurrent+1+DibWidth) = *MapCurrent;
						*(DibCurrent+DibWidth) = *MapCurrent;
					}
					DibCurrent += m_RadarScale;
					MapCurrent ++;
				}
				DibBase += DibWidth*m_RadarScale;
				MapBase += m_MapWidth;
			}
		}
		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		CPen Pen;
		Pen.CreatePen( PS_SOLID, 8, RGB(164,164,164) );
        CPen* pOldPen = dc->SelectObject( &Pen );

		dc->MoveTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,0);
		dc->LineTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);
		dc->LineTo(0,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);

        dc->SelectObject( pOldPen );
	}

	return Step;
}


void CBTEditDoc::GetRadarColours(void)
{
	int NumImages;

	if( (NumImages = m_Textures->GetNumImages()) > 0) {
		for(int i=0; i<NumImages; i++) {
			m_RadarColours[i] = m_Textures->GetImages()[i]->GetRepColour();
		}
	}
}


extern DWORD g_Types[MAXTILES];

SLONG CBTEditDoc::DrawRadarBlocking(CDIBDraw *DIBDraw)
{
	UWORD	Step = 0;
	int NumImages,Tid;

	if(DIBDraw->GetDIBBits() == NULL) {
		DebugPrint("**ERROR** Null DIBSection\n");
		return 0;
	}

	if(m_RadarMap) {
		if( (NumImages = m_Textures->GetNumImages()) != 0) {

			DWORD	DibWidth = DIBDraw->GetDIBWidth();
			DWORD	DibHeight = DIBDraw->GetDIBHeight();
			UWORD	*DibBase = (UWORD*)DIBDraw->GetDIBBits() + OVERSCAN*m_RadarScale + OVERSCAN*m_RadarScale*DibWidth;

			DWORD	x,y;
			UWORD	*DibCurrent;

			Step = 1;
			if((DibWidth < m_MapWidth*m_RadarScale) || (DibHeight < m_MapHeight*m_RadarScale)) {
				return 0;
			}

 			PixFormatExt Format;
			m_DirectDrawView->GetDisplayPixelFormat(Format);
			UWORD RedMul = 8-Format.RedBPP;
			UWORD GreenMul = 8-Format.GreenBPP;
			UWORD BlueMul = 8-Format.BlueBPP;

			CTile *Tile = m_HeightMap->GetMapTiles();
			UWORD Pix;

			for(y=0; y<m_MapHeight; y+=Step) {
				if(DibBase) {
					DibCurrent = DibBase;
					for(x=0; x<m_MapWidth; x+=Step) {
						if((Tid = Tile->TMapID-1) < NumImages) {

							
//							if((g_Types[Tid+1] == TF_TYPEWATER) || (g_Types[Tid+1] == TF_TYPECLIFFFACE)) {
							if(g_Types[Tid+1] == TF_TYPEWATER) {
								if(Tile->ZoneID == 0) {
									Pix = 0;
								} else {
									Tid = ((Tile->ZoneID)*32)%(256-64);
									Pix = DIBDraw->GetDIBValue(16,16,Tid+64);
								}
							} else if(g_Types[Tid+1] == TF_TYPECLIFFFACE) {
								Pix = DIBDraw->GetDIBValue(0,255,0);
							} else {
								if(Tile->ZoneID == 0) {
									Pix = 0;
								} else {
									Tid = ((Tile->ZoneID)*32)%(256-64);
									Pix = DIBDraw->GetDIBValue(Tid+64,Tid+64,Tid+64);
								}
							}

							*DibCurrent = Pix;	//m_RadarColours[Tid];
							if(m_RadarScale == 2) {
								*(DibCurrent+1) = Pix;	//m_RadarColours[Tid];
								*(DibCurrent+1+DibWidth) = Pix;	//m_RadarColours[Tid];
								*(DibCurrent+DibWidth) = Pix;	//m_RadarColours[Tid];
							}
						} else {
							*DibCurrent = 0;
						}
						DibCurrent += m_RadarScale;
						Tile++;
					}
					DibBase += DibWidth*m_RadarScale;
				}
			}
		}

		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		CPen Pen;
		Pen.CreatePen( PS_SOLID, 8, RGB(164,164,164) );
        CPen* pOldPen = dc->SelectObject( &Pen );

		dc->MoveTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,0);
		dc->LineTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);
		dc->LineTo(0,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);

        dc->SelectObject( pOldPen );
	}

	return Step;
}



SLONG CBTEditDoc::DrawRadarTextureMap(CDIBDraw *DIBDraw)
{
	UWORD	Step = 0;
	int NumImages,Tid;

	if(DIBDraw->GetDIBBits() == NULL) {
		DebugPrint("**ERROR** Null DIBSection\n");
		return 0;
	}

	if(m_RadarMap) {
		if( (NumImages = m_Textures->GetNumImages()) != 0) {

			DWORD	DibWidth = DIBDraw->GetDIBWidth();
			DWORD	DibHeight = DIBDraw->GetDIBHeight();
			UWORD	*DibBase = (UWORD*)DIBDraw->GetDIBBits() + OVERSCAN*m_RadarScale + OVERSCAN*m_RadarScale*DibWidth;

			DWORD	x,y;
			UWORD	*DibCurrent;

			Step = 1;
			if((DibWidth < m_MapWidth*m_RadarScale) || (DibHeight < m_MapHeight*m_RadarScale)) {
				return 0;
			}

 			PixFormatExt Format;
			m_DirectDrawView->GetDisplayPixelFormat(Format);
			UWORD RedMul = 8-Format.RedBPP;
			UWORD GreenMul = 8-Format.GreenBPP;
			UWORD BlueMul = 8-Format.BlueBPP;

			CTile *Tile = m_HeightMap->GetMapTiles();

			for(y=0; y<m_MapHeight; y+=Step) {
				if(DibBase) {
					DibCurrent = DibBase;
					for(x=0; x<m_MapWidth; x+=Step) {
						if((Tid = Tile->TMapID-1) < NumImages) {
							*DibCurrent = m_RadarColours[Tid];
							if(m_RadarScale == 2) {
								*(DibCurrent+1) = m_RadarColours[Tid];
								*(DibCurrent+1+DibWidth) = m_RadarColours[Tid];
								*(DibCurrent+DibWidth) = m_RadarColours[Tid];
							}
						} else {
							*DibCurrent = 0;
						}
						DibCurrent += m_RadarScale;
						Tile++;
					}
					DibBase += DibWidth*m_RadarScale;
				}
			}
		}

		CDC	*dc=dc->FromHandle((HDC)DIBDraw->GetDIBDC());

		CPen Pen;
		Pen.CreatePen( PS_SOLID, 8, RGB(164,164,164) );
        CPen* pOldPen = dc->SelectObject( &Pen );

		dc->MoveTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,0);
		dc->LineTo(m_MapWidth*m_RadarScale+4+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);
		dc->LineTo(0,m_MapHeight*m_RadarScale+4+OVERSCAN*m_RadarScale*2);

        dc->SelectObject( pOldPen );
	}

	return Step;
}

BOOL CBTEditDoc::InLocator(DWORD XCoord,DWORD YCoord)
{
	if(m_ShowLocators) {
		if((XCoord < GetRadarMapWidth()) && (YCoord < GetRadarMapHeight())) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CBTEditDoc::InLocatorBorder(DWORD XCoord,DWORD YCoord)
{
	if(m_ShowLocators) {
		if((XCoord < GetRadarMapWidth()) && (YCoord < GetRadarMapHeight())) {
			return TRUE;
		}
	}

	return FALSE;
}


void CBTEditDoc::EnableWireFrame(void)
{
	if(m_FillMode != FM_WIREFRAME) {
		m_FillMode = FM_WIREFRAME;
		m_DirectDrawView->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
		Update3DView();
	}
}

void CBTEditDoc::EnableTextures(void)
{
	if(m_FillMode != FM_SOLID) {
		m_FillMode = FM_SOLID;
		m_DirectDrawView->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
		Update3DView();
	}
}

void CBTEditDoc::EnableGouraud(BOOL Enable)
{
	if(m_ShadeMode != SM_GOURAUD) {
		m_ShadeMode = SM_GOURAUD;
		m_DirectDrawView->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
		Update3DView();
	} else {
		m_ShadeMode = SM_FLAT;
		m_DirectDrawView->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
		Update3DView();
	}
}



// Returns TRUE if the clipboard contains valid data.
//
BOOL CBTEditDoc::ClipboardIsValid(void)
{
	if(m_TileBuffer) {
		return TRUE;
	}

	return FALSE;
}


void CBTEditDoc::RotateClipboard(void)
{
}


void CBTEditDoc::XFlipClipboard(void)
{
}


void CBTEditDoc::YFlipClipboard(void)
{
}


// Copy all objects within a rectangle of tiles into the object clipboard.
//
// Tile0 = Index of top left tile.
// Tile1 = Index of bottom left tile.
//
void CBTEditDoc::CopyObjectRect(int Tile0,int Tile1)
{
	m_ObjectBuffer.clear();
	m_ObjectBufferSize = 0;

	int y0 = Tile0 / m_MapWidth;
	int x0 = Tile0 % m_MapWidth;
	int y1 = Tile1 / m_MapWidth;
	int x1 = Tile1 % m_MapWidth;

	if(x1 < x0) {
		int tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	if(y1 < y0) {
		int tmp = y1;
		y1 = y0;
		y0 = tmp;
	}

	ClipCoordToMap(x0,y0);
	ClipCoordToMap(x1,y1);

// Convert the tile coordinates to world coordinates.
	x0 *= m_TileWidth;
	x0 -= m_MapWidth*m_TileWidth/2;
	y0 *= m_TileHeight;
	y0 -= m_MapHeight*m_TileHeight/2;
	x1 ++;
	x1 *= m_TileWidth;
	x1 -= m_MapWidth*m_TileWidth/2;
	y1 ++;
	y1 *= m_TileHeight;
	y1 -= m_MapHeight*m_TileHeight/2;

	float tx0 = (float)x0;
	float tz0 = -(float)y1;
	float tx1 = (float)x1;
	float tz1 = -(float)y0;

// Copy all the objects within the area into the object clipboard.
	C3DObjectInstance *Instance;
	for (unsigned int i = 0; i < m_HeightMap->GetNumObjects(); ++i)
	{
		Instance = m_HeightMap->GetObjectPointer(i);
		if (Instance->Position.x >= tx0
		 && Instance->Position.z >= tz0
		 && Instance->Position.x <= tx1
		 && Instance->Position.z <= tz1)
		{
			C3DObjectInstance Data = *Instance;

			Data.Selected = FALSE;
			Data.Position.x -= tx0;
			Data.Position.z -= tz1;

			m_ObjectBuffer.push_back(Data);
			++m_ObjectBufferSize;
		}
	}
}


BOOL CBTEditDoc::WriteClipboard(char *FileName)
{
	if(m_TileBuffer)
	{
		FILE* Stream = fopen(FileName,"wb");
		if(Stream == NULL)
			return FALSE;

		ClipboardHeader Header;
		Header.Type[0] = 'c';
		Header.Type[1] = 'l';
		Header.Type[2] = 'p';
		Header.Type[3] = 'b';
		Header.Width = m_TileBufferWidth;
		Header.Height = m_TileBufferHeight;
		Header.NumObjects = m_ObjectBufferSize;

		fwrite(reinterpret_cast<const char*>(&Header), sizeof(ClipboardHeader), 1, Stream);

		CTile *SrcTile = m_TileBuffer;

		for(int y=0; y<m_TileBufferHeight; y++) {
			for(int x=0; x<m_TileBufferWidth; x++) {
				fwrite(SrcTile,sizeof(CTile),1,Stream);
				SrcTile++;
			}
		}

		for (std::list<C3DObjectInstance>::const_iterator curNode = m_ObjectBuffer.begin(); curNode != m_ObjectBuffer.end(); ++curNode)
		{
			fwrite(reinterpret_cast<const char*>(&*curNode), sizeof(*curNode), 1, Stream);
		}

		fclose(Stream);
	}
	else
	{
		MessageBox(NULL,"The clipboard is empty","Nothing to save",MB_OK);
	}

	return TRUE;
}


BOOL CBTEditDoc::ReadClipboard(char *FileName)
{
	FILE* Stream = fopen(FileName,"rb");
	if(Stream == NULL)
		return FALSE;

	ClipboardHeader Header;

	fread((void*)&Header,sizeof(ClipboardHeader),1,Stream);
	if(	(Header.Type[0] != 'c') ||
		(Header.Type[1] != 'l') ||
		(Header.Type[2] != 'p') ||
		(Header.Type[3] != 'b') ) {

		fclose(Stream);
		MessageBox(NULL,"Not a valid clipboard file","Error",MB_OK);
		return FALSE;
	}

	delete m_TileBuffer;
	m_TileBuffer = NULL;

	m_ObjectBuffer.clear();
	m_ObjectBufferSize = 0;

	m_TileBufferWidth = Header.Width;
	m_TileBufferHeight = Header.Height;
	m_ObjectBufferSize = Header.NumObjects;

	m_TileBuffer = new CTile[m_TileBufferWidth*m_TileBufferHeight];
	CTile *DstTile = m_TileBuffer;

	for (int y = 0; y < m_TileBufferHeight; ++y)
	{
		for (int x = 0; x < m_TileBufferWidth; ++x)
		{
			fread(reinterpret_cast<char*>(DstTile), sizeof(*DstTile), 1, Stream);
			++DstTile;
		}
	}

	for (unsigned int i = 0; i < m_ObjectBufferSize; ++i)
	{
		C3DObjectInstance Data;
		fread(&Data, sizeof(Data), 1, Stream);
		m_ObjectBuffer.push_back(Data);
	}

	return TRUE;
}


// Paste the current object clipboard to the map
//
// Dest  = tile index of top left corner to paste to.
//
void CBTEditDoc::PasteObjectRect(int Dest)
{
	if (m_ObjectBuffer.empty())
		return;

	int y0 = Dest / m_MapWidth;
	int x0 = Dest % m_MapWidth;

// Convert the tile coordinates to world coordinates.
	x0 *= m_TileWidth;
	x0 -= m_MapWidth*m_TileWidth/2;
	y0 *= m_TileHeight;
	y0 -= m_MapHeight*m_TileHeight/2;

	float tx0 = (float)x0;
	float tz0 = -(float)y0;

// Add all the objects in the object clipboard to the world.
	for (std::list<C3DObjectInstance>::const_iterator curNode = m_ObjectBuffer.begin(); curNode != m_ObjectBuffer.end(); ++curNode)
	{
		D3DVECTOR Position = curNode->Position;
		Position.x += tx0;
		Position.z += tz0;
		Position.y = m_HeightMap->GetHeight(Position.x,-Position.z);

		int ObjID;
		if(m_PasteWithPlayerID)
		{
			// Paste object using current player id.
			ObjID = m_HeightMap->AddObject(curNode->ObjectID, curNode->Rotation, Position, m_CurrentPlayer);
		}
		else
		{
			// Paste object using source player id.
			ObjID = m_HeightMap->AddObject(curNode->ObjectID, curNode->Rotation, Position, curNode->PlayerID);
		}
		m_HeightMap->SnapObject(ObjID);
		m_HeightMap->SetObjectTileFlags(ObjID,TF_HIDE);
		m_HeightMap->SetObjectTileHeights(ObjID);
	}
}


// Copy a rectangle of tiles into the tile clipboard.
//
// Tile0 = Index of top left tile.
// Tile1 = Index of bottom left tile.
//
void CBTEditDoc::CopyTileRect(int Tile0,int Tile1)
{
	if(m_TileBuffer) delete m_TileBuffer;
	m_TileBuffer = NULL;

	int y0 = Tile0 / m_MapWidth;
	int x0 = Tile0 % m_MapWidth;
	int y1 = Tile1 / m_MapWidth;
	int x1 = Tile1 % m_MapWidth;

	if(x1 < x0) {
		int tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	if(y1 < y0) {
		int tmp = y1;
		y1 = y0;
		y0 = tmp;
	}

	ClipCoordToMap(x0,y0);
	ClipCoordToMap(x1,y1);

	m_TileBufferWidth = (x1+1-x0);
	m_TileBufferHeight = (y1+1-y0);
	m_TileBuffer = new CTile[m_TileBufferWidth*m_TileBufferHeight];

	CTile *SrcTile = m_HeightMap->GetMapTiles()+x0+y0*m_MapWidth;
	CTile *DstTile = m_TileBuffer;

	int	x,y;

	for(y=0; y<m_TileBufferHeight; y++) {
		for(x=0; x<m_TileBufferWidth; x++) {
			*DstTile = *(SrcTile+x);
			DstTile++;
		}
		SrcTile+=m_MapWidth;
	}

	CopyObjectRect(Tile0,Tile1);
}


// Paste the current tile clipboard to the map
//
// Dest  = tile index of top left corner to paste too.
//
void CBTEditDoc::PasteTileRect(int Dest)
{
	if(m_TileBuffer) {
		int y0 = Dest / m_MapWidth;
		int x0 = Dest % m_MapWidth;
		CTile *DstTile = m_HeightMap->GetMapTiles()+x0+y0*m_MapWidth;
		CTile *SrcTile = m_TileBuffer;

		int	x,y;

		g_UndoRedo->BeginGroup();

		for(y=0; y<m_TileBufferHeight; y++) {
			for(x=0; x<m_TileBufferWidth; x++) {

				if( (x0+x < (int)m_MapWidth) && (y0+y < (int)m_MapHeight) ) {

					g_UndoRedo->AddUndo(DstTile+x);

					if(m_PasteFlags & PF_PASTETEXTURES) {
						(DstTile+x)->TMapID = SrcTile->TMapID;     
						(DstTile+x)->Flags = SrcTile->Flags;
					}

//					if(m_PasteFlags & PF_PASTEFLAGS) {
//						(DstTile+x)->Flags = SrcTile->Flags;
//					}

					if(m_PasteFlags & PF_PASTEHEIGHTS) {
						(DstTile+x)->Height[0] = SrcTile->Height[0];
						(DstTile+x)->Height[1] = SrcTile->Height[1];
						(DstTile+x)->Height[2] = SrcTile->Height[2];
						(DstTile+x)->Height[3] = SrcTile->Height[3];

						m_HeightMap->FixTileNormals(DstTile+x);
					}
				}

				SrcTile++;
			}
			DstTile+=m_MapWidth;
		}

		if(m_PasteFlags & PF_PASTEHEIGHTS) {
			StichTiles(x0,y0,x0+m_TileBufferWidth-1,y0+m_TileBufferHeight-1,TRUE);
		}

		g_UndoRedo->EndGroup();

		m_HeightMap->InitialiseSectors();

		if(m_PasteFlags & PF_PASTEOBJECTS) {
			PasteObjectRect(Dest);
		}
	}
}



void CBTEditDoc::StichTiles(int x0,int y0,int x1,int y1,BOOL AddUndo)
{
	int Width = (x1+1-x0);
	int Height = (y1+1-y0);
	int	x,y;
	CTile *Tile;

	for(y=0; y<Height; y++) {
		for(x=0; x<Width; x++) {

			Tile = m_HeightMap->GetMapTiles()+x0+x+(y0+y)*m_MapWidth;

			if((x==0) && (x0 != 0)){
				if(AddUndo) g_UndoRedo->AddUndo(Tile-1);
				m_HeightMap->SetVertexHeight(Tile-1,1,Tile->Height[0]);
				m_HeightMap->SetVertexHeight(Tile-1,2,Tile->Height[3]);
			}
			if((y==0) && (y0 != 0)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile-m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile-m_MapWidth,2,Tile->Height[1]);
				m_HeightMap->SetVertexHeight(Tile-m_MapWidth,3,Tile->Height[0]);
			}
			if((x==0) && (y==0) && (x0 != 0) && (y0 != 0)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile-1-m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile-1-m_MapWidth,2,Tile->Height[0]);
			}

			if((x==Width-1) && (x1 != m_MapWidth-1)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile+1);
				m_HeightMap->SetVertexHeight(Tile+1,0,Tile->Height[1]);
				m_HeightMap->SetVertexHeight(Tile+1,3,Tile->Height[2]);
			}
			if((y==Height-1) && (y1 != m_MapHeight-1)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile+m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile+m_MapWidth,0,Tile->Height[3]);
				m_HeightMap->SetVertexHeight(Tile+m_MapWidth,1,Tile->Height[2]);
			}
			if((x==Width-1) && (y==Height-1) && (x1 != m_MapWidth-1) && (y1 != m_MapHeight-1)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile+1+m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile+1+m_MapWidth,0,Tile->Height[2]);
			}

			if((x==0) && (y==Height-1) && (x0 != 0) && (y1 != m_MapHeight-1)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile-1+m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile-1+m_MapWidth,1,Tile->Height[3]);
			}
			if((x==Width-1) && (y==0) && (x1 != m_MapWidth-1) && (y0 != 0)) {
				if(AddUndo) g_UndoRedo->AddUndo(Tile+1-m_MapWidth);
				m_HeightMap->SetVertexHeight(Tile+1-m_MapWidth,3,Tile->Height[1]);
			}

		}
	}
}


void CBTEditDoc::XFlipTileRect(int Tile0,int Tile1)
{
	int y0 = Tile0 / m_MapWidth;
	int x0 = Tile0 % m_MapWidth;
	int y1 = Tile1 / m_MapWidth;
	int x1 = Tile1 % m_MapWidth;

	if(x1 < x0) {
		int tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	if(y1 < y0) {
		int tmp = y1;
		y1 = y0;
		y0 = tmp;
	}

	ClipCoordToMap(x0,y0);
	ClipCoordToMap(x1,y1);

	int Width = (x1+1-x0);
	int Height = (y1+1-y0);
	int	x,y;

	CTile *TileBuf = new CTile[Width];
	CTile *DstTile;

	for(y=0; y<Height; y++) {
		for(x=0; x<Width; x++) {
			TileBuf[x] = *(m_HeightMap->GetMapTiles()+x0+x+(y0+y)*m_MapWidth);
		}

		for(x=0; x<Width; x++) {
			DWORD Selected = x0+x+(y0+y)*m_MapWidth;

			DstTile = m_HeightMap->GetMapTiles()+Selected;

			D3DVECTOR	Position = DstTile->Position;
			*DstTile = TileBuf[Width-1-x];
			DstTile->Position = Position;

   			BOOL FlipX = m_HeightMap->GetTextureFlipX(Selected);
   			BOOL FlipY = m_HeightMap->GetTextureFlipY(Selected);
   			DWORD Rotate = m_HeightMap->GetTextureRotate(Selected);
   			if(Rotate & 1) {
   				m_HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
   			} else {
   				m_HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
   			}

			m_HeightMap->SetVertexFlip(Selected,m_HeightMap->GetVertexFlip(Selected) ? FALSE : TRUE);

			UBYTE Tmp;

			Tmp = DstTile->Height[0];
			DstTile->Height[0] = DstTile->Height[1];
			DstTile->Height[1] = Tmp;

			Tmp = DstTile->Height[2];
			DstTile->Height[2] = DstTile->Height[3];
			DstTile->Height[3] = Tmp;

			m_HeightMap->FixTileNormals(DstTile);
		}
	}

	delete TileBuf;

	StichTiles(x0,y0,x1,y1,FALSE);

	m_HeightMap->XFlipObjects(x0,y0,x1,y1);

	m_HeightMap->InitialiseSectors();
}


void CBTEditDoc::YFlipTileRect(int Tile0,int Tile1)
{
	int y0 = Tile0 / m_MapWidth;
	int x0 = Tile0 % m_MapWidth;
	int y1 = Tile1 / m_MapWidth;
	int x1 = Tile1 % m_MapWidth;

	if(x1 < x0) {
		int tmp = x1;
		x1 = x0;
		x0 = tmp;
	}

	if(y1 < y0) {
		int tmp = y1;
		y1 = y0;
		y0 = tmp;
	}

	ClipCoordToMap(x0,y0);
	ClipCoordToMap(x1,y1);

	int Width = (x1+1-x0);
	int Height = (y1+1-y0);
	int	x,y;

	CTile *TileBuf = new CTile[Height];
	CTile *DstTile;

	for(x=0; x<Width; x++) {
		for(y=0; y<Height; y++) {
			TileBuf[y] = *(m_HeightMap->GetMapTiles()+x0+x+(y0+y)*m_MapWidth);
		}

		for(y=0; y<Height; y++) {
			DWORD Selected = x0+x+(y0+y)*m_MapWidth;

			DstTile = m_HeightMap->GetMapTiles()+Selected;

			D3DVECTOR	Position = DstTile->Position;
			*DstTile = TileBuf[Height-1-y];
			DstTile->Position = Position;

   			BOOL FlipX = m_HeightMap->GetTextureFlipX(Selected);
   			BOOL FlipY = m_HeightMap->GetTextureFlipY(Selected);
   			DWORD Rotate = m_HeightMap->GetTextureRotate(Selected);
   			if(Rotate & 1) {
   				m_HeightMap->SetTextureFlip(Selected,!FlipX,FlipY);
   			} else {
   				m_HeightMap->SetTextureFlip(Selected,FlipX,!FlipY);
   			}

			m_HeightMap->SetVertexFlip(Selected,m_HeightMap->GetVertexFlip(Selected) ? FALSE : TRUE);

			UBYTE Tmp;

			Tmp = DstTile->Height[0];
			DstTile->Height[0] = DstTile->Height[3];
			DstTile->Height[3] = Tmp;

			Tmp = DstTile->Height[2];
			DstTile->Height[2] = DstTile->Height[1];
			DstTile->Height[1] = Tmp;

			m_HeightMap->FixTileNormals(DstTile);
		}
	}

	delete TileBuf;

	StichTiles(x0,y0,x1,y1,FALSE);

	m_HeightMap->YFlipObjects(x0,y0,x1,y1);

	m_HeightMap->InitialiseSectors();
}


void CBTEditDoc::ClipCoordToMap(int &x,int &y)
{
	if(x < 0) {
		x = 0;
	} else if(x >= (int)m_MapWidth) {
		x = m_MapWidth-1;
	}

	if(y < 0) {
		y = 0;
	} else if(y >= (int)m_MapHeight) {
		y = m_MapHeight-1;
	}
}


//DWORD BrushVersion = 1;
DWORD BrushVersion = 2;


void CBTEditDoc::OnFileSavetiletypes() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)TileTypeFilters,"ett","*.ett",FALSE,NULL,FileName,FullPath)) {
		FILE *Stream = fopen(FullPath,"wb");

		if(Stream == NULL) {
			MessageBox(NULL,"Error opening file for write.\nThe disk may be full of write protected.",FullPath,MB_OK);
			return;
		}

		WriteTileTypes(Stream);
		fclose(Stream);
	}
}

void CBTEditDoc::OnFileLoadtiletypes() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)TileTypeFilters,"ett","*.ett",TRUE,NULL,FileName,FullPath)) {
		FILE *Stream = fopen(FullPath,"rb");
		if( Stream == NULL) {
			MessageBox(NULL,"Error opening file!",FullPath,MB_OK);
			return;
		}

		ReadTileTypes(Stream);
		fclose(Stream);
	}

// Tile types have changed so need to re-initialise edge brushes.
	ReInitEdgeBrushes();
}


void CBTEditDoc::ReInitEdgeBrushes(void)
{
	for(int i=0; i<MAXEDGEBRUSHES; i++) {
		m_EdgeBrush[i]->SetBrushTextureFromMap();
	}
}


BOOL CBTEditDoc::WriteTileFlags(FILE *Stream)
{
	fprintf(Stream,"TileFlags {\n");
	fprintf(Stream,"    NumTiles %d\n",m_Textures->GetNumTiles());
	fprintf(Stream,"    Flags {\n");

	DWORD NumTiles = m_Textures->GetNumTiles();
	DWORD i=0;

	do {
		fprintf(Stream,"        ");
		for(DWORD j=0; j<16; j++) {
			fprintf(Stream,"%d ",m_Textures->GetTextureFlags(i));
			i++;
			if(i >= NumTiles) break;
		}
		fprintf(Stream,"\n");
	} while(i<NumTiles);

	fprintf(Stream,"    }\n");
	fprintf(Stream,"}\n");

	return TRUE;
}


BOOL CBTEditDoc::ReadTileFlags(FILE* Stream)
{
	LONG NumTiles;
	DWORD Type;

	fpos_t fpos;
	fgetpos(Stream, &fpos);

	if(StartChunk(Stream,"TileFlags") == FALSE) {
		fsetpos(Stream, &fpos);
		return TRUE;
	}
	CHECKTRUE(ReadLong(Stream,"NumTiles",(LONG*)&NumTiles));
	ASSERT(NumTiles == (LONG)m_Textures->GetNumTiles());
	CHECKTRUE(StartChunk(Stream,"Flags"));

	for(DWORD i=0; i<(DWORD)NumTiles; i++) {
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&Type));
		m_Textures->SetTextureFlags(i,Type);
	}

	CHECKTRUE(EndChunk(Stream));
	CHECKTRUE(EndChunk(Stream));

	InvalidateRect(NULL,NULL,NULL);

	return TRUE;
}


BOOL CBTEditDoc::WriteTileTypes(FILE* Stream)
{
	fprintf(Stream,"TileTypes {\n");
	fprintf(Stream,"    NumTiles %d\n",m_Textures->GetNumTiles());
	fprintf(Stream,"    Tiles {\n");

	DWORD NumTiles = m_Textures->GetNumTiles();
	DWORD i=0;

	do {
		fprintf(Stream,"        ");
		for(DWORD j=0; j<16; j++) {
			fprintf(Stream,"%d ",m_Textures->GetTextureType(i));
			i++;
			if(i >= NumTiles) break;
		}
		fprintf(Stream,"\n");
	} while(i<NumTiles);
	
	fprintf(Stream,"    }\n");
	fprintf(Stream,"}\n");

	return TRUE;
}


BOOL CBTEditDoc::ReadTileTypes(FILE* Stream)
{
	LONG NumTiles;
	DWORD Type;

	CHECKTRUE(StartChunk(Stream,"TileTypes"));
	CHECKTRUE(ReadLong(Stream,"NumTiles",(LONG*)&NumTiles));
	ASSERT(NumTiles == (LONG)m_Textures->GetNumTiles());
	CHECKTRUE(StartChunk(Stream,"Tiles"));

	for(DWORD i=0; i<(DWORD)NumTiles; i++) {
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&Type));
		if(Type >= TF_NUMTYPES) {
			Type = TF_DEFAULTTYPE;
		}
		m_Textures->SetTextureType(i,Type);
	}

	CHECKTRUE(EndChunk(Stream));
	CHECKTRUE(EndChunk(Stream));

	InvalidateRect(NULL,NULL,NULL);

	return TRUE;
}


void CBTEditDoc::OnFileSaveedgebrushes() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)EdgeBrushFilters,"ebr","*.ebr",FALSE,NULL,FileName,FullPath)) {
		FILE *Stream = fopen(FullPath,"wb");
		if(Stream == NULL) {
			MessageBox(NULL,"Error opening file for write.\nThe disk may be full of write protected.",FullPath,MB_OK);
			return;
		}
		WriteBrushes(Stream);
		fclose(Stream);
	}
}

void CBTEditDoc::OnFileLoadedgebrushes() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)EdgeBrushFilters,"ebr","*.ebr",TRUE,NULL,FileName,FullPath)) {
		FILE *Stream = fopen(FullPath,"rb");
		if(Stream == NULL) {
			MessageBox(NULL,"Error opening file for write.\nThe disk may be full of write protected.",FullPath,MB_OK);
			return;
		}
		ReadBrushes(Stream);
		fclose(Stream);
	}
}

BOOL CBTEditDoc::WriteBrushes(FILE* Stream)
{
	fprintf(Stream,"Brushes {\n");
	fprintf(Stream,"    Version %d\n",BrushVersion);
	fprintf(Stream,"    NumEdgeBrushes %d\n",MAXEDGEBRUSHES);
	fprintf(Stream,"    NumUserBrushes %d\n",0);

	fprintf(Stream,"    EdgeBrushes {\n");
	for(int i=0; i<MAXEDGEBRUSHES; i++) {
		m_EdgeBrush[i]->Write(Stream);
	}

	fprintf(Stream,"    }\n");

	fprintf(Stream,"}\n");

	return TRUE;
}


BOOL CBTEditDoc::ReadBrushes(FILE* Stream)
{
	LONG NumEdgeBrushes;
	LONG NumUserBrushes;
	LONG Version;

	for(int i=0; i<MAXEDGEBRUSHES; i++) {
		if(m_EdgeBrush[i]) delete m_EdgeBrush[i];
		m_EdgeBrush[i] = NULL;
	}
	for(i=0; i<MAXEDGEBRUSHES; i++) {
		m_EdgeBrush[i] = new CEdgeBrush(m_HeightMap,m_TextureWidth,m_TextureHeight,m_Textures->GetNumImages());
	}

	CHECKTRUE(StartChunk(Stream,"Brushes"));
	CHECKTRUE(ReadLong(Stream,"Version",(LONG*)&Version));
	CHECKTRUE(ReadLong(Stream,"NumEdgeBrushes",(LONG*)&NumEdgeBrushes));
	CHECKTRUE(ReadLong(Stream,"NumUserBrushes",(LONG*)&NumUserBrushes));

	CHECKTRUE(StartChunk(Stream,"EdgeBrushes"));
	for(i=0; i<MAXEDGEBRUSHES; i++) {
		if(Version == 1) {
			m_EdgeBrush[i]->ReadV1(Stream);
		} else if(Version == 2) {
			m_EdgeBrush[i]->ReadV2(Stream);
		}
	}
	CHECKTRUE(EndChunk(Stream));

	CHECKTRUE(EndChunk(Stream));

	return TRUE;
}


void ReadStatsString(FILE* Stream,char** String)
{
	int Brace = 0;
	int Size = 0;
	int Added = 0;
	fpos_t Pos;
	char Char;

// Skip any leading line feeds.
	do {
		Char = fgetc(Stream);
	} while((Char==0x0a) || (Char==0x0d));
	fseek(Stream,-1,SEEK_CUR);

	fgetpos(Stream,&Pos);

	do {
		Char = fgetc(Stream);

		switch(Char) {
			case	'{':
				Brace++;
				break;
			case	'}':
				Brace--;
				break;
			case	0x0a:
				Added++;
				break;
		}

		Size++;

	} while(Brace >= 0);

	fsetpos(Stream,&Pos);

	if(*String) delete *String;

	if(Size > 1) {
		*String = new char[Size+Added];
		DebugPrint("Allocated String#\n");

		char* Ptr = *String;
		for(int i=0; i<Size-1; i++)  {
			*Ptr = fgetc(Stream);
			if(*Ptr == 0x0a) {
				*Ptr = 0x0d;
				Ptr++;
				*Ptr = 0x0a;
			}
			Ptr++;
		}

		Size+=Added;

// Remove white space and line feeds from end of string.
		Ptr--;
		while((*Ptr==0x0a) || (*Ptr==0x0d) || (*Ptr==0x20) || (*Ptr=='\t')) {
			Ptr--;
			Size--;
		}

		if(Size <= 1) {
			delete *String;
			*String = NULL;
		} else {
			(*String)[Size-1] = 0;
		}
	} else {
		*String = NULL;
	}

	fseek(Stream,-1,SEEK_CUR);
}


void WriteStatsString(FILE* Stream,char* String,char* Indent)
{
	if(String) {
// Skip any leading line feeds.
		while((*String==0x0a) || (*String==0x0d)) {
			String++;
		}

		if(strlen(String) > 0) {
// Remove white space and line feeds from end of string.
			char *Ptr = String+strlen(String)-1;
			while((*Ptr==0x0a) || (*Ptr==0x0d) || (*Ptr==0x20) || (*Ptr=='\t')) {
				*Ptr = 0;
				if(Ptr == String) {
					break;
				}
				Ptr--;
			}

			if(strlen(String) > 0) {
				fprintf(Stream,Indent);
				for(DWORD i=0; i<strlen(String); i++) {
					if(String[i]!=0x0d) {
						fputc(String[i],Stream);
						if(String[i]==0x0a) {
							fprintf(Stream,Indent);
						}
					}
				}
				fprintf(Stream,"\n");
			}
		}
	}
}

void UpperCase(char *String)
{
	char *Ptr = String;
	while(*Ptr) {
		if((*Ptr>='a') && (*Ptr<='z')) {
			*Ptr = toupper(*Ptr);
		}
		Ptr++;
	}
}

void LowerCase(char *String)
{
	char *Ptr = String;
	while(*Ptr) {
		if((*Ptr>='A') && (*Ptr<='Z')) {
			*Ptr = tolower(*Ptr);
		}
		Ptr++;
	}
}


//void CBTEditDoc::OnTileHide() 
//{
//	m_EditTool = ET_HIDETILE;
////	EnableWireFrame();
//}
//
//void CBTEditDoc::OnUpdateTileHide(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_HIDETILE ? TRUE : FALSE);
//}

//void CBTEditDoc::OnTileShow() 
//{
//	m_EditTool = ET_SHOWTILE;
////	EnableWireFrame();
//}
//
//void CBTEditDoc::OnUpdateTileShow(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_SHOWTILE ? TRUE : FALSE);
//}

void CBTEditDoc::OnDrawEdgepaint() 
{
	m_EditTool = ET_EDGEPAINT;
	InitTextureTool();
//	EnableTextures();
	DisplayTextures(TRUE);

	// Add the brush properties dialog.
	if(m_BrushProp->GetSafeHwnd() == 0) {
		m_BrushProp->Create();
	}
}


void CBTEditDoc::SyncBrushDialog(void)
{
	if(m_BrushProp->GetSafeHwnd() != 0) {
		m_BrushProp->GetBrushData();
	}
}


void CBTEditDoc::OnUpdateDrawEdgepaint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_EDGEPAINT ? TRUE : FALSE);
}

//void CBTEditDoc::OnTypegrass() 
//{
//	m_EditTool = ET_TYPEGRASS;
////	m_CurrentType = TF_TYPEGRASS;
//}
//
//void CBTEditDoc::OnUpdateTypegrass(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_TYPEGRASS ? TRUE : FALSE);
//}

//void CBTEditDoc::OnTyperock() 
//{
//	m_EditTool = ET_TYPEROCK;
////	m_CurrentType = TF_TYPESTONE;
//}
//
//void CBTEditDoc::OnUpdateTyperock(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_TYPEROCK ? TRUE : FALSE);
//}

//void CBTEditDoc::OnTypesand() 
//{
//	m_EditTool = ET_TYPESAND;
////	m_CurrentType = TF_TYPESAND;
//}

//void CBTEditDoc::OnUpdateTypesand(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_TYPESAND ? TRUE : FALSE);
//}
//
//void CBTEditDoc::OnTypewater() 
//{
//	m_EditTool = ET_TYPEWATER;
////	m_CurrentType = TF_TYPEWATER;
//}
//
//void CBTEditDoc::OnUpdateTypewater(CCmdUI* pCmdUI) 
//{
//	pCmdUI->SetCheck(m_EditTool==ET_TYPEWATER ? TRUE : FALSE);
//}

void CBTEditDoc::OnFlipx() 
{
	m_EditTool = ET_XFLIPTILE;
}

void CBTEditDoc::OnUpdateFlipx(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_XFLIPTILE ? TRUE : FALSE);
}

void CBTEditDoc::OnFlipy() 
{
	m_EditTool = ET_YFLIPTILE;
}

void CBTEditDoc::OnUpdateFlipy(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_YFLIPTILE ? TRUE : FALSE);
}

void CBTEditDoc::OnRotate90() 
{
	m_EditTool = ET_ROTATETILE;
}

void CBTEditDoc::OnUpdateRotate90(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_ROTATETILE ? TRUE : FALSE);
}

void CBTEditDoc::OnObjectrotate() 
{
	if(m_EditTool != ET_ROTATEOBJECT) {
		m_EditTool = ET_ROTATEOBJECT;
		Update3DView();
	}
}

void CBTEditDoc::OnUpdateObjectrotate(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_ROTATEOBJECT ? TRUE : FALSE);
//	if(m_ShowBoxes) {
//		Update3DView();
//	}
}

void CBTEditDoc::OnViewEdgebrushes() 
{
	if(m_2DMode != M2D_EDGEBRUSHES) {
		m_2DMode = M2D_EDGEBRUSHES;
		m_WorldScrollX = WFView->GetScrollPos(SB_HORZ);
		m_WorldScrollY = WFView->GetScrollPos(SB_VERT);
		WFView->SetScrollPos(SB_HORZ, m_BrushScrollX, TRUE);
		WFView->SetScrollPos(SB_VERT, m_BrushScrollY, TRUE);
		m_ScrollX = m_BrushScrollX;
		m_ScrollY = m_BrushScrollY;
		InvalidateRect(NULL,NULL,NULL);
//		Update2DView();
	}

	// Add the brush properties dialog.
	if(m_BrushProp->GetSafeHwnd() == 0) {
		m_BrushProp->Create();
	}

}

void CBTEditDoc::OnUpdateViewEdgebrushes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_2DMode==M2D_EDGEBRUSHES ? TRUE : FALSE);
}

void CBTEditDoc::OnViewWorld() 
{
	if(m_2DMode != M2D_WORLD) {
		m_2DMode = M2D_WORLD;
		m_BrushScrollX = WFView->GetScrollPos(SB_HORZ);
		m_BrushScrollY = WFView->GetScrollPos(SB_VERT);
		WFView->SetScrollPos(SB_HORZ, m_WorldScrollX, TRUE);
		WFView->SetScrollPos(SB_VERT, m_WorldScrollY, TRUE);
		m_ScrollX = m_WorldScrollX;
		m_ScrollY = m_WorldScrollY;
		InvalidateRect(NULL,NULL,NULL);
//		Update2DView();
	}
}

void CBTEditDoc::OnUpdateViewWorld(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_2DMode==M2D_WORLD ? TRUE : FALSE);
}

void CBTEditDoc::OnViewLocatormaps() 
{
	m_ShowLocators = !m_ShowLocators;
	Update2DView();
	Update3DView();
}

void CBTEditDoc::OnUpdateViewLocatormaps(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ShowLocators);
}

void CBTEditDoc::OnOptionsZoomedin() 
{
	m_ScaleLocators = !m_ScaleLocators;

	if(m_ScaleLocators) {
		m_RadarScale = 2;
	} else {
		m_RadarScale = 1;
	}

	Update2DView();
	Update3DView();
}

void CBTEditDoc::OnUpdateOptionsZoomedin(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ScaleLocators);
}

void CBTEditDoc::OnViewShowheights() 
{
	m_RadarMode = RADAR_HEIGHTS;
	Update2DView();
	Update3DView();
}

void CBTEditDoc::OnUpdateViewShowheights(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_RadarMode == RADAR_HEIGHTS);
}

void CBTEditDoc::OnViewShowtextures() 
{
	m_RadarMode = RADAR_TEXTURES;
	Update2DView();
	Update3DView();
}

void CBTEditDoc::OnUpdateViewShowtextures(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_RadarMode == RADAR_TEXTURES);
}


void CBTEditDoc::OnViewSealevel() 
{
	m_ShowSeaLevel = !m_ShowSeaLevel;
	Update3DView();
}

void CBTEditDoc::OnUpdateViewSealevel(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ShowSeaLevel);
}

void CBTEditDoc::OnPaintatsealevel() 
{
	m_EditTool = ET_PAINTVERTEXSEALEVEL;
	InitVertexTool();
//	EnableTextures();
}

void CBTEditDoc::OnUpdatePaintatsealevel(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_PAINTVERTEXSEALEVEL ? TRUE : FALSE);
}


void CBTEditDoc::OnUpdateViewTextures(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_TVMode==MTV_TEXTURES ? TRUE : FALSE);
}


// Switch bottom right view to texture mode.
//
void CBTEditDoc::DisplayTextures(BOOL Redraw)
{
	if(m_TVMode != MTV_TEXTURES) {
		m_TVMode = MTV_TEXTURES;
		TextureView->SetViewScrollSize();
		TextureView->SetScrollPos(SB_HORZ, m_TexVScrollX, TRUE);
		TextureView->SetScrollPos(SB_VERT, m_TexVScrollY, TRUE);
		if(Redraw) {
			TextureView->InvalidateRect(NULL,TRUE);
		}
	}
}


void CBTEditDoc::OnViewZerocamera() 
{
// 	m_CameraRotation.x=D3DVAL(0);
   	m_CameraRotation.y=D3DVAL(0);
// 	m_CameraRotation.z=D3DVAL(0);
//	m_CameraPosition.y = GetHeightAt(m_CameraPosition)+CAM_HEIGHT;

	Invalidate3D();
}

void CBTEditDoc::OnViewZerocameraposition() 
{
   	m_CameraPosition.x=D3DVAL(0);
	m_CameraPosition.y = GetHeightAt(m_CameraPosition)+CAM_HEIGHT;
   	m_CameraPosition.z=D3DVAL(0);
	Invalidate3D();
}

void CBTEditDoc::OnViewFreecamera() 
{
	if(m_CameraMode != MCAM_FREE) {
		m_CameraMode = MCAM_FREE;
   		m_CameraRotation.x=D3DVAL(0);
   		m_CameraRotation.y=D3DVAL(0);
   		m_CameraRotation.z=D3DVAL(0);
		m_CameraPosition.y = GetHeightAt(m_CameraPosition)+512;
		Invalidate3D();
	}
}

void CBTEditDoc::OnUpdateViewFreecamera(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_CameraMode==MCAM_FREE ? TRUE : FALSE);
}

void CBTEditDoc::OnViewIsometric() 
{
	if(m_CameraMode != MCAM_ISOMETRIC) {
		m_CameraMode = MCAM_ISOMETRIC;
   		m_CameraRotation.x=D3DVAL(ISOMETRIC);
   		m_CameraRotation.y=D3DVAL(0);
   		m_CameraRotation.z=D3DVAL(0);
		m_CameraPosition.y = GetHeightAt(m_CameraPosition)+CAM_HEIGHT;
		Invalidate3D();
	}
}

void CBTEditDoc::OnUpdateViewIsometric(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_CameraMode==MCAM_ISOMETRIC ? TRUE : FALSE);
}


void CBTEditDoc::OnViewWireframe() 
{
	if(m_DrawMode != DM_WIREFRAME) {
		m_DrawMode = DM_WIREFRAME;
		EnableWireFrame();
	}
}

void CBTEditDoc::OnUpdateViewWireframe(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_DrawMode==DM_WIREFRAME ? TRUE : FALSE);
}

void CBTEditDoc::OnViewTextured() 
{
	if(m_DrawMode != DM_TEXTURED) {
		m_DrawMode = DM_TEXTURED;
		EnableTextures();
	}
}

void CBTEditDoc::OnUpdateViewTextured(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_DrawMode==DM_TEXTURED ? TRUE : FALSE);
}



void CBTEditDoc::OnViewTerraintypes() 
{
	g_OverlayTypes = !g_OverlayTypes;
	InvalidateRect(NULL,NULL,NULL);
}

void CBTEditDoc::OnUpdateViewTerraintypes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_OverlayTypes ? TRUE : FALSE);
}


void CBTEditDoc::OnViewFeatures() 
{
	m_ViewFeatures = !m_ViewFeatures;
	InvalidateRect(NULL,NULL,NULL);
}

void CBTEditDoc::OnUpdateViewFeatures(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ViewFeatures ? TRUE : FALSE);
}


void CBTEditDoc::OnHeightTilemode() 
{
	m_TileMode = TRUE;
}

void CBTEditDoc::OnUpdateHeightTilemode(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_TileMode ? TRUE : FALSE);
}

void CBTEditDoc::OnHeightVertexmode() 
{
	m_TileMode = FALSE;
}

void CBTEditDoc::OnUpdateHeightVertexmode(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_TileMode ? FALSE : TRUE);
}


void CBTEditDoc::InitVertexTool(void)
{
	m_ShowVerticies = TRUE;
	Update3DView();
}

void CBTEditDoc::InitTextureTool(void)
{
	m_ShowVerticies = FALSE;
	Update3DView();
}


BOOL CBTEditDoc::SelectAndLoadDataSet(void)
{
	char	FullPath[256];
	char	FileName[256];
	const string CurrentDir = Win::GetCurrentDirectory();

	if(GetFilePath((char*)FeatureSetFilters,"eds","*.eds",TRUE,NULL,FileName,FullPath)) {
		OnFileNew();
		LoadDataSet(FullPath);
	   	SetCurrentDirectory(CurrentDir.c_str());
		InvalidateRect(NULL,NULL,NULL);
		return TRUE;
	}

	return FALSE;
}


void CBTEditDoc::OnFileLoadfeatureset() 
{
	SelectAndLoadDataSet();
}



// If exiting then give the user a last chance to save the project.
//
BOOL CBTEditDoc::CanCloseFrame(CFrameWnd* pFrame) 
{

	int UserResp = MessageBox(NULL,
					"Do you wish to save before exiting?",
					"Un-Saved Changes!",
					MB_YESNOCANCEL);

	if(UserResp == IDCANCEL) return FALSE;

	if(UserResp == IDYES) {
		OnFileSave();
	}

	return CDocument::CanCloseFrame(pFrame);
}


void CBTEditDoc::OnUndo() 
{
//	g_UndoRedo->DumpUndo();
	g_UndoRedo->Undo();
	Update2DView();
	Update3DView();
}


void CBTEditDoc::OnRedo() 
{
//	g_UndoRedo->DumpRedo();
	g_UndoRedo->Redo();
	Update2DView();
	Update3DView();
}

void CBTEditDoc::OnViewGouraudshading() 
{
	if(m_ShadeMode != SM_GOURAUD) {
		m_HeightMap->EnableGouraud(TRUE);
		EnableGouraud(TRUE);
	} else {
		m_HeightMap->EnableGouraud(FALSE);
		EnableGouraud(FALSE);
	}
}

void CBTEditDoc::OnUpdateViewGouraudshading(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ShadeMode==SM_GOURAUD ? TRUE : FALSE);
}


void CBTEditDoc::OnViewBoundingspheres() 
{
   	if(m_ShowBoxes) {
   		m_ShowBoxes = FALSE;
		Update3DView();
   	}  else {
   		m_ShowBoxes = TRUE;
		Update3DView();
   	}
}

void CBTEditDoc::OnUpdateViewBoundingspheres(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_ShowBoxes ? TRUE : FALSE);
}

void CBTEditDoc::OnFileExportwarzonescenario() 
{
	char FileName[256];
	char PathName[256];
	char FullPath[256];
	if(GetFilePath((char*)WarzoneFilters,"gam","*.gam",FALSE,PathName,FileName,FullPath)) {
		WriteDeliveranceStart(FullPath);
	}
}

void CBTEditDoc::OnFileExportwarzonescenarioexpand() 
{
	char FileName[256];
	char PathName[256];
	char FullPath[256];
	if(GetFilePath((char*)WarzoneFilters,"gam","*.gam",FALSE,PathName,FileName,FullPath)) {
		WriteDeliveranceExpand(FullPath);
	}
}

void CBTEditDoc::OnFileExportwarzonemission() 
{
	char FileName[256];
	char PathName[256];
	char FullPath[256];
	if(GetFilePath((char*)WarzoneFilters,"gam","*.gam",FALSE,PathName,FileName,FullPath)) {
		WriteDeliveranceMission(FullPath);
	}
}


void CBTEditDoc::OnFileImportwarzonescenario() 
{
	// TODO: Add your command handler code here
	
}

void CBTEditDoc::OnViewAutoheightset() 
{
	if(m_AutoHeight) {
		m_AutoHeight = FALSE;
	} else {
		m_AutoHeight = TRUE;
	}
	
}

void CBTEditDoc::OnUpdateViewAutoheightset(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_AutoHeight ? TRUE : FALSE);
}

void CBTEditDoc::OnOptionsResettextureflags() 
{
	m_Textures->ResetTextureFlags();
	InvalidateRect(NULL,NULL,NULL);
}


void CBTEditDoc::OnEnableautosnap() 
{
   	if(m_EnableAutoSnap) {
   		m_EnableAutoSnap = FALSE;
   	}  else {
   		m_EnableAutoSnap = TRUE;
   	}

	m_HeightMap->m_NoObjectSnap = !m_EnableAutoSnap;
}

void CBTEditDoc::OnUpdateEnableautosnap(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EnableAutoSnap ? TRUE : FALSE);
}

void CBTEditDoc::OnFileSavemapsegment() 
{
	SaveSegmentDialog Dlg(NULL, 0, 0, 16, 16);

	if(Dlg.DoModal() == IDOK)
	{
		char FileName[256];
		char PathName[256];
		char FullPath[256];

		if(GetFilePath((char*)LandscapeFilters, "lnd", "*.lnd", FALSE, PathName, FileName, FullPath))
		{
			WriteProject(FullPath, Dlg.StartX(), Dlg.StartY(), Dlg.Width(), Dlg.Height());
		}
	}
}

void CBTEditDoc::OnTypebackedearth() 
{
	m_EditTool = ET_TYPEBAKEDEARTH;
	m_CurrentType = TF_TYPEBAKEDEARTH;
}

void CBTEditDoc::OnUpdateTypebackedearth(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEBAKEDEARTH ? TRUE : FALSE);
}

void CBTEditDoc::OnTypecliffface() 
{
	m_EditTool = ET_TYPECLIFFFACE;
	m_CurrentType = TF_TYPECLIFFFACE;
}

void CBTEditDoc::OnUpdateTypecliffface(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPECLIFFFACE ? TRUE : FALSE);
}

void CBTEditDoc::OnTypegreenmud() 
{
	m_EditTool = ET_TYPEGREENMUD;
	m_CurrentType = TF_TYPEGREENMUD;
}

void CBTEditDoc::OnUpdateTypegreenmud(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEGREENMUD ? TRUE : FALSE);
}

void CBTEditDoc::OnTypepinkrock() 
{
	m_EditTool = ET_TYPEPINKROCK;
	m_CurrentType = TF_TYPEPINKROCK;
}

void CBTEditDoc::OnUpdateTypepinkrock(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEPINKROCK ? TRUE : FALSE);
}

void CBTEditDoc::OnTyperedbrush() 
{
	m_EditTool = ET_TYPEREDBRUSH;
	m_CurrentType = TF_TYPEREDBRUSH;
}

void CBTEditDoc::OnUpdateTyperedbrush(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEREDBRUSH ? TRUE : FALSE);
}

void CBTEditDoc::OnTyperoad() 
{
	m_EditTool = ET_TYPEROAD;
	m_CurrentType = TF_TYPEROAD;
}

void CBTEditDoc::OnUpdateTyperoad(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEROAD ? TRUE : FALSE);
}

void CBTEditDoc::OnTyperubble() 
{
	m_EditTool = ET_TYPERUBBLE;
	m_CurrentType = TF_TYPERUBBLE;
}

void CBTEditDoc::OnUpdateTyperubble(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPERUBBLE ? TRUE : FALSE);
}

void CBTEditDoc::OnTypesand() 
{
	m_EditTool = ET_TYPESAND;
	m_CurrentType = TF_TYPESAND;
}

void CBTEditDoc::OnUpdateTypesand(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPESAND ? TRUE : FALSE);
}

void CBTEditDoc::OnTypesandybrush() 
{
	m_EditTool = ET_TYPESANDYBRUSH;
	m_CurrentType = TF_TYPESANDYBRUSH;
}

void CBTEditDoc::OnUpdateTypesandybrush(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPESANDYBRUSH ? TRUE : FALSE);
}

void CBTEditDoc::OnTypesheetice() 
{
	m_EditTool = ET_TYPESHEETICE;
	m_CurrentType = TF_TYPESHEETICE;
}

void CBTEditDoc::OnUpdateTypesheetice(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPESHEETICE ? TRUE : FALSE);
}

void CBTEditDoc::OnTypeslush() 
{
	m_EditTool = ET_TYPESLUSH;
	m_CurrentType = TF_TYPESLUSH;
}

void CBTEditDoc::OnUpdateTypeslush(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPESLUSH ? TRUE : FALSE);
}

void CBTEditDoc::OnTypewater() 
{
	m_EditTool = ET_TYPEWATER;
	m_CurrentType = TF_TYPEWATER;
}

void CBTEditDoc::OnUpdateTypewater(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_TYPEWATER ? TRUE : FALSE);
}


void CBTEditDoc::OnFileImportclipboard() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)ClipboardFilters,"clp","*.clp",TRUE,NULL,FileName,FullPath)) {
		if(!ReadClipboard(FullPath)) {
			MessageBox(NULL,"Unable to load clipboard file.","Error",MB_OK);
		}
	}
}

void CBTEditDoc::OnFileExportclipboard() 
{
	char	FullPath[256];
	char	FileName[256];

	if(GetFilePath((char*)ClipboardFilters,"clp","*.clp",FALSE,NULL,FileName,FullPath)) {
		if(!WriteClipboard(FullPath)) {
			MessageBox(NULL,"Unable to save clipboard file.","Error",MB_OK);
		}
	}
}


void CBTEditDoc::SetButtonLapse(void)
{
	m_ButtonLapse = 1;
}

BOOL CBTEditDoc::ProcessButtonLapse(void)
{
	if(m_ButtonLapse>0) {
		m_ButtonLapse--;
		return FALSE;
	}

	return TRUE;
}


BOOL CBTEditDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	_splitpath((char*)lpszPathName,Drive,Dir,FName,Ext);

	DeleteProjectName();
	m_FileName = new char[MAX_FILESTRING];
	m_PathName = new char[MAX_FILESTRING];
	m_FullPath = new char[MAX_FILESTRING];

	sprintf(m_FileName,"%s.%s",FName,Ext);
	sprintf(m_PathName,"%s%s",Drive,Dir);
	strcpy(m_FullPath,(char*)lpszPathName);

	SetCurrentDirectory(m_PathName);

	if(ReadProject(m_FullPath)) {
		SetTitle(m_FileName);
		return TRUE;
	}

	return FALSE;
}


void CBTEditDoc::OnMapEditscrolllimits() 
{
	LimitsDialog LimitsDlg(m_HeightMap->GetScrollLimits().begin(), m_HeightMap->GetScrollLimits().end(), m_HeightMap->GetMapWidth(), m_HeightMap->GetMapHeight());

	// Don't use the resulting list of scrolllimits if the user didn't press OK
	if (LimitsDlg.DoModal() != IDOK)
		return;

	m_HeightMap->SetScrollLimits(LimitsDlg.firstLimit(), LimitsDlg.lastLimit());
}


//void CBTEditDoc::OnMapImportwarzonemap() 
//{
//	char FileName[256];
//	char PathName[256];
//	char FullPath[256];
//
//	m_EnableRefresh = FALSE;
//
//	if(m_HeightMap->GetNumTextures() == 0) {
//		MessageBox( NULL, "You must load a texture set before importing a map", "Error", MB_OK );
//		return;
//	}
//
//	if(GetFilePath((char*)MapFilters,"map","*.map",TRUE,PathName,FileName,FullPath)) {
//		FILE *Stream;
//		Stream = fopen(FullPath,"rb");
//
//		if(m_HeightMap->ReadDeliveranceMap(Stream)) {
//			BuildRadarMap();
//		} else {
//			MessageBox( NULL, "Unable to import map, size may be wrong", "Error", MB_OK );
//		}
//
//		fclose(Stream);
//	}
//
//	InvalidateRect(NULL,NULL,NULL);
//	m_EnableRefresh = TRUE;
//}

void CBTEditDoc::OnMapSetplayerclanmappings() 
{
	CPlayerMap Dlg;
	if(Dlg.DoModal() != IDOK) {
		return;
	}
}

#define CAPTURE_XSTEP	(2560.0F)
#define CAPTURE_ZSTEP	(1792.0F+128.0F)

//#if(0)
//void CBTEditDoc::OnMapExportmapasbitmap() 
//{
//	BOOL OsShowLocators = m_ShowLocators;
//	D3DVECTOR OsCameraPosition = m_CameraPosition;
//	D3DVECTOR OsCameraRotation = m_CameraRotation;
//	int ImageY = 0;
//
//	m_ShowLocators = FALSE;
//	m_Captureing = TRUE;
//	m_CameraRotation.x = 90.0F;
//	m_CameraRotation.y = 0.0F;
//	m_CameraRotation.z = 0.0F;
//	m_CameraPosition.y = 2535.0F;
//	m_HeightMap->m_IgnoreDroids = TRUE;
//	m_HeightMap->m_Flatten = TRUE;
//
//	float cx,cz;
//	float ex,ez;
//
//	// Starting X.
//	cx = -(float)((m_MapWidth/2)*m_TileWidth);
//	// Starting Z.
//	cz = (float)(((m_MapHeight/2)*m_TileHeight)+m_TileHeight/2);
//	// Ending X.
//	ex = (float)((m_MapWidth/2)*m_TileWidth);
//	// Ending Z.
//	ez = -(float)((m_MapHeight/2)*m_TileHeight);
//
//	int NumX = (int)(m_MapWidth*m_TileWidth+CAPTURE_XSTEP-1)/CAPTURE_XSTEP;
//	int NumZ = (int)(m_MapHeight*m_TileHeight+CAPTURE_ZSTEP-1)/CAPTURE_ZSTEP;
//
//	int BigWidth = NumX * 640;
//	int BigHeight = NumZ * 480;
//
//	CDIBDraw *Dib = new CDIBDraw(m_3DWnd,BigWidth,BigHeight,g_WindowsIs555);
//
//	for(int i=0; i<NumZ; i++) {
//		int ImageX = 0;
//		// Starting X.
//		cx = -(float)((m_MapWidth/2)*m_TileWidth);
//		for(int j=0; j<NumX; j++) {
//			m_CameraPosition.x = cx;
//			m_CameraPosition.z = cz;
//
//			Update3DView();
//
//			HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
//
//			BitBlt((HDC)Dib->GetDIBDC(), j*640, i*480, Dib->GetDIBWidth(), Dib->GetDIBHeight(),
//					   hdc, 0, 0, SRCCOPY);
//
//			m_DirectDrawView->ReleaseBackBufferDC(hdc);
//
////			CaptureScreen(ImageX,ImageY);
//			ImageX++;
//
//			cx += CAPTURE_XSTEP;
//		}
//		ImageY ++;
//		cz  -= CAPTURE_ZSTEP;
//	}
//
//	BMPHandler *Bmp = new BMPHandler;
//	Bmp->Create(BigWidth,BigHeight,Dib->GetDIBBits(),NULL,16);
//	Bmp->WriteBMP("c:\\test.bmp",TRUE);
//	delete Bmp;
//
//	delete Dib;	
//
//	MessageBox( NULL, "Done", "Big Job", MB_OK );
//
//	m_HeightMap->m_IgnoreDroids = FALSE;
//	m_HeightMap->m_Flatten = FALSE;
//	m_CameraPosition = OsCameraPosition;
//	m_CameraRotation = OsCameraRotation;
//	m_Captureing = FALSE;
//	m_ShowLocators = OsShowLocators;
//	Update3DView();
//}
//
//#else

// Export the entire map as one BMP.
//
void CBTEditDoc::OnMapExportmapasbitmap() 
{
	BOOL OsShowLocators = m_ShowLocators;
	D3DVECTOR OsCameraPosition = m_CameraPosition;
	D3DVECTOR OsCameraRotation = m_CameraRotation;
	int ImageY = 0;

	char FileName[256];
	char PathName[256];
	char FullPath[256];
	if(GetFilePath((char*)BMPFilters,"bmp","*.bmp",FALSE,PathName,FileName,FullPath)) {

		SetCursor(g_Wait);

		// Don't show locator maps.
		m_ShowLocators = FALSE;
		// So the renderer knows were capturing.
		m_Captureing = TRUE;
		// Top down.
		m_CameraRotation.x = 90.0F;
		m_CameraRotation.y = 0.0F;
		m_CameraRotation.z = 0.0F;
		m_CameraPosition.y = 2535.0F;
		// Don't render droids.
		m_HeightMap->m_IgnoreDroids = TRUE;
		// Flatten everything to get rid of parallax effects.
		m_HeightMap->m_Flatten = TRUE;

		float cx,cz;

		// Starting X.
		cx = -(float)((m_MapWidth/2)*m_TileWidth)-4*m_TileWidth;
		// Starting Z.
		cz = (float)(((m_MapHeight/2)*m_TileHeight)+m_TileHeight/2);	// - 4*m_TileWidth;

		// Number of steps needed to traverse the entire map.
		int NumX = (int)(m_MapWidth*m_TileWidth+CAPTURE_XSTEP-1)/CAPTURE_XSTEP;
		int NumZ = (int)(m_MapHeight*m_TileHeight+CAPTURE_ZSTEP-1)/CAPTURE_ZSTEP;

		NumX++;
		NumZ++;

		// The size of the BMP were going to create.
		int BigWidth = NumX * g_MaxXRes;	//640;
		int BigHeight = NumZ * g_MaxYRes;	// 480;

		// Create a big BMP to put the bitmap in.
		BMPHandler *Bmp = new BMPHandler;
		Bmp->Create(BigWidth, BigHeight, 16);
		// Create a device context for the BMP so we can blit into it.
		HDC BmpHdc = Bmp->CreateDC(m_3DWnd);

		// Now step through the map one screen at a time
		for(int i=0; i<NumZ; i++) {
			int ImageX = 0;
			// Starting X.
			cx = -(float)((m_MapWidth/2)*m_TileWidth);
//			cx += 4*m_TileWidth;
			for(int j=0; j<NumX; j++) {
				m_CameraPosition.x = cx;
				m_CameraPosition.z = cz;

				Update3DView();

				// and blit the contents of the back buffer into the BMP.
				HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
				BitBlt(BmpHdc, j*g_MaxXRes, i*g_MaxYRes, BigWidth,BigHeight,
						   hdc, 0, 0, SRCCOPY);
				m_DirectDrawView->ReleaseBackBufferDC(hdc);

				ImageX++;

				cx += CAPTURE_XSTEP;
			}
			ImageY ++;
			cz  -= CAPTURE_ZSTEP;
		}

		// All done so delete the BMP's device context
		Bmp->DeleteDC(BmpHdc);				  
		// save it to disk.
   		Bmp->WriteBMP(FullPath);
		// and delete the BMP.
		delete Bmp;

		SetCursor(g_Pointer);
		MessageBox( NULL, "Done", "Big Job", MB_OK );

		// Restore the 3d view state.
		m_HeightMap->m_IgnoreDroids = FALSE;
		m_HeightMap->m_Flatten = FALSE;
		m_CameraPosition = OsCameraPosition;
		m_CameraRotation = OsCameraRotation;
		m_Captureing = FALSE;
		m_ShowLocators = OsShowLocators;
		Update3DView();
	}
}


//BOOL CBTEditDoc::CaptureScreen(int ImageX,int ImageY)
//{
////	CDIBDraw *DIBDraw = new CDIBDraw(m_3DWnd,m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2,m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2,g_WindowsIs555);
//
//	CDIBDraw *Dib = new CDIBDraw(m_3DWnd,640,480,g_WindowsIs555);
//	HDC hdc = (HDC)m_DirectDrawView->GetBackBufferDC();
//
//	BitBlt((HDC)Dib->GetDIBDC(), 0, 0, Dib->GetDIBWidth(), Dib->GetDIBHeight(),
//			   hdc, 0, 0, SRCCOPY);
//
//	m_DirectDrawView->ReleaseBackBufferDC(hdc);
//
//	BMPHandler *Bmp = new BMPHandler;
//	Bmp->Create(640,480,Dib->GetDIBBits(),NULL,16);
//
//	char ImageName[256];
//	sprintf(ImageName,"c:\\test%d_%d.bmp",ImageX,ImageY);
//	Bmp->WriteBMP(ImageName,TRUE);
//	
//	delete Dib;	
//	delete Bmp;
//	
//	return TRUE;
//}



/* Create Zone Maps

	giSetMapData(m_HeightMap);




*/

void CBTEditDoc::OnMapRefreshzones() 
{
	giSetMapData(m_HeightMap);
	if(!giCreateZones()) {
		MessageBox( NULL, "Error", "Max number of zones exceeded", MB_OK );
	}
	giUpdateMapZoneIDs();
	giDeleteZones();
	Update2DView();
}


void CBTEditDoc::OnMapLoadheightmap() 
{
	char	FullPath[256];

	m_EnableRefresh = FALSE;

	if(GetFilePath((char*)BitmapFilters,"pcx","*.pcx",TRUE,NULL,NULL,FullPath)) {
		if(m_HeightMap->SetHeightFromBitmap(FullPath)) {
			BuildRadarMap();
		}
	}

	InvalidateRect(NULL,NULL,NULL);
	m_EnableRefresh = TRUE;
}


void CBTEditDoc::OnMapSaveheightmap() 
{
	char FileName[256];
	char PathName[256];
	char FullPath[256];

	if(GetFilePath((char*)BitmapFilters,"pcx","*.pcx",FALSE,PathName,FileName,FullPath)) {
		m_HeightMap->WriteHeightMap(FullPath);
	}
}

void CBTEditDoc::OnMapLoadtilemap() 
{
	char	FullPath[256];

	m_EnableRefresh = FALSE;

	if(GetFilePath((char*)BitmapFilters,"pcx","*.pcx",TRUE,NULL,NULL,FullPath)) {
		if(m_HeightMap->SetTileIDsFromBitmap(FullPath)) {
			BuildRadarMap();
		}
	}

	InvalidateRect(NULL,NULL,NULL);
	m_EnableRefresh = TRUE;
}


void CBTEditDoc::OnMapSavetilemap() 
{
	char FileName[256];
	char PathName[256];
	char FullPath[256];

	if(GetFilePath((char*)BitmapFilters,"pcx","*.pcx",FALSE,PathName,FileName,FullPath)) {
		m_HeightMap->WriteTileIDMap(FullPath);
	}
}


void CBTEditDoc::OnPlayer0() 
{
	m_CurrentPlayer = 0;
}

void CBTEditDoc::OnUpdatePlayer0(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==0) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer1() 
{
	m_CurrentPlayer = 1;
}

void CBTEditDoc::OnUpdatePlayer1(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==1) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer2() 
{
	m_CurrentPlayer = 2;
}

void CBTEditDoc::OnUpdatePlayer2(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==2) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer3() 
{
	m_CurrentPlayer = 3;
}

void CBTEditDoc::OnUpdatePlayer3(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==3) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer4() 
{
	m_CurrentPlayer = 4;
}

void CBTEditDoc::OnUpdatePlayer4(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==4) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer5() 
{
	m_CurrentPlayer = 5;
}

void CBTEditDoc::OnUpdatePlayer5(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==5) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer6() 
{
	m_CurrentPlayer = 6;
}

void CBTEditDoc::OnUpdatePlayer6(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==6) ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer7() 
{
	m_CurrentPlayer = 7;
}

void CBTEditDoc::OnUpdatePlayer7(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( (m_CurrentPlayer==7) ? TRUE : FALSE);
}


void CBTEditDoc::OnPlayer0en() 
{
	m_HeightMap->m_EnablePlayers[0] = m_HeightMap->m_EnablePlayers[0] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer0en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[0] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer1en() 
{
	m_HeightMap->m_EnablePlayers[1] = m_HeightMap->m_EnablePlayers[1] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer1en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[1] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer2en() 
{
	m_HeightMap->m_EnablePlayers[2] = m_HeightMap->m_EnablePlayers[2] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer2en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[2] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer3en() 
{
	m_HeightMap->m_EnablePlayers[3] = m_HeightMap->m_EnablePlayers[3] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer3en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[3] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer4en() 
{
	m_HeightMap->m_EnablePlayers[4] = m_HeightMap->m_EnablePlayers[4] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer4en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[4] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer5en() 
{
	m_HeightMap->m_EnablePlayers[5] = m_HeightMap->m_EnablePlayers[5] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer5en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[5] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer6en() 
{
	m_HeightMap->m_EnablePlayers[6] = m_HeightMap->m_EnablePlayers[6] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer6en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[6] ? TRUE : FALSE);
}

void CBTEditDoc::OnPlayer7en() 
{
	m_HeightMap->m_EnablePlayers[7] = m_HeightMap->m_EnablePlayers[7] ? FALSE : TRUE;
	WFView->InvalidateRect(NULL,NULL);
	BTEditView->InvalidateRect(NULL,NULL);
}

void CBTEditDoc::OnUpdatePlayer7en(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_EnablePlayers[7] ? TRUE : FALSE);
}


void CBTEditDoc::GetLightDirection(D3DVECTOR *LightDir)
{
	*LightDir = m_LightRotation;
}

void CBTEditDoc::SetLightDirection(D3DVECTOR *LightDir)
{
	D3DVECTOR LightDirection;

	m_LightRotation = *LightDir;
	m_DirectMaths->DirectionVector(m_LightRotation,LightDirection);
	m_DirectMaths->SetLightDirection(m_LightID,&LightDirection);
}

void CBTEditDoc::OnOptionsUsenames() 
{
	m_HeightMap->m_UseRealNames = m_HeightMap->m_UseRealNames ? FALSE : TRUE;
	InvalidateRect(NULL,NULL,FALSE);
}

void CBTEditDoc::OnUpdateOptionsUsenames(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_HeightMap->m_UseRealNames ? TRUE : FALSE);
}

void CBTEditDoc::OnMarkrect() 
{
	m_EditTool = ET_MARKRECT;
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnUpdateMarkrect(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_EditTool==ET_MARKRECT ? TRUE : FALSE);
}

void CBTEditDoc::OnPaste() 
{
	m_EditTool = ET_PASTERECT;
//	m_PasteFlags = PF_PASTEALL;
	DisplayTextures(TRUE);
}

void CBTEditDoc::OnUpdatePaste(CCmdUI* pCmdUI) 
{
	if(m_TileBuffer != NULL) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck((m_EditTool==ET_PASTERECT)&&(m_PasteFlags==PF_PASTETEXTURES) ? TRUE : FALSE);
	} else {
		pCmdUI->Enable(FALSE);
	}
}

void CBTEditDoc::OnXflipmarked() 
{
	if(m_HeightMap->SelectionBoxValid()) {
		XFlipTileRect(m_HeightMap->GetSelectionBox0(),m_HeightMap->GetSelectionBox1());
		WFView->InvalidateRect(NULL,NULL);
		BTEditView->InvalidateRect(NULL,NULL);
	}
}

void CBTEditDoc::OnUpdateXflipmarked(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_HeightMap->SelectionBoxValid());
}

void CBTEditDoc::OnYflipmarked() 
{
	if(m_HeightMap->SelectionBoxValid()) {
		YFlipTileRect(m_HeightMap->GetSelectionBox0(),m_HeightMap->GetSelectionBox1());
		WFView->InvalidateRect(NULL,NULL);
		BTEditView->InvalidateRect(NULL,NULL);
	}
}

void CBTEditDoc::OnUpdateYflipmarked(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_HeightMap->SelectionBoxValid());
}

void CBTEditDoc::OnCopymarked() 
{
	if(m_HeightMap->SelectionBoxValid()) {
		CopyTileRect(m_HeightMap->GetSelectionBox0(),m_HeightMap->GetSelectionBox1());
	}
}

void CBTEditDoc::OnUpdateCopymarked(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_HeightMap->SelectionBoxValid());
}

void CBTEditDoc::OnPasteprefs() 
{
	CPastePrefs PastePrefsDlg;

	PastePrefsDlg.m_Heights = (m_PasteFlags & PF_PASTEHEIGHTS) ? TRUE : FALSE;
	PastePrefsDlg.m_Textures = (m_PasteFlags & PF_PASTETEXTURES) ? TRUE : FALSE;
	PastePrefsDlg.m_Objects = (m_PasteFlags & PF_PASTEOBJECTS) ? TRUE : FALSE;

	if(PastePrefsDlg.DoModal() == IDOK) {
		if(PastePrefsDlg.m_Heights == TRUE) {
			m_PasteFlags |= PF_PASTEHEIGHTS;
		} else {
			m_PasteFlags &= ~PF_PASTEHEIGHTS;
		}
		if(PastePrefsDlg.m_Textures == TRUE) {
			m_PasteFlags |= PF_PASTETEXTURES;
		} else {
			m_PasteFlags &= ~PF_PASTETEXTURES;
		}
		if(PastePrefsDlg.m_Objects == TRUE) {
			m_PasteFlags |= PF_PASTEOBJECTS;
		} else {
			m_PasteFlags &= ~PF_PASTEOBJECTS;
		}
	}
}

void CBTEditDoc::OnUpdatePasteprefs(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CBTEditDoc::OnUpdateFileExportclipboard(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_TileBuffer != NULL);
}

std::string EditorDataFileName(const std::string& fileName)
{
	char	Drive[256];
	char	Dir[256];
	char	FName[256];
	char	Ext[256];
	char	AltName[512];

	// Try the data directory.
	_splitpath(fileName.c_str(), Drive, Dir, FName, Ext);
	snprintf(AltName, sizeof(AltName), "%s\\data\\%s%s", g_HomeDirectory.c_str(), FName, Ext);

	return std::string(AltName);
}

// Open a file, if the file is'nt in the current directory then tries the editors
// data directory.
//
FILE *OpenEditorFile(const char* fileName)
{
	FILE *Stream;

	// Try the current directory.
	Stream = fopen(fileName,"rb");

	if(Stream == NULL)
		Stream = fopen(EditorDataFileName(fileName).c_str(), "rb");

	return Stream;
}

void CBTEditDoc::OnDrawbrushfill() 
{
	m_EditTool = ET_EDGEFILL;
	InitTextureTool();
	DisplayTextures(TRUE);

	// Add the brush properties dialog.
	if(m_BrushProp->GetSafeHwnd() == 0) {
		m_BrushProp->Create();
	}
}

void CBTEditDoc::OnUpdateDrawbrushfill(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_EditTool==ET_EDGEFILL ? TRUE : FALSE);
}
