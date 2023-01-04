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
#include "lib/framework/frame.h"

#include "lib/framework/frameresource.h"
#include "lib/framework/file.h"

#include "bitimage.h"
#include "tex.h"

#include <set>
#include <algorithm>
#include <unordered_map>
#include <vector>

static std::vector<IMAGEFILE *> files;

struct ImageMergeRectangle
{
	bool operator <(ImageMergeRectangle const &b) const
	{
		if (std::max(siz.x, siz.y) != std::max(b.siz.x, b.siz.y))
		{
			return std::max(siz.x, siz.y) < std::max(b.siz.x, b.siz.y);
		}
		if (std::min(siz.x, siz.y) != std::min(b.siz.x, b.siz.y))
		{
			return std::min(siz.x, siz.y) < std::min(b.siz.x, b.siz.y);
		}
		return siz.x                  < b.siz.x;
	}

	int index;  // Index in ImageDefs array.
	size_t page;   // Texture page index.
	Vector2i loc = Vector2i(0, 0), siz = Vector2i(0, 0);
	iV_Image *data;
};

struct ImageMerge
{
	static const int pageSize = 256;

	void arrange();

	std::vector<ImageMergeRectangle> images;
	std::vector<int> pages;  // List of page sizes, normally all pageSize, unless an image is too large for a normal page.
};

AtlasImageDef *iV_GetImage(const WzString &filename)
{
	for (const auto& file : files)
	{
		auto pImageDef = file->find(filename);
		if (pImageDef != nullptr)
		{
			return pImageDef;
		}
	}
	debug(LOG_ERROR, "%s not found in image list!", filename.toUtf8().c_str());
	return nullptr;
}

// Used to provide empty space between sprites arranged on the page, to avoid display glitches when scaling + drawing
#define SPRITE_BUFFER_PIXELS 1

void checkRect(const ImageMergeRectangle& rect, const int pageSize)
{
	if ((rect.loc.x + rect.siz.x) > pageSize)
	{
		debug(LOG_ERROR, "Merge rectangle bounds extend outside of pageSize bounds: %d > %d", (rect.loc.x + rect.siz.x), pageSize);
	}
	if ((rect.loc.y + rect.siz.y) > pageSize)
	{
		debug(LOG_ERROR, "Merge rectangle bounds extend outside of pageSize bounds: %d > %d", (rect.loc.y + rect.siz.y), pageSize);
	}
	assert(rect.loc.x >= 0);
	assert(rect.loc.y >= 0);
}

inline void ImageMerge::arrange()
{
	std::multiset<ImageMergeRectangle> freeSpace;
	pages.clear();

	std::sort(images.begin(), images.end());

	std::vector<ImageMergeRectangle>::iterator r = images.end();
	while (r != images.begin())
	{
		--r;

		std::multiset<ImageMergeRectangle>::iterator f = freeSpace.lower_bound(*r);  // Find smallest free rectangle which is large enough.
		while (f != freeSpace.end() && (f->siz.x < r->siz.x || f->siz.y < r->siz.y))
		{
			++f;  // Rectangle has wrong shape.
		}
		if (f == freeSpace.end())
		{
			// No free space, make new page.
			int s = pageSize;
			while (s < (r->siz.x | r->siz.y))
			{
				s *= 2;
			}
			ImageMergeRectangle newPage;
			newPage.page = pages.size();
			newPage.loc = Vector2i(0, 0);
			newPage.siz = Vector2i(s, s);
			pages.push_back(s);
			f = freeSpace.insert(newPage);
		}
		r->page = f->page;
		r->loc = f->loc;
		ImageMergeRectangle spRight;
		ImageMergeRectangle spDown;
		spRight.page = f->page;
		spDown.page  = f->page;
		spRight.loc = f->loc + Vector2i(r->siz.x + SPRITE_BUFFER_PIXELS, 0);
		spDown.loc  = f->loc + Vector2i(0, r->siz.y + SPRITE_BUFFER_PIXELS);
		spRight.siz = Vector2i(f->siz.x - (r->siz.x + SPRITE_BUFFER_PIXELS), r->siz.y);
		spDown.siz  = Vector2i(r->siz.x, f->siz.y - (r->siz.y + SPRITE_BUFFER_PIXELS));
		if (spRight.siz.x <= spDown.siz.y)
		{
			// Split horizontally.
			spDown.siz.x = f->siz.x;
		}
		else
		{
			// Split vertically.
			spRight.siz.y = f->siz.y;
		}
		checkRect(spDown, pages.at(spDown.page));
		checkRect(spRight, pages.at(spRight.page));
		if (spRight.siz.x > 0 && spRight.siz.y > 0)
		{
			freeSpace.insert(spRight);
		}
		if (spDown.siz.x > 0 && spDown.siz.y > 0)
		{
			freeSpace.insert(spDown);
		}
		freeSpace.erase(f);
	}
}

