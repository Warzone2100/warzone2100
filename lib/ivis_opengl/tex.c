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
#include "lib/framework/frame.h"

#include <SDL/SDL_opengl.h>
#ifdef __APPLE__
#include <opengl/glu.h>
#else
#include <GL/glu.h>
#endif

#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/ivispatch.h"

#include "screen.h"

//*************************************************************************

iTexPage _TEX_PAGE[iV_TEX_MAX];
int _TEX_INDEX;

//*************************************************************************


/**************************************************************************
	Add an image buffer given in s as a new texture page in the texture
	table.  We check first if the given image has already been loaded,
	as a sanity check (should never happen).  The texture numbers are
	stored in a special texture table, not in the resource system, for
	some unknown reason.

	Returns the texture number of the image.
**************************************************************************/
int pie_AddBMPtoTexPages(iTexture* s, const char* filename, int type, BOOL bResource)
{
	int	i = 0;

	debug(LOG_TEXTURE, "pie_AddBMPtoTexPages: %s type=%d resource=%s page=%d", filename,
	      type, bResource ? "true" : "false", _TEX_INDEX);
	assert(s != NULL);

	/* Have we already loaded this one? (Should generally not happen here.) */
	while (i < _TEX_INDEX) {
		if (strcasecmp(filename, _TEX_PAGE[i].name) == 0) {
			// this happens with terrain for some reason, which is necessary
			debug(LOG_TEXTURE, "pie_AddBMPtoTexPages: %s loaded again", filename);
		}
		i++;
	}

	/* Get next available texture page */
	i = _TEX_INDEX;

	/* Have we used up too many? */
	if (_TEX_INDEX >= iV_TEX_MAX) {
		debug(LOG_ERROR, "pie_AddBMPtoTexPages: too many texture pages");
		assert(FALSE);
		return -1;
	}

	/* Stick the name into the tex page structures */
	strcpy(_TEX_PAGE[i].name, filename);

	/* Store away all the info */
	/* DID come from a resource */
	_TEX_PAGE[i].bResource = bResource;
	_TEX_PAGE[i].tex.bmp = s->bmp;
	_TEX_PAGE[i].tex.width = s->width;
	_TEX_PAGE[i].tex.height = s->height;
	_TEX_PAGE[i].type = type;

	glGenTextures(1, &_TEX_PAGE[i].textPage3dfx);
	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[i].textPage3dfx);

	if ((s->width & (s->width-1)) == 0 && (s->height & (s->height-1)) == 0)
	{
		gluBuild2DMipmaps(GL_TEXTURE_2D, wz_texture_compression, s->width, s->height,
			     GL_RGBA, GL_UNSIGNED_BYTE, s->bmp);
	} else {
		debug(LOG_ERROR, "pie_AddBMPtoTexPages: non POT texture %s.", filename);
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if( i == SKY_TEXPAGE )
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	/* Send back the texpage number so we can store it in the IMD */

	_TEX_INDEX++;

	return i;
}

void pie_ChangeTexPage(int tex_index, iTexture* s, int type, BOOL bResource)
{
	assert(s != NULL);

	/* DID come from a resource */
	_TEX_PAGE[tex_index].bResource = bResource;
	// Default values
	_TEX_PAGE[tex_index].tex.bmp = s->bmp;
	_TEX_PAGE[tex_index].tex.width = s->width;
	_TEX_PAGE[tex_index].tex.height = s->height;
	_TEX_PAGE[tex_index].type = type;

	glBindTexture(GL_TEXTURE_2D, _TEX_PAGE[tex_index].textPage3dfx);

	if ((s->width & (s->width-1)) == 0 && (s->height & (s->height-1)) == 0)
	{
		gluBuild2DMipmaps(GL_TEXTURE_2D, wz_texture_compression, s->width, s->height,
			     GL_RGBA, GL_UNSIGNED_BYTE, s->bmp);
	} else {
		debug(LOG_ERROR, "pie_ChangeTexPage: non POT texture %i", tex_index);
	}

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

/**************************************************************************
	Return the texture number for a given texture resource.  We keep
	textures in a separate data structure _TEX_PAGE apart from the
	normal resource system.
**************************************************************************/
int iV_GetTexture(char *filename)
{
	int i  = 0;

	/* Have we already loaded this one then? */
	while (i < iV_TEX_MAX) {
		if (strcasecmp(filename, _TEX_PAGE[i].name) == 0) {
			return i;
		}
		i++;
	}

	/* This should never happen - by now all textures should have been loaded. */
	debug(LOG_ERROR, "*** texture %s not loaded! ***", filename);
	debug(LOG_ERROR, "Available texture pages in memory (%d out of %d max):", _TEX_INDEX, iV_TEX_MAX);
	for (i = 0; i < iV_TEX_MAX; i++) {
		debug(LOG_ERROR, "   %02d : %s", i, _TEX_PAGE[i].name);
	}
	debug(LOG_ERROR, "This error probably means you did not specify for this texture");
	debug(LOG_ERROR, "to be preloaded in the appropriate wrf files before referencing");
	debug(LOG_ERROR, "it in some pie file.  Remember that patches override several");
	debug(LOG_ERROR, "standard wrf files as well.");

	return -1;
}

int pie_ReloadTexPage(char *filename, char *pBuffer)
{
	int i = iV_GetTexture(filename);

	if (i == -1)
	{
		return -1;
	}
	debug(LOG_TEXTURE, "Reloading texture %s from index %d, max is %d", filename, i, _TEX_INDEX);

	if (_TEX_PAGE[i].tex.bmp)
	{
		free(_TEX_PAGE[i].tex.bmp);
	}
	pie_PNGLoadMem(pBuffer, &_TEX_PAGE[i].tex);

	return i;
}

/*
	Alex - fixed this so it doesn't try to free up the memory if it got the page from resource
	handler - this is because the resource handler will deal with freeing it, and in all probability
	will have already done so by the time this is called, thus avoiding an 'already freed' moan.
*/
void pie_TexShutDown(void)
{
	int i = 0, j = 0;

	while (i < _TEX_INDEX) {
		/*	Only free up the ones that were NOT allocated through resource handler cos they'll already
			be free */
		if(_TEX_PAGE[i].bResource == FALSE)
		{
			if(_TEX_PAGE[i].tex.bmp) {
				j++;
				free(_TEX_PAGE[i].tex.bmp);
			}
		}
		i++;
	}

	debug(LOG_TEXTURE, "pie_TexShutDown successful - did free %d texture pages\n", j);
}

void pie_TexInit(void)
{
	int i = 0;

	while (i < iV_TEX_MAX) {
		_TEX_PAGE[i].name[0] = '\0';
		_TEX_PAGE[i].tex.bmp = NULL;
		_TEX_PAGE[i].tex.width = 0;
		_TEX_PAGE[i].tex.height = 0;
		i++;
	}
	debug(LOG_TEXTURE, "pie_TexInit successful - initialized %d texture pages\n", i);
}
