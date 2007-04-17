/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 * Screen.c
 *
 * Basic double buffered display using direct draw.
 *
 */

#include "lib/framework/frame.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <physfs.h>
#include <png.h>

#if !defined(__cplusplus) && !defined(CHAR_BIT)
// A char is _always_ 1 byte large,
// assume bytes are 8 bits large
# define CHAR_BIT 8
#endif

#include "lib/framework/frameint.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "screen.h"

/* The Current screen size and bit depth */
UDWORD		screenWidth = 0;
UDWORD		screenHeight = 0;
UDWORD		screenDepth = 0;

SDL_Surface     *screen;

/* global used to indicate preferred internal OpenGL format */
int wz_texture_compression;

//backDrop
UWORD*  pBackDropData = NULL;
BOOL    bBackDrop = FALSE;
BOOL    bUpload = FALSE;

//fog
SDWORD	fogColour = 0;

static char screendump_filename[MAX_PATH];
static BOOL screendump_required = FALSE;

static UDWORD	backDropWidth = BACKDROP_WIDTH;
static UDWORD	backDropHeight = BACKDROP_HEIGHT;
static GLuint backDropTexture = ~0;

/* Initialise the double buffered display */
BOOL screenInitialise(
			UDWORD		width,		// Display width
			UDWORD		height,		// Display height
			UDWORD		bitDepth,	// Display bit depth
			BOOL		fullScreen	// Whether to start windowed
							// or full screen
			)
{
	static int video_flags = 0;
	int bpp = 0, value;
	GLint glval;

	/* Store the screen information */
	screenWidth = width;
	screenHeight = height;
	screenDepth = bitDepth;

	// Calculate the common flags for windowed and fullscreen modes.
	if (video_flags == 0) {
		// Fetch the video info.
		const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

		if (!video_info) {
			return FALSE;
		}

		// The flags to pass to SDL_SetVideoMode.
		video_flags  = SDL_OPENGL;    // Enable OpenGL in SDL.
		video_flags |= SDL_ANYFORMAT; // Don't emulate requested BPP if not available.
		video_flags |= SDL_HWPALETTE; // Store the palette in hardware.

		// This checks to see if surfaces can be stored in memory.
		if (video_info->hw_available) {
			video_flags |= SDL_HWSURFACE;
		} else {
			video_flags |= SDL_SWSURFACE;
		}

		// This checks if hardware blits can be done.
		if (video_info->blit_hw) {
			video_flags |= SDL_HWACCEL;
		}

		if (fullScreen) {
			video_flags |= SDL_FULLSCREEN;
		}

		// Set the double buffer OpenGL attribute.
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		bpp = SDL_VideoModeOK(width, height, bitDepth, video_flags);
		if (!bpp) {
			debug( LOG_ERROR, "Error: Video mode %dx%d@%dbpp is not supported!\n", width, height, bitDepth );
			return FALSE;
		}
		switch ( bpp )
		{
			case 32:
			case 24:
				SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
				SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
				SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
				break;
			case 16:
				debug( LOG_ERROR, "Warning: Using colour depth of %i instead of %i.", bpp, screenDepth );
				debug( LOG_ERROR, "         You will experience graphics glitches!" );
				SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
				SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
				SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
				SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
				SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
				break;
			case 8:
				debug( LOG_ERROR, "Error: You don't want to play Warzone with a bit depth of %i, do you?", bpp );
				exit( 1 );
				break;
			default:
				debug( LOG_ERROR, "Error: Unsupported bit depth: %i", bpp );
				exit( 1 );
				break;
		}
	}

	screen = SDL_SetVideoMode(width, height, bpp, video_flags);
	if ( !screen ) {
		debug( LOG_ERROR, "Error: SDL_SetVideoMode failed (%s).", SDL_GetError() );
		return FALSE;
	}
	if ( SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1)
	{
		debug( LOG_ERROR, "OpenGL initialization did not give double buffering!" );
	}
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glval);
	debug( LOG_TEXTURE, "Maximum texture size: %dx%d", (int)glval, (int)glval );
	if ( glval < 512 ) // FIXME: Replace by a define that gives us the real maximum
	{
		debug( LOG_ERROR, "OpenGL reports a texture size (%d) that is less than required!", (int)glval );
		debug( LOG_ERROR, "This is either a bug in OpenGL or your graphics card is really old!" );
		debug( LOG_ERROR, "Trying to run the game anyway..." );
	}

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, height, 0, 1, -1);
	glMatrixMode(GL_TEXTURE);
	glScalef(1/256.0, 1/256.0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	return TRUE;
}


/* Release the DD objects */
void screenShutDown(void)
{
	if (screen != NULL)
	{
		SDL_FreeSurface(screen);
	}
}

