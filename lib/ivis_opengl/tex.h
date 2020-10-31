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
#ifndef _tex_
#define _tex_

#include "lib/framework/wzstring.h"
#include "gfx_api.h"
#include "png_util.h"

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#define iV_TEX_INVALID 0
#define iV_TEXNAME_MAX 64

#define iV_TEXNAME_TCSUFFIX "_tcmask"

//*************************************************************************

gfx_api::texture& pie_Texture(size_t page);
size_t pie_NumberOfPages();
size_t pie_ReserveTexture(const char *name, const size_t& width, const size_t& height);
void pie_AssignTexture(size_t page, gfx_api::texture* texture);

//*************************************************************************

bool scaleImageMaxSize(iV_Image *s, int maxWidth, int maxHeight);

optional<size_t> iV_GetTexture(const char *filename, bool compression = true, int maxWidth = -1, int maxHeight = -1);
void iV_unloadImage(iV_Image *image);
gfx_api::pixel_format iV_getPixelFormat(const iV_Image *image);

bool replaceTexture(const WzString &oldfile, const WzString &newfile);
size_t pie_AddTexPage(iV_Image *s, const char *filename, bool gameTexture);
size_t pie_AddTexPage(iV_Image *s, const char *filename, bool gameTexture, size_t page);
void pie_TexInit();

std::string pie_MakeTexPageName(const std::string& filename);
std::string pie_MakeTexPageTCMaskName(const std::string& filename);

//*************************************************************************

void pie_TexShutDown();

#endif
