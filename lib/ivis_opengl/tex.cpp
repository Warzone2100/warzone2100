/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/framework/opengl.h"

#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/png_util.h"

#include <QtCore/QList>
#include "screen.h"

//*************************************************************************

struct iTexPage
{
	char name[iV_TEXNAME_MAX];
	GLuint id;
};

QList<iTexPage> _TEX_PAGE;

static void pie_PrintLoadedTextures(void);

//*************************************************************************

GLuint pie_Texture(int page)
{
	return _TEX_PAGE[page].id;
}

int pie_NumberOfPages()
{
	return _TEX_PAGE.size();
}

// Add a new texture page to the list
int pie_ReserveTexture(const char *name)
{
	iTexPage tex;
	glGenTextures(1, &tex.id);
	sstrcpy(tex.name, name);
	_TEX_PAGE.append(tex);
	return _TEX_PAGE.size() - 1;
}

/**************************************************************************
	Add an image buffer given in s as a new texture page in the texture
	table.  We check first if the given image has already been loaded,
	as a sanity check (should never happen).  The texture numbers are
	stored in a special texture table, not in the resource system, for
	some unknown reason.

	Returns the texture number of the image.
**************************************************************************/
int pie_AddTexPage(iV_Image *s, const char* filename, int maxTextureSize, bool useMipmaping)
{
	unsigned int i = 0;
	int width, height;
	void *bmp;
	bool scaleDown = false;
	GLint minfilter;
	iTexPage tex;
	int page = -1;

	ASSERT(s && filename, "Bad input parameter");

	/* Have we already loaded this one? Should not happen here. */
	while (i < _TEX_PAGE.size())
	{
		if (strncmp(filename, _TEX_PAGE[i].name, iV_TEXNAME_MAX) == 0)
		{
			pie_PrintLoadedTextures();
		}
		ASSERT(strncmp(filename, _TEX_PAGE[i].name, iV_TEXNAME_MAX) != 0,
		       "%s loaded again! Already loaded as %s|%u", filename,
		       _TEX_PAGE[i].name, i);
		i++;
	}

	page = _TEX_PAGE.size();
	debug(LOG_TEXTURE, "%s page=%d", filename, page);

	/* Stick the name into the tex page structures */
	sstrcpy(tex.name, filename);

	glGenTextures(1, &tex.id);

	_TEX_PAGE.append(tex);

	pie_SetTexturePage(page);

	width = s->width;
	height = s->height;
	bmp = s->bmp;
	if ((width & (width-1)) == 0 && (height & (height-1)) == 0)
	{
		if (maxTextureSize > 0 && width > maxTextureSize)
		{
			width = maxTextureSize;
			scaleDown = true;
		}
		if (maxTextureSize > 0 && height > maxTextureSize)
		{
			height = maxTextureSize;
			scaleDown = true;
		}
		if (scaleDown)
		{
			debug(LOG_TEXTURE, "scaling down texture %s from %ix%i to %ix%i", filename, s->width, s->height, width, height);
			bmp = malloc(4 * width * height); // FIXME: don't know for sure it is 4 bytes per pixel
			gluScaleImage(iV_getPixelFormat(s), s->width, s->height, GL_UNSIGNED_BYTE, s->bmp,
			                                    width,    height,    GL_UNSIGNED_BYTE, bmp);
			free(s->bmp);
		}

		if (maxTextureSize)
		{
			// this is a 3D texture, use texture compression
			gluBuild2DMipmaps(GL_TEXTURE_2D, wz_texture_compression, width, height, iV_getPixelFormat(s), GL_UNSIGNED_BYTE, bmp);
		}
		else
		{
			// this is an interface texture, do not use compression
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, iV_getPixelFormat(s), GL_UNSIGNED_BYTE, bmp);
		}
	}
	else
	{
		debug(LOG_ERROR, "Non-POT texture %s", filename);
	}
	
	// it is uploaded, we do not need it anymore
	free(bmp); 
	s->bmp = NULL;

	if (useMipmaping)
	{
		minfilter = GL_LINEAR_MIPMAP_LINEAR;
	}
	else
	{
		minfilter = GL_LINEAR;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Use anisotropic filtering, if available, but only max 4.0 to reduce processor burden
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		GLfloat max;

		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	/* Send back the texpage number so we can store it in the IMD */
	return page;
}

/*!
 * Turns filename into a pagename if possible
 * \param[in,out] filename Filename to pagify
 */
