/*
 * Screen.c
 *
 * Basic double buffered display using direct draw.
 *
 */
 
#ifdef WIN32
/* We need this kludge to avoid a redefinition of INT32 in a jpeglib header */
#define XMD_H
#endif

#include <stdio.h>
#ifndef _MSC_VER		//um.. can't find this in win32?  ..Bah, compiler specific crud.
#include <stdint.h>
#endif
#include <string.h>
#include <SDL/SDL.h>
#ifdef _MSC_VER	
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <GL/gl.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <jpeglib.h>
// the following two lines compromise an ugly hack because some jpeglib.h
// actually contain configure-created defines that conflict with ours!
// man, those jpeglib authors should get a frigging clue...
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#ifdef __cplusplus
}
#endif
#include <physfs.h>
#include <setjmp.h>

#include "frame.h"
#include "frameint.h"
#include "piestate.h"
#include "screen.h"

/* Control Whether the back buffer is in system memory for full screen */
#define FULL_SCREEN_SYSTEM	TRUE

/* The Current screen size and bit depth */
UDWORD		screenWidth = 0;
UDWORD		screenHeight = 0;
UDWORD		screenDepth = 0;

SDL_Surface     *screen;

/* The current screen mode (full screen/windowed) */
SCREEN_MODE		screenMode = SCREEN_WINDOWED;

/* Which mode (of operation) the library is running in */
DISPLAY_MODES	displayMode;

/* The Front and back buffers */
LPDIRECTDRAWSURFACE4	psFront = NULL;
/* The back buffer is not static to give a back door to display routines so
 * they can get at the back buffer directly.
 * Mainly did this to link in Sam's 3D engine.
 */
LPDIRECTDRAWSURFACE4	psBack = NULL;

/* The palette for palettised modes */
//static LPDIRECTDRAWPALETTE	psPalette = NULL;

/* The actual palette entries for the display palette */
#define PAL_MAX				256
static SDL_Color			asPalEntries[PAL_MAX];

/* The bit depth at which it is assumed the mode is palettised */
#define PALETTISED_BITDEPTH   8

/* The number of bits in one colour gun of the windows PALETTEENTRY struct */
#define PALETTEENTRY_BITS 8

/* The Pixel format of the front buffer */
DDPIXELFORMAT		sFrontBufferPixelFormat;

/* The Pixel format of the back buffer */
DDPIXELFORMAT		sBackBufferPixelFormat;

/* Window's Pixel format */
DDPIXELFORMAT		sWinPixelFormat;

/* The size of the windows display mode */
//static UDWORD		winDispWidth, winDispHeight;

// The current flip state
FLIP_STATE	screenFlipState;

//backDrop
#define BACKDROP_WIDTH	640
#define BACKDROP_HEIGHT	480
UWORD*  pBackDropData = NULL;
BOOL    bBackDrop = FALSE;
BOOL    bUpload = FALSE;

//fog
DWORD	fogColour = 0;

static char screendump_filename[255];
static unsigned int screendump_num = 0;
static BOOL screendump_required = FALSE;

/* flag forcing buffers into video memory */
static BOOL	g_bVidMem;

static UDWORD	backDropWidth = BACKDROP_WIDTH;
static UDWORD	backDropHeight = BACKDROP_HEIGHT;
static GLuint backDropTexture = -1;

SDL_Surface *screenGetSDL() {
        return screen;
}

