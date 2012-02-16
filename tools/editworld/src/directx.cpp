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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include "directx.h"
#include "geometry.h"
#include "debugprint.hpp"

//#define FOGGY
#define FILTER
//#define DUMPCAPS

/*

THINGS TO DO

 Make texture load handle rescaling properly.

 Texture caching.

 Mip maping.

 Multi textureing.

 Alpha blending useing material colour alpha.

 Addative transparency.

*/

// ---- Global definitions ---------------------------------------------------------------------------
//


BOOL CALLBACK BuildDeviceListCallback(GUID* lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam);
BOOL CALLBACK FindDeviceCallback(GUID* lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam);
HRESULT CALLBACK EnumModesCallback(SURFACEDESC* lpDDSurfaceDesc,LPVOID lpContext);
#ifdef DX6
HRESULT CALLBACK BuildTextureListCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lParam);
#else
HRESULT CALLBACK BuildTextureListCallback(SURFACEDESC *DeviceFmt, LPVOID lParam);
#endif
HRESULT CALLBACK EnumD3DDevicesCallback(LPGUID guid, 
										LPSTR lpDeviceDescription,
										LPSTR lpDeviceName,
										LPD3DDEVICEDESC lpD3DHWDeviceDesc,
										LPD3DDEVICEDESC lpD3DHELDeviceDesc,
										LPVOID lpUserArg);
#ifdef DX6
HRESULT CALLBACK EnumZBufferFormatsCallback( DDPIXELFORMAT* pddpf,void* pddpfDesired );
#endif

BOOL GetPixelFormatExt(DDPIXELFORMAT *ddpfPixelFormat,PixFormatExt *PixFormat);
BOOL FillSurface(void *Bits,DWORD Width,DWORD Height,
					PALETTEENTRY *PaletteEntries,
					DWORD ColourKey,
					UBYTE Alpha,void *AlphaMap,
					IDDSURFACE* Surface,
					PixFormatExt *PixFormat);

int MatchFormat(TFORMAT TextureFormat,DWORD AlphaTypes);
DWORD BPPToDDBD(int bpp);


// ---- Global variables -----------------------------------------------------------------------------
//

BOOL g_WindowsIs555;			// TRUE if Windows display uses 555 bits per pixel.
TextureList	g_TextureList;		// List of available texture formats.
BOOL g_IsPrimary;				// Using the primary display device?
DDCAPS g_HalCaps;
DDCAPS g_HelCaps;

// ---- CDirectDraw ----------------------------------------------------------------------------------
//

// Direct draw constructor.
//
// Just initialise member variables and build a list of devices.
//
CDirectDraw::CDirectDraw(void)
{
	m_Device = NULL;
	m_BackBuffer = NULL;
	m_FrontBuffer = NULL;
	m_ZBuffer = NULL;
	m_Clipper = NULL;
	m_Palette = NULL;

	m_Direct3D = NULL;
	m_3DDevice = NULL;
	m_Viewport = NULL;
	m_BackMaterial = NULL;
	m_BackBufferSurface = NULL;
	m_FrontBufferSurface = NULL;

	m_InScene = 0;
	m_LastMatHandle = 0;
#ifdef DX6
	m_LastTexInterface = NULL;
#else
	m_LastTexHandle = 0;
#endif

	m_ClipState = 0;

//	Build a list of avaible Direct Draw devices.
	
	m_DeviceList.MaxDevices = MAX_DD_DEVICES;
	m_DeviceList.NumDevices = 0;
    DirectDrawEnumerate(BuildDeviceListCallback, (LPVOID)&m_DeviceList);
}


CDirectDraw::~CDirectDraw(void)
{
	RestoreWindowsMode();
	CleanUp(TRUE);
}


BOOL CDirectDraw::CleanUp(BOOL FreeAll)
{
	RELEASE(m_3DDevice);
	RELEASE(m_Viewport);

    RELEASE(m_Palette);
    RELEASE(m_BackBuffer);
    RELEASE(m_FrontBuffer);
    RELEASE(m_ZBuffer);
	RELEASE(m_Clipper);

	DELOBJ(m_BackMaterial);
	DELOBJ(m_BackBufferSurface);
	DELOBJ(m_FrontBufferSurface);

	if(FreeAll) {
		RELEASE(m_Direct3D);
		RELEASE(m_Device);
	}

	return TRUE;
}


// Create the DirectDraw device and initialise display.
//
//BOOL CDirectDraw::Create(HWND hWnd,char *szDevice,char *sz3DDevice,Profile *UserProfile)
BOOL CDirectDraw::Create(void *hWnd,char *szDevice,char *sz3DDevice,Profile *UserProfile)
{
	HRESULT ddrval;
	DeviceHook DHook;

 	if(UserProfile) {
		m_Profile = *UserProfile;
	} else {
		m_Profile.FullScreen = TRUE;
		m_Profile.TripleBuffer = FALSE;
		m_Profile.XRes = 640;
		m_Profile.YRes = 480;
		m_Profile.BPP = 16;
		m_Profile.UseHardware = TRUE;
		m_Profile.UseZBuffer = TRUE;
		m_Profile.ColourModel = RGBCOLOR;
	}

	m_hWnd = (HWND)hWnd;

	DHook.Name = szDevice;
	DHook.Accelerate3D = UserProfile->UseHardware;
	DHook.Windowed3D = !UserProfile->FullScreen;
	DHook.Device = NULL;

	if((szDevice && szDevice[0]) || (DHook.Accelerate3D)) {
		DirectDrawEnumerate(FindDeviceCallback, (LPVOID)&DHook);
		m_Device = DHook.Device;
	}

    if(m_Device == NULL)
    {
#ifdef DX6
		IDirectDraw *pDD;
        ddrval = DirectDrawCreate(NULL, &pDD, NULL);
		// Query the DirectDraw driver for an IDirectDraw4 interface.
		pDD->QueryInterface( IID_IDirectDraw4, (void**)&m_Device);
		pDD->Release();

		m_Device->GetCaps(&g_HalCaps,&g_HelCaps);

#else
        ddrval = DirectDrawCreate(NULL, &m_Device, NULL);
#endif
		g_IsPrimary = TRUE;
    }

    if(m_Device == NULL)
    {
        MessageBox(NULL, "Requires DirectDraw", "Error", MB_OK);
        return FALSE;
    }

	if(m_Profile.FullScreen) {
		ddrval = m_Device->SetCooperativeLevel(m_hWnd,
									DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX);
	} else {
		ddrval = m_Device->SetCooperativeLevel(m_hWnd,DDSCL_NORMAL);
	}
	if(ddrval != DD_OK) {
		DisplayError(ddrval,__FILE__,__LINE__,0);
		return FALSE;
	}

#ifdef DX6
    ddrval = m_Device->QueryInterface(IID_IDirect3D3, (void**)&m_Direct3D);
#else
	ddrval = m_Device->QueryInterface(IID_IDirect3D2, (void**)&m_Direct3D);
#endif
	if(ddrval != DD_OK)
	{
		MessageBox(NULL, "Requires DirectX 5.0 or later", "Error", MB_OK);
		return FALSE;
	}

	if(!GetWindowsMode()) {
		return FALSE;
	}

    if(m_WindowsBPP < 16) {
		MessageBox(NULL, "This program requires the windows\ndisplay depth to be set to at least 16 bits.\nPlease alter the display properties",
					"Invalid Display Mode", MB_OK);
		return FALSE;
	}

	if(!SetMode()) {
		m_Profile.BPP = 8;
		if(!SetMode()) {
			return FALSE;
		}
	} 

	m_BackBufferSurface = new CSurface(m_BackBuffer,NULL);
	m_FrontBufferSurface = new CSurface(m_FrontBuffer,NULL);

	if(!GetDisplayFormat()) {
		return FALSE;
	}
	
	if(!Create3D(sz3DDevice)) {
		return FALSE;
	}

	if(!SetDefaultRenderState()) {
		return FALSE;
	}

	return TRUE;
}


BOOL CDirectDraw::GetWindowsMode(void)
{
    SURFACEDESC ddsd;
	HRESULT	ddrval;

    memset(&ddsd, 0, sizeof(SURFACEDESC));
    ddsd.dwSize = sizeof(SURFACEDESC);
    DXCALL(m_Device->GetDisplayMode(&ddsd));

    m_WindowsXRes = ddsd.dwWidth;
    m_WindowsYRes = ddsd.dwHeight;
    m_WindowsBPP = ddsd.ddpfPixelFormat.dwRGBBitCount;

	GetPixelFormatExt(&ddsd.ddpfPixelFormat,&m_WindowsPixelFormat);

	if( (m_WindowsPixelFormat.RedBPP == 5) &&
		(m_WindowsPixelFormat.GreenBPP == 5) &&
		(m_WindowsPixelFormat.BlueBPP == 5) ) {
		g_WindowsIs555 = TRUE;
	} else {
		g_WindowsIs555 = FALSE;
	}

	return TRUE;
}


BOOL CDirectDraw::GetDisplayFormat(void)
{
	HRESULT	ddrval;
    SURFACEDESC ddsd;

    memset(&ddsd, 0, sizeof(SURFACEDESC));
    ddsd.dwSize = sizeof(SURFACEDESC);
    DXCALL(m_BackBuffer->GetSurfaceDesc(&ddsd));

	GetPixelFormatExt(&ddsd.ddpfPixelFormat,&m_PixFormat);

	return TRUE;

//// Bit of a hack this.
//	PixelFormat Tmp={
//		{"RGB (5,5,5)"}, // Name
//	FALSE,		// TRUE if texture is palettized and pixels are palette indecei..
//	0,		// If Palettized==TRUE then indicates bits per pixel.
//
//	16,	//UWORD	RGBBPP;			// If Palattized==FALSE then bits per pixel,
//	0,	//UWORD	AlphaBPP;		// Bits per pixel for alpha,
//	5,	//UWORD	RedBPP;			// red,
//	5,	//UWORD	GreenBPP;		// Green,
//	5,	//UWORD	BlueBPP;		// and Blue.
//	0,	//DWORD	ABitMask;		// Bit masks for alpha,red,green and blue.
//	0x7c00,	//DWORD	RBitMask;
//	0x3e0,	//DWORD	GBitMask;
//	0x1f,	//DWORD	BBitMask;
//	0,	//UWORD	AlphaShift;		// Shift values for alpha,red,green and blue.
//	10,	//UWORD	RedShift;
//	5,	//UWORD	GreenShift;
//	0,	//UWORD	BlueShift;
//	};
//	m_PixFormat2 = Tmp;
}

BOOL CDirectDraw::RestoreWindowsMode(void)
{
	HRESULT	ddrval;

	if(m_Profile.FullScreen) {
#ifdef DX6
		DXCALL(m_Device->SetDisplayMode(m_WindowsXRes, m_WindowsYRes, m_WindowsBPP,0,0));
#else
		DXCALL(m_Device->SetDisplayMode(m_WindowsXRes, m_WindowsYRes, m_WindowsBPP));
#endif
	}

	return TRUE;
}

// Initialise the display.
//
BOOL CDirectDraw::SetMode(void)
{
    HRESULT ddrval;

	if(m_Profile.FullScreen) {
#ifdef DX6
		DXCALL(m_Device->SetDisplayMode(m_Profile.XRes, m_Profile.YRes, m_Profile.BPP,0,0));
#else
		DXCALL(m_Device->SetDisplayMode(m_Profile.XRes, m_Profile.YRes, m_Profile.BPP));
#endif
	}
    m_ScreenSize.cx = m_Profile.XRes;
    m_ScreenSize.cy = m_Profile.YRes;

    // get rid of any previous display surfaces and palette.
    CleanUp(FALSE);

	if(!CreateBuffers()) {
		return FALSE;
	}

	if(!CreatePalette()) {
		return FALSE;
	}

    return TRUE;
}


// Create the display buffers.
//
BOOL CDirectDraw::CreateBuffers(void)
{
    HRESULT ddrval;
    SURFACEDESC ddsd;

	if(m_Profile.FullScreen) {
		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
							  DDSCAPS_FLIP |
							  DDSCAPS_COMPLEX |
							  DDSCAPS_3DDEVICE |
							  DDSCAPS_VIDEOMEMORY;

		if(m_Profile.TripleBuffer) {
			ddsd.dwBackBufferCount = 2;
		} else {
			ddsd.dwBackBufferCount = 1;
		}

		// try to get a triple/double buffered video memory surface.
		ddrval = m_Device->CreateSurface(&ddsd, &m_FrontBuffer, NULL);

		// if that failed try to get a double buffered video memory surface.
		if (ddrval != DD_OK && ddsd.dwBackBufferCount == 2)
		{
		   ddsd.dwBackBufferCount = 1;
		   ddrval = m_Device->CreateSurface(&ddsd, &m_FrontBuffer, NULL);
		}

		if (ddrval != DD_OK)
		{
			// settle for a main memory surface.
			ddsd.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
			ddrval = m_Device->CreateSurface(&ddsd, &m_FrontBuffer, NULL);
		}

		if(ddrval != DD_OK) {
			DisplayError(ddrval,__FILE__,__LINE__,0);
			return FALSE;
		}

		// get a pointer to the back buffer
		SURFACECAPS caps;
		caps.dwCaps = DDSCAPS_BACKBUFFER;
		DXCALL(m_FrontBuffer->GetAttachedSurface(&caps, &m_BackBuffer));
	} else {
		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
							  DDSCAPS_3DDEVICE |
							  DDSCAPS_VIDEOMEMORY;
		DXCALL(m_Device->CreateSurface(&ddsd, &m_FrontBuffer, NULL));

		memset(&ddsd,0,sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
							  DDSCAPS_3DDEVICE;
	   	ddsd.dwWidth = m_Profile.XRes;
		ddsd.dwHeight = m_Profile.YRes;
		DXCALL(m_Device->CreateSurface(&ddsd, &m_BackBuffer, NULL));

		// Setup a clipper attached to the client window and the front buffer.
		DXCALL(m_Device->CreateClipper(0,&m_Clipper,NULL));

		if(!SetClipper(m_hWnd)) {
			return FALSE;
		}
	}

	return TRUE;
}


BOOL CDirectDraw::RestoreSurfaces(void)
{
	HRESULT ddrval;

	DebugPrint("Restoring display surfaces.\n");

	if(m_BackBuffer) DXCALL(m_BackBuffer->Restore());
	if(m_FrontBuffer) DXCALL(m_FrontBuffer->Restore());
	if(m_ZBuffer) DXCALL(m_ZBuffer->Restore());

	return TRUE;
}


BOOL CDirectDraw::SetClipper(void *hWnd)
{
	if(!m_Profile.FullScreen) {
		HRESULT ddrval;

		m_hWnd = (HWND)hWnd;
		DXCALL(m_Clipper->SetHWnd(0,(HWND)hWnd));
		DXCALL(m_FrontBuffer->SetClipper(m_Clipper));
	}

	return TRUE;
}


void CDirectDraw::OnResize(void *hWnd)
{
	if((!m_Profile.FullScreen) && (hWnd!=NULL)) {
// If were running in a window then we need to know where it is so we can do direct draw
// blits to it.
		m_hWnd = (HWND)hWnd;
		GetClientRect((HWND)hWnd, &m_rcWindow);
		ClientToScreen((HWND)hWnd, (LPPOINT)&m_rcWindow);
		ClientToScreen((HWND)hWnd, (LPPOINT)&m_rcWindow+1);
	}
}


// Create the Z buffer.
//
BOOL CDirectDraw::CreateZBuffer(void)
{
	if(m_Profile.UseZBuffer) {
#ifdef DX6
		SURFACEDESC ddsd;
		HRESULT	ddrval;

		memset(&ddsd, 0, sizeof(SURFACEDESC));
		ddsd.dwSize = sizeof(SURFACEDESC);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	    memcpy( &ddsd.ddpfPixelFormat, &m_ddpfZBuffer, sizeof(DDPIXELFORMAT) );
	    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.dwWidth = m_Profile.XRes;
		ddsd.dwHeight = m_Profile.YRes;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;

		if (m_Profile.Hardware) {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		} else {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		}

		m_ZBufferDepth = ddsd.ddpfPixelFormat.dwZBufferBitDepth;

		DXCALL(m_Device->CreateSurface(&ddsd, &m_ZBuffer, NULL));

	// Attach the Z buffer to the back surface.
		DXCALL(m_BackBuffer->AddAttachedSurface(m_ZBuffer));
#else
		SURFACEDESC ddsd;
		HRESULT	ddrval;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
		ddsd.dwWidth = m_Profile.XRes;
		ddsd.dwHeight = m_Profile.YRes;
		ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;

		if (m_Profile.Hardware) {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		} else {
			ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		}

		// Get the Z buffer bit depth from this driver's D3D device description
		DWORD devDepth = m_DevDesc.dwDeviceZBufferBitDepth;

		if (devDepth & DDBD_32)
			ddsd.dwZBufferBitDepth = 32;
		else if (devDepth & DDBD_24)
			ddsd.dwZBufferBitDepth = 24;
		else if (devDepth & DDBD_16)
			ddsd.dwZBufferBitDepth = 16;
		else if (devDepth & DDBD_8)
			ddsd.dwZBufferBitDepth = 8;
		else {
			DebugPrint("Unsupported Z-buffer depth requested by device.\n");
			return FALSE;
		}

		m_ZBufferDepth = ddsd.dwZBufferBitDepth;
		DXCALL(m_Device->CreateSurface(&ddsd, &m_ZBuffer, NULL));

	// Attach the Z buffer to the back surface.
		DXCALL(m_BackBuffer->AddAttachedSurface(m_ZBuffer));
#endif
	}

	return TRUE;
}


// Restore display surfaces.
//
BOOL CDirectDraw::RestoreBuffers(void)
{
	HRESULT	ddrval;

	DXCALL(m_FrontBuffer->Restore());
	DXCALL(m_BackBuffer->Restore());

	return TRUE;
}


// Create and set the default display palette.
//
BOOL CDirectDraw::CreatePalette(void)
{
    HRESULT ddrval;

    // create a default palette if we are in a paletized display mode.
    //
    // NOTE because we want to be able to show dialog boxs and
    // use our menu, we leave the windows reserved colors as is
    // so things dont look ugly.
    //
    // palette is setup like so:
    //
    //      10      windows system colors
    //      64      red wash
    //      64      grn wash
    //      64      blu wash
    //
    if (m_Profile.BPP == 8) {
        PALETTEENTRY ape[256];

        // get the current windows colors.
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE), 0,  10, &ape[0]);
        GetPaletteEntries((HPALETTE)GetStockObject(DEFAULT_PALETTE), 10, 10, &ape[246]);

        // make the palette.
        for (int i=0; i<64; i++)
        {
            ape[10+64*0+i].peRed   = i * 255/63;
            ape[10+64*0+i].peGreen = 0;
            ape[10+64*0+i].peBlue  = 0;

            ape[10+64*1+i].peRed   = 0;
            ape[10+64*1+i].peGreen = i * 255/63;
            ape[10+64*1+i].peBlue  = 0;

            ape[10+64*2+i].peRed   = 0;
            ape[10+64*2+i].peGreen = 0;
            ape[10+64*2+i].peBlue  = i * 255/63;
        }

        // create the palette.
        DXCALL(m_Device->CreatePalette(DDPCAPS_8BIT, ape, &m_Palette, NULL));

		if(!SetDisplayPalette()) {
			return FALSE;
		}
    }

	return TRUE;
}

// Set the palette for the display surfaces.
//
BOOL CDirectDraw::SetDisplayPalette(void)
{
	if(m_Profile.BPP == 8) {
		HRESULT     ddrval;

		DXCALL(m_BackBuffer->SetPalette(m_Palette));
		DXCALL(m_FrontBuffer->SetPalette(m_Palette));
	}

	return TRUE;
}


// Modify the palette used for the display surfaces.
//
BOOL CDirectDraw::SetDisplayPaletteEntries(PALETTEENTRY *PaletteEntries)
{
	if(m_Profile.BPP == 8) {
		HRESULT	ddrval;

		DXCALL(m_Palette->SetEntries(0, 0, 256,PaletteEntries));
	}

	return TRUE;
}


// Get the palette used for the display surfaces.
//
BOOL CDirectDraw::GetDisplayPaletteEntries(PALETTEENTRY *PaletteEntries)
{
	if(m_Profile.BPP == 8) {
		HRESULT ddrval;

		DXCALL(m_Palette->GetEntries(0, 0, 256,PaletteEntries));
	}

	return TRUE;
}


