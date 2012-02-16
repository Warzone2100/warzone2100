/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_DIRECTX_H__
#define __INCLUDED_DIRECTX_H__

// Define DX6 in the project properties to use new interfaces in DirectX 6,
// otherwise defaults to using old Direct X 5 interfaces.

#include <ddraw.h>
#include <d3d.h>
#include "typedefs.h"
#include "macros.h"

#define MAX_DD_DEVICES	8
#define MAX_DD_FORMATS	32
#define MAX_DD_MODES	64
#define MAX_DD_NAMESIZE	128
#define MAX_D3D_DEVICES	8
#define MAX_MATERIALS	128

//#define FOGGY		// Test fog.
//#define FILTER		// Test bilinear filtering.
//#define DITHER		// Test dither.
//#define SPECULAR	// Test specular.
//#define ANTIALIAS	// Test anti alias.
//#define DUMPCAPS	// Print entire device caps on startup.

#ifdef FOGGY
#define	SKYRED		96	// The default colour values for the background.
#define	SKYGREEN	96
#define	SKYBLUE		96
#else
#define	SKYRED		0	// The default colour values for the background.
#define	SKYGREEN	64 
#define	SKYBLUE		128
#endif

#ifdef DX6
typedef IDirectDrawSurface4	IDDSURFACE;
typedef IDirectDrawPalette	IDDPALETTE;
typedef IDirectDraw4		IDDRAW;
typedef	IDirectDrawClipper	IDDCLIPPER;
typedef	IDirect3D3			ID3D;
typedef	IDirect3DDevice3	ID3DDEVICE;
typedef	IDirect3DViewport3	ID3DVIEPORT;
typedef IDirect3DMaterial3	ID3DMATERIAL;
typedef IDirect3DTexture2	ID3DTEXTURE;
typedef DDSURFACEDESC2		SURFACEDESC;
typedef DDSCAPS2			SURFACECAPS;
#else
typedef IDirectDrawSurface	IDDSURFACE;
typedef IDirectDrawPalette	IDDPALETTE;
typedef IDirectDraw			IDDRAW;
typedef	IDirectDrawClipper	IDDCLIPPER;
typedef	IDirect3D2			ID3D;
typedef	IDirect3DDevice2	ID3DDEVICE;
typedef	IDirect3DViewport2	ID3DVIEPORT;
typedef IDirect3DMaterial2	ID3DMATERIAL;
typedef IDirect3DTexture2	ID3DTEXTURE;
typedef DDSURFACEDESC		SURFACEDESC;
typedef DDSCAPS				SURFACECAPS;
#endif

// Colour models.
//
typedef enum {
	DONTCARE,
	RAMPCOLOR,
	RGBCOLOR,
} CMODEL;

typedef enum {
	SURFACE_SYSTEMMEMORY,
	SURFACE_VIDEOMEMORY,
} SURFACEMEMTYPE;

// Structure to define a requested system profile.
//
struct Profile {
	BOOL	FullScreen;
	DWORD	XRes;			// Resolution.
	DWORD	YRes;
	DWORD	BPP;			// Bits per pixel. ( Ignored in windowed mode. )
	BOOL	TripleBuffer;
	BOOL	UseHardware;	// 3D characteristics.
	BOOL	UseZBuffer;
	CMODEL	ColourModel;

	BOOL	Hardware;
	BOOL	DoesTexture;
	BOOL	DoesZBuffer;
	BOOL	DoesWindow;
};

// Structure to define the current state of the renderer.
//
struct D3DState {
	UWORD SourceBlend;
	UWORD DestBlend;
	UWORD BlendMode;
	BOOL BlendEnable;

	UWORD MagFilterMode;
	UWORD MinFilterMode;

	UWORD ShadeMode;
	UWORD FillMode;
	BOOL TextureCorrect;
	BOOL ZBuffer;
	BOOL AntiAlias;
	BOOL Dither;
	BOOL Specular;

	BOOL FogEnabled;
	UWORD FogMode;
	DWORD FogColour;
	D3DVALUE FogDensity;
	D3DVALUE FogTableDensity;
    D3DVALUE FogStart;
	D3DVALUE FogStop;

