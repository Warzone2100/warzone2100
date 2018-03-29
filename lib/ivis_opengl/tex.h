/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#ifndef _tex_
#define _tex_

#include "lib/framework/wzstring.h"
#include "gfx_api.h"
#include "png_util.h"

#define iV_TEX_INVALID 0
#define iV_TEXNAME_MAX 64

#define iV_TEXNAME_TCSUFFIX "_tcmask"

//*************************************************************************

gfx_api::texture& pie_Texture(int page);
int pie_NumberOfPages();
int pie_ReserveTexture(const char *name, const size_t& width, const size_t& height);
void pie_AssignTexture(int page, gfx_api::texture* texture);

//*************************************************************************

int iV_GetTexture(const char *filename, bool compression = true);
void iV_unloadImage(iV_Image *image);
gfx_api::pixel_format iV_getPixelFormat(const iV_Image *image);

bool replaceTexture(const WzString &oldfile, const WzString &newfile);
int pie_AddTexPage(iV_Image *s, const char *filename, bool gameTexture, int page = -1);
void pie_TexInit();

void pie_MakeTexPageName(char *filename);
void pie_MakeTexPageTCMaskName(char *filename);

//*************************************************************************

void pie_TexShutDown();

#endif
