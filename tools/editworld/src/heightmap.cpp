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

//#include "stdafx.h"
#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "math.h"
#include "typedefs.h"
#include "debugprint.hpp"

#include "directx.h"
#include "geometry.h"

#include "ddimage.h"
#include "heightmap.h"
#include "tiletypes.h"

#include "assert.h"
#include "undoredo.h"
#include "gateway.hpp"

#include <fstream>
#include <string>
#include <algorithm>

using std::string;

//#define MIPMAP_TILES

/*
 Warzone export types:

 Campaign Start.
  Saves the map.
  Saves droids,structures and features within the specified scroll limits

  CampaignStart.gam
  CampaignStart\dinit.bjo
  CampaignStart\struct.bjo
  CampaignStart\feat.bjo
  CampaignStart\game.map
  CampaignStart\ttypes.ttp

 Campaign Expand.
  Saves droids,structures and features within the expansion area which is
  specified by two scroll limits ( the current and the expanded ).

  CampaignExpand.gam
  CampaignExpand\dinit.bjo
  CampaignExpand\struct.bjo
  CampaignExpand\feat.bjo

 Mission.
  Much the same as a Campaign Start.

  Mission.gam
  Mission\dinit.bjo
  Mission\struct.bjo
  Mission\feat.bjo
  Mission\game.map
  Mission\ttypes.ttp
*/


// Structures and defines for SeedFill().
//typedef int Pixel;		/* 1-channel frame buffer assumed */
//
//struct FRect {			/* window: a discrete 2-D rectangle */
//    int x0, y0;			/* xmin and ymin */
//    int x1, y1;			/* xmax and ymax (inclusive) */
//};
//
//struct Segment {
//	int y;			//                                                        
//	int xl;			// Filled horizontal segment of scanline y for xl<=x<=xr. 
//	int xr;			// Parent segment was on line y-dy.  dy=1 or -1           
//	int dy;			//
//};

#define MAX 10000		/* max depth of stack */