void pie_MakeTexPageName(char * filename)
{
	char *c = strstr(filename, iV_TEXNAME_TCSUFFIX);
	if (c)
	{
		*(c + 7) = '\0';
		return;
	}
	c = strchr(filename + 5, '-');
	if (c)
	{
		*c = '\0';
	}
}

/*!
 * Turns page filename into a pagename + tc mask if possible
 * \param[in,out] filename Filename to pagify
 */
void pie_MakeTexPageTCMaskName(char * filename)
{
	if (strncmp(filename, "page-", 5) == 0)
	{
		int i;
		for ( i = 5; i < iV_TEXNAME_MAX-1 && isdigit(filename[i]); i++) {}
		filename[i] = '\0';
		strcat(filename, iV_TEXNAME_TCSUFFIX);
	}
}

/*!
 * Print the names of all loaded textures to LOG_ERROR
 */
static void pie_PrintLoadedTextures(void)
{
	debug(LOG_ERROR, "Texture pages in memory: %u", _TEX_PAGE.size());
	for (int i = 0; i < _TEX_PAGE.size() && _TEX_PAGE[i].name[0] != '\0'; i++ )
	{
		debug(LOG_ERROR, "%02d : %s", i, _TEX_PAGE[i].name);
	}
}

/** Retrieve the texture number for a given texture resource.
 *
 *  @note We keep textures in a separate data structure _TEX_PAGE apart from the
 *        normal resource system.
 *
 *  @param filename The filename of the texture page to search for.
 *
 *  @return a non-negative index number for the texture, negative if no texture
 *          with the given filename could be found
 */
int iV_GetTexture(const char *filename)
{
	unsigned int i = 0;
	iV_Image sSprite;
	char path[PATH_MAX];

	/* Have we already loaded this one then? */
	sstrcpy(path, filename);
	pie_MakeTexPageName(path);
	for (i = 0; i < _TEX_PAGE.size(); i++)
	{
		if (strncmp(path, _TEX_PAGE[i].name, iV_TEXNAME_MAX) == 0)
		{
			return i;
		}
	}

	// Try to load it
	sstrcpy(path, "texpages/");
	sstrcat(path, filename);
	if (!iV_loadImage_PNG(path, &sSprite))
	{
		debug(LOG_ERROR, "Failed to load %s", path);
		return -1;
	}
	sstrcpy(path, filename);
	pie_MakeTexPageName(path);
	return pie_AddTexPage(&sSprite, path, -1, true);	// FIXME, -1, use getTextureSize()
}


/**************************************************************************
	WRF files may specify overrides for the textures on a map. This
	is done through an ugly hack involving cutting the texture name
	down to just "page-NN", where NN is the page number, and
	replaceing the texture page with the same name if another file
	with this prefix is loaded.
**************************************************************************/
int pie_ReplaceTexPage(iV_Image *s, const char *texPage, int maxTextureSize, bool useMipmaping)
{
	int i = iV_GetTexture(texPage);

	ASSERT(i >= 0, "pie_ReplaceTexPage: Cannot find any %s to replace!", texPage);
	if (i < 0)
	{
		return -1;
	}

	glDeleteTextures(1, &_TEX_PAGE[i].id);
	debug(LOG_TEXTURE, "Reloading texture %s from index %d", texPage, i);
	_TEX_PAGE[i].name[0] = '\0';
	pie_AddTexPage(s, texPage, maxTextureSize, useMipmaping);

	return i;
}

void pie_TexShutDown(void)
{
	// TODO, lazy deletions for faster loading of next level
	debug(LOG_TEXTURE, "Cleaning out %u textures", _TEX_PAGE.size());
	int _TEX_INDEX = _TEX_PAGE.size() - 1;
	while (_TEX_INDEX > 0)
	{
		glDeleteTextures(1, &_TEX_PAGE[_TEX_INDEX--].id);
	}
	_TEX_PAGE.clear();
}

void pie_TexInit(void)
{
	debug(LOG_TEXTURE, "pie_TexInit successful");
}


void iV_unloadImage(iV_Image *image)
{
	if (image)
	{
		if (image->bmp)
		{
			free(image->bmp);
			image->bmp = NULL;
		}
	}
	else
	{
		debug(LOG_ERROR, "Tried to free invalid image!");
	}
}


unsigned int iV_getPixelFormat(const iV_Image *image)
{
	switch (image->depth)
	{
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			debug(LOG_ERROR, "iV_getPixelFormat: Unsupported image depth: %u", image->depth);
			return GL_INVALID_ENUM;
	}
}
