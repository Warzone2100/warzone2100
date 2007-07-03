#ifndef __TEXTSEL_INCLUDED__
#define __TEXTSEL_INCLUDED__

#include "directx.h"
#include "geometry.h"

//#include "DDView.h"
#include "ddimage.h"
#include "dibdraw.h"
#include "pcxhandler.h"
#include "bmphandler.h"
#include "heightmap.h"




struct CTexture2D {
	char Name[256];
	IDDSURFACE *Surface;
	DDImage* SpriteImage;
	DWORD Width;
	DWORD Height;
};

class CTextureSelector {
public:
	CTextureSelector(CDirectDraw *DirectDraw);
	~CTextureSelector(void);
	void DeleteData(void);
	BOOL Read(DWORD NumTextures,char **TextureList,DWORD TextureWidth,DWORD TextureHeight,
  				DWORD NumObjects,CHeightMap *HeightMap,BOOL InitTypes=TRUE);
	void CalcSize(HWND hWnd);
	void Draw(CDIBDraw *DIBDraw,int XOffset,int YOffset);
	void DrawTileAttribute(CDIBDraw *DIBDraw,int XPos,int YPos,DWORD Flags);
	void SetSelectedTexture(DWORD Selected);
	DWORD GetNumTiles(void);
	DWORD GetTextureFlags(DWORD Index);
	void SetTextureFlags(DWORD Index,DWORD Flags);
	DWORD GetTextureType(DWORD Index);
	void SetTextureType(DWORD Index,DWORD Type);
	void SetTextureType(DWORD XCoord,DWORD YCoord,DWORD Type);
	BOOL SelectTexture(DWORD XCoord,DWORD YCoord);
	DWORD GetWidth(HWND hWnd);
	DWORD GetHeight(HWND hWnd);
	DWORD GetNumTextures(void) { return m_NumTextures; }
	char *GetTextureName(DWORD TextureIndex) { return m_Texture[TextureIndex].Name; }
	RECT &GetSelectedRect(void) { return m_SelectedRect; }
	DWORD GetSelectedID(void) { return m_SelectedID; }
	DDImage **GetImages(void) { return m_SpriteImage; }
	DWORD GetNumImages(void) { return m_NumSprites; }

	BOOL GetTextureFlipX(DWORD Index);
	BOOL GetTextureFlipY(DWORD Index);
	DWORD GetTextureRotate(DWORD Index);
	BOOL GetTextureToggleFlipX(DWORD Index);
	BOOL GetTextureToggleFlipY(DWORD Index);
	BOOL GetTextureIncrementRotate(DWORD Index);

	void SetTextureFlip(DWORD Index,BOOL FlipX,BOOL FlipY);
	void SetTextureRotate(DWORD Index,DWORD Rotate);
	void SetTextureToggleFlip(DWORD Index,BOOL FlipX,BOOL FlipY);
	void SetTextureIncrementRotate(DWORD Index,BOOL Rotate);


	BOOL GetTextureRandomRotate(DWORD Index);
	BOOL GetTextureRandomFlipY(DWORD Index);
	BOOL GetTextureRandomFlipX(DWORD Index);
	void SetTextureRandomFlip(DWORD Index,BOOL FlipX,BOOL FlipY);
	void SetTextureRandomRotate(DWORD Index,BOOL Rotate);

	void ResetTextureFlags(void);

protected:
	int m_StructureBase;
	int m_NumStructures;
	CHeightMap* m_HeightMap;
	CDirectDraw *m_DirectDraw;
	DWORD m_Width;
	DWORD m_Height;
	DWORD m_ObjHeight;
	DWORD m_NumTextures;
	DWORD m_NumSprites;
	CTexture2D m_Texture[8192];	// Should be allocated
	DDImage* m_SpriteImage[MAXTILES];
	DWORD m_SpriteFlags[MAXTILES];
	CTexture2D m_Button;
	DDImage* m_ButtonImage[8];
	DWORD m_NumObjects;
	char **m_ObjectNames;
	CFont m_Font;
	DWORD m_TextureWidth;
	DWORD m_TextureHeight;
	RECT m_SelectedRect;
	DWORD m_SelectedID;
};

#endif
