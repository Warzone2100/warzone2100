/*
 * dderror.h
 *
 * Convert direct draw error codes to an error string.
 *
 */
#ifndef _dderror_h
#define _dderror_h

/* Check the header files have been included from frame.h if they
 * are used outside of the framework library.
 */
#if !defined(_frame_h) && !defined(FRAME_LIB_INCLUDE)
#error Framework header files MUST be included from Frame.h ONLY.
#endif

#pragma warning (disable : 4201 4214 4115 4514)
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3drm.h>
#pragma warning (default : 4201 4214 4115)

#include "types.h"

/* Turn a DD, D3D, or D3DRM error code into a string */
extern STRING *DDErrorToString(HRESULT error);

#endif

