/*
 * Input.c
 *
 * Processes all keyboard and mouse input.
 *
 */

#include <stdio.h>

#pragma warning (disable : 4201 4214 4115 4514)
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <ddraw.h>
#pragma warning (default : 4201 4214 4115)

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

/* The input buffer printf's */
//#define DEBUG_GROUP1
#include "Types.h"
#include "Debug.h"
#include "Input.h"
#include "Screen.h"
#include "frameint.h"
#include "Fractions.h"
#include "Frame.h"

/* The possible states for keys */
typedef enum _key_state
{
	KEY_UP,
	KEY_PRESSED,
	KEY_DOWN,
	KEY_RELEASED,
	KEY_PRESSRELEASE,	// When a key goes up and down in a frame
	KEY_DOUBLECLICK,	// Only used by mouse keys
	KEY_DRAG,			// Only used by mouse keys
} KEY_STATE;


/* The current state of the keyboard */
static KEY_STATE aKeyState[KEY_MAXSCAN];

/* Mouse wheel stuff */
static	UDWORD	oldWheelPos = 0;
static	BOOL	bMouseWheelForward = FALSE;
static	BOOL	bMouseWheelBackwards = FALSE;
static	BOOL	bMouseWheelStatic = TRUE;

/* The current location of the mouse */
static SDWORD		mouseXPos, mouseYPos;

/* How far the mouse has to move to start a drag */
#define DRAG_THRESHOLD	5

/* Which button is being used for a drag */
static MOUSE_KEY_CODE	dragKey;

/* The start of a possible drag by the mouse */
static SDWORD			dragX, dragY;

/* The current mouse button state */
static KEY_STATE aMouseState[3];

#ifdef WIN32
/* The size of the input buffer */
#define INPUT_MAXSTR	512

/* The input string buffer */
static UDWORD	pInputBuffer[INPUT_MAXSTR];
static UDWORD	*pStartBuffer, *pEndBuffer;

void keyScanToString(KEY_CODE code, STRING *ascii, UDWORD maxStringSize)
{
#ifdef PSX
	DBPRINTF(("keyscantostring ... not installed\n"));
#else
	if(code == KEY_MAXSCAN)
	{
		strcpy(ascii,"???");
		return;
	}
	ASSERT(((code >= 0) && (code <= KEY_MAXSCAN), "Invalid key code: %d", code));
	GetKeyNameText((UDWORD)((UWORD)code<<16),ascii,maxStringSize);
#endif
}


/* Initialise the input module */
void inputInitialise(void)
{
	UDWORD	i;

	for(i=0; i<KEY_MAXSCAN; i++)
	{
		aKeyState[i] = KEY_UP;
	}

	for (i=0; i<3; i++)
	{
		aMouseState[i] = KEY_UP;
	}

	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;

	dragX = mouseXPos = screenWidth/2;
	dragY = mouseYPos = screenHeight/2;
	dragKey = MOUSE_LMB;
}

/* add count copies of the characater code to the input buffer */
void inputAddBuffer(UDWORD code, UDWORD count)
{
	UDWORD	*pNext;

	/* Calculate what pEndBuffer will be set to next */
	pNext = pEndBuffer + 1;
	if (pNext >= pInputBuffer + INPUT_MAXSTR)
	{
		pNext = pInputBuffer;
	}

	while (pNext != pStartBuffer && count > 0)
	{
		/* Store the character */
		*pEndBuffer = code;
		pEndBuffer = pNext;
		count -= 1;

		/* Calculate what pEndBuffer will be set to next */
		pNext = pEndBuffer + 1;
		if (pNext >= pInputBuffer + INPUT_MAXSTR)
		{
			pNext = pInputBuffer;
		}
	}
}


/* Clear the input buffer */
void inputClearBuffer(void)
{
	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;
}


/* Return the next key press or 0 if no key in the buffer.
 * The key returned will have been remaped to the correct ascii code for the
 * windows key map.
 * All key presses are buffered up (including windows auto repeat).
 */
