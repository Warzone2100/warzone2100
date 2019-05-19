/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 * screen.cpp
 *
 * Basic double buffered display using OpenGL.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/ivis_opengl/png_util.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/framework/physfs_ext.h"

#include "screen.h"
#include "src/console.h"
#include "src/levels.h"
#include "lib/framework/wzapp.h"

#include <time.h>
#include <vector>
#include <algorithm>
#include <physfs.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

/* global used to indicate preferred internal OpenGL format */
bool wz_texture_compression = 0;

static bool		bBackDrop = false;
static char		screendump_filename[PATH_MAX];
static bool		screendump_required = false;

static GFX *backdropGfx = nullptr;
static bool backdropIsMapPreview = false;

static bool perfStarted = false;
struct PERF_STORE
{
	uint64_t counters[PERF_COUNT];
};
static std::vector<PERF_STORE> perfList;
static PERF_POINT queryActive = PERF_COUNT;

static int preview_width = 0, preview_height = 0;
static Vector2i player_pos[MAX_PLAYERS];
static WzText player_Text[MAX_PLAYERS];
static bool mappreview = false;

/* Initialise the double buffered display */
bool screenInitialise()
{
	screenWidth = MAX(screenWidth, 640);
	screenHeight = MAX(screenHeight, 480);

	pie_Skybox_Init();

	// Generate backdrop render
	backdropGfx = new GFX(GFX_TEXTURE, 2);

	return true;
}

bool wzPerfAvailable()
{
	return gfx_api::context::get().debugPerfAvailable();
}

void wzPerfStart()
{
	perfStarted = gfx_api::context::get().debugPerfStart(perfList.size());
}

void wzPerfWriteOut(const std::vector<PERF_STORE> &perfList, const WzString &outfile)
{
	PHYSFS_file *fileHandle = PHYSFS_openWrite(outfile.toUtf8().c_str());
	if (fileHandle)
	{
		const char fileHeader[] = "START, EFF, TERRAIN, SKY, LOAD, PRTCL, WATER, MODELS, MISC, GUI\n";
		if (WZ_PHYSFS_writeBytes(fileHandle, fileHeader, sizeof(fileHeader)) != sizeof(fileHeader))
		{
			// Failed to write header to file
			debug(LOG_ERROR, "wzPerfWriteOut: could not write header to %s; PHYSFS error: %s", outfile.toUtf8().c_str(), WZ_PHYSFS_getLastError());
			PHYSFS_close(fileHandle);
			return;
		}
		for (size_t i = 0; i < perfList.size(); i++)
		{
			WzString line;
			line += WzString::number(perfList[i].counters[PERF_START_FRAME]);
			for (int j = 1; j < PERF_COUNT; j++)
			{
				line += ", " + WzString::number(perfList[i].counters[j]);
			}
			line += "\n";
			if (WZ_PHYSFS_writeBytes(fileHandle, line.toUtf8().c_str(), line.toUtf8().length()) != line.toUtf8().length())
			{
				// Failed to write line to file
				debug(LOG_ERROR, "wzPerfWriteOut: could not line to %s; PHYSFS error: %s", outfile.toUtf8().c_str(), WZ_PHYSFS_getLastError());
				PHYSFS_close(fileHandle);
				return;
			}
		}
		PHYSFS_close(fileHandle);
	}
	else
	{
		debug(LOG_ERROR, "%s could not be opened: %s", outfile.toUtf8().c_str(), WZ_PHYSFS_getLastError());
		assert(!"wzPerfWriteOut: couldn't open file for writing");
	}
}

void wzPerfShutdown()
{
	if (perfList.empty())
	{
		return;
	}

	// write performance counter list to file
	wzPerfWriteOut(perfList, "gfx-performance.csv");

	// all done, clear data
	gfx_api::context::get().debugPerfStop(); perfStarted = false;
	perfList.clear();
	queryActive = PERF_COUNT;
}

