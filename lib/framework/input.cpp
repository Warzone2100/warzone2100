/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
*/

/*
 * Input.c
 *
 * Processes all keyboard and mouse input.
 *
 */
#include "frame.h"

/* Allow frame header files to be singly included */
#define FRAME_LIB_INCLUDE

#include "input.h"
#include "frameint.h"
#include "configfile.h"

#include "lib/gamelib/gtime.h"

/* The possible states for keys */
enum KEY_STATE
{
	KEY_UP,
	KEY_PRESSED,
	KEY_DOWN,
	KEY_RELEASED,
	KEY_PRESSRELEASE,	// When a key goes up and down in a frame
	KEY_DOUBLECLICK,	// Only used by mouse keys
	KEY_DRAG,			// Only used by mouse keys
};

struct INPUT_STATE
{
	KEY_STATE state; /// Last key/mouse state
	UDWORD lastdown; /// last key/mouse button down timestamp
	Vector2i pressPos;    ///< Location of last mouse press event.
	Vector2i releasePos;  ///< Location of last mouse release event.
};

/// constant for the interval between 2 singleclicks for doubleclick event in ms
#define DOUBLE_CLICK_INTERVAL 250

/* The current state of the keyboard */
static INPUT_STATE aKeyState[KEY_MAXSCAN];

/* The current location of the mouse */
static Uint16 mouseXPos, mouseYPos;

/* How far the mouse has to move to start a drag */
#define DRAG_THRESHOLD	5

/* Which button is being used for a drag */
static MOUSE_KEY_CODE dragKey;

/* The start of a possible drag by the mouse */
static SDWORD dragX, dragY;

/* The current mouse button state */
static INPUT_STATE aMouseState[6];


/* The size of the input buffer */
#define INPUT_MAXSTR	512

/* The input string buffer */
struct InputKey
{
	UDWORD key;
	utf_32_char unicode;
};
static InputKey	pInputBuffer[INPUT_MAXSTR];
static InputKey	*pStartBuffer, *pEndBuffer;


static KEY_CODE sdlKeyToKeyCode(SDLKey key)
{
	return (KEY_CODE)key;
}

static SDLKey keyCodeToSDLKey(KEY_CODE code)
{
	return (SDLKey)code;
}


void keyScanToString(KEY_CODE code, char *ascii, UDWORD maxStringSize)
{
	if(keyCodeToSDLKey(code) == KEY_MAXSCAN)
	{
		strcpy(ascii,"???");
		return;
	}
	else if (code == KEY_LCTRL)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Ctrl");
		return;
	}
	else if (code == KEY_LSHIFT)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Shift");
		return;
	}
	else if (code == KEY_LALT)
	{
		// shortcuts with modifier keys work with either key.
		strcpy(ascii, "Alt");
		return;
	}
	else if (code == KEY_LMETA)
	{
		// shortcuts with modifier keys work with either key.
#ifdef WZ_OS_MAC
		strcpy(ascii, "Cmd");
#else
		strcpy(ascii, "Meta");
#endif
		return;
	}
	ASSERT( keyCodeToSDLKey(code) < KEY_MAXSCAN, "Invalid key code: %d", code );
	snprintf(ascii, maxStringSize, "%s", SDL_GetKeyName(keyCodeToSDLKey(code)));
	if (ascii[0] >= 'a' && ascii[0] <= 'z' && ascii[1] != 0)
	{
		// capitalize
		ascii[0] += 'A'-'a';
	}
}


/* Initialise the input module */
void inputInitialise(void)
{
	unsigned int i;

	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}

	for (i = 0; i < 6; i++)
	{
		aMouseState[i].state = KEY_UP;
	}

	pStartBuffer = pInputBuffer;
	pEndBuffer = pInputBuffer;

	dragX = mouseXPos = screenWidth/2;
	dragY = mouseYPos = screenHeight/2;
	dragKey = MOUSE_LMB;

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

// Cyclic increment.
static InputKey *inputPointerNext(InputKey *p)
{
    return p + 1 == pInputBuffer + INPUT_MAXSTR ? pInputBuffer : p + 1;
}

