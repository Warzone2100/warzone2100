/*
 * DXInput.c
 *
 * Input functions using direct input.
 *
 */

#include "frame.h"
#include "frameint.h"
#include "dxinput.h"

// The direct input object
LPDIRECTINPUT  psDI = NULL; 

// The direct input mouse object
LPDIRECTINPUTDEVICE psDIMouse = NULL;

// Whether the mouse has been aquired
static BOOL			DIMouseAcquired = FALSE;

// Initial mouse scale
#define DINP_MOUSESCALE		2

// mouse coordinates
static SDWORD		mickeyX, mickeyY, mickeyScale;

// Initialise the DXInput system
BOOL DInpInitialise(void)
{
	HRESULT			hr; 
//	DIDATAFORMAT	sDataFormat;

	// Create the direct input object;
	hr = DirectInputCreate(hInstance, DIRECTINPUT_VERSION, &psDI, NULL); 
	if (FAILED(hr))
	{
		DBERROR(("DXInpInitialise: couldn't create DI object"));
		return FALSE;
	}

	hr = psDI->lpVtbl->CreateDevice(psDI, &GUID_SysMouse, &psDIMouse, NULL);
	if (FAILED(hr))
	{
		DBERROR(("DXInpInitialise: couldn't create mouse object"));
		return FALSE;
	}

/*	memcpy(&sDataFormat, &c_dfDIMouse, sizeof(DIDATAFORMAT));
	sDataFormat.dwFlags = DIDF_ABSAXIS;
	sDataFormat.dwSize = sizeof(DIDATAFORMAT);
	sDataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
	sDataFormat.dwDataSize = 0;
	sDataFormat.dwNumObjs = 0;
	sDataFormat.rgodf = NULL;*/
	hr = psDIMouse->lpVtbl->SetDataFormat(psDIMouse, &c_dfDIMouse);//&sDataFormat);
	if (FAILED(hr))
	{
		DBERROR(("DXInpInitialise: couldn't set mouse format"));
		return FALSE;
	}
 
	hr = psDIMouse->lpVtbl->SetCooperativeLevel(psDIMouse, hWndMain,
				   DISCL_EXCLUSIVE | DISCL_FOREGROUND);
 	if (FAILED(hr))
	{
		DBERROR(("DXInpInitialise: couldn't set mouse cooperative level"));
		return FALSE;
	}

	DIMouseAcquired = FALSE;
	if (!DInpMouseAcc(DINP_MOUSEACQUIRE))
	{
		DBERROR(("DXInpInitialise: couldn't acquire mouse"));
		return FALSE;
	}

	mickeyScale = DINP_MOUSESCALE;
	mickeyX = (SDWORD)screenWidth * mickeyScale / 2;
	mickeyY = (SDWORD)screenHeight * mickeyScale / 2;

	return TRUE;
}


// Shut down the DX input system
void DInpShutDown(void)
{
	RELEASE(psDIMouse);
	RELEASE(psDI);
}


// Aquire or release input for the mouse
BOOL DInpMouseAcc(UDWORD aquireType)
{
	HRESULT			hr;

	if (!psDIMouse)
	{
		return FALSE;
	}

	switch (aquireType)
	{
	case DINP_MOUSEACQUIRE:
		if (!DIMouseAcquired)
		{
			hr = psDIMouse->lpVtbl->Acquire(psDIMouse);
			if (FAILED(hr))
			{
				ASSERT((FALSE, "DInpMouseAcc: failed to aquire mouse"));
				return FALSE;
			}
			DIMouseAcquired = TRUE;
		}
		break;
	case DINP_MOUSERELEASE:
		if (DIMouseAcquired)
		{
			hr = psDIMouse->lpVtbl->Unacquire(psDIMouse);
			if (FAILED(hr))
			{
				ASSERT((FALSE, "DInpMouseAcc: failed to unaquire mouse"));
				return FALSE;
			}
			DIMouseAcquired = FALSE;
		}
		break;
	default:
		ASSERT((FALSE, "DInpMouseAcc: unknown aquire type"));
		return FALSE;
	}
	return TRUE;
}


// Get the current state of the mouse
BOOL DInpGetMouseState(SDWORD *pX, SDWORD *pY, SDWORD *pButtons)
{
	HRESULT			hr;
	DIMOUSESTATE	sState;

	hr = psDIMouse->lpVtbl->GetDeviceState(psDIMouse,
					sizeof(DIMOUSESTATE), &sState);
	if (hr == DIERR_INPUTLOST)
	{
		DIMouseAcquired = FALSE;
		DInpMouseAcc(DINP_MOUSEACQUIRE);
		hr = psDIMouse->lpVtbl->GetDeviceState(psDIMouse,
						sizeof(DIMOUSESTATE), &sState);
	}
	if (FAILED(hr))
	{
		ASSERT((FALSE, "DInpGetMouseState: failed to get mouse state\n%s",
				DIErrorToString(hr)));
		return FALSE;
	}

	// update the mickey coords
	mickeyX += sState.lX;
	if (mickeyX < 0)
	{
		mickeyX = 0;
	}
	if (mickeyX >= (SDWORD)screenWidth * mickeyScale)
	{
		mickeyX = (SDWORD)screenWidth * mickeyScale - 1;
	}
	mickeyY += sState.lY;
	if (mickeyY < 0)
	{
		mickeyY = 0;
	}
	if (mickeyY >= (SDWORD)screenHeight * mickeyScale)
	{
		mickeyY = (SDWORD)screenHeight * mickeyScale - 1;
	}

	*pX = mickeyX / mickeyScale;
	*pY = mickeyY / mickeyScale;
	*pButtons = 0;
	if (sState.rgbButtons[0])
	{
		*pButtons |= DINP_LMB;
	}
	if (sState.rgbButtons[1])
	{
		*pButtons |= DINP_MMB;
	}
	if (sState.rgbButtons[2])
	{
		*pButtons |= DINP_RMB;
	}

	return TRUE;
}

// convert a direct input error to a string
STRING *DIErrorToString(HRESULT	dierror)
{
	switch (dierror)
	{
	case DIERR_INPUTLOST:
		return "Access to the device has been lost.\nIt must be re-acquired.";
		break;
	case DIERR_INVALIDPARAM:
		return "An invalid parameter was passed to the returning function,\n"
			   "or the object was not in a state that admitted the function\n"
			   "to be called.";
		break;
	case DIERR_NOTACQUIRED:
		return "The operation cannot be performed unless the device is acquired.";
		break;
	case DIERR_NOTINITIALIZED:
		return "This object has not been initialized";
		break;
	case E_PENDING:
		return "The data necessary to complete this operation is not yet available.";
		break;
	default:
		return "Direct Input error";
		break;
	}
}
