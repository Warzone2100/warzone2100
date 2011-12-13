#include <QtCore/QCoreApplication>
#include "lib/framework/wzapp.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/pieclip.h"
#include "src/warzoneconfig.h"
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
#include <QtCore/QSize>
#include <QtCore/QString>
#include "scrap.h"
#include "wz2100icon.h"

extern void mainLoop();

unsigned                screenWidth = 0;   // Declared in frameint.h.
unsigned                screenHeight = 0;  // Declared in frameint.h.
static unsigned         screenDepth = 0;
static SDL_Surface *    screen = NULL;

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

void wzSetCursor(CURSOR index)
{
	// TBD
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
	// no-op
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

void wzMain(int &argc, char **argv)
{
	QCoreApplication app(argc, argv);  // For Qt-script.
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
	//initCursors();
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

void wzMain3()
{
	mainLoop();
}
