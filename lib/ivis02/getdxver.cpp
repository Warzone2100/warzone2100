/**************************************************************************

    GetDXVer.cpp

    Demonstrates how applications can detect what version of DirectX
    is installed.

 **************************************************************************/
/**************************************************************************

    (C) Copyright 1995-1997 Microsoft Corp.  All rights reserved.

    You have a royalty-free right to use, modify, reproduce and
    distribute the Sample Files (and/or any modified version) in
    any way you find useful, provided that you agree that
    Microsoft has no warranty obligations or liability for any
    Sample Application Files which are modified.

 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include <dinput.h>
#include <d3drm.h>

/****************************************************************************
 *
 *      GetDXVersion
 *
 *  This function returns two arguments:
 *	dwDXVersion:
 *	    0	    No DirectX installed
 *	    0x100   DirectX version 1 installed
 *	    0x200   DirectX 2 installed
 *	    0x300   DirectX 3 installed
 *	    0x500   At least DirectX 5 installed.
 *      0x600   At least DirectX 6 installed.
 *	dwDXPlatform:
 *	    0	                            Unknown (This is a failure case. Should never happen)
 *	    VER_PLATFORM_WIN32_WINDOWS	    Windows 9X platform
 *	    VER_PLATFORM_WIN32_NT	    Windows NT platform
 * 
 * Please note that this code is intended as a general guideline. Your app will
 * probably be able to simply query for functionality (via COM's QueryInterface)
 * for one or two components.
 * Please also note:
 *      "if (dxVer != 0x500) return FALSE;" is BAD. 
 *      "if (dxVer < 0x500) return FALSE;" is MUCH BETTER.
 * to ensure your app will run on future releases of DirectX.
 *
 ****************************************************************************/

typedef HRESULT(WINAPI * DIRECTDRAWCREATE) (GUID *, LPDIRECTDRAW *, IUnknown *);
typedef HRESULT(WINAPI * DIRECTINPUTCREATE) (HINSTANCE, DWORD, LPDIRECTINPUT *,
                                             IUnknown *);