/* Initialise the double buffered display */
BOOL screenInitialise(	UDWORD		width,		// Display width
			UDWORD		height,		// Display height
			UDWORD		bitDepth,	// Display bit depth
			BOOL		fullScreen,	// Whether to start windowed
							// or full screen.
			BOOL		bVidMem,	// Whether to put surfaces in
							// video memory
			BOOL		bDDraw,		// Whether to create ddraw surfaces
			HANDLE		hWindow)	// The main windows handle
{
	/* Store the screen information */
	screenWidth = width;
	screenHeight = height;
	screenDepth = 24;

	/* store vidmem flag */
	g_bVidMem = bVidMem;

	{
		static int video_flags = 0;
		int bpp;

		// Calculate the common flags for windowed and fullscreen modes.
		if (video_flags == 0) {
			const SDL_VideoInfo* video_info;

			// Fetch the video info.
			video_info = SDL_GetVideoInfo( );

			if (!video_info) {
				return FALSE;
			}

			// The flags to pass to SDL_SetVideoMode.
			video_flags  = SDL_OPENGL;          // Enable OpenGL in SDL.
			video_flags |= SDL_ANYFORMAT;       // Don't emulate requested BPP if not available.
			video_flags |= SDL_HWPALETTE;       // Store the palette in hardware.

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

			SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
			SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
			SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

    			// Set the double buffer OpenGL attribute.
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		}

		if (fullScreen) {
			video_flags |= SDL_FULLSCREEN;
			screenMode = SCREEN_FULLSCREEN;
		} else {
			screenMode = SCREEN_WINDOWED;
		}
		
		bpp = SDL_VideoModeOK(width, height, screenDepth, video_flags);
		if (!bpp) {
			printf("Error: Video mode %dx%d@%dbpp is not supported!\n", width, height, screenDepth);
			return FALSE;
		}
		if (bpp != screenDepth) {
			printf("Warning: Using colour depth of %d instead of %d.\n", bpp, screenDepth);
		}
		screen = SDL_SetVideoMode(width, height, bpp, video_flags);
		if (!screen) {
			printf("Error: SDL_SetVideoMode failed (%s).\n", SDL_GetError());
			return FALSE;
		}
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

BOOL screenReInit( void )
{
	BOOL	bFullScreen;

	if ( screenMode == SCREEN_FULLSCREEN )
	{
		bFullScreen = TRUE;
	}
	else
	{
		bFullScreen = FALSE;
	}

	screenShutDown();

	return screenInitialise( screenWidth, screenHeight, screenDepth, bFullScreen,
				 g_bVidMem, TRUE, hWndMain );
}

/* Return a pointer to the Direct Draw 2 object */
LPDIRECTDRAW4 screenGetDDObject(void)
{
	return NULL;
}


/* Return a pointer to the Direct Draw back buffer surface */
LPDIRECTDRAWSURFACE4 screenGetSurface(void)
{
	return psBack;
}

/* Return a pointer to the Front buffer pixel format */
DDPIXELFORMAT *screenGetFrontBufferPixelFormat(void)
{
	return NULL;
}

/* Return a bit depth of the Front buffer */
UDWORD screenGetFrontBufferBitDepth(void)
{
	return screen->format->BitsPerPixel;
}

/* Return a pixel masks of the Front buffer */
BOOL screenGetFrontBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask)
{
	*amask = screen->format->Amask;
	*rmask = screen->format->Rmask;
	*gmask = screen->format->Gmask;
	*bmask = screen->format->Bmask;
	return TRUE;
}

/* Return a pointer to the back buffer pixel format */
DDPIXELFORMAT *screenGetBackBufferPixelFormat(void)
{
	return NULL;
}

/* Return a bit depth of the Back buffer */
UDWORD screenGetBackBufferBitDepth(void)
{
	return screen->format->BitsPerPixel;
}

/* Return a pixel masks of the Back buffer */
BOOL screenGetBackBufferPixelFormatMasks(ULONG *amask, ULONG *rmask, ULONG *gmask, ULONG *bmask)
{
	*amask = screen->format->Amask;
	*rmask = screen->format->Rmask;
	*gmask = screen->format->Gmask;
	*bmask = screen->format->Bmask;
	return TRUE;
}

/*
 * screenRestoreSurfaces
 *
 * Restore the direct draw surfaces if they have been lost.
 *
 * This is only used internally within the library.
 */
void screenRestoreSurfaces(void)
{
	/* nothing to do */
}

void screen_SetBackDrop(UWORD *newBackDropBmp, UDWORD width, UDWORD height)
{
	bBackDrop = TRUE;
	pBackDropData = newBackDropBmp;
	backDropWidth = width;
	backDropHeight = height;
}


/* Image structure */
//typedef struct {
//	unsigned int	width;
//	unsigned int	height;
//	unsigned int	channels;
//	unsigned char*	data;
//} pie_image;

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
	image->data = malloc(width*height*channels);

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

typedef struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
}* my_error_ptr;

void my_error_exit(j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

METHODDEF(void) init_source(j_decompress_ptr cinfo) {}

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
  static JOCTET dummy[] = { (JOCTET) 0xFF, (JOCTET) JPEG_EOI };

  /* Insert a fake EOI marker */
  cinfo->src->next_input_byte = dummy;
  cinfo->src->bytes_in_buffer = 2;

  return TRUE;
}

METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes > 0) {
		while (num_bytes > (long) cinfo->src->bytes_in_buffer) {
			num_bytes -= (long) cinfo->src->bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		cinfo->src->next_input_byte += (size_t) num_bytes;
		cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void) term_source(j_decompress_ptr cinfo) {}

BOOL image_load_from_jpg(pie_image* image, const char* filename)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	int row_stride;
	int image_size;
	uintptr_t tmp;
	JSAMPARRAY ptr[1];
	struct jpeg_source_mgr jsrc;
	char *buffer;
	UDWORD fsize;

	if (image == NULL) return TRUE;

	if (!loadFile(filename, &buffer, &fsize)) {
		debug(LOG_ERROR, "Could not load backdrop file \"%s\"!", filename);
		return TRUE;
	}

	jpeg_create_decompress(&cinfo);
	cinfo.src = &jsrc;
	jsrc.init_source = init_source;
	jsrc.fill_input_buffer = fill_input_buffer;
	jsrc.skip_input_data = skip_input_data;
	jsrc.resync_to_restart = jpeg_resync_to_restart;
	jsrc.term_source = term_source;
	jsrc.bytes_in_buffer = fsize;
	jsrc.next_input_byte = (JOCTET *)buffer;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		debug(LOG_ERROR, "Error during jpg decompression of \"%s\"!", filename);
		FREE(buffer);
		return TRUE;
	}
	jpeg_read_header(&cinfo, TRUE);

	cinfo.out_color_space = JCS_RGB;
	cinfo.quantize_colors = FALSE;
	cinfo.scale_num   = 1;
	cinfo.scale_denom = 1;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.do_fancy_upsampling = FALSE;

	jpeg_calc_output_dimensions(&cinfo);

	row_stride = cinfo.output_width * cinfo.output_components;
	image_size = row_stride * cinfo.output_height;

	image_create(image, cinfo.output_width, cinfo.output_height, cinfo.output_components);

	jpeg_start_decompress(&cinfo);

	tmp = (uintptr_t)image->data;
	while (cinfo.output_scanline < cinfo.output_height) {
		ptr[0] = (JSAMPARRAY)tmp;
		jpeg_read_scanlines(&cinfo, (JSAMPARRAY)ptr, 1);
		tmp += row_stride;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	FREE(buffer);

	return FALSE;
}

//=====================================================================

