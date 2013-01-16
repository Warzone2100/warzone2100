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

#ifndef __INCLUDED_BTEDITDOC_H__
#define __INCLUDED_BTEDITDOC_H__

// BTEditDoc.h : interface of the CBTEditDoc class
//
/////////////////////////////////////////////////////////////////////////////

//#include "DDView.h"
#include "directx.h"
#include "geometry.h"

#include "ddimage.h"
#include "heightmap.h"
//#include "Object3D.h"
#include "pcxhandler.h"
#include "textsel.h"
#include <list>
#include "brush.h"
#include "chnkio.h"
#include "tiletypes.h"

#include "brushprop.h"

#define EBVIEW_YOFFSET 32

//#include "DataTypes.h"

//#define RSTEP (2)

//#define OVERSCAN	(4)
#define DEFAULT_TEXTURESIZE	(64)	//(128)
#define DEFAULT_TILESIZE	(128)
#define DEFAULT_TILESIZE	(128)
#define DEFAULT_MAPSIZE		(64)
#define DEFAULT_MAPSIZE		(64)
#define DEFAULT_RADARSCALE	(2)
#define DEFAULT_HEIGHTSCALE (2)

#define CAM_HEIGHT	(820)
//#define CAM_HEIGHT	(500)	//(1000)	//(2200)
#define ISOMETRIC	(44)	//(60)

//#define	MAXTEXTURELIST	16
#define MAXEDGEBRUSHES	16


enum {
	FM_SOLID,
	FM_WIREFRAME,
	FM_POINTS,
};

enum {
	DM_WIREFRAME,
	DM_TEXTURED
};

enum {
	SM_FLAT,
	SM_GOURAUD
};

// Possible values for currently selected 2D mode.
enum {
	M2D_WORLD,
	M2D_EDGEBRUSHES,
	M2D_BRUSHES
};

enum {
	MTV_TEXTURES,
	MTV_STATS,
	MTV_DEPLOYMENTS
};

enum {
	MCAM_FREE,
	MCAM_ISOMETRIC
};

enum {
	RADAR_TEXTURES,
	RADAR_HEIGHTS
};

// Possible values for currently selected tool.
enum {
	ET_GET,
	ET_PAINT,
	ET_EDGEPAINT,
	ET_EDGEFILL,
	ET_BRUSHPAINT,
	ET_FILL,
	ET_GATEWAY,
	ET_GETVERTEX,
	ET_PAINTVERTEX,
	ET_PAINTVERTEXSEALEVEL,
	ET_DRAGVERTEX,
	ET_SELECTRECT,
	ET_SELECTPOINT,
	ET_MARKRECT,
	ET_COPYRECT,
	ET_PASTERECT,
	ET_TRIDIR1,
	ET_TRIDIR2,
	ET_PLACEOBJECT,
	ET_SELECTOBJECT,
	ET_DESELECTOBJECT,
	ET_DRAGOBJECT,
	ET_GETOBJECTHEIGHT,
	ET_ROTATEOBJECT,
	ET_HIDETILE,
	ET_SHOWTILE,

	ET_TYPESFIRST,
	ET_TYPESAND = ET_TYPESFIRST,
	ET_TYPESANDYBRUSH,
	ET_TYPEBAKEDEARTH,
	ET_TYPEGREENMUD,
	ET_TYPEREDBRUSH,
	ET_TYPEPINKROCK,
	ET_TYPEROAD,
	ET_TYPEWATER,
	ET_TYPECLIFFFACE,
	ET_TYPERUBBLE,
	ET_TYPESHEETICE,
	ET_TYPESLUSH,
	ET_TYPESLAST = ET_TYPESLUSH,

	ET_ROTATETILE,
	ET_XFLIPTILE,
	ET_YFLIPTILE,

	ET_POINT,
	ET_MOVE,
};

// Flags for defining type of paste operation.
#define	PF_PASTETEXTURES	1
#define	PF_PASTEFLAGS		2
#define PF_PASTEHEIGHTS		4
#define PF_PASTEOBJECTS		8
#define	PF_MATCHEDGES		16
#define PF_PASTEALL			0xffff