BOOL CDirectDraw::ClearBackBuffer(void)
{
	HRESULT	ddrval;
	D3DRECT	Rect = {0,0,m_Profile.XRes,m_Profile.YRes};

	DXCALL(m_Viewport->Clear(1, &Rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER));

	return TRUE;
}


// Switch display buffers.
//
BOOL CDirectDraw::SwitchBuffers(void)
{
	HRESULT ddrval;
	int XOffset = 0;
	int YOffset = 0;

	if(m_Profile.FullScreen) {
		DXCALL(m_FrontBuffer->Flip(NULL, DDFLIP_WAIT));
	} else {
		RECT Front;
		RECT Buffer;
		RECT rcWindow;

		GetClientRect(m_hWnd, &rcWindow);
		ClientToScreen(m_hWnd, (LPPOINT)&rcWindow);
		ClientToScreen(m_hWnd, (LPPOINT)&rcWindow+1);

		SetRect(&Buffer, XOffset, YOffset,
				m_Profile.XRes, m_Profile.YRes);
		SetRect(&Front, rcWindow.left, rcWindow.top,
				rcWindow.left+m_Profile.XRes-XOffset, rcWindow.top+m_Profile.YRes-YOffset);

		DXCALL(m_FrontBuffer->Blt(&Front, m_BackBuffer,
				 &Buffer, DDBLT_WAIT, NULL));
	}

	return TRUE;
}

	
// Create the 3d device and viewport.
//
BOOL CDirectDraw::Create3D(char *sz3DDevice)
{
	HRESULT ddrval;

// Enumerate available D3D devices.
	m_3DDeviceList.MaxDevices = MAX_D3D_DEVICES;
	m_3DDeviceList.NumDevices = 0;
	m_3DDeviceList.WindowsBPP = m_WindowsBPP;
	m_3DDeviceList.IsPrimary = g_IsPrimary;
	DXCALL(m_Direct3D->EnumDevices(EnumD3DDevicesCallback, (LPVOID*)&m_3DDeviceList));

	m_3DDeviceIndex = MatchDevice(sz3DDevice);

	DebugPrint("\nSelected ");
	Dump3DDeviceDesc(&m_3DDeviceList.List[m_3DDeviceIndex]);

#ifdef DX6
    // Using the device GUID, let's enumerate a format for our z-buffer
	memset(&m_ddpfZBuffer,0,sizeof(DDPIXELFORMAT));
	m_ddpfZBuffer.dwFlags = DDPF_ZBUFFER;
    DXCALL(m_Direct3D->EnumZBufferFormats(m_3DDeviceList.List[m_3DDeviceIndex].Guid, EnumZBufferFormatsCallback,
                                (void*)&m_ddpfZBuffer));

    if( sizeof(DDPIXELFORMAT) != m_ddpfZBuffer.dwSize )
    {
        DebugPrint("Device doesn't support requested zbuffer format");
        return FALSE;
    }
	if(!CreateZBuffer()) {
		return FALSE;
	}

	DXCALL(m_Direct3D->CreateDevice(m_3DDeviceList.List[m_3DDeviceIndex].Guid, m_BackBuffer, &m_3DDevice,NULL));
#else
	if(!CreateZBuffer()) {
		return FALSE;
	}

	DXCALL(m_Direct3D->CreateDevice(m_3DDeviceList.List[m_3DDeviceIndex].Guid, m_BackBuffer, &m_3DDevice));
#endif
	
// Make a Viewport
    DXCALL(m_Direct3D->CreateViewport(&m_Viewport, NULL));
    DXCALL(m_3DDevice->AddViewport(m_Viewport));

// Setup the viewport for a reasonable viewing area
    D3DVIEWPORT2 viewData;
    memset(&viewData, 0, sizeof(D3DVIEWPORT2));
    viewData.dwSize = sizeof(D3DVIEWPORT2);
    viewData.dwX = 0;
    viewData.dwY = 0;
    viewData.dwWidth  = m_ScreenSize.cx;
    viewData.dwHeight = m_ScreenSize.cy;
    viewData.dvClipX = -1.0f;
    viewData.dvClipWidth = 2.0f;
    viewData.dvClipHeight = (D3DVALUE)(m_ScreenSize.cy * 2.0 / m_ScreenSize.cx);
    viewData.dvClipY = viewData.dvClipHeight / 2.0f;
    viewData.dvMinZ = 0.0f;
    viewData.dvMaxZ = 1.0f;

    DXCALL(m_Viewport->SetViewport2(&viewData));
    DXCALL(m_3DDevice->SetCurrentViewport(m_Viewport));

	if(!CreateBackground()) {
		return FALSE;
	}

	// Get a list of available texture formats.
	g_TextureList.MaxFormats = MAX_DD_FORMATS;
	g_TextureList.NumFormats = 0;
    DXCALL(m_3DDevice->EnumTextureFormats(BuildTextureListCallback, (LPVOID)&g_TextureList));
    
	return TRUE;
}


void CDirectDraw::Dump3DDeviceDesc(D3DDevice *Device)
{
	DebugPrint("Device - %s\n",Device->Name);
	DebugPrint(" Description = %s\n",Device->Description);
	DebugPrint(" IsHardware = %s\n",Device->IsHardware ? "TRUE" : "FALSE");
	DebugPrint(" DoesTextures = %s\n",Device->DoesTextures ? "TRUE" : "FALSE");
	DebugPrint(" DoesZBuffer = %s\n",Device->DoesZBuffer ? "TRUE" : "FALSE");
	DebugPrint(" CanDoWindow = %s\n",Device->CanDoWindow ? "TRUE" : "FALSE");
#ifdef DUMPCAPS
	DebugPrint(" dcmColorModel %d\n",(CMODEL)Device->DeviceDesc.dcmColorModel);	     /* Color model of device */
	DebugPrint(" dwDevCaps %d\n",Device->DeviceDesc.dwDevCaps);	             /* Capabilities of device */
	DebugPrint(" Transform Caps\n");
	if(Device->DeviceDesc.dtcTransformCaps.dwCaps & D3DTRANSFORMCAPS_CLIP) DebugPrint("  D3DTRANSFORMCAPS_CLIP\n");
	DebugPrint(" Lighting Caps\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwLightingModel & D3DLIGHTINGMODEL_RGB) DebugPrint("  D3DLIGHTINGMODEL_RGB\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwLightingModel & D3DLIGHTINGMODEL_MONO) DebugPrint("  D3DLIGHTINGMODEL_MONO\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwCaps & D3DLIGHTCAPS_POINT) DebugPrint("  D3DLIGHTCAPS_POINT\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwCaps & D3DLIGHTCAPS_SPOT) DebugPrint("  D3DLIGHTCAPS_SPOT\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwCaps & D3DLIGHTCAPS_DIRECTIONAL) DebugPrint("  D3DLIGHTCAPS_DIRECTIONAL\n");
	if(Device->DeviceDesc.dlcLightingCaps.dwCaps & D3DLIGHTCAPS_PARALLELPOINT) DebugPrint("  D3DLIGHTCAPS_PARALLELPOINT\n");
	DebugPrint(" NumLights %d\n",Device->DeviceDesc.dlcLightingCaps.dwNumLights);
	DebugPrint(" Line Caps\n");
	DumpPrimCaps(&Device->DeviceDesc.dpcLineCaps);
	DebugPrint(" Tri Caps\n");
	DumpPrimCaps(&Device->DeviceDesc.dpcTriCaps);
	DebugPrint(" dwDeviceRenderBitDepth %d\n",Device->DeviceDesc.dwDeviceRenderBitDepth); /* One of DDBB_8, 16, etc.. */
	DebugPrint(" dwDeviceZBufferBitDepth %d\n",Device->DeviceDesc.dwDeviceZBufferBitDepth);/* One of DDBD_16, 32, etc.. */
	DebugPrint(" dwMaxBufferSize %d\n",Device->DeviceDesc.dwMaxBufferSize);        /* Maximum execute buffer size */
	DebugPrint(" dwMaxVertexCount %d\n",Device->DeviceDesc.dwMaxVertexCount);       /* Maximum vertex count */
	DebugPrint(" dwMinTextureWidth %d\n",Device->DeviceDesc.dwMinTextureWidth);
	DebugPrint(" dwMinTextureHeight %d\n",Device->DeviceDesc.dwMinTextureHeight);
	DebugPrint(" dwMaxTextureWidth %d\n",Device->DeviceDesc.dwMaxTextureWidth);
	DebugPrint(" dwMaxTextureHeight %d\n",Device->DeviceDesc.dwMaxTextureHeight);
	DebugPrint(" dwMinStippleWidth %d\n",Device->DeviceDesc.dwMinStippleWidth);
	DebugPrint(" dwMaxStippleWidth %d\n",Device->DeviceDesc.dwMaxStippleWidth);
	DebugPrint(" dwMinStippleHeight %d\n",Device->DeviceDesc.dwMinStippleHeight);
	DebugPrint(" dwMaxStippleHeight %d\n",Device->DeviceDesc.dwMaxStippleHeight);
#endif
	DebugPrint("\n");
}

void CDirectDraw::DumpPrimCaps(D3DPRIMCAPS *Caps)
{
	DebugPrint("  Misc Caps\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_MASKPLANES) DebugPrint("   D3DPMISCCAPS_MASKPLANES\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_MASKZ) DebugPrint("   D3DPMISCCAPS_MASKZ\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_LINEPATTERNREP) DebugPrint("   D3DPMISCCAPS_LINEPATTERNREP\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_CONFORMANT) DebugPrint("   D3DPMISCCAPS_CONFORMANT\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_CULLNONE) DebugPrint("   D3DPMISCCAPS_CULLNONE\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_CULLCW) DebugPrint("   D3DPMISCCAPS_CULLCW\n");
	if(Caps->dwMiscCaps & D3DPMISCCAPS_CULLCCW) DebugPrint("   D3DPMISCCAPS_CULLCCW\n");

	DebugPrint("  Raster Caps\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_DITHER) DebugPrint("   D3DPRASTERCAPS_DITHER\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ROP2) DebugPrint("   D3DPRASTERCAPS_ROP2\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_XOR) DebugPrint("   D3DPRASTERCAPS_XOR\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_PAT) DebugPrint("   D3DPRASTERCAPS_PAT\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ZTEST) DebugPrint("   D3DPRASTERCAPS_ZTEST\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_SUBPIXEL) DebugPrint("   D3DPRASTERCAPS_SUBPIXEL\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_SUBPIXELX) DebugPrint("   D3DPRASTERCAPS_SUBPIXELX\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX) DebugPrint("   D3DPRASTERCAPS_FOGVERTEX\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) DebugPrint("   D3DPRASTERCAPS_FOGTABLE\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_STIPPLE) DebugPrint("   D3DPRASTERCAPS_STIPPLE\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT) DebugPrint("   D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT) DebugPrint("   D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES) DebugPrint("   D3DPRASTERCAPS_ANTIALIASEDGES\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_MIPMAPLODBIAS) DebugPrint("   D3DPRASTERCAPS_MIPMAPLODBIAS\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ZBIAS) DebugPrint("   D3DPRASTERCAPS_ZBIAS\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR) DebugPrint("   D3DPRASTERCAPS_ZBUFFERLESSHSR\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_FOGRANGE) DebugPrint("   D3DPRASTERCAPS_FOGRANGE\n");
	if(Caps->dwRasterCaps & D3DPRASTERCAPS_ANISOTROPY) DebugPrint("   D3DPRASTERCAPS_ANISOTROPY\n");

	DebugPrint("  ZCmpCaps\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_NEVER) DebugPrint("   D3DPCMPCAPS_NEVER\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_LESS) DebugPrint("   D3DPCMPCAPS_LESS\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_EQUAL) DebugPrint("   D3DPCMPCAPS_EQUAL\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_LESSEQUAL) DebugPrint("   D3DPCMPCAPS_LESSEQUAL\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_GREATER) DebugPrint("   D3DPCMPCAPS_GREATER\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_NOTEQUAL) DebugPrint("   D3DPCMPCAPS_NOTEQUAL\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_GREATEREQUAL) DebugPrint("   D3DPCMPCAPS_GREATEREQUAL\n");
	if(Caps->dwZCmpCaps & D3DPCMPCAPS_ALWAYS) DebugPrint("   D3DPCMPCAPS_ALWAYS\n");

	DebugPrint("  SrcBlendCaps\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_ZERO) DebugPrint("   D3DPBLENDCAPS_ZERO\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_ONE) DebugPrint("   D3DPBLENDCAPS_ONE\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR) DebugPrint("   D3DPBLENDCAPS_SRCCOLOR\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR) DebugPrint("   D3DPBLENDCAPS_INVSRCCOLOR\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) DebugPrint("   D3DPBLENDCAPS_SRCALPHA\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) DebugPrint("   D3DPBLENDCAPS_INVSRCALPHA\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_DESTALPHA) DebugPrint("   D3DPBLENDCAPS_DESTALPHA\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_INVDESTALPHA) DebugPrint("   D3DPBLENDCAPS_INVDESTALPHA\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR) DebugPrint("   D3DPBLENDCAPS_DESTCOLOR\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) DebugPrint("   D3DPBLENDCAPS_INVDESTCOLOR\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHASAT) DebugPrint("   D3DPBLENDCAPS_SRCALPHASAT\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA) DebugPrint("   D3DPBLENDCAPS_BOTHSRCALPHA\n");
	if(Caps->dwSrcBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA) DebugPrint("   D3DPBLENDCAPS_BOTHINVSRCALPHA\n");

	DebugPrint("  DestBlendCaps\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_ZERO) DebugPrint("   D3DPBLENDCAPS_ZERO\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_ONE) DebugPrint("   D3DPBLENDCAPS_ONE\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR) DebugPrint("   D3DPBLENDCAPS_SRCCOLOR\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_INVSRCCOLOR) DebugPrint("   D3DPBLENDCAPS_INVSRCCOLOR\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_SRCALPHA) DebugPrint("   D3DPBLENDCAPS_SRCALPHA\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) DebugPrint("   D3DPBLENDCAPS_INVSRCALPHA\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_DESTALPHA) DebugPrint("   D3DPBLENDCAPS_DESTALPHA\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_INVDESTALPHA) DebugPrint("   D3DPBLENDCAPS_INVDESTALPHA\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_DESTCOLOR) DebugPrint("   D3DPBLENDCAPS_DESTCOLOR\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_INVDESTCOLOR) DebugPrint("   D3DPBLENDCAPS_INVDESTCOLOR\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_SRCALPHASAT) DebugPrint("   D3DPBLENDCAPS_SRCALPHASAT\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_BOTHSRCALPHA) DebugPrint("   D3DPBLENDCAPS_BOTHSRCALPHA\n");
	if(Caps->dwDestBlendCaps & D3DPBLENDCAPS_BOTHINVSRCALPHA) DebugPrint("   D3DPBLENDCAPS_BOTHINVSRCALPHA\n");

	DebugPrint("  AlphaCmpCaps\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_NEVER) DebugPrint("   D3DPCMPCAPS_NEVER\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_LESS) DebugPrint("   D3DPCMPCAPS_LESS\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_EQUAL) DebugPrint("   D3DPCMPCAPS_EQUAL\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_LESSEQUAL) DebugPrint("   D3DPCMPCAPS_LESSEQUAL\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_GREATER) DebugPrint("   D3DPCMPCAPS_GREATER\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_NOTEQUAL) DebugPrint("   D3DPCMPCAPS_NOTEQUAL\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_GREATEREQUAL) DebugPrint("   D3DPCMPCAPS_GREATEREQUAL\n");
	if(Caps->dwAlphaCmpCaps & D3DPCMPCAPS_ALWAYS) DebugPrint("   D3DPCMPCAPS_ALWAYS\n");

	DebugPrint("  ShadeCaps\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORFLATMONO) DebugPrint("   D3DPSHADECAPS_COLORFLATMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORFLATRGB) DebugPrint("   D3DPSHADECAPS_COLORFLATRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDMONO) DebugPrint("   D3DPSHADECAPS_COLORGOURAUDMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB) DebugPrint("   D3DPSHADECAPS_COLORGOURAUDRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORPHONGMONO) DebugPrint("   D3DPSHADECAPS_COLORPHONGMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_COLORPHONGRGB) DebugPrint("   D3DPSHADECAPS_COLORPHONGRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARFLATMONO) DebugPrint("   D3DPSHADECAPS_SPECULARFLATMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARFLATRGB) DebugPrint("   D3DPSHADECAPS_SPECULARFLATRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDMONO) DebugPrint("   D3DPSHADECAPS_SPECULARGOURAUDMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARGOURAUDRGB) DebugPrint("   D3DPSHADECAPS_SPECULARGOURAUDRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARPHONGMONO) DebugPrint("   D3DPSHADECAPS_SPECULARPHONGMONO\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_SPECULARPHONGRGB) DebugPrint("   D3DPSHADECAPS_SPECULARPHONGRGB\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAFLATBLEND) DebugPrint("   D3DPSHADECAPS_ALPHAFLATBLEND\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAFLATSTIPPLED) DebugPrint("   D3DPSHADECAPS_ALPHAFLATSTIPPLED\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND) DebugPrint("   D3DPSHADECAPS_ALPHAGOURAUDBLEND\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED) DebugPrint("   D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAPHONGBLEND) DebugPrint("   D3DPSHADECAPS_ALPHAPHONGBLEND\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_ALPHAPHONGSTIPPLED) DebugPrint("   D3DPSHADECAPS_ALPHAPHONGSTIPPLED\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_FOGFLAT) DebugPrint("   D3DPSHADECAPS_FOGFLAT\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_FOGGOURAUD) DebugPrint("   D3DPSHADECAPS_FOGGOURAUD\n");
	if(Caps->dwShadeCaps & D3DPSHADECAPS_FOGPHONG) DebugPrint("   D3DPSHADECAPS_FOGPHONG\n");

	DebugPrint("  TextureCaps\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) DebugPrint("   D3DPTEXTURECAPS_PERSPECTIVE\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_POW2) DebugPrint("   D3DPTEXTURECAPS_POW2\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_ALPHA) DebugPrint("   D3DPTEXTURECAPS_ALPHA\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_TRANSPARENCY) DebugPrint("   D3DPTEXTURECAPS_TRANSPARENCY\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_BORDER) DebugPrint("   D3DPTEXTURECAPS_BORDER\n");
	if(Caps->dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) DebugPrint("   D3DPTEXTURECAPS_SQUAREONLY\n");

	DebugPrint("  TextureFilterCaps\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_NEAREST) DebugPrint("   D3DPTFILTERCAPS_NEAREST\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_LINEAR) DebugPrint("   D3DPTFILTERCAPS_LINEAR\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_MIPNEAREST) DebugPrint("   D3DPTFILTERCAPS_MIPNEAREST\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR) DebugPrint("   D3DPTFILTERCAPS_MIPLINEAR\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST) DebugPrint("   D3DPTFILTERCAPS_LINEARMIPNEAREST\n");
	if(Caps->dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR) DebugPrint("   D3DPTFILTERCAPS_LINEARMIPLINEAR\n");

	DebugPrint("  TextureBlendCaps\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_DECAL) DebugPrint("   D3DPTBLENDCAPS_DECAL\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATE) DebugPrint("   D3DPTBLENDCAPS_MODULATE\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_DECALALPHA) DebugPrint("   D3DPTBLENDCAPS_DECALALPHA\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) DebugPrint("   D3DPTBLENDCAPS_MODULATEALPHA\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_DECALMASK) DebugPrint("   D3DPTBLENDCAPS_DECALMASK\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEMASK) DebugPrint("   D3DPTBLENDCAPS_MODULATEMASK\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_COPY) DebugPrint("   D3DPTBLENDCAPS_COPY\n");
	if(Caps->dwTextureBlendCaps & D3DPTBLENDCAPS_ADD) DebugPrint("   D3DPTBLENDCAPS_ADD\n");

	DebugPrint("  TextureAddressCaps\n");
	if(Caps->dwTextureAddressCaps & D3DPTADDRESSCAPS_WRAP) DebugPrint("   D3DPTADDRESSCAPS_WRAP\n");
	if(Caps->dwTextureAddressCaps & D3DPTADDRESSCAPS_MIRROR) DebugPrint("   D3DPTADDRESSCAPS_MIRROR\n");
	if(Caps->dwTextureAddressCaps & D3DPTADDRESSCAPS_CLAMP) DebugPrint("   D3DPTADDRESSCAPS_CLAMP\n");
	if(Caps->dwTextureAddressCaps & D3DPTADDRESSCAPS_BORDER) DebugPrint("   D3DPTADDRESSCAPS_BORDER\n");
	if(Caps->dwTextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV) DebugPrint("   D3DPTADDRESSCAPS_INDEPENDENTUV\n");

	DebugPrint("  dwStippleWidth %d\n",Caps->dwStippleWidth);
	DebugPrint("  dwStippleHeight %d\n",Caps->dwStippleHeight);
}


int CDirectDraw::MatchDevice(char *sz3DDevice)
{
	int	i;

// First off, try to find the named device.
	if(sz3DDevice) {
		for(i=0; i<m_3DDeviceList.NumDevices; i++) {
			if (lstrcmpi(m_3DDeviceList.List[i].Name, sz3DDevice)==0) {
				DebugPrint("Found named device %s\n",sz3DDevice);

				m_DevDesc = m_3DDeviceList.List[i].DeviceDesc;
				m_Profile.Hardware = m_3DDeviceList.List[i].IsHardware;
				return i;
			}
		}
	}

// If asked for hardware device then try and find one.
	if(m_Profile.UseHardware) {

// First try and find one that matches hardware and colour model.
		for(i=0; i<m_3DDeviceList.NumDevices; i++) {
			if((m_3DDeviceList.List[i].IsHardware) &&
				(m_3DDeviceList.List[i].DoesTextures) &&
				(m_3DDeviceList.List[i].DeviceDesc.dcmColorModel == (DWORD)m_Profile.ColourModel)) {

				m_DevDesc = m_3DDeviceList.List[i].DeviceDesc;
				m_Profile.Hardware = TRUE;
				return i;
			}
		}

// If that fails then try and find any hardware device.
		for(i=0; i<m_3DDeviceList.NumDevices; i++) {
			if((m_3DDeviceList.List[i].IsHardware) &&
				(m_3DDeviceList.List[i].DoesTextures)) {

				m_Profile.ColourModel = (CMODEL)m_3DDeviceList.List[i].DeviceDesc.dcmColorModel;

				m_DevDesc = m_3DDeviceList.List[i].DeviceDesc;
				m_Profile.Hardware = TRUE;
				return i;
			}
		}
	}

// If no hardware device then try for MMX device.
	m_Profile.UseHardware = FALSE;

	for(i=0; i<m_3DDeviceList.NumDevices; i++) {
		if(strncmp(m_3DDeviceList.List[i].Name,"MMX",3) == 0) {

			m_DevDesc = m_3DDeviceList.List[i].DeviceDesc;
			m_Profile.Hardware = FALSE;
			return i;
		}
	}

// If no hardware or MMX device then get software device that matches the colour model.
	m_Profile.UseHardware = FALSE;

	for(i=0; i<m_3DDeviceList.NumDevices; i++) {
		if(m_3DDeviceList.List[i].DeviceDesc.dcmColorModel == (DWORD)m_Profile.ColourModel) {

			m_DevDesc = m_3DDeviceList.List[i].DeviceDesc;
			m_Profile.Hardware = FALSE;
			return i;
		}
	}

	DebugPrint("*** No device matches found ***\n");

	return 0;
}


// Define the default state.
BOOL CDirectDraw::SetDefaultRenderState(void)
{
	D3DState	DefaultState;

	DefaultState.ShadeMode = D3DSHADE_GOURAUD;
	DefaultState.FillMode = D3DFILL_SOLID;
	DefaultState.TextureCorrect = TRUE;

#ifdef ANTIALIAS
	DefaultState.AntiAlias = TRUE;
#else
	DefaultState.AntiAlias = FALSE;
#endif

#ifdef DITHER
	DefaultState.Dither = TRUE;
#else
	DefaultState.Dither = FALSE;
#endif

#ifdef SPECULAR
	DefaultState.Specular = TRUE;
#else
	DefaultState.Specular = FALSE;
#endif

#ifdef FILTER
	if (m_Profile.Hardware) {
		DefaultState.MagFilterMode = D3DFILTER_LINEAR;
		DefaultState.MinFilterMode = D3DFILTER_LINEAR;
	} else {
		DefaultState.MagFilterMode = D3DFILTER_NEAREST;
		DefaultState.MinFilterMode = D3DFILTER_NEAREST;
	}
#else
	DefaultState.MagFilterMode = D3DFILTER_NEAREST;
	DefaultState.MinFilterMode = D3DFILTER_NEAREST;
#endif

	DefaultState.ZBuffer = m_Profile.UseZBuffer;

	DefaultState.BlendEnable = FALSE;
	DefaultState.BlendMode = D3DTBLEND_MODULATE;
	DefaultState.SourceBlend = D3DBLEND_SRCALPHA;
	DefaultState.DestBlend = D3DBLEND_INVSRCALPHA;

#ifdef FOGGY
	DefaultState.FogEnabled = TRUE;
	DefaultState.FogMode = D3DFOG_LINEAR;
#else
	DefaultState.FogEnabled = FALSE;
	DefaultState.FogMode = D3DFOG_NONE;
#endif

	DefaultState.FogColour = RGB_MAKE(SKYRED,SKYGREEN,SKYBLUE);
	DefaultState.FogDensity = D3DVAL(0.5);
	DefaultState.FogTableDensity = D3DVAL(0.5);
    DefaultState.FogStart = D3DVAL(1000);		// Distance from camera that fog starts.
	DefaultState.FogStop = D3DVAL(2000);		// Distance from camera that fog reaches max density.
//SetFog(TRUE,SKYRED,SKYGREEN,SKYBLUE,D3DVAL(1000),D3DVAL(2000));

	DefaultState.AmbientLight = RGBA_MAKE(64,64,64,0);

	return SetRenderState(&DefaultState);
}

	
// Set the default 3d state.
//
BOOL CDirectDraw::SetRenderState(D3DState *State)
{
	HRESULT ddrval;

	m_CurrentState = *State;

	BeginScene();

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, m_CurrentState.ShadeMode));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, m_CurrentState.TextureCorrect));

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE,m_CurrentState.ZBuffer && m_Profile.UseZBuffer));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, m_CurrentState.ZBuffer && m_Profile.UseZBuffer));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL));

