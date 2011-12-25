#include <QtCore/QCoreApplication>
#include "lib/framework/wzapp.h"
#include "lib/framework/input.h"
#include "lib/framework/utf.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/gamelib/gtime.h"
#include "src/warzoneconfig.h"
#include <SDL.h>
#include <QtCore/QSize>
#include <QtCore/QString>
#include "scrap.h"
#include "wz2100icon.h"
#include "cursors_sdl.h"

extern void mainLoop();

unsigned                screenWidth = 0;   // Declared in frameint.h.
unsigned                screenHeight = 0;  // Declared in frameint.h.
static unsigned         screenDepth = 0;
static SDL_Surface *    screen = NULL;

QCoreApplication *appPtr;


/* The possible states for keys */
enum KEY_STATE
{
	KEY_UP,
	KEY_PRESSED,
	KEY_DOWN,
	KEY_RELEASED,
	KEY_PRESSRELEASE,	// When a key goes up and down in a frame
	KEY_DOUBLECLICK,	// Only used by mouse keys
	KEY_DRAG			// Only used by mouse keys
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
static INPUT_STATE aMouseState[MOUSE_BAD];


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

/**************************/
/***     Misc support   ***/
/**************************/

#define WIDG_MAXSTR 80 // HACK, from widget.h

/* Put a character into a text buffer overwriting any text under the cursor */
QString wzGetSelection()
{
	QString retval;
	static char* scrap = NULL;
	int scraplen;

	get_scrap(T('T','E','X','T'), &scraplen, &scrap);
	if (scraplen > 0 && scraplen < WIDG_MAXSTR-2)
	{
		retval = QString::fromUtf8(scrap);
	}
	return retval;
}

void wzSetSwapInterval(bool swap)
{
	// TBD
}

bool wzGetSwapInterval()
{
	return false; // TBD
}

QList<QSize> wzAvailableResolutions()
{
	QList<QSize> list;
	int count;
	SDL_Rect **modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);
	for (count = 0; modes[count]; count++)
	{
		QSize s(modes[count]->w, modes[count]->h);
		list.push_back(s);
	}
	return list;
}

void wzShowMouse(bool visible)
{
	SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

int wzGetTicks()
{
	return SDL_GetTicks();
}

void wzFatalDialog(char const*)
{
	// no-op
}

void wzScreenFlip()
{
	SDL_GL_SwapBuffers();
}

void wzQuit()
{
	// Create a quit event to halt game loop.
	SDL_Event quitEvent;
	quitEvent.type = SDL_QUIT;
	SDL_PushEvent(&quitEvent);
}

void wzGrabMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_ON);
}

void wzReleaseMouse()
{
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

/**************************/
/***    Thread support  ***/
/**************************/

WZ_THREAD *wzThreadCreate(int (*threadFunc)(void *), void *data)
{
	return SDL_CreateThread(threadFunc, data);
}

int wzThreadJoin(WZ_THREAD *thread)
{
	int result;
	SDL_WaitThread(thread, &result);
	return result;
}

void wzThreadStart(WZ_THREAD *thread)
{
	(void)thread; // no-op
}

void wzYieldCurrentThread()
{
	SDL_Delay(40);
}

WZ_MUTEX *wzMutexCreate()
{
	return SDL_CreateMutex();
}

void wzMutexDestroy(WZ_MUTEX *mutex)
{
	SDL_DestroyMutex(mutex);
}

void wzMutexLock(WZ_MUTEX *mutex)
{
	SDL_LockMutex(mutex);
}

void wzMutexUnlock(WZ_MUTEX *mutex)
{
	SDL_UnlockMutex(mutex);
}

WZ_SEMAPHORE *wzSemaphoreCreate(int startValue)
{
	return SDL_CreateSemaphore(startValue);
}

void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore)
{
	SDL_DestroySemaphore(semaphore);
}

void wzSemaphoreWait(WZ_SEMAPHORE *semaphore)
{
	SDL_SemWait(semaphore);
}

void wzSemaphorePost(WZ_SEMAPHORE *semaphore)
{
	SDL_SemPost(semaphore);
}

/**************************/
/***     Main support   ***/
/**************************/

static inline KEY_CODE sdlKeyToKeyCode(SDLKey key)
{
	return (KEY_CODE)key;
}