#define PUSH(Y, XL, XR, DY)	/* push new segment on stack */ \
    if (sp<stack+MAX && Y+(DY)>=win->y0 && Y+(DY)<=win->y1) \
    {sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define POP(Y, XL, XR, DY)	/* pop segment off stack */ \
    {sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}



extern DWORD g_Flags[MAXTILES];
extern DWORD g_Types[MAXTILES];


#define CURRENT_OBJECTLIST_VERSION	3		// Version number for project object list save.
#define CURRENT_SCROLLLIMITS_VERSION 1		// Version number for project scroll limits save.
#define CURRENT_GATEWAY_VERSION 1			// Version number for project gateways save.

//#define	CURRENT_VERSION_NUM			2		// Version number used for warzone saves.

#define	CURRENT_GAME_VERSION_NUM  	8		// Version number used for warzone game save.
#define	CURRENT_MAP_VERSION_NUM  	10		// Version number used for warzone map save.
//#define	CURRENT_TEMPLATE_VERSION_NUM 2		// Version number used for warzone template file.
//#define	CURRENT_DROID_VERSION_NUM	2		// Version number used for warzone droid file.
//#define CURRENT_DROIDINIT_VERSION_NUM 2 	// Version number used for warzone droid init file.

//#define CURRENT_MAP_VERSION_NUM		2		// Version number used for warzone map saves.
//#define CURRENT_FEATURE_VERSION_NUM	2		// Version number used for feature map saves.
//#define CURRENT_STRUCTURE_VERSION_NUM 2		// Version number used for structure map saves.

extern CUndoRedo *g_UndoRedo;

#define WRITENAMEDOBJECTS
#define READNAMEDOBJECTS

//#define SRADIUS
#define OBJECTNORMALS		FALSE
#define OBJECTSMOOTHNORMALS	TRUE
#define DEFAULTHEIGHT	128
#define MIDCOLOUR		128
#define ADJUSTFEATURECOORDS
#define SPHEREDIVS	36

#define NEGZIN2D

#define HGTMULT	1
#define TEXTUREEDGE	0.005
#define PAGESIZE 256

//#define TESTFLAGS	// If defined then write random flags when doing flood fill.

extern string g_HomeDirectory;
extern string g_WorkDirectory;
extern FILE *OpenEditorFile(const char *FileName);
extern std::string EditorDataFileName(const std::string& fileName);

char *IMDTypeNames[]={
	"Feature",
	"Structure",
	"Unit"
};

D3DVECTOR ZeroVector={0.0F,0.0F,0.0F};

//BOOL IsBinary(DWORD Value);	// Defined in BTEditDoc.cpp

HFONT g_Font;

BOOL g_OverlayZoneIDs;

float SinLook[SPHEREDIVS];
float CosLook[SPHEREDIVS];


CHeightMap::CHeightMap(CDirectDraw *DirectDrawView,CGeometry *DirectMaths,CMatManager *MatManager,
					   SLONG MapWidth,SLONG MapHeight,SLONG TileWidth,SLONG TileHeight,SLONG TextureSize) :
	m_MapTiles(NULL),
	m_MapSectors(NULL),
	m_DDView(DirectDrawView),
	m_DirectMaths(DirectMaths),
	m_MatManager(MatManager),
	m_MapWidth(MapWidth),
	m_MapHeight(MapHeight),
	m_TileWidth(TileWidth),
	m_TileHeight(TileHeight),
	m_NumSourceTextures(0),
	m_NumTextures(0),
	m_TextureList(NULL),
	m_HeightScale(1),
//	m_SectorsPerRow(64),
//	m_SectorsPerColumn(64),
	m_SeaLevel(0),
	m_DrawRadius(SECTORDRAWRADIUS),
	m_TextureWidth(TextureSize),
	m_TextureHeight(TextureSize),
	m_Gouraud(TRUE),
	m_NewObjectID(0),
	m_FeatureSet(NULL),
	m_Num3DObjects(0),
	m_Objects(NULL),
	m_TotalInstances(0),
	m_Structures(NULL),
	m_Features(NULL),
	m_Templates(NULL),
	m_NoObjectSnap(FALSE),
	m_Flatten(FALSE),
	m_IgnoreDroids(FALSE),
	m_TerrainMorph(FALSE),
	m_UseRealNames(TRUE),
	m_RenderPlayerID(0)
{
	g_Font = CreateFont(10,0,0,0,FW_NORMAL,0,0,0,
				DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif");

	m_Texture[0].TextureID = m_MatManager->GetMaterialID("Default");

	g_OverlayZoneIDs = FALSE;

	SetMapSize(MapWidth,MapHeight);
	SetTileSize(TileWidth,TileHeight);

//	m_Texture[0].TextureID = m_MatManager->GetMaterialID("Default");
//	D3DCOLORVALUE	Diffuse;
//	Diffuse.r = ((float)0)/((float)256);
//	Diffuse.g = ((float)96)/((float)256);
//	Diffuse.b = ((float)128)/((float)256);
//	Diffuse.a = 0.0F;	//1.0F;

	MatDesc Desc;
	memset(&Desc,0,sizeof(Desc));
	Desc.Type = MT_FLAT;
	Desc.TexFormat = TF_NOTEXTURE;
	Desc.Name = "Default";
	Desc.Alpha = 128;
	Desc.Diffuse.r = ((float)0)/((float)256);
	Desc.Diffuse.g = ((float)96)/((float)256);
	Desc.Diffuse.b = ((float)128)/((float)256);
	Desc.Diffuse.a = 0.0F;	//1.0F;
	m_DefaultMaterial = m_MatManager->CreateMaterial(&Desc);
//	m_DefaultMaterial = m_MatManager->CreateMaterial(MT_FLAT,"Default",
//											NULL,
//											1,NULL,
//											0,
//											0,
//											NULL,
//											128,NULL,
//											NOTEXTURE,
//											&Diffuse,&Diffuse,NULL);
	m_Texture[0].TextureID = m_DefaultMaterial;

// Create a material for the sea.
	Desc.Name = "SeaMat";
	Desc.Diffuse.r = ((float)43)/((float)256);
	Desc.Diffuse.g = ((float)102)/((float)256);
	Desc.Diffuse.b = ((float)128)/((float)256);
	Desc.Diffuse.a = 0.0F;	//1.0F;
	m_SeaMaterial = m_MatManager->CreateMaterial(&Desc);
//	m_SeaMaterial = m_MatManager->CreateMaterial(MT_FLAT,"SeaMat",
//											NULL,
//											1,NULL,
//											0,
//											0,
//											NULL,
//											128,NULL,
//											NOTEXTURE,
//											&Diffuse,&Diffuse,NULL);

// Create a material for lines.
//	D3DCOLORVALUE	Diffuse;
	Desc.Name = "LineMat";
	Desc.Diffuse.r = ((float)255)/((float)256);
	Desc.Diffuse.g = ((float)255)/((float)256);
	Desc.Diffuse.b = ((float)255)/((float)256);
	Desc.Diffuse.a = 0.0F;	//1.0F;
	m_LineMaterial = m_MatManager->CreateMaterial(&Desc);
//	m_LineMaterial = m_MatManager->CreateMaterial(MT_FLAT,"LineMat",
//											NULL,
//											1,NULL,
//											0,
//											0,
//											NULL,
//											128,NULL,
//											NOTEXTURE,
//											&Diffuse,&Diffuse,NULL);

	Desc.Name = "LineMat2";
	Desc.Diffuse.r = ((float)255)/((float)256);
	Desc.Diffuse.g = ((float)0)/((float)256);
	Desc.Diffuse.b = ((float)0)/((float)256);
	Desc.Diffuse.a = 0.0F;	//1.0F;
	m_LineMaterial2 = m_MatManager->CreateMaterial(&Desc);
//	m_LineMaterial2 = m_MatManager->CreateMaterial(MT_FLAT,"LineMat2",
//											NULL,
//											1,NULL,
//											0,
//											0,
//											NULL,
//											128,NULL,
//											NOTEXTURE,
//											&Diffuse,&Diffuse,NULL);

	InitialiseTextMaps();

	float Angle = 0.0F;
	float Step = (float)(2*M_PI/SPHEREDIVS);

	for(int i=0; i<SPHEREDIVS; i++) {
		SinLook[i] = sin(Angle);
		CosLook[i] = cos(Angle);
		Angle += Step;
	}

	InitialiseScrollLimits();
	InitialiseGateways();

	for(i=0; i<MAX_PLAYERS; i++) {
		m_EnablePlayers[i] = TRUE;
	}

	for(i=0; i<MAX_OBJNAMES; i++) {
		m_ObjNames[i].IDString = NULL;
		m_ObjNames[i].Name = NULL;
	}

//	m_SelectionX0 = 0;
//	m_SelectionY0 = 0;
//	m_SelectionX1 = 4;
//	m_SelectionY1 = 4;
	ClearSelectionBox();
}


CHeightMap::~CHeightMap()
{
// Free up the map.
	delete m_MapTiles;

// Free up the map sectors.
	DWORD	i;

// Free up the default material.
	m_MatManager->DeleteMaterial(m_DefaultMaterial);

// Free up the sea material.
	m_MatManager->DeleteMaterial(m_SeaMaterial);

// And the line materials.
	m_MatManager->DeleteMaterial(m_LineMaterial);
	m_MatManager->DeleteMaterial(m_LineMaterial2);

// And any terrain textures.
	for(i=0; i<m_NumTextures; i++) {
		m_MatManager->DeleteMaterial(m_Texture[i].TextureID);
	}

// Free up the list of terrain texture names.
	if(m_TextureList) {
		for(i=0; i<m_NumSourceTextures; i++) {
			delete m_TextureList[i];
		}
		delete m_TextureList ;
	}

	m_NumTextures = 0;
	m_NumSourceTextures = 0;

// Free up and object instances.
	Delete3DObjectInstances();

// And there models.
	Delete3DObjects();

	if(m_Structures) delete m_Structures;
	if(m_Features) delete m_Features;
	if(m_Templates) delete m_Templates;

	DeleteAllScrollLimits();
	DeleteAllGateways();

	for(i=0; i<MAX_OBJNAMES; i++) {
		if(m_ObjNames[i].IDString) delete m_ObjNames[i].IDString;
		if(m_ObjNames[i].Name) delete m_ObjNames[i].Name;
	}
}


void CHeightMap::EnableGouraud(BOOL Enable)
{
	m_Gouraud = Enable;
}


char** CHeightMap::GetTextureList()
{
	return m_TextureList;
}

DWORD CHeightMap::GetNumTextures()
{
	return m_NumSourceTextures;
}

CGrdLandIO* CHeightMap::Read(FILE *Stream)
{
	CGrdLandIO *Project;

	Project = new CGrdLandIO();

// Read the landscape into the IO class.
	if(!Project->Read(Stream)) {
		MessageBox(NULL,"The project file may be currupt!","Bad project file!",MB_OK);
		delete Project;
		return NULL;
	}
 
//	DeleteTextureList();
	
// Re-initialise the texture list.
/* 12/04/99
	Project->GetTextureSize(&m_TextureWidth,&m_TextureHeight);

	m_NumSourceTextures = Project->GetNumTextures();
	m_TextureList = new char*[m_NumSourceTextures];
	for(i=0; i<m_NumSourceTextures; i++) {
		m_TextureList[i] = new char[strlen(Project->GetTextureName(i))+1];
		strcpy(m_TextureList[i],Project->GetTextureName(i));
	}
*/
// Create and initialise a map with the given dimensions.
	DWORD	MapWidth,MapHeight,NumTiles;
	Project->GetMapSize(&MapWidth,&MapHeight);
	SetMapSize(MapWidth,MapHeight);
	NumTiles = MapWidth*MapHeight;

// Set tile size and re-initialise tile vertices with tile size.
	DWORD	TileWidth,TileHeight;
	Project->GetTileSize(&TileWidth,&TileHeight);
	SetTileSize(TileWidth,TileHeight);

// Set Texture id and height for each corner for all tiles.
// Also sets vertex , texture flip and type flags.
	DWORD	TileNum;
	CGrdTileIO *GrdTile;
	for(TileNum = 0; TileNum < NumTiles; TileNum++) {
   		GrdTile = Project->GetTile(TileNum);
   		SetTextureID(TileNum,GrdTile->GetTextureID());
   		SetVertexFlip(TileNum,GrdTile->GetVertexFlip());
//   		SetTextureFlip(TileNum,GrdTile->GetTextureFlip());
   		SetTextureFlip(TileNum,FALSE,FALSE);
   		SetVertexHeight(TileNum,0,GrdTile->GetVertexHeight(0) * m_HeightScale);
   		SetVertexHeight(TileNum,1,GrdTile->GetVertexHeight(1) * m_HeightScale);
   		SetVertexHeight(TileNum,2,GrdTile->GetVertexHeight(2) * m_HeightScale);
   		SetVertexHeight(TileNum,3,GrdTile->GetVertexHeight(3) * m_HeightScale);
   		SetMapZoneID(TileNum,0);
//   		SetTileVisible(TileNum,GrdTile->GetFlags());
		SetTileFlags(TileNum,GrdTile->GetFlags() & ~TF_HIDE);
	}

// Load and initialise textures from texture list.
	if(!InitialiseTextures(m_NumSourceTextures,m_TextureList,m_TextureWidth,m_TextureHeight)) {
		MessageBox(NULL,"Please ensure terrain texture files are\nin the same directory as the project file.","Unable to load terrain textures!",MB_OK);
		delete Project;
		return NULL;
	}

// Initialise map sectors.
	InitialiseSectors();

	return Project;
}




BOOL CHeightMap::AddTexture(char *TextureName)
{
//// Delete the current texture list.
//	if(m_TextureList) {
//		for(i=0; i<m_NumTextures; i++) {
//			delete m_TextureList[i];
//		}
//		delete m_TextureList;
//	}
//	m_NumTextures = 0;


// Add the new texture.
	if(m_TextureList == NULL) {
		m_TextureList = new char*[16];
	}

	ASSERT(m_NumSourceTextures < 16);

	m_TextureList[m_NumSourceTextures] = new char[strlen(TextureName)+1];
	strcpy(m_TextureList[m_NumSourceTextures],TextureName);

	m_NumSourceTextures++;

// Delete any existing textures.
	for(int i=0; i<(int)m_NumTextures; i++) {
		m_MatManager->DeleteMaterial(m_Texture[i].TextureID);
	}
	m_NumTextures = 0;

// Load and initialise textures.
	return InitialiseTextures(m_NumSourceTextures,m_TextureList,m_TextureWidth,m_TextureHeight);
}


BOOL CHeightMap::InitialiseTextures(DWORD NumTextures,char **TextureList,DWORD TextureWidth,DWORD TextureHeight)
{
	char	FileName[256];
	char	Drive[256];
   	char	Dir[256];
   	char	FName[256];
   	char	Ext[256];

	m_TextureWidth = TextureWidth;
	m_TextureHeight = TextureHeight;

	DWORD	TexNum;
	DWORD	TexMatNum=0;

	ASSERT(NumTextures<=1);

	for(TexNum = 0; TexNum<NumTextures; TexNum++) {
		ASSERT(TexNum < 16);

   		strcpy(m_Texture[TexNum].Name,TextureList[TexNum]);
   		strcpy(FileName,TextureList[TexNum]);
		_splitpath(FileName,Drive,Dir,FName,Ext);

		DWORD	i;
		for(i=0; i<strlen(Ext); i++) {
			if( islower( Ext[i] ) ) {
			  Ext[i] = toupper( Ext[i] );
			}
		}

		DWORD TilesPerPage = ((PAGESIZE/m_TextureWidth)*(PAGESIZE/m_TextureWidth));

// Currently only handles .PCX files.
		if(strcmp(Ext,".PCX")==0) {	
			DebugPrint("\nLoading terrain texture %d : %s\n",TexNum,FileName);
// Load the PCX bitmap for the textures.

			std::ifstream PCXFile(FileName, std::ios_base::binary);
			if (!PCXFile.is_open())
			{
				MessageBox(NULL, FileName, "Unable to open file.", MB_OK);
				return FALSE;
			}

			PCXHandler* PCXTexBitmap = new PCXHandler;

			if(!PCXTexBitmap->ReadPCX(PCXFile))
			{
				delete PCXTexBitmap;
				return FALSE;
			}

			int Width = PCXTexBitmap->GetBitmapWidth();
			int Height = PCXTexBitmap->GetBitmapHeight();

			int NumTiles = (Width/m_TextureWidth) * (Height/m_TextureWidth);	// Number of tiles in source page.
			if(NumTiles > MAX_TILETEXTURES) {
				MessageBox(NULL,"Too many tile textures.","Unable to load texture.",MB_OK);
				delete PCXTexBitmap;
				return FALSE;
			}

			if((Width/m_TextureWidth)*m_TextureWidth != Width) {
				MessageBox(NULL,"Texture page width is not a multiple\nof the tile texture width.","Unable to load texture.",MB_OK);
				delete PCXTexBitmap;
				return FALSE;
			}

			if((Height/m_TextureHeight)*m_TextureHeight != Height) {
				MessageBox(NULL,"Texture page width is not a multiple\nof the tile texture width.","Unable to load texture.",MB_OK);
				delete PCXTexBitmap;
				return FALSE;
			}

			int NumPages = (NumTiles+TilesPerPage-1)/TilesPerPage;	// Number of texture pages needed to hold them.
			int NumPageTiles = TilesPerPage;
			if(NumTiles < NumPageTiles) {
				NumPageTiles = NumTiles;
			}

//			DebugPrint("NumTiles = %d\n",NumTiles);
//			DebugPrint("NumPages = %d\n",NumPages);
//			DebugPrint("TilesPerPage = %d\n",TilesPerPage);
//			DebugPrint("NumPageTiles = %d\n",NumPageTiles);

			int SourceX = 0;	// Current XY of source texture.
			int SourceY = 0;

			// Allocate a bitmap for the destination.
			UBYTE *PageBits = new UBYTE[PAGESIZE*PAGESIZE];

			// For each page...
			for(int Page=0; Page<NumPages; Page++) {
				// Generate a unique name.
				char PageName[256];
				sprintf(PageName,"%s%d",FileName,Page);
				// Scan across and down the destination page...
				for(int DestY=0; (DestY<PAGESIZE) && (NumTiles); DestY+=m_TextureWidth) {
					for(int DestX=0; (DestX<PAGESIZE) && (NumTiles); DestX+=m_TextureWidth) {
						// Copying the current source texture into it..
						CopyTexture((UBYTE*)PCXTexBitmap->GetBitmapBits(),Width,SourceX,SourceY,
									(UBYTE*)PageBits,PAGESIZE,DestX,DestY,m_TextureWidth);
						// And skip to the next source texture.
						SourceX+=m_TextureWidth;
						if(SourceX >= Width) {
							SourceX = 0;
							SourceY += m_TextureWidth;
						}
						NumTiles--;
					}
				}

// Finally create the D3D material for each page and store it's ID and size in the texture lookup array.

#ifdef MIPMAP_TILES
// Generate mip maps from 256x256 down to 4x4.
				UBYTE *MipMapBits[32];
				int mw = PAGESIZE;
				int mh = PAGESIZE;
				int mCount = 0;
				int sx,sy;

				while(mw >= 64) {
					MipMapBits[mCount] = new UBYTE[mw*mh];

					UBYTE *Src = (UBYTE*)PageBits;
					UBYTE *Dst = MipMapBits[mCount];

					for(sy=0; sy<PAGESIZE; sy+=(1<<mCount)) {
						for(sx=0; sx<PAGESIZE; sx+=(1<<mCount)) {
							*Dst = *(Src + sx+sy*PAGESIZE);
							Dst++;
						}
					}

					mw /= 2;
					mh /= 2;
					mCount++;
				}

//				D3DCOLORVALUE	Diffuse;
				MatDesc Desc;

				memset(Desc,0,sizeof(Desc));
				Desc.Type = MT_MIPMAPED;
				Desc.TexFormat = TF_PAL8BIT;
				Desc.Name = PageName;
				Desc.Count = mCount;
				Desc.MipMapBits = (void**)MipMapBits;
				Desc.Width = PAGESIZE;
				Desc.Height = PAGESIZE;
				Desc.PaletteEntries = PCXTexBitmap->GetBitmapPaletteEntries();
				Desc.Alpha = 255;
				Desc.Diffuse.r = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.g = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.b = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.a = 0.0F;	//1.0F;
				
				m_Texture[TexMatNum].Width = PAGESIZE;
				m_Texture[TexMatNum].Height = PAGESIZE;
				m_Texture[TexMatNum].NumTiles = NumPageTiles;
				m_Texture[TexMatNum].TextureID = m_MatManager->CreateMaterial(&Desc);
//				m_Texture[TexMatNum].TextureID = m_MatManager->CreateMaterial(MT_MIPMAPED,PageName,
//												NULL,
//												mCount,(void**)MipMapBits,
//												PAGESIZE,PAGESIZE,
//												PCXTexBitmap->GetBitmapPaletteEntries(),
//												255,NULL,
//												PAL8BIT,
//												&Diffuse,&Diffuse,NULL);
				for(int i=0; i<mCount; i++) {
					delete MipMapBits[i];
				}
#else
//				D3DCOLORVALUE	Diffuse;
				MatDesc Desc;

				memset(&Desc,0,sizeof(Desc));
				Desc.Type = MT_TEXTURED;
				Desc.TexFormat = TF_PAL8BIT;
				Desc.Name = PageName;
				Desc.Bits = (void**)PageBits;
				Desc.Width = PAGESIZE;
				Desc.Height = PAGESIZE;
				Desc.PaletteEntries = PCXTexBitmap->GetBitmapPaletteEntries();
				Desc.Alpha = 255;
				Desc.Diffuse.r = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.g = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.b = ((float)MIDCOLOUR)/((float)256);
				Desc.Diffuse.a = 0.0F;	//1.0F;

				m_Texture[TexMatNum].Width = PAGESIZE;
				m_Texture[TexMatNum].Height = PAGESIZE;
				m_Texture[TexMatNum].NumTiles = NumPageTiles;
				m_Texture[TexMatNum].TextureID = m_MatManager->CreateMaterial(&Desc);
//				m_Texture[TexMatNum].TextureID = m_MatManager->CreateMaterial(MT_TEXTURED,PageName,
//												(void*)PageBits,
//												1,NULL,
//												PAGESIZE,PAGESIZE,
//												PCXTexBitmap->GetBitmapPaletteEntries(),
//												255,NULL,
//												PAL8BIT,
//												&Diffuse,&Diffuse,NULL);
#endif
// Test background material.
//				m_DDView->SetBackgroundMaterial(m_MatManager->GetMaterialHandle(m_Texture[TexMatNum].TextureID));

//				PCXHandler *TestPCX = new PCXHandler();
//				TestPCX->Create(PAGESIZE,PAGESIZE,PageBits,PCXTexBitmap->GetBitmapPaletteEntries());
//				sprintf(PageName,"D%d%s",Page,FileName);
//				TestPCX->WritePCX(PageName);
//				delete TestPCX;

				TexMatNum++;
			}
			delete PageBits;


			delete PCXTexBitmap;
			DebugPrint("\nTerrain texture %d id : %d\n\n",TexNum,m_Texture[TexNum].TextureID);
		}
	}

	m_NumSourceTextures = NumTextures;	// The number of source texture files loaded.
	m_NumTextures = TexMatNum;			// The actual number of texture pages created.

	InitialiseTextMaps();

	FixTextureIDS();

	return TRUE;
}

// Copy an 8bit palettized texture from one page to another.
//
void CHeightMap::CopyTexture(UBYTE *SourceBits,int SourcePitch,int SourceX,int SourceY,
							 UBYTE *DestBits,int DestPitch,int DestX,int DestY,int TileSize)
{
	UBYTE *Src = SourceBits+SourceX+(SourceY*SourcePitch);
	UBYTE *Dst = DestBits+DestX+(DestY*DestPitch);

	for(int y=0; y<TileSize; y++) {
		for(int x=0; x<TileSize; x++) {
			Dst[x] = Src[x];
		}
		Dst += DestPitch;
		Src += SourcePitch;
	}
}

//	InitialiseTextMaps()
//
// Creates an array of texture maps. Each element contains the UV coordinates for that map and the
// ID of the D3D material it exists in.
//
void CHeightMap::InitialiseTextMaps()
{
	DWORD	TexNum;
	DWORD	MapNum = 0;
	DWORD	x,y;

	DebugPrint("*\n");
	m_TextureMaps[0].TextureID = 0;
	m_TextureMaps[0].u0 = D3DVAL(0);
	m_TextureMaps[0].v0 = D3DVAL(0);
	m_TextureMaps[0].u1 = D3DVAL(0);
	m_TextureMaps[0].v1 = D3DVAL(0);
	m_TextureMaps[0].u2 = D3DVAL(0);
	m_TextureMaps[0].v2 = D3DVAL(0);
	m_TextureMaps[0].u3 = D3DVAL(0);
	m_TextureMaps[0].v3 = D3DVAL(0);
	m_MaxTileID = 0;

	int TotalTiles = 0;

	for(TexNum = 0; TexNum < m_NumTextures; TexNum++) {
		ASSERT(TexNum < 16);
		DWORD PageTiles = 0;
		for(y=0; (y<m_Texture[TexNum].Height) && (PageTiles < m_Texture[TexNum].NumTiles); y+=m_TextureHeight) {
			for(x=0; (x<m_Texture[TexNum].Width) && (PageTiles < m_Texture[TexNum].NumTiles); x+=m_TextureWidth) {
				SetTextureMap(MapNum,TexNum,x,y);
				MapNum++;
				PageTiles++;
				TotalTiles++;
			}
		}
	}

	m_MaxTMapID = MapNum;
	m_MaxTileID = TotalTiles;
}

// Set an entry in the texture UV definition table.
//
void CHeightMap::SetTextureMap(DWORD TexNum,DWORD PageNum,DWORD x,DWORD y)
{
	m_TextureMaps[TexNum+1].TextureID = m_Texture[PageNum].TextureID;
	m_TextureMaps[TexNum+1].u0 = ( ( ((float)x) ) / m_Texture[PageNum].Width )+(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].v0 = ( ( ((float)y) ) / m_Texture[PageNum].Height )+(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].u1 = ( ( ((float)x+m_TextureWidth)  ) / m_Texture[PageNum].Width )-(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].v1 = ( ( ((float)y) ) / m_Texture[PageNum].Height )+(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].u2 = ( ( ((float)x+m_TextureWidth)  ) / m_Texture[PageNum].Width )-(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].v2 = ( ( ((float)y+m_TextureHeight)  ) / m_Texture[PageNum].Height )-(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].u3 = ( ( ((float)x)  ) / m_Texture[PageNum].Width )+(float)TEXTUREEDGE;
	m_TextureMaps[TexNum+1].v3 = ( ( ((float)y+m_TextureHeight)  ) / m_Texture[PageNum].Height )-(float)TEXTUREEDGE;
//	DebugPrint("TexNum %d , ID %d\n",TexNum+1,m_TextureMaps[TexNum+1].TextureID);
}


BOOL CHeightMap::WriteHeightMap(char *FileName)
{
	UBYTE *Bits = new UBYTE[m_MapWidth*m_MapHeight];
	UBYTE *Ptr = Bits;
	int x,y,i,TileNum;

	for(y=0; y<m_MapHeight; y++) {
		for(x=0; x<m_MapWidth; x++) {
			TileNum = y * m_MapWidth + x;

			*Ptr = (UBYTE)GetVertexHeight(TileNum,0);
			Ptr++;
		}
	}

	PALETTEENTRY Palette[256];
	for(i=0; i<256; i++) {
		Palette[i].peRed = i;
		Palette[i].peGreen = i;
		Palette[i].peBlue = i;
		Palette[i].peFlags = 0;
	}

	PCXHandler TestPCX;
	if(TestPCX.Create(m_MapWidth,m_MapHeight,Bits,Palette))
	{
		std::ofstream output(FileName, std::ios_base::binary);
		if(!output.is_open())
		{
			MessageBox(NULL, FileName, "Unable to create file.", MB_OK);
			return FALSE;
		}

		TestPCX.WritePCX(output);
	}

	return TRUE;
}


// Load in a bitmap and set map heights from it.
//
BOOL CHeightMap::SetHeightFromBitmap(char *FileName)
{
	char	Drive[256];
   	char	Dir[256];
   	char	FName[256];
   	char	Ext[256];

	_splitpath(FileName,Drive,Dir,FName,Ext);

	DWORD	i;
	for(i=0; i<strlen(Ext); i++) {
		if( islower( Ext[i] ) ) {
		  Ext[i] = toupper( Ext[i] );
		}
	}

   	if(strcmp(Ext,".PCX")==0) {	

   	// Load the PCX bitmap for the textures.
		std::ifstream PCXFile(FileName, std::ios_base::binary);
		if (!PCXFile.is_open())
		{
			MessageBox(NULL, FileName, "Unable to open file.", MB_OK);
			return FALSE;
		}

		PCXHandler* PCXTexBitmap = new PCXHandler;

		if(!PCXTexBitmap->ReadPCX(PCXFile))
		{
			MessageBox(NULL,"The height map is not a valid PCX","Unable to load height map!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}

		if( (m_MapWidth!=PCXTexBitmap->GetBitmapWidth()) || 
		    (m_MapHeight!=PCXTexBitmap->GetBitmapHeight()) ) {
			MessageBox(NULL,"The height map size does not match the world size","Illegal size!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}
		if(PCXTexBitmap->GetBitmapBitCount() != 8) {
			MessageBox(NULL,"The height map must be a 256 colour grey scale image.","Illegal bit depth!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}

		DWORD TileNum;
		UBYTE *Bits = (UBYTE*)PCXTexBitmap->GetBitmapBits();
		DWORD BMapWidth = PCXTexBitmap->GetBitmapWidth();
		DWORD BMapHeight = PCXTexBitmap->GetBitmapHeight();

		DWORD x,y;

		FixTilePositions();

		for(y=0; y<BMapHeight; y++) {
   			for(x=0; x<BMapWidth; x++) {
				UBYTE *Height = Bits + y * PCXTexBitmap->GetBitmapWidth() + x;

				TileNum = y * m_MapWidth + x;

				SetVertexHeight(TileNum,0,(float)(*Height)*HGTMULT);

				if(x < BMapWidth-1) {
					SetVertexHeight(TileNum,1,(float)(*(Height+1))*HGTMULT);
				} else {
					SetVertexHeight(TileNum,1,0.0F);
				}

				if((x < BMapWidth-1) && (y < BMapHeight-1)) {
					SetVertexHeight(TileNum,2,(float)(*(Height+BMapWidth+1))*HGTMULT);
				} else {
					SetVertexHeight(TileNum,2,0.0F);
				}

				if(y < BMapHeight-1) {
					SetVertexHeight(TileNum,3,(float)(*(Height+BMapWidth))*HGTMULT);
				} else {
					SetVertexHeight(TileNum,3,0.0F);
				}

				FixTileNormals(&m_MapTiles[TileNum]);
   			}
   		}

   		delete PCXTexBitmap;
   	}

   	if(strcmp(Ext,".BMP")==0) {	
		MessageBox(NULL,"BMP's currently not supported for this operation","Unable to load height map!",MB_OK);
   	}

	InitialiseSectors();

	return TRUE;
}


BOOL CHeightMap::SetTileIDsFromBitmap(char *FullPath)
{
	char	Drive[256];
   	char	Dir[256];
   	char	FName[256];
   	char	Ext[256];

	_splitpath(FullPath,Drive,Dir,FName,Ext);

	DWORD	i;
	for(i=0; i<strlen(Ext); i++) {
		if( islower( Ext[i] ) ) {
		  Ext[i] = toupper( Ext[i] );
		}
	}

	if(strcmp(Ext,".PCX")==0)
	{	
		std::ifstream PCXFile(FullPath, std::ios_base::binary);
		if (!PCXFile.is_open())
		{
			MessageBox(NULL, FullPath, "Unable to open file.", MB_OK);
			return FALSE;
		}

		PCXHandler* PCXTexBitmap = new PCXHandler;

		if(!PCXTexBitmap->ReadPCX(PCXFile))
		{
			MessageBox(NULL,"The tile map is not a valid PCX","Unable to load height map!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}

		if( (m_MapWidth!=PCXTexBitmap->GetBitmapWidth()) || 
		    (m_MapHeight!=PCXTexBitmap->GetBitmapHeight()) ) {
			MessageBox(NULL,"The tile map size does not match the world size","Illegal size!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}
		if(PCXTexBitmap->GetBitmapBitCount() != 8) {
			MessageBox(NULL,"The tile map must be a 256 colour grey scale image.","Illegal bit depth!",MB_OK);
			delete PCXTexBitmap;
			return FALSE;
		}

		DWORD TileNum;
		UBYTE *Bits = (UBYTE*)PCXTexBitmap->GetBitmapBits();
		DWORD BMapWidth = PCXTexBitmap->GetBitmapWidth();
		DWORD BMapHeight = PCXTexBitmap->GetBitmapHeight();

		DWORD x,y;

		for(y=0; y<BMapHeight; y++) {
   			for(x=0; x<BMapWidth; x++) {
				int TileID = (int)*(Bits + y * PCXTexBitmap->GetBitmapWidth() + x);

				TileID += 1;
				if(TileID > m_MaxTileID-1) {
					TileID = m_MaxTileID-1;
				}

				TileNum = y * m_MapWidth + x;

				SetTextureID(TileNum,TileID);
   			}
   		}

   		delete PCXTexBitmap;
   	}

   	if(strcmp(Ext,".BMP")==0) {	
		MessageBox(NULL,"BMP's currently not supported for this operation","Unable to load height map!",MB_OK);
   	}

	return TRUE;
}


BOOL CHeightMap::WriteTileIDMap(char *FullPath)
{
	UBYTE *Bits = new UBYTE[m_MapWidth*m_MapHeight];
	UBYTE *Ptr = Bits;
	int x,y,i,TileNum;

	for(y=0; y<m_MapHeight; y++) {
		for(x=0; x<m_MapWidth; x++) {
			TileNum = y * m_MapWidth + x;

			*Ptr = (UBYTE)GetTextureID(TileNum)-1;
			Ptr++;
		}
	}

	PALETTEENTRY Palette[256];
	for(i=0; i<256; i++) {
		Palette[i].peRed = i;
		Palette[i].peGreen = i;
		Palette[i].peBlue = i;
		Palette[i].peFlags = 0;
	}

	PCXHandler TestPCX;
	if(TestPCX.Create(m_MapWidth,m_MapHeight,Bits,Palette))
	{
		std::ofstream output(FullPath, std::ios_base::binary);
		if(!output.is_open())
		{
			MessageBox(NULL, FullPath, "Unable to create file.", MB_OK);
			return FALSE;
		}

		TestPCX.WritePCX(output);
	}

	return TRUE;
}


void CHeightMap::SetMapSize(DWORD MapWidth,DWORD MapHeight)
{
	m_MapWidth = MapWidth;
	m_MapHeight = MapHeight;

// At the moment , changing the size of the map will destroy the current tile data.
	if(m_MapTiles) delete m_MapTiles;
	m_NumTiles = m_MapWidth*m_MapHeight;
	m_MapTiles = new CTile[m_NumTiles];

	SLONG	x,z;
	CTile *Tile = m_MapTiles;

	FixTilePositions();

	for(z=0; z<m_MapHeight; z++) {
		for(x=0; x<m_MapWidth; x++) {
			Tile->TMapID = 0;	//1;
			Tile->Flags = 0;
			FixTileVerticies(Tile,x,z,FIX_X | FIX_Y | FIX_Z);
			FixTileNormals(Tile);
			FixTileTextures(Tile);
//			Tile->Flags |= TF_TYPEGRASS;
			Tile++;
		}
	}

	m_SectorsPerRow = 64;
	m_SectorsPerColumn = 64;

	m_TilesPerSectorRow = m_MapWidth / m_SectorsPerRow;
	if(m_TilesPerSectorRow < 1) {
		m_TilesPerSectorRow = 1;	// Need to handle this properly.
		m_SectorsPerRow = m_MapWidth;
	}
	m_TilesPerSectorColumn = m_MapHeight / m_SectorsPerColumn;
	if(m_TilesPerSectorColumn < 1) {
		m_TilesPerSectorColumn = 1;	// Need to handle this properly.
		m_SectorsPerColumn = m_MapHeight;
	}

	InitialiseSectors();
}

// FixTextureIDS()
//
// Go through entire map and if no texture page loaded then
// set all tile texture ID's to 0 ( un-textured ). If texture page
// is loaded then change any un-textured tiles to texture ID 1.
// Also if texture ID is greater than max texture ID then cap it.
//
void CHeightMap::FixTextureIDS()
{
	SLONG	x,z;
	CTile *Tile = m_MapTiles;

	int DefMat = m_MatManager->GetMaterialID("Default");

	for(z=0; z<m_MapHeight; z++) {
		for(x=0; x<m_MapWidth; x++) {
			if((int)m_Texture[0].TextureID > DefMat) {
				if(Tile->TMapID == 0) {
					Tile->TMapID = 1;
				} else {
					if(Tile->TMapID > m_MaxTMapID) {
						Tile->TMapID = m_MaxTMapID;
					}
				}
			} else {
				Tile->TMapID = 0;
			}
			Tile++;
		}
	}
}


void CHeightMap::SetTileSize(DWORD TileWidth,DWORD TileHeight)
{
	m_TileWidth = TileWidth;
	m_TileHeight = TileHeight;
	m_ViewRadius = DRAWRADIUS*m_TileWidth;

	SLONG	x,z;
	CTile *Tile = m_MapTiles;

	FixTilePositions();

	for(z=0; z<m_MapHeight; z++) {
		for(x=0; x<m_MapWidth; x++) {
			FixTileVerticies(Tile,x,z,FIX_X | FIX_Z);
			FixTileNormals(Tile);
			Tile++;
		}
	}
}

void CHeightMap::SetTextureSize(DWORD TextureWidth,DWORD TextureHeight)
{
	m_TextureWidth = TextureWidth;
	m_TextureHeight = TextureHeight;

	CTile *Tile = m_MapTiles;

	InitialiseTextMaps();

//	for(z=0; z<m_MapHeight; z++) {
//		for(x=0; x<m_MapWidth; x++) {
//			FixTileTextures(Tile);
//			Tile++;
//		}
//	}

}

void CHeightMap::SetTileFlags(DWORD TileNum,DWORD Flags)
{
	m_MapTiles[TileNum].Flags = Flags;
}

DWORD CHeightMap::GetTileFlags(DWORD MapX,DWORD MapY)
{
	return m_MapTiles[MapY*m_MapWidth + MapX].Flags;
}

DWORD CHeightMap::GetTileFlags(DWORD TileNum)
{
	return m_MapTiles[TileNum].Flags;
}

void CHeightMap::SetTileType(DWORD TileNum,DWORD Type)
{
//	m_MapTiles[TileNum].Flags &= ~TF_TYPEMASK;
//	m_MapTiles[TileNum].Flags |= Type;
}


DWORD CHeightMap::GetTileType(DWORD MapX,DWORD MapY)
{
	return g_Types[m_MapTiles[MapY*m_MapWidth + MapX].TMapID];
}


DWORD CHeightMap::GetTileType(DWORD TileNum)
{
	return g_Types[m_MapTiles[TileNum].TMapID];
}


void CHeightMap::SetTileVisible(DWORD TileNum,DWORD Flag)
{
	if(Flag == TF_SHOW) {
		m_MapTiles[TileNum].Flags &= ~TF_HIDE;
	} else {
		m_MapTiles[TileNum].Flags |= TF_HIDE;
	}
}


DWORD CHeightMap::GetTileVisible(DWORD TileNum)
{
	if(m_MapTiles[TileNum].Flags & TF_HIDE) {
		return TF_HIDE;
	} else {
		return TF_SHOW;
	}
}


void CHeightMap::SetMapZoneID(int TileNum,int ID)
{
	m_MapTiles[TileNum].ZoneID = ID;
}

void CHeightMap::SetMapZoneID(int x,int z,int ID)
{
	m_MapTiles[z*m_MapWidth + x].ZoneID = ID;
}


void CHeightMap::SetTextureID(DWORD TileNum,DWORD TMapID)
{
	m_MapTiles[TileNum].TMapID = TMapID;
	FixTileTextures(&m_MapTiles[TileNum]);
}

//#if(0)
//
//DWORD FillOldID,FillNewID,FillType;
//
//void CHeightMap::FillMap(DWORD Selected,DWORD TextureID,DWORD Type)
//{
//	SWORD y = (SWORD)(Selected / m_MapWidth);
//	SWORD x = (SWORD)(Selected - (y*m_MapWidth));
//
//	FillOldID = m_MapTiles[(y*m_MapWidth)+x].TMapID,TextureID;
//	FillNewID = TextureID;
//	FillType = Type;
//
//	FloodFill(x,y);
//}
//
//
//// This routine needs re-writing because it causes a stack overflow if used with large areas.
//// Took out unnecessary parameters to reduce stack usage by 12 bytes per call.
////
//void CHeightMap::FloodFill(SWORD x,SWORD y)
//{
//	if(FillOldID != FillNewID) {
//		if((x>=0) && (x<m_MapWidth) && (y>=0) && (y<=m_MapHeight)) {
//			if(m_MapTiles[(y*m_MapWidth)+x].TMapID == FillOldID) {
//				m_MapTiles[(y*m_MapWidth)+x].TMapID = FillNewID;
//				m_MapTiles[(y*m_MapWidth)+x].Flags &= ~TF_TYPEMASK;
//				m_MapTiles[(y*m_MapWidth)+x].Flags |= FillType;
//
//				FloodFill(x,y-1);
//				FloodFill(x,y+1);
//				FloodFill(x-1,y);
//				FloodFill(x+1,y);
//			}
//		}
//	}
//}
//
//#else

void CHeightMap::ApplyRandomness(int Selected,UDWORD SelFlags)
{
	if(SelFlags & (SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY | SF_RANDTEXTUREROTATE |
							SF_TEXTUREFLIPX | SF_TEXTUREFLIPY | SF_TEXTUREROTMASK)) {
		BOOL FlipX = GetTextureFlipX(Selected);
		BOOL FlipY = GetTextureFlipY(Selected);
		DWORD Rotate = GetTextureRotate(Selected);
		BOOL OFlipX = FlipX;
		BOOL OFlipY = FlipY;
		DWORD ORotate = Rotate;

		if(SelFlags & (SF_RANDTEXTUREFLIPX | SF_RANDTEXTUREFLIPY | SF_RANDTEXTUREROTATE)) {
//			do {
				if(SelFlags & SF_RANDTEXTUREFLIPX) {
					FlipX = rand()%2 ? FALSE : TRUE;
				}
				if(SelFlags & SF_RANDTEXTUREFLIPY) {
					FlipY = rand()%2 ? FALSE : TRUE;
				}
				if(SelFlags & SF_RANDTEXTUREROTATE) {
					Rotate = rand()%3;
				}

//				if(SelFlags & SF_TOGTEXTUREFLIPX) {
//					FlipX = FlipX ? FALSE : TRUE;
//				}
//				if(SelFlags & SF_TOGTEXTUREFLIPY) {
//					FlipY = FlipY ? FALSE : TRUE;
//				}
//				if(SelFlags & SF_INCTEXTUREROTATE) {
//					Rotate = (Rotate + 1)%3;
//				}
				// Make sure something changed.
//			} while((FlipX == OFlipX) && (FlipY == OFlipY) && (Rotate == ORotate));
		}

		if( !(SelFlags & SF_RANDTEXTUREFLIPX) ) {
			FlipX = FALSE;
		}
		if( !(SelFlags & SF_RANDTEXTUREFLIPY) ) {
			FlipY = FALSE;
		}
		if( !(SelFlags & SF_RANDTEXTUREROTATE) ) {
			Rotate = 0;
		}

		if(SelFlags & SF_TEXTUREFLIPX) {
			FlipX = TRUE;
		}

		if(SelFlags & SF_TEXTUREFLIPY) {
			FlipY = TRUE;
		}

		if(SelFlags & SF_TEXTUREROTMASK) {
			Rotate = (SelFlags & SF_TEXTUREROTMASK)>>SF_TEXTUREROTSHIFT;
		}

		SetTextureFlip(Selected,FlipX,FlipY);
		SetTextureRotate(Selected,Rotate);
	} else {
		SetTextureFlip(Selected,FALSE,FALSE);
		SetTextureRotate(Selected,0);
	}
}


void CHeightMap::FillMap(DWORD Selected,DWORD TextureID,DWORD Type,DWORD Flags)
{
	SWORD y = (SWORD)(Selected / m_MapWidth);
	SWORD x = (SWORD)(Selected - (y*m_MapWidth));

	FRect Rect;
	Rect.x0 = 0;
	Rect.y0 = 0;
	Rect.x1 = m_MapWidth-1;
	Rect.y1 = m_MapHeight-1;

	SeedFill(x,y,&Rect,0,Type,Flags);
	SeedFill(x,y,&Rect,TextureID,Type,Flags);
}

Pixel CHeightMap::PixelRead(int x,int y)
{
	return m_MapTiles[(y*m_MapWidth)+x].TMapID;
}


BOOL CHeightMap::PixelCompare(int x,int y,DWORD Tid,DWORD Type,DWORD Flags)
{
	return (m_MapTiles[y * m_MapWidth + x].TMapID == Tid
//	     && (m_MapTiles[y * m_MapWidth + x].Flags & TF_TYPEMASK) == Type)
	     && (m_MapTiles[y * m_MapWidth + x].Flags & TF_TEXTUREMASK) == Flags);
}


void CHeightMap::PixelWrite(int x,int y,Pixel nv,DWORD Type,DWORD Flags)
{
	assert(x >=0);
	assert(y >=0);
	assert(x < m_MapWidth);
	assert(y < m_MapHeight);

	m_MapTiles[(y*m_MapWidth)+x].TMapID = nv;
//	m_MapTiles[(y*m_MapWidth)+x].Flags &= ~TF_TYPEMASK;
//	m_MapTiles[(y*m_MapWidth)+x].Flags |= Type;

	m_MapTiles[(y*m_MapWidth)+x].Flags &= TF_GEOMETRYMASK;	// | TF_TYPEMASK;
	m_MapTiles[(y*m_MapWidth)+x].Flags |= Flags;

	ApplyRandomness((y*m_MapWidth)+x,Flags);
}

/*
 * fill: set the pixel at (x,y) and all of its 4-connected neighbors
 * with the same pixel value to the new pixel value nv.
 * A 4-connected neighbor is a pixel above, below, left, or right of a pixel.
 */
void CHeightMap::SeedFill(int x, int y, FRect *win, Pixel nv,DWORD Type,DWORD Flags)
{
    int l, x1, x2, dy;
    Pixel ov;							/* old pixel value */
    Segment stack[MAX], *sp = stack;	/* stack of filled segments */

    ov = PixelRead(x, y);		/* read pv at seed point */

    if (ov==nv || x<win->x0 || x>win->x1 || y<win->y0 || y>win->y1) {
		return;
	}

    PUSH(y, x, x, 1);			/* needed in some cases */
    PUSH(y+1, x, x, -1);		/* seed segment (popped 1st) */

    while (sp>stack) {
		/* pop segment off stack and fill a neighboring scan line */
		POP(y, x1, x2, dy);
		/*
		 * segment of scan line y-dy for x1<=x<=x2 was previously filled,
		 * now explore adjacent pixels in scan line y
		 */
		for (x=x1; x>=win->x0 && PixelRead(x, y)==ov; x--) {
			PixelWrite(x, y, nv,Type,Flags);
		}

		if (x>=x1) {
			goto skip;
		}

		l = x+1;
		if (l<x1) {
			PUSH(y, l, x1-1, -dy);		/* leak on left? */
		}
		x = x1+1;

		do {
			for (; x<=win->x1 && PixelRead(x, y)==ov; x++) {
				PixelWrite(x, y, nv,Type,Flags);
			}

			PUSH(y, l, x-1, dy);

			if (x>x2+1) {
				PUSH(y, x2+1, x-1, -dy);	/* leak on right? */
			}

skip:
			for (x++; x<=x2 && PixelRead(x, y)!=ov; x++);
			l = x;
		} while (x<=x2);
    }
}

//#endif


void CHeightMap::SetVertexFlip(DWORD TileNum,DWORD VertexFlip)
{
	if(VertexFlip) {
		m_MapTiles[TileNum].Flags |= TF_VERTEXFLIP;
	} else {
		m_MapTiles[TileNum].Flags &= ~TF_VERTEXFLIP;
	}
	FixTileNormals(&m_MapTiles[TileNum]);
}

void CHeightMap::SetTextureFlip(DWORD TileNum,BOOL FlipX,BOOL FlipY)
{
	if(FlipX) {
		m_MapTiles[TileNum].Flags |= TF_TEXTUREFLIPX;
	} else {
		m_MapTiles[TileNum].Flags &= ~TF_TEXTUREFLIPX;
	}
	if(FlipY) {
		m_MapTiles[TileNum].Flags |= TF_TEXTUREFLIPY;
	} else {
		m_MapTiles[TileNum].Flags &= ~TF_TEXTUREFLIPY;
	}
}


	
BOOL CHeightMap::GetTileGateway(int x,int y)
{
	return m_MapTiles[(y*m_MapWidth)+x].Flags & TF_TEXTUREGATEWAY;
}


void CHeightMap::SetTileGateway(int x,int y,BOOL IsGateway)
{
	if(IsGateway) {
		m_MapTiles[(y*m_MapWidth)+x].Flags |= TF_TEXTUREGATEWAY;
	} else {
		m_MapTiles[(y*m_MapWidth)+x].Flags &= ~TF_TEXTUREGATEWAY;
	}
}


BOOL CHeightMap::GetTileGateway(DWORD TileNum)
{
	return m_MapTiles[TileNum].Flags & TF_TEXTUREGATEWAY;
}


void CHeightMap::SetTileGateway(DWORD TileNum,BOOL IsGateway)
{
	if(IsGateway) {
		m_MapTiles[TileNum].Flags |= TF_TEXTUREGATEWAY;
	} else {
		m_MapTiles[TileNum].Flags &= ~TF_TEXTUREGATEWAY;
	}
}


void CHeightMap::SetTextureRotate(DWORD TileNum,DWORD Rotate)
{
	Rotate <<= TF_TEXTUREROTSHIFT;
	m_MapTiles[TileNum].Flags &= ~TF_TEXTUREROTMASK;
	m_MapTiles[TileNum].Flags |= Rotate;
}


float CHeightMap::GetTileHeight(int Index)
{
	return GetVertexHeight(Index,0);
}

void CHeightMap::RaiseTile(int Index,float dy)
{
	float y0,y1,y2,y3;
	y0 = GetVertexHeight(Index,0);
	y1 = GetVertexHeight(Index,1);
	y2 = GetVertexHeight(Index,2);
	y3 = GetVertexHeight(Index,3);

	y0 += dy;
	y1 += dy;
	y2 += dy;
	y3 += dy;

	if(y0 < 0.0F) y0 = 0.0F;
	if(y0 > 255.0F) y0 = 255.0F;
	if(y1 < 0.0F) y1 = 0.0F;
	if(y1 > 255.0F) y1 = 255.0F;
	if(y2 < 0.0F) y2 = 0.0F;
	if(y2 > 255.0F) y2 = 255.0F;
	if(y3 < 0.0F) y3 = 0.0F;
	if(y3 > 255.0F) y3 = 255.0F;

// Set the tiles vertex heights.
	SetVertexHeight(Index,0,y0);
	SetVertexHeight(Index,1,y1);
	SetVertexHeight(Index,2,y2);
	SetVertexHeight(Index,3,y3);

// Now make the ones adjoining it match.
	int x = Index%m_MapWidth;
	int y = Index/m_MapWidth;

//	DebugPrint("X %d Y %d\n",x,y);

	if(x > 0) {
		SetVertexHeight(Index-1,1,y0);
		SetVertexHeight(Index-1,2,y3);
	}

	if((x > 0) && (y > 0)) {
		SetVertexHeight(Index-m_MapWidth-1,2,y0);
	}

	if(y > 0) {
		SetVertexHeight(Index-m_MapWidth,2,y1);
		SetVertexHeight(Index-m_MapWidth,3,y0);
	}

	if((y > 0) && (x < m_MapWidth-1)) {
		SetVertexHeight(Index-m_MapWidth+1,3,y1);
	}

	if(x < m_MapWidth-1) {
		SetVertexHeight(Index+1,3,y2);
		SetVertexHeight(Index+1,0,y1);
	}

	if((y < m_MapHeight-1) && (x < m_MapWidth-1)) {
		SetVertexHeight(Index+m_MapWidth+1,0,y2);
	}

	if(y < m_MapHeight-1) {
		SetVertexHeight(Index+m_MapWidth,0,y3);
		SetVertexHeight(Index+m_MapWidth,1,y2);
	}

	if((y < m_MapHeight-1) && (x > 0)) {
		SetVertexHeight(Index+m_MapWidth-1,1,y3);
	}
}
	
void CHeightMap::SetTileHeight(int Index,float Height)
{
// Set the tiles vertex heights.
	SetVertexHeight(Index,0,Height);
	SetVertexHeight(Index,1,Height);
	SetVertexHeight(Index,2,Height);
	SetVertexHeight(Index,3,Height);

// Now make the ones adjoining it match.
	int x = Index%m_MapWidth;
	int y = Index/m_MapWidth;

//	DebugPrint("X %d Y %d\n",x,y);

	if(x > 0) {
		SetVertexHeight(Index-1,1,Height);
		SetVertexHeight(Index-1,2,Height);
	}

	if((x > 0) && (y > 0)) {
		SetVertexHeight(Index-m_MapWidth-1,2,Height);
	}

	if(y > 0) {
		SetVertexHeight(Index-m_MapWidth,2,Height);
		SetVertexHeight(Index-m_MapWidth,3,Height);
	}

	if((y > 0) && (x < m_MapWidth-1)) {
		SetVertexHeight(Index-m_MapWidth+1,3,Height);
	}

	if(x < m_MapWidth-1) {
		SetVertexHeight(Index+1,3,Height);
		SetVertexHeight(Index+1,0,Height);
	}

	if((y < m_MapHeight-1) && (x < m_MapWidth-1)) {
		SetVertexHeight(Index+m_MapWidth+1,0,Height);
	}

	if(y < m_MapHeight-1) {
		SetVertexHeight(Index+m_MapWidth,0,Height);
		SetVertexHeight(Index+m_MapWidth,1,Height);
	}

	if((y < m_MapHeight-1) && (x > 0)) {
		SetVertexHeight(Index+m_MapWidth-1,1,Height);
	}
}

void CHeightMap::SetVertexHeight(DWORD TileNum,DWORD Index,float y)
{
	ASSERT(Index < 4);
	m_MapTiles[TileNum].Height[Index] = (UBYTE)y;
	FixTileNormals(&m_MapTiles[TileNum]);
}

void CHeightMap::SetVertexHeight(CTile *Tile,DWORD Index,float y)
{
	ASSERT(Index < 4);
	Tile->Height[Index] = (UBYTE)y;
	FixTileNormals(Tile);
}

void CHeightMap::GetTextureSize(DWORD *TextureHeight,DWORD *TextureWidth)
{
	*TextureWidth = m_TextureWidth;
	*TextureHeight = m_TextureHeight;
}

void CHeightMap::GetMapSize(unsigned int& MapWidth, unsigned int& MapHeight)
{
	MapWidth = m_MapWidth;
	MapHeight = m_MapHeight;
}

void CHeightMap::GetTileSize(DWORD *TileWidth,DWORD *TileHeight)
{
	*TileWidth = m_TileWidth;
	*TileHeight = m_TileHeight;
}

DWORD CHeightMap::GetTextureID(DWORD TileNum)
{
	return m_MapTiles[TileNum].TMapID;
}

DWORD CHeightMap::GetVertexFlip(DWORD TileNum)
{
	return m_MapTiles[TileNum].Flags & TF_VERTEXFLIP ? 1 : 0;
}

BOOL CHeightMap::GetTextureFlipX(DWORD TileNum)
{
	return m_MapTiles[TileNum].Flags & TF_TEXTUREFLIPX ? 1 : 0;
}

BOOL CHeightMap::GetTextureFlipY(DWORD TileNum)
{
	return m_MapTiles[TileNum].Flags & TF_TEXTUREFLIPY ? 1 : 0;
}

DWORD CHeightMap::GetTextureRotate(DWORD TileNum)
{
	return (m_MapTiles[TileNum].Flags & TF_TEXTUREROTMASK) >> TF_TEXTUREROTSHIFT;
}

float CHeightMap::GetVertexHeight(DWORD TileNum,DWORD Index)
{
	return (float)m_MapTiles[TileNum].Height[Index];
}

void CHeightMap::SetHeightScale(DWORD HeightScale)
{
	SLONG	x,z;
	CTile *Tile = m_MapTiles;

	m_HeightScale = HeightScale;

	for(z=0; z<m_MapHeight; z++) {
		for(x=0; x<m_MapWidth; x++) {
//			Tile->Vertices[0].y /= m_HeightScale;
//			Tile->Vertices[1].y /= m_HeightScale;
//			Tile->Vertices[2].y /= m_HeightScale;
//			Tile->Vertices[3].y /= m_HeightScale;
//
//			Tile->Vertices[0].y *= HeightScale;
//			Tile->Vertices[1].y *= HeightScale;
//			Tile->Vertices[2].y *= HeightScale;
//			Tile->Vertices[3].y *= HeightScale;
//
			FixTileNormals(Tile);
			Tile++;
		}
	}

	InitialiseSectors();
}

_inline void CHeightMap::HeightToV3(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x - m_TileWidth/2;
	Result->z = Offset->z - m_TileHeight/2;
	Result->y = ((float)Height) * m_HeightScale;
}

_inline void CHeightMap::HeightToV2(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x + m_TileWidth/2;
	Result->z = Offset->z - m_TileHeight/2;
	Result->y = ((float)Height) * m_HeightScale;
}

_inline void CHeightMap::HeightToV1(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x + m_TileWidth/2;
	Result->z = Offset->z + m_TileHeight/2;
	Result->y = ((float)Height) * m_HeightScale;
}

_inline void CHeightMap::HeightToV0(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x - m_TileWidth/2;
	Result->z = Offset->z + m_TileHeight/2;
	Result->y = ((float)Height) * m_HeightScale;
}


_inline void CHeightMap::FHeightToV3(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x - m_TileWidth/2;
	Result->z = Offset->z - m_TileHeight/2;
	Result->y = 0.0F;	//((float)Height) * m_HeightScale;
}

_inline void CHeightMap::FHeightToV2(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x + m_TileWidth/2;
	Result->z = Offset->z - m_TileHeight/2;
	Result->y = 0.0F;	//((float)Height) * m_HeightScale;
}

_inline void CHeightMap::FHeightToV1(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x + m_TileWidth/2;
	Result->z = Offset->z + m_TileHeight/2;
	Result->y = 0.0F;	//((float)Height) * m_HeightScale;
}

_inline void CHeightMap::FHeightToV0(UBYTE Height,D3DVECTOR *Offset,D3DVECTOR *Result)
{
	Result->x = Offset->x - m_TileWidth/2;
	Result->z = Offset->z + m_TileHeight/2;
	Result->y = 0.0F;	//((float)Height) * m_HeightScale;
}



void CHeightMap::FixTileVerticies(CTile *Tile,SLONG x,SLONG z,DWORD Flags)
{
	if(Flags & FIX_Y) {
		Tile->Height[0] = DEFAULTHEIGHT;
		Tile->Height[1] = DEFAULTHEIGHT;
		Tile->Height[2] = DEFAULTHEIGHT;
		Tile->Height[3] = DEFAULTHEIGHT;
	}
}

void CHeightMap::FixTilePositions()
{
	CTile *Tile = m_MapTiles;

	SLONG x,z;

	for(z=0; z<m_MapHeight; z++) {
		for(x=0; x<m_MapWidth; x++) {
			Tile->Position.x = (float)(x*m_TileWidth+m_TileWidth/2) - (float)(m_MapWidth*m_TileWidth/2);
			Tile->Position.z = -((float)(z*m_TileHeight+m_TileHeight/2) - (float)(m_MapHeight*m_TileHeight/2));
			Tile->Position.y = 0.0F;
			Tile++;
		}
	}
}

#if(0)
void CHeightMap::SmoothNormals()
{
	SLONG x,z;

	for(z=0; z<m_MapHeight-1; z++) {
		for(x=0; x<m_MapWidth-1; x++) {

			CTile *Tile = m_MapTiles+x+z*m_MapWidth;

			Tile->VertexNormals[2].x += (Tile+1)->VertexNormals[3].x;
			Tile->VertexNormals[2].y += (Tile+1)->VertexNormals[3].y;
			Tile->VertexNormals[2].z += (Tile+1)->VertexNormals[3].z;

			Tile->VertexNormals[2].x += (Tile+1+m_MapWidth)->VertexNormals[0].x;
			Tile->VertexNormals[2].y += (Tile+1+m_MapWidth)->VertexNormals[0].y;
			Tile->VertexNormals[2].z += (Tile+1+m_MapWidth)->VertexNormals[0].z;

			Tile->VertexNormals[2].x += (Tile+m_MapWidth)->VertexNormals[1].x;
			Tile->VertexNormals[2].y += (Tile+m_MapWidth)->VertexNormals[1].y;
			Tile->VertexNormals[2].z += (Tile+m_MapWidth)->VertexNormals[1].z;

			Tile->VertexNormals[2].x /= 4;
			Tile->VertexNormals[2].y /= 4;
			Tile->VertexNormals[2].z /= 4;

			(Tile+1)->VertexNormals[3] = Tile->VertexNormals[2];
			(Tile+1+m_MapWidth)->VertexNormals[0] = Tile->VertexNormals[2];
			(Tile+m_MapWidth)->VertexNormals[1] = Tile->VertexNormals[2];
		}
	}
}

#else

  /*

	NORMAL_A -> NORMAL_D
	
	-------		-------
	|\	A |		| C  /|
	| \	  |     |   / |
	|  \  |     |  /  |
	| B \ |     | / D |
	-------     -------

*/

#define NORMAL_A (1)
#define NORMAL_B (2)
#define NORMAL_C (4)
#define NORMAL_D (8)

#define TT_INVALID (0)
#define TT_AB (1)
#define TT_CD (2)

UDWORD GetTileFlipType(SDWORD MapX, SDWORD MapY);
void AddNormal(SDWORD MapX, SDWORD MapY,UDWORD AddedNormals,D3DVECTOR *SummedVector);
void SmoothNormals();

// Returns the split type of the tile - see the little diagram above
//
UDWORD CHeightMap::GetTileFlipType(SDWORD MapX, SDWORD MapY)
{
	// Check for map coords being out of range
	if ((MapX<0) || (MapY<0)) return TT_INVALID;
	if ((MapX>=m_MapWidth) || (MapY>=m_MapHeight)) return TT_INVALID;

	// Are the tri's flipped?
	if(m_MapTiles[MapY*m_MapWidth + MapX].Flags & TF_VERTEXFLIP) return TT_CD;

	// Not flipped!
	return TT_AB;
}


void CHeightMap::AddNormal(SDWORD MapX, SDWORD MapY,UDWORD AddedNormals,D3DVECTOR *SummedVector)
{
	D3DVECTOR Norm;

	D3DVECTOR Coords[4];

	ASSERT( (MapX>=0) && (MapY>=0));
	ASSERT( (MapX>=m_MapWidth) && (MapY>=m_MapHeight));

	CTile *Tile = &m_MapTiles[MapY*m_MapWidth + MapX];

	HeightToV0(Tile->Height[0],&Tile->Position,&Coords[0]);
	HeightToV1(Tile->Height[1],&Tile->Position,&Coords[1]);
	HeightToV2(Tile->Height[2],&Tile->Position,&Coords[2]);
	HeightToV3(Tile->Height[3],&Tile->Position,&Coords[3]);

	if (AddedNormals & NORMAL_A)
	{
		m_DirectMaths->CalcNormal(&Coords[0],&Coords[1],&Coords[2],&Norm);
		SummedVector->x+=Norm.x;
		SummedVector->y+=Norm.y;
		SummedVector->z+=Norm.z;
	}

	if (AddedNormals & NORMAL_B)
	{
		m_DirectMaths->CalcNormal(&Coords[0],&Coords[2],&Coords[3],&Norm);
		SummedVector->x+=Norm.x;
		SummedVector->y+=Norm.y;
		SummedVector->z+=Norm.z;
	}
	if (AddedNormals & NORMAL_C)
	{
		m_DirectMaths->CalcNormal(&Coords[0],&Coords[1],&Coords[3],&Norm);
		SummedVector->x+=Norm.x;
		SummedVector->y+=Norm.y;
		SummedVector->z+=Norm.z;
	}
	if (AddedNormals & NORMAL_D)
	{
		m_DirectMaths->CalcNormal(&Coords[1],&Coords[2],&Coords[3],&Norm);
		SummedVector->x+=Norm.x;
		SummedVector->y+=Norm.y;
		SummedVector->z+=Norm.z;
	}
}


// Calculates all the lighting levels for each vertex
void CHeightMap::SmoothNormals()
{
	D3DVECTOR Normal;

	for (int MapY=0; MapY<m_MapHeight; MapY++)
	{
		for (int MapX=0; MapX<m_MapWidth; MapX++)
		{
			// Reset the normal
			Normal.x=Normal.y=Normal.z=0.0F;
			
			// Add in the normals  - upto 8 per vertex depending on the surrounding tile splitting arrangment
			// minimum of 1 normal - which would be on the corner of the map (no surrounding tiles)

			switch (GetTileFlipType(MapX-1,MapY-1))
			{
				case TT_AB:
					AddNormal(MapX-1,MapY-1,NORMAL_A|NORMAL_B,&Normal);
					break;
				case TT_CD:
					AddNormal(MapX-1,MapY-1,NORMAL_D,&Normal);
					break;
				default:
					break;
			}

			switch (GetTileFlipType(MapX,MapY-1))
			{
				case TT_AB:
					AddNormal(MapX,MapY-1,NORMAL_B,&Normal);
					break;
				case TT_CD:
					AddNormal(MapX,MapY-1,NORMAL_C|NORMAL_D,&Normal);
					break;
				default:
					break;
			}

			switch (GetTileFlipType(MapX-1,MapY))
			{
				case TT_AB:
					AddNormal(MapX-1,MapY,NORMAL_A,&Normal);
					break;
				case TT_CD:
					AddNormal(MapX-1,MapY,NORMAL_C|NORMAL_D,&Normal);
					break;
				default:
					break;
			}

			switch (GetTileFlipType(MapX,MapY))
			{
				case TT_AB:
					AddNormal(MapX,MapY,NORMAL_A|NORMAL_B,&Normal);
					break;
				case TT_CD:
					AddNormal(MapX,MapY,NORMAL_C,&Normal);
					break;
				default:
					break;
			}

			m_DirectMaths->D3DVECTORNormalise(&Normal);

			ASSERT( (MapX>=0) && (MapY>=0));
			ASSERT( (MapX>=m_MapWidth) && (MapY>=m_MapHeight));

			m_MapTiles[MapY*m_MapWidth + MapX].VertexNormals[0] = Normal;

			if((MapY-1 >= 0) && (MapX-1 >= 0)) {
				m_MapTiles[(MapY-1)*m_MapWidth + MapX-1].VertexNormals[2] = Normal;
			}
			if(MapY-1 >= 0) {
				m_MapTiles[(MapY-1)*m_MapWidth + MapX].VertexNormals[3] = Normal;
			}
			if(MapX-1 >= 0) {
				m_MapTiles[MapY*m_MapWidth + MapX-1].VertexNormals[1] = Normal;
			}
		}
	}
}
#endif

void CHeightMap::FixTileNormals(CTile *Tile)
{
	D3DVECTOR	v0,v1,v2;

	if(Tile->Flags & TF_VERTEXFLIP) {
		HeightToV0(Tile->Height[0],&Tile->Position,&v0);
		HeightToV1(Tile->Height[1],&Tile->Position,&v1);
		HeightToV3(Tile->Height[3],&Tile->Position,&v2);
		v0.x -= Tile->Position.x; v0.y -= Tile->Position.y; v0.z -= Tile->Position.z;
		v1.x -= Tile->Position.x; v1.y -= Tile->Position.y; v1.z -= Tile->Position.z;
		v2.x -= Tile->Position.x; v2.y -= Tile->Position.y; v2.z -= Tile->Position.z;
		m_DirectMaths->CalcPlaneEquation(&v0,&v1,&v2,&Tile->FaceNormals[0],&Tile->Offsets[0]);
		Tile->VertexNormals[0] = Tile->FaceNormals[0];
		Tile->VertexNormals[1] = Tile->FaceNormals[0];
		Tile->VertexNormals[3] = Tile->FaceNormals[0];

		HeightToV1(Tile->Height[1],&Tile->Position,&v0);
		HeightToV2(Tile->Height[2],&Tile->Position,&v1);
		HeightToV3(Tile->Height[3],&Tile->Position,&v2);
		v0.x -= Tile->Position.x; v0.y -= Tile->Position.y; v0.z -= Tile->Position.z;
		v1.x -= Tile->Position.x; v1.y -= Tile->Position.y; v1.z -= Tile->Position.z;
		v2.x -= Tile->Position.x; v2.y -= Tile->Position.y; v2.z -= Tile->Position.z;
		m_DirectMaths->CalcPlaneEquation(&v0,&v1,&v2,&Tile->FaceNormals[1],&Tile->Offsets[1]);
		Tile->VertexNormals[2] = Tile->FaceNormals[1];
	} else {
		HeightToV0(Tile->Height[0],&Tile->Position,&v0);
		HeightToV1(Tile->Height[1],&Tile->Position,&v1);
		HeightToV2(Tile->Height[2],&Tile->Position,&v2);
		v0.x -= Tile->Position.x; v0.y -= Tile->Position.y; v0.z -= Tile->Position.z;
		v1.x -= Tile->Position.x; v1.y -= Tile->Position.y; v1.z -= Tile->Position.z;
		v2.x -= Tile->Position.x; v2.y -= Tile->Position.y; v2.z -= Tile->Position.z;
		m_DirectMaths->CalcPlaneEquation(&v0,&v1,&v2,&Tile->FaceNormals[0],&Tile->Offsets[0]);
		Tile->VertexNormals[0] = Tile->FaceNormals[0];
		Tile->VertexNormals[1] = Tile->FaceNormals[0];
		Tile->VertexNormals[2] = Tile->FaceNormals[0];

		HeightToV0(Tile->Height[0],&Tile->Position,&v0);
		HeightToV2(Tile->Height[2],&Tile->Position,&v1);
		HeightToV3(Tile->Height[3],&Tile->Position,&v2);
		v0.x -= Tile->Position.x; v0.y -= Tile->Position.y; v0.z -= Tile->Position.z;
		v1.x -= Tile->Position.x; v1.y -= Tile->Position.y; v1.z -= Tile->Position.z;
		v2.x -= Tile->Position.x; v2.y -= Tile->Position.y; v2.z -= Tile->Position.z;
		m_DirectMaths->CalcPlaneEquation(&v0,&v1,&v2,&Tile->FaceNormals[1],&Tile->Offsets[1]);
		Tile->VertexNormals[3] = Tile->FaceNormals[1];
	}
}

void CHeightMap::FixTileTextures(CTile *Tile)
{
//   	Tile->Vertices[0].tu=m_TextureMaps[Tile->TMapID].u0;
//   	Tile->Vertices[0].tv=m_TextureMaps[Tile->TMapID].v0;
//   	Tile->Vertices[1].tu=m_TextureMaps[Tile->TMapID].u1;
//   	Tile->Vertices[1].tv=m_TextureMaps[Tile->TMapID].v1;
//   	Tile->Vertices[2].tu=m_TextureMaps[Tile->TMapID].u2;
//   	Tile->Vertices[2].tv=m_TextureMaps[Tile->TMapID].v2;
//   	Tile->Vertices[3].tu=m_TextureMaps[Tile->TMapID].u3;
//   	Tile->Vertices[3].tv=m_TextureMaps[Tile->TMapID].v3;
}


void CHeightMap::InitialiseSectors()
{
/*
	DWORD	NumTilesIn;
	SLONG	xs,zs;
	DWORD	i;
	DWORD	x,z;

	if(m_MapSectors) {
		CMapSector *CurSector = m_MapSectors;
		for(i=0; i<m_NumSectors; i++) {
			delete CurSector->TileIndices;
			CurSector++;
		}

		delete m_MapSectors;
	}

	m_MapSectors = new CMapSector[m_SectorsPerRow*m_SectorsPerColumn];
	CMapSector	*CurSector=m_MapSectors;
	m_NumSectors=0;

	for(z=0; z < (DWORD)m_SectorsPerColumn; z++) {
		for(x=0; x < (DWORD)m_SectorsPerRow; x++) {

// Create sector.
			NumTilesIn=0;
//			DebugPrint("%d,%d\n",x,z);

//			CurSector->TileIndices=new DWORD[(m_MapWidth/SECTORSPERROW)*(m_MapHeight/SECTORSPERROW)];
			CurSector->TileIndices=new DWORD[m_TilesPerSectorRow * m_TilesPerSectorColumn];
			CurSector->Hidden = 0;
//			for(zs=0; zs < m_MapHeight/SECTORSPERROW; zs++) {
//				for(xs=0; xs < m_MapWidth/SECTORSPERROW; xs++) {
			for(zs=0; zs < m_TilesPerSectorColumn; zs++) {
				for(xs=0; xs < m_TilesPerSectorRow; xs++) {
//					DWORD TileIndex = (z*(m_MapHeight/SECTORSPERROW)+zs)*m_MapHeight + 
//											(x*(m_MapWidth/SECTORSPERROW)+xs);
					DWORD TileIndex = (z*(m_TilesPerSectorRow)+zs)*m_MapHeight + 
											(x*(m_TilesPerSectorColumn)+xs);

					assert(TileIndex < m_MapWidth*m_MapHeight);

					CurSector->TileIndices[NumTilesIn]=TileIndex;
//					DebugPrint("%d ",CurSector->TileIndices[i]);

//					if(m_MapTiles[TileIndex].Flags & TF_HIDE) {
//						CurSector->Hidden++;
//					}
					NumTilesIn++;
				}
//				DebugPrint("\n");
			}
//			DebugPrint("\n");

			CurSector->TileCount = NumTilesIn;
			InitialiseBoundingBox(CurSector);

			CurSector++;
			m_NumSectors++;
		}
	}
*/
	SmoothNormals();
	
//	DebugPrint("NumTiles %d\nNumSectors %d\n",m_NumTiles,m_NumSectors);
//	DebugPrint("NumTilesIn %d\n",NumTilesIn);
}


void CHeightMap::InitialiseBoundingBox(CMapSector *Sector)
{
// Calculate bounding box for this sector.

	D3DVECTOR	Smallest,Largest;
	DWORD	i;

	Smallest.x=D3DVAL(m_MapWidth*m_TileWidth);
	Smallest.y=D3DVAL(1000000);
	Smallest.z=D3DVAL(m_MapHeight*m_TileHeight);
	Largest.x=D3DVAL(-m_MapWidth*m_TileWidth);
	Largest.y=D3DVAL(-1000000);
	Largest.z=D3DVAL(-m_MapHeight*m_TileHeight);

	D3DVECTOR v0,v1,v2,v3;
	for(i=0; i<Sector->TileCount; i++) {
		HeightToV0(m_MapTiles[Sector->TileIndices[i]].Height[0],&m_MapTiles[Sector->TileIndices[i]].Position,&v0);
		HeightToV1(m_MapTiles[Sector->TileIndices[i]].Height[1],&m_MapTiles[Sector->TileIndices[i]].Position,&v1);
		HeightToV2(m_MapTiles[Sector->TileIndices[i]].Height[2],&m_MapTiles[Sector->TileIndices[i]].Position,&v2);
		HeightToV3(m_MapTiles[Sector->TileIndices[i]].Height[3],&m_MapTiles[Sector->TileIndices[i]].Position,&v3);

		if(v0.x < Smallest.x) Smallest.x=v0.x;
		if(v0.y < Smallest.y) Smallest.y=v0.y;
		if(v0.z < Smallest.z) Smallest.z=v0.z;
		if(v0.x > Largest.x) Largest.x=v0.x;
		if(v0.y > Largest.y) Largest.y=v0.y;
		if(v0.z > Largest.z) Largest.z=v0.z;
		if(v1.x < Smallest.x) Smallest.x=v1.x;
		if(v1.y < Smallest.y) Smallest.y=v1.y;
		if(v1.z < Smallest.z) Smallest.z=v1.z;
		if(v1.x > Largest.x) Largest.x=v1.x;
		if(v1.y > Largest.y) Largest.y=v1.y;
		if(v1.z > Largest.z) Largest.z=v1.z;
		if(v2.x < Smallest.x) Smallest.x=v2.x;
		if(v2.y < Smallest.y) Smallest.y=v2.y;
		if(v2.z < Smallest.z) Smallest.z=v2.z;
		if(v2.x > Largest.x) Largest.x=v2.x;
		if(v2.y > Largest.y) Largest.y=v2.y;
		if(v2.z > Largest.z) Largest.z=v2.z;
		if(v3.x < Smallest.x) Smallest.x=v3.x;
		if(v3.y < Smallest.y) Smallest.y=v3.y;
		if(v3.z < Smallest.z) Smallest.z=v3.z;
		if(v3.x > Largest.x) Largest.x=v3.x;
		if(v3.y > Largest.y) Largest.y=v3.y;
		if(v3.z > Largest.z) Largest.z=v3.z;
	}

	Sector->BoundingBox[0].x=Smallest.x;
	Sector->BoundingBox[0].y=Smallest.y;
	Sector->BoundingBox[0].z=Smallest.z;

	Sector->BoundingBox[1].x=Largest.x;
	Sector->BoundingBox[1].y=Smallest.y;
	Sector->BoundingBox[1].z=Smallest.z;

	Sector->BoundingBox[2].x=Largest.x;
	Sector->BoundingBox[2].y=Largest.y;
	Sector->BoundingBox[2].z=Smallest.z;

	Sector->BoundingBox[3].x=Smallest.x;
	Sector->BoundingBox[3].y=Largest.y;
	Sector->BoundingBox[3].z=Smallest.z;


	Sector->BoundingBox[4].x=Largest.x;
	Sector->BoundingBox[4].y=Largest.y;
	Sector->BoundingBox[4].z=Largest.z;

	Sector->BoundingBox[5].x=Smallest.x;
	Sector->BoundingBox[5].y=Largest.y;
	Sector->BoundingBox[5].z=Largest.z;

	Sector->BoundingBox[6].x=Smallest.x;
	Sector->BoundingBox[6].y=Smallest.y;
	Sector->BoundingBox[6].z=Largest.z;

	Sector->BoundingBox[7].x=Largest.x;
	Sector->BoundingBox[7].y=Smallest.y;
	Sector->BoundingBox[7].z=Largest.z;
}


// Render the heightmap
//
void CHeightMap::DrawHeightMap(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	Draw3DSectors(CameraRotation,CameraPosition);
}


void CHeightMap::Draw3DGrid(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
}


void CHeightMap::Draw2DMap(CDIBDraw *DIBDraw,DDImage **Images,int NumImages,int ScrollX, int ScrollY)
{
	if(m_TextureWidth==0) return;	// For some reason this only happens when optimized.
	if(m_TextureHeight==0) return;

	char String[8];
	HWND hWnd = (HWND)DIBDraw->GetDIBWindow();
	RECT WindSize;

	GetClientRect(hWnd,&WindSize);

	SLONG	StartX = ScrollX/(int)m_TextureWidth;
	SLONG	StartZ = ScrollY/(int)m_TextureHeight;
	SLONG	EndX = StartX+(int)(WindSize.right/m_TextureWidth)+1;
	SLONG	EndZ = StartZ+(int)(WindSize.bottom/m_TextureHeight)+1;
	SLONG	x,z;
	SWORD	TMapID;

	int SelectionX0,SelectionY0;
	int SelectionX1,SelectionY1;
	int Tmp;

	SelectionX0 = m_SelectionX0;
	SelectionY0 = m_SelectionY0;
	SelectionX1 = m_SelectionX1;
	SelectionY1 = m_SelectionY1;
	if(SelectionX1 < SelectionX0) {
		Tmp = SelectionX1;
		SelectionX1 = SelectionX0;
		SelectionX0 = Tmp;
	}

	if(SelectionY1 < SelectionY0) {
		Tmp = SelectionY1;
		SelectionY1 = SelectionY0;
		SelectionY0 = Tmp;
	}

	HDC hdc = (HDC)DIBDraw->GetDIBDC();
	HPEN NormalPen = CreatePen(PS_SOLID,1,RGB(255,255,255));
	HPEN OldPen = (HPEN)SelectObject(hdc,NormalPen);

	for(z=StartZ; z<EndZ; z++) {
		for(x=StartX; x<EndX; x++) {
			if( (x>=0) && (x<m_MapWidth) && (z>=0) && (z<m_MapHeight)) {
				CTile *Tile = &m_MapTiles[z*m_MapWidth+x];
				if( (Tile->Flags & TF_HIDE) == 0) {
					TMapID = (SWORD)(Tile->TMapID)-1;
					if(TMapID >= NumImages) {
						TMapID = NumImages-1;
					}
					if(TMapID>=0) {
	   					Images[TMapID]->DDBlitImageDIB(DIBDraw,
														(x-StartX)*m_TextureWidth,
														(z-StartZ)*m_TextureHeight,
														Tile->Flags);
//						DisplayTerrainType(DIBDraw,(x-StartX)*m_TextureWidth,
//												  	(z-StartZ)*m_TextureHeight,
//													m_TextureWidth,m_TextureHeight,
//												  	Tile->Flags);
						DisplayTerrainType(DIBDraw,(x-StartX)*m_TextureWidth,
												  	(z-StartZ)*m_TextureHeight,
													m_TextureWidth,m_TextureHeight,
												  	g_Types[TMapID+1]);

						if(g_OverlayZoneIDs) {
							int tx = (x-StartX)*m_TextureWidth;
							int ty = (z-StartZ)*m_TextureHeight;
							sprintf(String, "%d",Tile->ZoneID);

							TextOut(hdc,tx,ty,String,strlen(String));
						}

	 					if(SelectionBoxValid()) {
							if( (x >= SelectionX0) && ( x <= SelectionX1) && 
								(z >= SelectionY0) && ( z <= SelectionY1) ) {

								int x0 = (x-StartX) * m_TextureWidth;
								int y0 = (z-StartZ) * m_TextureHeight;
								int x1 = (x-StartX+1) * m_TextureWidth;
								int y1 = (z-StartZ+1) * m_TextureHeight;
								x1 --;
								y1 --;

 								MoveToEx(hdc,x0,y0,NULL);
   								LineTo(hdc,x1,y0);
   								LineTo(hdc,x1,y1);
   								LineTo(hdc,x0,y1);
   								LineTo(hdc,x0,y0);
							}
						}
					}
				}
			}
		}
	}

	SelectObject(hdc,OldPen);
	DeleteObject(NormalPen);

	ScrollX/=(int)m_TextureWidth;
	ScrollY/=(int)m_TextureHeight;

	if (m_ScrollLimits.empty())
		return;

	HDC	dc=(HDC)DIBDraw->GetDIBDC();
	NormalPen = CreatePen(PS_SOLID,1,RGB(255,0,255));
	OldPen = (HPEN)SelectObject(dc,NormalPen);
	
	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		int x0 = (curNode->MinX - ScrollX) * m_TextureWidth;
		int y0 = (curNode->MinZ - ScrollY) * m_TextureHeight;
		int x1 = (curNode->MaxX - ScrollX) * m_TextureWidth; 
		int y1 = (curNode->MaxZ - ScrollY) * m_TextureHeight;

		MoveToEx(dc, x0, y0, NULL);
		LineTo(dc, x1, y0);
		LineTo(dc, x1, y1);
		LineTo(dc, x0, y1);
		LineTo(dc, x0, y0);
//		curNode->MinX;
//		curNode->MinZ;
//		curNode->MaxX;
//		curNode->MaxZ;
	}

	SelectObject(dc,OldPen);
	DeleteObject(NormalPen);
}


//void CHeightMap::DrawTileAttribute(CDIBDraw *DIBDraw,int XPos,int YPos,DWORD Flags)
//{
//	COLORREF Colour;
//
//	switch(Flags&TF_TYPEALL) {
//		case TF_TYPEGRASS:
//			Colour = RGB(0x00,0x77,0x00);
//			break;
//		case TF_TYPESTONE:
//			Colour = RGB(0x77,0x77,0x77);
//			break;
//		case TF_TYPESAND:
//			Colour = RGB(0xff,0xff,0x00);
//			break;
//		case TF_TYPEWATER:
//			Colour = RGB(0x00,0x00,0xFF);
//			break;
//		default:
//			Colour = RGB(0x00,0x00,0x00);
//	}
//
//	HDC hdc = DIBDraw->GetDIBDC();;
//	HBRUSH Brush = CreateSolidBrush(Colour);
//	RECT Rect;
//	SetRect(&Rect,XPos+24,YPos+24,XPos+24+16,YPos+24+16);
//	FillRect(hdc,&Rect,Brush);
//	DeleteObject(Brush);
//
////	SetBkColor(hdc,RGB(Colour);
////	SetTextColor(hdc,RGB(0,0,0));
////	TextOut(DIBDraw->GetDIBDC(),XPos,YPos,"R",1);
//}


int CHeightMap::Select2DFace(DWORD XPos,DWORD YPos,int ScrollX,int ScrollY)
{
	SLONG	SelX = (ScrollX/(int)m_TextureWidth)+(int)(XPos/m_TextureWidth);
	SLONG	SelY = (ScrollY/(int)m_TextureHeight)+(int)(YPos/m_TextureHeight);

	int Selected = SelY*m_MapWidth + SelX;

	if((SelX < 0) || (SelY < 0)) {
		return -1;
	}

	if((SelX >= m_MapWidth) || (SelY >= m_MapHeight)) {
		return -1;
	}


	ASSERT(Selected <m_MapWidth*m_MapHeight);

	return Selected;
}


void CHeightMap::DrawTile(DWORD TileNum,D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	CTile	*Tile=&m_MapTiles[TileNum];
			
	FixTileTextures(Tile);
	D3DVERTEX Vertex[4];

	if(Tile->Flags & TF_VERTEXFLIP) {
		if(m_Flatten) {
			FHeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
			FHeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
			FHeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
			FHeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
		} else {
			HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
			HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
			HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
			HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
		}
		if(m_Gouraud) {
			*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->VertexNormals[0];
			*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->VertexNormals[1];
			*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->VertexNormals[2];
			*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->VertexNormals[3];
		} else {
			*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->FaceNormals[0];
			*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->FaceNormals[0];
			*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->FaceNormals[1];
			*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->FaceNormals[1];
		}
		SetTileVertexUV(Tile,Vertex[0],Vertex[1],Vertex[3],Vertex[2]);

		m_MatManager->DrawTriangleStrip(m_TextureMaps[Tile->TMapID].TextureID,Vertex,4);
	} else {
		if(m_Flatten) {
			FHeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
			FHeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
			FHeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
			FHeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
		} else {
			HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
			HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
			HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
			HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
		}
		if(m_Gouraud) {
			*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->VertexNormals[0];
			*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->VertexNormals[1];
			*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->VertexNormals[2];
			*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->VertexNormals[3];
		} else {
			*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->FaceNormals[0];
			*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->FaceNormals[0];
			*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->FaceNormals[1];
			*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->FaceNormals[1];
		}
		SetTileVertexUV(Tile,Vertex[0],Vertex[1],Vertex[2],Vertex[3]);

		m_MatManager->DrawTriangleFan(m_TextureMaps[Tile->TMapID].TextureID,Vertex,4);
	}
}


float CHeightMap::GetVisibleRadius()
{
	return (float)(m_DrawRadius * m_TileWidth);
//	return (float)(m_TilesPerSectorRow * m_DrawRadius * m_TileWidth);
}


void CHeightMap::DrawSea(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	float x0,z0,x1,z1;
	float xRad = (float)(m_TilesPerSectorRow * m_DrawRadius * m_TileWidth);
	float zRad = (float)(m_TilesPerSectorColumn * m_DrawRadius * m_TileHeight);

	x0=CameraPosition.x - xRad;
	z0=CameraPosition.z - zRad;
	x1=CameraPosition.x + xRad;
	z1=CameraPosition.z + zRad;

	float SeaLevel = (float)(m_SeaLevel * m_HeightScale);

	D3DVERTEX Vertex[4];
	Vertex[0].x = x0;	Vertex[0].y = (float)SeaLevel;	Vertex[0].z = z0;
	Vertex[0].nx = (float)0;	Vertex[0].ny = (float)1;	Vertex[0].nz = (float)0;
	Vertex[0].tu = Vertex[0].tv = (float)0;
	Vertex[1] = Vertex[2] = Vertex[3] = Vertex[0];
	Vertex[3].x = x1;
	Vertex[2].x = x1; Vertex[2].z = z1;
	Vertex[1].z = z1;
	m_MatManager->DrawTriangleFan(m_SeaMaterial,Vertex,4);
}


D3DVECTOR g_WorldRotation = {0.0F,0.0F,0.0F};
//#define TESTTRANSFORM // If defined, draws points instead of triangles.

void CHeightMap::Draw3DTiles(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	SLONG StartX;
	SLONG StartZ;
	SLONG EndX;
	SLONG EndZ;
	SLONG x,z;

	StartX = (SLONG)((CameraPosition.x / m_TileWidth) + m_MapWidth/2);
	StartZ = (SLONG)((-CameraPosition.z / m_TileHeight) + m_MapHeight/2);
	EndX = StartX + m_DrawRadius;
	EndZ = StartZ + m_DrawRadius;
	StartX-=m_DrawRadius;
	StartZ-=m_DrawRadius;

	if(StartX < 0) {
		StartX = 0;
	}
	if(StartZ < 0) {
		StartZ = 0;
	}
	if(EndX > m_MapWidth) {	//-1) {
		EndX = m_MapWidth;	//-1;
	}
	if(EndZ > m_MapHeight) {	//-1) {
		EndZ = m_MapHeight;	//-1;
	}
	if( (StartX >= EndX) || (StartZ >= EndZ) ) {
		return;
	}

	int SelectionX0,SelectionY0;
	int SelectionX1,SelectionY1;
	int Tmp;

	SelectionX0 = m_SelectionX0;
	SelectionY0 = m_SelectionY0;
	SelectionX1 = m_SelectionX1;
	SelectionY1 = m_SelectionY1;
	if(SelectionX1 < SelectionX0) {
		Tmp = SelectionX1;
		SelectionX1 = SelectionX0;
		SelectionX0 = Tmp;
	}

	if(SelectionY1 < SelectionY0) {
		Tmp = SelectionY1;
		SelectionY1 = SelectionY0;
		SelectionY0 = Tmp;
	}

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
//	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetObjectMatrix(&g_WorldRotation,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	CTile	*Tile;

	DWORD VIndex = 0;

//		m_DDView->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE ,TRUE);

	for(z=StartZ; z<EndZ; z++) {
		Tile = &m_MapTiles[z*m_MapWidth + StartX];
		for(x=StartX; x<EndX; x++) {
			if( (Tile->Flags & TF_HIDE) == 0 ) {
				FixTileTextures(Tile);
				D3DVERTEX Vertex[5];

				if(Tile->Flags & TF_VERTEXFLIP) {
					if(m_Flatten) {
						FHeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
						FHeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
						FHeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
						FHeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
					} else {
						HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
						HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
						HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
						HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
					}
					if(m_Gouraud) {
						*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->VertexNormals[0];
						*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->VertexNormals[1];
						*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->VertexNormals[2];
						*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->VertexNormals[3];
					} else {
						*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->FaceNormals[0];
						*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->FaceNormals[0];
						*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->FaceNormals[1];
						*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->FaceNormals[1];
					}
					SetTileVertexUV(Tile,Vertex[0],Vertex[1],Vertex[3],Vertex[2]);

					m_MatManager->DrawTriangleStrip(m_TextureMaps[Tile->TMapID].TextureID,Vertex,4);

					if(SelectionBoxValid()) {
						if( (x >= SelectionX0) && ( x <= SelectionX1) && 
							(z >= SelectionY0) && ( z <= SelectionY1) ) {

							m_DDView->SetRenderState(D3DRENDERSTATE_ZENABLE,FALSE);

							Vertex[4] = Vertex[2];
							Vertex[2] = Vertex[3];
							Vertex[3] = Vertex[4];
							
							Vertex[1].x--;
							Vertex[2].x--;
							Vertex[3].y--;
							Vertex[4] = Vertex[0];
							m_MatManager->DrawPolyLine(m_LineMaterial,Vertex,5);
							m_DDView->SetRenderState(D3DRENDERSTATE_ZENABLE,TRUE);
						}
					}
				} else {
					if(m_Flatten) {
						FHeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
						FHeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
						FHeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
						FHeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
					} else {
						HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertex[0]);
						HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertex[1]);
						HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertex[2]);
						HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertex[3]);
					}
					if(m_Gouraud) {
						*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->VertexNormals[0];
						*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->VertexNormals[1];
						*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->VertexNormals[2];
						*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->VertexNormals[3];
					} else {
						*( (D3DVECTOR*)&Vertex[0].nx ) = Tile->FaceNormals[0];
						*( (D3DVECTOR*)&Vertex[1].nx ) = Tile->FaceNormals[0];
						*( (D3DVECTOR*)&Vertex[2].nx ) = Tile->FaceNormals[1];
						*( (D3DVECTOR*)&Vertex[3].nx ) = Tile->FaceNormals[1];
					}
					SetTileVertexUV(Tile,Vertex[0],Vertex[1],Vertex[2],Vertex[3]);

  					m_MatManager->DrawTriangleFan(m_TextureMaps[Tile->TMapID].TextureID,Vertex,4);

 					if(SelectionBoxValid()) {
						if( (x >= SelectionX0) && ( x <= SelectionX1) && 
							(z >= SelectionY0) && ( z <= SelectionY1) ) {

							m_DDView->SetRenderState(D3DRENDERSTATE_ZENABLE,FALSE);
							Vertex[1].x--;
							Vertex[2].x--;
							Vertex[3].y--;
							Vertex[4] = Vertex[0];
							m_MatManager->DrawPolyLine(m_LineMaterial,Vertex,5);
							m_DDView->SetRenderState(D3DRENDERSTATE_ZENABLE,TRUE);
						}
					}
				}

				VIndex+=4;
			}

			Tile++;
		}
	}

//		m_DDView->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE ,FALSE);
}


void CHeightMap::ClearSelectionBox()
{
	m_SelectionBox0 = 0;
	m_SelectionBox1 = 0;
	m_SelectionX0 = -1;
	m_SelectionY0 = -1;
	m_SelectionX1 = -1;
	m_SelectionY1 = -1;
	m_SelectionWidth = 0;
	m_SelectionHeight = 0;
}


void CHeightMap::SetSelectionBox0(int TileID)
{
	m_SelectionBox0 = TileID;
	m_SelectionX0 = TileID % m_MapWidth;
	m_SelectionY0 = TileID / m_MapWidth;
	m_SelectionX1 = -1;
	m_SelectionY1 = -1;
	m_SelectionWidth = 0;
	m_SelectionHeight = 0;
}


void CHeightMap::SetSelectionBox1(int TileID)
{
	m_SelectionBox1 = TileID;
	m_SelectionX1 = TileID % m_MapWidth;
	m_SelectionY1 = TileID / m_MapWidth;
	m_SelectionWidth = m_SelectionX1-m_SelectionX0;
	m_SelectionHeight = m_SelectionY1-m_SelectionY0;
}


void CHeightMap::SetSelectionBox(int TileID,int Width,int Height)
{
	m_SelectionX0 = TileID % m_MapWidth;
	m_SelectionY0 = TileID / m_MapWidth;
	m_SelectionX1 = m_SelectionX0 + Width;
	m_SelectionY1 = m_SelectionY0 + Height;

	m_SelectionBox0 = TileID;
	m_SelectionBox1 = m_SelectionX1 + m_SelectionY1*m_MapWidth;

	m_SelectionWidth = Width;
	m_SelectionHeight = Height;
}


BOOL CHeightMap::SelectionBoxValid()
{
	if(m_SelectionX0 < 0) {
		return FALSE;
	}
	if(m_SelectionY0 < 0) {
		return FALSE;
	}
	if(m_SelectionX1 < 0) {
		return FALSE;
	}
	if(m_SelectionY1 < 0) {
		return FALSE;
	}

	return TRUE;
}


void CHeightMap::ClipSelectionBox()
{
	DebugPrint("%d %d %d %d (%d,%d) (%d,%d)\n",m_SelectionX0,m_SelectionY0,m_SelectionX1,m_SelectionY1,m_SelectionWidth,m_SelectionHeight, m_MapWidth,m_MapHeight);

	if(m_SelectionX0 < 0) {
		m_SelectionX0 = 0;
		m_SelectionX1 = m_SelectionWidth;
	}

	if(m_SelectionY0 < 0) {
		m_SelectionY0 = 0;
		m_SelectionY1 = m_SelectionHeight;
	}

	if(m_SelectionX0+m_SelectionWidth >= m_MapWidth) {
		m_SelectionX1 = m_MapWidth-1;
		m_SelectionX0 = m_MapWidth-1-m_SelectionWidth;
	}

	if(m_SelectionY0+m_SelectionHeight >= m_MapHeight) {
		m_SelectionY1 = m_MapHeight-1;
		m_SelectionY0 = m_MapHeight-1-m_SelectionHeight;
	}

	m_SelectionBox0 = m_SelectionX0 + m_SelectionY0*m_MapWidth;
	m_SelectionBox1 = m_SelectionX1 + m_SelectionY1*m_MapWidth;
}


void CHeightMap::Draw3DSectors(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	Draw3DTiles(CameraRotation,CameraPosition);
}

void CHeightMap::Draw3DVerticies(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	SLONG StartX;
	SLONG StartZ;
	SLONG EndX;
	SLONG EndZ;
	SLONG x,z;

	StartX = (SLONG)((CameraPosition.x / m_TileWidth) + m_MapWidth/2);
	StartZ = (SLONG)((-CameraPosition.z / m_TileHeight) + m_MapHeight/2);
	EndX = StartX + m_DrawRadius;
	EndZ = StartZ + m_DrawRadius;
	StartX-=m_DrawRadius;
	StartZ-=m_DrawRadius;

	if(StartX < 0) {
		StartX = 0;
	}
	if(StartZ < 0) {
		StartZ = 0;
	}
	if(EndX > m_MapWidth) {	//-1) {
		EndX = m_MapWidth;	//-1;
	}
	if(EndZ > m_MapHeight) { //-1) {
		EndZ = m_MapHeight;	//-1;
	}
	if( (StartX >= EndX) || (StartZ >= EndZ) ) {
		return;
	}

	PALETTEENTRY Pal[256];
	for(int i=0; i<256; i++) {
		Pal[i].peRed=255; Pal[i].peGreen=255; Pal[i].peBlue=255;
	}
	CSurface BackBuffer(m_DDView->GetBackBuffer(),Pal);

	UWORD Col1 = BackBuffer.GetValue(0,0,0);
	UWORD Col2 = BackBuffer.GetValue(255,255,255);

	BackBuffer.Lock();

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	CTile	*Tile;

	for(z=StartZ; z<EndZ; z++) {
		Tile = &m_MapTiles[z*m_MapWidth + StartX];
		for(x=StartX; x<EndX; x++) {
			if( (Tile->Flags & TF_HIDE) == 0 ) {
				D3DVERTEX Vertices[4];
				D3DVERTEX TVertices[4];
				D3DHVERTEX HVertex[4];
				BOOL AllClipped;

				HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertices[0]);

				AllClipped = !m_DirectMaths->TransformVertex(TVertices, Vertices,1,HVertex);
				if(!AllClipped) {
					if(HVertex[0].dwFlags == 0) {
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y,Col1);
						BackBuffer.PutPixel((SLONG)TVertices[0].x-1,(SLONG)TVertices[0].y,Col1);
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y-1,Col1);
						BackBuffer.PutPixel((SLONG)TVertices[0].x+1,(SLONG)TVertices[0].y,Col1);
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y+1,Col1);
						TVertices[0].x += 1;
						TVertices[0].y += 1;
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y,Col2);
						BackBuffer.PutPixel((SLONG)TVertices[0].x-1,(SLONG)TVertices[0].y,Col2);
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y-1,Col2);
						BackBuffer.PutPixel((SLONG)TVertices[0].x+1,(SLONG)TVertices[0].y,Col2);
						BackBuffer.PutPixel((SLONG)TVertices[0].x,(SLONG)TVertices[0].y+1,Col2);
					}
				}
			}

			Tile++;
		}
	}

	BackBuffer.Unlock();
}


// Set the UV coordinates for each vertex in a tile taking into account the tiles
// flip/rotation flags.
//
void CHeightMap::SetTileVertexUV(CTile *Tile,D3DVERTEX &v0,D3DVERTEX &v1,D3DVERTEX &v2,D3DVERTEX &v3)
{
// Get the UV coordinates from the texture map lookup.
	TextureDef *TMap = &m_TextureMaps[Tile->TMapID];
	v0.tu=TMap->u0;	v0.tv=TMap->v0;	v1.tu=TMap->u1;	v1.tv=TMap->v1;
	v2.tu=TMap->u2;	v2.tv=TMap->v2;	v3.tu=TMap->u3;	v3.tv=TMap->v3;


// If x flipped then swap U for v0,v1 and v2,v3 
	float Tmp;
	if(Tile->Flags & TF_TEXTUREFLIPX) {
		Tmp = v0.tu;
		v0.tu = v1.tu;
		v1.tu = Tmp;
		Tmp = v2.tu;
		v2.tu = v3.tu;
		v3.tu = Tmp;
	}

// If x flipped then swap V for v0,v3 and v1,v2 
	if(Tile->Flags & TF_TEXTUREFLIPY) {
		Tmp = v0.tv;
		v0.tv = v3.tv;
		v3.tv = Tmp;
		Tmp = v1.tv;
		v1.tv = v2.tv;
		v2.tv = Tmp;
	}

// Now handle any rotation.
	float TmpU,TmpV;
	switch(Tile->Flags & TF_TEXTUREROTMASK) {
   		case TF_TEXTUREROT90:
   			TmpU = v0.tu; TmpV = v0.tv;
   			v0.tu = v3.tu; v0.tv = v3.tv;
   			v3.tu = v2.tu; v3.tv = v2.tv;
   			v2.tu = v1.tu; v2.tv = v1.tv;
   			v1.tu = TmpU; v1.tv = TmpV;
   			break;

   		case TF_TEXTUREROT180:
   			TmpU = v0.tu; TmpV = v0.tv;
   			v0.tu = v2.tu; v0.tv = v2.tv;
   			v2.tu = TmpU; v2.tv = TmpV;
   			TmpU = v1.tu; TmpV = v1.tv;
   			v1.tu = v3.tu; v1.tv = v3.tv;
   			v3.tu = TmpU; v3.tv = TmpV;
   			break;

   		case TF_TEXTUREROT270:
   			TmpU = v0.tu; TmpV = v0.tv;
   			v0.tu = v1.tu; v0.tv = v1.tv;
   			v1.tu = v2.tu; v1.tv = v2.tv;
   			v2.tu = v3.tu; v2.tv = v3.tv;
   			v3.tu = TmpU; v3.tv = TmpV;
   			break;
   }
}

SelVertex	SelVerticies[MAXSELECTEDVERTS];
int CompareVerticies(const void *arg1,const void * arg2);


int	CHeightMap::FindVerticies(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
							DWORD XPos,DWORD YPos)
{
	SLONG StartX;
	SLONG StartZ;
	SLONG EndX;
	SLONG EndZ;
	SLONG x,z;

	StartX = (SLONG)((CameraPosition.x / m_TileWidth) + m_MapWidth/2);
	StartZ = (SLONG)((-CameraPosition.z / m_TileHeight) + m_MapHeight/2);
	EndX = StartX + m_DrawRadius;
	EndZ = StartZ + m_DrawRadius;
	StartX-=m_DrawRadius;
	StartZ-=m_DrawRadius;

	if(StartX < 0) {
		StartX = 0;
	}
	if(StartZ < 0) {
		StartZ = 0;
	}
	if(EndX > m_MapWidth-1) {
		EndX = m_MapWidth-1;
	}
	if(EndZ > m_MapHeight-1) {
		EndZ = m_MapHeight-1;
	}
	if( (StartX >= EndX) || (StartZ >= EndZ) ) {
		return 1;
	}

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	CTile	*Tile;
	D3DVERTEX Vertices[4];
	D3DVERTEX TVertices[4];
	D3DHVERTEX HVertex[4];
	BOOL	AllClipped;

	DWORD	TMap = 1;
	int	Selected = 0;
	int dx,dy;

	for(z=StartZ; z<EndZ; z++) {
		Tile = &m_MapTiles[z*m_MapWidth + StartX];
		for(x=StartX; x<EndX; x++) {
// Transform tile verticies.
			HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertices[0]);
			HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertices[1]);
			HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertices[2]);
			HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertices[3]);

			AllClipped = !m_DirectMaths->TransformVertex(TVertices, Vertices,4,HVertex);
			if(!AllClipped) {
				for(int j=0; j<4; j++) {
					if(HVertex[j].dwFlags == 0) {
						dx = abs((int)TVertices[j].x - (int)XPos);
						dy = abs((int)TVertices[j].y - (int)YPos);
						if((dx <= GRABRADIUS) && (dy <= GRABRADIUS)) {
							assert(Selected < MAXSELECTEDVERTS);
							SelVerticies[Selected].TileIndex = z*m_MapWidth + x;
							SelVerticies[Selected].VertIndex = j;
							SelVerticies[Selected].z = TVertices[j].z;
							Selected++;
						}
					}
				}
			}
			Tile++;
		}
	}

	qsort((void*)SelVerticies,Selected,sizeof(SelVertex),CompareVerticies);

	int NumSel = 1;

	for(int i=1; i<(DWORD)Selected; i++) {
		if(SelVerticies[i].z == SelVerticies[0].z) {
			NumSel++;
		}
	}

	return NumSel;