// call after swap buffers
void wzPerfFrame()
{
	if (!perfStarted)
	{
		return; // not started yet
	}
	ASSERT(queryActive == PERF_COUNT, "Missing wfPerfEnd() call");
	PERF_STORE store;
	for (int i = 0; i < PERF_COUNT; i++)
	{
		store.counters[i] = gfx_api::context::get().debugGetPerfValue((PERF_POINT)i);
	}
	perfList.push_back(store);
	gfx_api::context::get().debugPerfStop(); perfStarted = false;

	// Make a screenshot to document sample content
	time_t aclock;
	struct tm *t;

	time(&aclock);           /* Get time in seconds */
	t = localtime(&aclock);  /* Convert time to struct */

	ssprintf(screendump_filename, "screenshots/wz2100-perf-sample-%02d-%04d%02d%02d_%02d%02d%02d.png", perfList.size() - 1,
	         t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	screendump_required = true;
	gfx_api::context::get().debugStringMarker("Performance sample complete");
}

static const char *sceneActive = nullptr;
void wzSceneBegin(const char *descr)
{
	ASSERT(sceneActive == nullptr, "Out of order scenes: Wanted to start %s, was in %s", descr, sceneActive);
	gfx_api::context::get().debugSceneBegin(descr);
	sceneActive = descr;
}

void wzSceneEnd(const char *descr)
{
	ASSERT(descr == nullptr || strcmp(descr, sceneActive) == 0, "Out of order scenes: Wanted to stop %s, was in %s", descr, sceneActive);
	gfx_api::context::get().debugSceneEnd(descr);
	sceneActive = nullptr;
}

void wzPerfBegin(PERF_POINT pp, const char *descr)
{
	ASSERT(queryActive == PERF_COUNT || pp > queryActive, "Out of order timer query call");
	queryActive = pp;
	gfx_api::context::get().debugPerfBegin(pp, descr);
}

void wzPerfEnd(PERF_POINT pp)
{
	ASSERT(queryActive == pp, "Mismatched wzPerfBegin...End");
	queryActive = PERF_COUNT;
	gfx_api::context::get().debugPerfEnd(pp);
}

void screenShutDown()
{
	pie_ShutDown();
	pie_TexShutDown();
	iV_TextShutdown();

	pie_Skybox_Shutdown();

	delete backdropGfx;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/// Display a random backdrop from files in dirname starting with basename.
/// dirname must have a trailing slash.
void screen_SetRandomBackdrop(const char *dirname, const char *basename)
{
	std::vector<std::string> names;  // vector to hold the strings we want
	char **rc = PHYSFS_enumerateFiles(dirname); // all the files in dirname

	// Walk thru the files in our dir, adding the ones that start with basename to our vector of strings
	size_t len = strlen(basename);
	for (char **i = rc; *i != nullptr; i++)
	{
		// does our filename start with basename?
		if (!strncmp(*i, basename, len))
		{
			names.push_back(*i);
		}
	}
	PHYSFS_freeList(rc);

	if (names.empty())
	{
		std::string searchPath = dirname;
		if (!searchPath.empty() && searchPath.back() != '/')
		{
			searchPath += "/";
		}
		debug(LOG_FATAL, "Missing files: \"%s%s*\" - data files / folders may be corrupt.", searchPath.c_str(), basename);
		abort();
		return;
	}

	// pick a random name from our vector of names
	int ran = rand() % names.size();
	std::string full_path = std::string(dirname) + names[ran];

	screen_SetBackDropFromFile(full_path.c_str());
}

void screen_SetBackDropFromFile(const char *filename)
{
	backdropGfx->loadTexture(filename);
	screen_Upload(nullptr);
}

void screen_StopBackDrop()
{
	bBackDrop = false;	//checking [movie]
}

void screen_RestartBackDrop()
{
	bBackDrop = true;
}

bool screen_GetBackDrop()
{
	return bBackDrop;
}

void screen_Display()
{
	// Draw backdrop
	const auto& modelViewProjectionMatrix = glm::ortho(0.f, (float)pie_GetVideoBufferWidth(), (float)pie_GetVideoBufferHeight(), 0.f);
	gfx_api::BackDropPSO::get().bind();
	gfx_api::BackDropPSO::get().bind_constants({ modelViewProjectionMatrix, glm::vec2(0.f), glm::vec2(0.f), glm::vec4(1), 0 });
	backdropGfx->draw<gfx_api::BackDropPSO>(modelViewProjectionMatrix);

	if (mappreview)
	{
		int s1 = screenWidth / preview_width;
		int s2 = screenHeight / preview_height;
		int scale = MIN(s1, s2);
		int w = preview_width * scale;
		int h = preview_height * scale;

		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			int x = player_pos[i].x;
			int y = player_pos[i].y;
			char text[5];

			if (x == 0x77777777)
			{
				continue;
			}

			x = screenWidth / 2 - w / 2 + x * scale;
			y = screenHeight / 2 - h / 2 + y * scale;
			ssprintf(text, "%d", i);
			player_Text[i].setText(text, font_large);
			player_Text[i].render(x - 1, y - 1, WZCOL_BLACK);
			player_Text[i].render(x + 1, y - 1, WZCOL_BLACK);
			player_Text[i].render(x - 1, y + 1, WZCOL_BLACK);
			player_Text[i].render(x + 1, y + 1, WZCOL_BLACK);
			player_Text[i].render(x, y, WZCOL_WHITE);
		}
	}
}

//******************************************************************
//slight hack to display maps (or whatever) in background.
//bitmap MUST be (BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT) for now.
void screen_GenerateCoordinatesAndVBOs()
{
	assert(backdropGfx != nullptr);

	gfx_api::gfxFloat x1 = 0, x2 = screenWidth, y1 = 0, y2 = screenHeight;
	gfx_api::gfxFloat tx = 1, ty = 1;
	int scale = 0, w = 0, h = 0;
	const float aspect = screenWidth / (float)screenHeight, backdropAspect = 4 / (float)3;

	if (aspect < backdropAspect)
	{
		int offset = (screenWidth - screenHeight * backdropAspect) / 2;
		x1 += offset;
		x2 -= offset;
	}
	else
	{
		int offset = (screenHeight - screenWidth / backdropAspect) / 2;
		y1 += offset;
		y2 -= offset;
	}

	if (backdropIsMapPreview) // preview
	{
		int s1 = screenWidth / preview_width;
		int s2 = screenHeight / preview_height;
		scale = MIN(s1, s2);

		w = preview_width * scale;
		h = preview_height * scale;
		x1 = screenWidth / 2 - w / 2;
		x2 = screenWidth / 2 + w / 2;
		y1 = screenHeight / 2 - h / 2;
		y2 = screenHeight / 2 + h / 2;

		tx = preview_width / (float)BACKDROP_HACK_WIDTH;
		ty = preview_height / (float)BACKDROP_HACK_HEIGHT;
	}

	// Generate coordinates and put them into VBOs
	gfx_api::gfxFloat texcoords[8] = { 0.0f, 0.0f,  tx, 0.0,  0.0f, ty,  tx, ty };
	gfx_api::gfxFloat vertices[8] = { x1, y1,  x2, y1,  x1, y2,  x2, y2 };
	backdropGfx->buffers(4, vertices, texcoords);
}

void screen_Upload(const char *newBackDropBmp)
{
	backdropIsMapPreview = false;

	if (newBackDropBmp) // preview
	{
		// Slight hack to display maps previews in background.
		// Bitmap MUST be (BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT) for now.
		backdropGfx->makeTexture(BACKDROP_HACK_WIDTH, BACKDROP_HACK_HEIGHT, gfx_api::pixel_format::FORMAT_RGB8_UNORM_PACK8, newBackDropBmp);
		backdropIsMapPreview = true;
	}

	// Generate coordinates and put them into VBOs
	screen_GenerateCoordinatesAndVBOs();
}

void screen_updateGeometry()
{
	if (backdropGfx != nullptr)
	{
		screen_GenerateCoordinatesAndVBOs();
	}
}

void screen_enableMapPreview(int width, int height, Vector2i *playerpositions)
{
	int i;
	mappreview = true;
	preview_width = width;
	preview_height = height;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		player_pos[i].x = playerpositions[i].x;
		player_pos[i].y = playerpositions[i].y;
	}
}