IMAGEFILE *iV_LoadImageFile(const char *fileName)
{
	// Find the directory of images.
	std::string imageDir = fileName;
	if (imageDir.find_last_of('.') != std::string::npos)
	{
		imageDir.erase(imageDir.find_last_of('.'));
	}
	imageDir += '/';

	// Load the image list file.
	char *pFileData;
	unsigned pFileSize;
	if (!loadFile(fileName, &pFileData, &pFileSize))
	{
		debug(LOG_ERROR, "iV_LoadImageFile: failed to open %s", fileName);
		return nullptr;
	}

	char *ptr = pFileData;
	// count lines, which is identical to number of images
	int numImages = 0;
	while (ptr < pFileData + pFileSize && *ptr != '\0')
	{
		numImages += *ptr == '\n';
		++ptr;
	}
	IMAGEFILE *imageFile = new IMAGEFILE;
	imageFile->imageDefs.resize(numImages);
	imageFile->imageNamesMap.reserve(numImages);
	ImageMerge pageLayout;
	pageLayout.images.resize(numImages);
	ptr = pFileData;
	numImages = 0;
	while (ptr < pFileData + pFileSize)
	{
		int temp, retval;
		AtlasImageDef *AtlasImageDef = &imageFile->imageDefs[numImages];

		char tmpName[256];
		retval = sscanf(ptr, "%d,%d,%255[^\r\n\",]%n", &AtlasImageDef->XOffset, &AtlasImageDef->YOffset, tmpName, &temp);
		if (retval != 3)
		{
			debug(LOG_ERROR, "Bad line in \"%s\".", fileName);
			delete imageFile;
			free(pFileData);
			return nullptr;
		}
		imageFile->imageNamesMap.insert(std::make_pair(WzString::fromUtf8(tmpName), AtlasImageDef));
		std::string spriteName = imageDir + tmpName;

		ImageMergeRectangle *imageRect = &pageLayout.images[numImages];
		imageRect->index = numImages;
		imageRect->data = new iV_Image;
		if (!iV_loadImage_PNG2(spriteName.c_str(), *imageRect->data, true))
		{
			debug(LOG_ERROR, "Failed to find image \"%s\" listed in \"%s\".", spriteName.c_str(), fileName);
			delete imageFile;
			free(pFileData);
			return nullptr;
		}
		imageRect->siz = Vector2i(imageRect->data->width(), imageRect->data->height());
		numImages++;
		ptr += temp;
		while (ptr < pFileData + pFileSize && *ptr++ != '\n') {} // skip rest of line
	}
	free(pFileData);

	pageLayout.arrange();  // Arrange all the images onto texture pages (attempt to do so with as few pages as possible).
	imageFile->pages.resize(pageLayout.pages.size());

	std::vector<iV_Image> ivImages(pageLayout.pages.size());

	for (unsigned p = 0; p < pageLayout.pages.size(); ++p)
	{
		int size = pageLayout.pages[p];
		ivImages[p].allocate(size, size, 4, true);
		imageFile->pages[p].size = size;
		// Must set imageFile->pages[p].id later, after filling out ivImages[p].bmp.
	}

	for (std::vector<ImageMergeRectangle>::const_iterator r = pageLayout.images.begin(); r != pageLayout.images.end(); ++r)
	{
		imageFile->imageDefs[r->index].TPageID = r->page;
		imageFile->imageDefs[r->index].Tu = r->loc.x;
		imageFile->imageDefs[r->index].Tv = r->loc.y;
		imageFile->imageDefs[r->index].Width = r->siz.x;
		imageFile->imageDefs[r->index].Height = r->siz.y;

		// Copy image data onto texture page.
		iV_Image *srcImage = r->data;
		int srcChannels = srcImage->channels();
		int srcStride = srcImage->width() * srcChannels; // Not sure whether to pad in the case that srcDepth ≠ 4, however this apparently cannot happen.
		const unsigned char *srcBytes = srcImage->bmp() + 0 * srcChannels + 0 * srcStride;
		iV_Image *dstImage = &ivImages[r->page];
		int dstChannels = dstImage->channels();
		int dstStride = dstImage->width() * dstChannels;
		unsigned char *dstBytes = dstImage->bmp_w() + r->loc.x * dstChannels + r->loc.y * dstStride;
		Vector2i size = r->siz;
		unsigned char rgba[4] = {0x00, 0x00, 0x00, 0xFF};
		for (int y = 0; y < size.y; ++y)
			for (int x = 0; x < size.x; ++x)
			{
				memcpy(rgba, srcBytes + x * srcChannels + y * srcStride, srcChannels);
				memcpy(dstBytes + x * dstChannels + y * dstStride, rgba, dstChannels);
			}

		// Finished reading the image data and copying it to the texture page, delete it.
		r->data->clear();
		delete r->data;
	}

	// Debug usage only. Dump all images to disk (do mkdir images/, first). Doesn't dump the alpha channel, since the .ppm format doesn't support that.
	/*for (unsigned p = 0; p < pageLayout.pages.size(); ++p)
	{
		char fName[100];
		sprintf(fName, "%s-%d", fileName, p);
		printf("Dump %s\n", fName);
		FILE *f = fopen(fName, "wb");
		iV_Image *image = &ivImages[p];
		fprintf(f, "P6\n%d %d\n255\n", image->width, image->height);
		for (int x = 0; x < image->width*image->height; ++x)
			if (fwrite(image->bmp + x*4, 3, 1, f) == 0)
				abort();
		fclose(f);
	}*/

	// Upload texture pages and free image data.
	for (unsigned p = 0; p < pageLayout.pages.size(); ++p)
	{
		char arbitraryName[256];
		ssprintf(arbitraryName, "%s-%03u", fileName, p);
		// Now we can set imageFile->pages[p].id. This free()s the ivImages[p].bmp array!
		gfx_api::texture *pTexture = gfx_api::context::get().loadTextureFromUncompressedImage(std::move(ivImages[p]), gfx_api::texture_type::user_interface, arbitraryName);
		imageFile->pages[p].id = pie_AddTexPage(pTexture, arbitraryName, gfx_api::texture_type::user_interface);
	}
	ivImages.clear();

	// duplicate some data, since we want another access point to these data structures now, FIXME
	for (unsigned i = 0; i < imageFile->imageDefs.size(); i++)
	{
		imageFile->imageDefs[i].textureId = imageFile->pages[imageFile->imageDefs[i].TPageID].id;
		imageFile->imageDefs[i].invTextureSize = 1.f / imageFile->pages[imageFile->imageDefs[i].TPageID].size;
	}

	files.push_back(imageFile);

	return imageFile;
}

void iV_FreeImageFile(IMAGEFILE *imageFile)
{
	delete imageFile;
}

IMAGEFILE::~IMAGEFILE()
{
	auto it = std::find(files.begin(), files.end(), this);
	ASSERT(it != files.end(), "Calling iV_FreeImageFile for IMAGEFILE that wasn't loaded??");
	if (it != files.end())
	{
		files.erase(it);
	}
}

AtlasImageDef* IMAGEFILE::find(WzString const &filename)
{
	auto it = imageNamesMap.find(filename);
	if (it != imageNamesMap.end())
	{
		return it->second;

	}
//	debug(LOG_ERROR, "%s not found in image list!", filename.toUtf8().c_str());
	return nullptr; // Error, image not found.
}

void iV_ImageFileShutdown()
{
	ASSERT(files.empty(), "%zu IMAGEFILEs were not properly unloaded", files.size());
	for (auto file : files)
	{
		delete file;
	}
	files.clear();
}