/*
	SLONG StartX;
	SLONG StartZ;
	SLONG EndX;
	SLONG EndZ;
	DWORD i,j;
	SLONG x,z;
	CMapSector	*CurSector=m_MapSectors;

	StartX = (SLONG)((CameraPosition.x / m_TileWidth) + m_MapWidth/2) / m_TilesPerSectorRow;	//TILESPERSECTORROW;
	StartZ = (SLONG)((-CameraPosition.z / m_TileHeight) + m_MapHeight/2) / m_TilesPerSectorColumn;	//TILESPERSECTORROW;
	EndX = StartX + m_DrawRadius;
	EndZ = StartZ + m_DrawRadius;
	StartX-=m_DrawRadius;
	StartZ-=m_DrawRadius;

	CTile	*Tile=m_MapTiles;

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);

// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	D3DVERTEX Vertices[4];
	D3DVERTEX TVertices[4];
	D3DHVERTEX HVertex[4];
	BOOL	AllClipped;

	DWORD	TMap = 1;
	int	Selected = 0;
	int dx,dy;
	for(z=StartZ; z<EndZ; z++) {

		if((z>=0) && (z<m_SectorsPerColumn)) {
			CurSector=&m_MapSectors[z*m_SectorsPerRow+StartX];
			
			for(x=StartX; x<EndX; x++) {
				if((x>=0) && (x<m_SectorsPerRow)) {
// Is the sector visible.
					if(BoundsCheck(CurSector)) {
					
						for(i=0; i<CurSector->TileCount; i++) {
// Transform tile verticies.
							HeightToV0(m_MapTiles[CurSector->TileIndices[i]].Height[0],&m_MapTiles[CurSector->TileIndices[i]].Position,(D3DVECTOR*)&Vertices[0]);
							HeightToV1(m_MapTiles[CurSector->TileIndices[i]].Height[1],&m_MapTiles[CurSector->TileIndices[i]].Position,(D3DVECTOR*)&Vertices[1]);
							HeightToV2(m_MapTiles[CurSector->TileIndices[i]].Height[2],&m_MapTiles[CurSector->TileIndices[i]].Position,(D3DVECTOR*)&Vertices[2]);
							HeightToV3(m_MapTiles[CurSector->TileIndices[i]].Height[3],&m_MapTiles[CurSector->TileIndices[i]].Position,(D3DVECTOR*)&Vertices[3]);

							AllClipped = !m_DirectMaths->TransformVertex(TVertices, Vertices,4,HVertex);
							if(!AllClipped) {
   	 							for(j=0; j<4; j++) {
   									if(HVertex[j].dwFlags == 0) {
										dx = abs((int)TVertices[j].x - (int)XPos);
										dy = abs((int)TVertices[j].y - (int)YPos);
//										DebugPrint("%d,%d %d,%d : %d,%d\n",(int)TVertices[j].x,(int)TVertices[j].y,(int)XPos,(int)YPos,dx,dy);
										if((dx <= GRABRADIUS) && (dy <= GRABRADIUS)) {
											ASSERT(Selected < MAXSELECTEDVERTS);
											SelVerticies[Selected].TileIndex = CurSector->TileIndices[i];
											SelVerticies[Selected].VertIndex = j;
											SelVerticies[Selected].z = TVertices[j].z;
											Selected++;
										}
   									}
   								}
							}
						}
					}
				}

				CurSector++;
			}
		}
	}

	qsort((void*)SelVerticies,Selected,sizeof(SelVertex),CompareVerticies);

	int NumSel = 1;

	for(i=1; i<(DWORD)Selected; i++) {
		if(SelVerticies[i].z == SelVerticies[0].z) {
			NumSel++;
		}
	}

	return NumSel;
*/
}