static inline SDLKey keyCodeToSDLKey(KEY_CODE code)
{
	return (SDLKey)code;
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

	for (i = 0; i < MOUSE_BAD; i++)
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
void inputLoseFocus(void)
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

// TODO: Rewrite this silly thing
void setMousePos(uint16_t x, uint16_t y)
{
	static int mousewarp = -1;

	if (mousewarp == -1)
	{
		int val;

		mousewarp = 1;
		if ((val = getMouseWarp()))
		{
			mousewarp = !val;
		}
	}
	if (mousewarp)
		SDL_WarpMouse(x, y);
}

/*!
 * Handle keyboard events
 */
static void inputHandleKeyEvent(SDL_KeyboardEvent * keyEvent)
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
static void inputHandleMouseButtonEvent(SDL_MouseButtonEvent * buttonEvent)
{
	mouseXPos = buttonEvent->x;
	mouseYPos = buttonEvent->y;

	MOUSE_KEY_CODE mouseKeyCode;
	switch (buttonEvent->button)
	{
		default: return;  // Unknown button.
		case SDL_BUTTON_LEFT: mouseKeyCode = MOUSE_LMB; break;
		case SDL_BUTTON_MIDDLE: mouseKeyCode = MOUSE_MMB; break;
		case SDL_BUTTON_RIGHT: mouseKeyCode = MOUSE_RMB; break;
		case SDL_BUTTON_WHEELUP: mouseKeyCode = MOUSE_WUP; break;
		case SDL_BUTTON_WHEELDOWN: mouseKeyCode = MOUSE_WDN; break;
	}

	switch (buttonEvent->type)
	{
		case SDL_MOUSEBUTTONDOWN:
			aMouseState[mouseKeyCode].pressPos.x = mouseXPos;
			aMouseState[mouseKeyCode].pressPos.y = mouseYPos;
			if ( aMouseState[mouseKeyCode].state == KEY_UP
				|| aMouseState[mouseKeyCode].state == KEY_RELEASED
				|| aMouseState[mouseKeyCode].state == KEY_PRESSRELEASE )
			{
				if ( (buttonEvent->button != SDL_BUTTON_WHEELUP		//skip doubleclick check for wheel
					&& buttonEvent->button != SDL_BUTTON_WHEELDOWN))
				{
					// whether double click or not
					if ( realTime - aMouseState[mouseKeyCode].lastdown < DOUBLE_CLICK_INTERVAL )
					{
						aMouseState[mouseKeyCode].state = KEY_DOUBLECLICK;
						aMouseState[mouseKeyCode].lastdown = 0;
					}
					else
					{
						aMouseState[mouseKeyCode].state = KEY_PRESSED;
						aMouseState[mouseKeyCode].lastdown = realTime;
					}

				}
				else	//mouse wheel up/down was used, so notify.
				{
					aMouseState[mouseKeyCode].state = KEY_PRESSED;
					aMouseState[mouseKeyCode].lastdown = 0;
				}

				if (mouseKeyCode < 4) // Not the mousewheel
				{
					dragKey = mouseKeyCode;
					dragX = mouseXPos;
					dragY = mouseYPos;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			aMouseState[mouseKeyCode].releasePos.x = mouseXPos;
			aMouseState[mouseKeyCode].releasePos.y = mouseYPos;
			if (aMouseState[mouseKeyCode].state == KEY_PRESSED)
			{
				aMouseState[mouseKeyCode].state = KEY_PRESSRELEASE;
			}
			else if ( aMouseState[mouseKeyCode].state == KEY_DOWN
					|| aMouseState[mouseKeyCode].state == KEY_DRAG
					|| aMouseState[mouseKeyCode].state == KEY_DOUBLECLICK)
			{
				aMouseState[mouseKeyCode].state = KEY_RELEASED;
			}
			break;
		default:
			break;
	}
}


/*!
 * Handle mousemotion events
 */
static void inputHandleMouseMotionEvent(SDL_MouseMotionEvent * motionEvent)
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

void wzMain(int &argc, char **argv)
{
	appPtr = new QCoreApplication(argc, argv);  // For Qt-script.
}

bool wzMain2()
{
	//BEGIN **** Was in old frameInitialise. ****
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	uint32_t rmask = 0xff000000;
	uint32_t gmask = 0x00ff0000;
	uint32_t bmask = 0x0000ff00;
	uint32_t amask = 0x000000ff;
#else
	uint32_t rmask = 0x000000ff;
	uint32_t gmask = 0x0000ff00;
	uint32_t bmask = 0x00ff0000;
	uint32_t amask = 0xff000000;
#endif

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		debug( LOG_ERROR, "Error: Could not initialise SDL (%s).\n", SDL_GetError() );
		return false;
	}

	SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom((void*)wz2100icon.pixel_data, wz2100icon.width, wz2100icon.height, wz2100icon.bytes_per_pixel * 8,
	                                        wz2100icon.width * wz2100icon.bytes_per_pixel, rmask, gmask, bmask, amask), NULL);
	SDL_WM_SetCaption(PACKAGE_NAME, NULL);

	/* initialise all cursors */
	sdlInitCursors();
	//END **** Was in old frameInitialise. ****

	// TODO Fall back to windowed mode, if fullscreen mode fails.
	//BEGIN **** Was in old screenInitialise. ****
	unsigned        width = pie_GetVideoBufferWidth();
	unsigned        height = pie_GetVideoBufferHeight();
	unsigned        bitDepth = pie_GetVideoBufferDepth();
	unsigned        fsaa = war_getFSAA();
	bool            fullScreen = war_getFullscreen();
	bool            vsync = war_GetVsync();

	// Fetch the video info.
	const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

	if (video_info == NULL)
	{
		debug(LOG_ERROR, "SDL_GetVideoInfo failed.");
		return false;
	}

	if (width == 0 || height == 0)
	{
		pie_SetVideoBufferWidth(width = screenWidth = video_info->current_w);
		pie_SetVideoBufferHeight(height = screenHeight = video_info->current_h);
		pie_SetVideoBufferDepth(bitDepth = screenDepth = video_info->vfmt->BitsPerPixel);
	}
	else
	{
		screenWidth = width;
		screenHeight = height;
		screenDepth = bitDepth;
	}
	screenWidth = MAX(screenWidth, 640);
	screenHeight = MAX(screenHeight, 480);

	// The flags to pass to SDL_SetVideoMode.
	int video_flags  = SDL_OPENGL;    // Enable OpenGL in SDL.
	video_flags |= SDL_ANYFORMAT; // Don't emulate requested BPP if not available.

	if (fullScreen)
	{
		video_flags |= SDL_FULLSCREEN;
	}

	// Set the double buffer OpenGL attribute.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Enable vsync if requested by the user
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);

	// Enable FSAA anti-aliasing if and at the level requested by the user
	if (fsaa)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, fsaa);
	}

	int bpp = SDL_VideoModeOK(width, height, bitDepth, video_flags);
	if (!bpp)
	{
		debug(LOG_ERROR, "Video mode %dx%d@%dbpp is not supported!", width, height, bitDepth);
		return false;
	}
	switch (bpp)
	{
		case 32:
		case 24:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 16:
			info("Using colour depth of %i instead of %i.", bpp, screenDepth);
			info("You will experience graphics glitches!");
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
			break;
		case 8:
			debug(LOG_FATAL, "You don't want to play Warzone with a bit depth of %i, do you?", bpp);
			exit(1);
			break;
		default:
			debug(LOG_FATAL, "Unsupported bit depth: %i", bpp);
			exit(1);
			break;
	}

	screen = SDL_SetVideoMode(width, height, bpp, video_flags);
	if (!screen)
	{
		debug(LOG_ERROR, "SDL_SetVideoMode failed (%s).", SDL_GetError());
		return false;
	}
	int value = 0;
	if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1 || value == 0)
	{
		debug( LOG_FATAL, "OpenGL initialization did not give double buffering!" );
		debug( LOG_FATAL, "Double buffering is required for this game!");
		exit(1);
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//END **** Was in old screenInitialise. ****
	return true;
}