#if 0
static GLuint image_create_texture(char* filename) {
	pie_image image;
	GLuint texture;

	image_init(&image);

	if (!image_load_from_jpg(&image, filename)) {
		glGenTextures(1, &texture);

		pie_SetTexturePage(-1);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			     image.width, image.height,
			     0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	image_delete(&image);

	return texture;
}
#endif
//=====================================================================

void screen_SetBackDropFromFile(char* filename) {
	pie_image image;

	image_init(&image);

	if (!image_load_from_jpg(&image, filename)) {
		if (backDropTexture == -1) {
			glGenTextures(1, &backDropTexture);
		}

		pie_SetTexturePage(-1);
		glBindTexture(GL_TEXTURE_2D, backDropTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
			     image.width, image.height,
			     0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	image_delete(&image);
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
//bitmap MUST be 512x512 for now.  -Q
void screen_Upload(UWORD* newBackDropBmp) 
{	
//	pie_image image;
//	image_init(&image);
	if(newBackDropBmp!=NULL)
	{
//	imagetest=malloc((sizeof(char)*512*512*512));
//	image_load_from_jpg(&image, "texpages\\bdrops\\test1.jpg");
	glGenTextures(1, &backDropTexture);
	pie_SetTexturePage(-1);
	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		   512,512,//backDropWidth, backDropHeight,
			0, GL_RGB, GL_UNSIGNED_BYTE, newBackDropBmp);//);image.data

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);//GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
/*
//	image_delete(&image);
//	free(image);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	pie_SetTexturePage(-1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glPolygonMode(GL_FRONT,GL_LINE);
	glColor3f(1, 1, 1);
	glPushMatrix();
	glTranslatef(0,0,-13);
//	glBegin(GL_QUADS);
//		glVertex3f(-1.0f, 1.0f, 0.0f);				// Top Left
//		glVertex3f( 1.0f, 1.0f, 0.0f);				// Top Right
//		glVertex3f( 1.0f,-1.0f, 0.0f);				// Bottom Right
//		glVertex3f(-1.0f,-1.0f, 0.0f);				// Bottom Left
//		glEnd();
glBegin(GL_TRIANGLE_FAN);
glVertex3f(10, -12, 0);
glVertex3f(10, 12, 0);
glVertex3f(-10, 12, 0);
glVertex3f(-10, -12, 0);
glEnd();
//	glTexCoord2f(0, 0);
//	glVertex2f(0, 0);
//	glTexCoord2f(512, 0);
//	glVertex2f(screenWidth*2, 0);
//	glTexCoord2f(0, 512);
//	glVertex2f(0, screenHeight*2);
//	glTexCoord2f(512, 512);
//	glVertex2f(screenWidth*2, screenHeight*2);
//	glEnd();
	glPopMatrix();
		SDL_GL_SwapBuffers();
		SDL_GL_SwapBuffers();
*/
//	return;


	}
	
	
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	pie_SetTexturePage(-1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backDropTexture);
	glColor3f(1, 1,1);

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

void screen_SetFogColour(UDWORD newFogColour)
{
	UDWORD red, green, blue;
	static UDWORD currentFogColour = 0;

	if (newFogColour != currentFogColour)
	{

		if (screen->format->BitsPerPixel == 16)//only set in 16 bit modes
		{
			if (screen->format->Gmask == 0x07e0)//565
			{
				red = newFogColour >> 8;
				red &= screen->format->Rmask;
				green = newFogColour >> 5;
				green &= screen->format->Gmask;
				blue = newFogColour >> 3;
				blue &= screen->format->Bmask;
				fogColour = red + green + blue;
			}		
			else if (screen->format->Gmask == 0x03e0)//555
			{
				red = newFogColour >> 9;
				red &= screen->format->Rmask;
				green = newFogColour >> 6;
				green &= screen->format->Gmask;
				blue = newFogColour >> 3;
				blue &= screen->format->Bmask;
				fogColour = red + green + blue;
			}		
		}
		currentFogColour = newFogColour;
	}
	return;
}

/* Swap between windowed and full screen mode */
void screenToggleMode(void)
{
	
	if ((displayMode == MODE_WINDOWED) || (displayMode == MODE_FULLSCREEN))
	{
		/* The framework can only run in the current screen mode */
		return;
	}

	if (SDL_WM_ToggleFullScreen(screen))
	{
		if (screenMode == SCREEN_WINDOWED)
		{
			screenMode = SCREEN_FULLSCREEN;
		}
		else if (screenMode == SCREEN_FULLSCREEN)
		{
			screenMode = SCREEN_WINDOWED;
		}
	}
}

#define BUFFER_SIZE 4096

typedef struct {
	struct jpeg_destination_mgr pub;
	struct PHYSFS_File* file;
	JOCTET * buffer;
} my_jpeg_destination_mgr;

METHODDEF(void) init_destination(j_compress_ptr cinfo) 
{
	my_jpeg_destination_mgr* dm = (my_jpeg_destination_mgr*)(cinfo->dest);

	/* Allocate the output buffer --- it will be released when done with image */
	dm->buffer = (JOCTET *)malloc(BUFFER_SIZE*sizeof(JOCTET));

	dm->pub.next_output_byte = dm->buffer;
	dm->pub.free_in_buffer = BUFFER_SIZE;
}

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo) 
{
	my_jpeg_destination_mgr* dm = (my_jpeg_destination_mgr*)cinfo->dest;

	PHYSFS_write(dm->file, dm->buffer, BUFFER_SIZE, 1);

	dm->pub.next_output_byte = dm->buffer;
	dm->pub.free_in_buffer = BUFFER_SIZE;

  return TRUE;
}

METHODDEF(void) term_destination(j_compress_ptr cinfo) {
	my_jpeg_destination_mgr* dm = (my_jpeg_destination_mgr*)cinfo->dest;

	PHYSFS_write(dm->file, dm->buffer, BUFFER_SIZE-dm->pub.free_in_buffer, 1);

	free(dm->buffer);
}

void screenDoDumpToDiskIfRequired() 
{
	static unsigned char* buffer = NULL;
	static unsigned int buffer_size = 0;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	my_jpeg_destination_mgr jdest;
	JSAMPROW row_pointer[1];
	int row_stride;

	if (!screendump_required) return;

	row_stride = screen->w * 3;

	if (row_stride * screen->h > buffer_size) {
		if (buffer != NULL) {
			free(buffer);
		}
		buffer_size = row_stride * screen->h;
		buffer = malloc(buffer_size);
	}
	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	screendump_required = FALSE;

	if ((jdest.file = PHYSFS_openWrite(screendump_filename)) == NULL) {
		return;
	}

	printf("Saving screenshot %s\n", screendump_filename);

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	cinfo.dest = &jdest;
	jdest.pub.init_destination = init_destination;
	jdest.pub.empty_output_buffer = empty_output_buffer;
	jdest.pub.term_destination = term_destination;

	cinfo.image_width = screen->w; 
	cinfo.image_height = screen->h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 75, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = & buffer[(screen->h-cinfo.next_scanline-1) * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	PHYSFS_close(jdest.file);
	jpeg_destroy_compress(&cinfo);
}


/* Swap between windowed and full screen mode */
BOOL screenToggleVideoPlaybackMode(void)
{
#if 0
	HRESULT				ddrval;
	DDSURFACEDESC2		ddsd;		// Direct Draw surface description
#if !FULL_SCREEN_SYSTEM
	DDSCAPS				ddscaps;
#endif

	if ((displayMode == MODE_WINDOWED) || (displayMode == MODE_FULLSCREEN))
	{
		/* The framework can only run in the current screen mode */
		return TRUE;
	}

	if (screenMode == SCREEN_WINDOWED)
	{
		return TRUE;
	}

	if (screenMode == SCREEN_FULLSCREEN)
	{
		screenDepth = 16;
		screenMode = SCREEN_FULLSCREEN_VIDEO;
		RELEASE(psPalette);
	}
	else if (screenMode == SCREEN_FULLSCREEN_VIDEO)
	{
		screenDepth = 8;
		screenMode = SCREEN_FULLSCREEN;
	}

	RELEASE(psBack);
	RELEASE(psFront);

	/* Set the display mode */
	ddrval = psDD->lpVtbl->SetDisplayMode(
				psDD,
				screenWidth, screenHeight, screenDepth,
				0,0);		// Set these so the DD1 version is used
	if (ddrval != DD_OK)
	{
		DBERROR(("Set display mode failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

#if FULL_SCREEN_SYSTEM
	/* Create the Primary Surface. */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	ddrval = psDD->lpVtbl->CreateSurface(
					psDD,
					&ddsd,
					&psFront,
					NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Primary Surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}

	//copy the palette
	memset(&sFrontBufferPixelFormat, 0, sizeof(DDPIXELFORMAT));
	sFrontBufferPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval = psFront->lpVtbl->GetPixelFormat(
				psFront,
				&sFrontBufferPixelFormat);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't get pixel format:\n%s",
					DDErrorToString(ddrval)));
		return FALSE;
	}

	/* Create the back buffer */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = screenWidth;
	ddsd.dwHeight = screenHeight;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	/* Force the back buffer into system memory unless flag set */
	if ( !g_bVidMem )
	{
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	}
	ddrval = psDD->lpVtbl->CreateSurface(
				psDD,
				&ddsd,
				&psBack,
				NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Back surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
#else
	/* Create the Primary Surface. */
	memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

	ddsd.dwBackBufferCount = 1;	// Use 2 for triple buffering, 1 for double

	ddrval = psDD->lpVtbl->CreateSurface(
					psDD,
					&ddsd,
					&psFront,
					NULL);
	if (ddrval != DD_OK)
	{
		DBERROR(("Create Primary Surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
	
	//copy the palette
	memset(&sFrontBufferPixelFormat, 0, sizeof(DDPIXELFORMAT));
	sFrontBufferPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddrval = psFront->lpVtbl->GetPixelFormat(
				psFront,
				&sFrontBufferPixelFormat);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't get pixel format:\n%s",
					DDErrorToString(ddrval)));
		return FALSE;
	}

	/* Get the back buffer */
	memset(&ddscaps, 0, sizeof(DDSCAPS));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = psFront->lpVtbl->GetAttachedSurface(
				psFront,
				&ddscaps,
				&psBack);
	if (ddrval != DD_OK)
	{
		DBERROR(("Get Back surface failed:\n%s", DDErrorToString(ddrval)));
		return FALSE;
	}
#endif
	/* If we are in a palettised mode, create a palette */
	if (screenDepth == PALETTISED_BITDEPTH)
	{
		/* Create the palette from the stored palette entries */
		ddrval = psDD->lpVtbl->CreatePalette(
						psDD,
						DDPCAPS_8BIT,
						asPalEntries,
						&psPalette,
						NULL);
		if (ddrval != DD_OK)
		{
			DBERROR(("Failed to create palette:\n%s", DDErrorToString(ddrval)));
			return FALSE;
		}

		/* Assign the palette to the front buffer */
		ddrval = psFront->lpVtbl->SetPalette(psFront, psPalette);
		if (ddrval != DD_OK)
		{
			DBERROR(("Couldn't set palette for front buffer:\n%s",
						DDErrorToString(ddrval)));
		}

		/* Assign the palette to the back buffer */
		ddrval = psFront->lpVtbl->SetPalette(psBack, psPalette);
		if (ddrval != DD_OK)
		{
			DBERROR(("Couldn't set palette for back buffer:\n%s",
						DDErrorToString(ddrval)));
		}
	}

#endif
	return TRUE;
}

SCREEN_MODE screenGetMode( void )
{
	return screenMode;
}

/* Set screen mode */
void screenSetMode(SCREEN_MODE mode)
{
	/* If the mode is the same as the current one, don't have to do
	 * anything.  Otherwise toggle the mode.
	 */
	if (mode != screenMode)
	{
		screenToggleMode();
	}
}

/* In full screen mode flip to the GDI buffer.
 * Use this if you want the user to see any GDI output.
 * This is mainly used so that ASSERTs and message boxes appear
 * even in full screen mode.
 */
void screenFlipToGDI(void)
{
	/* do nothing */
}

/* Set palette entries for the display buffer
 * first specifies the first palette entry. count the number of entries
 * The psPalette should have at least first + count entries in it.
 */
void screenSetPalette(UDWORD first, UDWORD count, PALETTEENTRY *psEntries)
{
	UDWORD	i;

	ASSERT(((first+count-1 < PAL_MAX),
		"screenSetPalette: invalid entry range"));

	if (count == 0)
	{
		return;
	}

	/* ensure that colour 0 is black and 255 is white */
	if ((first == 0 || first == 255) && count == 1)
	{
		return;
	}

	if (first == 0)
	{
		first = 1;
		count -= 1;
	}
	if (first + count - 1 == PAL_MAX)
	{
		count -= 1;
	}

        for (i = 0; i < count; i++)
        {
                asPalEntries[first + i].r = psEntries[first + i].peRed;
                asPalEntries[first + i].g = psEntries[first + i].peGreen;
                asPalEntries[first + i].b = psEntries[first + i].peBlue;
        }

        SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, asPalEntries + first, first, count);
}

/* Return the best colour match when in a palettised mode */
UBYTE screenGetPalEntry(UBYTE red, UBYTE green, UBYTE blue)
{
	UDWORD	i, minDist, dist;
	UDWORD	redDiff,greenDiff,blueDiff;
	UBYTE	colour;

	ASSERT((screen->format->BitsPerPixel == 8,
		"screenSetPalette: not in a palettised mode"));

	minDist = 0xff*0xff*0xff;
	colour = 0;
	for(i = 0; i < PAL_MAX; i++)
	{
		redDiff = asPalEntries[i].r - red;
		greenDiff = asPalEntries[i].g - green;
		blueDiff = asPalEntries[i].b - blue;
		dist = redDiff*redDiff + greenDiff*greenDiff + blueDiff*blueDiff;
		if (dist < minDist)
		{
			minDist = dist;
			colour = (UBYTE)i;
		}
		if (minDist == 0)
		{
			break;
		}
	}

	return colour;
}

char* screenDumpToDisk(char* path) {
	while (1) {
		sprintf(screendump_filename, "%swz2100_shot_%03i.jpg", path, ++screendump_num);
		if (!PHYSFS_exists(screendump_filename)) {
			break;
		}
	}

	screendump_required = TRUE;

	return screendump_filename;
}

/* Output text to the display screen at location x,y.
 * The remaining arguments are as printf.
 */
void screenTextOut(UDWORD x, UDWORD y, STRING *pFormat, ...)
{
}