int CompareVerticies(const void *arg1,const void * arg2)
{
	if( ((SelVertex*)arg1)->z == ((SelVertex*)arg2)->z) return 0;
	if( ((SelVertex*)arg1)->z < ((SelVertex*)arg2)->z) return -1;
	return 1;
}

							
int	CHeightMap::SelectFace(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
							DWORD XPos,DWORD YPos)
{
	SLONG StartX;
	SLONG StartZ;
	SLONG EndX;
	SLONG EndZ;
	SLONG x,z;

	StartX = (SLONG)((CameraPosition.x / m_TileWidth) + m_MapWidth/2);
	StartZ = (SLONG)((-CameraPosition.z / m_TileHeight) + m_MapHeight/2);
	EndX = StartX + m_DrawRadius;
	EndZ = StartZ + m_DrawRadius;
	StartX-=m_DrawRadius;
	StartZ-=m_DrawRadius;

	if(StartX < 0) {
		StartX = 0;
	}
	if(StartZ < 0) {
		StartZ = 0;
	}
	if(EndX > m_MapWidth) {	//-1) {
		EndX = m_MapWidth;	//-1;
	}
	if(EndZ > m_MapHeight) {	//-1) {
		EndZ = m_MapHeight;	//-1;
	}
	if( (StartX >= EndX) || (StartZ >= EndZ) ) {
		return -1;
	}

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	CTile	*Tile;

	D3DVERTEX Vertices[3];
	D3DVERTEX TVertices[3];
	D3DHVERTEX HVertex[3];
	D3DVECTOR Average;
	BOOL	AllClipped;
	BOOL	SomeClipped;
	DWORD j;

	DWORD	TMap = 1;
	int	Selected = -1;
	float SmallestZ;


	for(z=StartZ; z<EndZ; z++) {
		Tile = &m_MapTiles[z*m_MapWidth + StartX];
		for(x=StartX; x<EndX; x++) {
			HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertices[0]);
			HeightToV1(Tile->Height[1],&Tile->Position,(D3DVECTOR*)&Vertices[1]);
			HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertices[2]);

			AllClipped = !m_DirectMaths->TransformVertex(TVertices, Vertices,3,HVertex);
			SomeClipped = FALSE;
			for(j=0; j<3; j++) {
				if(HVertex[j].dwFlags) {
					SomeClipped = TRUE;
					break;
				}
			}
			if((!AllClipped) && (!SomeClipped)) {
				if( m_DirectMaths->PointInFace((float)XPos,(float)YPos,(D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[1].x),(D3DVECTOR*)(&TVertices[2].x)) ) {
					m_DirectMaths->FindAverageVec((D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[0].x),&Average);
					if((Average.z < SmallestZ) || (Selected<0)) {
						SmallestZ = Average.z;
						Selected = z*m_MapWidth + x;
					}
				}
			}

			HeightToV0(Tile->Height[0],&Tile->Position,(D3DVECTOR*)&Vertices[0]);
			HeightToV2(Tile->Height[2],&Tile->Position,(D3DVECTOR*)&Vertices[1]);
			HeightToV3(Tile->Height[3],&Tile->Position,(D3DVECTOR*)&Vertices[2]);

			AllClipped = !m_DirectMaths->TransformVertex(TVertices, Vertices,3,HVertex);
			SomeClipped = FALSE;
			for(j=0; j<3; j++) {
				if(HVertex[j].dwFlags) {
					SomeClipped = TRUE;
					break;
				}
			}
			if((!AllClipped) && (!SomeClipped)) {
				if( m_DirectMaths->PointInFace((float)XPos,(float)YPos,(D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[1].x),(D3DVECTOR*)(&TVertices[2].x)) ) {
					m_DirectMaths->FindAverageVec((D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[0].x),(D3DVECTOR*)(&TVertices[0].x),&Average);
					if((Average.z < SmallestZ) || (Selected<0)) {
						SmallestZ = Average.z;
						Selected = z*m_MapWidth + x;
					}
				}
			}
			Tile++;
		}
	}

	return Selected;
}



void CHeightMap::SwitchTriDirection(DWORD TileNum)
{
}


BOOL CHeightMap::BoundsCheck(CMapSector *Sector)
{
	D3DVERTEX ResultBox[8];

	return(m_DirectMaths->TransformVertex(&ResultBox[0],&Sector->BoundingBox[0],8));
}


// Get height using tiles plane equation.
//
float CHeightMap::GetHeight(float x, float z)
{
	D3DVECTOR start;
	D3DVECTOR delta;
	float i,j,t;
	float y;

	x+=(m_TileWidth*m_MapWidth/2);
	z+=(m_TileHeight*m_MapHeight/2);

	if( (x < 0.0) || (x > (float)(m_MapWidth*m_TileWidth)) ) return 0.0F;
	if( (z < 0.0) || (z > (float)(m_MapHeight*m_TileHeight)) ) return 0.0F;
	
	SLONG	tx = (SLONG)(x / m_TileWidth);
	SLONG	tz = (SLONG)(z / m_TileHeight);
	CTile *Tile = GetTile(tz * m_MapWidth + tx);

	start.x = (float)(((SLONG)x)%m_TileWidth) - (float)(m_TileWidth/2);
	start.z = -((float)(((SLONG)z)%m_TileHeight) - (float)(m_TileHeight/2));
	start.y = -2000.0F;
	delta.x = delta.z = 0.0F;
	delta.y = 4000.0F;

	DWORD	Index;

	if(start.x - (-start.z) < 0) {
		Index = 1;
	} else {
		Index = 0;
	}

	D3DVECTOR *Normal = &Tile->FaceNormals[Index];
	float Offset = Tile->Offsets[Index];

//	DebugPrint("%d : %.1f,%.1f : %.4f,%.4f,%.4f,%.4f : ",Index,start.x,start.z,Normal->x,Normal->y,Normal->z,Offset);

	i = Normal->x * start.x +
		Normal->y * start.y +
		Normal->z * start.z + Offset;

	j = Normal->x * delta.x +
		Normal->y * delta.y +
		Normal->z * delta.z;

		if(j==0) {	// Check for and handle divide by 0.
			DebugPrint("Divide by 0 in GetHeight()\n");
			return 0.0F;
		} else {
			t = -i / j;
			if(t<0) {
				DebugPrint("Error in GetHeight()\n");
				return 0.0F;
			} else {
				y = (delta.y * t) + start.y;
			}
		}

	return y;
}


// Get height using interpolation.
//
float CHeightMap::GetInterpolatedHeight(float xPos,float yPos)
{
	UDWORD	retVal;
	UDWORD tileX, tileY, tileYOffset;
	SDWORD h0, hx, hy, hxy;
	SDWORD dx, dy, ox, oy;
	UDWORD x = (UDWORD)(xPos+(m_MapWidth*m_TileWidth/2));
	UDWORD y = (UDWORD)(yPos+(m_MapHeight*m_TileHeight/2));


	if(x >= m_MapWidth*m_TileWidth) {
		return 0.0F;
	}
	if(y >= m_MapHeight*m_TileHeight) {
		return 0.0F;
	}

	tileX = x / m_TileWidth;
	tileY = y / m_TileHeight;
	
	tileYOffset = (tileY * m_MapWidth);

	ox = (SDWORD)x - (SDWORD)(tileX * m_TileWidth);
	oy = (SDWORD)y - (SDWORD)(tileY * m_TileHeight);

//	ASSERT((ox < TILE_UNITS, "mapHeight: x offset too big"));
//	ASSERT((oy < TILE_UNITS, "mapHeight: y offset too big"));
//	ASSERT((ox >= 0, "mapHeight: x offset too small"));
//	ASSERT((oy >= 0, "mapHeight: y offset too small"));

	//different code for 4 different triangle cases
	if (m_MapTiles[tileX + tileYOffset].Flags & TF_VERTEXFLIP)
	{
		if ((ox + oy) > m_TileWidth)//tile split top right to bottom left object if in bottom right half
		{
			ox = m_TileWidth - ox;
			oy = m_TileHeight - oy;
			hy = m_MapTiles[tileX + tileYOffset + m_MapWidth].Height[0];
			hx = m_MapTiles[tileX + 1 + tileYOffset].Height[0];
			hxy= m_MapTiles[tileX + 1 + tileYOffset + m_MapWidth].Height[0];

			dx = ((hy - hxy) * ox )/ m_TileWidth;
			dy = ((hx - hxy) * oy )/ m_TileHeight;

			retVal = (UDWORD)(((hxy + dx + dy)) * m_HeightScale);
			return (float)(retVal);
		}
		else //tile split top right to bottom left object if in top left half
		{
			h0 = m_MapTiles[tileX + tileYOffset].Height[0];
			hy = m_MapTiles[tileX + tileYOffset + m_MapWidth].Height[0];
			hx = m_MapTiles[tileX + 1 + tileYOffset].Height[0];

			dx = ((hx - h0) * ox )/ m_TileWidth;
			dy = ((hy - h0) * oy )/ m_TileHeight;

			retVal = (UDWORD)((h0 + dx + dy) * m_HeightScale);
			return (float)(retVal);
		}
	}
	else
	{
		if (ox > oy) //tile split topleft to bottom right object if in top right half
		{
			h0 = m_MapTiles[tileX + tileYOffset].Height[0];
			hx = m_MapTiles[tileX + 1 + tileYOffset].Height[0];
			hxy= m_MapTiles[tileX + 1 + tileYOffset + m_MapWidth].Height[0];

			dx = ((hx - h0) * ox )/ m_TileWidth;
			dy = ((hxy - hx) * oy )/ m_TileHeight;
			retVal = (UDWORD)(((h0 + dx + dy)) * m_HeightScale);
			return (float)(retVal);
		}
		else //tile split topleft to bottom right object if in bottom left half
		{
			h0 = m_MapTiles[tileX + tileYOffset].Height[0];
			hy = m_MapTiles[tileX + tileYOffset + m_MapWidth].Height[0];
			hxy = m_MapTiles[tileX + 1 + tileYOffset + m_MapWidth].Height[0];

			dx = ((hxy - hy) * ox )/ m_TileWidth;
			dy = ((hy - h0) * oy )/ m_TileHeight;

			retVal = (UDWORD)((h0 + dx + dy) * m_HeightScale);
			return (float)(retVal);
		}
	}
	return 0.0F;
}



#define iV_IMD_FLAT			0x00000100
#define iV_IMD_TEX			0x00000200
#define iV_IMD_WIRE			0x00000400
#define iV_IMD_TRANS			0x00000800
#define iV_IMD_GOUR			0x00001000
#define iV_IMD_NOCUL			0x00002000
#define iV_IMD_TEXANIM		0x00004000	// iV_IMD_TEX must be set also
#define iV_IMD_PSXTEX		0x00008000	// - use playstation texture allocation method
#define iV_IMD_BSPFRESH		0x00010000	// Freshly created by the BSP 
#define iV_IMD_NOHALFPSXTEX 0x00020000
#define iV_IMD_ALPHA		0x00040000

//#define iV_IMD_XFLAT		0x00000100
//#define iV_IMD_XTEX			0x00000200
//#define iV_IMD_XWIRE		0x00000400
//#define iV_IMD_XTRANS		0x00000800
//#define iV_IMD_XGOUR		0x00001000
//#define iV_IMD_XNOCUL		0x00002000
//#define iV_IMD_XOUTLINE		0x00004000
//#define iV_IMD_XFIXVIEW		0x00008000

// Simple safe divide. Divides b by a, returns b if a is zero.
//
_inline float SafeDivide(float a, float b)
{
	if(a == 0) return b;
	return b/a;
}


BOOL CHeightMap::ReadFeatureStats(const char *ScriptFile, const char* IMDDir, const char* TextDir)
{
	BOOL Flanged = TRUE;
   	BOOL TileSnap = TRUE;
   	int ColourIndex = -1;
   	NORMALTYPE ShadeMode = NT_DEFAULTNORMALS;
	BOOL Ok = TRUE;

	std::ifstream file(ScriptFile, std::ios_base::binary);
	if(!file.is_open())
		return FALSE;

	fileParser Parser(file, FP_SKIPCOMMENTS);
	Parser.SetBreakCharacters("=,\n\r\t");
	int NumFeatures = Parser.CountTokens()/FEATURE_STATS_SIZE;

	m_NumFeatures = NumFeatures;

	if(NumFeatures) {
		if(m_Features) delete m_Features;
		m_Features = new _feature_stats[NumFeatures];

//		sscanf(pFeatureData,"%[^','],%d,%d,%d,%d,%d,%[^','],%[^','],%d,%d,%d",
//			&featureName, &psFeature->baseWidth, &psFeature->baseBreadth,
//			&psFeature->damageable, &psFeature->armour, &psFeature->body, 
//			&GfxFile, &type, &psFeature->tileDraw, &psFeature->allowLOS, 
//			&psFeature->visibleAtStart);

		for(int j=0; j<NumFeatures; j++) {
   			Parser.Parse(m_Features[j].featureName,sizeof(m_Features[j].featureName));
   			Parser.ParseInt(NULL,(int*)&m_Features[j].baseWidth);
   			Parser.ParseInt(NULL,(int*)&m_Features[j].baseBreadth);
   			Parser.ParseInt(NULL,(int*)&m_Features[j].damageable);
   			Parser.ParseInt(NULL,(int*)&m_Features[j].armour);
   			Parser.ParseInt(NULL,(int*)&m_Features[j].body);
   			Parser.Parse(m_Features[j].IMDName,sizeof(m_Features[j].IMDName));
   			Parser.Parse(m_Features[j].type,sizeof(m_Features[j].type));
   			Parser.ParseInt(NULL,(int*)&m_Features[j].tileDraw); 
   			Parser.ParseInt(NULL,(int*)&m_Features[j].allowLOS);
   			Parser.ParseInt(NULL,(int*)&m_Features[j].visibleAtStart); 

//			DebugPrint("%s\n",m_Features[j].featureName);

			const string Name = g_WorkDirectory + "\\" + string(IMDDir) + string(m_Features[j].IMDName);
			if(!ReadIMD(Name.c_str(), m_Features[j].featureName, TextDir, IMD_FEATURE, Flanged, TileSnap, ColourIndex, ShadeMode, j)) {
				char String[256];
				sprintf(String,"Feature : %s\nObject : %s",m_Features[j].featureName,Name);
				MessageBox(NULL,String,"Error reading file",MB_OK);
				Ok = FALSE;
			}
		}
	}

	return Ok;
}

		
BOOL CHeightMap::ReadStructureStats(const char* ScriptFile,char *IMDDir,char *TextDir)
{
	BOOL Flanged = TRUE;
   	BOOL TileSnap = TRUE;
   	int ColourIndex = -1;
   	NORMALTYPE ShadeMode = NT_DEFAULTNORMALS;
	C3DObject BasePlate;
	BOOL Ok = TRUE;
	char techLevel[MAX_NAME_SIZE];
	char strength[MAX_NAME_SIZE];

	std::ifstream file(ScriptFile, std::ios_base::binary);
	if(!file.is_open())
		return FALSE;

	fileParser Parser(file, FP_SKIPCOMMENTS);
	Parser.SetBreakCharacters("=,\n\r\t");
	int NumStructs = Parser.CountTokens()/STRUCTURE_STATS_SIZE;

	m_NumStructures = NumStructs;

	if(NumStructs) {
		if(m_Structures) delete m_Structures;
		m_Structures = new _structure_stats[NumStructs];

////		sscanf(pStructData,"%[^','],%[^','],%d,%d,%d,%[^','],%d,%d,%d,%d,%d,%d,%d,%d,%d,\
////			%d,%[^','],%[^','],%d,%[^','],%[^','],%d,%d",
////			&StructureName, &type, &psStructure->terrainType, 
////			&psStructure->baseWidth, &psStructure->baseBreadth, &foundation, 
////			&psStructure->buildPoints, &psStructure->height, 
////			&psStructure->armourValue, &psStructure->bodyPoints, 
////			&psStructure->repairSystem, &psStructure->powerToBuild, 
////			&psStructure->minimumPower, &psStructure->resistance, 
////			&psStructure->quantityLimit, &psStructure->sizeModifier, 
////			&ecmType, &sensorType, &psStructure->weaponSlots, &GfxFile,
////			&baseIMD, &psStructure->numFuncs, &psStructure->numWeaps);

//		sscanf(pStructData,"%[^','],%[^','],%[^','],%d,%d,%d,%[^','],%d,%d,%d,%d,\
//			%d,%d,%d,%d,%d,%d,%[^','],%[^','],%d,%[^','],%[^','],%d,%d",
//			&StructureName, &type, &techLevel, &psStructure->terrainType, 
//			&psStructure->baseWidth, &psStructure->baseBreadth, &foundation, 
//			&psStructure->buildPoints, &psStructure->height, 
//			&psStructure->armourValue, &psStructure->bodyPoints, 
//			&psStructure->repairSystem, &psStructure->powerToBuild, 
//			&psStructure->minimumPower, &psStructure->resistance, 
//			&psStructure->quantityLimit, &psStructure->sizeModifier, 
//			&ecmType, &sensorType, &psStructure->weaponSlots, &GfxFile,
//			&baseIMD, &psStructure->numFuncs, &psStructure->numWeaps);


//		sscanf(pStructData,"%[^','],%[^','],%[^','],%[^','],%d,%d,%d,%[^','],\
//			%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%[^','],%[^','],%d,%[^','],%[^','],\
//			%d,%d",
//			&StructureName, &type, &techLevel, &strength, &psStructure->terrainType, 
//			&psStructure->baseWidth, &psStructure->baseBreadth, &foundation, 
//			&psStructure->buildPoints, &psStructure->height, 
//			&psStructure->armourValue, &psStructure->bodyPoints, 
//			&psStructure->repairSystem, &psStructure->powerToBuild, 
//			&psStructure->minimumPower, &psStructure->resistance, 
//			&psStructure->quantityLimit, &psStructure->sizeModifier, 
//			&ecmType, &sensorType, &psStructure->weaponSlots, &GfxFile,
//			&baseIMD, &psStructure->numFuncs, &psStructure->numWeaps);

		for(int j=0; j<NumStructs; j++) {
			Parser.Parse(m_Structures[j].StructureName,sizeof(m_Structures[j].StructureName));
			Parser.Parse(m_Structures[j].Type,sizeof(m_Structures[j].Type));
			Parser.Parse(techLevel,sizeof(techLevel));
			m_Structures[j].techLevel = SetTechLevel(techLevel);
			Parser.Parse(strength,sizeof(strength));
//			DebugPrint("%s,%s\n",techLevel,strength);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].terrainType);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].baseWidth);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].baseBreadth);
			Parser.Parse(m_Structures[j].Foundation,sizeof(m_Structures[j].Foundation));
			Parser.ParseInt(NULL,(int*)&m_Structures[j].buildPoints);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].height); 
			Parser.ParseInt(NULL,(int*)&m_Structures[j].armourValue);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].bodyPoints); 
			Parser.ParseInt(NULL,(int*)&m_Structures[j].repairSystem);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].powerToBuild); 
			Parser.ParseInt(NULL,(int*)&m_Structures[j].minimumPower);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].resistance);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].quantityLimit);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].sizeModifier);
			Parser.Parse(m_Structures[j].ecmType,sizeof(m_Structures[j].ecmType));
			Parser.Parse(m_Structures[j].sensorType,sizeof(m_Structures[j].sensorType));
			Parser.ParseInt(NULL,(int*)&m_Structures[j].weaponSlots);

// Read 8 IMD's , player 0 - 7.

			int ObjectID = m_Num3DObjects;

			for(int i=0; i<1; i++) {
				Parser.Parse(m_Structures[j].IMDName[i],sizeof(m_Structures[j].IMDName[i]));
//				DebugPrint("%s\n",m_Structures[j].IMDName[i]);

				const string Name = g_WorkDirectory + "\\" + string(IMDDir) + string(m_Structures[j].IMDName[i]);
				if(!ReadIMD(Name.c_str(), m_Structures[j].StructureName, TextDir, IMD_STRUCTURE, Flanged, TileSnap, ColourIndex, ShadeMode, j, i))
				{
					char String[256];
					sprintf(String,"Structure : %s\nObject : %s",m_Structures[j].StructureName,Name);
					MessageBox(NULL,String,"Error reading file",MB_OK);
					Ok = FALSE;
				}
			}


// Read the IMD for the base plate.
			Parser.Parse(m_Structures[j].BaseIMD,sizeof(m_Structures[j].BaseIMD));
//			DebugPrint("%s\n",m_Structures[j].BaseIMD);

			if(strlen(m_Structures[j].BaseIMD) > 1)
			{
				const string Name = g_WorkDirectory + "\\" + string(IMDDir) + string(m_Structures[j].BaseIMD);
				if (!ReadIMD(Name.c_str(), m_Structures[j].StructureName, TextDir, IMD_STRUCTURE,
				             Flanged, TileSnap, ColourIndex, ShadeMode, j, 0, &BasePlate))
				{
					char String[256];
					sprintf(String,"Structure : %s\nObject : %s",m_Structures[j].StructureName,Name);
					MessageBox(NULL,String,"Error reading file",MB_OK);
					Ok = FALSE;
				}

				for(i=0; i<1; i++) {
					m_3DObjects[ObjectID+i].AttachedObject = new C3DObject;
					memcpy(m_3DObjects[ObjectID+i].AttachedObject,&BasePlate,sizeof(C3DObject));
				}

			}

			Parser.ParseInt(NULL,(int*)&m_Structures[j].numFuncs);
			Parser.ParseInt(NULL,(int*)&m_Structures[j].numWeaps);
//			DebugPrint("%s\n",m_Structures[j].StructureName);
		}
	}

	return Ok;
}

/*sets the tech level for the stat passed in - returns TRUE if set OK*/
TECH_LEVEL CHeightMap::SetTechLevel(char *pLevel)
{
	TECH_LEVEL		techLevel = MAX_TECH_LEVELS;

	if (!strcmp(pLevel,"Level One (default)"))
	if (!strcmp(pLevel,"Level One"))
	{
		techLevel = TECH_LEVEL_ONE;
	}
	else if (!strcmp(pLevel,"Level Two"))
	{
		techLevel = TECH_LEVEL_TWO;
	}
	else if (!strcmp(pLevel,"Level Three"))
	{
		techLevel = TECH_LEVEL_THREE;
	}
	else if (!strcmp(pLevel,"Level One-Two"))
	{
		techLevel = TECH_LEVEL_ONE_TWO;
	}
	else if (!strcmp(pLevel,"Level Two-Three"))
	{
		techLevel = TECH_LEVEL_TWO_THREE;
	}
	else if (!strcmp(pLevel,"Level All"))
	{
		techLevel = TECH_LEVEL_ALL;
	}
	else if (!strcmp(pLevel,"Don't Display"))
	{
		techLevel = MAX_TECH_LEVELS;
	}
	//invalid tech level passed in
	else
	{
		assert(FALSE);
	}

	return techLevel;
}



BOOL CHeightMap::ReadTemplateStats(const char* ScriptFile, char* IMDDir, char* TextDir)
{
	char String[256];
	BOOL Ok = TRUE;

	std::ifstream file(ScriptFile, std::ios_base::binary);
	if(!file.is_open())
		return FALSE;

	fileParser Parser(file, FP_SKIPCOMMENTS);
	Parser.SetBreakCharacters("=,\n\r\t");
	int NumTemplates = Parser.CountTokens()/DROID_TEMPLATE_STATS_SIZE;

	m_NumTemplates = NumTemplates;

	delete m_Templates;
	m_Templates = new DroidTemplate[NumTemplates];

	if(m_Templates == NULL)
	{
		MessageBox(NULL,"Error","Unable to allocate template list.",MB_OK);
		return FALSE;
	}

	for (int i=0; i < NumTemplates; i++) {

		// Get the template name.
		Parser.Parse(m_Templates[i].Name,sizeof(m_Templates[i].Name));
//		DebugPrint("%s\n",m_Templates[i].Name);

		// Skip the rest.
		for(int j=0; j<DROID_TEMPLATE_STATS_SIZE-1; j++) {
			Parser.Parse(String,sizeof(String));
		}

//		if(i==102) {
//			DebugPrint("Bang\n");
//		}

		string Name = g_HomeDirectory + "\\Data\\Icon.PIE";
		if(!ReadIMD(Name.c_str(), m_Templates[i].Name, TextDir, IMD_DROID, FALSE, FALSE, 0, NT_SMOOTHNORMALS, i))
		{
			char String[256];
			sprintf(String,"Template : %s\nObject : %s",Name,"Icon.PIE");
			MessageBox(NULL,String,"Error reading file",MB_OK);
			Ok = FALSE;
		}
	}

	return Ok;
}



BOOL CHeightMap::ReadIMDObjects(char *ScriptFile)
{
	std::ifstream file(ScriptFile, std::ios_base::binary);
	if(!file.is_open())
	{
		file.open(EditorDataFileName(ScriptFile).c_str(), std::ios_base::binary);
	}

	if(!file.is_open())
		return FALSE;

	delete m_FeatureSet;

	char	Drive[16];
	char	Dir[256];
	char	FName[256];
	char	Ext[16];
	char	FeatureSet[512];

	_splitpath(ScriptFile,Drive,Dir,FName,Ext);
	strcpy(FeatureSet,Dir);
	strcat(FeatureSet,FName);
	strcat(FeatureSet,Ext);
	m_FeatureSet = new char[strlen(FeatureSet)+1];
	strcpy(m_FeatureSet,FeatureSet);

	fileParser Parser(file, FP_SKIPCOMMENTS | FP_QUOTES | FP_LOWERCASE);

	if(!ReadMisc(Parser,"miscbegin","miscend")) {
		return FALSE;
	}
	if(!ReadFeatures(Parser,"featuresbegin","featuresend")) {
		return FALSE;
	}
	if(!ReadStructures(Parser,"structuresbegin","structuresend")) {
		return FALSE;
	}
	if(!ReadTemplates(Parser,"droidsbegin","droidsend")) {
		return FALSE;
	}
	if(!ReadObjects(Parser,"objectsbegin","objectsend",IMD_OBJECT)) {
		return FALSE;
	}

	return TRUE;
}


BOOL CHeightMap::ReadObjectNames(char *FileName)
{
	char String[256];
	char Name[256];

	std::ifstream file(FileName, std::ios_base::binary);
	if(!file.is_open())
		return FALSE;

	fileParser Parser(file, FP_SKIPCOMMENTS | FP_QUOTES);

	m_NumNames = 0;

	while(1) {
		Parser.Parse(String,sizeof(String));
		if(String[0] == 0) break;

		Parser.Parse(Name,sizeof(Name));
		if(Name[0] == 0) break;

		if(m_NumNames < MAX_OBJNAMES) {
			m_ObjNames[m_NumNames].IDString = new char[sizeof(String)+1];
			m_ObjNames[m_NumNames].Name = new char[sizeof(Name)+1];
			strcpy(m_ObjNames[m_NumNames].IDString,String);
			strcpy(m_ObjNames[m_NumNames].Name,Name);
			m_NumNames++;
		}

//		DebugPrint("%s : %s\n",String,Name);
	}

	return TRUE;
}


int CHeightMap::MatchObjName(char *IDString)
{
	for(int i=0; i<m_NumNames; i++) {
		if(strcmp(m_ObjNames[i].IDString,IDString) == 0) {
			return i;
		}
	}

	return -1;
}


BOOL CHeightMap::ReadMisc(fileParser& Parser,char *Begin,char *End)
{
	char String[256];
	char TextureName[256];
	DWORD NumObjects = 0;
	int ColourIndex = -1;

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,Begin) != 0) {
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("%s Not found!\n",Begin);
		return FALSE;
	}

	if(!Parser.ParseString("tiletextures",TextureName,sizeof(TextureName))) {
		DebugPrint("TILETEXTURES directive not found!\n");
		MessageBox(NULL,"TILETEXTURES directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	strcpy(m_TileTextureName,TextureName);

	if(!Parser.ParseString("objectnames",String,sizeof(String))) {
		DebugPrint("OBJECTNAMES directive not found!\n");
		MessageBox(NULL,"OBJECTNAMES directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	strcpy(m_ObjectNames,String);
	ReadObjectNames(m_ObjectNames);

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,"brushbegin") != 0) {
		MessageBox(NULL,"BRUSHBEGIN directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("BRUSHBEGIN Not found!\n");
		return FALSE;
	}

	for(int i=0; i<16; i++) {
		Parser.ParseInt(NULL,&m_BrushHeightMode[i]);
		Parser.ParseInt(NULL,&m_BrushHeight[i]);
		Parser.ParseInt(NULL,&m_BrushRandomRange[i]);
		for(int j=0; j<16; j++) {
			Parser.ParseInt(NULL,&m_BrushTiles[i][j]);
			Parser.ParseInt(NULL,&m_BrushFlags[i][j]);
		}
	}

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,"brushend") != 0) {
		MessageBox(NULL,"BRUSHEND directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("BRUSHEND Not found!\n");
		return FALSE;
	}

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,"typebegin") != 0) {
		MessageBox(NULL,"TYPEBEGIN directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("TYPEBEGIN Not found!\n");
		return FALSE;
	}

	for(i=0; i<128; i++) {
		Parser.ParseInt(NULL,&m_TileTypes[i]);
	}

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,"typeend") != 0) {
		MessageBox(NULL,"TYPEEND directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("TYPEEND Not found!\n");
		return FALSE;
	}

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,End) != 0) {
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("%s Not found!\n",End);
		return FALSE;
	}

	return TRUE;
}



