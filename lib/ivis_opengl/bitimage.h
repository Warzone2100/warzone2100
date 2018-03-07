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
#ifndef __INCLUDED_BITIMAGE__
#define __INCLUDED_BITIMAGE__

#include "ivisdef.h"
#include "pietypes.h"
#include "lib/framework/wzstring.h"

static inline WZ_DECL_PURE unsigned short iV_GetImageWidth(const IMAGEFILE *ImageFile, const unsigned short ID)
{
	assert(ID < ImageFile->imageDefs.size());
	return Image(ImageFile, ID).width();
}


static inline WZ_DECL_PURE unsigned short iV_GetImageHeight(const IMAGEFILE *ImageFile, const unsigned short ID)
{
	assert(ID < ImageFile->imageDefs.size());
	return Image(ImageFile, ID).height();
}


static inline WZ_DECL_PURE short iV_GetImageXOffset(const IMAGEFILE *ImageFile, const unsigned short ID)
{
	assert(ID < ImageFile->imageDefs.size());
	return Image(ImageFile, ID).xOffset();
}


static inline WZ_DECL_PURE short iV_GetImageYOffset(const IMAGEFILE *ImageFile, const unsigned short ID)
{
	assert(ID < ImageFile->imageDefs.size());
	return Image(ImageFile, ID).yOffset();
}

ImageDef *iV_GetImage(const WzString &filename);
IMAGEFILE *iV_LoadImageFile(const char *FileData);
void iV_FreeImageFile(IMAGEFILE *ImageFile);

#endif