UDWORD inputGetKey(void)
{
	UDWORD	retVal;

	if (pStartBuffer != pEndBuffer)
	{
		retVal = *pStartBuffer;
		pStartBuffer += 1;

		if (pStartBuffer >= pInputBuffer + INPUT_MAXSTR)
		{
			pStartBuffer = pInputBuffer;
		}
	}
	else
	{
		retVal = 0;
	}

	return retVal;
}
#endif
/* Deal with windows messages to maintain the state of the keyboard and mouse */
void inputProcessMessages(UINT message, WPARAM wParam, LPARAM lParam)
{
	UDWORD	code,i, repeat, vk;
	FRACT	divX,divY;
	UDWORD	scrX,scrY;


	/* Loose the warning message */
	(void)wParam;

	/* Calculate the scan code for the key */
	code = (lParam >> 16) & 0x1ff;

	switch (message)
	{
		/* Deal with keyboard messages */
	case WM_KEYDOWN:
		/* Check for any of the virtual keys for the input buffer */
		repeat = lParam & 0xf;
		switch (wParam)
		{
		case VK_LEFT:
			vk = INPBUF_LEFT;
			break;
		case VK_RIGHT:
			vk = INPBUF_RIGHT;
			break;
		case VK_UP:
			vk = INPBUF_UP;
			break;
		case VK_DOWN:
			vk = INPBUF_DOWN;
			break;
		case VK_HOME:
			vk = INPBUF_HOME;
			break;
		case VK_END:
			vk = INPBUF_END;
			break;
		case VK_INSERT:
			vk = INPBUF_INS;
			break;
		case VK_DELETE:
			vk = INPBUF_DEL;
			break;
		case VK_PRIOR:
			vk = INPBUF_PGUP;
			break;
		case VK_NEXT:
			vk = INPBUF_PGDN;
			break;
		default:
			/* no useful data for the buffer so set repeat to 0 */
			repeat = 0;
			break;
		}
		if (repeat > 0)
		{
#ifdef PSX
			DBPRINTF(("WM_KEYDOWN %x %x\n",vk,repeat));
#endif

#ifdef WIN32
			DBP1(("Code: %x\n", vk));
			inputAddBuffer(vk, repeat);
#endif
		}


		/*************************************************************/
		/* NO BREAK HERE as keydown gives normal key presses as well */
		/*************************************************************/
	case WM_SYSKEYDOWN:

#ifdef PSX
		DBPRINTF(("WM_KEYDOWN %d\n",code));
#endif

		if ((aKeyState[code] == KEY_UP) ||
			(aKeyState[code] == KEY_RELEASED) ||
			(aKeyState[code] == KEY_PRESSRELEASE))
		{
			aKeyState[code] = KEY_PRESSED;
		}
		break;

		/*
	case WM_MOUSEWHEEL:	// not defined in non-NT.....bugger.
		wheelVal = HIWORD(wParam);
		if(wheelVal<0)
		{
			bMouseWheelForward = TRUE;
			bMouseWheelBackwards = FALSE;
			bMouseWheelStatic = FALSE;
		}
		if(wheelVal>0)
		{
		 	bMouseWheelForward = FALSE;
			bMouseWheelBackwards = TRUE;
			bMouseWheelStatic = FALSE;
		}
		else
		{
			bMouseWheelForward = FALSE;
			bMouseWheelBackwards = FALSE;
			bMouseWheelStatic = TRUE;
		}

		break;
	   */
	case WM_KEYUP:
	case WM_SYSKEYUP:

#ifdef PSX
		DBPRINTF(("WM_KEYUP %d\n",code));
#endif

		if (aKeyState[code] == KEY_PRESSED)
		{
			aKeyState[code] = KEY_PRESSRELEASE;
		}
		else if (aKeyState[code] == KEY_DOWN)
		{
			aKeyState[code] = KEY_RELEASED;
		}
		break;

		/* Deal with mouse messages */
	case WM_MOUSEMOVE:
		if(!mouseDown(MOUSE_MMB))
		{
			/* store the current mouse position */
			mouseXPos = LOWORD(lParam);
			mouseYPos = HIWORD(lParam);
	#ifdef WIN32		// ffs am
			if(bRunningUnderGlide)
			{
				scrX = GetSystemMetrics(SM_CXFULLSCREEN);
				scrY = GetSystemMetrics(SM_CYFULLSCREEN);

				divX = MAKEFRACT(mouseXPos) / scrX;
				divY = MAKEFRACT(mouseYPos) / scrY;

				mouseXPos = MAKEINT(divX*screenWidth);
				mouseYPos = MAKEINT(divY*screenHeight);
			}
	#endif
			/*
			if(mouseXPos>=screenWidth)
			{
				mouseXPos = screenWidth-1;
			}
			if(mouseYPos >= screenHeight)
			{
				mouseYPos = screenHeight-1;
			}
			*/

			/* now see if a drag has started */
			if ((aMouseState[dragKey] == KEY_PRESSED ||
				 aMouseState[dragKey] == KEY_DOWN) &&
				(ABSDIF(dragX,mouseXPos) > DRAG_THRESHOLD ||
				 ABSDIF(dragY,mouseYPos) > DRAG_THRESHOLD))
			{
	//		DBPRINTF(("dragging\n"));
				aMouseState[dragKey] = KEY_DRAG;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		if ((aMouseState[MOUSE_LMB] == KEY_UP) ||
			(aMouseState[MOUSE_LMB] == KEY_RELEASED) ||
			(aMouseState[MOUSE_LMB] == KEY_PRESSRELEASE))
		{
			aMouseState[MOUSE_LMB] = KEY_PRESSED;
			dragKey = MOUSE_LMB;
			dragX = mouseXPos;
			dragY = mouseYPos;
		}
		break;
	case WM_LBUTTONUP:
		if (aMouseState[MOUSE_LMB] == KEY_PRESSED)
		{
			aMouseState[MOUSE_LMB] = KEY_PRESSRELEASE;
		}
		else if (aMouseState[MOUSE_LMB] == KEY_DOWN ||
				 aMouseState[MOUSE_LMB] == KEY_DRAG)
		{
			aMouseState[MOUSE_LMB] = KEY_RELEASED;
		}
		break;
	case WM_LBUTTONDBLCLK:
		aMouseState[MOUSE_LMB] = KEY_DOUBLECLICK;
		break;
	case WM_RBUTTONDOWN:
		if ((aMouseState[MOUSE_RMB] == KEY_UP) ||
			(aMouseState[MOUSE_RMB] == KEY_RELEASED) ||
			(aMouseState[MOUSE_RMB] == KEY_PRESSRELEASE))
		{
			aMouseState[MOUSE_RMB] = KEY_PRESSED;
			dragKey = MOUSE_RMB;
			dragX = mouseXPos;
			dragY = mouseYPos;
		}
		break;
	case WM_RBUTTONUP:
		if (aMouseState[MOUSE_RMB] == KEY_PRESSED)
		{
			aMouseState[MOUSE_RMB] = KEY_PRESSRELEASE;
		}
		else if (aMouseState[MOUSE_RMB] == KEY_DOWN ||
				 aMouseState[MOUSE_RMB] == KEY_DRAG)
		{
			aMouseState[MOUSE_RMB] = KEY_RELEASED;
		}
		break;
	case WM_RBUTTONDBLCLK:
		aMouseState[MOUSE_RMB] = KEY_DOUBLECLICK;
		break;
	case WM_MBUTTONDOWN:
		if ((aMouseState[MOUSE_MMB] == KEY_UP) ||
			(aMouseState[MOUSE_MMB] == KEY_RELEASED) ||
			(aMouseState[MOUSE_MMB] == KEY_PRESSRELEASE))
		{
			aMouseState[MOUSE_MMB] = KEY_PRESSED;
			dragKey = MOUSE_MMB;
			dragX = mouseXPos;
			dragY = mouseYPos;
		}
		break;
	case WM_MBUTTONUP:
		if (aMouseState[MOUSE_MMB] == KEY_PRESSED)
		{
			aMouseState[MOUSE_MMB] = KEY_PRESSRELEASE;
		}
		else if (aMouseState[MOUSE_MMB] == KEY_DOWN ||
				 aMouseState[MOUSE_MMB] == KEY_DRAG)
		{
			aMouseState[MOUSE_MMB] = KEY_RELEASED;
		}
		break;
	case WM_MBUTTONDBLCLK:
		aMouseState[MOUSE_MMB] = KEY_DOUBLECLICK;
		break;
	case WM_KILLFOCUS:
		/* Lost the window focus, have to take this as a global key up */
		for(i=0; i<KEY_MAXSCAN; i++)
		{
			if ((aKeyState[i] == KEY_PRESSED) ||
				(aKeyState[i] == KEY_DOWN))
			{
				aKeyState[i] = KEY_RELEASED;
			}
		}
		for (i=0; i<3; i++)
		{
			if ((aMouseState[i] == KEY_PRESSED) ||
				(aMouseState[i] == KEY_DOWN) ||
				(aMouseState[i] == KEY_DRAG))
			{
				aMouseState[i] = KEY_RELEASED;
			}
		}
		break;
	case WM_CHAR:
		DBP1(("Code: %d\n", wParam));
		/* Get the repeat count */
		repeat = lParam & 0xf;
		/* Store the repeat count number of characters
		   while there is space in the buffer */
#ifdef WIN32
		inputAddBuffer(wParam, repeat);
#endif
#ifdef PSX
		DBPRINTF(("WM_CHAR  %d %d\n",wParam,repeat));
#endif
		break;
	default:
		break;
	}
}

/* This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
void inputNewFrame(void)
{
	UDWORD i;

#ifdef WIN32
	/* Do the keyboard */
	for (i=0; i< KEY_MAXSCAN; i++)
	{
		if (aKeyState[i] == KEY_PRESSED)
		{
			aKeyState[i] = KEY_DOWN;
		}
		else if ((aKeyState[i] == KEY_RELEASED) ||
				 (aKeyState[i] == KEY_PRESSRELEASE))
		{
			aKeyState[i] = KEY_UP;
		}
	}
#else
	/* Do the keyboard */
	for (i=0; i< KEY_MAXSCAN; i++)
	{
		aKeyState[i] = KEY_UP;
	}
#endif

	/* Do the mouse */
	for(i=0; i<3; i++)
	{
		if (aMouseState[i] == KEY_PRESSED)
		{
			aMouseState[i] = KEY_DOWN;
		}
		else if ((aMouseState[i] == KEY_RELEASED) ||
				 (aMouseState[i] == KEY_DOUBLECLICK) ||
				 (aMouseState[i] == KEY_PRESSRELEASE))
		{
			aMouseState[i] = KEY_UP;
		}
	}
}

/* This returns true if the key is currently depressed */
BOOL keyDown(KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < KEY_MAXSCAN), "Invalid key code: %d", code));
	return (aKeyState[code] != KEY_UP);
}