BOOL CHeightMap::ReadFeatures(fileParser& Parser,char *Begin,char *End)
{
	char String[256];
	char StatsDir[256];
	char IMDDir[256];
	char TextDir[256];
	DWORD NumObjects = 0;
	int ColourIndex = -1;

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,Begin) != 0) {
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("%s Not found!\n",Begin);
		return FALSE;
	}

	if(!Parser.ParseString("statsdir",StatsDir,sizeof(StatsDir))) {
		DebugPrint("STATSDIR directive not found!\n");
		MessageBox(NULL,"STATSDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("imddir",IMDDir,sizeof(IMDDir))) {
		DebugPrint("IMDDIR directive not found!\n");
		MessageBox(NULL,"IMDDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("textdir",TextDir,sizeof(TextDir))) {
		DebugPrint("TEXTDIR directive not found!\n");
		MessageBox(NULL,"TEXTDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	while(1)
	{
		Parser.Parse(String,sizeof(String));
		if(strcmp(String,End) == 0) break;

		const string Name = g_WorkDirectory + "\\" + string(StatsDir) + string(String);
		if (!ReadFeatureStats(Name.c_str(), IMDDir, TextDir))
		{
			DebugPrint("Error loading IMD %s\n", Name.c_str());
			MessageBox(NULL, "Error reading IMD", Name.c_str(), MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}


BOOL CHeightMap::ReadStructures(fileParser& Parser,char *Begin,char *End)
{
	char String[256];
	char StatsDir[256];
	char IMDDir[256];
	char TextDir[256];
	DWORD NumObjects = 0;
	int ColourIndex = -1;

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,Begin) != 0) {
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("%s Not found!\n",Begin);
		return FALSE;
	}

	if(!Parser.ParseString("statsdir",StatsDir,sizeof(StatsDir))) {
		DebugPrint("STATSDIR directive not found!\n");
		MessageBox(NULL,"STATSDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("imddir",IMDDir,sizeof(IMDDir))) {
		DebugPrint("IMDDIR directive not found!\n");
		MessageBox(NULL,"IMDDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("textdir",TextDir,sizeof(TextDir))) {
		DebugPrint("TEXTDIR directive not found!\n");
		MessageBox(NULL,"TEXTDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	while(1) {
		Parser.Parse(String,sizeof(String));
		if(strcmp(String,End) == 0) break;

		const string Name = g_WorkDirectory + "\\" + string(StatsDir) + string(String);
		if (!ReadStructureStats(Name.c_str(), IMDDir, TextDir))
		{
			DebugPrint("Error loading IMD %s\n", Name.c_str());
			MessageBox(NULL,"Error reading IMD", Name.c_str(), MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}


BOOL CHeightMap::ReadTemplates(fileParser& Parser,char *Begin,char *End)
{
	char String[256];
	char StatsDir[256];
	char IMDDir[256];
	char TextDir[256];
	DWORD NumObjects = 0;
	int ColourIndex = -1;

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,Begin) != 0) {
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		DebugPrint("%s Not found!\n",Begin);
		return FALSE;
	}

	if(!Parser.ParseString("statsdir",StatsDir,sizeof(StatsDir))) {
		DebugPrint("STATSDIR directive not found!\n");
		MessageBox(NULL,"STATSDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("imddir",IMDDir,sizeof(IMDDir))) {
		DebugPrint("IMDDIR directive not found!\n");
		MessageBox(NULL,"IMDDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("textdir",TextDir,sizeof(TextDir))) {
		DebugPrint("TEXTDIR directive not found!\n");
		MessageBox(NULL,"TEXTDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	while(1) {
		Parser.Parse(String,sizeof(String));
		if(strcmp(String,End) == 0) break;

		const string Name = g_WorkDirectory + "\\" + string(StatsDir) + string(String);
		if(!ReadTemplateStats(Name.c_str(), IMDDir, TextDir))
		{
			DebugPrint("Error loading IMD %s\n", Name.c_str());
			MessageBox(NULL,"Error reading IMD", Name.c_str(), MB_OK);
			return FALSE;
		}
	}

	return TRUE;
}



BOOL CHeightMap::ReadObjects(fileParser& Parser,char *Begin,char *End,int TypeID)
{
	char String[256];
	char Description[256];
	char Type[256];
	char StatsDir[256];
	char IMDDir[256];
	char TextDir[256];
	DWORD NumObjects = 0;
	BOOL Flanged;
	BOOL TileSnap;
	int ColourIndex = -1;

	Parser.Parse(String,sizeof(String));
	if(strcmp(String,Begin) != 0) {
		DebugPrint("%s Not found!\n",Begin);
		MessageBox(NULL,"Section directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("statsdir",StatsDir,sizeof(StatsDir))) {
		DebugPrint("STATSDIR directive not found!\n");
		MessageBox(NULL,"STATSDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("imddir",IMDDir,sizeof(IMDDir))) {
		DebugPrint("IMDDIR directive not found!\n");
		MessageBox(NULL,"IMDDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	if(!Parser.ParseString("textdir",TextDir,sizeof(TextDir))) {
		DebugPrint("TEXTDIR directive not found!\n");
		MessageBox(NULL,"TEXTDIR directive not found!","Error parsing data set!",MB_OK);
		return FALSE;
	}

	while(1) {
		Parser.Parse(String,sizeof(String));
		if(strcmp(String,End) == 0) break;

		short Flags = Parser.GetFlags();
		Parser.SetFlags(Flags & ~(FP_LOWERCASE | FP_UPPERCASE));
		Parser.Parse(Description,sizeof(Description));
		Parser.SetFlags(Flags);
		
		Parser.Parse(Type,sizeof(Type));
		if(strcmp(Type,"flanged") == 0) {
			Flanged = TRUE;
		} else if(strcmp(Type,"overlayed") == 0) {
			Flanged = FALSE;
		} else {
			MessageBox(NULL,"Invalid flange type.\nShould be one of FLANGED,OVERLAYED","Error parsing data set!",MB_OK);
			return FALSE;
		}

		Parser.Parse(Type,sizeof(Type));
		if(strcmp(Type,"tilesnap") == 0) {
			TileSnap = TRUE;
		} else if(strcmp(Type,"nosnap") == 0) {
			TileSnap = FALSE;
		} else {
			MessageBox(NULL,"Invalid snap type.\nShould be one of TILESNAP,NOSNAP","Error parsing data set!",MB_OK);
			return FALSE;
		}

		Parser.Parse(Type,sizeof(Type));
		if(strncmp(Type,"key",3) == 0) {
			if(sscanf(&Type[3],"%d",&ColourIndex) != 1) {
				MessageBox(NULL,"Invalid key index.\nShould be number between 0-255","Error parsing data set!",MB_OK);
				return FALSE;
			}
		} else if(strcmp(Type,"nokey") == 0) {
			ColourIndex = -1;
		} else {
			MessageBox(NULL,"Invalid key type.\nShould be one of KEY,NOKEY","Error parsing data set!",MB_OK);
			return FALSE;
		}

		NORMALTYPE ShadeMode;

		Parser.Parse(Type,sizeof(Type));
		if(strcmp(Type,"smoothshade") == 0) {
			ShadeMode = NT_SMOOTHNORMALS;
		} else if(strcmp(Type,"flatshade") == 0) {
			ShadeMode = NT_FACENORMALS;
		} else if(strcmp(Type,"noshade") == 0) {
			ShadeMode = NT_DEFAULTNORMALS;
		} else {
			MessageBox(NULL,"Invalid shade type.\nShould be one of SMOOTHSHADE,FLATSHADE,NOSHADE","Error parsing data set!",MB_OK);
			return FALSE;
		}

		char *Desc = NULL;
		if(_stricmp(Description,"NONAME")) {
			Desc = Description;
		}

		const string Name = g_WorkDirectory + "\\" + string(IMDDir) + string(String);

		if(!ReadIMD(Name.c_str(), Desc, TextDir, TypeID, Flanged, TileSnap, ColourIndex, ShadeMode))
		{
			DebugPrint("Error loading IMD %s\n", Name.c_str());
			MessageBox(NULL,"Error reading IMD", Name.c_str(), MB_OK);
			return FALSE;
		}
		NumObjects++;
	}

	return TRUE;
}

	
// Read an IMD format 3d object.
//
// If DoNormals = TRUE then calculates normals for each face.
//
// Returns TRUE if successful.
//
BOOL CHeightMap::ReadIMD(const char* FileName,char *Description, const char* TextDir,int TypeID,BOOL Flanged,BOOL Snap,int ColourKeyIndex,NORMALTYPE NType,
						 int StructureIndex,int PlayerIndex,C3DObject *Object)
{
	char Buffer[80];
	SLONG Version;
	DWORD Flags;
	DWORD NumLevels;
	char Drive[256];
	char Dir[256];
	char FName[256];
	char Ext[256];
	char texType[256];
	BOOL NormalSmooth = OBJECTSMOOTHNORMALS;
	int i;
	char ch;
	C3DObject *DestObject;
	int Width,Height;

	_splitpath(FileName,Drive,Dir,FName,Ext);

	if(Object == NULL) {
		DestObject = &m_3DObjects[m_Num3DObjects];
		m_Num3DObjects++;
	} else {
		DestObject = Object;
	}

	DestObject->RealName = MatchObjName(Description);

	FILE *Stream = fopen(FileName,"rb");
	if(Stream == NULL) {
		return FALSE;
	}

// Read the type and version.
	fscanf(Stream,"%s %d",Buffer,&Version);

// Check it's an IMD or a PIE file.
	if( (strcmp("IMD",Buffer) !=0) && (strcmp("PIE",Buffer) !=0) ) {
		fclose(Stream);
		return FALSE;
	}

// Read the 1st section name
	fscanf(Stream,"%s %x",Buffer,&Flags);

// Textured?
	if(Flags & iV_IMD_TEX) {
		char texfile[80];
		int ptype, pwidth, pheight;

// Read in the texture name and parameters.
		fscanf(Stream,"%s %d",	Buffer,&ptype);
		
		if(Version == 1) {
			fscanf(Stream,"%s",&texfile);
		} else if(Version == 2) {
   			getc(Stream);

   			for( i = 0; (i < 80) &&  ((ch = getc(Stream)) != EOF) && (ch != '.'); i++ )
   			{
				texfile[i] = (char)ch;
   			}

   			if (fscanf(Stream,"%s", texType) != 1)
   			{
   				fclose(Stream);
   				return FALSE;
   			}

   			if (strcmp(texType,"pcx") != 0)
   			{
   				fclose(Stream);
   				return FALSE;
   			}

   			texfile[i] = 0;

			ASSERT( (strlen(texfile) + strlen(".pcx") + 2) < 80 );
			strcat(texfile,".pcx");

   			if (fscanf(Stream,"%d %d", &pwidth, &pheight) != 2)
   			{
   				fclose(Stream);
   				return FALSE;
   			}
		} else {
			return FALSE;
		}

		fscanf(Stream,"%d %d",&pwidth,&pheight);

		if(strcmp(Buffer,"TEXTURE") != 0) {
			return FALSE;
		}


		if( m_MatManager->FindMaterial(texfile,&DestObject->TextureID) == FALSE) {

			DebugPrint("## %s ## \n",texfile);

			string TName;

			if((TextDir) && (TextDir[0] != 0))
			{
				TName = g_WorkDirectory + "\\" + string(TextDir) + string(texfile);
			}
			else
			{
	// set the path for the texture to the path used for the .IMD
				TName = string(Drive) + string(Dir) + string(texfile);
			}
  
	// Read the specified bitmap.
			// HACK: using a pointer here to be able to reconstruct the std::ifstream
			//       by means of delete followed by new, this since apparently MSVC's
			//       std::ifstream implementation has trouble with a second attempt to
			//       open another file using the same instance of std::ifstream
			std::ifstream* PCXFile = new std::ifstream(TName.c_str(), std::ios_base::binary);
			if (!PCXFile->is_open())
			{
				TName = string(Drive) + string(Dir) + string(texfile);

				// Reconstruct std::ifstream object;
				// A simple PCXFile->close() followed by PCXFile->open(...) should work, but doesn't
				delete PCXFile;
				PCXFile = new std::ifstream(TName.c_str(), std::ios_base::binary);

				if (!PCXFile->is_open())
				{
					MessageBox(NULL, TName.c_str(), "Unable to open file.", MB_OK);
					fclose(Stream);
					delete PCXFile;
					return FALSE;
				}
			}

			PCXHandler* PCXTexBitmap = new PCXHandler;
			if(!PCXTexBitmap->ReadPCX(*PCXFile, BMR_ROUNDUP))
			{
				delete PCXTexBitmap;
				fclose(Stream);
				delete PCXFile;
				return FALSE;
			}
			delete PCXFile;

	// And create a D3D material from it.
			Width = PCXTexBitmap->GetBitmapWidth();
			Height = PCXTexBitmap->GetBitmapHeight();
	//		D3DCOLORVALUE	Diffuse;
			MatDesc Desc;

			memset(&Desc,0,sizeof(Desc));
			Desc.Type = MT_TEXTURED;
			Desc.TexFormat = TF_PAL8BIT;
	//		Desc.AlphaTypes = AT_COLOURKEY;	// | AT_ALPHABLEND;
			Desc.ColourKey = 0x00ff00;
			Desc.Bits = (void*)PCXTexBitmap->GetBitmapBits();
			Desc.Name = texfile;
			Desc.Width = Width;
			Desc.Height = Height;
			Desc.PaletteEntries = PCXTexBitmap->GetBitmapPaletteEntries();
			Desc.Alpha = 127;
			Desc.Diffuse.r = ((float)MIDCOLOUR)/((float)256);
			Desc.Diffuse.g = ((float)MIDCOLOUR)/((float)256);
			Desc.Diffuse.b = ((float)MIDCOLOUR)/((float)256);
			Desc.Diffuse.a = 0.0F;	//1.0F;
			DestObject->TextureID = m_MatManager->CreateMaterial(&Desc);

			delete PCXTexBitmap;
		} else {
			Width = m_MatManager->GetMaterialWidth(DestObject->TextureID);
			Height = m_MatManager->GetMaterialHeight(DestObject->TextureID);
		}

	} 
// Get the number of levels. Not sure what this is used for yet ( detail levels or animation mabey? )
	fscanf(Stream,"%s %d",Buffer,&NumLevels);

	if(strcmp(Buffer,"LEVELS") != 0) {
		return FALSE;
	}

// Get the level number for this data set.
	DWORD Level;
	fscanf(Stream,"%s %d",Buffer,&Level);

	int NumPoints;	// Number of points in the poly.
	int NumPolys;	// Number of polygons in the model.
	int NumTris = 0; // Number of triangles in the model.
	int NumVerts;	// Number of verticies in the model.

// Number of points in the object.
//	DWORD Number;
	fscanf(Stream,"%s %d",Buffer,&NumVerts);
	if(strcmp(Buffer,"POINTS") != 0) {
		return FALSE;
	}

	D3DVECTOR Smallest;
	D3DVECTOR Largest;
	InitSmallestLargest(Smallest,Largest);

// Read each point.
	D3DVECTOR *Point = new D3DVECTOR[NumVerts];			// Model verticies.
	D3DVECTOR *PointNormals = new D3DVECTOR[NumVerts];	// Array of summed normals.
	memset(PointNormals,0,NumVerts*sizeof(D3DVECTOR));

	for(i=0; i<NumVerts; i++) {
		int x,y,z;
		fscanf(Stream,"%d %d %d",&x,&y,&z);
		Point[i].x = (float)x;
		Point[i].y = (float)y;
		Point[i].z = (float)z;
		UpdateSmallest(Point[i],Smallest);
		UpdateLargest(Point[i],Largest);
	}

// Number of polygons in the object.
	fscanf(Stream,"%s %d",Buffer,&NumPolys);
	if(strcmp(Buffer,"POLYGONS") != 0) {
		fclose(Stream);
		return FALSE;
	}

	DWORD PolyFlags;
	int i0,i1,i2,i3;
	int u,v;
	CTriangle *Triangle = new CTriangle[1024];
	D3DVECTOR DefNormal = {0.0F,1.0F,0.0F};		// Default normal value.
	D3DVECTOR Normal = {0.0F,1.0F,0.0F};		// Default normal value.
	int nFrames;
	int pbRate;
	int tWidth;
	int tHeight;

// Read all the polygons into a temporary array of triangles. 4 sided ones are converted to
// 2 triangles, fill in the normal and texture values.

	fpos_t FilePos;
	fgetpos(Stream,&FilePos);

	for(i=0; i<NumPolys; i++) {
		fscanf(Stream,"%x %d",&PolyFlags,&NumPoints);

		if(PolyFlags & iV_IMD_TRANS) {
			ColourKeyIndex=0;
		}

		switch(NumPoints) {
			case 3:
				Triangle[NumTris].Flags = PolyFlags;
				fscanf(Stream,"%d %d %d",&i0,&i1,&i2);

				assert(i0 < NumVerts);
				assert(i1 < NumVerts);
				assert(i2 < NumVerts);

				// Just skip the texture anim stuff for now.
				if(PolyFlags & iV_IMD_TEXANIM) {
					fscanf(Stream,"%d %d %d %d",
							&nFrames,
							&pbRate,
							&tWidth,
							&tHeight);
					Triangle[NumTris].TAFrames = nFrames;
					Triangle[NumTris].TARate = pbRate;
					Triangle[NumTris].TAWidth = tWidth;
					Triangle[NumTris].TAHeight = tHeight;
				}

				Triangle[NumTris].v0.x = Point[i0].x;
				Triangle[NumTris].v0.y = Point[i0].y;
				Triangle[NumTris].v0.z = Point[i0].z;
				Triangle[NumTris].v1.x = Point[i1].x;
				Triangle[NumTris].v1.y = Point[i1].y;
				Triangle[NumTris].v1.z = Point[i1].z;
				Triangle[NumTris].v2.x = Point[i2].x;
				Triangle[NumTris].v2.y = Point[i2].y;
				Triangle[NumTris].v2.z = Point[i2].z;
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v0.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v0.tv = SafeDivide((float)Height,(float)v);
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v1.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v1.tv = SafeDivide((float)Height,(float)v);
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v2.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v2.tv = SafeDivide((float)Height,(float)v);

				if(NType == NT_SMOOTHNORMALS) {
					m_DirectMaths->CalcNormal(&Point[i0],&Point[i1],&Point[i2],&Normal);
					if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
						DebugPrint("Tri  Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i1,i2);
						DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
									Point[i0].x,Point[i0].y,Point[i0].z,
									Point[i1].x,Point[i1].y,Point[i1].z,
									Point[i2].x,Point[i2].y,Point[i2].z);

						Normal = DefNormal;
					}
					PointNormals[i0].x += Normal.x;	PointNormals[i0].y += Normal.y;	PointNormals[i0].z += Normal.z;
					PointNormals[i1].x += Normal.x;	PointNormals[i1].y += Normal.y;	PointNormals[i1].z += Normal.z;
					PointNormals[i2].x += Normal.x;	PointNormals[i2].y += Normal.y;	PointNormals[i2].z += Normal.z;
				} else {
					if(NType == NT_FACENORMALS) {
						m_DirectMaths->CalcNormal(&Point[i0],&Point[i1],&Point[i2],&Normal);
						if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
							DebugPrint("Tri  Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i1,i2);
							DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
										Point[i0].x,Point[i0].y,Point[i0].z,
										Point[i1].x,Point[i1].y,Point[i1].z,
										Point[i2].x,Point[i2].y,Point[i2].z);

							Normal = DefNormal;
						}
					}
					*( (D3DVECTOR*)&Triangle[NumTris].v0.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris].v1.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris].v2.nx ) = Normal;
				}

				NumTris ++;
				break;
			case 4:
				Triangle[NumTris].Flags = PolyFlags;
				Triangle[NumTris+1].Flags = PolyFlags;
				fscanf(Stream,"%d %d %d %d",&i0,&i1,&i2,&i3);

				assert(i0 < NumVerts);
				assert(i1 < NumVerts);
				assert(i2 < NumVerts);
				assert(i3 < NumVerts);

				// Just skip the texture anim stuff for now.
				if(PolyFlags & iV_IMD_TEXANIM) {
					fscanf(Stream,"%d %d %d %d",
							&nFrames,
							&pbRate,
							&tWidth,
							&tHeight);
					Triangle[NumTris].TAFrames = nFrames;
					Triangle[NumTris].TARate = pbRate;
					Triangle[NumTris].TAWidth = tWidth;
					Triangle[NumTris].TAHeight = tHeight;
					Triangle[NumTris+1].TAFrames = nFrames;
					Triangle[NumTris+1].TARate = pbRate;
					Triangle[NumTris+1].TAWidth = tWidth;
					Triangle[NumTris+1].TAHeight = tHeight;
				}

				Triangle[NumTris].v0.x = Point[i0].x;
				Triangle[NumTris].v0.y = Point[i0].y;
				Triangle[NumTris].v0.z = Point[i0].z;
				Triangle[NumTris].v1.x = Point[i1].x;
				Triangle[NumTris].v1.y = Point[i1].y;
				Triangle[NumTris].v1.z = Point[i1].z;
				Triangle[NumTris].v2.x = Point[i2].x;
				Triangle[NumTris].v2.y = Point[i2].y;
				Triangle[NumTris].v2.z = Point[i2].z;

 				Triangle[NumTris+1].v0.x = Point[i0].x;
				Triangle[NumTris+1].v0.y = Point[i0].y;
				Triangle[NumTris+1].v0.z = Point[i0].z;
				Triangle[NumTris+1].v1.x = Point[i2].x;
				Triangle[NumTris+1].v1.y = Point[i2].y;
				Triangle[NumTris+1].v1.z = Point[i2].z;
				Triangle[NumTris+1].v2.x = Point[i3].x;
				Triangle[NumTris+1].v2.y = Point[i3].y;
				Triangle[NumTris+1].v2.z = Point[i3].z;
				
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v0.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v0.tv = SafeDivide((float)Height,(float)v);
				Triangle[NumTris+1].v0.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris+1].v0.tv = SafeDivide((float)Height,(float)v);
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v1.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v1.tv = SafeDivide((float)Height,(float)v);
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris].v2.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris].v2.tv = SafeDivide((float)Height,(float)v);
				Triangle[NumTris+1].v1.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris+1].v1.tv = SafeDivide((float)Height,(float)v);
				fscanf(Stream,"%d %d",&u,&v);
				Triangle[NumTris+1].v2.tu = SafeDivide((float)Width,(float)u);
				Triangle[NumTris+1].v2.tv = SafeDivide((float)Height,(float)v);

				if(NType == NT_SMOOTHNORMALS) {
					m_DirectMaths->CalcNormal(&Point[i0],&Point[i1],&Point[i2],&Normal);
					if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
//						DebugPrint("Quad Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i1,i2);
//						DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
//									Point[i0].x,Point[i0].y,Point[i0].z,
//									Point[i1].x,Point[i1].y,Point[i1].z,
//									Point[i2].x,Point[i2].y,Point[i2].z);

						Normal = DefNormal;
					}

					PointNormals[i0].x += Normal.x;	PointNormals[i0].y += Normal.y;	PointNormals[i0].z += Normal.z;
					PointNormals[i1].x += Normal.x;	PointNormals[i1].y += Normal.y;	PointNormals[i1].z += Normal.z;
					PointNormals[i2].x += Normal.x;	PointNormals[i2].y += Normal.y;	PointNormals[i2].z += Normal.z;

					m_DirectMaths->CalcNormal(&Point[i0],&Point[i2],&Point[i3],&Normal);
					if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
//						DebugPrint("Quad Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i2,i3);
//						DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
//									Point[i0].x,Point[i0].y,Point[i0].z,
//									Point[i2].x,Point[i2].y,Point[i2].z,
//									Point[i3].x,Point[i3].y,Point[i3].z);

						Normal = DefNormal;
					}

					PointNormals[i0].x += Normal.x;	PointNormals[i0].y += Normal.y;	PointNormals[i0].z += Normal.z;
					PointNormals[i2].x += Normal.x;	PointNormals[i2].y += Normal.y;	PointNormals[i2].z += Normal.z;
					PointNormals[i3].x += Normal.x;	PointNormals[i3].y += Normal.y;	PointNormals[i3].z += Normal.z;

				} else {
					if(NType == NT_FACENORMALS) {
						m_DirectMaths->CalcNormal(&Point[i0],&Point[i1],&Point[i2],&Normal);
						if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
//							DebugPrint("Quad Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i1,i2);
//							DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
//										Point[i0].x,Point[i0].y,Point[i0].z,
//										Point[i1].x,Point[i1].y,Point[i1].z,
//										Point[i2].x,Point[i2].y,Point[i2].z);

							Normal = DefNormal;
						}
					}
					*( (D3DVECTOR*)&Triangle[NumTris].v0.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris].v1.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris].v2.nx ) = Normal;

					if(NType == NT_FACENORMALS) {
						m_DirectMaths->CalcNormal(&Point[i0],&Point[i2],&Point[i3],&Normal);
						if((Normal.x == 0.0F) && (Normal.y == 0.0F) && (Normal.z == 0.0F)) {
//							DebugPrint("Quad Normal Error %s : F%d : I%d,I%d,I%d : ",FName,i,i0,i1,i2);
//							DebugPrint("(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)\n",
//										Point[i0].x,Point[i0].y,Point[i0].z,
//										Point[i1].x,Point[i1].y,Point[i1].z,
//										Point[i2].x,Point[i2].y,Point[i2].z);

							Normal = DefNormal;
						}
					}
					*( (D3DVECTOR*)&Triangle[NumTris+1].v0.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris+1].v1.nx ) = Normal;
					*( (D3DVECTOR*)&Triangle[NumTris+1].v2.nx ) = Normal;
				}
				
				NumTris += 2;
				break;
		}
	}

	if(NType == NT_SMOOTHNORMALS) {
		for(i=0; i<NumVerts; i++) {
			m_DirectMaths->D3DVECTORNormalise(&PointNormals[i]);
		}

		NumTris = 0;
		fsetpos(Stream,&FilePos);
		for(i=0; i<NumPolys; i++) {

			fscanf(Stream,"%x %d",&PolyFlags,&NumPoints);

			if(PolyFlags & iV_IMD_TRANS) {
				ColourKeyIndex=0;
			}
			
			switch(NumPoints) {
				case 3:
					Triangle[NumTris].Flags = PolyFlags;
					fscanf(Stream,"%d %d %d",&i0,&i1,&i2);

					// Just skip the texture anim stuff for now.
					if(PolyFlags & iV_IMD_TEXANIM) {
						fscanf(Stream,"%d %d %d %d",
								&nFrames,
								&pbRate,
								&tWidth,
								&tHeight);
						Triangle[NumTris].TAFrames = nFrames;
						Triangle[NumTris].TARate = pbRate;
						Triangle[NumTris].TAWidth = tWidth;
						Triangle[NumTris].TAHeight = tHeight;
					}

					*( (D3DVECTOR*)&Triangle[NumTris].v0.nx ) = PointNormals[i0];
					*( (D3DVECTOR*)&Triangle[NumTris].v1.nx ) = PointNormals[i1];
					*( (D3DVECTOR*)&Triangle[NumTris].v2.nx ) = PointNormals[i2];
					
					fscanf(Stream,"%d %d",&u,&v);
					fscanf(Stream,"%d %d",&u,&v);
					fscanf(Stream,"%d %d",&u,&v);

					NumTris ++;
					break;
				case 4:
					Triangle[NumTris].Flags = PolyFlags;
					Triangle[NumTris+1].Flags = PolyFlags;
					fscanf(Stream,"%d %d %d %d",&i0,&i1,&i2,&i3);

					// Just skip the texture anim stuff for now.
					if(PolyFlags & iV_IMD_TEXANIM) {
						fscanf(Stream,"%d %d %d %d",
								&nFrames,
								&pbRate,
								&tWidth,
								&tHeight);
						Triangle[NumTris].TAFrames = nFrames;
						Triangle[NumTris].TARate = pbRate;
						Triangle[NumTris].TAWidth = tWidth;
						Triangle[NumTris].TAHeight = tHeight;
						Triangle[NumTris+1].TAFrames = nFrames;
						Triangle[NumTris+1].TARate = pbRate;
						Triangle[NumTris+1].TAWidth = tWidth;
						Triangle[NumTris+1].TAHeight = tHeight;
					}

					*( (D3DVECTOR*)&Triangle[NumTris].v0.nx ) = PointNormals[i0];
					*( (D3DVECTOR*)&Triangle[NumTris].v1.nx ) = PointNormals[i1];
					*( (D3DVECTOR*)&Triangle[NumTris].v2.nx ) = PointNormals[i2];

					*( (D3DVECTOR*)&Triangle[NumTris+1].v0.nx ) = PointNormals[i0];
					*( (D3DVECTOR*)&Triangle[NumTris+1].v1.nx ) = PointNormals[i2];
					*( (D3DVECTOR*)&Triangle[NumTris+1].v2.nx ) = PointNormals[i3];

					fscanf(Stream,"%d %d",&u,&v);
					fscanf(Stream,"%d %d",&u,&v);
					fscanf(Stream,"%d %d",&u,&v);
					fscanf(Stream,"%d %d",&u,&v);
					
					NumTris += 2;
					break;
			}
		}
	}

// Find a radius centered at the local origin that encloses the bounding box.
	float Radius = abs(Smallest.x);
	if(abs(Smallest.y) > Radius) Radius = abs(Smallest.y);
	if(abs(Smallest.z) > Radius) Radius = abs(Smallest.z);
	if(abs(Largest.x) > Radius) Radius = abs(Largest.x);
	if(abs(Largest.y) > Radius) Radius = abs(Largest.y);
	if(abs(Largest.z) > Radius) Radius = abs(Largest.z);

// Find a radius centered at the object center of volume that encloses the object.
	float SRadius = 0.0F;
	D3DVECTOR Center;
	Center.x = (Smallest.x + Largest.x) / 2;
	Center.y = (Smallest.y + Largest.y) / 2;
	Center.z = (Smallest.z + Largest.z) / 2;

#ifdef SRADIUS
	for(i=0; i<NumVerts; i++) {
		float dx = Point[i].x - Center.x;
		float dy = Point[i].y - Center.y;
		float dz = Point[i].z - Center.z;
		float Len = sqrt(dx*dx + dy*dy + dz*dz);
		if(Len > SRadius) {
			SRadius = Len;
		}
	}
#endif

	if(ColourKeyIndex >= 0) {
		m_MatManager->SetColourKey(DestObject->TextureID,0);
	}


	DestObject->Name = new char[strlen(FName)+1];
	strcpy(DestObject->Name,FName);
	if(Description) {
		DestObject->Description = new char[strlen(Description)+1];
		strcpy(DestObject->Description,Description);
	} else {
		DestObject->Description = NULL;
	}

//	DebugPrint("%s : SI %d PI %d\n",Description,StructureIndex,PlayerIndex);

	DestObject->TypeID = TypeID;
	DestObject->StructureID = StructureIndex;
	DestObject->PlayerID = PlayerIndex;
	DestObject->Flanged = Flanged;
	DestObject->Snap = Snap;
	DestObject->ColourKeyIndex = ColourKeyIndex;
	DestObject->Smallest = Smallest;
	DestObject->Largest = Largest;
	DestObject->Center = Center;
	DestObject->Radius = Radius;
	DestObject->SRadius = SRadius;
	DestObject->NumTris = NumTris;
	DestObject->Triangle = new CTriangle[NumTris];
	DestObject->AttachedObject = NULL;

// Store the triangle list.
	for(i=0; i<NumTris; i++) {
		DestObject->Triangle[i] = Triangle[i];
	}

// Delete temporary buffers.
	delete Point;
	delete PointNormals;
	delete Triangle;

	fclose(Stream);

	return TRUE;
}


void CHeightMap::InitSmallestLargest(D3DVECTOR &Smallest,D3DVECTOR &Largest)
{
	Smallest.x = 999999999.0F;
	Smallest.y = 999999999.0F;
	Smallest.z = 999999999.0F;

	Largest.x = -999999999.0F;
	Largest.y = -999999999.0F;
	Largest.z = -999999999.0F;
}

void CHeightMap::UpdateSmallest(D3DVECTOR &v,D3DVECTOR &Smallest)
{
	if(v.x < Smallest.x) Smallest.x = v.x;
	if(v.y < Smallest.y) Smallest.y = v.y;
	if(v.z < Smallest.z) Smallest.z = v.z;
}

void CHeightMap::UpdateLargest(D3DVECTOR &v,D3DVECTOR &Largest)
{
	if(v.x > Largest.x) Largest.x = v.x;
	if(v.y > Largest.y) Largest.y = v.y;
	if(v.z > Largest.z) Largest.z = v.z;
}


// Render an IMD in the normal fashion.
//
void CHeightMap::RenderIMD(C3DObject *Object)
{
	BOOL CullOn = TRUE;

	for(int i=0; i<Object->NumTris; i++) {
		if((Object->Triangle[i].Flags & iV_IMD_NOCUL) && CullOn) {
			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE);
			CullOn = FALSE;
		} else if(!CullOn) {
			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
			CullOn = TRUE;
		}

		if(Object->Triangle[i].Flags & iV_IMD_TEXANIM) {
		   	int framesPerLine = 256 / Object->Triangle[i].TAWidth;
		   	float vFrame = 0.0F;
		   	int frame = m_RenderPlayerID;

		   	while (frame >= framesPerLine)
		   	{
		   		frame -= framesPerLine;
		   		vFrame += (float)Object->Triangle[i].TAHeight;
		   	}
		   	float uFrame = (float)(frame * Object->Triangle[i].TAWidth);

		   	uFrame = uFrame/256.0F;
		   	vFrame = vFrame/256.0F;

			D3DVERTEX ModVert[3];
			ModVert[0] = Object->Triangle[i].v0;
			ModVert[1] = Object->Triangle[i].v1;
			ModVert[2] = Object->Triangle[i].v2;

			ModVert[0].tu += uFrame;
		   	ModVert[0].tv += vFrame;
		   	ModVert[1].tu += uFrame;
		   	ModVert[1].tv += vFrame;
		   	ModVert[2].tu += uFrame;
		   	ModVert[2].tv += vFrame;

			m_MatManager->DrawTriangle(Object->TextureID,ModVert,3);
		} else {
			m_MatManager->DrawTriangle(Object->TextureID,&Object->Triangle[i].v0,3);
		}
	}

	m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
}


// Render an IMD flat ie with it's y component forced to 0.
//
void CHeightMap::RenderFlatIMD(C3DObject *Object)
{
	BOOL CullOn = TRUE;

   	// Render with flattened y component.
   	for(int i=0; i<Object->NumTris; i++) {
		 if((Object->Triangle[i].Flags & iV_IMD_NOCUL) && CullOn) {
 			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE);
 			CullOn = FALSE;
		 } else if(!CullOn) {
 			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
 			CullOn = TRUE;
		 }

		 D3DVERTEX ModVert[3];
		 ModVert[0] = Object->Triangle[i].v0;
		 ModVert[1] = Object->Triangle[i].v1;
		 ModVert[2] = Object->Triangle[i].v2;
		 ModVert[0].y = 0.0F;	//ModVert[0].y*0.1;
		 ModVert[1].y = 0.0F;	//ModVert[1].y*0.1;
		 ModVert[2].y = 0.0F;	//ModVert[2].y*0.1;

		 if(Object->Triangle[i].Flags & iV_IMD_TEXANIM) {
				int framesPerLine = 256 / Object->Triangle[i].TAWidth;
				float vFrame = 0.0F;
			   	int frame = m_RenderPlayerID;

				while (frame >= framesPerLine)
				{
					frame -= framesPerLine;
					vFrame += (float)Object->Triangle[i].TAHeight;
				}
				float uFrame = (float)(frame * Object->Triangle[i].TAWidth);

				uFrame = uFrame/256.0F;
				vFrame = vFrame/256.0F;

				ModVert[0].tu += uFrame;
				ModVert[0].tv += vFrame;
				ModVert[1].tu += uFrame;
				ModVert[1].tv += vFrame;
				ModVert[2].tu += uFrame;
				ModVert[2].tv += vFrame;
		 }

		 m_MatManager->DrawTriangle(Object->TextureID,ModVert,3);
	}

	m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
}


// Render an IMD and morph it's y component to fit the landscape.
//
void CHeightMap::RenderTerrainMorphIMD(C3DObject *Object,D3DVECTOR *Position,D3DVECTOR *Rotation)
{
	BOOL CullOn = TRUE;

	float CenterHeight = GetHeight(Position->x, -Position->z);
	int Angle = (int)Rotation->y;

   	// ensure angle is in range 0-360 and is a multiple of 90
	Angle /= 90;
	Angle *= 90;
	Angle = Angle % 360;

	// Render it.
   	for(int i=0; i<Object->NumTris; i++) {
		 if((Object->Triangle[i].Flags & iV_IMD_NOCUL) && CullOn) {
 			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_NONE);
 			CullOn = FALSE;
		 } else if(!CullOn) {
 			m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
 			CullOn = TRUE;
		 }

		 D3DVERTEX ModVert[3];
		 ModVert[0] = Object->Triangle[i].v0;
		 ModVert[1] = Object->Triangle[i].v1;
		 ModVert[2] = Object->Triangle[i].v2;

		 ModVert[0].y -= CenterHeight;
		 ModVert[1].y -= CenterHeight;
		 ModVert[2].y -= CenterHeight;

		 switch(Angle) {
			 case 0:
				ModVert[0].y += GetInterpolatedHeight(Position->x+ModVert[0].x, -Position->z-ModVert[0].z);
				ModVert[1].y += GetInterpolatedHeight(Position->x+ModVert[1].x, -Position->z-ModVert[1].z);
				ModVert[2].y += GetInterpolatedHeight(Position->x+ModVert[2].x, -Position->z-ModVert[2].z);
				break;
			 case 90:
				ModVert[0].y += GetInterpolatedHeight(Position->x-ModVert[0].z, -Position->z-ModVert[0].x);
				ModVert[1].y += GetInterpolatedHeight(Position->x-ModVert[1].z, -Position->z-ModVert[1].x);
				ModVert[2].y += GetInterpolatedHeight(Position->x-ModVert[2].z, -Position->z-ModVert[2].x);
				break;
			 case 180:
				ModVert[0].y += GetInterpolatedHeight(Position->x-ModVert[0].x, -Position->z+ModVert[0].z);
				ModVert[1].y += GetInterpolatedHeight(Position->x-ModVert[1].x, -Position->z+ModVert[1].z);
				ModVert[2].y += GetInterpolatedHeight(Position->x-ModVert[2].x, -Position->z+ModVert[2].z);
				break;
			 case 270:
				ModVert[0].y += GetInterpolatedHeight(Position->x+ModVert[0].z, -Position->z+ModVert[0].x);
				ModVert[1].y += GetInterpolatedHeight(Position->x+ModVert[1].z, -Position->z+ModVert[1].x);
				ModVert[2].y += GetInterpolatedHeight(Position->x+ModVert[2].z, -Position->z+ModVert[2].x);
				break;
		 }

		 if(Object->Triangle[i].Flags & iV_IMD_TEXANIM) {
				int framesPerLine = 256 / Object->Triangle[i].TAWidth;
				float vFrame = 0.0F;
			   	int frame = m_RenderPlayerID;

				while (frame >= framesPerLine)
				{
					frame -= framesPerLine;
					vFrame += (float)Object->Triangle[i].TAHeight;
				}
				float uFrame = (float)(frame * Object->Triangle[i].TAWidth);

				uFrame = uFrame/256.0F;
				vFrame = vFrame/256.0F;

				ModVert[0].tu += uFrame;
				ModVert[0].tv += vFrame;
				ModVert[1].tu += uFrame;
				ModVert[1].tv += vFrame;
				ModVert[2].tu += uFrame;
				ModVert[2].tv += vFrame;
		 }
		 m_MatManager->DrawTriangle(Object->TextureID,ModVert,3);
	}

	m_DDView->SetRenderState(D3DRENDERSTATE_CULLMODE,D3DCULL_CCW);
}


// Draw a 3d object.
//
void CHeightMap::DrawIMD(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap)
{
	BOOL CullOn = TRUE;
	if(GroundSnap) {
		Position.y = GetHeight(Position.x, -Position.z);
	}


	D3DVECTOR TempVector = Position;
	D3DVECTOR Scale = {1.0,0.2,1.0};

	if(m_Flatten) {
		TempVector.y = 2.0F;	// Sorting hack!
	} else {
		TempVector.y += 2.0F;	// Sorting hack!
	}
// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&Rotation,&TempVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	if(ObjectID >= m_Num3DObjects) return;

	C3DObject *Object = &m_3DObjects[ObjectID];

	if(Object->ColourKeyIndex >= 0) {
		m_DDView->EnableColourKey(TRUE);
	}

	if(Object->AttachedObject) {
		C3DObject *Object2 = Object->AttachedObject;

		if(m_Flatten) {
			// Render flat.
			RenderFlatIMD(Object2);
		} else if(m_TerrainMorph) {
			// Morph the y component to fit the terrain it's sitting over
			RenderTerrainMorphIMD(Object2,&Position,&Rotation);
		} else {
			// Render normally.
			RenderIMD(Object2);
		}
	}

   	if(m_Flatten) {
		// Render flat.
		RenderFlatIMD(Object);
	} else if(m_TerrainMorph) {
		// Morph the y component to fit the terrain it's sitting over
		RenderTerrainMorphIMD(Object,&Position,&Rotation);
   	} else {
		// Render normally.
		RenderIMD(Object);
	}

	if(Object->ColourKeyIndex >= 0) {
		m_DDView->EnableColourKey(FALSE);
	}

}