	DWORD AmbientLight;
};



// Structure to define a direct 3d device.
//
struct D3DDevice {
	GUID	Guid;							// The devices unique id.
	char	Name[MAX_DD_NAMESIZE];			// Name of the device.
	char	Description[MAX_DD_NAMESIZE];	// Textual description of the device.
	D3DDEVICEDESC	DeviceDesc;				// Full description of the device.
	BOOL	IsHardware;						// Is it a hardware device.
	BOOL	DoesTextures;					// Can it do texturing.
	BOOL	DoesZBuffer;					// Can it do z buffering.
	BOOL	CanDoWindow;					// Can it run in a window.
	DWORD	MaxBufferSize;					// Max execute buffer size.
	DWORD	MaxBufferVerts;					// Max number of verticies in an execute buffer.
};

// Structure to hold a list of direct 3d devices.
//
//
struct D3DDeviceList {
	int MaxDevices;
	int NumDevices;
	BOOL IsPrimary;		// Do these 3d devices live on the primary display device.
	DWORD WindowsBPP;
	D3DDevice List[MAX_D3D_DEVICES];
};

struct DisplayMode {
	int Width;
	int Height;
	int BPP;
};

struct ModeList {
	int MaxModes;
	int NumModes;
	DisplayMode Modes[MAX_DD_MODES];
};

// Structure to hold a list of direct draw device names and there associated
// direct 3D devices.
//
struct DeviceList {
	int MaxDevices;
	int NumDevices;
	char List[MAX_DD_DEVICES][MAX_DD_NAMESIZE];
	GUID *Guid[MAX_DD_DEVICES];
	BOOL IsPrimary[MAX_DD_DEVICES];
	ModeList DisplayModes[MAX_DD_DEVICES];
	D3DDeviceList D3DDevices[MAX_D3D_DEVICES];
};

// Structure to define a device hook name when
// calling direct draw enum device callback.
//
struct DeviceHook {
	char *Name;
	BOOL Accelerate3D;
	BOOL Windowed3D;
	IDDRAW	*Device;
};


// Texture formats.
//
typedef enum {
	TF_NOTEXTURE,
	TF_PAL4BIT,			// Palettized 4 bit.
	TF_PAL8BIT,			// Palettized 8 bit.
	TF_RGB16BIT,		// RGB.
	TF_RGB32BIT,		// RGB.
	TF_RGB1ALPHA16BIT,	// RGB and 1 bit of alpha.
	TF_RGB1ALPHA32BIT,	// RGB and 1 bit of alpha.
	TF_RGBALPHA16BIT,	// RGB and  2 or more bits of alpha.
	TF_RGBALPHA32BIT,	// RGB and  2 or more bits of alpha.
	TF_ANY,				// Not fussy, anything will do.
} TFORMAT;


#define AT_COLOURKEY	0x00000001
#define AT_ALPHABLEND	0x00000002

// Valid material types.
typedef enum {
	MT_FLAT,				// Un-textured.
	MT_TEXTURED,			// Textured.
	MT_MIPMAPED,			// Mip maped.
} MATERIALTYPE;


// Material creation descriptor.
struct MatDesc {
	DWORD Type;				// One of the MATERIALTYPE enumerated values.
	DWORD AlphaTypes;		// Specify the types of alpha we want.
	SDWORD ColourKey;		// RGB of key if AlphaTypes includes AT_COLOURKEY.
	char *Name;				// Unique name for the material.
	void *Bits;				// Source texture map if type is MT_TEXTURED.
	DWORD Count;			// Mip map count if Type is MT_MIPMAPED.
	void **MipMapBits;		// Source mip map if Type is MT_MIPMAPED.
	DWORD Width;			// Width of the texture if Type is MT_TEXTURED or MT_MIPMAPED
	DWORD Height;			// Height of the texture if Type is MT_TEXTURED or MT_MIPMAPED
	PALETTEENTRY *PaletteEntries;	// Source textures palette, also destination palette if
									// TexFormat is TF_PAL8BIT or TF_PAL4BIT.
	UBYTE Alpha;			// Global alpha value to use if AlphaTypes includes AT_ALPHABLEND.
	void *AlphaMap;			// Optional alpha map, replaces Alpha for per pixel alpha blending.
	TFORMAT TexFormat;		// One of the TFORMAT enumerated values.
	D3DCOLORVALUE Ambient;	// Material colours.
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	DWORD Scale;			// Texture scale factor ( not implemented )
};