/*!
 * Activation (focus change) eventhandler
 */
static void handleActiveEvent(SDL_ActiveEvent * activeEvent)
{
#if 0
	// Ignore focus loss through SDL_APPMOUSEFOCUS, since it mostly happens accidentialy
	// active.state is a bitflag! Mixed events (eg. APPACTIVE|APPMOUSEFOCUS) will thus not be ignored.
	if ( activeEvent->state == SDL_APPMOUSEFOCUS )
	{
		setMouseScroll(activeEvent->gain);
		return;
	}
	if ( activeEvent->gain == 1 )
	{
		debug( LOG_NEVER, "WM_SETFOCUS");
		if (focusState != FOCUS_IN)
		{
			focusState = FOCUS_IN;

			// Don't pause in multiplayer!
			if (war_GetPauseOnFocusLoss() && !NetPlay.bComms)
			{
				gameTimeStart();
				audio_ResumeAll();
				cdAudio_Resume();
			}
			// enable scrolling
			setScrollPause(false);
			resetScroll();
		}
	}
	else
	{
		debug( LOG_NEVER, "WM_KILLFOCUS");
		if (focusState != FOCUS_OUT)
		{
			focusState = FOCUS_OUT;

			// Don't pause in multiplayer!
			if (war_GetPauseOnFocusLoss() && !NetPlay.bComms)
			{
				gameTimeStop();
				audio_PauseAll();
				cdAudio_Pause();
			}
			inputLooseFocus();
			// stop scrolling
			setScrollPause(true);
		}
	}
#endif
}

// Actual mainloop
void wzMain3()
{
	SDL_Event event;

	while (true)
	{
		/* Deal with any windows messages */
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				inputHandleKeyEvent(&event.key);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				inputHandleMouseButtonEvent(&event.button);
				break;
			case SDL_MOUSEMOTION:
				inputHandleMouseMotionEvent(&event.motion);
				break;
			case SDL_ACTIVEEVENT:
				handleActiveEvent(&event.active);
				break;
			case SDL_QUIT:
				//saveConfig();
				return;
			default:
				break;
			}
		}
		mainLoop();
		inputNewFrame();
	}
}

void wzShutdown()
{
	sdlFreeCursors();
	SDL_Quit();
	delete appPtr;
	appPtr = NULL;
}