// Draw a 3d objects bounding box.
//
void CHeightMap::DrawIMDBox(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,DWORD MatID,BOOL GroundSnap,D3DCOLOR Colour)
{
	if(GroundSnap) {
		Position.y = GetHeight(Position.x, -Position.z);
	}

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.

	m_DirectMaths->SetObjectMatrix(&Rotation,&Position,&CameraPosition);
	m_DirectMaths->SetTransformation();

	if(ObjectID >= m_Num3DObjects) return;

	C3DObject *Object = &m_3DObjects[ObjectID];

	D3DLVERTEX	Vertex[5];
	Vertex[0].color = Colour;
	Vertex[1].color = Colour;
	Vertex[2].color = Colour;
	Vertex[3].color = Colour;
	Vertex[4].color = Colour;

	Vertex[0].x = Object->Smallest.x;	Vertex[0].y = Object->Smallest.y;	Vertex[0].z = Object->Smallest.z;
	Vertex[1].x = Object->Largest.x;	Vertex[1].y = Object->Smallest.y;	Vertex[1].z = Object->Smallest.z;
	Vertex[2].x = Object->Largest.x;	Vertex[2].y = Object->Smallest.y;	Vertex[2].z = Object->Largest.z;
	Vertex[3].x = Object->Smallest.x;	Vertex[3].y = Object->Smallest.y;	Vertex[3].z = Object->Largest.z;
	Vertex[4].x = Object->Smallest.x;	Vertex[4].y = Object->Smallest.y;	Vertex[4].z = Object->Smallest.z;
	m_MatManager->DrawPolyLine(MatID,Vertex,5);

	Vertex[0].y = Object->Largest.y;
	Vertex[1].y = Object->Largest.y;
	Vertex[2].y = Object->Largest.y;
	Vertex[3].y = Object->Largest.y;
	Vertex[4].y = Object->Largest.y;
	m_MatManager->DrawPolyLine(MatID,Vertex,5);

	Vertex[0].x = Object->Smallest.x;	Vertex[0].y = Object->Largest.y;	Vertex[0].z = Object->Smallest.z;
	Vertex[1].x = Object->Smallest.x;	Vertex[1].y = Object->Smallest.y;	Vertex[1].z = Object->Smallest.z;
	Vertex[2].x = Object->Largest.x;	Vertex[2].y = Object->Largest.y;	Vertex[2].z = Object->Smallest.z;
	Vertex[3].x = Object->Largest.x;	Vertex[3].y = Object->Smallest.y;	Vertex[3].z = Object->Smallest.z;
	m_MatManager->DrawLine(MatID,Vertex,4);

	Vertex[0].z = Object->Largest.z;
	Vertex[1].z = Object->Largest.z;
	Vertex[2].z = Object->Largest.z;
	Vertex[3].z = Object->Largest.z;
	m_MatManager->DrawLine(MatID,Vertex,4);
}


void CHeightMap::DrawIMDStats(C3DObjectInstance *Data,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
	C3DObject *Object = &m_3DObjects[Data->ObjectID];

	// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&Data->Rotation,&Data->Position,&CameraPosition);
	m_DirectMaths->SetTransformation();

	D3DVERTEX	Test;
	if(!m_DirectMaths->TransformVertex(&Test,(D3DVERTEX*)&ZeroVector,1,NULL,FALSE)) {
		return;
	}

	float Radius = Object->Radius;
	// Given the transformed center point, scale the radius.
	float TestRadius = Radius*(1.0F-Test.z)*1.25F;

	// Test for offscreen and return if so.
	if(Test.x+TestRadius < 0) return;
	if(Test.y+TestRadius < 0) return;
	if(Test.x-TestRadius > 639) return;
	if(Test.y-TestRadius > 439) return;

	CSurface *Surface = m_DDView->GetBackBufferSurface();
	HDC hdc = Surface->GetDC();

	DrawStats(hdc,(int)Test.x-TestRadius,(int)Test.y-TestRadius,Object,Data);

	Surface->ReleaseDC();
}


void CHeightMap::DrawStats(HDC hdc,int x,int y,C3DObject *Object,C3DObjectInstance *Data)
{
	SelectObject(hdc,g_Font);
	char String[256];

//	DebugPrint("OTID %d OPID %d\n",Object->TypeID,Object->PlayerID);
	
	Object = &m_3DObjects[Data->ObjectID];

	if(Object->TypeID == IMD_STRUCTURE) {
		if(Data->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Player %d Dir %.2f (%s)",
							GetObjectName(Object),
//							m_Structures[Object->StructureID].StructureName,
							Data->UniqueID,
							Data->PlayerID,Data->Rotation.y,
							Data->ScriptName);
//							Object->PlayerID,Data->Rotation.y);
		} else {
			sprintf(String,"%s ID %d Player %d (%s)",
							GetObjectName(Object),
//							m_Structures[Object->StructureID].StructureName,
							Data->UniqueID,
							Data->PlayerID,
							Data->ScriptName);
//							Object->PlayerID);
		}
	} else if(Object->TypeID == IMD_DROID) {
		char *Name = GetObjectName(Object);

//		if(Object->Description) {
//			Name = Object->Description;
//		} else {
//			Name = Object->Name;
//		}
		if(Data->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Player %d Dir %.2f (%s)",
							Name,Data->UniqueID,Data->PlayerID,Data->Rotation.y,Data->ScriptName);
		} else {
			sprintf(String,"%s ID %d Player %d (%s)" ,
							Name,Data->UniqueID,Data->PlayerID,Data->ScriptName);
		}
	} else {
		char *Name = GetObjectName(Object);

//		if(Object->Description) {
//			Name = Object->Description;
//		} else {
//			Name = Object->Name;
//		}
		if(Data->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Dir %.2f (%s)",
							Name,Data->UniqueID,Data->Rotation.y,Data->ScriptName);
		} else {
			sprintf(String,"%s ID %d (%s)",
							Name,Data->UniqueID,Data->ScriptName);
		}
	}

	TextOut(hdc,x,y,String,strlen(String));
}


void CHeightMap::DrawIMDSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,
						 DWORD MatID,BOOL GroundSnap,UBYTE Red,UBYTE Green,UBYTE Blue)
{
	if(GroundSnap) {
		Position.y = GetHeight(Position.x, -Position.z);
	}

	if(ObjectID >= m_Num3DObjects) return;
	C3DObject *Object = &m_3DObjects[ObjectID];
	D3DVECTOR ObjPos = Position;

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&Rotation,&ObjPos,&CameraPosition);
	m_DirectMaths->SetTransformation();

	D3DVERTEX	Test;
	if(!m_DirectMaths->TransformVertex(&Test,(D3DVERTEX*)&ZeroVector,1,NULL,FALSE)) {
		return;
	}

#ifdef SRADIUS
	float Radius = Object->SRadius;
#else
	float Radius = Object->Radius;
#endif
	// Given the transformed center point, scale the radius.
	float TestRadius = Radius*(1.0F-Test.z)*1.25F;

	// Test for offscreen and return if so.
	if(Test.x+TestRadius < 0) return;
	if(Test.y+TestRadius < 0) return;
	if(Test.x-TestRadius > 639) return;
	if(Test.y-TestRadius > 439) return;

	D3DLVERTEX	Vertex[SPHEREDIVS+1];

	DWORD Colour = RGBA_MAKE(Red,Green,Blue,0);

//	This version works on any card that can render points.
#ifdef SRADIUS
	for(int j=0; j<SPHEREDIVS; j++) {
		for(int i=0; i<SPHEREDIVS; i++) {
			Vertex[i].color = Colour;
			Vertex[i].x = Object->Center.x+Radius*SinLook[i] * CosLook[j];
			Vertex[i].y = Object->Center.y+Radius*CosLook[i];
			Vertex[i].z = Object->Center.z+Radius*SinLook[i] * SinLook[j];
		}

		m_MatManager->DrawPoint(MatID,Vertex,SPHEREDIVS);
	}
#else
	for(int j=0; j<SPHEREDIVS; j++) {
		for(int i=0; i<SPHEREDIVS; i++) {
			Vertex[i].color = Colour;
			Vertex[i].x = Radius*SinLook[i] * CosLook[j];
			Vertex[i].y = Radius*CosLook[i];
			Vertex[i].z = Radius*SinLook[i] * SinLook[j];
		}

		m_MatManager->DrawPoint(MatID,Vertex,SPHEREDIVS);
	}
#endif
//	This version works on any card that can't. (Software Yuk!)
//	D3DLVERTEX	TVertex[SPHEREDIVS+1];
//
//	CSurface *Surface = m_DDView->GetBackBufferSurface();
//	Surface->Lock();
//
//	for(int j=0; j<SPHEREDIVS; j++) {
//		for(int i=0; i<SPHEREDIVS; i++) {
//			Vertex[i].x = Radius*SinLook[i] * CosLook[j];
//			Vertex[i].y = Radius*CosLook[i];
//			Vertex[i].z = Radius*SinLook[i] * SinLook[j];
//		}
//
//		if(m_DirectMaths->TransformVertex((D3DVERTEX*)TVertex,(D3DVERTEX*)Vertex,SPHEREDIVS,NULL,FALSE)) {
//			for(int i=0; i<SPHEREDIVS; i++) {
//				Surface->PutPixel(TVertex[i].x,TVertex[i].y,Red,Green,Blue);
//			}
//		}
//	}
//	Surface->Unlock();
}



// Draw a 2d objects bounding box.
//
void CHeightMap::DrawIMDFootprint2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
								int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip)
{
	if(Instance->ObjectID >= m_Num3DObjects) return;

	C3DObject *Object = &m_3DObjects[Instance->ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	ScrollX/=(int)m_TextureWidth;
	ScrollX*=(int)m_TextureWidth;
	ScrollY/=(int)m_TextureHeight;
	ScrollY*=(int)m_TextureHeight;

	VECTOR2D Pos;
	Pos.x = Instance->Position.x;
	Pos.y = -Instance->Position.z;

	Pos.x += (float)(m_MapWidth*m_TileWidth)/2;
	Pos.x /= (float)(m_TileWidth/m_TextureWidth);
	Pos.y += (float)(m_MapHeight*m_TileHeight)/2;
	Pos.y /= (float)(m_TileHeight/m_TextureHeight);
	Pos.x -= (float)ScrollX;
	Pos.y -= (float)ScrollY;

	if( (Pos.x < Clip->left) || (Pos.x > Clip->right) ||
		(Pos.y < Clip->top) || (Pos.y > Clip->bottom)) {
		return;
	}

	HDC	dc=(HDC)DIBDraw->GetDIBDC();

	// Draw a filled rect to show the objects footprint.

	if(Object->TypeID == IMD_STRUCTURE) {
		COLORREF FillColour = RGB(	0,
									((Instance->UniqueID%16)*15)+15,
									0);
		HDC hdc = (HDC)DIBDraw->GetDIBDC();
		HBRUSH Brush = CreateSolidBrush(FillColour);
		RECT Rect;

		int TileX = (Pos.x / m_TextureWidth) - (m_Structures[Object->StructureID].baseWidth/2);
		int TileY = (Pos.y / m_TextureHeight) - (m_Structures[Object->StructureID].baseBreadth/2);

		TileX *= m_TextureWidth;
		TileY *= m_TextureHeight;

		int Width = m_Structures[Object->StructureID].baseWidth * m_TextureWidth;
		int Height = m_Structures[Object->StructureID].baseBreadth * m_TextureHeight;

		SetRect(&Rect,TileX,TileY,TileX+Width,TileY+Height);
		FillRect(dc,&Rect,Brush);

		DeleteObject(Brush);
	} else if(Object->TypeID == IMD_FEATURE) {
		COLORREF FillColour = RGB(	0,
									0,
									((Instance->UniqueID%16)*15)+15 );
		HDC hdc = (HDC)DIBDraw->GetDIBDC();
		HBRUSH Brush = CreateSolidBrush(FillColour);
		RECT Rect;

		int TileX = (Pos.x / m_TextureWidth) - (m_Features[Object->StructureID].baseWidth/2);
		int TileY = (Pos.y / m_TextureHeight) - (m_Features[Object->StructureID].baseBreadth/2);

		TileX *= m_TextureWidth;
		TileY *= m_TextureHeight;

		int Width = m_Features[Object->StructureID].baseWidth * m_TextureWidth;
		int Height = m_Features[Object->StructureID].baseBreadth * m_TextureHeight;

		SetRect(&Rect,TileX,TileY,TileX+Width,TileY+Height);
		FillRect(dc,&Rect,Brush);

		DeleteObject(Brush);
	}
}


// Draw a 2d objects bounding box.
//
void CHeightMap::DrawIMDBox2D(CDIBDraw *DIBDraw,C3DObjectInstance *Instance,
								int ScrollX,int ScrollY,COLORREF Colour,RECT *Clip)
{
	if(Instance->ObjectID >= m_Num3DObjects) return;

	C3DObject *Object = &m_3DObjects[Instance->ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	ScrollX/=(int)m_TextureWidth;
	ScrollX*=(int)m_TextureWidth;
	ScrollY/=(int)m_TextureHeight;
	ScrollY*=(int)m_TextureHeight;

	VECTOR2D Pos;
	Pos.x = Instance->Position.x;
	Pos.y = -Instance->Position.z;

	Pos.x += (float)(m_MapWidth*m_TileWidth)/2;
	Pos.x /= (float)(m_TileWidth/m_TextureWidth);
	Pos.y += (float)(m_MapHeight*m_TileHeight)/2;
	Pos.y /= (float)(m_TileHeight/m_TextureHeight);
	Pos.x -= (float)ScrollX;
	Pos.y -= (float)ScrollY;

	if( (Pos.x < Clip->left) || (Pos.x > Clip->right) ||
		(Pos.y < Clip->top) || (Pos.y > Clip->bottom)) {
		return;
	}

	HDC	dc=(HDC)DIBDraw->GetDIBDC();
	HPEN NormalPen = CreatePen(PS_SOLID,1,Colour);
	HPEN OldPen = (HPEN)SelectObject(dc,NormalPen);


	MoveToEx(dc,(int)Pos.x-8,(int)Pos.y,NULL);
	LineTo(dc,(int)Pos.x+8,(int)Pos.y);
	MoveToEx(dc,(int)Pos.x,(int)Pos.y-8,NULL);
	LineTo(dc,(int)Pos.x,(int)Pos.y+8);

	D3DVECTOR Smallest = Object->Smallest;
	D3DVECTOR Largest = Object->Largest;

#ifdef NEGZIN2D
	Smallest.z = -Smallest.z;		// Negate z's
	Largest.z = -Largest.z;
#endif

	Smallest.x /= (float)(m_TileWidth/m_TextureWidth);
	Smallest.z /= (float)(m_TileHeight/m_TextureHeight);
	Largest.x /= (float)(m_TileWidth/m_TextureWidth);
	Largest.z /= (float)(m_TileHeight/m_TextureHeight);

	VECTOR2D Vec[4];
	VECTOR2D TVec[4];

	Vec[0].x = Smallest.x; Vec[0].y = Smallest.z;
	Vec[1].x = Largest.x; Vec[1].y = Smallest.z;
	Vec[2].x = Largest.x; Vec[2].y = Largest.z;
	Vec[3].x = Smallest.x; Vec[3].y = Largest.z;

	m_DirectMaths->RotateVector2D(Vec,TVec,&Pos,Instance->Rotation.y,4);

	MoveToEx(dc,(int)TVec[0].x,(int)TVec[0].y,NULL);
	LineTo(dc,(int)TVec[1].x,(int)TVec[1].y);
	LineTo(dc,(int)TVec[2].x,(int)TVec[2].y);
	LineTo(dc,(int)TVec[3].x,(int)TVec[3].y);
	LineTo(dc,(int)TVec[0].x,(int)TVec[0].y);
	LineTo(dc,(int)TVec[2].x,(int)TVec[2].y);
	MoveToEx(dc,(int)TVec[1].x,(int)TVec[1].y,NULL);
	LineTo(dc,(int)TVec[3].x,(int)TVec[3].y);

//	TextOut(dc,(int)Pos.x+4,(int)Pos.y-20,
//				Object->Name,
//				strlen(Object->Name) );

	char String[256];
//	if(Rotation.y != 0.0F) {
//		sprintf(String,"%s %.2f",Object->Name,Rotation.y);
//	} else {
//		sprintf(String,"%s",Object->Name);
//	}

 	Object = &m_3DObjects[Instance->ObjectID];

	SelectObject(dc,g_Font);
	if(Object->TypeID == IMD_STRUCTURE) {
		if(Instance->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Player %d Dir %.2f",
													GetObjectName(Object),
//													m_Structures[Object->StructureID].StructureName,
													Instance->UniqueID,
													Instance->PlayerID,
//													Object->PlayerID,
													Instance->Rotation.y);
		} else {
			sprintf(String,"%s ID %d Player %d",
												GetObjectName(Object),
//												m_Structures[Object->StructureID].StructureName,
												Instance->UniqueID,
  												Instance->PlayerID);
//												Object->PlayerID);
		}
	} else if(Object->TypeID == IMD_DROID) {
		char *Name = GetObjectName(Object);

//		if(Object->Description) {
//			Name = Object->Description;
//		} else {
//			Name = Object->Name;
//		}
		if(Instance->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Player %d Dir %.2f",Name,
													Instance->UniqueID,
													Instance->PlayerID,Instance->Rotation.y);
		} else {
			sprintf(String,"%s ID %d Player %d",Name,
												Instance->UniqueID,
												Instance->PlayerID);
		}
	} else {
		char *Name = GetObjectName(Object);

//		if(Object->Description) {
//			Name = Object->Description;
//		} else {
//			Name = Object->Name;
//		}
		if(Instance->Rotation.y != 0.0F) {
			sprintf(String,"%s ID %d Dir %.2f",Name,
											Instance->UniqueID,
											Instance->Rotation.y);
		} else {
			sprintf(String,"%s ID %d",Name,
									Instance->UniqueID);
		}
	}
	TextOut(dc,(int)Pos.x+4,(int)Pos.y-20,String,strlen(String));

	SelectObject(dc,OldPen);

	DeleteObject(NormalPen);
}


// Check to see if a click in 3d falls within the bounding box of an object.
// Currently just does a naff check to see if the hit point is near one of
// the corners of the transformed bounding box.
//
BOOL CHeightMap::ObjectHit3D(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap,
						 int HitX,int HitY)
{
	C3DObject *Object = &m_3DObjects[ObjectID];

	if(GroundSnap) {
		Position.y = GetHeight(Position.x, -Position.z);
	}

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&Rotation,&Position,&CameraPosition);
	m_DirectMaths->SetTransformation();

	D3DVERTEX Vertex[8];
	D3DVERTEX TVertex[8];
	D3DHVERTEX HVertex[8];
	BOOL AllClipped;

	Vertex[0].x = Object->Smallest.x;	Vertex[0].y = Object->Smallest.y;	Vertex[0].z = Object->Smallest.z;
	Vertex[1].x = Object->Largest.x;	Vertex[1].y = Object->Smallest.y;	Vertex[1].z = Object->Smallest.z;
	Vertex[2].x = Object->Largest.x;	Vertex[2].y = Object->Smallest.y;	Vertex[2].z = Object->Largest.z;
	Vertex[3].x = Object->Smallest.x;	Vertex[3].y = Object->Smallest.y;	Vertex[3].z = Object->Largest.z;
	Vertex[4].x = Object->Smallest.x;	Vertex[4].y = Object->Largest.y;	Vertex[4].z = Object->Smallest.z;
	Vertex[5].x = Object->Largest.x;	Vertex[5].y = Object->Largest.y;	Vertex[5].z = Object->Smallest.z;
	Vertex[6].x = Object->Largest.x;	Vertex[6].y = Object->Largest.y;	Vertex[6].z = Object->Largest.z;
	Vertex[7].x = Object->Smallest.x;	Vertex[7].y = Object->Largest.y;	Vertex[7].z = Object->Largest.z;

	AllClipped = !m_DirectMaths->TransformVertex(TVertex, Vertex,8,HVertex);

	if(!AllClipped) {
		for(int i=0; i<8; i++) {
			if(HVertex[i].dwFlags == 0) {
				float dx = abs(HitX - TVertex[i].x);
				float dy = abs(HitY - TVertex[i].y);

				if((dx < 8.0F) && (dy < 8.0F)) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}


// Check to see if a click in 3d falls within the bounding sphere of an object.
//
SDWORD CHeightMap::ObjectHit3DSphere(DWORD ObjectID,D3DVECTOR &Rotation,D3DVECTOR &Position,
						 D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL GroundSnap,
						 int HitX,int HitY)
{
	if(GroundSnap) {
		Position.y = GetHeight(Position.x, -Position.z);
	}

	if(ObjectID >= m_Num3DObjects) return -1;

	C3DObject *Object = &m_3DObjects[ObjectID];
	D3DVECTOR ObjPos = Position;

//	Center.x += Object->Largest.x;	Center.y += Object->Largest.y;	Center.z += Object->Largest.z;
//	Center.x /= 2.0F;	Center.y /= 2.0F;	Center.z /= 2.0F;
//	ObjPos.x += Center.x;	ObjPos.y += Center.y;	ObjPos.z += Center.z;

// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&Rotation,&ObjPos,&CameraPosition);
	m_DirectMaths->SetTransformation();

	float Radius = Object->Radius;

	D3DVERTEX	Test;
	if(!m_DirectMaths->TransformVertex(&Test,(D3DVERTEX*)&ZeroVector,1,NULL,-1)) {
		return -1;
	}

	// Given the transformed center point, scale the radius.
	Radius = Radius*(1.0F-Test.z)*1.25F;

	// Test for offscreen and return if so.
	if(Test.x+Radius < 0) return -1;
	if(Test.y+Radius < 0) return -1;
	if(Test.x-Radius > 639) return -1;
	if(Test.y-Radius > 439) return -1;

	float dx = Test.x - HitX;
	float dy = Test.y - HitY;
	float Dist = sqrt(dx*dx + dy*dy);

	if(Dist < Radius) return (SDWORD)Dist;

	return -1;
}


// Check to see if a click in 2d falls within the bounding box of an object.
//
BOOL CHeightMap::ObjectHit2D(DWORD ObjectID,
								D3DVECTOR &Rotation,D3DVECTOR &Position,
								int ScrollX,int ScrollY,
								int HitX,int HitY)
{
	VECTOR2D Vec,TVec;
	BOOL Hit = TRUE;

// Convert the screen position and scroll values into a world space coordinate.
	ScrollX -= m_TextureWidth*OVERSCAN;
	ScrollY -= m_TextureHeight*OVERSCAN;
	HitX = (HitX+ScrollX)*(m_TileWidth/m_TextureWidth) - (m_MapWidth*m_TileWidth/2);
	HitY = (HitY+ScrollY)*(m_TileHeight/m_TextureHeight) - (m_MapHeight*m_TileHeight/2);

#ifdef NEGZIN2D
// Make it relative to the object to be tested.
	Vec.x = ((float)HitX) - Position.x;
	Vec.y = -(((float)HitY) + Position.z);

// Rotate the vector into object space.
	m_DirectMaths->RotateVector2D(&Vec,&TVec,NULL,Rotation.y,1);
#else
// Make it relative to the object to be tested.
	Vec.x = ((float)HitX) - Position.x;
	Vec.y = ((float)HitY) + Position.z;

// Rotate the vector into object space.
	m_DirectMaths->RotateVector2D(&Vec,&TVec,NULL,-Rotation.y,1);
#endif

// Now compare to see if it's inside the objects bounding box.
	C3DObject *Object = &m_3DObjects[ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	if((TVec.x < Object->Smallest.x) || (TVec.x > Object->Largest.x)) {
		Hit = FALSE;
	}

	if((TVec.y < Object->Smallest.z) || (TVec.y > Object->Largest.z)) {
		Hit = FALSE;
	}

	return Hit;
}


// Add an instantiation of a 3d object to be displayed in the world.
//
DWORD CHeightMap::AddObject(DWORD ObjectID, const D3DVECTOR& Rotation, const D3DVECTOR& Position, DWORD PlayerID)
{
//	DebugPrint("Added Object, ID %d\n",ObjectID);

	C3DObject* Object = &m_3DObjects[ObjectID];

	C3DObjectInstance Data;
	Data.ObjectID = ObjectID;
	Data.Position = Position;
	Data.Rotation = Rotation;
	Data.Selected = FALSE;
	Data.ScriptName[0] = 0;

//	if(Object->TypeID == IMD_STRUCTURE) {
//		Data.PlayerID = Object->PlayerID;
//	} else {
		Data.PlayerID = PlayerID;
//	}
	Data.UniqueID = m_NewObjectID++;

	m_Objects.push_back(Data);

//	SetObjectTileHeights(m_TotalInstances);

	++m_TotalInstances;

	return m_TotalInstances - 1;
}


// Add an instantiation of a 3d object to be displayed in the world.
//
DWORD CHeightMap::AddObject(DWORD ObjectID, const D3DVECTOR& Rotation, const D3DVECTOR& Position, DWORD UniqueID,
							DWORD PlayerID, const char* ScriptName)
{
	C3DObject *Object = &m_3DObjects[ObjectID];

	C3DObjectInstance Data;
	Data.ObjectID = ObjectID;
	Data.Position = Position;
	Data.Rotation = Rotation;
	Data.Selected = FALSE;
	strcpy(Data.ScriptName, ScriptName);

//	if(Object->TypeID == IMD_STRUCTURE) {
//		Data.PlayerID = Object->PlayerID;
//	} else {
		Data.PlayerID = PlayerID;
//	}
	Data.UniqueID = UniqueID;

	if(UniqueID >= m_NewObjectID)
	{
		m_NewObjectID = UniqueID + 1;
	}

	m_Objects.push_back(Data);

//	SetObjectTileHeights(m_TotalInstances);

	++m_TotalInstances;

	return m_TotalInstances-1;
}



// Remove the specified instantiation of a 3d object.
//
void CHeightMap::RemoveObject(DWORD Index)
{
	if(m_Objects.empty()
	 || Index >= m_Objects.size())
		return;

	std::list<C3DObjectInstance>::iterator toRemove = m_Objects.begin();
	std::advance(toRemove, Index);

	m_Objects.erase(toRemove);
	--m_TotalInstances;
}


// Select an object.
//
void CHeightMap::Select3DObject(DWORD Index)
{
	GetObjectPointer(Index)->Selected = TRUE;
}


// DeSelect an object.
//
void CHeightMap::DeSelect3DObject(DWORD Index)
{
	GetObjectPointer(Index)->Selected = FALSE;
}


// Select all objects.
//
void CHeightMap::SelectAll3DObjects()
{
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		curNode->Selected = TRUE;
	}
}


// DeSelect all objects.
//
void CHeightMap::DeSelectAll3DObjects()
{
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		curNode->Selected = FALSE;
	}
}


// Delete selected objects.
//
void CHeightMap::DeleteSelected3DObjects()
{
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end();)
	{
		if (curNode->Selected)
		{
			std::list<C3DObjectInstance>::iterator toRemove = curNode++;
			m_Objects.erase(toRemove);
			--m_TotalInstances;

			continue;
		}

		++curNode;
	}
}


void CHeightMap::Get3DObjectRotation(DWORD Index,D3DVECTOR &Rotation)
{
	Rotation = GetObjectPointer(Index)->Rotation;
}


void CHeightMap::Set3DObjectRotation(DWORD Index,D3DVECTOR &Rotation)
{
	GetObjectPointer(Index)->Rotation = Rotation;
}

char *CHeightMap::GetObjectInstanceName(int Index)
{
	return m_3DObjects[GetObjectPointer(Index)->ObjectID].Name;
}

char *CHeightMap::GetObjectInstanceDescription(int Index)
{
	return GetObjectName(&m_3DObjects[GetObjectPointer(Index)->ObjectID]);
}

char *CHeightMap::GetObjectInstanceScriptName(int Index)
{
	return GetObjectPointer(Index)->ScriptName;
}

void CHeightMap::SetObjectInstanceScriptName(int Index,char *ScriptName)
{
	if(ScriptName != NULL) {
		assert(strlen(ScriptName) < MAX_SCRIPTNAME);
		strcpy(GetObjectPointer(Index)->ScriptName , ScriptName);
	} else {
		GetObjectPointer(Index)->ScriptName[0] = 0;
	}
}



BOOL CHeightMap::GetObjectInstanceFlanged(int Index)
{
	return m_3DObjects[GetObjectPointer(Index)->ObjectID].Flanged;
}

int CHeightMap::GetObjectID(int Index)
{
	return GetObjectPointer(Index)->ObjectID;
}


BOOL CHeightMap::GetObjectInstanceSnap(int Index)
{
	return m_3DObjects[GetObjectPointer(Index)->ObjectID].Snap;
}


char *CHeightMap::GetObjectTypeName(int Index)
{
	return IMDTypeNames[m_3DObjects[Index].TypeID];
}

int CHeightMap::GetObjectInstancePlayerID(int Index)
{
	return GetObjectPointer(Index)->PlayerID;
}

void CHeightMap::SetObjectInstancePlayerID(int Index,int PlayerID)
{
	assert(PlayerID < 8);

	C3DObjectInstance *Instance = GetObjectPointer(Index);
	C3DObject *Object = &m_3DObjects[Instance->ObjectID];

	Instance->PlayerID = PlayerID;

//	if(Object->TypeID == IMD_STRUCTURE) {
//		int CurPlayerID = Object->PlayerID;
//		Instance->ObjectID += PlayerID - CurPlayerID;
//		assert(m_3DObjects[Instance->ObjectID].PlayerID < 8);
//	}
}

int CHeightMap::GetObjectInstanceUniqueID(int Index)
{
	return GetObjectPointer(Index)->UniqueID;
}



char *CHeightMap::GetObjectInstanceTypeName(int Index)
{
	return IMDTypeNames[m_3DObjects[GetObjectPointer(Index)->ObjectID].TypeID];
}


int CHeightMap::GetObjectType(int Index)
{
	return m_3DObjects[Index].TypeID;
}

int CHeightMap::GetObjectPlayer(int Index)
{
	return m_3DObjects[Index].PlayerID;
}


char *CHeightMap::GetObjectName(int Index)
{
   	if( (m_3DObjects[Index].RealName >= 0) && (m_UseRealNames == TRUE) ) {
   		return m_ObjNames[m_3DObjects[Index].RealName].Name;
   	}
   	if(m_3DObjects[Index].Description) {
   		return m_3DObjects[Index].Description;
   	} 
   	return m_3DObjects[Index].Name;
}


char *CHeightMap::GetObjectName(C3DObject *Object)
{
   	if( (Object->RealName >= 0) && (m_UseRealNames == TRUE) ) {
   		return m_ObjNames[Object->RealName].Name;
   	}
   	if(Object->Description) {
   		return Object->Description;
   	} 
   	return Object->Name;
}





void CHeightMap::Delete3DObjects()
{
	for(int i=0; i<m_Num3DObjects; i++) {
		m_MatManager->DeleteMaterial(m_3DObjects[i].TextureID);
		delete m_3DObjects[i].Name;
		delete m_3DObjects[i].Description;
		delete m_3DObjects[i].Triangle;

		if(m_3DObjects[i].AttachedObject) {
			if(m_3DObjects[i].PlayerID == 0) {
				delete m_3DObjects[i].AttachedObject->Name;
				delete m_3DObjects[i].AttachedObject->Description;
				delete m_3DObjects[i].AttachedObject->Triangle;
			}
			delete m_3DObjects[i].AttachedObject;
		}
	}
	m_Num3DObjects = 0;

	if(m_FeatureSet) {
		delete m_FeatureSet;
		m_FeatureSet = NULL;
	}
}

// Delete all data associated with 3d objects.
//
void CHeightMap::Delete3DObjectInstances()
{
	m_Objects.clear();
	m_TotalInstances = 0;
}



BOOL CHeightMap::ObjectIsStructure(int ObjectID)
{
	if(m_3DObjects[ObjectID].TypeID == IMD_STRUCTURE) {
		return TRUE;
	}

	return FALSE;
}


BOOL CHeightMap::StructureIsDefense(int ObjectID)
{
	if(m_3DObjects[ObjectID].TypeID == IMD_STRUCTURE) {
		if(strcmp(m_Structures[m_3DObjects[ObjectID].StructureID].Type,"DEFENSE") == 0) {
			return TRUE;
		}
	}

	return FALSE;
}


BOOL CHeightMap::StructureIsWall(int ObjectID)
{
	if(m_3DObjects[ObjectID].TypeID == IMD_STRUCTURE) {
		if( (strcmp(m_Structures[m_3DObjects[ObjectID].StructureID].Type,"WALL") == 0) || 
			(strcmp(m_Structures[m_3DObjects[ObjectID].StructureID].Type,"CORNER WALL") == 0) ) {
			return TRUE;
		}
	}

	return FALSE;
}



// Draw all 3d objects.
//
void CHeightMap::DrawObjects(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,BOOL Boxed)
{
	const int CullRadius = static_cast<int>(GetVisibleRadius());
	
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if(m_EnablePlayers[curNode->PlayerID] == TRUE)
		{
			SetRenderPlayerID(curNode->PlayerID);

			if(abs(static_cast<int>(curNode->Position.x - CameraPosition.x)) < CullRadius)
			{
				if( abs(static_cast<int>(curNode->Position.z - CameraPosition.z)) < CullRadius)
				{
					if (m_3DObjects[curNode->ObjectID].TypeID == IMD_STRUCTURE
					 || m_3DObjects[curNode->ObjectID].TypeID == IMD_FEATURE
					 || !m_IgnoreDroids)
					{
						m_TerrainMorph = FALSE;
						if(m_3DObjects[curNode->ObjectID].TypeID == IMD_STRUCTURE)
						{
							if (strcmp(m_Structures[m_3DObjects[curNode->ObjectID].StructureID].Type, "WALL") == 0
							 || strcmp(m_Structures[m_3DObjects[curNode->ObjectID].StructureID].Type, "CORNER WALL") == 0)
							{
								m_TerrainMorph = TRUE;
							}
						}

						DrawIMD(curNode->ObjectID, curNode->Rotation, curNode->Position, CameraRotation, CameraPosition, TRUE);
					}

					if(Boxed)
					{
						if(curNode->Selected)
						{
							DrawIMDSphere(curNode->ObjectID, curNode->Rotation, curNode->Position,
								CameraRotation, CameraPosition, m_LineMaterial2, TRUE, 255, 0, 0);
						}
						else
						{
							DrawIMDSphere(curNode->ObjectID, curNode->Rotation, curNode->Position,
								CameraRotation, CameraPosition, m_LineMaterial2, TRUE, 255, 255, 255);
						}
					}

	//				if((curNode->Selected) || (m_3DObjects[curNode->ObjectID].TypeID == IMD_DROID))
					if(curNode->Selected)
						DrawIMDStats(&*curNode, CameraRotation, CameraPosition);
				}
			}
		}
	}
}

// Draw all 3d objects ( as 2d boxes ).
//
void CHeightMap::DrawObjects2D(CDIBDraw *DIBDraw,int ScrollX,int ScrollY,RECT *Clip)
{
	std::list<C3DObjectInstance>::iterator curNode;

	for (curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (m_EnablePlayers[curNode->PlayerID])
		{
			if(curNode->Selected)
			{
				DrawIMDFootprint2D(DIBDraw, &*curNode, ScrollX, ScrollY, RGB(255, 0, 0), Clip);
			}
			else
			{
				DrawIMDFootprint2D(DIBDraw, &*curNode, ScrollX, ScrollY, RGB(255, 255, 255), Clip);
			}
		}
	}

	for (curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if(m_EnablePlayers[curNode->PlayerID])
		{
			if(curNode->Selected)
			{
				DrawIMDBox2D(DIBDraw, &*curNode, ScrollX, ScrollY, RGB(255, 0, 0), Clip);
			}
			else
			{
				DrawIMDBox2D(DIBDraw, &*curNode, ScrollX, ScrollY, RGB(255, 255, 255), Clip);
			}
		}
	}
}