/* This returns true if the key went from being up to being down this frame */
BOOL keyPressed(KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < KEY_MAXSCAN), "Invalid key code: %d", code));
	return ((aKeyState[code] == KEY_PRESSED) || (aKeyState[code] == KEY_PRESSRELEASE));
}

/* This returns true if the key went from being down to being up this frame */
BOOL keyReleased(KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < KEY_MAXSCAN), "Invalid key code: %d", code));
	return ((aKeyState[code] == KEY_RELEASED) || (aKeyState[code] == KEY_PRESSRELEASE));
}

/* Return the X coordinate of the mouse */
SDWORD mouseX(void)
{
	return mouseXPos;
}

/* Return the Y coordinate of the mouse */
SDWORD mouseY(void)
{
	return mouseYPos;
}

/* The next three are mutually exclusive */

/* True if mouse wheel moved forward in last frame */
BOOL	mouseWheelForward( void)
{
	return(bMouseWheelForward);
}

/* True if mouse wheel moved backwards in last frame */
BOOL	mouseWheelBackwards( void)
{
	return(bMouseWheelBackwards);
}

/* True if mouse wheel remained still in last frame */
BOOL	mouseWheelStatic( void)
{
	return(bMouseWheelStatic);
}


/* This returns true if the mouse key is currently depressed */
BOOL mouseDown(MOUSE_KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < 3), "Invalid mouse key code: %d", code));
	return (aMouseState[code] != KEY_UP);
}