/* add count copies of the characater code to the input buffer */
static void inputAddBuffer(UDWORD key, utf_32_char unicode)
{
	/* Calculate what pEndBuffer will be set to next */
	InputKey	*pNext = inputPointerNext(pEndBuffer);

	if (pNext == pStartBuffer)
	{
		return;  // Buffer full.
	}

	// Add key to buffer.
	pEndBuffer->key = key;
	pEndBuffer->unicode = unicode;
	pEndBuffer = pNext;
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
UDWORD inputGetKey(utf_32_char *unicode)
{
	UDWORD	retVal;

	if (pStartBuffer == pEndBuffer)
	{
	    return 0;  // Buffer empty.
	}

	retVal = pStartBuffer->key;
	if (unicode)
	{
	    *unicode = pStartBuffer->unicode;
	}
	if (!retVal)
	{
	    retVal = ' ';  // Don't return 0 if we got a virtual key, since that's interpreted as no input.
	}
	pStartBuffer = inputPointerNext(pStartBuffer);

	return retVal;
}


/*!
 * Handle keyboard events
 */
void inputHandleKeyEvent(SDL_KeyboardEvent * keyEvent)
{
	UDWORD code, vk;
	utf_32_char unicode = keyEvent->keysym.unicode;

	switch (keyEvent->type)
	{
		case SDL_KEYDOWN:
			switch (keyEvent->keysym.sym)
			{
				case SDLK_LEFT:
					vk = INPBUF_LEFT;
					break;
				case SDLK_RIGHT:
					vk = INPBUF_RIGHT;
					break;
				case SDLK_UP:
					vk = INPBUF_UP;
					break;
				case SDLK_DOWN:
					vk = INPBUF_DOWN;
					break;
				case SDLK_HOME:
					vk = INPBUF_HOME;
					break;
				case SDLK_END:
					vk = INPBUF_END;
					break;
				case SDLK_INSERT:
					vk = INPBUF_INS;
					break;
				case SDLK_DELETE:
					vk = INPBUF_DEL;
					break;
				case SDLK_PAGEUP:
					vk = INPBUF_PGUP;
					break;
				case SDLK_PAGEDOWN:
					vk = INPBUF_PGDN;
					break;
				default:
					vk = keyEvent->keysym.sym;
					break;
			}

			debug( LOG_INPUT, "Key Code (pressed): 0x%x, %d, [%c] SDLkey=[%s]", vk, vk, (vk < 128) && (vk > 31) ? (char) vk : '?' , SDL_GetKeyName(keyCodeToSDLKey((KEY_CODE)keyEvent->keysym.sym)));
			if (unicode < 32)
			{
				unicode = 0;
			}
			inputAddBuffer(vk, unicode);

			code = sdlKeyToKeyCode(keyEvent->keysym.sym);
			if ( aKeyState[code].state == KEY_UP ||
				 aKeyState[code].state == KEY_RELEASED ||
				 aKeyState[code].state == KEY_PRESSRELEASE )
			{
				//whether double key press or not
					aKeyState[code].state = KEY_PRESSED;
					aKeyState[code].lastdown = 0;
			}
			break;
		case SDL_KEYUP:
			code = sdlKeyToKeyCode(keyEvent->keysym.sym);
			if (aKeyState[code].state == KEY_PRESSED)
			{
				aKeyState[code].state = KEY_PRESSRELEASE;
			}
			else if (aKeyState[code].state == KEY_DOWN )
			{
				aKeyState[code].state = KEY_RELEASED;
			}
			break;
		default:
			break;
	}
}


/*!
 * Handle mousebutton events
 */
void inputHandleMouseButtonEvent(SDL_MouseButtonEvent * buttonEvent)
{
	mouseXPos = buttonEvent->x;
	mouseYPos = buttonEvent->y;

	switch (buttonEvent->type)
	{
		case SDL_MOUSEBUTTONDOWN:
			aMouseState[buttonEvent->button].pressPos.x = mouseXPos;
			aMouseState[buttonEvent->button].pressPos.y = mouseYPos;
			if ( aMouseState[buttonEvent->button].state == KEY_UP
				|| aMouseState[buttonEvent->button].state == KEY_RELEASED
				|| aMouseState[buttonEvent->button].state == KEY_PRESSRELEASE )
			{
				if ( (buttonEvent->button != SDL_BUTTON_WHEELUP		//skip doubleclick check for wheel
					&& buttonEvent->button != SDL_BUTTON_WHEELDOWN))
				{
				//whether double click or not
					if ( realTime - aMouseState[buttonEvent->button].lastdown < DOUBLE_CLICK_INTERVAL )
					{
						aMouseState[buttonEvent->button].state = KEY_DOUBLECLICK;
						aMouseState[buttonEvent->button].lastdown = 0;
					}
					else
					{
						aMouseState[buttonEvent->button].state = KEY_PRESSED;
						aMouseState[buttonEvent->button].lastdown = realTime;
					}

				}
				else	//mouse wheel up/down was used, so notify.
				{
					aMouseState[buttonEvent->button].state = KEY_PRESSED;
					aMouseState[buttonEvent->button].lastdown = 0;
				}

				if (buttonEvent->button < 4) // Not the mousewheel
				{
					dragKey = (MOUSE_KEY_CODE)buttonEvent->button;
					dragX = mouseXPos;
					dragY = mouseYPos;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			aMouseState[buttonEvent->button].releasePos.x = mouseXPos;
			aMouseState[buttonEvent->button].releasePos.y = mouseYPos;
			if (aMouseState[buttonEvent->button].state == KEY_PRESSED)
			{
				aMouseState[buttonEvent->button].state = KEY_PRESSRELEASE;
			}
			else if ( aMouseState[buttonEvent->button].state == KEY_DOWN
					|| aMouseState[buttonEvent->button].state == KEY_DRAG
					|| aMouseState[buttonEvent->button].state == KEY_DOUBLECLICK)
			{
				aMouseState[buttonEvent->button].state = KEY_RELEASED;
			}
			break;
		default:
			break;
	}
}


/*!
 * Handle mousemotion events
 */
void inputHandleMouseMotionEvent(SDL_MouseMotionEvent * motionEvent)
{
	switch (motionEvent->type)
	{
		case SDL_MOUSEMOTION:
			/* store the current mouse position */
			mouseXPos = motionEvent->x;
			mouseYPos = motionEvent->y;

			/* now see if a drag has started */
			if ((aMouseState[dragKey].state == KEY_PRESSED ||
			     aMouseState[dragKey].state == KEY_DOWN) &&
			    (ABSDIF(dragX, mouseXPos) > DRAG_THRESHOLD ||
			     ABSDIF(dragY, mouseYPos) > DRAG_THRESHOLD))
			{
				aMouseState[dragKey].state = KEY_DRAG;
			}
			break;
		default:
			break;
	}
}


/*!
 * This is called once a frame so that the system can tell
 * whether a key was pressed this turn or held down from the last frame.
 */
void inputNewFrame(void)
{
	unsigned int i;

	/* Do the keyboard */
	for (i = 0; i < KEY_MAXSCAN; i++)
	{
		if (aKeyState[i].state == KEY_PRESSED)
		{
			aKeyState[i].state = KEY_DOWN;
		}
		else if ( aKeyState[i].state == KEY_RELEASED  ||
		          aKeyState[i].state == KEY_PRESSRELEASE )
		{
			aKeyState[i].state = KEY_UP;
		}
	}

	/* Do the mouse */
	for (i = 0; i < 6; i++)
	{
		if (aMouseState[i].state == KEY_PRESSED)
		{
			aMouseState[i].state = KEY_DOWN;
		}
		else if ( aMouseState[i].state == KEY_RELEASED
		       || aMouseState[i].state == KEY_DOUBLECLICK
		       || aMouseState[i].state == KEY_PRESSRELEASE )
		{
			aMouseState[i].state = KEY_UP;
		}
	}
}

/*!
 * Release all keys (and buttons) when we loose focus
 */
// FIXME This seems to be totally ignored! (Try switching focus while the dragbox is open)
void inputLooseFocus(void)
{
	unsigned int i;

	/* Lost the window focus, have to take this as a global key up */
	for(i = 0; i < KEY_MAXSCAN; i++)
	{
		aKeyState[i].state = KEY_UP;
	}
	for (i = 0; i < 6; i++)
	{
		aMouseState[i].state = KEY_UP;
	}
}

/* This returns true if the key is currently depressed */
bool keyDown(KEY_CODE code)
{
	ASSERT( keyCodeToSDLKey(code) < KEY_MAXSCAN, "Invalid key code: %d", code );
	return (aKeyState[code].state != KEY_UP);
}

/* This returns true if the key went from being up to being down this frame */
bool keyPressed(KEY_CODE code)
{
	ASSERT( keyCodeToSDLKey(code) < KEY_MAXSCAN, "Invalid key code: %d", code );
	return ((aKeyState[code].state == KEY_PRESSED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the key went from being down to being up this frame */
bool keyReleased(KEY_CODE code)
{
	ASSERT( keyCodeToSDLKey(code) < KEY_MAXSCAN, "Invalid key code: %d", code );
	return ((aKeyState[code].state == KEY_RELEASED) || (aKeyState[code].state == KEY_PRESSRELEASE));
}

/* Return the X coordinate of the mouse */
Uint16 mouseX(void)
{
	return mouseXPos;
}

/* Return the Y coordinate of the mouse */
Uint16 mouseY(void)
{
	return mouseYPos;
}

Vector2i mousePressPos(MOUSE_KEY_CODE code)
{
	return aMouseState[code].pressPos;
}

Vector2i mouseReleasePos(MOUSE_KEY_CODE code)
{
	return aMouseState[code].releasePos;
}

/* This returns true if the mouse key is currently depressed */
bool mouseDown(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state != KEY_UP) ||

	       // holding down LMB and RMB counts as holding down MMB
	       (code == MOUSE_MMB && aMouseState[MOUSE_LMB].state != KEY_UP && aMouseState[MOUSE_RMB].state != KEY_UP);
}

/* This returns true if the mouse key was double clicked */
bool mouseDClicked(MOUSE_KEY_CODE code)
{
	return (aMouseState[code].state == KEY_DOUBLECLICK);
}

/* This returns true if the mouse key went from being up to being down this frame */
bool mousePressed(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_PRESSED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* This returns true if the mouse key went from being down to being up this frame */
bool mouseReleased(MOUSE_KEY_CODE code)
{
	return ((aMouseState[code].state == KEY_RELEASED) ||
			(aMouseState[code].state == KEY_DOUBLECLICK) ||
			(aMouseState[code].state == KEY_PRESSRELEASE));
}

/* Check for a mouse drag, return the drag start coords if dragging */
bool mouseDrag(MOUSE_KEY_CODE code, UDWORD *px, UDWORD *py)
{
	if ((aMouseState[code].state == KEY_DRAG) ||

	    // dragging LMB and RMB counts as dragging MMB
	    (code == MOUSE_MMB && ((aMouseState[MOUSE_LMB].state == KEY_DRAG && aMouseState[MOUSE_RMB].state != KEY_UP) ||
	     (aMouseState[MOUSE_LMB].state != KEY_UP && aMouseState[MOUSE_RMB].state == KEY_DRAG))))
	{
		*px = dragX;
		*py = dragY;
		return true;
	}

	return false;
}

void SetMousePos(Uint16 x, Uint16 y)
{
	static int mousewarp = -1;

	if (mousewarp == -1)
	{
		int val;

		mousewarp = 1;
		if (getWarzoneKeyNumeric("nomousewarp", &val))
		{
			mousewarp = !val;
		}
	}
	if (mousewarp)
		SDL_WarpMouse(x, y);
}