// Draw object representations in the radar.
//
void CHeightMap::DrawRadarObjects(CDIBDraw *DIBDraw,int Scale)
{
//	HDC	dc=(HDC)DIBDraw->GetDIBDC();
//	HPEN NormalPen = CreatePen(PS_SOLID,2,RGB(0,255,0));
//	HPEN OldPen = SelectObject(dc,NormalPen);
	UWORD PixelFeature = DIBDraw->GetDIBValue(64,64,255);
	UWORD PixelStructure = DIBDraw->GetDIBValue(0,255,0);
	UWORD PixelDroid =  DIBDraw->GetDIBValue(255,32,255);
	UWORD PixelResource =  DIBDraw->GetDIBValue(255,0,0);


	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (m_EnablePlayers[curNode->PlayerID])
		{
			D3DVECTOR Position = curNode->Position;

   			Position.z = -Position.z;
   			Position.x /= static_cast<float>(m_TileWidth);
   			Position.x += static_cast<float>(m_MapWidth / 2);
			Position.x *= Scale;
   			Position.z /= static_cast<float>(m_TileHeight);
   			Position.z += static_cast<float>(m_MapHeight / 2);
			Position.z *= Scale;

			Position.x += OVERSCAN * Scale;
			Position.z += OVERSCAN * Scale;

			if (m_3DObjects[curNode->ObjectID].TypeID == IMD_FEATURE)
			{
	//			DebugPrint("%s\n",m_Features[m_3DObjects[curNode->ObjectID].StructureID].featureName);
				if (strcmp(m_Features[m_3DObjects[curNode->ObjectID].StructureID].featureName, "OilResource") == 0)
				{
					DIBDraw->PutDIBFatPixel(PixelResource, static_cast<int>(Position.x), static_cast<int>(Position.z));
				}
				else
				{
					DIBDraw->PutDIBFatPixel(PixelFeature, static_cast<int>(Position.x), static_cast<int>(Position.z));
				}
	//			DIBDraw->PutDIBFatPixel(PixelFeature,(int)Position.x,(int)Position.z);
			}
			else if (m_3DObjects[curNode->ObjectID].TypeID == IMD_STRUCTURE)
			{
				DIBDraw->PutDIBFatPixel(PixelStructure, static_cast<int>(Position.x), static_cast<int>(Position.z));
			}
			else if (m_3DObjects[curNode->ObjectID].TypeID == IMD_DROID)
			{
				DIBDraw->PutDIBFatPixel(PixelDroid, static_cast<int>(Position.x), static_cast<int>(Position.z));
			}
		}
//		DIBDraw->PutDIBFatPixel(Pixel2, static_cast<int>(Position.x), static_cast<int>(Position.z));

//		MoveToEx(dc, static_cast<int>(Position.x), static_cast<int>(Position.z), NULL);
//		LineTo(dc, static_cast<int>(Position.x), static_cast<int>(Position.z));
	}

//	SelectObject(dc, OldPen);
//	DeleteObject(NormalPen);
}

// Hit test for objects in 3d view.
//
int CHeightMap::FindObjectHit3D(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition,int HitX,int HitY)
{
	int Index = 0;

	int CullRadius = (int)GetVisibleRadius();
	int ClosestDist = -1;
	int Closest = -1;

#if 0
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (abs(static_cast<int>(curNode->Position.x - CameraPosition.x)) < CullRadius)
		{
			if (abs(static_cast<int>(curNode->Position.z - CameraPosition.z)) < CullRadius)
			{
				if (ObjectHit3D(curNode->ObjectID, curNode->Rotation, curNode->Position,
					            CameraRotation, CameraPosition, TRUE, HitX, HitY))
				{
					return Index;
				}
			}
		}
		++Index;
	}
#else
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (m_EnablePlayers[curNode->PlayerID])
		{
			if (abs(static_cast<int>(curNode->Position.x - CameraPosition.x)) < CullRadius)
			{
				if (abs(static_cast<int>(curNode->Position.z - CameraPosition.z)) < CullRadius)
				{
					int Dist = ObjectHit3DSphere(curNode->ObjectID, curNode->Rotation, curNode->Position,
					                             CameraRotation, CameraPosition, TRUE, HitX, HitY);

					if (ClosestDist < 0
					 || Dist < ClosestDist)
					{
						if (Dist >= 0)
						{
							ClosestDist = Dist;
							Closest = Index;
						}
					}
				}
			}
		}

		++Index;
	}
#endif

	return Closest;
}

// Hit test for objects in 2d view.
//
int CHeightMap::FindObjectHit2D(int ScrollX,int ScrollY,int HitX,int HitY)
{
	int Index = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if(m_EnablePlayers[curNode->PlayerID]
		 && ObjectHit2D(curNode->ObjectID, curNode->Rotation, curNode->Position,
		                ScrollX, ScrollY, HitX, HitY))
		{
			return Index;
		}

		++Index;
	}

	return -1;
}

// Get a pointer to the specified 3d objects instance structure.
//
C3DObjectInstance *CHeightMap::GetObjectPointer(DWORD Index)
{
	if (m_Objects.size() <= Index)
		return NULL;

	std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin();
	std::advance(curNode, Index);

	return &*curNode;
}

// Snap an objects position to the landscape tiles.
//
void CHeightMap::SnapObject(DWORD Index)
{
	C3DObjectInstance *Data = GetObjectPointer(Index);
	C3DObject *Object = &m_3DObjects[Data->ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	if((!Object->Snap) || (m_NoObjectSnap)) {
		return;
	}

	int x,z;
	int Width;
	int Height;

// Make relative to map top left.
	Data->Position.x += (float)(m_TileWidth*m_MapWidth/2 + OVERSCAN*m_TileWidth);
	Data->Position.z += (float)(m_TileHeight*m_MapHeight/2 + OVERSCAN*m_TileHeight);

// Find size in tiles.
	switch(Object->TypeID) {
		case	IMD_STRUCTURE:	// If it's a structure then get size from stats.
			Width = m_Structures[Object->StructureID].baseWidth;
			Height = m_Structures[Object->StructureID].baseBreadth;
			break;

		case	IMD_FEATURE:	// If it's a feature then get size from stats;
			Width = m_Features[Object->StructureID].baseWidth;
			Height = m_Features[Object->StructureID].baseBreadth;
			break;

		default:
			Width = (int)(Object->Largest.x - Object->Smallest.x);
			Height = (int)(Object->Largest.z - Object->Smallest.z);

			Width = Width/m_TileWidth;
			Height = Height/m_TileHeight;
	}

// Snap to tile corner.
	x = (int)(Data->Position.x)/m_TileWidth;
	x = x*m_TileWidth;

// If the width is odd then snap to tile center
	if((Width & 1) || (Width == 0)) {
		x += m_TileWidth/2;
	}

// Snap to tile corner.
	z = (int)(Data->Position.z)/m_TileHeight;
	z = z*m_TileHeight;

// If the height is odd then snap to tile center
	if((Height & 1) || (Height == 0)) {
		z += m_TileHeight/2;
	}

	Data->Position.x = (float)x;
	Data->Position.z = (float)z;

// Make relative to map center.
	Data->Position.x -= (float)(m_TileWidth*m_MapWidth/2  + OVERSCAN*m_TileWidth);
	Data->Position.z -= (float)(m_TileHeight*m_MapHeight/2  + OVERSCAN*m_TileHeight);
}


// Show or hide the tiles underneath an object.
//
// This version copes with 90 degree rotations.
//
void CHeightMap::SetObjectTileFlags(DWORD Index,DWORD Flags)
{
	return;
/*
	C3DObjectInstance *Data = GetObjectPointer(Index);
	C3DObject *Object = &m_3DObjects[Data->ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	if(!Object->Flanged) return;

// Make relative to map top left.
	int x = (int)Data->Position.x + m_TileWidth*m_MapWidth/2;
	int z = (int)(-Data->Position.z) + m_TileHeight*m_MapHeight/2;

// Can only deal with 90 degree rotations.
	if( ((int)Data->Rotation.y)%90 !=0) {
		return;
	}

	VECTOR2D Vec[2],TVec[2];
	Vec[0].x = Object->Smallest.x;
	Vec[0].y = -Object->Smallest.z;
	Vec[1].x = Object->Largest.x;
	Vec[1].y = -Object->Largest.z;
	m_DirectMaths->RotateVector2D(Vec,TVec,NULL,Data->Rotation.y,2);

	int sx = x+(int)TVec[0].x;
	int sz = z+(int)TVec[0].y;
	int ex = x+(int)TVec[1].x;
	int ez = z+(int)TVec[1].y;
	int Tmp;

	if(ex < sx) {
		Tmp = ex;
		ex = sx;
		sx = Tmp;
	}

	if(ez < sz) {
		Tmp = ez;
		ez = sz;
		sz = Tmp;
	}

	sx = (sx + m_TileWidth - 1) / m_TileWidth;
	sz = (sz + m_TileHeight - 1) / m_TileHeight;
	ex /= m_TileWidth;
	ez /= m_TileHeight;

// Find size in tiles.
	int Width = ex-sx;
	int Height = ez-sz;

	for(int iz=sz; iz<sz+Height; iz++) {
		for(int ix=sx; ix<sx+Width; ix++) {
			if((ix >= 0) && (ix < m_MapWidth) && (iz >= 0) && (iz < m_MapHeight)) {
				SetTileVisible(iz * m_MapWidth + ix,Flags);
			}
		}
	}
*/
}


// Show or hide the tiles underneath an object.
//
// This version copes with 90 degree rotations.
//
void CHeightMap::SetObjectTileHeights(DWORD Index)
{
	C3DObjectInstance *Data = GetObjectPointer(Index);
	C3DObject *Object = &m_3DObjects[Data->ObjectID];

	if(Object->AttachedObject) {	// Assume base plate is always bigger than the object.
		Object = Object->AttachedObject;
	}

	if(!Object->Flanged) return;

// Make relative to map top left.
	int x = (int)Data->Position.x + m_TileWidth*m_MapWidth/2;
	int z = (int)(-Data->Position.z) + m_TileHeight*m_MapHeight/2;
	float y = Data->Position.y / m_HeightScale;

// Can only deal with 90 degree rotations.
	if( ((int)Data->Rotation.y)%90 !=0) {
		return;
	}

	VECTOR2D Vec[2],TVec[2];
	Vec[0].x = Object->Smallest.x;
	Vec[0].y = -Object->Smallest.z;
	Vec[1].x = Object->Largest.x;
	Vec[1].y = -Object->Largest.z;
	m_DirectMaths->RotateVector2D(Vec,TVec,NULL,Data->Rotation.y,2);

	int sx = x+(int)TVec[0].x;
	int sz = z+(int)TVec[0].y;
	int ex = x+(int)TVec[1].x;
	int ez = z+(int)TVec[1].y;
	int Tmp;

	if(ex < sx) {
		Tmp = ex;
		ex = sx;
		sx = Tmp;
	}

	if(ez < sz) {
		Tmp = ez;
		ez = sz;
		sz = Tmp;
	}

	sx = (sx + m_TileWidth - 1) / m_TileWidth;
	sz = (sz + m_TileHeight - 1) / m_TileHeight;
	ex /= m_TileWidth;
	ez /= m_TileHeight;

// Find size in tiles.
	int Width = ex-sx;
	int Height = ez-sz;

	for(int iz=sz; iz<sz+Height; iz++) {
		for(int ix=sx; ix<sx+Width; ix++) {
			if((ix >= 0) && (ix < m_MapWidth) && (iz >= 0) && (iz < m_MapHeight)) {
				SetTileHeight(iz * m_MapWidth + ix,y);
			}
		}
	}
}


// Show or hide the tiles underneath all objects.
//
void CHeightMap::SetObjectTileFlags(DWORD Flags)
{
	return;
/*
	for(int i=0; i<m_TotalInstances; i++) {
		SetObjectTileFlags(i,Flags);
	}
*/
}

// Show or hide the tiles underneath all objects.
//
void CHeightMap::SetObjectTileHeights()
{
	for(int i=0; i<m_TotalInstances; i++) {
		SetObjectTileHeights(i);
	}
}

// Set a 3d objects position.
//
void CHeightMap::Set3DObjectPosition(DWORD Index,D3DVECTOR &Position)
{
	C3DObjectInstance *Data = GetObjectPointer(Index);
	C3DObject *Object = &m_3DObjects[Data->ObjectID];

	switch(Object->TypeID) {
//		case	IMD_FEATURE:
//			Stats = &m_Features[Object->StructureID];
//			Data->Position.x = (((UDWORD)Position.x) & (~(m_TileWidth-1)) ) + Stats->baseWidth * m_TileWidth / 2;
//			Data->Position.y = (((UDWORD)Position.y) & (~(m_TileWidth-1)) ) + Stats->baseBreadth * m_TileWidth / 2;
//			Data->Position.z = Position.z;
		default:
			Data->Position = Position;
	}
}

// Get a 3d objects position.
//
void CHeightMap::Get3DObjectPosition(DWORD Index,D3DVECTOR &Position)
{
	C3DObjectInstance *Data = GetObjectPointer(Index);
	Position = Data->Position;
}


void StripPath(char *Dst,char *Src)
{
	char *Pnt = Src+strlen(Src);

	while(Pnt >= Src) {
		if((*Pnt=='\\') || (*Pnt=='/') || (*Pnt==':')) {
			break;
		}
		Pnt--;
	}
	Pnt++;

	strcpy(Dst,Pnt);
}


	
BOOL CHeightMap::WriteObjectList(FILE *Stream)
{
	return WriteObjectList(Stream,0,0,m_MapWidth,m_MapHeight);
}

// Write the object list.
//
BOOL CHeightMap::WriteObjectList(FILE *Stream,UWORD StartX,UWORD StartY,UWORD Width,UWORD Height)
{
	C3DObject *Object;
	char FeatureSet[256];
	int NumObjects = 0;

	if(CheckUniqueScriptNames()==FALSE) {
		MessageBox(NULL,"Duplicate script names found\n","Warning",MB_OK);
	}

	if(CheckUniqueIDs()==FALSE) {
		if(MessageBox(NULL,"Duplicate IDs found\nDo you want to renumber all objects?","Warning",MB_YESNO) == IDYES) {
			SetUniqueIDs();
		}
	}

// Count how many objects we need to save.
	std::list<C3DObjectInstance>::iterator curNode;
	for (curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		float NewCx = static_cast<float>(m_MapWidth / 2 - (StartX + Width / 2)) * m_TileWidth;
		float NewCz = static_cast<float>(m_MapHeight / 2 - (StartY + Height / 2)) * m_TileHeight;
		float RadX =  static_cast<float>(Width / 2) * m_TileWidth;
		float RadZ =  static_cast<float>(Height / 2) * m_TileHeight;

		D3DVECTOR Position = curNode->Position;
		Position.x += NewCx;
		Position.z -= NewCz;

		if (abs(Position.x) <= RadX
		 && abs(Position.z) <= RadZ)
			++NumObjects;
	}

	fprintf(Stream,"ObjectList {\n");
#ifdef WRITENAMEDOBJECTS
	fprintf(Stream,"    Version %d\n",CURRENT_OBJECTLIST_VERSION);
#endif
	if(m_FeatureSet) {
		StripPath(FeatureSet,m_FeatureSet);
		fprintf(Stream,"	FeatureSet %s\n",FeatureSet);
	} else {
		fprintf(Stream,"	FeatureSet %s\n",NULL);
	}
	fprintf(Stream,"    NumObjects %d\n",NumObjects);
//	fprintf(Stream,"    NumObjects %d\n",m_TotalInstances);
	fprintf(Stream,"    Objects {\n");

	for (curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		float NewCx = static_cast<float>(m_MapWidth / 2 - (StartX + Width / 2)) * m_TileWidth;
		float NewCz = static_cast<float>(m_MapHeight / 2 - (StartY + Height / 2)) * m_TileHeight;
		float RadX = static_cast<float>(Width / 2) * m_TileWidth;
		float RadZ = static_cast<float>(Height / 2) * m_TileHeight;

		D3DVECTOR Position = curNode->Position;
		Position.x += NewCx;
		Position.z -= NewCz;

		if (abs(Position.x) <= RadX
		 && abs(Position.z) <= RadZ)
		{
	#ifdef WRITENAMEDOBJECTS
			Object = &m_3DObjects[curNode->ObjectID];

			fprintf(Stream, "        %d ", curNode->UniqueID);
			fprintf(Stream, "%d ", Object->TypeID);
			if(Object->TypeID == IMD_STRUCTURE)
			{
				fprintf(Stream, "\"%s\" ", m_Structures[Object->StructureID].StructureName);
				fprintf(Stream, "%d ", curNode->PlayerID);

				if(curNode->ScriptName[0])
				{
					fprintf(Stream, "\"%s\" ", curNode->ScriptName);
				}
				else
				{
					fprintf(Stream, "\"NONAME\" ");
				}
			}
			else
			{
				if(Object->Description)
				{
					fprintf(Stream, "\"%s\" ", Object->Description);
					fprintf(Stream, "%d ", curNode->PlayerID);

					if(curNode->ScriptName[0])
					{
						fprintf(Stream, "\"%s\" ", curNode->ScriptName);
					}
					else
					{
						fprintf(Stream, "\"NONAME\" ");
					}
				}
				else
				{
					fprintf(Stream, "\"NONAME\" ");
					fprintf(Stream, "%d ", curNode->ObjectID);
				}
			}
	#else
			fprintf(Stream, "        %d ", curNode->ObjectID);
	#endif

			fprintf(Stream, "%.2f %.2f %.2f ", Position.x, Position.y, Position.z);
			fprintf(Stream, "%.2f %.2f %.2f\n", curNode->Rotation.x, curNode->Rotation.y, curNode->Rotation.z);

		}
	}

	fprintf(Stream, "    }\n");
	fprintf(Stream, "}\n");

	return TRUE;
}


BOOL CHeightMap::CheckUniqueScriptNames()
{

	int NumIDs = 0;
	char *ScriptNames = new char[m_TotalInstances*MAX_SCRIPTNAME];
	int NumDups = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		for (unsigned int i = 0; i < NumIDs; ++i)
		{
			if(curNode->ScriptName[0])
			{
				if(_stricmp(curNode->ScriptName, &ScriptNames[i * MAX_SCRIPTNAME]) == 0)
				{
					++NumDups;
				}
			}
		}

		if (curNode->ScriptName[0])
		{
			strcpy(&ScriptNames[NumIDs * MAX_SCRIPTNAME], curNode->ScriptName);
		}

		++NumIDs;
	}

	delete ScriptNames;

	if(NumDups) {
		return FALSE;
	}

	return TRUE;
}


BOOL CHeightMap::CheckUniqueIDs()
{
	int NumIDs = 0;
	int *IDs = new int[m_TotalInstances];
	int NumDups = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		for(unsigned int i = 0; i < NumIDs; ++i)
		{
			if(curNode->UniqueID == IDs[i])
				NumDups++;
		}

		IDs[NumIDs] = curNode->UniqueID;
		++NumIDs;
	}

	delete IDs;

	if(NumDups) {
		return FALSE;
	}

	return TRUE;
}


void CHeightMap::SetUniqueIDs()
{
	unsigned int ID = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		curNode->UniqueID = ID++;
	}
}


// Read the object list.
//
BOOL CHeightMap::ReadObjectList(FILE *Stream)
{
	LONG Version;
	LONG NumObjects;
	LONG ObjectID;
	LONG UniqueID;
	D3DVECTOR Position;
	D3DVECTOR Rotation;
	char FeatureSet[32];
	char ScriptName[256];

	ScriptName[0] = 0;

	CHECKTRUE(StartChunk(Stream,"ObjectList"));

	fpos_t Pos;
	int PosOk;
	PosOk = fgetpos(Stream, &Pos );
	if(!ReadLong(Stream,"Version",(LONG*)&Version)) {
		PosOk = fsetpos(Stream, &Pos );
		DebugPrint("No Version Number in object list.\n");
		Version = 0;
	}

	CHECKTRUE(ReadQuotedString(Stream,"FeatureSet",FeatureSet));

/* 12/4/99
	if(strcmp(FeatureSet,"(null)")) {
		sprintf(FullPath,"%s\\%s",g_WorkDirectory.c_str(),FeatureSet);
//		strcpy(FullPath,g_WorkDirectory.c_str());
//		strcat(FullPath,"\\Data\\");
//		strcat(FullPath,FeatureSet);
		if(!ReadIMDObjects(FullPath)) {
			MessageBox(NULL,"An error occured while loading the objects","Error",MB_OK);
			return FALSE;
		}
	}
*/
	CHECKTRUE(ReadLong(Stream,"NumObjects",(LONG*)&NumObjects));
	CHECKTRUE(StartChunk(Stream,"Objects"));

	if(Version < 2) {
		m_NewObjectID = 0;
	}

	LONG PlayerID;

	for(int i=0; i<NumObjects; i++) {
		BOOL ObjectOK = TRUE;

		if(Version > 0) {
			char Name[256];
			LONG TypeID;

			if(Version >= 2) {
				CHECKTRUE(ReadLong(Stream,NULL,&UniqueID));
			}

			CHECKTRUE(ReadLong(Stream,NULL,&TypeID));
			CHECKTRUE(ReadQuotedString(Stream,NULL,Name));
//			DebugPrint("%s\n",Name);
			if(TypeID == IMD_STRUCTURE) {
				CHECKTRUE(ReadLong(Stream,NULL,&PlayerID));
//				DebugPrint("## %s PlayerID %d\n",Name,PlayerID);
//				ObjectID = GetStructureID(Name,PlayerID);
				ObjectID = GetObjectID(Name);
				if(ObjectID < 0) {
					ObjectOK = FALSE;
//					return FALSE;
				}
				if(Version >= 3) {
					CHECKTRUE(ReadQuotedString(Stream,NULL,ScriptName));
					if(strcmp(ScriptName,"NONAME") == 0) {
						ScriptName[0] = 0;
					}
				}
			} else {
				if(strcmp(Name,"NONAME")) {	// If a name is defined then next field is the player ID.
					CHECKTRUE(ReadLong(Stream,NULL,&PlayerID));	// PlayerID not used in this case.
					ObjectID = GetObjectID(Name);
//					if(ObjectID < 0) {
//						return FALSE;
//					}
					if(Version >= 3) {
						CHECKTRUE(ReadQuotedString(Stream,NULL,ScriptName));
						if(strcmp(ScriptName,"NONAME") == 0) {
							ScriptName[0] = 0;
						}
					}
				} else {					// If no name defined then next field is the objects ID.
					CHECKTRUE(ReadLong(Stream,NULL,&ObjectID));
				}
			}
		} else {
			PlayerID = 0;
			CHECKTRUE(ReadLong(Stream,NULL,&ObjectID));
		}

		CHECKTRUE(ReadFloat(Stream,NULL,&Position.x));
		CHECKTRUE(ReadFloat(Stream,NULL,&Position.y));
		CHECKTRUE(ReadFloat(Stream,NULL,&Position.z));
		CHECKTRUE(ReadFloat(Stream,NULL,&Rotation.x));
		CHECKTRUE(ReadFloat(Stream,NULL,&Rotation.y));
		CHECKTRUE(ReadFloat(Stream,NULL,&Rotation.z));

		if(ObjectOK == TRUE) {
			if((ObjectID >= 0) && (ObjectID < m_Num3DObjects)) {
				if(Version >= 2) {
					AddObject(ObjectID,Rotation,Position,UniqueID,PlayerID,ScriptName);
				} else {
					AddObject(ObjectID,Rotation,Position,PlayerID);
				}
			} else {
	//			MessageBox(NULL,"ObjectID not found","Error",MB_OK);
			}
		}
	}

	CHECKTRUE(EndChunk(Stream));
	CHECKTRUE(EndChunk(Stream));

	if(CheckUniqueScriptNames()==FALSE) {
		MessageBox(NULL,"Duplicate script names found\n","Warning",MB_OK);
	}

	if(CheckUniqueIDs()==FALSE) {
		if(MessageBox(NULL,"Duplicate IDs found\nDo you want to renumber all objects?","Warning",MB_YESNO) == IDYES) {
			SetUniqueIDs();
		}
	}

	return TRUE;
}


//LONG CHeightMap::GetStructureID(char *Name,LONG PlayerID)
//{
//	char Drive[256];
//	char Dir[256];
//	char FName[256];
//	char Ext[256];
//
//	for(int i=0; i<m_NumStructures; i++) {
//		if(strcmp(Name,m_Structures[i].StructureName) == 0) {
//			// Found a match in the structure list.
//			// now need to find the IMD specified in the structure list
//			// in the IMD list.
//
//			for(LONG j=0; j<m_Num3DObjects; j++) {
//
//				_splitpath(m_Structures[i].IMDName[PlayerID],Drive,Dir,FName,Ext);
//
//				if( (strcmp(FName , m_3DObjects[j].Name) == 0)  &&
//					(m_3DObjects[j].PlayerID == PlayerID) ) {
////					DebugPrint("Found %s\n",Name);
//					return j;
//				}
//			}
//			MessageBox(NULL,FName,"Not found",MB_OK);
//
//			return -1;
//		}
//	}
//
//	MessageBox(NULL,Name,"Not found",MB_OK);
//
//	return -1;
//}


LONG CHeightMap::GetObjectID(char *Name)
{
//	char Drive[256];
//	char Dir[256];
//	char FName[256];
//	char Ext[256];

	for(int i=0; i<m_Num3DObjects; i++) {
		if(m_3DObjects[i].Description) {
			if(strcmp(Name,m_3DObjects[i].Description) == 0) {
//				DebugPrint("Found %s\n",Name);
				return i;
			}
		}
	}

	MessageBox(NULL,Name,"Object not in stats!",MB_OK);

	return -1;
}


// Import a map and feature list in Deliverance format.
//
//BOOL CHeightMap::ReadDeliveranceMap(FILE *Stream)
//{
//	MAP_SAVEHEADER Header;
//	MAP_SAVETILE Tile;
//
//	if( fread(&Header,SAVE_HEADER_SIZE,1,Stream) != 1) {
//		return FALSE;
//	}
//
//	if( (Header.aFileType[0] != 'm') ||
//		(Header.aFileType[1] != 'a') ||
//		(Header.aFileType[2] != 'p') ||
//		(Header.aFileType[3] != ' ') ) {
//		return FALSE;
//	}
//
////	if(Header.version != 2) {
////		return FALSE;
////	}
//
//	if( (Header.width != m_MapWidth) ||
//		(Header.height != m_MapHeight) ) {
//		return FALSE;
//	}
//
//
//	for(int z = 0; z < m_MapHeight; z++) {
//		for(int x = 0; x < m_MapWidth; x++) {
//
//			if( fread(&Tile,SAVE_TILE_SIZE,1,Stream) != 1) {
//				return FALSE;
//			}
//
//			SetTextureID(x+z*m_MapWidth,(Tile.texture & TILE_NUMMASK)+1);
//
//			DWORD Rotate = (Tile.texture&TILE_ROTMASK) >> TILE_ROTSHIFT;
//			BOOL XFlip = Tile.texture & TILE_XFLIP;
//			BOOL YFlip = Tile.texture & TILE_YFLIP;
//
//			SetTextureFlip(x+z*m_MapWidth,XFlip,YFlip);
//			SetTextureRotate(x+z*m_MapWidth,Rotate);
////			SetTileType(x+z*m_MapWidth, (Tile.type << TF_TYPESHIFT) & TF_TYPEMASK);
//			
//			SetVertexHeight(x+z*m_MapWidth,0,Tile.height);
//
//			if(x > 0) {
//				SetVertexHeight((x-1)+z*m_MapWidth,1,Tile.height);
//			}
//			if((x > 0) && (z > 0)) {
//				SetVertexHeight((x-1)+(z-1)*m_MapWidth,2,Tile.height);
//			}
//			if(z > 0) {
//				SetVertexHeight(x+(z-1)*m_MapWidth,3,Tile.height);
//			}
//		}
//	}
//
//	InitialiseSectors();
//
//	return TRUE;
//}


BOOL CHeightMap::WriteDeliveranceGame(FILE *Stream,UDWORD GameType,int LimIndex)
{
	GAME_SAVEHEADER		Header;
	SAVE_GAME			SaveGame;

	/* Put the file header on the file */
	Header.aFileType[0] = 'g';
	Header.aFileType[1] = 'a';
	Header.aFileType[2] = 'm';
	Header.aFileType[3] = 'e';
	Header.version = CURRENT_GAME_VERSION_NUM;

	if( fwrite(&Header,GAME_HEADER_SIZE,1,Stream) != 1) return FALSE;

	memset(&SaveGame,0,sizeof(SaveGame));

	SaveGame.gameTime = 0;
//	SaveGame.x = m_MapWidth/2;
//	SaveGame.y = m_MapHeight/2;
//	SaveGame.width = m_MapWidth;
//	SaveGame.height = m_MapHeight;

	SaveGame.GameType = GameType;

	if (LimIndex >= 0)
	{
		assert(LimIndex < m_ScrollLimits.size());
		std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin();
		std::advance(curNode, LimIndex);

		SaveGame.ScrollMinX = curNode->MinX;
		SaveGame.ScrollMinY = curNode->MinZ;
		SaveGame.ScrollMaxX = curNode->MaxX;
		SaveGame.ScrollMaxY = curNode->MaxZ;
	}
	else
	{
		SaveGame.ScrollMinX = 0;
		SaveGame.ScrollMinY = 0;
		SaveGame.ScrollMaxX = m_MapWidth;
		SaveGame.ScrollMaxY = m_MapHeight;
	}

//
//	int i=0;
//	TmpNode = m_ScrollLimits;
//	while(TmpNode!=NULL) {
//		Data = TmpNode->GetData();
//		if(Data->ScriptName[0]) {
//			SaveGame.ScrollLimits[i].LimitID = Data->UniqueID;
//			SaveGame.ScrollLimits[i].MinX = Data->MinX;
//			SaveGame.ScrollLimits[i].MinZ = Data->MinZ;
//			SaveGame.ScrollLimits[i].MaxX = Data->MaxX;
//			SaveGame.ScrollLimits[i].MaxZ = Data->MaxZ;
//		}
//
//		TmpNode = TmpNode->GetNextNode();
//		i++;
//		if(i >= MAX_LIMITS) {
//			MessageBox(NULL,"Max scroll limits exceeded, ignoring extras","Warning",MB_OK);
//			break;
//		}
//	}
//
//	SaveGame.NumLimits = i;

	if (fwrite(&SaveGame, sizeof(SAVE_GAME), 1, Stream) != 1)
		return FALSE;

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceTileTypes(FILE *Stream)
{
	TILETYPE_SAVEHEADER Header;

	/* Put the file header on the file */
	Header.aFileType[0] = 't';
	Header.aFileType[1] = 't';
	Header.aFileType[2] = 'y';
	Header.aFileType[3] = 'p';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = m_MaxTMapID;

	if( fwrite(&Header,TILETYPE_SAVEHEADER_SIZE,1,Stream) != 1) return FALSE;

	for(int i=0; i<m_MaxTMapID; i++) {
		UWORD Type = g_Types[i+1];
		fwrite(&Type,sizeof(Type),1,Stream);
//		DebugPrint("%d : %d\n",i,Type);
	}	

	return TRUE;
}

// Write out the map and feature list in Deliverance format.
//
BOOL CHeightMap::WriteDeliveranceMap(FILE *Stream)
{
	MAP_SAVEHEADER Header;
	MAP_SAVETILE Tile;
	unsigned int MapWidth,MapHeight;

// Get the size of the map
	GetMapSize(MapWidth, MapHeight);

// Fill in the header.
	Header.aFileType[0] = 'm';
	Header.aFileType[1] = 'a';
	Header.aFileType[2] = 'p';
	Header.aFileType[3] = ' ';
	Header.version = CURRENT_MAP_VERSION_NUM;	//CURRENT_GAME_VERSION_NUM;
	Header.width = MapWidth;
	Header.height = MapHeight;

// Write out the header.
	if( fwrite(&Header,SAVE_HEADER_SIZE,1,Stream) != 1) return FALSE;

// Now write out the tiles.
	for(UDWORD i=0; i < MapWidth * MapHeight; ++i)
	{
		UBYTE TMapID = (UBYTE)GetTextureID(i);
		UDWORD Texture;
		if(TMapID == 0) {
			Texture = (UWORD)(TMapID & TILE_NUMMASK);
		} else {
			Texture = (UWORD)((TMapID-1) & TILE_NUMMASK);
		}
		
		if(GetTextureFlipX(i)) {
			Texture |= TILE_XFLIP;
		}
		if(GetTextureFlipY(i)) {
			Texture |= TILE_YFLIP;
		}
		DWORD Rotate = GetTextureRotate(i);
		Rotate = Rotate << TILE_ROTSHIFT;
		Texture |= Rotate;
		

		if(GetVertexFlip(i)) {
			Texture |= TILE_TRIFLIP;
		}

		Tile.texture = Texture;
//		Tile.type = GetTileType(i);
//
//		if(GetTileVisible(i) == TF_HIDE) {
//			Tile.type |= TER_OBJECT;
//		}

		Tile.height = (UBYTE)GetVertexHeight(i,0);

		if( fwrite(&Tile,SAVE_TILE_SIZE,1,Stream) != 1) return FALSE;
	}

	GATEWAY_SAVEHEADER GateHeader;

	GateHeader.version = CURRENT_GATEWAY_VERSION;
	GateHeader.numGateways = m_NumGateways;
	
	if (fwrite(&GateHeader, sizeof(GateHeader), 1, Stream) != 1)
		return FALSE;


	for (std::list<GateWay>::const_iterator curNode = m_Gateways.begin(); curNode != m_Gateways.end(); ++curNode)
	{
		GATEWAY_SAVE Gate;
		Gate.x0 = static_cast<UBYTE>(curNode->x0);
		Gate.y0 = static_cast<UBYTE>(curNode->y0);
		Gate.x1 = static_cast<UBYTE>(curNode->x1);
		Gate.y1 = static_cast<UBYTE>(curNode->y1);

		if (fwrite(&Gate, sizeof(Gate), 1, Stream) != 1)
			return FALSE;
	}

	giSetMapData(this);
	if(!giCreateZones()) {
		MessageBox( NULL, "Error", "Max number of zones exceeded", MB_OK );
	}
	giWriteZones(Stream);
	giUpdateMapZoneIDs();
	giDeleteZones();

	return TRUE;
}


#if 0
BOOL CHeightMap::ReadDeliveranceFeatures(FILE *Stream)
{
	FEATURE_SAVEHEADER Header;
	SAVE_FEATURE Feature;

	fread(&Header, 1, FEATURE_HEADER_SIZE, Stream);

	if (Header.aFileType[0] != 'f'
	 || Header.aFileType[1] != 'e'
	 || Header.aFileType[2] != 'a'
	 || Header.aFileType[3] != 't')
		return FALSE;

	if(Header.version != 1)
		return FALSE;


	for(int Index = 0; Index<Header.quantity; Index++) {
		fread(&Feature,1,sizeof(Feature),Stream);

		C3DObject *Object = &m_3DObjects[Feature.featureInc];

		D3DVECTOR Position;
		D3DVECTOR Rotation={0.0F,0.0F,0.0F};

		Rotation.y = (float)Feature.direction;
		Position.y = 0.0F;

#ifdef ADJUSTFEATURECOORDS
		Position.x = (float)Feature.x - (float)(m_MapWidth*m_TileWidth/2);
		Position.x += (abs(Object->Largest.x - Object->Smallest.x) / m_TileWidth ) * m_TileWidth / 2;

		Position.z = (float)Feature.y - (float)(m_MapHeight*m_TileHeight/2);
		Position.z += (abs(Object->Largest.z - Object->Smallest.z) / m_TileHeight ) * m_TileHeight / 2;
		Position.z = -Position.z;
#else
		Position.x = (float)Feature.x - (float)(m_MapWidth*m_TileWidth/2);

		Position.z = (float)Feature.y - (float)(m_MapHeight*m_TileHeight/2);
		Position.z = -Position.z;
#endif
		int ObjID = AddObject(Feature.featureInc,Rotation,Position);
		SnapObject(ObjID);
	}

	return TRUE;
}
#endif


int CHeightMap::CountObjectsOfType(int Type,int Exclude,int Include)
{
	unsigned int Count = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (IncludeIt(curNode->Position.x, curNode->Position.z, Exclude, Include))
		{
			C3DObject *Object = &m_3DObjects[curNode->ObjectID];

			if(Object->TypeID == Type)
				++Count;
		}
	}

	return Count;
}


void CHeightMap::GetLimitRect(int Index, LimitRect& Limit)
{
	assert(Index < m_ScrollLimits.size());

	std::list<CScrollLimits>::const_iterator ScrollNode = m_ScrollLimits.begin();
	std::advance(ScrollNode, Index);

	Limit.x0 = (ScrollNode->MinX - (m_MapWidth / 2)) * m_TileWidth;
	Limit.z0 = (ScrollNode->MinZ - (m_MapHeight / 2)) * m_TileHeight;

	Limit.x1 = (ScrollNode->MaxX - (m_MapWidth / 2)) * m_TileWidth;
	Limit.z1 = (ScrollNode->MaxZ - (m_MapHeight / 2)) * m_TileHeight;
}


BOOL CHeightMap::InLimit(float x,float z,int Index)
{
	LimitRect Limit;

	GetLimitRect(Index,Limit);

	z = -z;

	if((x >= Limit.x0) && (x <= Limit.x1) && (z >= Limit.z0) && (z <= Limit.z1)) {
		return TRUE;
	}

	return FALSE;
}