/* Return a pointer to the back buffer surface */
void *screenGetSurface(void)
{
	return NULL;
}

void screen_SetBackDrop(UWORD *newBackDropBmp, UDWORD width, UDWORD height)
{
	bBackDrop = TRUE;
	pBackDropData = newBackDropBmp;
	backDropWidth = width;
	backDropHeight = height;
}

BOOL image_init(pie_image* image)
{
	if (image == NULL) return TRUE;

	image->width = 0;
	image->height = 0;
	image->channels = 0;
	image->data = NULL;

	return FALSE;
}

BOOL image_create(pie_image* image,
		  unsigned int width,
		  unsigned int height,
		  unsigned int channels)
{
	if (image == NULL) return TRUE;

	image->width = width;
	image->height = height;
	image->channels = channels;
	if (image->data != NULL) {
		free(image->data);
	}
	image->data = (unsigned char*)malloc(width*height*channels);

	return FALSE;
}

BOOL image_delete(pie_image* image)
{
	if (image == NULL) return TRUE;

	if (image->data != NULL) {
		free(image->data);
		image->data = NULL;
	}

	return FALSE;
}

void screen_SetBackDropFromFile(const char* filename)
{
	// HACK : We should use a resource handler here!
	const char *extension = strrchr(filename, '.');// determine the filetype

	if(!extension)
	{
		debug(LOG_ERROR, "Image without extension: \"%s\"!", filename);
		return; // filename without extension... don't bother
	}

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(-1);

	if( strcmp(extension,".png") == 0 )
	{
		iTexture imagePNG;

		if (pie_PNGLoadFile( filename, &imagePNG ) )
		{
			if (~backDropTexture == 0)
				glGenTextures(1, &backDropTexture);

			glBindTexture(GL_TEXTURE_2D, backDropTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					imagePNG.width, imagePNG.height,
					0, GL_RGBA, GL_UNSIGNED_BYTE, imagePNG.bmp);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

			free(imagePNG.bmp);
		}
		return;
	}
	else
		debug(LOG_ERROR, "Unknown extension \"%s\" for image \"%s\"!", extension, filename);
}
//===================================================================

void screen_StopBackDrop(void)
{
	bBackDrop = FALSE;	//checking [movie]
}

void screen_RestartBackDrop(void)
{
	bBackDrop = TRUE;
}

BOOL screen_GetBackDrop(void)
{
	return bBackDrop;
}
//******************************************************************
//slight hack to display maps (or whatever) in background.
//bitmap MUST be (BACKDROP_HACK_WIDTH * BACKDROP_HACK_HEIGHT) for now.
void screen_Upload(const char *newBackDropBmp)
{
	if(newBackDropBmp != NULL)
	{
		glGenTextures(1, &backDropTexture);
		pie_SetTexturePage(-1);
		glBindTexture(GL_TEXTURE_2D, backDropTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			BACKDROP_HACK_WIDTH, BACKDROP_HACK_HEIGHT,
			0, GL_RGB, GL_UNSIGNED_BYTE, newBackDropBmp);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Make sure the current texture page is reloaded after we are finished
	// Otherwise WZ will think it is still loaded and not load it again
	pie_SetTexturePage(-1);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glColor3f(1, 1, 1);

	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(255, 0);
		glVertex2f(screenWidth, 0);
		glTexCoord2f(0, 255);
		glVertex2f(0, screenHeight);
		glTexCoord2f(255, 255);
		glVertex2f(screenWidth, screenHeight);
	glEnd();
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{
	(void) SDL_WM_ToggleFullScreen(screen);
}

// Screenshot code goes below this
static const unsigned int channelBitdepth = 8;
static const unsigned int channelsPerPixel = 3;

// PNG callbacks
static void wzpng_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PHYSFS_file* fileHandle = (PHYSFS_file*)png_get_io_ptr(png_ptr);

	PHYSFS_write(fileHandle, data, length, 1);
}

static void wzpng_flush_data(png_structp png_ptr)
{
	PHYSFS_file* fileHandle = (PHYSFS_file*)png_get_io_ptr(png_ptr);
	
	PHYSFS_flush(fileHandle);
}

// End of PNG callbacks

static inline void PNGCleanup(png_infop *info_ptr, png_structp *png_ptr)
{
	if (*info_ptr != NULL)
		png_destroy_info_struct(*png_ptr, info_ptr);
	if (*png_ptr != NULL)
		png_destroy_read_struct(png_ptr, NULL, NULL);
}

static inline void screen_DumpPNG(PHYSFS_file* fileHandle, const unsigned char* inputBuffer, unsigned int width, unsigned int height, unsigned int channels)
{
	png_infop info_ptr = NULL;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		debug(LOG_ERROR, "screen_DumpPNG: Unable to create png struct\n");
		return PNGCleanup(&info_ptr, &png_ptr);
	}

	info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL)
	{
		debug(LOG_ERROR, "screen_DumpPNG: Unable to create png info struct\n");
		return PNGCleanup(&info_ptr, &png_ptr);
	}

	// If libpng encounters an error, it will jump into this if-branch
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug(LOG_ERROR, "screen_DumpPNG: Error encoding PNG data\n");
	}
	else
	{
		unsigned int currentRow;
		unsigned int row_stride = width * channels;
		const unsigned char** scanlines = malloc(sizeof(const unsigned char*) * height);

		if (scanlines == NULL)
		{
			debug(LOG_ERROR, "screen_DumpPNG: Couldn't allocate memory\n");
			return PNGCleanup(&info_ptr, &png_ptr);
		}

		png_set_write_fn(png_ptr, fileHandle, wzpng_write_data, wzpng_flush_data);

		// Set the compression level of ZLIB
		// Right now we stick with the default, since that one is the
		// fastest which still produces acceptable filesizes.
		// The highest compression level hardly produces smaller files than default.
		//
		// Below are some benchmarks done while taking screenshots at 1280x1024
		// Z_NO_COMPRESSION:
		// black (except for GUI): 398 msec
		// 381, 391, 404, 360 msec
		// 
		// Z_BEST_SPEED:
		// black (except for GUI): 325 msec
		// 611, 406, 461, 608 msec
		// 
		// Z_DEFAULT_COMPRESSION:
		// black (except for GUI): 374 msec
		// 1154, 1121, 627, 790 msec
		// 
		// Z_BEST_COMPRESSION:
		// black (except for GUI): 439 msec
		// 1600, 1078, 1613, 1700 msec

		// Not calling this function is equal to using the default
		// so to spare some CPU cycles we comment this out.
		// png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);

		png_set_IHDR(png_ptr, info_ptr, width, height, channelBitdepth,
			             PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// Create an array of scanlines
		for (currentRow = 0; currentRow < height; ++currentRow)
		{
			// We're filling the scanline from the bottom up here,
			// otherwise we'd have a vertically mirrored image.
			scanlines[currentRow] = &inputBuffer[row_stride * (height - currentRow - 1)];
		}

		png_set_rows(png_ptr, info_ptr, (const png_bytepp)scanlines);

		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	}

	return PNGCleanup(&info_ptr, &png_ptr);
}