void screen_disableMapPreview()
{
	mappreview = false;
}

// Screenshot code goes below this
static const unsigned int channelsPerPixel = 3;

class ScreenshotSaveRequest {
public:
	typedef std::function<void (const ScreenshotSaveRequest&)> onDeleteFunc;

	std::string fileName;
	iV_Image image;
	onDeleteFunc onDelete;

	ScreenshotSaveRequest(const std::string & fileName, iV_Image image, const onDeleteFunc & onDelete)
	: fileName(fileName)
	, image(image)
	, onDelete(onDelete)
	{ }

	~ScreenshotSaveRequest()
	{
		onDelete(*this);
	}
};

/** This runs in a separate thread */
static int saveScreenshotThreadFunc(void * saveRequest)
{
	assert(saveRequest != nullptr);
	ScreenshotSaveRequest * pSaveRequest = static_cast<ScreenshotSaveRequest *>(saveRequest);
	
	IMGSaveError pngerror = iV_saveImage_PNG(pSaveRequest->fileName.c_str(), &(pSaveRequest->image));

//	//iV_saveImage_JPEG is *NOT* thread-safe, and cannot be safely called from another thread
//	iV_saveImage_JPEG(fileName, &image);

	if (!pngerror.noError())
	{
		// dispatch logging the error to the main thread
		wzAsyncExecOnMainThread([pngerror]
			{
				debug(LOG_ERROR, "%s\n", pngerror.text.c_str());
			}
		);
	}
	else
	{
		// display message to user about screenshot
		// this must be dispatched back to the main thread
		std::string fileName = pSaveRequest->fileName;
		wzAsyncExecOnMainThread([fileName]
		   {
			   snprintf(ConsoleString, sizeof(ConsoleString), "Screenshot %s saved!", fileName.c_str());
			   addConsoleMessage(ConsoleString, LEFT_JUSTIFY, SYSTEM_MESSAGE);
		   }
		);
	}

	delete pSaveRequest;
	return 0;
}