// Possible values for snap type.
//enum {
//	SNAP_NONE,
//	SNAP_CENTER,
//	SNAP_TOPLEFT,
//	SNAP_TOPRIGHT,
//	SNAP_BOTTOMLEFT,
//	SNAP_BOTTOMRIGHT,
//	SNAP_QUAD,
//	SNAP_CUSTOM,
//};

//#define MAXOBJECTINSTANCES	16


struct ClipboardHeader {
	char Type[4];	// Validation string.
	int Width;		// Width in tiles.
	int Height;		// Height in tiles.
	int NumObjects;	// Number of 3d objects.
};


extern int g_CursorTileX;
extern int g_CursorTileZ;


class CBTEditDoc : public CDocument,CChnkIO
{
protected:
	DWORD m_CurrentPlayer;
	int m_RadarScale;
	int m_RadarMode;
	UWORD m_RadarColours[8192];	// Should be allocated.

	BOOL m_ViewFeatures;
	BOOL m_ShowVerticies;
	char** m_UnitIDs;
	int m_CameraMode;
	void DisplayTextures(BOOL Redraw);
	void DisplayStats(BOOL Redraw);
	void DisplayDeployments(BOOL Redraw);
	BOOL LoadDataTypes(void);
	CSize m_TexVScrollSize;
	int m_TexVScrollX;
	int m_TexVScrollY;
	int m_BrushScrollY;
	int m_BrushScrollX;
	int m_WorldScrollY;
	int m_WorldScrollX;
	BOOL m_EnableRefresh;
	DWORD m_EditTool;
	DWORD m_PasteFlags;
	BOOL m_2DGotFocus;
	BOOL m_3DGotFocus;
	BOOL m_AutoSync;
	BOOL m_AutoHeight;
	CBrushProp *m_BrushProp;
	CBTEditDoc();
	DECLARE_DYNCREATE(CBTEditDoc)

// Attributes
public:
	BOOL m_Captureing;
	BOOL m_EnableAutoSnap;
	BOOL m_ShowBoxes;
	int m_SelObject;
	BOOL m_SelIsTexture;
	int m_LightID;
	BOOL m_TileMode;
	D3DVECTOR m_LightRotation;

	int m_TileBufferWidth;
	int m_TileBufferHeight;
	CTile *m_TileBuffer;
	BOOL m_PasteWithPlayerID;

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBTEditDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBTEditDoc();
	//	CForceList *GetDeploymentList(void) { return m_Deployments; }

	BOOL GetAutoHeight(void) { return m_AutoHeight; }
	int GetRadarScale(void) { return m_RadarScale; }
	char** GetTextureList(void);
	DWORD GetTextureWidth(void) { return m_TextureWidth; }
	DWORD GetTextureHeight(void) { return m_TextureHeight; }
	DWORD GetMapWidth(void) { return m_MapWidth; }
	DWORD GetMapHeight(void) { return m_MapHeight; }
	DWORD GetTileWidth(void) { return m_TileWidth; }
	DWORD GetTileHeight(void) { return m_TileHeight; }
	CHeightMap *GetHeightMap(void) { return m_HeightMap; }
	CTextureSelector *GetTextureSelector(void) { return m_Textures; }

	DWORD m_SelTexture;	// The currently selected texture.
	DWORD m_SelType;	// The currently selected texture's type.
	DWORD m_SelFlags;	// The currently selected texture's flags.
	DWORD m_CurrentType;	// The last texture type specified in the tool bar.

	DWORD GetSelectedType(void) { return m_CurrentType; }

	void InitialiseData(void);
	void DeleteData(void);

