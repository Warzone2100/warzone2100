/*
 * DXInput.h
 *
 * Input functions using Direct Input.
 *
 */
#ifndef _DXInput_h
#define _DXInput_h

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

// The direct input object
extern LPDIRECTINPUT  psDI; 

// The direct input mouse object
extern LPDIRECTINPUTDEVICE psDIMouse; 

// Initialise the Direct X Input system
extern BOOL DInpInitialise(void);

// Shut down the DX input system
extern void DInpShutDown(void);

// Aquire or release the mouse
#define DINP_MOUSEACQUIRE       0
#define DINP_MOUSERELEASE       1
extern BOOL DInpMouseAcc(UDWORD aquireType);


// mouse buttons
#define DINP_LMB		0x01
#define DINP_MMB		0x02
#define DINP_RMB		0x04

// Get the current state of the mouse
extern BOOL DInpGetMouseState(SDWORD *pX, SDWORD *pY, SDWORD *pButtons);


// convert a direct input error to a string
extern STRING *DIErrorToString(HRESULT dierror);


#endif