/* This returns true if the mouse key was double clicked */
BOOL mouseDClicked(MOUSE_KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < 3), "Invalid mouse key code: %d", code));
	return (aMouseState[code] == KEY_DOUBLECLICK);
}

/* This returns true if the mouse key went from being up to being down this frame */
BOOL mousePressed(MOUSE_KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < 3), "Invalid mouse key code: %d", code));
	return ((aMouseState[code] == KEY_PRESSED) ||
			(aMouseState[code] == KEY_PRESSRELEASE));
}

/* This returns true if the mouse key went from being down to being up this frame */
BOOL mouseReleased(MOUSE_KEY_CODE code)
{
	ASSERT(((code >= 0) && (code < 3), "Invalid mouse key code: %d", code));
	return ((aMouseState[code] == KEY_RELEASED) ||
			(aMouseState[code] == KEY_DOUBLECLICK) ||
			(aMouseState[code] == KEY_PRESSRELEASE));
}

/* Check for a mouse drag, return the drag start coords if dragging */
BOOL mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py)
{
	ASSERT(((code >= 0) && (code < 3), "Invalid mouse key code: %d", code));
	if (aMouseState[code] == KEY_DRAG)
	{
		*px = dragX;
		*py = dragY;
		return TRUE;
	}

	return FALSE;
}