extern "C"
{
void
GetDXVersion(LPDWORD pdwDXVersion, LPDWORD pdwDXPlatform)
{
    HRESULT                 hr;
    HINSTANCE               DDHinst = 0;
    HINSTANCE               DIHinst = 0;
    LPDIRECTDRAW            pDDraw = 0;
    LPDIRECTDRAW2           pDDraw2 = 0;
    DIRECTDRAWCREATE        DirectDrawCreate = 0;
    DIRECTINPUTCREATE       DirectInputCreate = 0;
    OSVERSIONINFO           osVer;
    LPDIRECTDRAWSURFACE     pSurf = 0;
    LPDIRECTDRAWSURFACE3    pSurf3 = 0;
    LPDIRECTDRAWSURFACE4    pSurf4 = 0;

    /*
     * First get the windows platform
     */
    osVer.dwOSVersionInfoSize = sizeof(osVer);
    if (!GetVersionEx(&osVer))
    {
        *pdwDXVersion = 0;
        *pdwDXPlatform = 0;
        return;
    }

    if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        *pdwDXPlatform = VER_PLATFORM_WIN32_NT;
        /*
         * NT is easy... NT 4.0 is DX2, 4.0 SP3 is DX3, 5.0 is DX5
         * and no DX on earlier versions.
         */
        if (osVer.dwMajorVersion < 4)
        {
            *pdwDXPlatform = 0; //No DX on NT3.51 or earlier
            return;
        }
        if (osVer.dwMajorVersion == 4)
        {
            /*
             * NT4 up to SP2 is DX2, and SP3 onwards is DX3, so we are at least DX2
             */
            *pdwDXVersion = 0x200;

            /*
             * We're not supposed to be able to tell which SP we're on, so check for dinput
             */
            DIHinst = LoadLibrary("DINPUT.DLL");
            if (DIHinst == 0)
            {
                /*
                 * No DInput... must be DX2 on NT 4 pre-SP3
                 */
                OutputDebugString("Couldn't LoadLibrary DInput\r\n");
                return;
            }

            DirectInputCreate = (DIRECTINPUTCREATE)
                GetProcAddress(DIHinst, "DirectInputCreateA");
            FreeLibrary(DIHinst);

            if (DirectInputCreate == 0)
            {
                /*
                 * No DInput... must be pre-SP3 DX2
                 */
                OutputDebugString("Couldn't GetProcAddress DInputCreate\r\n");
                return;
            }

            /*
             * It must be NT4, DX2
             */
            *pdwDXVersion = 0x300;  //DX3 on NT4 SP3 or higher
            return;
        }
        /*
         * Else it's NT5 or higher, and it's DX5a or higher:
         */

        /*
         * Drop through to Win9x tests for a test of DDraw (DX6 or higher)
         */
    }
    else
    {
        /*
         * Not NT... must be Win9x
         */
        *pdwDXPlatform = VER_PLATFORM_WIN32_WINDOWS;
    }

    /*
     * Now we know we are in Windows 9x (or maybe 3.1), so anything's possible.
     * First see if DDRAW.DLL even exists.
     */
    DDHinst = LoadLibrary("DDRAW.DLL");
    if (DDHinst == 0)
    {
        *pdwDXVersion = 0;
        *pdwDXPlatform = 0;
        FreeLibrary(DDHinst);
        return;
    }

    /*
     *  See if we can create the DirectDraw object.
     */
    DirectDrawCreate = (DIRECTDRAWCREATE)
        GetProcAddress(DDHinst, "DirectDrawCreate");
    if (DirectDrawCreate == 0)
    {
        *pdwDXVersion = 0;
        *pdwDXPlatform = 0;
        FreeLibrary(DDHinst);
        OutputDebugString("Couldn't LoadLibrary DDraw\r\n");
        return;
    }

    hr = DirectDrawCreate(NULL, &pDDraw, NULL);
    if (FAILED(hr))
    {
        *pdwDXVersion = 0;
        *pdwDXPlatform = 0;
        FreeLibrary(DDHinst);
        OutputDebugString("Couldn't create DDraw\r\n");
        return;
    }

    /*
     *  So DirectDraw exists.  We are at least DX1.
     */
    *pdwDXVersion = 0x100;

    /*
     *  Let's see if IID_IDirectDraw2 exists.
     */
    hr = pDDraw->QueryInterface(IID_IDirectDraw2, (LPVOID *) & pDDraw2);
    if (FAILED(hr))
    {
        /*
         * No IDirectDraw2 exists... must be DX1
         */
        pDDraw->Release();
        FreeLibrary(DDHinst);
        OutputDebugString("Couldn't QI DDraw2\r\n");
        return;
    }
    /*
     * IDirectDraw2 exists. We must be at least DX2
     */
    pDDraw2->Release();
    *pdwDXVersion = 0x200;

    /*
     *  See if we can create the DirectInput object.
     */
    DIHinst = LoadLibrary("DINPUT.DLL");
    if (DIHinst == 0)
    {
        /*
         * No DInput... must be DX2
         */
        OutputDebugString("Couldn't LoadLibrary DInput\r\n");
        pDDraw->Release();
        FreeLibrary(DDHinst);
        return;
    }

    DirectInputCreate = (DIRECTINPUTCREATE )GetProcAddress(DIHinst, "DirectInputCreateA");
    FreeLibrary(DIHinst);

    if (DirectInputCreate == 0)
    {
        /*
         * No DInput... must be DX2
         */
        FreeLibrary(DDHinst);
        pDDraw->Release();
        OutputDebugString("Couldn't GetProcAddress DInputCreate\r\n");
        return;
    }

    /*
     * DirectInputCreate exists. That's enough to tell us that we are at least DX3
     */
    *pdwDXVersion = 0x300;

    /*
     * Checks for 3a vs 3b?
     */

    /*
     * We can tell if DX5 is present by checking for the existence of IDirectDrawSurface3.
     * First we need a surface to QI off of.
     */
    DDSURFACEDESC desc;

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = pDDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        /*
         * Failure. This means DDraw isn't properly installed.
         */
        pDDraw->Release();
        FreeLibrary(DDHinst);
        *pdwDXVersion = 0;
        OutputDebugString("Couldn't Set coop level\r\n");
        return;
    }

    hr = pDDraw->CreateSurface(&desc, &pSurf, NULL);
    if (FAILED(hr))
    {
        /*
         * Failure. This means DDraw isn't properly installed.
         */
        pDDraw->Release();
        FreeLibrary(DDHinst);
        *pdwDXVersion = 0;
        OutputDebugString("Couldn't CreateSurface\r\n");
        return;
    }

    /*
     * Try for the IDirectDrawSurface3 interface. If it works, we're on DX5 at least
     */
    if (FAILED(pSurf->QueryInterface(IID_IDirectDrawSurface3, (LPVOID *) & pSurf3)))
    {
        pDDraw->Release();
        FreeLibrary(DDHinst);

        return;
    }

    /*
     * QI for IDirectDrawSurface3 succeeded. We must be at least DX5
     */
    *pdwDXVersion = 0x500;

    /*
     * Try for the IDirectDrawSurface4 interface. If it works, we're on DX6 at least
     */
    if (FAILED(pSurf->QueryInterface(IID_IDirectDrawSurface4, (LPVOID *) & pSurf4)))
    {
        pDDraw->Release();
        FreeLibrary(DDHinst);

        return;
    }

    /*
     * QI for IDirectDrawSurface4 succeeded. We must be at least DX6
     */
    *pdwDXVersion = 0x600;

//    if (pSurf4) {pSurf4->Release();}
//    if (pSurf3) {pSurf3->Release();}
    if (pSurf) {pSurf->Release();}
//    if (pDDraw2) {pDDraw2->Release();}
    if (pDDraw) {pDDraw->Release();}
    FreeLibrary(DDHinst);

    return;
}
}