#ifndef DX6
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, m_CurrentState.MagFilterMode));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, m_CurrentState.MinFilterMode));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, m_CurrentState.BlendMode));
#endif

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE ,FALSE));	// Only do this when needed.

//	m_CurrentState.BlendEnable = TRUE;
//	m_CurrentState.BlendMode = D3DTBLEND_MODULATE;
//	m_CurrentState.SourceBlend = D3DBLEND_SRCALPHA;
//	m_CurrentState.DestBlend = D3DBLEND_INVSRCALPHA;

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, m_CurrentState.SourceBlend));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND , m_CurrentState.DestBlend));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, m_CurrentState.BlendEnable));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, m_CurrentState.FillMode));

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, m_CurrentState.Dither));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, m_CurrentState.Specular));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_ANTIALIAS, m_CurrentState.AntiAlias));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, m_CurrentState.FogEnabled));

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR, m_CurrentState.FogColour));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEDENSITY ,*(ULONG*)&m_CurrentState.FogTableDensity));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGMODE, m_CurrentState.FogMode));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGSTART, *(ULONG*)&m_CurrentState.FogStart));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGEND,  *(ULONG*)&m_CurrentState.FogStop));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGDENSITY ,*(ULONG*)&m_CurrentState.FogDensity));

	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_AMBIENT, m_CurrentState.AmbientLight));

#ifdef DX6
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE ));
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE ));
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE ));
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ));
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 ));
	//    DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 ));
 #ifdef FILTER
	// Set bilinear filtering.
	if (m_Profile.Hardware) {
		DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_LINEAR ));
		DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ));
	} else {
		DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_POINT ));
		DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ));
	}
 #else
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFN_POINT ));
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ));
 #endif

 #ifdef MIPMAP
	// Set mip maping mode.
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR ));
 #else
	DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTFP_POINT ));
 #endif
#endif

#ifdef DX6
	DWORD dwNumPasses;
	DXCALL(m_3DDevice->ValidateDevice( &dwNumPasses ));
#endif
	EndScene();

	return TRUE;
}


BOOL CDirectDraw::SetFog(BOOL Enable,UBYTE Red,UBYTE Green,UBYTE Blue,ULONG Start,ULONG Stop)
{
	HRESULT ddrval;

	float FStart = D3DVAL(Start);
	float FStop = D3DVAL(Stop);

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, Enable));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR, RGB_MAKE(Red,Green,Blue)));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGMODE, Enable ? D3DFOG_LINEAR : D3DFOG_NONE));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGSTART, *(ULONG*)&FStart));
	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_FOGEND,  *(ULONG*)&FStop));

	return TRUE;
}


BOOL CDirectDraw::SetFilter(BOOL Enable)
{
	HRESULT ddrval;

	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG,
			Enable ? D3DFILTER_LINEAR : D3DFILTER_NEAREST));
	DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN,
			Enable ? D3DFILTER_LINEAR : D3DFILTER_NEAREST));

	return TRUE;
}


BOOL CDirectDraw::SetAmbient(UBYTE Red,UBYTE Green,UBYTE Blue)
{
	HRESULT ddrval;

	DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_AMBIENT, RGBA_MAKE(Red,Green,Blue,0)));

	return TRUE;
}
	
// Create a default material for the background and attach it to the viewport.
//
BOOL CDirectDraw::CreateBackground(void)
{
	HRESULT ddrval;
	D3DCOLORVALUE Colour={
		(D3DVALUE)((float)SKYRED+1)/256,
		(D3DVALUE)((float)SKYGREEN+1)/256,
		(D3DVALUE)((float)SKYBLUE+1)/256};

	m_BackMaterial = new CMaterial;
	m_BackMaterial->Create(m_Direct3D,m_3DDevice,&Colour,&Colour,NULL);
	DXCALL(m_Viewport->SetBackground(m_BackMaterial->GetHandle()));

	return TRUE;
}


// Set the backgrounds material handle.
//
BOOL CDirectDraw::SetBackgroundMaterial(D3DMATERIALHANDLE Handle)
{
	HRESULT ddrval;
	DXCALL(m_Viewport->SetBackground(Handle));

	return TRUE;
}


void *CDirectDraw::GetBackBufferDC(void)
{
	HDC	hdc;
	HRESULT ddrval;

    DXCALL(m_BackBuffer->GetDC(&hdc));

	return hdc;
}

BOOL CDirectDraw::ReleaseBackBufferDC(void *hdc)
{
	HRESULT ddrval;

    DXCALL(m_BackBuffer->ReleaseDC((HDC)hdc));

	return TRUE;
}


BOOL CDirectDraw::BeginScene(void)
{
	HRESULT ddrval;

	m_InScene++;

	if(m_InScene == 1) {
		DXCALL(m_3DDevice->BeginScene());
	}

	return TRUE;
}

BOOL CDirectDraw::EndScene(void)
{
	HRESULT ddrval;

	if(m_InScene == 1) {
		DXCALL(m_3DDevice->EndScene());
	}

	m_InScene--;

	return TRUE;
}

#ifdef DX6
#define UPDATERENDERSTATE(MatHandle,TexInterface) \
	if(MatHandle != m_LastMatHandle) { \
		DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_MATERIAL, MatHandle)); \
		m_LastMatHandle = MatHandle; \
	} \
	if(TexInterface != m_LastTexInterface) { \
		DXCALL(m_3DDevice->SetTexture(0,TexInterface)); \
		DXCALL(m_3DDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR )); \
		m_LastTexInterface = TexInterface; \
	}

// Transform , light and render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_VERTEX ,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


// Transform  and render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


// Render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
							D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
								D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,ID3DTEXTURE *TexInterface,
									D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexInterface);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DFVF_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}
#else
#define UPDATERENDERSTATE(MatHandle,TexHandle) \
	if(MatHandle != m_LastMatHandle) { \
		DXCALL(m_3DDevice->SetLightState(D3DLIGHTSTATE_MATERIAL, MatHandle)); \
		m_LastMatHandle = MatHandle; \
	} \
	if(TexHandle != m_LastTexHandle) { \
		DXCALL(m_3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, TexHandle)); \
		m_LastTexHandle = TexHandle; \
	}

// Transform , light and render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_VERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


// Transform  and render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_LVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


// Render