struct PixFormatExt {
	char	Name[MAX_DD_NAMESIZE];
	BOOL	Palettized;		// TRUE if texture is palettized and pixels are pallete indecei..
	UWORD	IndexBPP;		// If Palettized==TRUE then indicates bits per pixel.

	UWORD	RGBBPP;			// If Palattized==FALSE then bits per pixel,
	UWORD	AlphaBPP;		// Bits per pixel for alpha,
	UWORD	RedBPP;			// red,
	UWORD	GreenBPP;		// Green,
	UWORD	BlueBPP;		// and Blue.
	DWORD	ABitMask;		// Bit masks for alpha,red,green and blue.
	DWORD	RBitMask;
	DWORD	GBitMask;
	DWORD	BBitMask;
	UWORD	AlphaShift;		// Shift values for alpha,red,green and blue.
	UWORD	RedShift;
	UWORD	GreenShift;
	UWORD	BlueShift;
};

// Structure to describe a texture format.
//
struct TextureDesc {
	DDPIXELFORMAT PixelFormat;
	PixFormatExt PixelFormatExt;
};

// Structure to hold a list of texture formats.
//
struct TextureList {
	int MaxFormats;
	int NumFormats;
	TextureDesc List[MAX_DD_FORMATS];
};


class CTexture {
	private:
		IDDSURFACE	*m_MemorySurface;  // system memory surface
		IDDSURFACE	*m_DeviceSurface;  // video memory texture
		IDDPALETTE	*m_Palette;
#ifdef DX6
		ID3DTEXTURE	*m_Texture;
#else
		D3DTEXTUREHANDLE	m_Handle;
#endif
		int m_Width,m_Height;

	public:
 		CTexture(void);
 		~CTexture(void);
	// 	BOOL Create(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device,
	//				void *Bits,DWORD Width,DWORD Height,
	//				PALETTEENTRY *PaletteEntries,
	//				TFORMAT TexFormat,
	//				UBYTE Alpha=255,void *AlphaMap=NULL,
	//				DWORD Scale=1);
		BOOL Create(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
							MatDesc *Desc);
	//	BOOL CreateMipMap(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
	//						DWORD Count,void **Bits,DWORD Width,DWORD Height,
	//						PALETTEENTRY *PaletteEntries,
	//						TFORMAT TexFormat,
	//						UBYTE Alpha,void *AlphaMap,
	//						DWORD Scale);
		BOOL CreateMipMap(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
							MatDesc *Desc);
#ifdef DX6
 		ID3DTEXTURE *GetTexture(void) { return m_Texture; }
#else
 		D3DTEXTUREHANDLE GetHandle(void) { return m_Handle; }
#endif
 		HRESULT Restore(void);
 		BOOL SetColourKey(SWORD ColourKeyIndex);
		int GetWidth(void) { return m_Width; }
		int GetHeight(void) { return m_Height; }
};


class CSurface {
	private:
		IDDSURFACE *m_Surface;
		SURFACEDESC m_ddsd;
		PALETTEENTRY *m_PaletteEntries;
		ULONG *m_RGBLookup;
		UWORD *m_RedLookup;
		UWORD *m_GreenLookup;
		UWORD *m_BlueLookup;
		DWORD m_BitCount;
		DWORD m_RBitMask,m_GBitMask,m_BBitMask;
		UWORD m_RedShift,m_GreenShift,m_BlueShift;
		UWORD m_RedBits,m_GreenBits,m_BlueBits;
		HDC m_hdc;
		BOOL m_IsLocked;
	public:
		CSurface(IDDSURFACE *Surface,PALETTEENTRY *PaletteEntries);
		~CSurface();
	//	void PutPixel(SLONG XCoord,SLONG YCoord,UWORD Index);
		UWORD GetValue(UBYTE Red,UBYTE Green,UBYTE Blue);
		void PutPixel(SLONG XCoord,SLONG YCoord,UWORD Value);
		void PutPixel(SLONG XCoord,SLONG YCoord,UBYTE Red,UBYTE Green,UBYTE Blue);
		void Line(SLONG X0,SLONG Y0,SLONG X1,SLONG Y1,UWORD Index);
		void Line(SLONG X0,SLONG Y0,SLONG X1,SLONG Y1,UBYTE Red,UBYTE Green,UBYTE Blue);
		void Clear(UWORD Index);
		void Clear(UBYTE Red,UBYTE Green,UBYTE Blue);
		void *Lock(void);
		void Unlock(void);
		HDC GetDC(void);
		void ReleaseDC(void);
};