/** Retrieves the currently displayed screen and throws it in a buffer
 *  \param width the screen's width
 *  \param height the screen's height
 *  \param the number of channels per pixel (since we're using RGB, 3 is a sane default)
 *  \return a pointer to a buffer holding all pixels of the image
 */
static const unsigned char* screen_DumpInBuffer(unsigned int width, unsigned int height, unsigned int channels)
{
	static unsigned char* buffer = NULL;
	static unsigned int buffer_size = 0;

	unsigned int row_stride = width * channels;

	if (row_stride * height > buffer_size) {
		if (buffer != NULL) {
			free(buffer);
		}
		buffer_size = row_stride * height;
		buffer = (unsigned char*)malloc(buffer_size);
	}
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	return buffer;
}

void screenDoDumpToDiskIfRequired(void)
{
	const char* fileName = screendump_filename;
	const unsigned char* inputBuffer = NULL;
	PHYSFS_file* fileHandle;

	if (!screendump_required) return;

	fileHandle = PHYSFS_openWrite(fileName);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "screenDoDumpToDiskIfRequired: PHYSFS_openWrite failed (while openening file %s) with error: %s\n", fileName, PHYSFS_getLastError());
		return;
	}
	debug( LOG_3D, "Saving screenshot %s\n", fileName );

	// Dump the currently displayed screen in a buffer
	inputBuffer = screen_DumpInBuffer(screen->w, screen->h, channelsPerPixel);

	// Write the screen to a PNG
	screen_DumpPNG(fileHandle, inputBuffer, screen->w, screen->h, channelsPerPixel);

	screendump_required = FALSE;
	PHYSFS_close(fileHandle);
}

void screenDumpToDisk(const char* path) {
	static unsigned int screendump_num = 0;

	while (++screendump_num != 0) {
		// We can safely use '/' as path separator here since PHYSFS uses that as its default separator
		snprintf(screendump_filename, MAX_PATH, "%s/wz2100_shot_%03i.png", path, screendump_num);
		if (!PHYSFS_exists(screendump_filename)) {
			// Found a usable filename, so we'll stop searching.
			break;
		}
	}

	ASSERT( screendump_num != 0, "screenDumpToDisk: integer overflow; no more filenumbers available.\n" );

	// If we have an integer overflow, we don't want to go about and overwrite files
	if (screendump_num != 0)
		screendump_required = TRUE;
}