BOOL CDirectDraw::DrawPoint(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_POINTLIST,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
							D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINELIST,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawPolyLine(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_LINESTRIP,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangle(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
								D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleFan(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}


BOOL CDirectDraw::DrawTriangleStrip(D3DMATERIALHANDLE MatHandle,D3DTEXTUREHANDLE TexHandle,
									D3DTLVERTEX *Vertex,DWORD Count)
{
	HRESULT ddrval;

	UPDATERENDERSTATE(MatHandle,TexHandle);
	DXCALL(m_3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,D3DVT_TLVERTEX,
											(LPVOID)Vertex,Count,m_ClipState));
	return TRUE;
}
#endif

// Create a surface for storing a bitmap ( not textures ).
//
IDDSURFACE *CDirectDraw::CreateImageSurface(DWORD Width,DWORD Height,SURFACEMEMTYPE MemType)
{
    SURFACEDESC ddsd;
	IDDSURFACE *Surface;
	HRESULT	ddrval;

	memset( &ddsd, 0, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = Width;
    ddsd.dwHeight = Height;

	if(MemType == SURFACE_SYSTEMMEMORY) {
		ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	} else {
		ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	}

    DXCALL(m_Device->CreateSurface(&ddsd, &Surface, NULL));

	return(Surface);
}


// Create a bitmap surface.
//
IDDSURFACE *CDirectDraw::CreateImageSurfaceFromBitmap(void *Bits,DWORD Width,DWORD Height,
														PALETTEENTRY *PaletteEntries,SURFACEMEMTYPE MemType)
{
	HRESULT	ddrval;
	IDDSURFACE *Surface;
	IDDPALETTE *Palette = NULL;

// Create the surface.
	Surface=CreateImageSurface(Width,Height,MemType);

// Copy the bitmap to the surface converting the pixel format if needed.
	FillSurface(Bits,Width,Height,PaletteEntries,0,255,NULL,Surface,&m_PixFormat);

// Attach the palette to the surface.
	if(m_PixFormat.Palettized) {
        DXCALL(m_Device->CreatePalette(DDPCAPS_8BIT, PaletteEntries, &Palette, NULL));
		DXCALL(Surface->SetPalette(Palette));
		RELEASE(Palette);
	}

	return Surface;
}


// ---- CMaterial -------------------------------------------------------------------------------
//


CMaterial::CMaterial(void)
{
	m_Handle = 0;
	m_Material = NULL;
}


CMaterial::~CMaterial(void)
{
	RELEASE(m_Material);
}


BOOL CMaterial::Create(ID3D *Direct3D,ID3DDEVICE *Device,
				  D3DCOLORVALUE *Ambient,
				  D3DCOLORVALUE *Diffuse,
				  D3DCOLORVALUE *Specular,D3DVALUE Power,
				  D3DCOLORVALUE *Emissive,
				  D3DTEXTUREHANDLE TexHandle)
{
	HRESULT ddrval;
    D3DMATERIAL Mat;
	D3DCOLORVALUE Default = {(D3DVALUE)0.0F, (D3DVALUE)0.0F, (D3DVALUE)0.0F, (D3DVALUE)1.0F};

	DXCALL(Direct3D->CreateMaterial(&m_Material, NULL));

    memset(&Mat, 0, sizeof(D3DMATERIAL));
    Mat.dwSize = sizeof(D3DMATERIAL);

	if(Diffuse) {
		Mat.diffuse = *Diffuse;	//128,128,128,1
		Mat.diffuse.a = 1.0f;
	} else {
		Mat.diffuse = Default;
	}

	if(Ambient) {
		Mat.ambient = *Ambient;	//128,128,128,0
	} else {
		Mat.ambient = Default;
	}

	if(Specular) {
		Mat.specular = *Specular; //0,0,0,1
	} else {
#ifdef SPECULAR
		Mat.specular.r = 1.0f;
		Mat.specular.g = 1.0f;
		Mat.specular.b = 1.0f;
		Mat.specular.a = 1.0f;
#else
		Mat.specular = Default;
#endif
	}

	Mat.power = Power;

	if(Emissive) {
		Mat.emissive = *Emissive; //0,0,0,1
	} else {
		Mat.emissive = Default;
	}

#ifndef DX6
	Mat.hTexture = TexHandle;
#endif
    Mat.dwRampSize = 16;

    DXCALL(m_Material->SetMaterial(&Mat));
    DXCALL(m_Material->GetHandle(Device, &m_Handle));

	return TRUE;
}

/*
Prototype:
void CMaterial::GetMaterialColour(D3DCOLORVALUE *Ambient,
								D3DCOLORVALUE *Diffuse,
								D3DCOLORVALUE *Specular)

Description:
 Get the colour attributes of the specified material.

Parameters:
	D3DCOLORVALUE *Ambient	Pointer to place to store Ambient colour.
							Ignored if NULL.
	D3DCOLORVALUE *Diffuse	Pointer to place to store Diffuse colour.
							Ignored if NULL.
	D3DCOLORVALUE *Specular	Pointer to place to store Specular colour.
							Ignored if NULL.

Returns:
 Material's ambient,diffuse and specular colours.

Implementation Notes:
*/
BOOL CMaterial::GetMaterialColour(D3DCOLORVALUE *Ambient,
							D3DCOLORVALUE *Diffuse,
							D3DCOLORVALUE *Specular)
{
	D3DMATERIAL Mat;
	HRESULT ddrval;

   	memset(&Mat, 0, sizeof(D3DMATERIAL));
   	Mat.dwSize = sizeof(D3DMATERIAL);
	DXCALL(m_Material->GetMaterial(&Mat));

	if(Diffuse) {
		*Diffuse = Mat.diffuse;
	}

	if(Ambient) {
		*Ambient = Mat.ambient;
	}

	if(Specular) {   	
		*Specular = Mat.specular;
	}

	return TRUE;
}

/*
Prototype:
void CMaterial::SetMaterialColour(D3DCOLORVALUE *Ambient,
								D3DCOLORVALUE *Diffuse,
								D3DCOLORVALUE *Specular)

Description:
 Set the colour attributes of the specified material.

Parameters:
	D3DCOLORVALUE *Ambient	Pointer to Ambient colour.
							Ignored if NULL.
	D3DCOLORVALUE *Diffuse	Pointer to Diffuse colour.
							Ignored if NULL.
	D3DCOLORVALUE *Specular	Pointer to Specular colour.
							Ignored if NULL.

Returns:
 None.

Implementation Notes:
*/
BOOL CMaterial::SetMaterialColour(D3DCOLORVALUE *Ambient,
							D3DCOLORVALUE *Diffuse,
							D3DCOLORVALUE *Specular)
{
	D3DMATERIAL Mat;		// Material description
	HRESULT ddrval;

   	memset(&Mat, 0, sizeof(D3DMATERIAL));
   	Mat.dwSize = sizeof(D3DMATERIAL);
   	DXCALL(m_Material->GetMaterial(&Mat));

	if(Diffuse) {
		Mat.diffuse = *Diffuse;
	}

	if(Ambient) {
		Mat.ambient = *Ambient;
	}

	if(Specular) {   	
		Mat.specular = *Specular;
	}
   	
	DXCALL(m_Material->SetMaterial(&Mat));

	return TRUE;
}


// ---- CTexture --------------------------------------------------------------------------------
//


CTexture::CTexture(void)
{
	m_MemorySurface = NULL;
	m_DeviceSurface = NULL;
	m_Palette = NULL;
#ifdef DX6
    m_Texture = NULL;
#else
	m_Handle = 0;
#endif
}


CTexture::~CTexture(void)
{
#ifdef DX6
    RELEASE(m_Texture);
#endif
	RELEASE(m_MemorySurface);
	RELEASE(m_DeviceSurface);
	RELEASE(m_Palette);
}

#ifdef SCALETEXTURES

BOOL CTexture::Create(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
						void *Bits,DWORD Width,DWORD Height,
						PALETTEENTRY *PaletteEntries,
						TFORMAT TexFormat,
						UBYTE Alpha,void *AlphaMap,
						DWORD Scale)
{
//	HRESULT ddrval;
//    SURFACEDESC ddsd;
//	int TexFormatIndex;
//	BOOL IsHardware;
//	BOOL MaxSizeValid = FALSE;
//
//	Width /= Scale;
//	Height /= Scale;
//
//	DebugPrint("Scale %d : Width %d , Height %d\n",Scale,Width,Height);
//
//	// Are we using a hardware renderer.
//    D3DDEVICEDESC hal, hel;
//    memset(&hal,0,sizeof(hal));
//    hal.dwSize = sizeof(hal);
//    memset(&hel,0,sizeof(hel));
//    hel.dwSize = sizeof(hel);
//    Device3D->GetCaps(&hal, &hel);
//	IsHardware = hal.dcmColorModel ? TRUE : FALSE;
//
//	if(IsHardware) {
//		// If the max texture width and height device caps are valid then check the
//		// texture width against them and correct if necessary.
//		DWORD MaxWidth = hal.dwMaxTextureWidth;
//		DWORD MaxHeight = hal.dwMaxTextureHeight;
//		if((MaxWidth > 0) && (MaxHeight > 0)) {
//			MaxSizeValid = TRUE;
//			if(Width > MaxWidth) Width = MaxWidth;
//			if(Height > MaxHeight) Height = MaxHeight;
//		}
//	}
//
//	TexFormatIndex = MatchFormat(TexFormat);
//
//    memset(&ddsd,0,sizeof(ddsd));
//    ddsd.dwSize = sizeof(ddsd);
//	ddsd.ddpfPixelFormat = g_TextureList.List[TexFormatIndex].PixelFormat;
//    ddsd.dwFlags |= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
//    ddsd.dwWidth = Width/Scale;
//    ddsd.dwHeight = Height/Scale;
//
//	// Create the device surface , in video memory if hardware, else in system memory.
//    if(IsHardware) {
//        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD;
//	    ddrval = DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL);
//		if(ddrval == DDERR_OUTOFVIDEOMEMORY) {
//			DebugPrint(" Out of video memory!\n");
//
//			Scale++;
//			return Create(DirectDraw,Direct3D,Device3D,
//							Bits,Width,Height,
//							PaletteEntries,
//							TexFormat,
//							Alpha,AlphaMap,
//							Scale);
//		} else if(ddrval != DD_OK) {
//			DisplayError(ddrval,__FILE__,__LINE__);
//		}
//
//		// Check to see if it went into VRAM ( needs to with most 3d hardware ).
//		SURFACECAPS SurfaceCaps;
//		DXCALL(m_DeviceSurface->GetCaps(&SurfaceCaps));
//		if( !(SurfaceCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) {
//			if( !(hal.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) ) {
//				DebugPrint(" SysMemory , Can't do!\n");
//
//				RELEASE(m_DeviceSurface);
//				Scale++;
//
//				return Create(DirectDraw,Direct3D,Device3D,
//								Bits,Width,Height,
//								PaletteEntries,
//								TexFormat,
//								Alpha,AlphaMap,
//								Scale);
//			}
//		}
//	} else {
//        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY| DDSCAPS_TEXTURE;
//	    DXCALL(DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL));
//	}
//
//	// If hardware then create the system memory surface for the source texture.
//	if(IsHardware) {
//        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
//        if ((ddrval = DirectDraw->CreateSurface(&ddsd, &m_MemorySurface, NULL)) != DD_OK) {
//			DisplayError(ddrval,__FILE__,__LINE__,0);
//			RELEASE(m_DeviceSurface);
//			return FALSE;
//		}
//
//		if(!MaxSizeValid) {
//			// Get a texture interface and handle to test if we can create a texture this size.
//			LPDIRECT3DTEXTURE2	TmpTex=NULL;
////			D3DTEXTUREHANDLE	TmpTexHandle;
//			DXCALL(m_MemorySurface->QueryInterface(IID_IDirect3DTexture2,(LPVOID *)&TmpTex));
//
////			ddrval = TmpTex->GetHandle(Device3D, &TmpTexHandle);
//			// If the texture creation failed then try again with a smaller scale.
//			if( (ddrval == D3DERR_TEXTURE_CREATE_FAILED) || (ddrval == D3DERR_TEXTURE_BADSIZE) ) {
//					DebugPrint(" Texture create failed , trying new scale!\n");
//
//					RELEASE(m_DeviceSurface);
//					RELEASE(m_MemorySurface);
//					RELEASE(TmpTex);
//					Scale++;
//
//					return Create(DirectDraw,Direct3D,Device3D,
//									Bits,Width,Height,
//									PaletteEntries,
//									TexFormat,
//									Alpha,AlphaMap,
//									Scale);
//			} else {
//				if(ddrval != DD_OK) {
//					DisplayError(ddrval,__FILE__,__LINE__);
//					RELEASE(m_DeviceSurface);
//					RELEASE(m_MemorySurface);
//					RELEASE(TmpTex);
//					return FALSE;
//				}
//			}
//
//			RELEASE(TmpTex);
//		}
//	} else {
//        m_MemorySurface = m_DeviceSurface;
//        m_DeviceSurface->AddRef();
//	}
//
////	// Set the colour key , default to palette index 0.
////	SetColourKey(ColourKeyIndex);
//
//	// Create the palette and attach to the surfaces.
//    if(ddsd.ddpfPixelFormat.dwRGBBitCount <= 8)
//    {
//
//		DWORD *pe = (DWORD*)PaletteEntries;
//        for( int i=0; i<256; i++ )
//        {
//            pe[i] = 0xff000000 + RGB( GetBValue(pe[i]), GetGValue(pe[i]),
//                                      GetRValue(pe[i]) );
//
//			// Set alpha for transparent pixels
//			if( dwFlags & D3DTEXTR_TRANSPARENTBLACK )
//            {
//                if( (pe[i]&0x00ffffff) == 0x00000000 )
//                    pe[i] &= 0x00ffffff;
//            }
//            else if( dwFlags & D3DTEXTR_TRANSPARENTWHITE )
//            {
//                if( (pe[i]&0x00ffffff) == 0x00ffffff )
//                    pe[i] &= 0x00ffffff;
//            }
//		}
//        // Create & attach a palette with the bitmap's colors
////        LPDIRECTDRAWPALETTE  pPalette;
////		if( dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
////            pDD->CreatePalette( DDPCAPS_8BIT|DDPCAPS_ALPHA, (PALETTEENTRY*)pe, &pPalette, NULL );
//
//        DirectDraw->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALPHA, PaletteEntries, &m_Palette, NULL);
//
//        m_MemorySurface->SetPalette(m_Palette);
//        m_DeviceSurface->SetPalette(m_Palette);
//    }
//
//	// Copy the bitmap to the surface.
//	if(!FillSurface(Bits,Width,Height,
//					PaletteEntries,Alpha,AlphaMap,
//					m_MemorySurface,
//					&g_TextureList.List[TexFormatIndex].PixelFormatExt)) {
//		RELEASE(m_DeviceSurface);
//		RELEASE(m_MemorySurface);
//		RELEASE(m_Palette);
//		return FALSE;
//	}
//
//	// Restore the video memory surface and load texture into it from system memory surface.
//	ddrval = Restore();
//	if(ddrval == DDERR_NOBLTHW) {
//		// If the load failed then could be the card can't handle textures at this size
//		// so change the scale and try again.
//		DebugPrint(" Load Failed!\n");
//
//		RELEASE(m_DeviceSurface);
//		RELEASE(m_MemorySurface);
//		RELEASE(m_Palette);
//		Scale++;
//
//		return Create(DirectDraw,Direct3D,Device3D,
//						Bits,Width,Height,
//						PaletteEntries,
//						TexFormat,
//						Alpha,AlphaMap,
//						Scale);
//	} else if(ddrval != DD_OK) {
//		DisplayError(ddrval,__FILE__,__LINE__);
//		RELEASE(m_DeviceSurface);
//		RELEASE(m_MemorySurface);
//		RELEASE(m_Palette);
//		return FALSE;
//	}
//
//    // Get the texture handle
//#ifdef DX6
//    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&m_Texture));
//#else
//    IDirect3DTexture2 *Texture;
//    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&Texture));
//    DXCALL(Texture->GetHandle(Device3D, &m_Handle));
//    RELEASE(Texture);
//#endif

	return TRUE;
}

#else

//BOOL CTexture::Create(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
//						void *Bits,DWORD Width,DWORD Height,
//						PALETTEENTRY *PaletteEntries,
//						TFORMAT TexFormat,
//						UBYTE Alpha,void *AlphaMap,
//						DWORD Scale)


// Need to test with alpha maps.
//
BOOL CTexture::Create(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
						MatDesc *Desc)
{
	HRESULT ddrval;
    SURFACEDESC ddsd;
	int TexFormatIndex;
	BOOL IsHardware;
	BOOL MaxSizeValid = FALSE;

	TFORMAT TexFormat = Desc->TexFormat;
	void *Bits = Desc->Bits;
	DWORD Width = Desc->Width;
	DWORD Height = Desc->Height;
	PALETTEENTRY *PaletteEntries = Desc->PaletteEntries;
	UBYTE Alpha = Desc->Alpha;
	void *AlphaMap = Desc->AlphaMap;
	DWORD Scale = Desc->Scale;
	DWORD PalCaps;

	m_Width = Width;
	m_Height = Height;
	
	// Are we using a hardware renderer.
    D3DDEVICEDESC hal, hel;
    memset(&hal,0,sizeof(hal));
    hal.dwSize = sizeof(hal);
    memset(&hel,0,sizeof(hel));
    hel.dwSize = sizeof(hel);
    Device3D->GetCaps(&hal, &hel);
	IsHardware = hal.dcmColorModel ? TRUE : FALSE;

	if(IsHardware) {
		// If the max texture width and height device caps are valid then check the
		// texture width and height.
		DWORD MaxWidth = hal.dwMaxTextureWidth;
		DWORD MaxHeight = hal.dwMaxTextureHeight;
		if((MaxWidth > 0) && (MaxHeight > 0)) {
			MaxSizeValid = TRUE;
			if(Width > MaxWidth) {
//				Width = MaxWidth;
				return FALSE;
			}
			if(Height > MaxHeight) {
//				Height = MaxHeight;
				return FALSE;
			}
		}
	}

	PalCaps = DDPCAPS_8BIT;
#ifdef DX6
	if(g_HalCaps.dwPalCaps & DDPCAPS_ALPHA) {
		// Supports Alpha in palettes.
		PalCaps |= DDPCAPS_ALPHA;
	} else {
		// Dos'nt support alpha in palettes and colour keying or alpha blending was
		// asked for and a palettized format was asked for then change the asked for format
		// to an RGB alpha format.
		if(TexFormat == TF_PAL8BIT) {
			if(Desc->AlphaTypes & AT_ALPHABLEND) {
				TexFormat = TF_RGBALPHA16BIT;
			} else if(Desc->AlphaTypes & AT_COLOURKEY) {
				TexFormat = TF_RGB1ALPHA16BIT;
			}
		}
	}
#endif

	// First try for the requested format.
	TexFormatIndex = MatchFormat(TexFormat,0);

	// If that failed and they want alpha blending then try for a 16 bit RGBA format with >1 alpha bits.
	// Note, they may want colour keying as well and thats alright because we can still do colour keying
	// with a >1 bit alpha texture.
	if(TexFormatIndex < 0) {
		if( Desc->AlphaTypes & AT_ALPHABLEND ) {
			TexFormatIndex = MatchFormat(TF_RGBALPHA16BIT,0);
		}
	}

	// If that failed and they want colour keying then try for a 16 bit RGBA format with 1 alpha bit.
	if(TexFormatIndex < 0) {
		if( Desc->AlphaTypes & AT_COLOURKEY ) {
			TexFormatIndex = MatchFormat(TF_RGB1ALPHA16BIT,0);
		}
	}

	// If that failed then try for the first format in the list and pray it works.
	if(TexFormatIndex < 0) {
		TexFormatIndex = MatchFormat(TF_ANY,0);
	}

	// Everything failed so give up and go home.
	if(TexFormatIndex < 0) {
		return FALSE;
	}

	DebugPrint("MatchFormat() = %s\n",g_TextureList.List[TexFormatIndex].PixelFormatExt.Name);

    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
	ddsd.ddpfPixelFormat = g_TextureList.List[TexFormatIndex].PixelFormat;
    ddsd.dwFlags |= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = Width;
    ddsd.dwHeight = Height;

	// Create the device surface , in video memory if hardware, else in system memory.
    if(IsHardware) {
        ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD;
	    ddrval = DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL);
		if(ddrval == DDERR_OUTOFVIDEOMEMORY) {
			DisplayError(ddrval,__FILE__,__LINE__);
			return FALSE;
		} else if(ddrval != DD_OK) {
			DisplayError(ddrval,__FILE__,__LINE__);
			return FALSE;
		}

		// Check to see if it went into VRAM ( needs to with most 3d hardware ).
		SURFACECAPS SurfaceCaps;
		DXCALL(m_DeviceSurface->GetCaps(&SurfaceCaps));
		if( !(SurfaceCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) {
			if( !(hal.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) ) {
				RELEASE(m_DeviceSurface);
				return FALSE;
			}
		}
	} else {
        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY| DDSCAPS_TEXTURE;
	    DXCALL(DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL));
	}

	// If hardware then create the system memory surface for the source texture.
	if(IsHardware) {
        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
        if ((ddrval = DirectDraw->CreateSurface(&ddsd, &m_MemorySurface, NULL)) != DD_OK) {
			DisplayError(ddrval,__FILE__,__LINE__,0);
			RELEASE(m_DeviceSurface);
			return FALSE;
		}

		if(!MaxSizeValid) {
			// Get a texture interface and handle to test if we can create a texture this size.
			LPDIRECT3DTEXTURE2	TmpTex=NULL;
			DXCALL(m_MemorySurface->QueryInterface(IID_IDirect3DTexture2,(LPVOID *)&TmpTex));

			// If the texture creation failed then try again with a smaller scale.
			if( (ddrval == D3DERR_TEXTURE_CREATE_FAILED) || (ddrval == D3DERR_TEXTURE_BADSIZE) ) {
					DebugPrint(" Texture create failed!\n");
					DisplayError(ddrval,__FILE__,__LINE__);

					RELEASE(m_DeviceSurface);
					RELEASE(m_MemorySurface);
					RELEASE(TmpTex);
					return FALSE;
			} else if(ddrval != DD_OK) {
					DisplayError(ddrval,__FILE__,__LINE__);
					RELEASE(m_DeviceSurface);
					RELEASE(m_MemorySurface);
					RELEASE(TmpTex);
					return FALSE;
			}

			RELEASE(TmpTex);
		}
	} else {
        m_MemorySurface = m_DeviceSurface;
        m_DeviceSurface->AddRef();
	}
	
	// Do pre-processing on the source textures palette.
   	DWORD *pe = (DWORD*)PaletteEntries;
   	DWORD a;
   	// If they want alpha blending then set the alpha level in the palette.
   	if(Desc->AlphaTypes & AT_ALPHABLEND) {
   		a = Alpha << 24;
   	} else {
   		// Otherwise default to alpha = 255.
   		a = 0xff000000;
   	}
	for( int i=0; i<256; i++ ) {
		*pe = a + RGB( GetRValue(*pe), GetGValue(*pe),
								  GetBValue(*pe) );

		// If they want colour keying then check for the palette entry matching the key colour
		// and set it's alpha to zero if it does.
//		if(Desc->AlphaTypes & AT_COLOURKEY) {
		if(Desc->ColourKey >= 0) {
			if((*pe&0x00ffffff) == Desc->ColourKey ) {
				// Set the RGB and Alpha to zero.
				*pe = 0x00000000;
			}
		}

		pe++;
   	}

	// If it's a palettized texture then create the palette and attach to the surfaces.
    if(ddsd.ddpfPixelFormat.dwRGBBitCount <= 8) {
//#ifdef DX6
//		DWORD *pe = (DWORD*)PaletteEntries;
//		DWORD a;
//
//		// If they want alpha blending then set the alpha level in the palette.
//		if(Desc->AlphaTypes & AT_ALPHABLEND) {
//			a = Alpha << 24;
//		} else {
//			// Otherwise default to alpha = 255.
//			a = 0xff000000;
//		}
//
//        for( int i=0; i<256; i++ ) {
//            *pe = a + RGB( GetRValue(*pe), GetGValue(*pe),
//                                      GetBValue(*pe) );
//
//			// If they want colour keying then check for the palette entry matching the key colour
//			// and set it's alpha to zero if it does.
//			if(Desc->AlphaTypes & AT_COLOURKEY) {
//				if((*pe&0x00ffffff) == Desc->ColourKey ) {
//					// Set the RGB and Alpha to zero.
//					*pe = 0x00000000;
////					*pe &= 0x00ffffff;
//				}
//			}
//
//			pe++;
//		}
//#endif
        // Create & attach a palette with the bitmap's colors
		DirectDraw->CreatePalette(PalCaps, PaletteEntries, &m_Palette, NULL);
        m_MemorySurface->SetPalette(m_Palette);
        m_DeviceSurface->SetPalette(m_Palette);
    }

	DWORD AlphaTmp;
	if( (Desc->AlphaTypes & AT_COLOURKEY) && (!(Desc->AlphaTypes & AT_ALPHABLEND)) ) {
		// If they only want colour keying then pass 255 as the alpha into FillSurface.
		AlphaTmp = 255;
	} else if(Desc->AlphaTypes & AT_ALPHABLEND) {
		// If they want alpha blending (and possibly colour keying) then pass the users alpha value
		// into FillSurface.
		AlphaTmp = Alpha;
	} else {
		// No alpha blending or colour keying so pass 255 as the alpha into FillSurface,
		AlphaTmp = 255;
	}

	// Copy the bitmap to the system memory surface, this could probably be done with
	// a Blt but we want to be able to play with the textures alpha channels so we do it ourselves.
	if(!FillSurface(Bits,Width,Height,
					PaletteEntries,Desc->ColourKey,AlphaTmp,AlphaMap,
					m_MemorySurface,
					&g_TextureList.List[TexFormatIndex].PixelFormatExt)) {
		RELEASE(m_DeviceSurface);
		RELEASE(m_MemorySurface);
		RELEASE(m_Palette);
		return FALSE;
	}

	// Restore the video memory surface and load texture into it from system memory surface.
	ddrval = Restore();
	if(ddrval != DD_OK) {
		// If the load failed then could be the card can't handle textures at this size
		// so change the scale and try again.
		DisplayError(ddrval,__FILE__,__LINE__);
		RELEASE(m_DeviceSurface);
		RELEASE(m_MemorySurface);
		RELEASE(m_Palette);
		return FALSE;
	}

#ifdef DX6
    // Get the texture interface pointer.
    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&m_Texture));
#else
    // Get the texture handle.
    IDirect3DTexture2 *Texture;
    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&Texture));
    DXCALL(Texture->GetHandle(Device3D, &m_Handle));
    RELEASE(Texture);
#endif

	return TRUE;
}

#endif