class CMaterial {
	private:
		D3DMATERIALHANDLE	m_Handle;
		ID3DMATERIAL *m_Material;

	public:
		CMaterial(void);
		~CMaterial(void);
		BOOL Create(ID3D *Direct3D,ID3DDEVICE *Device,
					  D3DCOLORVALUE *Ambient=NULL,
					  D3DCOLORVALUE *Diffuse=NULL,
					  D3DCOLORVALUE *Specular=NULL,D3DVALUE Power=1.0F,
					  D3DCOLORVALUE *Emissive=NULL,
					  D3DTEXTUREHANDLE TexHandle = 0);
		D3DMATERIALHANDLE GetHandle(void) { return m_Handle; }
		BOOL GetMaterialColour(D3DCOLORVALUE *Ambient,D3DCOLORVALUE *Diffuse,D3DCOLORVALUE *Specular);
		BOOL SetMaterialColour(D3DCOLORVALUE *Ambient,D3DCOLORVALUE *Diffuse,D3DCOLORVALUE *Specular);
};



// The direct draw encapsulation class.
//
// Manages creation of the direct draw device, display buffers
// and the display palette. Also handles the direct 3d interface
// creation.
//
class CDirectDraw {
	private:
		HWND					m_hWnd;
		DWORD					m_WindowsBPP;
		DWORD				    m_WindowsXRes;
		DWORD				    m_WindowsYRes;
		UWORD					m_NumDevices;
		DeviceList				m_DeviceList;
		char                    m_DeviceName[MAX_DD_NAMESIZE];
		D3DDeviceList			m_3DDeviceList;
		int						m_3DDeviceIndex;
		IDDRAW					*m_Device;
		IDDSURFACE				*m_FrontBuffer;
		IDDSURFACE				*m_BackBuffer;
		IDDSURFACE				*m_ZBuffer;
		IDDCLIPPER				*m_Clipper;
		IDDPALETTE				*m_Palette;
		ID3D					*m_Direct3D;
		ID3DDEVICE		        *m_3DDevice;
		ID3DVIEPORT				*m_Viewport;
		RECT					m_rcWindow;
		DWORD					m_ZBufferDepth;
#ifdef DX6
		DDPIXELFORMAT			m_ddpfZBuffer;
		DDCAPS					m_HalCaps;
		DDCAPS					m_HelCaps;
#endif
		SIZE					m_ScreenSize;
		Profile					m_Profile;
		D3DDEVICEDESC			m_HALDevDesc;
		D3DDEVICEDESC			m_HELDevDesc;
		D3DDEVICEDESC			m_DevDesc;
		PixFormatExt			m_PixFormat;
		PixFormatExt			m_WindowsPixelFormat;
		D3DState				m_CurrentState;
		CMaterial				*m_BackMaterial;
		SWORD					m_InScene;
		D3DMATERIALHANDLE		m_LastMatHandle;
#ifdef DX6
		ID3DTEXTURE				*m_LastTexInterface;
#else
		D3DTEXTUREHANDLE		m_LastTexHandle;
#endif
		CSurface				*m_BackBufferSurface;
		CSurface				*m_FrontBufferSurface;
		BOOL					m_ClipState;

	public:
		  // Core methods.
		CDirectDraw(void);
		~CDirectDraw(void);
		BOOL Create(void *hWnd,char *szDevice,char *sz3DDevice,Profile *UserProfile = NULL);
		BOOL SetDefaultRenderState(void);
		BOOL SetRenderState(D3DState *State);
		BOOL SetMode(void);
		BOOL SetDisplayPalette(void);
		BOOL ClearBackBuffer(void);
		BOOL SwitchBuffers(void);
		BOOL RestoreBuffers(void);