BOOL CHeightMap::IncludeIt(float x,float z,int Exclude,int Include)
{
	if( (Exclude < 0) && (Include < 0) ) {
		return TRUE;
	}

	if((Exclude >= 0) && (Include >= 0)) {
		if( (!InLimit(x,z,Exclude)) && InLimit(x,z,Include) ) {
			return TRUE;
		}
	} else if(Include >= 0) {
		if( InLimit(x,z,Include) ) {
			return TRUE;
		}
	} else if(Exclude >= 0) {
		if( !InLimit(x,z,Exclude) ) {
			return TRUE;
		}
	}

	return FALSE;
}


// Write out the object list in Deliverance format.
//
BOOL CHeightMap::WriteDeliveranceFeatures(FILE *Stream,UDWORD GameType,int Exclude,int Include)
{
	FEATURE_SAVEHEADER Header;
	SAVE_FEATURE Feature;

	Header.aFileType[0] = 'f';
	Header.aFileType[1] = 'e';
	Header.aFileType[2] = 'a';
	Header.aFileType[3] = 't';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = CountObjectsOfType(IMD_FEATURE,Exclude,Include);

	fwrite(&Header,FEATURE_HEADER_SIZE,1,Stream);

	DebugPrint("Exporting features\n");
	DebugPrint("Exclude %d Include %d\n",Exclude,Include);
	DebugPrint("Quantity %d\n",Header.quantity);

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		C3DObject *Object = &m_3DObjects[curNode->ObjectID];
		_feature_stats *Stats = &m_Features[Object->StructureID];

		if (Object->TypeID == IMD_FEATURE)
		{
			if (IncludeIt(curNode->Position.x, curNode->Position.z, Exclude, Include))
			{
				Feature.id = curNode->UniqueID;

				Feature.x = static_cast<UDWORD>(curNode->Position.x + (m_MapWidth * m_TileWidth / 2));
				Feature.y = static_cast<UDWORD>(-curNode->Position.z + (m_MapHeight * m_TileHeight / 2));
				Feature.z = static_cast<UDWORD>(curNode->Position.y);

				Feature.direction = static_cast<UDWORD>(curNode->Rotation.y) % 360;

				Feature.inFire = FALSE;
				Feature.burnDamage = 0;

				if(Object->Description)
				{
					strcpy((char*)Feature.name, Object->Description);
				}
				else
				{
					MessageBox(NULL,"Missing feature description","Error",MB_OK);
					return FALSE;
				}

				DebugPrint("Included Feature : %s %d ( %d %d )\n",
							Feature.name,Feature.id,Feature.x,Feature.y);

				fwrite(&Feature, sizeof(Feature), 1, Stream);
			}
		}
	}

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceStructures(FILE *Stream,UDWORD GameType,int Exclude,int Include)
{
	STRUCT_SAVEHEADER	Header;
	SAVE_STRUCTURE		SaveStructure;

	Header.aFileType[0] = 's';
	Header.aFileType[1] = 't';
	Header.aFileType[2] = 'r';
	Header.aFileType[3] = 'u';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = CountObjectsOfType(IMD_STRUCTURE,Exclude,Include);

	fwrite(&Header,STRUCT_HEADER_SIZE,1,Stream);

	DebugPrint("Exporting structures\n");
	DebugPrint("Exclude %d Include %d\n",Exclude,Include);
	DebugPrint("Quantity %d\n",Header.quantity);

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		C3DObject *Object = &m_3DObjects[curNode->ObjectID];

		if (Object->TypeID == IMD_STRUCTURE)
		{
			if (IncludeIt(curNode->Position.x, curNode->Position.z, Exclude, Include))
			{
				_structure_stats *Struct = &m_Structures[Object->StructureID];

				SaveStructure.id = curNode->UniqueID;

				SaveStructure.x = static_cast<UDWORD>(curNode->Position.x + (m_MapWidth * m_TileWidth / 2));
				SaveStructure.y = static_cast<UDWORD>(-curNode->Position.z + (m_MapHeight * m_TileHeight / 2));
				SaveStructure.z = static_cast<UDWORD>(curNode->Position.y);

				SaveStructure.direction = static_cast<UDWORD>(curNode->Rotation.y) % 360;
				SaveStructure.player = curNode->PlayerID;
				SaveStructure.inFire = 0;
				SaveStructure.burnStart = 0;
				SaveStructure.burnDamage = 0;
				strcpy((char*)SaveStructure.name,m_Structures[Object->StructureID].StructureName);
				SaveStructure.status = SS_BUILT;
				SaveStructure.currentBuildPts = 0;
				SaveStructure.body = Struct->bodyPoints;
				SaveStructure.armour = Struct->armourValue;
				SaveStructure.resistance = Struct->resistance;
				SaveStructure.repair = Struct->repairSystem;
				SaveStructure.subjectInc = -1;
				SaveStructure.timeStarted = 0;

				if( (strcmp(Struct->Type,"RESEARCH")==0) || (strcmp(Struct->Type,"RESEARCH MODULE")==0) ) {
					SaveStructure.output = 4;	// Assumed values. Must change.
					SaveStructure.capacity = 0;
					SaveStructure.quantity = 0;
				} else if( (strcmp(Struct->Type,"FACTORY")==0) || (strcmp(Struct->Type,"FACTORY MODULE")==0) ) {
					SaveStructure.output = 20;	// Assumed values. Must change.
					SaveStructure.capacity = 0;
					SaveStructure.quantity = 0;
				} else {
					SaveStructure.output = 0;
					SaveStructure.capacity = 0;
					SaveStructure.quantity = 0;
				}

				DebugPrint("Included Structure : %s %d %d ( %d %d )\n",
							SaveStructure.name,SaveStructure.id,SaveStructure.player,
							SaveStructure.x,SaveStructure.y);

				fwrite(&SaveStructure,sizeof(SaveStructure),1,Stream);
			}
		}
	}

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceDroidInit(FILE *Stream,UDWORD GameType,int Exclude,int Include)
{
	DROIDINIT_SAVEHEADER	Header;
	SAVE_DROIDINIT			Droid;

	Header.aFileType[0] = 'd';
	Header.aFileType[1] = 'i';
	Header.aFileType[2] = 'n';
	Header.aFileType[3] = 't';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = CountObjectsOfType(IMD_DROID,Exclude,Include);

	fwrite(&Header,DROIDINIT_HEADER_SIZE,1,Stream);

	DebugPrint("Exporting droids\n");
	DebugPrint("Exclude %d Include %d\n",Exclude,Include);
	DebugPrint("Quantity %d\n",Header.quantity);
	
	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		C3DObject *Object = &m_3DObjects[curNode->ObjectID];

		if (Object->TypeID == IMD_DROID)
		{
			if (IncludeIt(curNode->Position.x, curNode->Position.z, Exclude, Include))
			{
				memset(&Droid, 0, sizeof(Droid));

				if (Object->Description)
				{
					strcpy((char*)Droid.name, Object->Description);
				}
				else
				{
					strcpy((char*)Droid.name, Object->Name);
				}


				Droid.id = curNode->UniqueID;
				Droid.player = curNode->PlayerID;
				Droid.x = static_cast<UDWORD>(curNode->Position.x + (m_MapWidth * m_TileWidth / 2));
				Droid.y = static_cast<UDWORD>(-curNode->Position.z + (m_MapHeight * m_TileHeight / 2));
				Droid.z = static_cast<UDWORD>(curNode->Position.y);
				Droid.direction = static_cast<UDWORD>(curNode->Rotation.y) % 360;

				DebugPrint("Included Droid : %s %d %d ( %d %d )\n",
				           Droid.name, curNode->UniqueID, curNode->PlayerID, Droid.x, Droid.y);

				fwrite(&Droid, sizeof(Droid), 1, Stream);
			}
		}
	}

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceDroids(FILE *Stream)
{
	DROID_SAVEHEADER	Header;

	Header.aFileType[0] = 'd';
	Header.aFileType[1] = 'r';
	Header.aFileType[2] = 'o';
	Header.aFileType[3] = 'd';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = 0;	//CountObjectsOfType(IMD_DROID);

	fwrite(&Header,DROID_HEADER_SIZE,1,Stream);

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceTemplates(FILE *Stream)
{
	TEMPLATE_SAVEHEADER	Header;

	Header.aFileType[0] = 't';
	Header.aFileType[1] = 'e';
	Header.aFileType[2] = 'm';
	Header.aFileType[3] = 'p';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = 0;

	fwrite(&Header,TEMPLATE_HEADER_SIZE,1,Stream);

	return TRUE;
}



#if 0
BOOL CHeightMap::WriteNecromancerMap(FILE *Stream)
{
	return WriteDeliveranceMap(Stream);
}
#endif


#if 0
BOOL CHeightMap::WriteNecromancerObjects(FILE *Stream)
{
	NOB_SAVEHEADER Header;

	Header.aFileType[0] = 'n';
	Header.aFileType[1] = 'o';
	Header.aFileType[2] = 'b';
	Header.aFileType[3] = 'f';
	Header.version = 1;
	Header.quantity = CountObjectsOfType(IMD_OBJECT,-1,-1);

	fwrite(&Header,FEATURE_HEADER_SIZE,1,Stream);

	for (std::list<C3DObjectInstance>::const_iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		const C3DObject& Object = m_3DObjects[curNode->ObjectID];

		if(Object.TypeID == IMD_OBJECT)
		{
			NOB_ENTRY Nob;

			Nob.id = curNode->ObjectID;
			Nob.XPos = (SDWORD)curNode->Position.x;
			Nob.YPos = (SDWORD)curNode->Position.y;
			Nob.ZPos = (SDWORD)curNode->Position.z;
			Nob.XRot = (SDWORD)curNode->Rotation.x;
			Nob.YRot = (SDWORD)curNode->Rotation.y;
			Nob.ZRot = (SDWORD)curNode->Rotation.z;
			fwrite(&Nob, sizeof(Nob), 1, Stream);
		}
	}

	return TRUE;
}
#endif


void CHeightMap::SetTileHeightUndo(int Index,float Height)
{
// Set the tiles vertex heights.
	SetVertexHeight(Index,0,Height);
	SetVertexHeight(Index,1,Height);
	SetVertexHeight(Index,2,Height);
	SetVertexHeight(Index,3,Height);

// Now make the ones adjoining it match.
	int x = Index%m_MapWidth;
	int y = Index/m_MapWidth;

	g_UndoRedo->AddUndo(&(GetMapTiles()[Index]));

	if(x > 0) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index-1]));
		SetVertexHeight(Index-1,1,Height);
		SetVertexHeight(Index-1,2,Height);
	}

	if((x > 0) && (y > 0)) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index-m_MapWidth-1]));
		SetVertexHeight(Index-m_MapWidth-1,2,Height);
	}

	if(y > 0) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index-m_MapWidth]));
		SetVertexHeight(Index-m_MapWidth,2,Height);
		SetVertexHeight(Index-m_MapWidth,3,Height);
	}

	if((y > 0) && (x < m_MapWidth-1)) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index-m_MapWidth+1]));
		SetVertexHeight(Index-m_MapWidth+1,3,Height);
	}

	if(x < m_MapWidth-1) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index+1]));
		SetVertexHeight(Index+1,3,Height);
		SetVertexHeight(Index+1,0,Height);
	}

	if((y < m_MapHeight-1) && (x < m_MapWidth-1)) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index+m_MapWidth+1]));
		SetVertexHeight(Index+m_MapWidth+1,0,Height);
	}

	if(y < m_MapHeight-1) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index+m_MapWidth]));
		SetVertexHeight(Index+m_MapWidth,0,Height);
		SetVertexHeight(Index+m_MapWidth,1,Height);
	}

	if((y < m_MapHeight-1) && (x > 0)) {
		g_UndoRedo->AddUndo(&(GetMapTiles()[Index+m_MapWidth-1]));
		SetVertexHeight(Index+m_MapWidth-1,1,Height);
	}
}


void CHeightMap::InitialiseScrollLimits()
{
	m_NumScrollLimits = 0;
	m_ScrollLimits.clear();
}


void CHeightMap::AddScrollLimit(int MinX, int MinZ, int MaxX, int MaxZ, const char* ScriptName)
{
	AddScrollLimit(MinX, MinZ, MaxX, MaxZ, m_NewObjectID, ScriptName);
	++m_NewObjectID;
}


void CHeightMap::AddScrollLimit(int MinX, int MinZ, int MaxX, int MaxZ, DWORD UniqueID, const char* ScriptName)
{
	CScrollLimits Data;
   	Data.UniqueID = UniqueID;
   	Data.MinX = MinX;
   	Data.MinZ = MinZ;
   	Data.MaxX = MaxX;
   	Data.MaxZ = MaxZ;
   	strcpy(Data.ScriptName, ScriptName);

	m_ScrollLimits.push_back(Data);

	++m_NumScrollLimits;
}


void CHeightMap::SetScrollLimit(int Index, int MinX, int MinZ, int MaxX, int MaxZ, const char* ScriptName)
{
	if (Index >= m_ScrollLimits.size())
		return;

	std::list<CScrollLimits>::iterator limits = m_ScrollLimits.begin();
	std::advance(limits, Index);

	assert(limits != m_ScrollLimits.end());

	limits->MinX = MinX;
	limits->MinZ = MinZ;
	limits->MaxX = MaxX;
	limits->MaxZ = MaxZ;

	strcpy(limits->ScriptName, ScriptName);
}


void CHeightMap::DeleteAllScrollLimits()
{
	m_ScrollLimits.clear();
	m_NumScrollLimits = 0;
}


unsigned int CHeightMap::FindScrollLimit(unsigned int UniqueID)
{
	unsigned int Index = 0;

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		if(curNode->UniqueID == UniqueID)
			return Index;

		++Index;
	}

	return 0;
}


unsigned int CHeightMap::FindScrollLimit(const char* ScriptName)
{
	unsigned int Index = 0;

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		if(_stricmp(curNode->ScriptName, ScriptName) == 0)
			return Index;

		++Index;
	}

	return 0;
}


void CHeightMap::DeleteScrollLimit(unsigned int Index)
{
	if (Index >= m_ScrollLimits.size())
		return;

	std::list<CScrollLimits>::iterator toRemove = m_ScrollLimits.begin();
	std::advance(toRemove, Index);
	m_ScrollLimits.erase(toRemove);
	--m_NumScrollLimits;
}


void CHeightMap::DrawScrollLimits(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
// Initialise world matrix with camera rotation.
	m_DirectMaths->SetWorldMatrix(&CameraRotation);
// Initialise object matrix with object rotation and position.
	m_DirectMaths->SetObjectMatrix(&ZeroVector,&ZeroVector,&CameraPosition);
	m_DirectMaths->SetTransformation();

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		float MinX = static_cast<float>(curNode->MinX - m_MapWidth/2) * m_TileWidth;
		float MaxX = static_cast<float>(curNode->MaxX - m_MapWidth/2) * m_TileWidth;
		float MinZ = static_cast<float>(curNode->MinZ - m_MapHeight/2) * m_TileHeight;
		float MaxZ = static_cast<float>(curNode->MaxZ - m_MapHeight/2) * m_TileHeight;
		
		D3DVERTEX Vertex[4];
		Vertex[0].x = MinX;	Vertex[0].y = (float)128;	Vertex[0].z = MinZ;
		Vertex[0].nx = (float)0;	Vertex[0].ny = (float)1;	Vertex[0].nz = (float)0;
		Vertex[0].tu = Vertex[0].tv = (float)0;
		Vertex[1] = Vertex[2] = Vertex[3] = Vertex[0];
		Vertex[3].x = MaxX;
		Vertex[2].x = MaxX; Vertex[2].z = MaxZ;
		Vertex[1].z = MaxZ;
		m_MatManager->DrawTriangleFan(m_SeaMaterial,Vertex,4);
	}
}


BOOL CHeightMap::CheckLimitsWithin(int ExcludeIndex, int IncludeIndex)
{
	// HACK: Freakin MSVC's STL implemenation lacks std::max!! so this is a workaround
	//assert(std::max(ExcludeIndex, IncludeIndex) < m_ScrollLimits.size());
	assert((ExcludeIndex < IncludeIndex ? IncludeIndex : ExcludeIndex) < m_ScrollLimits.size());

	std::list<CScrollLimits>::const_iterator Exclude = m_ScrollLimits.begin();
	std::advance(Exclude, ExcludeIndex);

	std::list<CScrollLimits>::const_iterator Include = m_ScrollLimits.begin();
	std::advance(Include, IncludeIndex);

	if (Exclude->MinX < Include->MinX
	 || Exclude->MaxX > Include->MaxX
	 || Exclude->MinZ < Include->MinZ
	 || Exclude->MaxZ > Include->MaxZ)
		return FALSE;

	return TRUE;
}


BOOL CHeightMap::CheckUniqueLimitsScriptNames()
{
	unsigned int NumIDs = 0;
	char* ScriptNames = new char[m_NumScrollLimits * MAX_SCRIPTNAME];
	unsigned int NumDups = 0;

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		for (unsigned int i = 0; i < NumIDs; ++i)
		{
			if(curNode->ScriptName[0])
			{
				if(_stricmp(curNode->ScriptName, &ScriptNames[i * MAX_SCRIPTNAME]) == 0)
					++NumDups;
			}
		}

		if(curNode->ScriptName[0])
		{
			strcpy(&ScriptNames[NumIDs*MAX_SCRIPTNAME], curNode->ScriptName);
		}

		++NumIDs;
	}

	delete [] ScriptNames;

	if (NumDups)
		return FALSE;

	return TRUE;
}


BOOL CHeightMap::WriteScrollLimits(FILE *Stream,int StartX,int StartY,int Width,int Height)
{
	fprintf(Stream,"ScrollLimits {\n");
	fprintf(Stream,"    Version %d\n",CURRENT_SCROLLLIMITS_VERSION);
	fprintf(Stream,"    NumLimits %d\n",m_NumScrollLimits);
	fprintf(Stream,"    Limits {\n");

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		fprintf(Stream, "        \"%s\" %d %d %d %d %d\n",
		        curNode->ScriptName, curNode->UniqueID, curNode->MinX, curNode->MinZ, curNode->MaxX, curNode->MaxZ);
	}

	fprintf(Stream, "    }\n");
	fprintf(Stream, "}\n");

	return TRUE;
}


BOOL CHeightMap::ReadScrollLimits(FILE *Stream)
{
	LONG Version;
	LONG NumLimits;
	LONG UniqueID;
	LONG MinX;
	LONG MinZ;
	LONG MaxX;
	LONG MaxZ;
	char ScriptName[MAX_SCRIPTNAME];

	fpos_t pos;

	fgetpos( Stream, &pos );
		  
	if(!StartChunk(Stream,"ScrollLimits")) {
		fsetpos( Stream, &pos );
		return TRUE;
	}
	CHECKTRUE(ReadLong(Stream,"Version",(LONG*)&Version));
	CHECKTRUE(ReadLong(Stream,"NumLimits",(LONG*)&NumLimits));
	CHECKTRUE(StartChunk(Stream,"Limits"));

	for(int i=0; i<NumLimits; i++) {
		CHECKTRUE(ReadQuotedString(Stream,NULL,ScriptName));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&UniqueID));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&MinX));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&MinZ));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&MaxX));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&MaxZ));

		AddScrollLimit(MinX,MinZ,MaxX,MaxZ,UniqueID,ScriptName);
	}

	CHECKTRUE(EndChunk(Stream));
	CHECKTRUE(EndChunk(Stream));

	return TRUE;
}


BOOL CHeightMap::WriteDeliveranceLimits(FILE *Stream)
{
#if 0
	LIMITS_SAVEHEADER Header;

	Header.aFileType[0] = 'l';
	Header.aFileType[1] = 'm';
	Header.aFileType[2] = 't';
	Header.aFileType[3] = 's';
	Header.version = CURRENT_GAME_VERSION_NUM;
	Header.quantity = m_NumScrollLimits;

	if (fwrite(&Header, LIMITS_SAVEHEADER_SIZE, 1, Stream) != 1)
		return FALSE;

	for (std::list<CScrollLimits>::const_iterator curNode = m_ScrollLimits.begin(); curNode != m_ScrollLimits.end(); ++curNode)
	{
		if(curNode->ScriptName[0])
		{
			LIMITS Limit;

			Limit.LimitID = curNode->UniqueID;
			Limit.MinX = curNode->MinX;
			Limit.MinZ = curNode->MinZ;
			Limit.MaxX = curNode->MaxX;
			Limit.MaxZ = curNode->MaxZ;
			fwrite(&Limit, sizeof(Limit), 1, Stream);
		}
	}
#endif

	return TRUE;
}


// Initialise gateway data.
//
void CHeightMap::InitialiseGateways()
{
	m_NumGateways = 0;
	m_Gateways.clear();
}


// Add a gateway.
//
int CHeightMap::AddGateway(int x0,int y0,int x1,int y1)
{
	int dx = x1-x0;
	int dy = y1-y0;

	// Check for vertical or horizontal
	if((dx != 0) && (dy != 0)) {
		// not the case..
		if(abs(dx) > abs(dy)) {	// More horizontal than vertical..
			y1 = y0; // make it horizontal,
		} else {
			x1 = x0; // otherwise make it vertical.
		}
	}

	if(x0 > x1) {
		int tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	
	if(y0 > y1) {
		int tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	GateWay Data;
	Data.Selected = FALSE;
   	Data.Flags = GATEF_GROUND;
   	Data.x0 = x0;
   	Data.y0 = y0;
   	Data.x1 = x1;
   	Data.y1 = y1;

	m_Gateways.push_back(Data);

	DebugPrint("Gateway added : %d %d %d %d\n",x0,y0,x1,y1);

	++m_NumGateways;

	return m_NumGateways-1;
}


// Remove and de-allocate a gateway.
//
void CHeightMap::DeleteGateway(int Index)
{
	if (m_Gateways.empty()
	 || m_Gateways.size() <= Index)
		return;

	std::list<GateWay>::iterator toRemove = m_Gateways.begin();
	std::advance(toRemove, Index);
	m_Gateways.erase(toRemove);
	--m_NumGateways;
}


// Remove and de-allocate all gateways.
//
void CHeightMap::DeleteAllGateways()
{
	m_Gateways.clear();
	m_NumGateways = 0;
}


// De-select all gateways.
//
void CHeightMap::DeSelectGateways()
{
	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		curGateway->Selected = FALSE;
	}
}


// Select a gateway by index.
//
void CHeightMap::SelectGateway(int Index)
{
	DeSelectGateways();

	if (Index >= m_Gateways.size())
		return;

	std::list<GateWay>::iterator toSelect = m_Gateways.begin();
	std::advance(toSelect, Index);

	toSelect->Selected = TRUE;
}


BOOL CHeightMap::GetGateway(int Index, int& x0, int& y0, int& x1, int& y1)
{
	if (Index >= m_Gateways.size())
		return FALSE;

	std::list<GateWay>::iterator curGateway = m_Gateways.begin();
	std::advance(curGateway, Index);

	x0 = curGateway->x0;
	y0 = curGateway->y0;
	x1 = curGateway->x1;
	y1 = curGateway->y1;

	return TRUE;
}


// Find a gateway given a map tile xy coord.
//
int CHeightMap::FindGateway(int x,int y)
{
	unsigned int i = 0;

	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		if (x >= curGateway->x0
		 && x <= curGateway->x1
		 && y >= curGateway->y0
		 && y <= curGateway->y1)
			return i;

		++i;
	}

	return -1;
}


void CHeightMap::SetGateway(int Index, int x0, int y0, int x1, int y1)
{
	if (Index >= m_Gateways.size())
		return;

	std::list<GateWay>::iterator curGateway = m_Gateways.begin();
	std::advance(curGateway, Index);

	int dx = x1-x0;
	int dy = y1-y0;

	// Check for vertical or horizontal
	if (dx != 0
	 && dy != 0)
	{
		// not the case..
		if (abs(dx) > abs(dy)) // More horizontal than vertical..
		{
			y1 = y0; // make it horizontal,
		}
		else
		{
			x1 = x0; // otherwise make it vertical.
		}
	}

	if(x0 > x1)
		std::swap(x0, x1);
	
	if(y0 > y1)
		std::swap(y0, y1);

	curGateway->x0 = x0;
	curGateway->y0 = y0;
	curGateway->x1 = x1;
	curGateway->y1 = y1;
}



// Return TRUE if tile is a blocking tile.
//
BOOL CHeightMap::TileIsBlockingLand(int x,int y)
{
	int	Type = GetTileType(x,y);

	return ((Type == TF_TYPEWATER) || (Type == TF_TYPECLIFFFACE));
}


// Return TRUE if tile is a blocking tile.
//
BOOL CHeightMap::TileIsBlockingWater(int x,int y)
{
	int	Type = GetTileType(x,y);

	return (Type != TF_TYPEWATER);
}


// Return TRUE if tile is a blocking tile.
//
BOOL CHeightMap::TileIsBlocking(int x,int y,int Water)
{
	if(Water) {
		return TileIsBlockingWater(x,y);
	}

	return TileIsBlockingLand(x,y);
}


// Check no blocking tiles in gateway span.
//
BOOL CHeightMap::CheckGatewayBlockingTiles(int Index)
{
	if (Index >= m_Gateways.size())
		return TRUE;

	std::list<GateWay>::const_iterator curGateway = m_Gateways.begin();
	std::advance(curGateway, Index);


	// If the 1st tile in the gateway is water then assume were doing
	// a gateway over water.
	bool Water = GetTileType(curGateway->x0, curGateway->y0) == TF_TYPEWATER;

	if (curGateway->x0 == curGateway->x1) // Vertical
	{
		// Now check tiles to left & right are not blocking and tiles
		// along span are not blocking.
		for (int y = curGateway->y0; y <= curGateway->y1; ++y)
		{
			if (TileIsBlocking(curGateway->x0, y, Water))
				return FALSE;
		}

	}
	else // Horizontal.
	{
		// Now check tiles above and below are not blocking and tiles
		// along span are not blocking.
		for (int x = curGateway->x0; x <= curGateway->x1; ++x)
		{
			if (TileIsBlocking(x, curGateway->y0, Water))
				return FALSE;
		}

	}

	return TRUE;
}


// Check a gateway dos'nt overlap any others.
//
BOOL CHeightMap::CheckGatewayOverlap(int CurIndex,int x0,int y0,int x1,int y1)
{
	int dx = x1-x0;
	int dy = y1-y0;

	// Check for vertical or horizontal
	if((dx != 0) && (dy != 0)) {
		// not the case..
		if(abs(dx) > abs(dy)) {	// More horizontal than vertical..
			y1 = y0; // make it horizontal,
		} else {
			x1 = x0; // otherwise make it vertical.
		}
	}

	if(x0 > x1) {
		int tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	
	if(y0 > y1) {
		int tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	BOOL Vertical = FALSE;

	if(x0 == x1) {
		Vertical = TRUE;
	}


	unsigned int Index = 0;

	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		if (Index != CurIndex)
		{
			if(Vertical)
			{
				if(curGateway->x0 == curGateway->x1) // Vertical against vertical.
				{
					if(x0 == curGateway->x0)
					{
						if ((y0 >= curGateway->y0
						  && y0 <= curGateway->y1)
						 ||	(y1 >= curGateway->y0
						  && y1 <= curGateway->y1)
						 || (curGateway->y0 >= y0
						  && curGateway->y0 <= y1)
						 || (curGateway->y1 >= y0
						  && curGateway->y1 <= y1))
							return FALSE;
					}
				}
				else // Vertical against horizontal.
				{
					if (x0 >= curGateway->x0
					 && x0 <= curGateway->x1
					 && y0 <= curGateway->y0
					 && y1 >= curGateway->y0)
						return FALSE;
				}
			}
			else
			{
				if (curGateway->x0 == curGateway->x1)
				{	// Horizontal against vertical.
					if (y0 >= curGateway->y0
					 && y0 <= curGateway->y1
					 && x0 <= curGateway->x0
					 && x1 >= curGateway->x0)
						return FALSE;
				}
				else
				{	// Horizontal against horizontal.
					if (y0 == curGateway->y0)
					{
						if ((x0 >= curGateway->x0
						  && x0 <= curGateway->x1)
						 || (x1 >= curGateway->x0
						  && x1 <= curGateway->x1)
						 || (curGateway->x0 >= x0
						  && curGateway->x0 <= x1)
						 || (curGateway->x1 >= x0
						  && curGateway->x1 <= x1))
							return FALSE;
					}
				}
			}
		}

		++Index;
	}

	return TRUE;
}


// Write gateways to the project file.
//
BOOL CHeightMap::WriteGateways(FILE* Stream, int StartX, int StartY, int Width, int Height)
{
	fprintf(Stream,"Gateways {\n");
	fprintf(Stream,"    Version %d\n",CURRENT_GATEWAY_VERSION);
	fprintf(Stream,"    NumGateways %d\n",m_NumGateways);
	fprintf(Stream,"    Gates {\n");

	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		fprintf(Stream, "        %d %d %d %d\n", curGateway->x0, curGateway->y0, curGateway->x1, curGateway->y1);
	}

	fprintf(Stream, "    }\n");
	fprintf(Stream, "}\n");

	return TRUE;
}


// Read gateways from the project file.
//
BOOL CHeightMap::ReadGateways(FILE *Stream)
{
	LONG Version;
	LONG NumGateways;
	LONG x0,y0;
	LONG x1,y1;

	if(!StartChunk(Stream,"Gateways")) {
		return FALSE;
	}
	CHECKTRUE(ReadLong(Stream,"Version",(LONG*)&Version));
	CHECKTRUE(ReadLong(Stream,"NumGateways",(LONG*)&NumGateways));
	CHECKTRUE(StartChunk(Stream,"Gates"));

	for(int i=0; i<NumGateways; i++) {
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&x0));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&y0));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&x1));
		CHECKTRUE(ReadLong(Stream,NULL,(LONG*)&y1));

		AddGateway(x0,y0,x1,y1);
	}

	CHECKTRUE(EndChunk(Stream));
	CHECKTRUE(EndChunk(Stream));

	return TRUE;
}


// Write gateway data.
//
BOOL CHeightMap::WriteDeliveranceGateways(FILE *Stream)
{
	fprintf(Stream, "%d\r\n", m_NumGateways);

	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		fprintf(Stream, "%d %d %d %d\r\n", curGateway->x0, curGateway->y0, curGateway->x1, curGateway->y1);
	}

	return TRUE;
}


// Render the gateways in 3d.
//
void CHeightMap::DisplayGateways3D(D3DVECTOR &CameraRotation,D3DVECTOR &CameraPosition)
{
}


// Render the gateways in 2d.
//
void CHeightMap::DisplayGateways2D(CDIBDraw *DIBDraw,int ScrollX, int ScrollY,RECT *Clip)
{
	ScrollX /= (int)m_TextureWidth;
	ScrollY /= (int)m_TextureHeight;

	HDC	dc=(HDC)DIBDraw->GetDIBDC();
	HPEN NormalPen = CreatePen(PS_SOLID,1,RGB(0,255,255));
	HPEN OldPen = (HPEN)SelectObject(dc,NormalPen);

	for (std::list<GateWay>::iterator curGateway = m_Gateways.begin(); curGateway != m_Gateways.end(); ++curGateway)
	{
		int cw = static_cast<int>(m_TextureWidth);
		int ch = static_cast<int>(m_TextureHeight);

		int x0 = (static_cast<int>(curGateway->x0) - ScrollX) * cw;
		int y0 = (static_cast<int>(curGateway->y0) - ScrollY) * ch;
		int x1 = (static_cast<int>(curGateway->x1) - ScrollX) * cw;
		int y1 = (static_cast<int>(curGateway->y1) - ScrollY) * ch;

		if (x0 == x1)
		{
			for (; y0 < y1 + ch; y0 += ch)
			{
				MoveToEx(dc, x0, y0, NULL);
				LineTo(dc, x0 + cw, y0);
				LineTo(dc, x0 + cw, y0 + ch);
				LineTo(dc, x0, y0 + ch);
				LineTo(dc, x0, y0);
				LineTo(dc, x0 + cw, y0 + ch);
				MoveToEx(dc, x0 + cw, y0, NULL);
				LineTo(dc, x0, y0 + ch);
			}
		}
		else
		{
			for (; x0 < x1 + cw; x0 += cw)
			{
				MoveToEx(dc, x0,y0,NULL);
				LineTo(dc, x0 + cw, y0);
				LineTo(dc, x0 + cw, y0 + ch);
				LineTo(dc, x0, y0 + ch);
				LineTo(dc, x0, y0);
				LineTo(dc, x0 + cw, y0 + ch);
				MoveToEx(dc, x0 + cw, y0, NULL);
				LineTo(dc, x0, y0 + ch);
			}
		}
	}

	SelectObject(dc, OldPen);
	DeleteObject(NormalPen);
}



void CHeightMap::CountObjects(int Exclude,int Include)
{
	for(unsigned int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_WorldInfo.NumStructures[i] = 0;
		m_WorldInfo.NumWalls[i] = 0;
		m_WorldInfo.NumDroids[i] = 0;
	}

	m_WorldInfo.NumFeatures = 0;
	m_WorldInfo.NumObjects = 0;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if(IncludeIt(curNode->Position.x, curNode->Position.z, Exclude, Include))
		{
			C3DObject* Object = &m_3DObjects[curNode->ObjectID];

			switch(Object->TypeID)
			{
				case IMD_STRUCTURE:
					if (strcmp(m_Structures[m_3DObjects[curNode->ObjectID].StructureID].Type,"WALL") == 0
					 || strcmp(m_Structures[m_3DObjects[curNode->ObjectID].StructureID].Type,"CORNER WALL") == 0)
					{
						++m_WorldInfo.NumWalls[curNode->PlayerID];
					}
					else
					{
						++m_WorldInfo.NumStructures[curNode->PlayerID];
					}
					break;

				case IMD_DROID:
					++m_WorldInfo.NumDroids[curNode->PlayerID];
					break;

				case IMD_FEATURE:
					++m_WorldInfo.NumFeatures;
					break;

				case IMD_OBJECT:
					++m_WorldInfo.NumObjects;
					break;
			}
		}
	}
}


void CHeightMap::XFlipObjects(int x0,int y0,int x1,int y1)
{
	float CenX,CenZ;

	x1 ++;
	y1 ++;

	CenX = (float)(x1-x0);
	CenX /=2;
	CenX += (float)x0;

	CenZ = (float)(y1-y0);
	CenZ /=2;
	CenZ += (float)y0;

	CenX *= m_TileWidth;
	CenX -= m_MapWidth*m_TileWidth/2;

	CenZ *= m_TileHeight;
	CenZ -= m_MapHeight*m_TileHeight/2;
	CenZ = -CenZ;

	x0 *= m_TileWidth;
	x0 -= m_MapWidth*m_TileWidth/2;

	x1 *= m_TileWidth;
	x1 -= m_MapWidth*m_TileWidth/2;

	y0 *= m_TileHeight;
	y0 -= m_MapHeight*m_TileHeight/2;
	y0 = -y0;

	y1 *= m_TileHeight;
	y1 -= m_MapHeight*m_TileHeight/2;
	y1 = -y1;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (curNode->Position.x >= x0
		 && curNode->Position.x <= x1
		 && curNode->Position.z >= y1
		 && curNode->Position.z <= y0)
		{
			curNode->Position.x -= CenX;
			curNode->Position.x = -curNode->Position.x;
			curNode->Position.x += CenX;
		}
	}
}


void CHeightMap::YFlipObjects(int x0,int y0,int x1,int y1)
{
	float CenX,CenZ;

	x1 ++;
	y1 ++;

	CenX = (float)(x1-x0);
	CenX /=2;
	CenX += (float)x0;

	CenZ = (float)(y1-y0);
	CenZ /=2;
	CenZ += (float)y0;

	CenX *= m_TileWidth;
	CenX -= m_MapWidth*m_TileWidth/2;

	CenZ *= m_TileHeight;
	CenZ -= m_MapHeight*m_TileHeight/2;
	CenZ = -CenZ;

	x0 *= m_TileWidth;
	x0 -= m_MapWidth*m_TileWidth/2;

	x1 *= m_TileWidth;
	x1 -= m_MapWidth*m_TileWidth/2;

	y0 *= m_TileHeight;
	y0 -= m_MapHeight*m_TileHeight/2;
	y0 = -y0;

	y1 *= m_TileHeight;
	y1 -= m_MapHeight*m_TileHeight/2;
	y1 = -y1;

	for (std::list<C3DObjectInstance>::iterator curNode = m_Objects.begin(); curNode != m_Objects.end(); ++curNode)
	{
		if (curNode->Position.x >= x0
		 && curNode->Position.x <= x1
		 && curNode->Position.z >= y1
		 && curNode->Position.z <= y0)
		{
			curNode->Position.z -= CenZ;
			curNode->Position.z = -curNode->Position.z;
			curNode->Position.z += CenZ;
		}
	}
}