#ifdef DX6
//BOOL CTexture::CreateMipMap(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
//						DWORD Count,void **Bits,DWORD Width,DWORD Height,
//						PALETTEENTRY *PaletteEntries,
//						TFORMAT TexFormat,
//						UBYTE Alpha,void *AlphaMap,
//						DWORD Scale)
BOOL CTexture::CreateMipMap(IDDRAW *DirectDraw,ID3D *Direct3D,ID3DDEVICE *Device3D,
						MatDesc *Desc)
{
	HRESULT ddrval;
    SURFACEDESC ddsd;
	int TexFormatIndex;
	BOOL IsHardware;
	BOOL MaxSizeValid = FALSE;

	DWORD Count = Desc->Count;
	void **Bits = Desc->MipMapBits;
	DWORD Width = Desc->Width;
	DWORD Height = Desc->Height;
	PALETTEENTRY *PaletteEntries = Desc->PaletteEntries;
	TFORMAT TexFormat = Desc->TexFormat;
	UBYTE Alpha = Desc->Alpha;
	void *AlphaMap = Desc->AlphaMap;
	DWORD Scale = Desc->Scale;

	m_Width = Width;
	m_Height = Height;

	// Are we using a hardware renderer.
    D3DDEVICEDESC hal, hel;
    memset(&hal,0,sizeof(hal));
    hal.dwSize = sizeof(hal);
    memset(&hel,0,sizeof(hel));
    hel.dwSize = sizeof(hel);
    Device3D->GetCaps(&hal, &hel);
	IsHardware = hal.dcmColorModel ? TRUE : FALSE;

	if(IsHardware) {
		// If the max texture width and height device caps are valid then check the
		// texture width against them and correct if necessary.
		DWORD MaxWidth = hal.dwMaxTextureWidth;
		DWORD MaxHeight = hal.dwMaxTextureHeight;
		if((MaxWidth > 0) && (MaxHeight > 0)) {
			MaxSizeValid = TRUE;
			if(Width > MaxWidth) Width = MaxWidth;
			if(Height > MaxHeight) Height = MaxHeight;
		}
	}

	TexFormatIndex = MatchFormat(TexFormat,0);

	if(TexFormatIndex < 0) {
		if( Desc->AlphaTypes & AT_ALPHABLEND ) {
			TexFormatIndex = MatchFormat(TF_RGBALPHA16BIT,0);
		}
	}

	if(TexFormatIndex < 0) {
		if( Desc->AlphaTypes & AT_COLOURKEY ) {
			TexFormatIndex = MatchFormat(TF_RGB1ALPHA16BIT,0);
		}
	}

	if(TexFormatIndex < 0) {
		TexFormatIndex = MatchFormat(TF_ANY,0);
	}

	if(TexFormatIndex < 0) {
		return FALSE;
	}

	DebugPrint("MatchFormat() = %s\n",g_TextureList.List[TexFormatIndex].PixelFormatExt.Name);

    memset(&ddsd,0,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
	ddsd.ddpfPixelFormat = g_TextureList.List[TexFormatIndex].PixelFormat;
    ddsd.dwFlags         = DDSD_CAPS | DDSD_MIPMAPCOUNT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    ddsd.dwMipMapCount   = Count;
    ddsd.dwWidth = Width;
    ddsd.dwHeight = Height;

	// Create the device surface , in video memory if hardware, else in system memory.
    if(IsHardware) {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
	    ddrval = DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL);
		if(ddrval != DD_OK) {
			DisplayError(ddrval,__FILE__,__LINE__);
			return FALSE;
		}

		// Check to see if it went into VRAM ( needs to with most 3d hardware ).
		SURFACECAPS SurfaceCaps;
		DXCALL(m_DeviceSurface->GetCaps(&SurfaceCaps));
		if( !(SurfaceCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) {
			if( !(hal.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) ) {
				RELEASE(m_DeviceSurface);
				return FALSE;
			}
		}
	} else {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	    DXCALL(DirectDraw->CreateSurface(&ddsd, &m_DeviceSurface, NULL));
	}

	// If hardware then create the system memory surface for the source texture.
	if(IsHardware) {
	    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        if ((ddrval = DirectDraw->CreateSurface(&ddsd, &m_MemorySurface, NULL)) != DD_OK) {
			DisplayError(ddrval,__FILE__,__LINE__,0);
			RELEASE(m_DeviceSurface);
			return FALSE;
		}

		if(!MaxSizeValid) {
			// Get a texture interface and handle to test if we can create a texture this size.
			LPDIRECT3DTEXTURE2	TmpTex=NULL;
			DXCALL(m_MemorySurface->QueryInterface(IID_IDirect3DTexture2,(LPVOID *)&TmpTex));

			// If the texture creation failed then try again with a smaller scale.
			if( (ddrval == D3DERR_TEXTURE_CREATE_FAILED) || (ddrval == D3DERR_TEXTURE_BADSIZE) ) {
					DisplayError(ddrval,__FILE__,__LINE__);
					RELEASE(m_DeviceSurface);
					RELEASE(m_MemorySurface);
					RELEASE(TmpTex);
					return FALSE;
			} else if(ddrval != DD_OK) {
					DisplayError(ddrval,__FILE__,__LINE__);
					RELEASE(m_DeviceSurface);
					RELEASE(m_MemorySurface);
					RELEASE(TmpTex);
					return FALSE;
			}

			RELEASE(TmpTex);
		}
	} else {
        m_MemorySurface = m_DeviceSurface;
        m_DeviceSurface->AddRef();
	}

	// Create the palette and attach to the surfaces.
    if(ddsd.ddpfPixelFormat.dwRGBBitCount <= 8)
    {
#ifdef DX6
		DWORD *pe = (DWORD*)PaletteEntries;
		DWORD a;

		if(Desc->AlphaTypes & AT_ALPHABLEND) {
			a = Alpha << 24;
		} else {
			a = 0xff000000;
		}

        for( int i=0; i<256; i++ ) {
            *pe = a + RGB( GetRValue(*pe), GetGValue(*pe),
                                      GetBValue(*pe) );

			// Set alpha for transparent pixels
			if(Desc->AlphaTypes & AT_COLOURKEY) {
				if((*pe&0x00ffffff) == Desc->ColourKey ) {
					*pe &= 0x00ffffff;
				}
			}

			pe++;
		}
        // Create & attach a palette with the bitmap's colors
		DirectDraw->CreatePalette(DDPCAPS_8BIT|DDPCAPS_ALPHA, PaletteEntries, &m_Palette, NULL);
#else
        DirectDraw->CreatePalette(DDPCAPS_8BIT, PaletteEntries, &m_Palette, NULL);
#endif
        m_MemorySurface->SetPalette(m_Palette);
        m_DeviceSurface->SetPalette(m_Palette);
    }

	DWORD AlphaTmp;
	if( (Desc->AlphaTypes & AT_COLOURKEY) && (!(Desc->AlphaTypes & AT_ALPHABLEND)) ) {
		AlphaTmp = 255;
	} else {
		AlphaTmp = Alpha;
	}

	IDDSURFACE *DstSurface = m_MemorySurface;
	int MipWidth = Width;
	int MipHeight = Height;
	for(int i=0; i<Count; i++) {
		// Copy the bitmap to the surface system memory surface.
		if(!FillSurface(Bits[i],MipWidth,MipHeight,
						PaletteEntries,Desc->ColourKey,AlphaTmp,AlphaMap,
						DstSurface,
						&g_TextureList.List[TexFormatIndex].PixelFormatExt)) {
			RELEASE(m_DeviceSurface);
			RELEASE(m_MemorySurface);
			RELEASE(m_Palette);
			return FALSE;
		}

		MipWidth /= 2;
		MipHeight /= 2;
        // Get the next surface in the chain. Do a Release() call, though, to
        // avoid increasing the ref counts on the surfaces.
        SURFACECAPS ddsCaps;
        ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
        if(DstSurface->GetAttachedSurface(&ddsCaps,&DstSurface) == DD_OK) {
			DstSurface->Release();
		}
	}

	// Restore the video memory surface and load texture into it from system memory surface.
	ddrval = Restore();
	if(ddrval != DD_OK) {
		// If the load failed then could be the card can't handle textures at this size
		// so change the scale and try again.
		DisplayError(ddrval,__FILE__,__LINE__);
		RELEASE(m_DeviceSurface);
		RELEASE(m_MemorySurface);
		RELEASE(m_Palette);
		return FALSE;
	}

    // Get the texture handle
#ifdef DX6
    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&m_Texture));
#else
    IDirect3DTexture2 *Texture;
    DXCALL(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&Texture));
    DXCALL(Texture->GetHandle(Device3D, &m_Handle));
    RELEASE(Texture);
#endif

	return TRUE;
}
#endif


BOOL CTexture::SetColourKey(SWORD ColourKeyIndex)
{
	HRESULT ddrval;
	DDCOLORKEY ColourKey;

	if (ColourKeyIndex != -1) {
		ColourKey.dwColorSpaceLowValue = ColourKeyIndex;
		ColourKey.dwColorSpaceHighValue = ColourKeyIndex;
		DXCALL(m_DeviceSurface->SetColorKey(DDCKEY_SRCBLT, &ColourKey));
	}

	return TRUE;
}


HRESULT CTexture::Restore(void)
{
    HRESULT ddrval;
    IDirect3DTexture2  *MemoryTexture;
    IDirect3DTexture2  *DeviceTexture;

    if(m_DeviceSurface == NULL || m_MemorySurface == NULL) {
        return DDERR_GENERIC;
	}

    // we dont need to do this step for system memory surfaces.
    if(m_DeviceSurface == m_MemorySurface) {
        return DD_OK;
	}

    // restore the video memory texture surface.
    DXCALL_HRESULT(m_DeviceSurface->Restore());

    // call IDirect3DTexture::Load() to copy the texture to the device.
    DXCALL_HRESULT(m_DeviceSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&DeviceTexture));
    DXCALL_HRESULT(m_MemorySurface->QueryInterface(IID_IDirect3DTexture2, (void**)&MemoryTexture));

    ddrval = DeviceTexture->Load(MemoryTexture);

    RELEASE(DeviceTexture);
    RELEASE(MemoryTexture);

    return ddrval;
}


// ---- CMatManager -----------------------------------------------------------------------------
//


CMatManager::CMatManager(CDirectDraw *DirectDraw)
{
	m_DirectDraw = DirectDraw;

	for(int i=0; i<MAX_MATERIALS; i++) {
		memset(&m_Materials[i],0,sizeof(MaterialData));
	}

	m_NumMats = 0;
}


CMatManager::~CMatManager(void)
{
	for(int MaterialID=0; MaterialID<MAX_MATERIALS; MaterialID++) {
		if(m_Materials[MaterialID].InUse) {
			DELOBJ(m_Materials[MaterialID].Name);
			DELOBJ(m_Materials[MaterialID].Texture);
			DELOBJ(m_Materials[MaterialID].Material);
			memset(&m_Materials[MaterialID],0,sizeof(MaterialData));
		}
	}
}


BOOL CMatManager::FindMaterial(char *Name,DWORD *MatID)
{
	// Does the material already exist?
   	for(int i=0; i<MAX_MATERIALS; i++) {
   		if(m_Materials[i].Name) {
   			if(m_Materials[i].InUse) {
   				if(strcmp(m_Materials[i].Name,Name) == 0) {
   					// If it does then just increment it's reference count and
   					// return it's id.
   					m_Materials[i].InUse++;
//					DebugPrint("FindMaterial : Found %s\n",Name);
					*MatID = i;
					return TRUE;
   				}
   			}
   		}
   	}

	return FALSE;
}

//DWORD CMatManager::CreateMaterial(DWORD Type,char* Name,void *Bits,
//										DWORD Count,void **MipMapBits,
//										DWORD Width,DWORD Height,
//										PALETTEENTRY *PaletteEntries,
//										UBYTE Alpha,void *AlphaMap,
//										TFORMAT TexFormat,
//										D3DCOLORVALUE *Ambient,
//										D3DCOLORVALUE *Diffuse,
//										D3DCOLORVALUE *Specular,
//										DWORD Scale)
DWORD CMatManager::CreateMaterial(MatDesc *Desc)
{
//	Scale = 4;

	DWORD Type = Desc->Type;
	char *Name = Desc->Name;
	void *Bits = Desc->Bits;
	DWORD Count = Desc->Count;
	void **MipMapBits = Desc->MipMapBits;
	DWORD Width = Desc->Width;
	DWORD Height = Desc->Height;
	PALETTEENTRY *PaletteEntries = Desc->PaletteEntries;
	UBYTE Alpha = Desc->Alpha;
	void *AlphaMap = Desc->AlphaMap;
	TFORMAT TexFormat = Desc->TexFormat;
	D3DCOLORVALUE *Ambient = &Desc->Ambient;
	D3DCOLORVALUE *Diffuse = &Desc->Diffuse;
	D3DCOLORVALUE *Specular = &Desc->Specular;
	DWORD Scale = Desc->Scale;

	ASSERT(Name!=NULL);

	// Does the material already exist?
   	for(int i=0; i<MAX_MATERIALS; i++) {
   		if(m_Materials[i].Name) {
   			if(m_Materials[i].InUse) {
   				if(strcmp(m_Materials[i].Name,Name) == 0) {
   					// If it does then just increment it's reference count and
   					// return it's id.
   					m_Materials[i].InUse++;
					DebugPrint("CreateMaterial : Texture already loaded %s\n",Name);
   					return i;
   				}
   			}
   		}
   	}

	// Find an empty material slot.
	for(DWORD ThisMat=0; ThisMat<m_NumMats; ThisMat++) {
		if(m_Materials[ThisMat].InUse == 0) {
			break;
		}
	}

	if(ThisMat >= m_NumMats) {
		m_NumMats = ThisMat+1;
	}

	CTexture *Texture = NULL;
	CTexture *MipMap = NULL;
#ifdef DX6
	ID3DTEXTURE *TextureInterface = NULL;
#else
	D3DTEXTUREHANDLE TextureHandle = 0;
#endif
	CMaterial *Material = NULL;
	D3DMATERIALHANDLE MaterialHandle = 0;


	switch(Type) {
		case	MT_TEXTURED:
			if(Bits) {
				Texture = new CTexture;
				Texture->Create(m_DirectDraw->GetDevice(),
								m_DirectDraw->GetDirect3D(),
								m_DirectDraw->Get3DDevice(),
								Desc);
//				Texture->Create(m_DirectDraw->GetDevice(),m_DirectDraw->GetDirect3D(),m_DirectDraw->Get3DDevice(),
//									Bits,Width,Height,
//									PaletteEntries,TexFormat,
//									Alpha,AlphaMap,Scale);
		#ifdef DX6
				TextureInterface = Texture->GetTexture();
		#else
				TextureHandle = Texture->GetHandle();
		#endif
			}

			break;
#ifdef DX6
		case	MT_MIPMAPED:
			if(MipMapBits) {
				MipMap = new CTexture;
//				MipMap->CreateMipMap(m_DirectDraw->GetDevice(),m_DirectDraw->GetDirect3D(),m_DirectDraw->Get3DDevice(),
//									Count,MipMapBits,Width,Height,
//									PaletteEntries,TexFormat,
//									Alpha,AlphaMap,Scale);
				MipMap->CreateMipMap(m_DirectDraw->GetDevice(),
									m_DirectDraw->GetDirect3D(),
									m_DirectDraw->Get3DDevice(),
									Desc);
				TextureInterface = MipMap->GetTexture();
			}
			break;
#endif
	}

	Material = new CMaterial;
#ifdef DX6
	Material->Create(m_DirectDraw->GetDirect3D(),m_DirectDraw->Get3DDevice(),
					Ambient,Diffuse,Specular,1.0F,NULL,0);
#else
	Material->Create(m_DirectDraw->GetDirect3D(),m_DirectDraw->Get3DDevice(),
					Ambient,Diffuse,Specular,1.0F,NULL,TextureHandle);
#endif
	MaterialHandle = Material->GetHandle();

// Fill in the material info structure.
	m_Materials[ThisMat].Name = new char[strlen(Name)+1];
	strcpy(m_Materials[ThisMat].Name,Name);
   	m_Materials[ThisMat].Texture = Texture;
   	m_Materials[ThisMat].Material = Material;
#ifdef DX6
   	m_Materials[ThisMat].TexInterface = TextureInterface;
#else
   	m_Materials[ThisMat].TexHandle = TextureHandle;
#endif
   	m_Materials[ThisMat].MatHandle = MaterialHandle;
	m_Materials[ThisMat].InUse++;

	return ThisMat;
}


int CMatManager::GetMaterialID(char* Name)
{
	ASSERT(Name!=NULL);

	for(int i=0; i<MAX_MATERIALS; i++) {
		if(m_Materials[i].Name) {
			if(m_Materials[i].InUse) {
				if(strcmp(m_Materials[i].Name,Name) == 0) {
					return i;
				}
			}
		}
	}

	return -1;
}


BOOL CMatManager::DeleteMaterial(DWORD MaterialID)
{
	m_Materials[MaterialID].InUse--;

	if(m_Materials[MaterialID].InUse==0) {
		DELOBJ(m_Materials[MaterialID].Name);
		DELOBJ(m_Materials[MaterialID].Texture);
		DELOBJ(m_Materials[MaterialID].Material);
		memset(&m_Materials[MaterialID],0,sizeof(MaterialData));
	}

	return TRUE;
}

BOOL CMatManager::SetColourKey(DWORD MaterialID,SWORD ColourKeyIndex)
{
	m_Materials[MaterialID].Texture->SetColourKey(ColourKeyIndex);

	return TRUE;
}

BOOL CMatManager::SetMaterialColour(DWORD MaterialID,
						D3DCOLORVALUE *Ambient,
						D3DCOLORVALUE *Diffuse,
						D3DCOLORVALUE *Specular)
{
	m_Materials[MaterialID].Material->SetMaterialColour(Ambient,Diffuse,Specular);

	return TRUE;
}


BOOL CMatManager::GetMaterialColour(DWORD MaterialID,
						D3DCOLORVALUE *Ambient,
						D3DCOLORVALUE *Diffuse,
						D3DCOLORVALUE *Specular)
{
	m_Materials[MaterialID].Material->GetMaterialColour(Ambient,Diffuse,Specular);

	return TRUE;
}


D3DMATERIALHANDLE CMatManager::GetMaterialHandle(DWORD MaterialID)
{
	return m_Materials[MaterialID].MatHandle;
}

#ifdef DX6
ID3DTEXTURE *CMatManager::GetTextureInterface(DWORD MaterialID)
{
	return m_Materials[MaterialID].TexInterface;
}
#else
D3DTEXTUREHANDLE CMatManager::GetTextureHandle(DWORD MaterialID)
{
	return m_Materials[MaterialID].TexHandle;
}
#endif

BOOL CMatManager::ReleaseMaterials(void)
{
	return TRUE;
}


BOOL CMatManager::RestoreMaterials(void)
{
    HRESULT ddrval;

	DebugPrint("Restoring materials.\n");

	for(int MaterialID=0; MaterialID<MAX_MATERIALS; MaterialID++) {
		if(m_Materials[MaterialID].InUse) {
			if(m_Materials[MaterialID].Texture) {
				DXCALL(m_Materials[MaterialID].Texture->Restore());
			}
		}
	}

	return TRUE;
}


BOOL CMatManager::BeginScene(void)
{
	HRESULT ddrval;

	if( (ddrval=m_DirectDraw->BeginScene()) == DDERR_SURFACELOST) {

		// Surfaces lost so...

		m_DirectDraw->RestoreSurfaces();	// Restore display surfaces.
		RestoreMaterials();					// Restore material textures.

		DXCALL(m_DirectDraw->BeginScene());	// And try again.
	}

	return TRUE;
}


BOOL CMatManager::EndScene(void)
{
	return m_DirectDraw->EndScene();
}


// Transform and light and render.

#ifdef DX6
BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,

								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}

// Transform and render.

BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}

// Render.

BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexInterface,
								Vertex,Count);
}
#else
BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,

								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}

// Transform and render.

BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}

// Render.