		BOOL SetBackgroundMaterial(D3DMATERIALHANDLE Handle);

		BOOL SetFog(BOOL Enable,UBYTE Red,UBYTE Green,UBYTE Blue,ULONG Start,ULONG Stop);
		BOOL SetFilter(BOOL Filter);
		BOOL SetAmbient(UBYTE Red,UBYTE Green,UBYTE Blue);
		
		void *GetBackBufferDC(void);
		BOOL ReleaseBackBufferDC(void *hdc);

		BOOL BeginScene(void);
		BOOL EndScene(void);

		BOOL RestoreSurfaces(void);

		BOOL SetClipper(void *hWnd);
		void OnResize(void *hWnd);

		HRESULT SetRenderState(_D3DRENDERSTATETYPE StateReg,UWORD State) {	return m_3DDevice->SetRenderState(StateReg,State);	}
		HRESULT SetLightState(_D3DLIGHTSTATETYPE StateReg,UWORD State) { return m_3DDevice->SetLightState(StateReg,State);	}

//#ifdef DX6
//		void EnableColourKey(BOOL Enable) { m_3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, Enable); }
//#else
		void EnableBlend(BOOL Enable)  { m_3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, Enable); }
		void EnableColourKey(BOOL Enable) { m_3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE ,Enable); }
//#endif
		void EnableClip(BOOL Enable) { m_ClipState = Enable ? 0 : D3DDP_DONOTCLIP; }

#ifdef DX6
		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			  				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			   				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			  				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			   				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DLVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			  				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
			   				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
		   					D3DTLVERTEX *Vertex,DWORD Count);
#else
		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			  				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			   				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			  				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			   				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DLVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			  				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
			   				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
		   					D3DTLVERTEX *Vertex,DWORD Count);
#endif
		IDDSURFACE *CreateImageSurface(DWORD Width,DWORD Height,SURFACEMEMTYPE MemType=SURFACE_VIDEOMEMORY);
		IDDSURFACE *CreateImageSurfaceFromBitmap(void *Bits,DWORD Width,DWORD Height,
													PALETTEENTRY *PaletteEntries,SURFACEMEMTYPE MemType=SURFACE_VIDEOMEMORY);

	// Member access.
		HWND GetHWnd(void) { return m_hWnd; }
		UWORD GetNumDevices(void) { return m_NumDevices; }
		DeviceList *GetDeviceList(void) { return &m_DeviceList; }
		char *GetDeviceName(void) { return m_DeviceName; }
		IDDRAW *GetDevice(void) { return m_Device; }
		IDDSURFACE *GetFrontBuffer(void) { return m_FrontBuffer; }
		IDDSURFACE *GetBackBuffer(void) { return m_BackBuffer; }
		IDDPALETTE *GetDisplayPalette(void) { return m_Palette; }
		CSurface *GetBackBufferSurface(void) { return m_BackBufferSurface; }
		CSurface *GetFrontBufferSurface(void) { return m_FrontBufferSurface; }
		SIZE &GetScreenSize(void) { return m_ScreenSize; }
		BOOL GetDisplayPaletteEntries(PALETTEENTRY *PaletteEntries);
		BOOL SetDisplayPaletteEntries(PALETTEENTRY *PaletteEntries);
		DWORD GetBPP(void) { return m_Profile.BPP; }
		Profile &GetProfile(void) { return m_Profile; }

		ID3D *GetDirect3D(void) { return m_Direct3D; }
		ID3DDEVICE *Get3DDevice(void) { return m_3DDevice; }
		ID3DVIEPORT *GetViewport(void) { return m_Viewport; }

		void GetRes(DWORD *XRes,DWORD *YRes) { *XRes = m_ScreenSize.cx; *YRes = m_ScreenSize.cy; }
		void GetDisplayPixelFormat(PixFormatExt &PixFormat) { PixFormat = m_PixFormat; }

	private:
		BOOL GetWindowsMode(void);
		BOOL GetDisplayFormat(void);
		BOOL RestoreWindowsMode(void);
		BOOL Create3D(char *sz3DDevice);
		int MatchDevice(char *sz3DDevice);
		BOOL CleanUp(BOOL FreeAll = FALSE);
		BOOL CreateBuffers(void);
		BOOL CreateZBuffer(void);
		BOOL CreatePalette(void);
		BOOL CreateBackground(void);
		void Dump3DDeviceDesc(D3DDevice *Device);
		void DumpPrimCaps(D3DPRIMCAPS *Caps);
		BOOL Build3DDeviceList(void);
};


