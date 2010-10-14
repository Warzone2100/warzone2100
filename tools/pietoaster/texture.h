/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#ifndef _tex_h
#define _tex_h

#include "wzglobal.h"
#include "pie_types.h"

//*************************************************************************

#define iV_TEX_MAX 64
#define iV_TEX_INVALID -1
#define iV_TEXNAME_MAX 64

#define SKY_TEXPAGE "page-25"

//*************************************************************************

#define iV_TEXNAME(i)	((char *) (&_TEX_PAGE[(i)].name))

//*************************************************************************

typedef struct
{
	char name[iV_TEXNAME_MAX];
	unsigned int id;
	unsigned int w;
	unsigned int h;
} iTexPage;

//*************************************************************************

extern unsigned int _TEX_INDEX;
extern iTexPage _TEX_PAGE[iV_TEX_MAX];

//*************************************************************************

extern int iV_GetTexture(const char *filename);
extern void iV_unloadImage(iV_Image *image);
extern unsigned int iV_getPixelFormat(const iV_Image *image);

extern int pie_ReplaceTexPage(iV_Image *s, const char *texPage);
extern int pie_AddTexPage(iV_Image *s, const char *filename, int slot);
extern void pie_TexInit(void);

/*!
 * Turns filename into a pagename if possible
 * \param[in,out] filename Filename to pagify
 */
extern void pie_MakeTexPageName(char * filename);

//*************************************************************************

extern void pie_TexShutDown(void);

#endif