BOOL CMatManager::DrawPoint(int MaterialID,
		  				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPoint(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawLine(int MaterialID,
		   				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawPolyLine(int MaterialID,
		   				D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawPolyLine(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangle(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangle(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleFan(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleFan(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}


BOOL CMatManager::DrawTriangleStrip(int MaterialID,
	   					D3DTLVERTEX *Vertex,DWORD Count)
{
	return m_DirectDraw->DrawTriangleStrip(m_Materials[MaterialID].MatHandle,m_Materials[MaterialID].TexHandle,
								Vertex,Count);
}
#endif

// ---- CSurface --------------------------------------------------------------------------------
//

// Given an existing direct draw surface, construct a surface class to
// encapsulate it.
//
CSurface::CSurface(IDDSURFACE *Surface,PALETTEENTRY *PaletteEntries)
{
	m_Surface=NULL;
	m_PaletteEntries=NULL;
	m_RGBLookup=NULL;
	m_RedLookup = NULL;
	m_GreenLookup = NULL;
	m_BlueLookup = NULL;
	m_hdc=NULL;
	m_IsLocked=FALSE;

	HRESULT	ddrval;
	m_Surface=Surface;

    memset(&m_ddsd, 0, sizeof(SURFACEDESC));
    m_ddsd.dwSize = sizeof(SURFACEDESC);
    ddrval = Surface->GetSurfaceDesc(&m_ddsd);

	if(PaletteEntries) {
		m_PaletteEntries=new PALETTEENTRY[256];
		memcpy(m_PaletteEntries,PaletteEntries,sizeof(PALETTEENTRY)*256);
	}

	DWORD	i;

	m_BitCount=m_ddsd.ddpfPixelFormat.dwRGBBitCount;
	if(m_BitCount > 8) {
		ULONG Tmp;

		m_RBitMask=m_ddsd.ddpfPixelFormat.dwRBitMask;
		m_GBitMask=m_ddsd.ddpfPixelFormat.dwGBitMask;
		m_BBitMask=m_ddsd.ddpfPixelFormat.dwBBitMask;

   		m_RedShift=0;
   		Tmp=m_RBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			m_RedShift++;
   		}
   		m_RedBits=0;
   		Tmp=m_RBitMask;
   		for(i=0; i<m_ddsd.ddpfPixelFormat.dwRGBBitCount; i++) {
   			if(Tmp&1) m_RedBits++;
   			Tmp>>=1;
   		}

		m_GreenShift=0;
   		Tmp=m_GBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			m_GreenShift++;
   		}
   		m_GreenBits=0;
   		Tmp=m_GBitMask;
   		for(i=0; i<m_ddsd.ddpfPixelFormat.dwRGBBitCount; i++) {
   			if(Tmp&1) m_GreenBits++;
   			Tmp>>=1;
   		}

   		m_BlueShift=0;
   		Tmp=m_BBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			m_BlueShift++;
   		}
   		m_BlueBits=0;
   		Tmp=m_BBitMask;
   		for(i=0; i<m_ddsd.ddpfPixelFormat.dwRGBBitCount; i++) {
   			if(Tmp&1) m_BlueBits++;
   			Tmp>>=1;
   		}

		if(PaletteEntries) {
			m_RGBLookup=new ULONG[256];
			ULONG	r,g,b;

			for(i=0; i<256; i++) {
				r=(ULONG)(PaletteEntries[i].peRed >> (8-m_RedBits));
				r<<=m_RedShift;
				r&=m_RBitMask;

				g=(ULONG)(PaletteEntries[i].peGreen >> (8-m_GreenBits));
				g<<=m_GreenShift;
				g&=m_GBitMask;

				b=(ULONG)(PaletteEntries[i].peBlue >> (8-m_BlueBits));
				b<<=m_BlueShift;
				b&=m_BBitMask;

				m_RGBLookup[i]=r|g|b;
			}
		}

		if(m_BitCount==16) {
			m_RedLookup = new UWORD[256];
			m_GreenLookup = new UWORD[256];
			m_BlueLookup = new UWORD[256];

			for(i=0; i<256; i++) {
				m_RedLookup[i] = (UWORD)(i<<m_RedShift);
				m_RedLookup[i] &= m_RBitMask;
				m_GreenLookup[i] = (UWORD)(i<<m_GreenShift);
				m_GreenLookup[i] &= m_GBitMask;
				m_BlueLookup[i] = (UWORD)(i<<m_BlueShift);
				m_BlueLookup[i] &= m_BBitMask;
			}
		}
	}
}


CSurface::~CSurface()
{
	if(m_PaletteEntries) delete m_PaletteEntries;
	if(m_RGBLookup) delete m_RGBLookup;
	if(m_RedLookup) delete m_RedLookup;
	if(m_GreenLookup) delete m_GreenLookup;
	if(m_BlueLookup) delete m_BlueLookup;
}


//void CSurface::PutPixel(SLONG XCoord,SLONG YCoord,UWORD Index)
//{
//	if( (XCoord >= 0) && (XCoord < (SLONG)m_ddsd.dwWidth) &&
//		(YCoord >= 0) && (YCoord < (SLONG)m_ddsd.dwHeight) ) {
//   		if(m_BitCount==8) {
//
//			*(((UBYTE*)m_ddsd.lpSurface)+m_ddsd.lPitch*YCoord+XCoord)=(UBYTE)Index;
//
//		} else if(m_BitCount==16) {
//
//			*(UWORD*)(((UBYTE*)m_ddsd.lpSurface)+m_ddsd.lPitch*YCoord+(XCoord<<1)) =
//				(UWORD)m_RGBLookup[Index];
//
//		} else if(m_ddsd.ddpfPixelFormat.dwRGBBitCount==32) {
//
//			*(ULONG*)(((UBYTE*)m_ddsd.lpSurface)+m_ddsd.lPitch*YCoord+(XCoord<<2)) =
//				m_RGBLookup[Index];
//
//		}
//	}
//}


UWORD CSurface::GetValue(UBYTE Red,UBYTE Green,UBYTE Blue)
{
	return m_RedLookup[Red] | m_GreenLookup[Green] | m_BlueLookup[Blue];
}


void CSurface::PutPixel(SLONG XCoord,SLONG YCoord,UBYTE Red,UBYTE Green,UBYTE Blue)
{
	if(m_BitCount==16) {
		if( (XCoord >= 0) && (XCoord < (SLONG)m_ddsd.dwWidth) &&
			(YCoord >= 0) && (YCoord < (SLONG)m_ddsd.dwHeight) ) {
			UWORD *Ptr = (UWORD*)(((UBYTE*)m_ddsd.lpSurface)+m_ddsd.lPitch*YCoord+(XCoord<<1));

			*Ptr = m_RedLookup[Red] | m_GreenLookup[Green] | m_BlueLookup[Blue];
		}
	}
}


void CSurface::PutPixel(SLONG XCoord,SLONG YCoord,UWORD Value)
{
	if(m_BitCount==16) {
		if( (XCoord >= 0) && (XCoord < (SLONG)m_ddsd.dwWidth) &&
			(YCoord >= 0) && (YCoord < (SLONG)m_ddsd.dwHeight) ) {
			UWORD *Ptr = (UWORD*)(((UBYTE*)m_ddsd.lpSurface)+m_ddsd.lPitch*YCoord+(XCoord<<1));

			*Ptr = Value;
		}
	}
}


void CSurface::Clear(UWORD Index)
{
	DDBLTFX     ddbltfx;
	HRESULT     ddrval;

	ddbltfx.dwSize = sizeof( ddbltfx );
	ddbltfx.dwFillColor=Index;
	ddrval = m_Surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddbltfx );
}


void *CSurface::Lock(void)
{
    HRESULT     ddrval;

	ASSERT(m_IsLocked==FALSE);

	memset(&m_ddsd,0,sizeof(m_ddsd));
	m_ddsd.dwSize=sizeof(m_ddsd);
	ddrval=m_Surface->Lock(NULL,&m_ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL);

	m_IsLocked=TRUE;

	return((void*)m_ddsd.lpSurface);
}


void CSurface::Unlock(void)
{
    HRESULT     ddrval;

	ASSERT(m_IsLocked==TRUE);

	ddrval=m_Surface->Unlock(NULL);

	m_IsLocked=FALSE;
}


HDC CSurface::GetDC(void)
{
	HRESULT ddrval;

	ASSERT(m_hdc==NULL);

    ddrval=m_Surface->GetDC(&m_hdc);

	return(m_hdc);
}


void CSurface::ReleaseDC(void)
{
	HRESULT ddrval;

	ASSERT(m_hdc!=NULL);

    ddrval=m_Surface->ReleaseDC(m_hdc);
}

// ---- Globals ----------------------------------------------------------------------------------
//

// Build a list of direct draw device names.
//
BOOL CALLBACK BuildDeviceListCallback(GUID* lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam)
{
	HRESULT ddrval;
	IDDRAW *Device;
	ID3D *Direct3D;
	DeviceList *DList = (DeviceList*)lParam;
	int DevIndex = DList->NumDevices;

    wsprintf(DList->List[DevIndex],"%s (%s)",szName, szDevice);
	DList->IsPrimary[DevIndex] = DevIndex ? FALSE : TRUE;
	DList->Guid[DevIndex] = lpGUID;
	DebugPrint("%p : %s\n",
				DList->Guid[DevIndex],
				DList->List[DevIndex]);

#ifdef DX6
	IDirectDraw *pDD;
	DirectDrawCreate(lpGUID, &pDD, NULL);
	// Query the DirectDraw driver for a IDirectDraw4 interface.
	pDD->QueryInterface(IID_IDirectDraw4, (void**)&Device);
	pDD->Release();

	DXCALL(Device->QueryInterface(IID_IDirect3D3, (void**)&Direct3D));
#else
	DXCALL(DirectDrawCreate(lpGUID, &Device, NULL));
	DXCALL(Device->QueryInterface(IID_IDirect3D2, (void**)&Direct3D));
#endif
	ASSERT(Direct3D == NULL);

	// Enumerate display modes for this device.
	DList->DisplayModes[DevIndex].MaxModes = MAX_DD_MODES;
	DList->DisplayModes[DevIndex].NumModes = 0;
	DXCALL(Device->EnumDisplayModes(0,NULL,(LPVOID*)&DList->DisplayModes[DevIndex],EnumModesCallback));

//    memset(&ddsd, 0, sizeof(SURFACEDESC));
//    ddsd.dwSize = sizeof(SURFACEDESC);
//    DXCALL(Device->GetDisplayMode(&ddsd));
//    WindowsBPP = ddsd.ddpfPixelFormat.dwRGBBitCount;

	// Enumerate available D3D devices for this device.
	DList->D3DDevices[DevIndex].MaxDevices = MAX_D3D_DEVICES;
	DList->D3DDevices[DevIndex].NumDevices = 0;
//	DList->D3DDevices[DevIndex].WindowsBPP = WindowsBPP;
	DList->D3DDevices[DevIndex].IsPrimary = DList->IsPrimary[DevIndex];
	DXCALL(Direct3D->EnumDevices(EnumD3DDevicesCallback, (LPVOID*)&DList->D3DDevices[DevIndex]));

	for(int j=0; j<DList->D3DDevices[DevIndex].NumDevices; j++) {
		DebugPrint("  %s : %s\n",
					DList->D3DDevices[DevIndex].List[j].Name,
					DList->D3DDevices[DevIndex].List[j].Description);
	}

	RELEASE(Direct3D);
	RELEASE(Device);

	DList->NumDevices++;
	if(DList->NumDevices == DList->MaxDevices) {
	    return DDENUMRET_CANCEL;
	}

    return DDENUMRET_OK;
}


// Find and create a direct draw device.
//
BOOL CALLBACK FindDeviceCallback(GUID* lpGUID, LPSTR szName, LPSTR szDevice, LPVOID lParam)
{
    char ach[128];
    DeviceHook *DHook = (DeviceHook*)lParam;

    wsprintf(ach,"%s (%s)",szName, szDevice);
//	DebugPrint("%p %s\n",lpGUID,szDevice);

	// If a name was specified then see if this device matches.
	if(DHook->Name) {
		if (lstrcmpi(DHook->Name, szDevice) == 0 ||
			lstrcmpi(DHook->Name, ach) == 0) {
#ifdef DX6
			IDirectDraw *pDD;
			DirectDrawCreate(lpGUID, &pDD, NULL);
			pDD->QueryInterface( IID_IDirectDraw4, (void**)&DHook->Device);
			pDD->Release();
#else
			DirectDrawCreate(lpGUID, &DHook->Device, NULL);
#endif
			g_IsPrimary = lpGUID ? FALSE :TRUE;
			return DDENUMRET_CANCEL;
		}
	} else {
	// Currently bodged so that if it's not the primary and we've requested
	// a full screen mode then it's created ( assumes 3dfx ).

	// Need to create the device, and see if it's caps match our requirements
		if(lpGUID && DHook->Accelerate3D && !DHook->Windowed3D) {	// Not the primary;
#ifdef DX6
			IDirectDraw *pDD;
			DirectDrawCreate(lpGUID, &pDD, NULL);
			// Query the DirectDraw driver for a IDirectDraw4 interface.
			pDD->QueryInterface( IID_IDirectDraw4, (void**)&DHook->Device);
			pDD->Release();
#else
			DirectDrawCreate(lpGUID, &DHook->Device, NULL);
#endif
			g_IsPrimary = FALSE;

		}

	}

    return DDENUMRET_OK;
}


// Build a list of available video modes.
//
HRESULT CALLBACK EnumModesCallback(SURFACEDESC* lpDDSurfaceDesc,LPVOID lpContext)
{
	ModeList *DList = (ModeList*)lpContext;
	int ModeIndex = DList->NumModes;

// Only interested in 16 BPP modes.
	if(lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16) {
		DebugPrint("%d ",lpDDSurfaceDesc->dwWidth);
		DebugPrint("%d\n",lpDDSurfaceDesc->dwHeight);

		DList->Modes[ModeIndex].Width = lpDDSurfaceDesc->dwWidth;
		DList->Modes[ModeIndex].Height = lpDDSurfaceDesc->dwHeight;
		DList->Modes[ModeIndex].BPP = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

		DList->NumModes++;
		if(DList->NumModes == DList->MaxModes) {
			return DDENUMRET_CANCEL;
		}
	}

	return DDENUMRET_OK;
}


#ifdef DX6
HRESULT CALLBACK BuildTextureListCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lParam)
{
//    DDPIXELFORMAT ddpf = DeviceFmt->ddpfPixelFormat;
	TextureList *TList = (TextureList*)lParam;

	GetPixelFormatExt(lpDDPixFmt,&TList->List[TList->NumFormats].PixelFormatExt);
	TList->List[TList->NumFormats].PixelFormat = *lpDDPixFmt;

	DebugPrint("%s\n",TList->List[TList->NumFormats].PixelFormatExt.Name);

	TList->NumFormats++;

	if(TList->NumFormats == TList->MaxFormats) {
	    return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}
#else
// Build a list of available texture formats.
//
HRESULT CALLBACK BuildTextureListCallback(SURFACEDESC *DeviceFmt, LPVOID lParam)
{
    DDPIXELFORMAT ddpf = DeviceFmt->ddpfPixelFormat;
	TextureList *TList = (TextureList*)lParam;

	GetPixelFormatExt(&ddpf,&TList->List[TList->NumFormats].PixelFormatExt);
	TList->List[TList->NumFormats].PixelFormat = ddpf;

	DebugPrint("%s\n",TList->List[TList->NumFormats].PixelFormatExt.Name);

	TList->NumFormats++;

	if(TList->NumFormats == TList->MaxFormats) {
	    return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}
#endif

// Build list of available 3d devices.
//
HRESULT CALLBACK EnumD3DDevicesCallback(
			LPGUID guid, 
			LPSTR lpDeviceDescription,
			LPSTR lpDeviceName,
			LPD3DDEVICEDESC lpD3DHWDeviceDesc,
			LPD3DDEVICEDESC lpD3DHELDeviceDesc,
			LPVOID lpUserArg)
{
	D3DDeviceList *DList = (D3DDeviceList*)lpUserArg;
	D3DDevice *List = DList->List;
	int Index = DList->NumDevices;
	
    /*
     * Is this a hardware device or software emulation?  Checking the color
     * model for a valid model works.
     */
    if (lpD3DHWDeviceDesc->dcmColorModel) {
        List[Index].IsHardware = TRUE;
        memcpy(&List[Index].DeviceDesc, lpD3DHWDeviceDesc, sizeof(D3DDEVICEDESC));
    } else {
		if(!DList->IsPrimary) {
// If it's not a harware device and not on the primary display then skip it.
		    return D3DENUMRET_OK;
		}
        List[Index].IsHardware = FALSE;
        memcpy(&List[Index].DeviceDesc, lpD3DHELDeviceDesc, sizeof(D3DDEVICEDESC));
    }

    memcpy(&List[Index].Guid,guid,sizeof(GUID));
	strcpy(List[Index].Name,lpDeviceName);
	strcpy(List[Index].Description,lpDeviceDescription);
  
	List[Index].MaxBufferSize = 
        List[Index].DeviceDesc.dwMaxBufferSize;
	List[Index].MaxBufferVerts = 
        List[Index].DeviceDesc.dwMaxVertexCount;

	/*
     * Does this driver do texture mapping?
     */
    List[Index].DoesTextures =
        (List[Index].DeviceDesc.dpcTriCaps.dwTextureCaps &
         D3DPTEXTURECAPS_PERSPECTIVE) ? TRUE : FALSE;
    /*
     * Can this driver use a z-buffer?
     */
    List[Index].DoesZBuffer =
        List[Index].DeviceDesc.dwDeviceZBufferBitDepth
                ? TRUE : FALSE;
    /*
     * Can this driver render to the Windows display depth
     */
    List[Index].CanDoWindow =
        (List[Index].DeviceDesc.dwDeviceRenderBitDepth &
         BPPToDDBD(DList->WindowsBPP)) ? TRUE : FALSE;

// Need to check if it's the primary and set candowindow to FALSE.
	if(!DList->IsPrimary) {
	    List[Index].CanDoWindow = FALSE;
	}

	DebugPrint("\n  Name = %s\n",List[Index].Name);
	DebugPrint("  Description = %s\n",List[Index].Description);
	DebugPrint("  IsHardware = %s\n",List[Index].IsHardware ? "TRUE" : "FALSE");
	DebugPrint("  DoesTextures = %s\n",List[Index].DoesTextures ? "TRUE" : "FALSE");
	DebugPrint("  DoesZBuffer = %s\n",List[Index].DoesZBuffer ? "TRUE" : "FALSE");
	DebugPrint("  CanDoWindow = %s\n",List[Index].CanDoWindow ? "TRUE" : "FALSE");

	DList->NumDevices++;
	if(DList->NumDevices == DList->MaxDevices) {
	    return DDENUMRET_CANCEL;
	}

    return D3DENUMRET_OK;
}						  


#ifdef DX6
//-----------------------------------------------------------------------------
// Name: EnumZBufferFormatsCallback()
// Desc: Enumeration function to report valid pixel formats for z-buffers.
//-----------------------------------------------------------------------------
HRESULT CALLBACK EnumZBufferFormatsCallback( DDPIXELFORMAT* pddpf,void* pddpfDesired )
{
    if( NULL==pddpf || NULL==pddpfDesired )
        return D3DENUMRET_CANCEL;

    // If the current pixel format's match the desired ones (DDPF_ZBUFFER and
    // possibly DDPF_STENCILBUFFER), lets copy it and return. This function is
    // not choosy...it accepts the first valid format that comes along.
    if( pddpf->dwFlags == ((DDPIXELFORMAT*)pddpfDesired)->dwFlags )
    {
        memcpy( pddpfDesired, pddpf, sizeof(DDPIXELFORMAT) );

		// We're happy with a 16-bit z-buffer. Otherwise, keep looking.
		if( pddpf->dwZBufferBitDepth == 16 )
			return D3DENUMRET_CANCEL;
    }

    return D3DENUMRET_OK;
}
#endif



// Given a directdraw pixel format , build extended pixel format data and a name.
//
BOOL GetPixelFormatExt(DDPIXELFORMAT *ddpfPixelFormat,PixFormatExt *PixFormat)
{
	memset(PixFormat,0,sizeof(PixFormatExt));

	if (ddpfPixelFormat->dwFlags & DDPF_PALETTEINDEXED8) {

		sprintf(PixFormat->Name,"PALETTEINDEXED8");
        PixFormat->Palettized = TRUE;
        PixFormat->IndexBPP = 8;
		PixFormat->RGBBPP = 0;

    } else if (ddpfPixelFormat->dwFlags & DDPF_PALETTEINDEXED4) {

		sprintf(PixFormat->Name,"PALETTEINDEXED4");
        PixFormat->Palettized = TRUE;
        PixFormat->IndexBPP = 4;
		PixFormat->RGBBPP = 0;
		PixFormat->AlphaBPP = 0;

    } else if (ddpfPixelFormat->dwFlags & DDPF_ALPHAPIXELS) {
		if( (ddpfPixelFormat->dwFlags & DDPF_RGB)==0) {
			DebugPrint("  NON-RGB ALPHAPIXELS (not supported)\n");
			return FALSE;
		}

		ULONG m;
		ULONG a,r, g, b;

        PixFormat->Palettized = FALSE;
        PixFormat->IndexBPP = 0;
		PixFormat->RGBBPP = (UWORD)ddpfPixelFormat->dwRGBBitCount;
        PixFormat->ABitMask = ddpfPixelFormat->dwRGBAlphaBitMask;
        PixFormat->RBitMask = ddpfPixelFormat->dwRBitMask;
        PixFormat->GBitMask = ddpfPixelFormat->dwGBitMask;
        PixFormat->BBitMask = ddpfPixelFormat->dwBBitMask;
        for (a = 0, m = ddpfPixelFormat->dwRGBAlphaBitMask; !(m & 1); a++, m >>= 1);
        for (a = 0; m & 1; a++, m >>= 1);
        for (r = 0, m = ddpfPixelFormat->dwRBitMask; !(m & 1); r++, m >>= 1);
        for (r = 0; m & 1; r++, m >>= 1);
        for (g = 0, m = ddpfPixelFormat->dwGBitMask; !(m & 1); g++, m >>= 1);
        for (g = 0; m & 1; g++, m >>= 1);
        for (b = 0, m = ddpfPixelFormat->dwBBitMask; !(m & 1); b++, m >>= 1);
        for (b = 0; m & 1; b++, m >>= 1);
        PixFormat->AlphaBPP = (UWORD)a;
        PixFormat->RedBPP = (UWORD)r;
        PixFormat->GreenBPP = (UWORD)g;
        PixFormat->BlueBPP = (UWORD)b;
		ULONG Tmp;

   		PixFormat->AlphaShift=0;
   		Tmp=ddpfPixelFormat->dwRGBAlphaBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->AlphaShift++;
   		}

   		PixFormat->RedShift=0;
   		Tmp=ddpfPixelFormat->dwRBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->RedShift++;
   		}

		PixFormat->GreenShift=0;
   		Tmp=ddpfPixelFormat->dwGBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->GreenShift++;
   		}

   		PixFormat->BlueShift=0;
   		Tmp=ddpfPixelFormat->dwBBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->BlueShift++;
   		}

		sprintf(PixFormat->Name,"RGBALPHA %d (%d,%d,%d,%d)",
					PixFormat->RGBBPP,
					PixFormat->AlphaBPP,
					PixFormat->RedBPP,
					PixFormat->GreenBPP,
					PixFormat->BlueBPP);

		return TRUE;

    } else if (ddpfPixelFormat->dwFlags & DDPF_ALPHA) {
		if (ddpfPixelFormat->dwFlags & DDPF_RGB) {
			DebugPrint("  RGB ");
		}
		if (ddpfPixelFormat->dwFlags & DDPF_YUV) {
			DebugPrint("  YUV ");
		}
		DebugPrint("ALPHA (Not supported)\n");
		return FALSE;

    } else if (ddpfPixelFormat->dwFlags & DDPF_YUV) {
		if (ddpfPixelFormat->dwFlags & DDPF_YUV) {
			DebugPrint("  YUV (Not supported)\n");
		}
		return FALSE;

    } else {
		if( (ddpfPixelFormat->dwFlags & DDPF_RGB) == 0) {
			DebugPrint("  Unrecognized pixel format\n");
			return FALSE;
		}

		ULONG m;
		ULONG r, g, b;

        PixFormat->Palettized = FALSE;
        PixFormat->IndexBPP = 0;
		PixFormat->RGBBPP = (UWORD)ddpfPixelFormat->dwRGBBitCount;
        PixFormat->ABitMask = 0;
        PixFormat->RBitMask = ddpfPixelFormat->dwRBitMask;
        PixFormat->GBitMask = ddpfPixelFormat->dwGBitMask;
        PixFormat->BBitMask = ddpfPixelFormat->dwBBitMask;
        for (r = 0, m = ddpfPixelFormat->dwRBitMask; !(m & 1); r++, m >>= 1);
        for (r = 0; m & 1; r++, m >>= 1);
        for (g = 0, m = ddpfPixelFormat->dwGBitMask; !(m & 1); g++, m >>= 1);
        for (g = 0; m & 1; g++, m >>= 1);
        for (b = 0, m = ddpfPixelFormat->dwBBitMask; !(m & 1); b++, m >>= 1);
        for (b = 0; m & 1; b++, m >>= 1);
        PixFormat->AlphaBPP = 0;
        PixFormat->RedBPP = (UWORD)r;
        PixFormat->GreenBPP = (UWORD)g;
        PixFormat->BlueBPP = (UWORD)b;
		ULONG Tmp;

        PixFormat->AlphaShift = 0;

   		PixFormat->RedShift=0;
   		Tmp=ddpfPixelFormat->dwRBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->RedShift++;
   		}

		PixFormat->GreenShift=0;
   		Tmp=ddpfPixelFormat->dwGBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->GreenShift++;
   		}

   		PixFormat->BlueShift=0;
   		Tmp=ddpfPixelFormat->dwBBitMask;
   		while(!(Tmp&1)) {
   			Tmp>>=1;
   			PixFormat->BlueShift++;
   		}

		sprintf(PixFormat->Name,"RGB %d (%d,%d,%d)",
					PixFormat->RGBBPP,
					PixFormat->RedBPP,
					PixFormat->GreenBPP,
					PixFormat->BlueBPP);
    }

	return TRUE;
}