	void UpdateStatusBar(void);
	void Update3DView(HWND hWnd,HWND OwnerhWnd,
					  D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
	void Update3DView(void);
	int SelectFace(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD XPos,DWORD YPos);
	int Select2DFace(DWORD XPos,DWORD YPos,int ScrollX,int ScrollY);
	void ConvertCoord2d3d(SLONG &XPos,SLONG &YPos,SLONG ScrollX,SLONG ScrollY);
	float GetHeightAt(D3DVECTOR &Position);
	void DoGridSnap(D3DVECTOR &Position,int SnapMode = -1);
	void SetTextureID(DWORD Selected,DWORD TextureID);
	void FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags);
	void RedrawFace(HWND hWnd,DWORD Selected,D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
	void RedrawFace(DWORD Selected);

	void SetEditToolCursor(void);
	DWORD GetEditTool(void) { return m_EditTool; }

	void UpdateTextureView(HWND hWnd,HWND OwnerhWnd,SLONG ScrollX,SLONG ScrollY);
	void UpdateTextureView(void);
	DWORD GetTextureViewWidth(HWND hWnd);
	DWORD GetTextureViewHeight(HWND hWnd);
	BOOL SelectTexture(SLONG XPos,SLONG YPos);

	void Update2DView(void);
	void Update2DView(int Dest);
	void Update2DView(int Dest,int Corner0,int Corner1);
	void Update2DView(HWND hWnd,HWND OwnerhWnd,
								   SLONG ScrollX,SLONG ScrollY);

	DWORD Get2DViewWidth(void);
	DWORD Get2DViewHeight(void);

	BOOL WriteProject(char *FileName,UWORD StartX,UWORD StartY,UWORD Width,UWORD Height);
	BOOL WriteProject(char *FileName);
	BOOL WriteProjectBinary(char *FileName);
	BOOL ReadProject(char *FileName);
	BOOL WriteObjectDB(FILE *Stream);
	BOOL WriteDeployments(FILE *Stream);
	BOOL ReadDeployments(FILE *Stream);
//	BOOL ReadObjectDB(FILE* Stream);
	BOOL GetFilePath(char *FilterList,char *ExtType,char *Filter,BOOL Open,
							 char *PathName,char *FileName,char *FullPath);

	void Register3DWindow(HWND hWnd);
	void RegisterCamera(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
	void GetCamera(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition);
	void Register2DWindow(HWND hWnd);
	void RegisterScrollPosition(SLONG ScrollX,SLONG ScrollY);
	void GetScrollPosition(SLONG *ScrollX,SLONG *ScrollY);

	void BuildRadarMap(void);
	void DrawRadarMap2D(CDIBDraw *DIBDraw,DWORD XPos,DWORD YPos);
	void DrawRadarMap3D(CDIBDraw *DIBDraw,D3DVECTOR &CamPos);
	void GetRadarColours(void);
	SLONG DrawRadarHeightMap(CDIBDraw *DIBDraw);
	SLONG DrawRadarBlocking(CDIBDraw *DIBDraw);
	SLONG DrawRadarTextureMap(CDIBDraw *DIBDraw);
	DWORD GetRadarMapWidth(void) { return m_MapWidth*m_RadarScale+8+OVERSCAN*m_RadarScale*2; }
	DWORD GetRadarMapHeight(void) { return m_MapHeight*m_RadarScale+8+OVERSCAN*m_RadarScale*2; }
	BOOL InLocator(DWORD XCoord,DWORD YCoord);
	BOOL InLocatorBorder(DWORD XCoord,DWORD YCoord);
//	void DrawObjects3D(void);
//	void DrawObjects2D(CDIBDraw *DIBDraw,int ScrollX,int ScrollY);
	void EnableWireFrame(void);
	void EnableTextures(void);
	void EnableGouraud(BOOL Enable);

	BOOL WriteClipboard(char *FileName);
	BOOL ReadClipboard(char *FileName);
	void CopyObjectRect(int Tile0,int Tile1);
	void PasteObjectRect(int Dest);
	void CopyTileRect(int Tile0,int Tile1);
	void PasteTileRect(int Dest);
	BOOL ClipboardIsValid(void);
	void RotateClipboard(void);
	void XFlipClipboard(void);
	void YFlipClipboard(void);

	void ClipCoordToMap(int &x,int &y);
	void CenterCamera(void);

	BOOL GetAutoSync(void) { return m_AutoSync; }
	BOOL GetViewFeatures(void) { return m_ViewFeatures; }

//	CObjectDB* GetSelectedObject(void);
//	BOOL ObjectAlreadyLoaded(char *Name);
	CGeometry* GetDirectMaths(void) { return m_DirectMaths; }
	CDirectDraw* GetDirect3D(void) { return m_DirectDrawView; }

	void SelectAllObjects(void);
	void DeSelectAllObjects(void);
	void RemoveSelectedObjects(void);
	BOOL GetGravity(void) { return m_EnableGravity; }
	SLONG GetRotationSnap(void) { return m_RotSnap; }
	DWORD Get2DMode(void) { return m_2DMode; }
	void Set2DMode(DWORD Mode) { m_2DMode = Mode; }

	DWORD GetTVMode(void) { return m_TVMode; }
	void SetTVMode(DWORD Mode) { m_TVMode = Mode; }

	int GetCameraMode(void) { return m_CameraMode; }

	int GetEdgeBrushID(void) { return m_CurrentEdgeBrush; }
	CEdgeBrush* GetEdgeBrush(void) { return m_EdgeBrush[m_CurrentEdgeBrush]; }
	CBrushProp* GetBrushProp(void) { return	m_BrushProp; }
	CEdgeBrush* SetEdgeBrush(int ID);
	CEdgeBrush* NextEdgeBrush(void);
	CEdgeBrush* PrevEdgeBrush(void);
	BOOL WriteTileFlags(FILE *Stream);
	BOOL ReadTileFlags(FILE* Stream);
	BOOL WriteTileTypes(FILE* Stream);
	BOOL ReadTileTypes(FILE* Stream);
	BOOL WriteBrushes(FILE* Stream);
	BOOL ReadBrushes(FILE* Stream);
	void SyncBrushDialog(void);

	void Set2DViewPos(void);
	void Set3DViewPos(void);

	void Invalidate2D(void);
	void Invalidate3D(void);

//	void SetStatsView(CObjectDB *ObjectDB,int Instance=-1);

	void Set2DFocus(BOOL Focus) { m_2DGotFocus = Focus; }
	void Set3DFocus(BOOL Focus) { m_3DGotFocus = Focus; }

//	void BuildIDList(void);
//	char **GetIDList(void) { return m_UnitIDs; }
//	void FixForceListIndecies(void);

	void DisplayExportSummary(void);
	BOOL WriteDeliveranceStart(char *FileName);
	BOOL WriteDeliveranceExpand(char *FileName);
	BOOL WriteDeliveranceMission(char *FileName);

	BOOL WriteProjectNecromancer(char *FileName);

	void InitVertexTool(void);
	void InitTextureTool(void);

	void ReInitEdgeBrushes(void);

	void GetLightRotation(D3DVECTOR &Rotation);
	void SetLightRotation(D3DVECTOR &Rotation);

	DWORD GetCurrentPlayer(void) { return m_CurrentPlayer; }
	void SetCurrentPlayer(DWORD Player) { m_CurrentPlayer = Player; }

	void SetButtonLapse(void);
	BOOL ProcessButtonLapse(void);

	BOOL CaptureScreen(int ImageX,int ImageY);
	BOOL SelectAndLoadDataSet(void);
	BOOL LoadDataSet(char *FullPath);
	void GetLightDirection(D3DVECTOR *LightDir);
	void SetLightDirection(D3DVECTOR *LightDir);
	void StichTiles(int x0,int y0,int x1,int y1,BOOL AddUndo);
	void XFlipTileRect(int Tile0,int Tile1);
	void YFlipTileRect(int Tile0,int Tile1);
private:
	void LoadTextures(char *FileName);
	void DeleteProjectName(void);
	void CreateDefaultTextureList(void);
	void DeleteTextureList(void);

	char m_DataSetName[256];
	int m_ButtonLapse;
	DWORD m_FillMode;

	BOOL	m_ViewIsInitialised;
	CDirectDraw *m_DirectDrawView;
	CGeometry *m_DirectMaths;
	CMatManager *m_MatManager;
	PCXHandler *m_PCXBitmap;
	PCXHandler *m_PCXTexBitmap;
	LPDIRECTDRAWSURFACE m_TextureSurface;
	LPDIRECTDRAWSURFACE m_SpriteSurface;
	LPDIRECTDRAWSURFACE m_TextSurface;
	DDImage* m_SpriteImage;
	DDImage* m_TextImage;

	HWND m_3DWnd;
	D3DVECTOR m_CameraPosition;
	D3DVECTOR m_CameraRotation;

	HWND m_2DWnd;
	SLONG m_ScrollX;
	SLONG m_ScrollY;

	char *m_FileName;
	char *m_PathName;
	char *m_FullPath;
	CHeightMap* m_HeightMap;
	CTextureSelector *m_Textures;
	DWORD m_MapWidth;
	DWORD m_MapHeight;
	DWORD m_TileWidth;
	DWORD m_TileHeight;
	DWORD m_TextureWidth;
	DWORD m_TextureHeight;
	DWORD m_HeightScale;

	HCURSOR m_PointerDraw;
	HCURSOR m_PointerFill;

	HWND m_TVhWnd;
	SLONG m_TVScrollX;
	SLONG m_TVScrollY;

//	UBYTE *m_RadarMap;
	UWORD *m_RadarMap;

	int m_ObjectBufferSize;
	std::list<C3DObjectInstance> m_ObjectBuffer;

//	CDatabaseTypes *m_DatabaseTypes;

	BOOL m_EnableGravity;
	DWORD m_SnapX;
	DWORD m_SnapZ;
	DWORD m_SnapMode;
	SLONG m_RotSnap;

	DWORD m_2DMode;
	BOOL m_ShowLocators;
//	BOOL m_ShowObjects;
	BOOL m_ShowSeaLevel;
	CEdgeBrush* m_EdgeBrush[16];
	DWORD m_CurrentEdgeBrush;

	DWORD m_TVMode;
	DWORD m_DrawMode;
	DWORD m_ShadeMode;

// Generated message map functions
protected:
	BOOL m_ScaleLocators;
	//{{AFX_MSG(CBTEditDoc)
	afx_msg void OnMapSetmapsize();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileNew();
	afx_msg void OnMapLoadheightmap();
	afx_msg void OnDrawFill();
	afx_msg void OnDrawPaint();
	afx_msg void OnSelectArea();
	afx_msg void OnSelectPoint();
	afx_msg void OnHeightDrag();
	afx_msg void OnUpdateDrawPaint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateHeightDrag(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSelectArea(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSelectPoint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDrawFill(CCmdUI* pCmdUI);
	afx_msg void OnDrawGet();
	afx_msg void OnUpdateDrawGet(CCmdUI* pCmdUI);
	afx_msg void OnHeightPaint();
	afx_msg void OnUpdateHeightPaint(CCmdUI* pCmdUI);
	afx_msg void OnHeightPick();
	afx_msg void OnUpdateHeightPick(CCmdUI* pCmdUI);
	afx_msg void OnTridir1();
	afx_msg void OnUpdateTridir1(CCmdUI* pCmdUI);
	afx_msg void OnDrawEdgepaint();
	afx_msg void OnUpdateDrawEdgepaint(CCmdUI* pCmdUI);
	afx_msg void OnViewEdgebrushes();
	afx_msg void OnUpdateViewEdgebrushes(CCmdUI* pCmdUI);
	afx_msg void OnViewWorld();
	afx_msg void OnUpdateViewWorld(CCmdUI* pCmdUI);
	afx_msg void OnViewLocatormaps();
	afx_msg void OnUpdateViewLocatormaps(CCmdUI* pCmdUI);
	afx_msg void OnViewSealevel();
	afx_msg void OnUpdateViewSealevel(CCmdUI* pCmdUI);
	afx_msg void OnPaintatsealevel();
	afx_msg void OnUpdatePaintatsealevel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewTextures(CCmdUI* pCmdUI);
	afx_msg void OnViewZerocamera();
	afx_msg void OnViewZerocameraposition();
	afx_msg void OnViewFreecamera();
	afx_msg void OnUpdateViewFreecamera(CCmdUI* pCmdUI);
	afx_msg void OnViewIsometric();
	afx_msg void OnUpdateViewIsometric(CCmdUI* pCmdUI);
	afx_msg void OnViewWireframe();
	afx_msg void OnUpdateViewWireframe(CCmdUI* pCmdUI);
	afx_msg void OnViewTextured();
	afx_msg void OnUpdateViewTextured(CCmdUI* pCmdUI);
	afx_msg void OnFlipx();
	afx_msg void OnUpdateFlipx(CCmdUI* pCmdUI);
	afx_msg void OnFlipy();
	afx_msg void OnUpdateFlipy(CCmdUI* pCmdUI);
	afx_msg void OnRotate90();
	afx_msg void OnUpdateRotate90(CCmdUI* pCmdUI);
	afx_msg void OnViewTerraintypes();
	afx_msg void OnUpdateViewTerraintypes(CCmdUI* pCmdUI);
	afx_msg void OnHeightTilemode();
	afx_msg void OnUpdateHeightTilemode(CCmdUI* pCmdUI);
	afx_msg void OnHeightVertexmode();
	afx_msg void OnUpdateHeightVertexmode(CCmdUI* pCmdUI);
	afx_msg void OnPoint();
	afx_msg void OnUpdatePoint(CCmdUI* pCmdUI);
	afx_msg void OnMove();
	afx_msg void OnUpdateMove(CCmdUI* pCmdUI);
	afx_msg void OnObjectrotate();
	afx_msg void OnUpdateObjectrotate(CCmdUI* pCmdUI);
	afx_msg void OnViewFeatures();
	afx_msg void OnUpdateViewFeatures(CCmdUI* pCmdUI);
	afx_msg void OnViewShowheights();
	afx_msg void OnUpdateViewShowheights(CCmdUI* pCmdUI);
	afx_msg void OnViewShowtextures();
	afx_msg void OnUpdateViewShowtextures(CCmdUI* pCmdUI);
	afx_msg void OnFileLoadfeatureset();
	afx_msg void OnFileSaveedgebrushes();
	afx_msg void OnFileLoadedgebrushes();
	afx_msg void OnFileSavetiletypes();
	afx_msg void OnFileLoadtiletypes();
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnViewGouraudshading();
	afx_msg void OnUpdateViewGouraudshading(CCmdUI* pCmdUI);
	afx_msg void OnViewBoundingspheres();
	afx_msg void OnUpdateViewBoundingspheres(CCmdUI* pCmdUI);
	afx_msg void OnFileExportwarzonescenario();
	afx_msg void OnFileImportwarzonescenario();
	afx_msg void OnMapSaveheightmap();
	afx_msg void OnViewAutoheightset();
	afx_msg void OnUpdateViewAutoheightset(CCmdUI* pCmdUI);
	afx_msg void OnOptionsResettextureflags();
	afx_msg void OnPlayer1();
	afx_msg void OnUpdatePlayer1(CCmdUI* pCmdUI);
	afx_msg void OnPlayer2();
	afx_msg void OnUpdatePlayer2(CCmdUI* pCmdUI);
	afx_msg void OnPlayer3();
	afx_msg void OnUpdatePlayer3(CCmdUI* pCmdUI);
	afx_msg void OnPlayer4();
	afx_msg void OnUpdatePlayer4(CCmdUI* pCmdUI);
	afx_msg void OnPlayer5();
	afx_msg void OnUpdatePlayer5(CCmdUI* pCmdUI);
	afx_msg void OnPlayer6();
	afx_msg void OnUpdatePlayer6(CCmdUI* pCmdUI);
	afx_msg void OnPlayer7();
	afx_msg void OnUpdatePlayer7(CCmdUI* pCmdUI);
	afx_msg void OnPlayer0();
	afx_msg void OnUpdatePlayer0(CCmdUI* pCmdUI);
	afx_msg void OnEnableautosnap();
	afx_msg void OnUpdateEnableautosnap(CCmdUI* pCmdUI);
	afx_msg void OnFileSavemapsegment();
	afx_msg void OnTypebackedearth();
	afx_msg void OnUpdateTypebackedearth(CCmdUI* pCmdUI);
	afx_msg void OnTypecliffface();
	afx_msg void OnUpdateTypecliffface(CCmdUI* pCmdUI);
	afx_msg void OnTypegreenmud();
	afx_msg void OnUpdateTypegreenmud(CCmdUI* pCmdUI);
	afx_msg void OnTypepinkrock();
	afx_msg void OnUpdateTypepinkrock(CCmdUI* pCmdUI);
	afx_msg void OnTyperedbrush();
	afx_msg void OnUpdateTyperedbrush(CCmdUI* pCmdUI);
	afx_msg void OnTyperoad();
	afx_msg void OnUpdateTyperoad(CCmdUI* pCmdUI);
	afx_msg void OnTyperubble();
	afx_msg void OnUpdateTyperubble(CCmdUI* pCmdUI);
	afx_msg void OnTypesand();
	afx_msg void OnUpdateTypesand(CCmdUI* pCmdUI);
	afx_msg void OnTypesandybrush();
	afx_msg void OnUpdateTypesandybrush(CCmdUI* pCmdUI);
	afx_msg void OnTypesheetice();
	afx_msg void OnUpdateTypesheetice(CCmdUI* pCmdUI);
	afx_msg void OnTypeslush();
	afx_msg void OnUpdateTypeslush(CCmdUI* pCmdUI);
	afx_msg void OnTypewater();
	afx_msg void OnUpdateTypewater(CCmdUI* pCmdUI);
	afx_msg void OnFileImportclipboard();
	afx_msg void OnFileExportclipboard();
	afx_msg void OnMapEditscrolllimits();
	afx_msg void OnFileExportwarzonescenarioexpand();
	afx_msg void OnFileExportwarzonemission();
	afx_msg void OnMapSetplayerclanmappings();
	afx_msg void OnGateway();
	afx_msg void OnUpdateGateway(CCmdUI* pCmdUI);
	afx_msg void OnMapExportmapasbitmap();
	afx_msg void OnMapRefreshzones();
	afx_msg void OnOptionsZoomedin();
	afx_msg void OnUpdateOptionsZoomedin(CCmdUI* pCmdUI);
	afx_msg void OnMapSavetilemap();
	afx_msg void OnMapLoadtilemap();
	afx_msg void OnPlayer0en();
	afx_msg void OnUpdatePlayer0en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer1en();
	afx_msg void OnUpdatePlayer1en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer2en();
	afx_msg void OnUpdatePlayer2en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer3en();
	afx_msg void OnUpdatePlayer3en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer4en();
	afx_msg void OnUpdatePlayer4en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer5en();
	afx_msg void OnUpdatePlayer5en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer6en();
	afx_msg void OnUpdatePlayer6en(CCmdUI* pCmdUI);
	afx_msg void OnPlayer7en();
	afx_msg void OnUpdatePlayer7en(CCmdUI* pCmdUI);
	afx_msg void OnOptionsUsenames();
	afx_msg void OnUpdateOptionsUsenames(CCmdUI* pCmdUI);
	afx_msg void OnMarkrect();
	afx_msg void OnUpdateMarkrect(CCmdUI* pCmdUI);
	afx_msg void OnPaste();
	afx_msg void OnUpdatePaste(CCmdUI* pCmdUI);
	afx_msg void OnXflipmarked();
	afx_msg void OnUpdateXflipmarked(CCmdUI* pCmdUI);
	afx_msg void OnYflipmarked();
	afx_msg void OnUpdateYflipmarked(CCmdUI* pCmdUI);
	afx_msg void OnCopymarked();
	afx_msg void OnUpdateCopymarked(CCmdUI* pCmdUI);
	afx_msg void OnPasteprefs();
	afx_msg void OnUpdatePasteprefs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileExportclipboard(CCmdUI* pCmdUI);
	afx_msg void OnDrawbrushfill();
	afx_msg void OnUpdateDrawbrushfill(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CBTEditDoc *BTEditDoc;

#endif // __INCLUDED_BTEDITDOC_H__