/** Writes a screenshot of the current frame to file.
 *
 *  Performs the actual work of writing the frame currently displayed on screen
 *  to the filename specified by screenDumpToDisk().
 *
 *  @NOTE This function will only dump a screenshot to file if it was requested
 *        by screenDumpToDisk().
 *
 *  \sa screenDumpToDisk()
 */
void screenDoDumpToDiskIfRequired()
{
	const char *fileName = screendump_filename;
	iV_Image image = { 0, 0, 8, nullptr };

	if (!screendump_required)
	{
		return;
	}
	debug(LOG_3D, "Saving screenshot %s", fileName);

	// IMPORTANT: Must get the size of the viewport directly from the viewport, to account for
	//            high-DPI / display scaling factors (or only a sub-rect of the full viewport
	//            will be captured, as the logical screenWidth/Height may differ from the
	//            underlying viewport pixel dimensions).
	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	image.width = m_viewport[2];
	image.height = m_viewport[3];
	image.bmp = (unsigned char *)malloc(channelsPerPixel * image.width * image.height);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.bmp);

	// Dispatch encoding and saving screenshot to a background thread (since this is fairly costly)
	snprintf(ConsoleString, sizeof(ConsoleString), "Saving screenshot %s ...", fileName);
	addConsoleMessage(ConsoleString, LEFT_JUSTIFY, INFO_MESSAGE);
	ScreenshotSaveRequest * pSaveRequest =
		new ScreenshotSaveRequest(
			fileName,
			image,
			[](const ScreenshotSaveRequest& request)
			{
				if (request.image.bmp)
				{
					free(request.image.bmp);
				}
			}
		);
	WZ_THREAD * pSaveThread = wzThreadCreate(saveScreenshotThreadFunc, pSaveRequest);
	if (pSaveThread == nullptr)
	{
		debug(LOG_ERROR, "Failed to create thread for PNG encoding");
		delete pSaveRequest;
	}
	else
	{
		wzThreadDetach(pSaveThread);
		// the thread handles deleting pSaveRequest
	}

	screendump_required = false;
}

/** Registers the currently displayed frame for making a screen shot.
 *
 *  The filename will be suffixed with a number, such that no files are
 *  overwritten.
 *
 *  \param path The directory path to save the screenshot in.
 */
void screenDumpToDisk(const char *path, const char *level)
{
	unsigned int screendump_num = 0;
	time_t aclock;
	struct tm *t;

	time(&aclock);           /* Get time in seconds */
	t = localtime(&aclock);  /* Convert time to struct */

	ssprintf(screendump_filename, "%swz2100-%04d%02d%02d_%02d%02d%02d-%s.png", path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, level);

	while (PHYSFS_exists(screendump_filename))
	{
		ssprintf(screendump_filename, "%swz2100-%04d%02d%02d_%02d%02d%02d-%s-%d.png", path, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, level, ++screendump_num);
	}
	screendump_required = true;
}