/*
Prototype:
BOOL FillSurface(void *Bits,DWORD Width,DWORD Height,
				   PALETTEENTRY *PaletteEntries,
				   DWORD ColourKey,
				   UBYTE Alpha,void *AlphaMap,
				   IDDSURFACE *Surface,
				   PixFormatExt *PixFormat)
Description:
 Take an 8 bit palettized bitmap and copy it into a DirectDraw surface.
 Converts the pixel format to that specified by the PixFormat parameter
 which must match the format of the destination surface.
 If the destination surface is smaller than the bitmap then the bitmap
 is scaled.

Parameters:
	void *Bits						Pointer to 8bit palettized bitmap.
	DWORD Width						Width of the bitmap.
	DWORD Height					Height of the bitmap.
	PALETTEENTRY *PaletteEntries	Pointer to 256 colour palette.
	UBYTE Alpha						Value to use for alpha channel when
									creating alpha textures if AlphaMap is
									not NULL then this value is ignored.
	void *AlphaMap					Pointer to 8bit bitmap to use when
									creating alpha textures ,if this parameter
									is NULL then uses the previous parameter
									for all pixels.
	IDDSURFACE *Surface				Pointer to the destination Direct Draw
									Surface. the surface must be the same size
									or smaller than the source bitmap.
	PixelFormat *PixFormat			Specifies the pixel format to convert to,
									this must match that of the destination
									surface.
Returns:
 None.

Implementation notes:
 Scaling accuracy need improving.
 RGB32BIT texture code not tested yet.
 Pure Alpha and YUV textures not supported.
*/
BOOL FillSurface(void *Bits,DWORD Width,DWORD Height,
								   PALETTEENTRY *PaletteEntries,
								   DWORD ColourKey,
								   UBYTE Alpha,void *AlphaMap,
								   IDDSURFACE *Surface,
								   PixFormatExt *PixFormat)
{
    SURFACEDESC ddsd;
	HRESULT	ddrval;

	memset(&ddsd,0,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	DXCALL(Surface->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL));

	DWORD xScale = Width / ddsd.dwWidth;
	DWORD yScale = Height / ddsd.dwHeight;
	DWORD ScaledWidth = ddsd.dwWidth;
	DWORD ScaledHeight = ddsd.dwHeight;


	ASSERT(xScale > 0);
	ASSERT(yScale > 0);

	if(PixFormat->Palettized) {
		UBYTE *Src=(UBYTE*)Bits;
		UBYTE *Dst=(UBYTE*)ddsd.lpSurface;
		UBYTE *TmpSrc,*TmpDst;
		DWORD x,y;
		DWORD dx,dy;

		if((xScale>1) || (yScale>1)) {
			dy = 0;
			for(y=0; y<Height; y+=yScale) {
				TmpSrc = Src;
				TmpDst = Dst;
				dx=0;
				for(x=0; x<Width; x+=xScale) {
					if((dx<ScaledWidth) && (dy<ScaledHeight)) {
						*TmpDst=*TmpSrc;
						TmpDst++;
						TmpSrc+=xScale;
					}
					dx++;
				}
				dy++;
				Src+=Width*yScale;
				Dst+=ddsd.lPitch;
			}
		} else {
			for(y=0; y<Height; y++) {
				Dst=((UBYTE*)ddsd.lpSurface) + ddsd.lPitch*y;
				for(x=0; x<Width; x++) {
					*Dst=*Src;
					Src++;
					Dst++;
				}
			}
		}
	} else {
		ULONG a,r,g,b;

		if(PixFormat->RGBBPP == 16) {
			UBYTE *Src=(UBYTE*)Bits;
			UBYTE *AlphaSrc=(UBYTE*)AlphaMap;
			UWORD *Dst=(UWORD*)ddsd.lpSurface;
			UBYTE *TmpSrc;
			UBYTE *TmpAlphaSrc;
			UWORD *TmpDst;
			UWORD Index;
			DWORD x,y;
			DWORD dx,dy;
			DWORD AlphaTmp;

			if((xScale>1) || (yScale>1)) {
				dy = 0;
				for(y=0; y<Height; y+=yScale) {
					TmpSrc = Src;
					TmpAlphaSrc = AlphaSrc;
					TmpDst = Dst;
					dx=0;
					for(x=0; x<Width; x+=xScale) {
						Index=(UWORD)*TmpSrc;

						if((dx<ScaledWidth) && (dy<ScaledHeight)) {
							r=(UWORD)(PaletteEntries[Index].peRed);
							g=(UWORD)(PaletteEntries[Index].peGreen);
							b=(UWORD)(PaletteEntries[Index].peBlue);

							if( ((r<<16) | (g<<8) | b) == ColourKey) {
								AlphaTmp = 0;
							} else {
								AlphaTmp = Alpha;
							}

							r>>=(8-PixFormat->RedBPP);
							r<<=PixFormat->RedShift;
							r&=PixFormat->RBitMask;

							g>>=(8-PixFormat->GreenBPP);
							g<<=PixFormat->GreenShift;
							g&=PixFormat->GBitMask;

							b>>=(8-PixFormat->BlueBPP);
							b<<=PixFormat->BlueShift;
							b&=PixFormat->BBitMask;

							if(AlphaMap) {
								a=(UWORD)(*TmpAlphaSrc >> (8-PixFormat->AlphaBPP));
							} else {
								a=(UWORD)(AlphaTmp >> (8-PixFormat->AlphaBPP));
							}

							a<<=PixFormat->AlphaShift;
							a&=PixFormat->ABitMask;
								
							*TmpDst=(UWORD)(a|r|g|b);

							TmpDst++;
							TmpSrc+=xScale;
							TmpAlphaSrc+=xScale;
						}
						dx++;
					}
					dy++;
					AlphaSrc+=Width*yScale;
					Src+=Width*yScale;
					Dst+=ddsd.lPitch/2;
				}
			} else {
				for(y=0; y<Height; y++) {
					Dst=(UWORD*)(((UBYTE*)ddsd.lpSurface) + ddsd.lPitch*y);
					Src=((UBYTE*)Bits) + y*Width;
					AlphaSrc=((UBYTE*)AlphaMap) + y*Width;
					for(x=0; x<Width; x++) {
						Index=(UWORD)*Src;

						r=(UWORD)(PaletteEntries[Index].peRed);
						g=(UWORD)(PaletteEntries[Index].peGreen);
						b=(UWORD)(PaletteEntries[Index].peBlue);

						if( ((r<<16) | (g<<8) | b) == ColourKey) {
   							AlphaTmp = 0;
   						} else {
   							AlphaTmp = Alpha;
   						}

						r>>=(8-PixFormat->RedBPP);
						r<<=PixFormat->RedShift;
						r&=PixFormat->RBitMask;

						g>>=(8-PixFormat->GreenBPP);
						g<<=PixFormat->GreenShift;
						g&=PixFormat->GBitMask;

						b>>=(8-PixFormat->BlueBPP);
						b<<=PixFormat->BlueShift;
						b&=PixFormat->BBitMask;

						if(AlphaMap) {
							a=(UWORD)(*AlphaSrc >> (8-PixFormat->AlphaBPP));
						} else {
							a=(UWORD)(AlphaTmp >> (8-PixFormat->AlphaBPP));
						}
						a<<=PixFormat->AlphaShift;
						a&=PixFormat->ABitMask;

						*Dst=(UWORD)(a|r|g|b);

						Src++;
						AlphaSrc++;
						Dst++;
					}
				}
			}
		} else if(PixFormat->RGBBPP == 32) {
			UBYTE *Src=(UBYTE*)Bits;
			UBYTE *AlphaSrc=(UBYTE*)AlphaMap;
			ULONG *Dst=(ULONG*)ddsd.lpSurface;
			UBYTE *TmpSrc;
			UBYTE *TmpAlphaSrc;
			ULONG *TmpDst;
			UWORD Index;
			DWORD x,y;
			DWORD dx,dy;

			if((xScale>1) || (yScale>1)) {
				dy = 0;
				for(y=0; y<Height; y+=yScale) {
					TmpSrc = Src;
					TmpAlphaSrc = AlphaSrc;
					TmpDst = Dst;
					dx=0;
					for(x=0; x<Width; x+=xScale) {
						Index=(UWORD)*TmpSrc;

						if((dx<ScaledWidth) && (dy<ScaledHeight)) {
							if(AlphaMap) {
								a=(UWORD)(*TmpAlphaSrc >> (8-PixFormat->AlphaBPP));
							} else {
								a=(UWORD)(Alpha >> (8-PixFormat->AlphaBPP));
							}
							a<<=PixFormat->AlphaShift;
							a&=PixFormat->ABitMask;

							r=(UWORD)(PaletteEntries[Index].peRed >> (8-PixFormat->RedBPP));
							r<<=PixFormat->RedShift;
							r&=PixFormat->RBitMask;

							g=(UWORD)(PaletteEntries[Index].peGreen >> (8-PixFormat->GreenBPP));
							g<<=PixFormat->GreenShift;
							g&=PixFormat->GBitMask;

							b=(UWORD)(PaletteEntries[Index].peBlue >> (8-PixFormat->BlueBPP));
							b<<=PixFormat->BlueShift;
							b&=PixFormat->BBitMask;
							*TmpDst=(UWORD)(a|r|g|b);

							TmpDst++;
							TmpSrc+=xScale;
							TmpAlphaSrc+=xScale;
						}
						dx++;
					}
					dy++;
					AlphaSrc+=Width*yScale;
					Src+=Width*yScale;
					Dst+=ddsd.lPitch/2;
				}
			} else {
				for(y=0; y<Height; y++) {
					Dst=(ULONG*)(((UBYTE*)ddsd.lpSurface) + ddsd.lPitch*y);
					Src=((UBYTE*)Bits) + y*Width;
					AlphaSrc=((UBYTE*)AlphaMap) + y*Width;
					for(x=0; x<Width; x++) {
						Index=(UWORD)*Src;

						if(AlphaMap) {
							a=(UWORD)(*AlphaSrc >> (8-PixFormat->AlphaBPP));
						} else {
							a=(UWORD)(Alpha >> (8-PixFormat->AlphaBPP));
						}
						a<<=PixFormat->AlphaShift;
						a&=PixFormat->ABitMask;

						r=(UWORD)(PaletteEntries[Index].peRed >> (8-PixFormat->RedBPP));
						r<<=PixFormat->RedShift;
						r&=PixFormat->RBitMask;

						g=(UWORD)(PaletteEntries[Index].peGreen >> (8-PixFormat->GreenBPP));
						g<<=PixFormat->GreenShift;
						g&=PixFormat->GBitMask;

						b=(UWORD)(PaletteEntries[Index].peBlue >> (8-PixFormat->BlueBPP));
						b<<=PixFormat->BlueShift;
						b&=PixFormat->BBitMask;
						*Dst=(UWORD)(a|r|g|b);

						Src++;
						AlphaSrc++;
						Dst++;
					}
				}
			}
		}
	}

	DXCALL(Surface->Unlock(NULL));

	return TRUE;
}


int MatchFormat(TFORMAT TextureFormat,DWORD AlphaTypes)
{
	int	i;

	PixFormatExt *PixFormat;

	for(i=0; i<g_TextureList.NumFormats; i++) {
		PixFormat = &g_TextureList.List[i].PixelFormatExt;

		switch(TextureFormat) {
			case	TF_ANY:
				return i;
				break;

			case	TF_PAL4BIT:
				if( (PixFormat->IndexBPP == 4) && (PixFormat->Palettized) ){
					return i;
				}
				break;

			case	TF_PAL8BIT:
				if( (PixFormat->IndexBPP == 8) && (PixFormat->Palettized) ) {
					return i;
				}
				break;

			case	TF_RGB16BIT:
				if( (PixFormat->RGBBPP == 16) && (PixFormat->AlphaBPP == 0) ) {
					return i;
				}
				break;

			case	TF_RGBALPHA16BIT:
				if( (PixFormat->RGBBPP == 16) && (PixFormat->AlphaBPP >= 2) ) {
					return i;
				}
				break;

			case	TF_RGB1ALPHA16BIT:
				if( (PixFormat->RGBBPP == 16) && (PixFormat->AlphaBPP == 1) ) {
					return i;
				}
				break;
		}
	}

//	DebugPrint("No match found, defaulting to %s\n",g_TextureList.List[0].PixelFormatExt.Name);

	return -1;	// If no match found then default to first texture format.
}


// BPPToDDBD
// Convert an integer bit per pixel number to a DirectDraw bit depth flag
//
DWORD BPPToDDBD(int bpp)
{
    switch(bpp) {
        case 1:
            return DDBD_1;
        case 2:
            return DDBD_2;
        case 4:
            return DDBD_4;
        case 8:
            return DDBD_8;
        case 16:
            return DDBD_16;
        case 24:
            return DDBD_24;
        case 32:
            return DDBD_32;
    }

	return(0);
}