struct MaterialData {
	char *Name;
	int InUse;
	CTexture *Texture;
	CMaterial *Material;
#ifdef DX6
	ID3DTEXTURE *TexInterface;
#else
	D3DTEXTUREHANDLE	TexHandle;
#endif
	D3DMATERIALHANDLE	MatHandle;
};


class CMatManager {
	private:
		CDirectDraw *m_DirectDraw;
		DWORD m_NumMats;
		MaterialData m_Materials[MAX_MATERIALS];

	public:
		CMatManager(CDirectDraw *DirectDraw);
		~CMatManager(void);
		BOOL FindMaterial(char *Name,DWORD *MatID);
		DWORD CreateMaterial(MatDesc *Desc);
		int	GetMaterialWidth(DWORD MatID) {	return m_Materials[MatID].Texture->GetWidth(); }
		int	GetMaterialHeight(DWORD MatID) { return m_Materials[MatID].Texture->GetWidth(); }

	//	DWORD CreateMaterial(DWORD Type,char* Name,void *Bits,
	//											DWORD Count,void **MipMapBits,
	//											DWORD Width,DWORD Height,
	//											PALETTEENTRY *PaletteEntries,
	//											UBYTE Alpha,void *AlphaMap,
	//											TFORMAT TexFormat,
	//											D3DCOLORVALUE *Ambient,
	//											D3DCOLORVALUE *Diffuse,
	//											D3DCOLORVALUE *Specular,
	//											DWORD Scale=1);
		int GetMaterialID(char* Name);

		D3DMATERIALHANDLE GetMaterialHandle(DWORD MaterialID);
#ifdef DX6
		ID3DTEXTURE *GetTextureInterface(DWORD MaterialID);
#else
		D3DTEXTUREHANDLE GetTextureHandle(DWORD MaterialID);
#endif

		BOOL DeleteMaterial(DWORD MaterialID);
		BOOL SetColourKey(DWORD MaterialID,SWORD ColourKeyIndex);
		BOOL SetMaterialColour(DWORD MaterialID,
								D3DCOLORVALUE *Ambient,
								D3DCOLORVALUE *Diffuse,
								D3DCOLORVALUE *Specular);
		BOOL GetMaterialColour(DWORD MaterialID,
								D3DCOLORVALUE *Ambient,
								D3DCOLORVALUE *Diffuse,
								D3DCOLORVALUE *Specular);
		BOOL ReleaseMaterials(void);

		BOOL RestoreMaterials(void);

		BOOL BeginScene(void);
		BOOL EndScene(void);

		BOOL DrawPoint(int MaterialID,
	  				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(int MaterialID,
	   				D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(int MaterialID,
					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(int MaterialID,
					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(int MaterialID,
					D3DVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(int MaterialID,
					D3DVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(int MaterialID,
	  				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(int MaterialID,
	   				D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(int MaterialID,
					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(int MaterialID,
					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(int MaterialID,
					D3DLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(int MaterialID,
					D3DLVERTEX *Vertex,DWORD Count);

		BOOL DrawPoint(int MaterialID,
	  				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawLine(int MaterialID,
	   				D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawPolyLine(int MaterialID,
					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangle(int MaterialID,
					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleFan(int MaterialID,
					D3DTLVERTEX *Vertex,DWORD Count);
		BOOL DrawTriangleStrip(int MaterialID,
					D3DTLVERTEX *Vertex,DWORD Count);
};


	void DisplayError(HRESULT Code,LPCTSTR File,DWORD Line,...);

extern BOOL g_WindowsIs555;


#endif // __INCLUDED_DIRECTX_H__