// Interpret and display a DirectX error code.
//
void DisplayError(HRESULT Code,LPCTSTR File,DWORD Line,...)
{
	struct DDErrData {
		HRESULT	ErrCode;
		char	*ErrMess;
		char	*ErrDescribe;
	};

	static	DDErrData ErrorCheck[]={
		{DDERR_ALREADYINITIALIZED	,"DDERR_ALREADYINITIALIZED","The object has already been initialized."},
		{DDERR_BLTFASTCANTCLIP	,"DDERR_BLTFASTCANTCLIP","A clipper object is attached to a source surface that has passed into a call to the IDirectDrawSurface::BltFast method."},
		{DDERR_CANNOTATTACHSURFACE	,"DDERR_CANNOTATTACHSURFACE","A surface cannot be attached to another requested surface."},
		{DDERR_CANNOTDETACHSURFACE	,"DDERR_CANNOTDETACHSURFACE","A surface cannot be detached from another requested surface."},
		{DDERR_CANTCREATEDC 	,"DDERR_CANTCREATEDC","Windows cannot create any more device contexts (DCs)."},
		{DDERR_CANTDUPLICATE 	,"DDERR_CANTDUPLICATE","Primary and 3D surfaces, or surfaces that are implicitly created, cannot be duplicated."},
		{DDERR_CANTLOCKSURFACE 	,"DDERR_CANTLOCKSURFACE","Access to this surface is refused because an attempt was made to lock the primary surface without DCI support."},
		{DDERR_CANTPAGELOCK 	,"DDERR_CANTPAGELOCK","An attempt to page lock a surface failed. Page lock will not work on a display memory surface or an emulated primary surface."},
		{DDERR_CANTPAGEUNLOCK 	,"DDERR_CANTPAGEUNLOCK","An attempt to page unlock a surface failed. Page unlock will not work on a display memory surface or an emulated primary surface."},
		{DDERR_CLIPPERISUSINGHWND 	,"DDERR_CLIPPERISUSINGHWND","An attempt was made to set a clip list for a clipper object that is already monitoring a window handle."},
		{DDERR_COLORKEYNOTSET 	,"DDERR_COLORKEYNOTSET","No source color key is specified for this operation. 	"},
		{DDERR_CURRENTLYNOTAVAIL 	,"DDERR_CURRENTLYNOTAVAIL","No support is currently available. 	"},
		{DDERR_DCALREADYCREATED 	,"DDERR_DCALREADYCREATED","A device context has already been returned for this surface. Only one device context can be retrieved for each surface."},
		{DDERR_DIRECTDRAWALREADYCREATED 	,"DDERR_DIRECTDRAWALREADYCREATED","A DirectDraw object representing this driver has already been created for this process."},
		{DDERR_EXCEPTION 	,"DDERR_EXCEPTION","An exception was encountered while performing the requested operation. 	"},
		{DDERR_EXCLUSIVEMODEALREADYSET 	,"DDERR_EXCLUSIVEMODEALREADYSET","An attempt was made to set the cooperative level when it was already set to exclusive."},
		{DDERR_GENERIC 	,"DDERR_GENERIC","There is an undefined error condition. 	"},
		{DDERR_HEIGHTALIGN 	,"DDERR_HEIGHTALIGN","The height of the provided rectangle is not a multiple of the required alignment."},
		{DDERR_HWNDALREADYSET 	,"DDERR_HWNDALREADYSET","The DirectDraw CooperativeLevel HWND has already been set. It cannot be reset while the process has surfaces or palettes created."},
		{DDERR_HWNDSUBCLASSED 	,"DDERR_HWNDSUBCLASSED","DirectDraw is prevented from restoring state because the DirectDraw CooperativeLevel HWND has been subclassed."},
		{DDERR_IMPLICITLYCREATED 	,"DDERR_IMPLICITLYCREATED","The surface cannot be restored because it is an implicitly created surface."},
		{DDERR_INCOMPATIBLEPRIMARY 	,"DDERR_INCOMPATIBLEPRIMARY","The primary surface creation request does not match with the existing primary surface."},
		{DDERR_INVALIDCAPS 	,"DDERR_INVALIDCAPS","One or more of the capability bits passed to the callback function are incorrect."},
		{DDERR_INVALIDCLIPLIST 	,"DDERR_INVALIDCLIPLIST","DirectDraw does not support the provided clip list."},
		{DDERR_INVALIDDIRECTDRAWGUID 	,"DDERR_INVALIDDIRECTDRAWGUID","The GUID passed to the DirectDrawCreate function is not a valid DirectDraw driver identifier."},
		{DDERR_INVALIDMODE 	,"DDERR_INVALIDMODE","DirectDraw does not support the requested mode."},
		{DDERR_INVALIDOBJECT 	,"DDERR_INVALIDOBJECT","DirectDraw received a pointer that was an invalid DirectDraw object."},
		{DDERR_INVALIDPARAMS 	,"DDERR_INVALIDPARAMS","One or more of the parameters passed to the method are incorrect."},
		{DDERR_INVALIDPIXELFORMAT 	,"DDERR_INVALIDPIXELFORMAT","The pixel format was invalid as specified."},
		{DDERR_INVALIDPOSITION 	,"DDERR_INVALIDPOSITION","The position of the overlay on the destination is no longer legal."},
		{DDERR_INVALIDRECT 	,"DDERR_INVALIDRECT","The provided rectangle was invalid."},
		{DDERR_INVALIDSURFACETYPE 	,"DDERR_INVALIDSURFACETYPE","The requested operation could not be performed because the surface was of the wrong type."},
		{DDERR_LOCKEDSURFACES 	,"DDERR_LOCKEDSURFACES","One or more surfaces are locked, causing the failure of the requested operation."},
		{DDERR_NO3D 	,"DDERR_NO3D","No 3D is present."},
		{DDERR_NOALPHAHW 	,"DDERR_NOALPHAHW","No alpha acceleration hardware is present or available, causing the failure of the requested operation."},
		{DDERR_NOBLTHW 	,"DDERR_NOBLTHW","No blitter hardware is present."},
		{DDERR_NOCLIPLIST 	,"DDERR_NOCLIPLIST","No clip list is available."},
		{DDERR_NOCLIPPERATTACHED 	,"DDERR_NOCLIPPERATTACHED","No clipper object is attached to the surface object."},
		{DDERR_NOCOLORCONVHW 	,"DDERR_NOCOLORCONVHW","The operation cannot be carried out because no color conversion hardware is present or available."},
		{DDERR_NOCOLORKEY 	,"DDERR_NOCOLORKEY","The surface does not currently have a color key."},
		{DDERR_NOCOLORKEYHW 	,"DDERR_NOCOLORKEYHW","The operation cannot be carried out because there is no hardware support for the destination color key."},
		{DDERR_NOCOOPERATIVELEVELSET 	,"DDERR_NOCOOPERATIVELEVELSET","A create function is called without the IDirectDraw::SetCooperativeLevel method being called."},
		{DDERR_NODC 	,"DDERR_NODC","No device context (DC) has ever been created for this surface."},
		{DDERR_NODDROPSHW 	,"DDERR_NODDROPSHW","No DirectDraw raster operation (ROP) hardware is available."},
		{DDERR_NODIRECTDRAWHW 	,"DDERR_NODIRECTDRAWHW","Hardware-only DirectDraw object creation is not possible; the driver does not support any hardware."},
		{DDERR_NODIRECTDRAWSUPPORT 	,"DDERR_NODIRECTDRAWSUPPORT","DirectDraw support is not possible with the current display driver."},
		{DDERR_NOEMULATION 	,"DDERR_NOEMULATION","Software emulation is not available."},
		{DDERR_NOEXCLUSIVEMODE 	,"DDERR_NOEXCLUSIVEMODE","The operation requires the application to have exclusive mode, but the application does not have exclusive mode."},
		{DDERR_NOFLIPHW 	,"DDERR_NOFLIPHW","Flipping visible surfaces is not supported."},
		{DDERR_NOGDI 	,"DDERR_NOGDI","No GDI is present."},
		{DDERR_NOHWND 	,"DDERR_NOHWND","Clipper notification requires an HWND, or no HWND has been previously set as the CooperativeLevel HWND."},
		{DDERR_NOMIPMAPHW 	,"DDERR_NOMIPMAPHW","The operation cannot be carried out because no mipmap texture mapping hardware is present or available."},
		{DDERR_NOMIRRORHW 	,"DDERR_NOMIRRORHW","The operation cannot be carried out because no mirroring hardware is present or available."},
		{DDERR_NOOVERLAYDEST 	,"DDERR_NOOVERLAYDEST","The IDirectDrawSurface::GetOverlayPosition method is called on an overlay that the IDirectDrawSurface::UpdateOverlay method has not been called on to establish a destination."},
		{DDERR_NOOVERLAYHW 	,"DDERR_NOOVERLAYHW","The operation cannot be carried out because no overlay hardware is present or available."},
		{DDERR_NOPALETTEATTACHED 	,"DDERR_NOPALETTEATTACHED","No palette object is attached to this surface."},
		{DDERR_NOPALETTEHW 	,"DDERR_NOPALETTEHW","There is no hardware support for 16- or 256-color palettes."},
		{DDERR_NORASTEROPHW 	,"DDERR_NORASTEROPHW","The operation cannot be carried out because no appropriate raster operation hardware is present or available."},
		{DDERR_NOROTATIONHW 	,"DDERR_NOROTATIONHW","The operation cannot be carried out because no rotation hardware is present or available."},
		{DDERR_NOSTRETCHHW 	,"DDERR_NOSTRETCHHW","The operation cannot be carried out because there is no hardware support for stretching."},
		{DDERR_NOT4BITCOLOR 	,"DDERR_NOT4BITCOLOR","The DirectDrawSurface is not using a 4-bit color palette and the requested operation requires a 4-bit color palette."},
		{DDERR_NOT4BITCOLORINDEX 	,"DDERR_NOT4BITCOLORINDEX","The DirectDrawSurface is not using a 4-bit color index palette and the requested operation requires a 4-bit color index palette."},
		{DDERR_NOT8BITCOLOR 	,"DDERR_NOT8BITCOLOR","The DirectDrawSurface is not using an 8-bit color palette and the requested operation requires an 8-bit color palette."},
		{DDERR_NOTAOVERLAYSURFACE 	,"DDERR_NOTAOVERLAYSURFACE","An overlay component is called for a non-overlay surface"},
		{DDERR_NOTEXTUREHW 	,"DDERR_NOTEXTUREHW","The operation cannot be carried out because no texture mapping hardware is present or available."},
		{DDERR_NOTFLIPPABLE 	,"DDERR_NOTFLIPPABLE","An attempt has been made to flip a surface that cannot be flipped."},
		{DDERR_NOTFOUND 	,"DDERR_NOTFOUND","The requested item was not found."},
		{DDERR_NOTINITIALIZED 	,"DDERR_NOTINITIALIZED","An attempt was made to invoke an interface method of a DirectDraw object created by CoCreateInstance before the object was initialized."},
		{DDERR_NOTLOCKED 	,"DDERR_NOTLOCKED","An attempt is made to unlock a surface that was not locked."},
		{DDERR_NOTPAGELOCKED 	,"DDERR_NOTPAGELOCKED","An attempt is made to page unlock a surface with no outstanding page locks."},
		{DDERR_NOTPALETTIZED 	,"DDERR_NOTPALETTIZED","The surface being used is not a palette-based surface."},
		{DDERR_NOVSYNCHW 	,"DDERR_NOVSYNCHW","The operation cannot be carried out because there is no hardware support for vertical blank synchronized operations."},
		{DDERR_NOZBUFFERHW 	,"DDERR_NOZBUFFERHW","The operation cannot be carried out because there is no hardware support for z-buffers when creating a z-buffer in display memory or when performing a z-aware blit."},
		{DDERR_NOZOVERLAYHW 	,"DDERR_NOZOVERLAYHW","The overlay surfaces cannot be z-layered based on their BltOrder because the hardware does not support z-layering of overlays."},
		{DDERR_OUTOFCAPS 	,"DDERR_OUTOFCAPS","The hardware needed for the requested operation has already been allocated."},
		{DDERR_OUTOFMEMORY 	,"DDERR_OUTOFMEMORY","DirectDraw does not have enough memory to perform the operation."},
		{DDERR_OUTOFVIDEOMEMORY 	,"DDERR_OUTOFVIDEOMEMORY","DirectDraw does not have enough display memory to perform the operation."},
		{DDERR_OVERLAYCANTCLIP 	,"DDERR_OVERLAYCANTCLIP","The hardware does not support clipped overlays."},
		{DDERR_OVERLAYCOLORKEYONLYONEACTIVE 	,"DDERR_OVERLAYCOLORKEYONLYONEACTIVE","An attempt was made to have more than one color key active on an overlay."},
		{DDERR_OVERLAYNOTVISIBLE 	,"DDERR_OVERLAYNOTVISIBLE","The IDirectDrawSurface::GetOverlayPosition method is called on a hidden overlay."},
		{DDERR_PALETTEBUSY 	,"DDERR_PALETTEBUSY","Access to this palette is refused because the palette is locked by another thread."},
		{DDERR_PRIMARYSURFACEALREADYEXISTS 	,"DDERR_PRIMARYSURFACEALREADYEXISTS","This process has already created a primary surface."},
		{DDERR_REGIONTOOSMALL 	,"DDERR_REGIONTOOSMALL","The region passed to the IDirectDrawClipper::GetClipList method is too small."},
		{DDERR_SURFACEALREADYATTACHED 	,"DDERR_SURFACEALREADYATTACHED","An attempt was made to attach a surface to another surface to which it is already attached."},
		{DDERR_SURFACEALREADYDEPENDENT 	,"DDERR_SURFACEALREADYDEPENDENT","An attempt was made to make a surface a dependency of another surface to which it is already dependent."},
		{DDERR_SURFACEBUSY 	,"DDERR_SURFACEBUSY","Access to this surface is refused because the surface is locked by another thread."},
		{DDERR_SURFACEISOBSCURED 	,"DDERR_SURFACEISOBSCURED 	","Access to the surface is refused because the surface is obscured."},
		{DDERR_SURFACELOST 	,"DDERR_SURFACELOST","Access to this surface is refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have the IDirectDrawSurface::Restore method called on it."},
		{DDERR_SURFACENOTATTACHED 	,"DDERR_SURFACENOTATTACHED","The requested surface is not attached."},
		{DDERR_TOOBIGHEIGHT 	,"DDERR_TOOBIGHEIGHT","The height requested by DirectDraw is too large."},
		{DDERR_TOOBIGSIZE 	,"DDERR_TOOBIGSIZE","The size requested by DirectDraw is too large. However, the individual height and width are OK."},
		{DDERR_TOOBIGWIDTH 	,"DDERR_TOOBIGWIDTH","The width requested by DirectDraw is too large."},
		{DDERR_UNSUPPORTED 	,"DDERR_UNSUPPORTED","The operation is not supported."},
		{DDERR_UNSUPPORTEDFORMAT 	,"DDERR_UNSUPPORTEDFORMAT 	","The FourCC format requested is not supported by DirectDraw."},
		{DDERR_UNSUPPORTEDMASK 	,"DDERR_UNSUPPORTEDMASK","The bitmask in the pixel format requested is not supported by DirectDraw."},
		{DDERR_UNSUPPORTEDMODE 	,"DDERR_UNSUPPORTEDMODE","The display is currently in an unsupported mode."},
		{DDERR_VERTICALBLANKINPROGRESS 	,"DDERR_VERTICALBLANKINPROGRESS 	","A vertical blank is in progress."},
		{DDERR_WASSTILLDRAWING 	,"DDERR_WASSTILLDRAWING","The previous blit operation that is transferring information to or from this surface is incomplete."},
		{DDERR_WRONGMODE 	,"DDERR_WRONGMODE","This surface cannot be restored because it was created in a different mode."},
		{DDERR_XALIGN 	,"DDERR_XALIGN","The provided rectangle was not horizontally aligned on a required boundary."},

		{D3DERR_BADMAJORVERSION	,"D3DERR_BADMAJORVERSION	",""},
		{D3DERR_BADMINORVERSION	,"D3DERR_BADMINORVERSION	",""},
		{D3DERR_INVALID_DEVICE   ,"D3DERR_INVALID_DEVICE   ",""},
		{D3DERR_INITFAILED       ,"D3DERR_INITFAILED       ",""},
		{D3DERR_DEVICEAGGREGATED ,"D3DERR_DEVICEAGGREGATED ",""},
		{D3DERR_EXECUTE_CREATE_FAILED	,"D3DERR_EXECUTE_CREATE_FAILED	",""},
		{D3DERR_EXECUTE_DESTROY_FAILED	,"D3DERR_EXECUTE_DESTROY_FAILED	",""},
		{D3DERR_EXECUTE_LOCK_FAILED	,"D3DERR_EXECUTE_LOCK_FAILED	",""},
		{D3DERR_EXECUTE_UNLOCK_FAILED	,"D3DERR_EXECUTE_UNLOCK_FAILED	",""},
		{D3DERR_EXECUTE_LOCKED		,"D3DERR_EXECUTE_LOCKED		",""},
		{D3DERR_EXECUTE_NOT_LOCKED	,"D3DERR_EXECUTE_NOT_LOCKED	",""},
		{D3DERR_EXECUTE_FAILED		,"D3DERR_EXECUTE_FAILED		",""},
		{D3DERR_EXECUTE_CLIPPED_FAILED	,"D3DERR_EXECUTE_CLIPPED_FAILED	",""},
		{D3DERR_TEXTURE_NO_SUPPORT	,"D3DERR_TEXTURE_NO_SUPPORT	",""},
		{D3DERR_TEXTURE_CREATE_FAILED	,"D3DERR_TEXTURE_CREATE_FAILED	",""},
		{D3DERR_TEXTURE_DESTROY_FAILED	,"D3DERR_TEXTURE_DESTROY_FAILED	",""},
		{D3DERR_TEXTURE_LOCK_FAILED	,"D3DERR_TEXTURE_LOCK_FAILED	",""},
		{D3DERR_TEXTURE_UNLOCK_FAILED	,"D3DERR_TEXTURE_UNLOCK_FAILED	",""},
		{D3DERR_TEXTURE_LOAD_FAILED	,"D3DERR_TEXTURE_LOAD_FAILED	",""},
		{D3DERR_TEXTURE_SWAP_FAILED	,"D3DERR_TEXTURE_SWAP_FAILED	",""},
		{D3DERR_TEXTURE_LOCKED		,"D3DERR_TEXTURE_LOCKED		",""},
		{D3DERR_TEXTURE_NOT_LOCKED	,"D3DERR_TEXTURE_NOT_LOCKED	",""},
		{D3DERR_TEXTURE_GETSURF_FAILED	,"D3DERR_TEXTURE_GETSURF_FAILED	",""},
		{D3DERR_MATRIX_CREATE_FAILED	,"D3DERR_MATRIX_CREATE_FAILED	",""},
		{D3DERR_MATRIX_DESTROY_FAILED	,"D3DERR_MATRIX_DESTROY_FAILED	",""},
		{D3DERR_MATRIX_SETDATA_FAILED	,"D3DERR_MATRIX_SETDATA_FAILED	",""},
		{D3DERR_MATRIX_GETDATA_FAILED	,"D3DERR_MATRIX_GETDATA_FAILED	",""},
		{D3DERR_SETVIEWPORTDATA_FAILED	,"D3DERR_SETVIEWPORTDATA_FAILED	",""},
		{D3DERR_INVALIDCURRENTVIEWPORT   ,"D3DERR_INVALIDCURRENTVIEWPORT   ",""},
		{D3DERR_INVALIDPRIMITIVETYPE     ,"D3DERR_INVALIDPRIMITIVETYPE     ",""},
		{D3DERR_INVALIDVERTEXTYPE        ,"D3DERR_INVALIDVERTEXTYPE        ",""},
		{D3DERR_TEXTURE_BADSIZE          ,"D3DERR_TEXTURE_BADSIZE          ",""},
		{D3DERR_INVALIDRAMPTEXTURE		,"D3DERR_INVALIDRAMPTEXTURE		",""},
		{D3DERR_MATERIAL_CREATE_FAILED	,"D3DERR_MATERIAL_CREATE_FAILED	",""},
		{D3DERR_MATERIAL_DESTROY_FAILED	,"D3DERR_MATERIAL_DESTROY_FAILED	",""},
		{D3DERR_MATERIAL_SETDATA_FAILED	,"D3DERR_MATERIAL_SETDATA_FAILED	",""},
		{D3DERR_MATERIAL_GETDATA_FAILED	,"D3DERR_MATERIAL_GETDATA_FAILED	",""},
		{D3DERR_INVALIDPALETTE	        ,"D3DERR_INVALIDPALETTE	        ",""},
		{D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY ,"D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY ",""},
		{D3DERR_ZBUFF_NEEDS_VIDEOMEMORY  ,"D3DERR_ZBUFF_NEEDS_VIDEOMEMORY  ",""},
		{D3DERR_SURFACENOTINVIDMEM       ,"D3DERR_SURFACENOTINVIDMEM       ",""},
		{D3DERR_LIGHT_SET_FAILED		,"D3DERR_LIGHT_SET_FAILED		",""},
		{D3DERR_LIGHTHASVIEWPORT		,"D3DERR_LIGHTHASVIEWPORT		",""},
		{D3DERR_LIGHTNOTINTHISVIEWPORT           ,"D3DERR_LIGHTNOTINTHISVIEWPORT           ",""},
		{D3DERR_SCENE_IN_SCENE		,"D3DERR_SCENE_IN_SCENE		",""},
		{D3DERR_SCENE_NOT_IN_SCENE	,"D3DERR_SCENE_NOT_IN_SCENE	",""},
		{D3DERR_SCENE_BEGIN_FAILED	,"D3DERR_SCENE_BEGIN_FAILED	",""},
		{D3DERR_SCENE_END_FAILED		,"D3DERR_SCENE_END_FAILED		",""},
		{D3DERR_INBEGIN                  ,"D3DERR_INBEGIN                  ",""},
		{D3DERR_NOTINBEGIN               ,"D3DERR_NOTINBEGIN               ",""},
		{D3DERR_NOVIEWPORTS              ,"D3DERR_NOVIEWPORTS              ",""},
		{D3DERR_VIEWPORTDATANOTSET       ,"D3DERR_VIEWPORTDATANOTSET       ",""},
		{D3DERR_VIEWPORTHASNODEVICE      ,"D3DERR_VIEWPORTHASNODEVICE      ",""},
		{D3DERR_NOCURRENTVIEWPORT		,"D3DERR_NOCURRENTVIEWPORT		",""},

		{0,NULL}
	};

	if(Code!=DD_OK) {
		DDErrData	*ErrPtr;
		char	String[512];
		HRESULT	HandledErr;
		va_list	VarArg;

		va_start(VarArg,Line);
		while( (HandledErr=va_arg(VarArg,HRESULT)) !=0 ) {
			if(HandledErr==Code) {
				return;
			}
		}
		va_end(VarArg);
		
		ErrPtr=(DDErrData*)ErrorCheck;

		while(ErrPtr->ErrMess!=NULL) {
			if(ErrPtr->ErrCode==Code) {
				sprintf(String,"%s\n%s\n\nFile: %s\nLine: %d\n",ErrPtr->ErrMess,ErrPtr->ErrDescribe,File,Line);
//				OutputDebugString(String);
				DebugPrint(String);

				MessageBox( NULL, String, "Unhandled DD Error", MB_OK );
				return;
			}
			ErrPtr++;
		}

		sprintf(String,"Unknown code (%x)\n\nFile: %s\nLine: %d\n",Code,File,Line);
//		OutputDebugString(String);
		DebugPrint(String);

		MessageBox( NULL, String, "Unhandled DD Error", MB_OK );
		return;
	}
}

