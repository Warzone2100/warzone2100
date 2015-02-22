/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include <QtCore/QHash>
#include <QtCore/QString>

static QHash<QString, ImageDef *> images;
static QList<IMAGEFILE *> files;

struct ImageMergeRectangle
{
	bool operator <(ImageMergeRectangle const &b) const
	{
		if (std::max(siz.x, siz.y) != std::max(b.siz.x, b.siz.y)) return std::max(siz.x, siz.y) < std::max(b.siz.x, b.siz.y);
		if (std::min(siz.x, siz.y) != std::min(b.siz.x, b.siz.y)) return std::min(siz.x, siz.y) < std::min(b.siz.x, b.siz.y);
		                                                          return siz.x                  < b.siz.x;
	}

	int index;  // Index in ImageDefs array.
	int page;   // Texture page index.
	Vector2i loc, siz;
	iV_Image *data;
};

struct ImageMerge
{
	static const int pageSize = 256;

	void arrange();

	std::vector<ImageMergeRectangle> images;
	std::vector<int> pages;  // List of page sizes, normally all pageSize, unless an image is too large for a normal page.
};

ImageDef *iV_GetImage(const QString &filename, int x, int y)
{
	if (!images.contains(filename))
	{
		debug(LOG_ERROR, "%s not found in image list!", filename.toUtf8().constData());
		return NULL;
	}
	return images.value(filename);
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
		spRight.loc = f->loc + Vector2i(r->siz.x, 0);
		spDown.loc  = f->loc + Vector2i(0, r->siz.y);
		spRight.siz = Vector2i(f->siz.x - r->siz.x, r->siz.y);
		spDown.siz  = Vector2i(r->siz.x, f->siz.y - r->siz.y);
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
		return NULL;
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
	imageFile->imageNames.resize(numImages);
	ImageMerge pageLayout;
	pageLayout.images.resize(numImages);
	ptr = pFileData;
	numImages = 0;
	while (ptr < pFileData + pFileSize)
	{
		int temp, retval;
		ImageDef *ImageDef = &imageFile->imageDefs[numImages];

		char tmpName[256];
		retval = sscanf(ptr, "%d,%d,%255[^\r\n\",]%n", &ImageDef->XOffset, &ImageDef->YOffset, tmpName, &temp);
		if (retval != 3)
		{
			debug(LOG_ERROR, "Bad line in \"%s\".", fileName);
			delete imageFile;
			return NULL;
		}
		imageFile->imageNames[numImages].first = tmpName;
		imageFile->imageNames[numImages].second = numImages;
		std::string spriteName = imageDir + tmpName;

		ImageMergeRectangle *imageRect = &pageLayout.images[numImages];
		imageRect->index = numImages;
		imageRect->data = new iV_Image;
		if (!iV_loadImage_PNG(spriteName.c_str(), imageRect->data))
		{
			debug(LOG_ERROR, "Failed to find image \"%s\" listed in \"%s\".", spriteName.c_str(), fileName);
			delete imageFile;
			return NULL;
		}
		imageRect->siz = Vector2i(imageRect->data->width, imageRect->data->height);
		numImages++;
		ptr += temp;
		while (ptr < pFileData + pFileSize && *ptr++ != '\n') {} // skip rest of line

		images.insert(tmpName, ImageDef);
	}
	free(pFileData);

	std::sort(imageFile->imageNames.begin(), imageFile->imageNames.end());

	pageLayout.arrange();  // Arrange all the images onto texture pages (attempt to do so with as few pages as possible).
	imageFile->pages.resize(pageLayout.pages.size());

	std::vector<iV_Image> ivImages(pageLayout.pages.size());

	for (unsigned p = 0; p < pageLayout.pages.size(); ++p)
	{
		int size = pageLayout.pages[p];
		ivImages[p].depth = 4;
		ivImages[p].width = size;
		ivImages[p].height = size;
		ivImages[p].bmp = (unsigned char *)malloc(size*size*4);  // MUST be malloc, since this is free()d later by pie_AddTexPage().
		memset(ivImages[p].bmp, 0x00, size*size*4);
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
		int srcDepth = srcImage->depth;
		int srcStride = srcImage->width*srcDepth;  // Not sure whether to pad in the case that srcDepth ≠ 4, however this apparently cannot happen.
		unsigned char *srcBytes = srcImage->bmp + 0*srcDepth + 0*srcStride;
		iV_Image *dstImage = &ivImages[r->page];
		int dstDepth = dstImage->depth;
		int dstStride = dstImage->width*dstDepth;
		unsigned char *dstBytes = dstImage->bmp + r->loc.x*dstDepth + r->loc.y*dstStride;
		Vector2i size = r->siz;
		unsigned char rgba[4] = {0x00, 0x00, 0x00, 0xFF};
		for (int y = 0; y < size.y; ++y)
			for (int x = 0; x < size.x; ++x)
		{
			memcpy(rgba, srcBytes + x*srcDepth + y*srcStride, srcDepth);
			memcpy(dstBytes + x*dstDepth + y*dstStride, rgba, dstDepth);
		}

		// Finished reading the image data and copying it to the texture page, delete it.
		free(r->data->bmp);
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
		imageFile->pages[p].id = pie_AddTexPage(&ivImages[p], arbitraryName, false);
	}

	// duplicate some data, since we want another access point to these data structures now, FIXME
	for (unsigned i = 0; i < imageFile->imageDefs.size(); i++)
	{
		imageFile->imageDefs[i].textureId = imageFile->pages[imageFile->imageDefs[i].TPageID].id;
		imageFile->imageDefs[i].invTextureSize = 1.f / imageFile->pages[imageFile->imageDefs[i].TPageID].size;
	}

	files.append(imageFile);

	return imageFile;
}

void iV_FreeImageFile(IMAGEFILE *imageFile)
{
	// so when we get here, it is time to redo everything. will clean this up later. TODO.
	files.clear();
	images.clear();
	delete imageFile;
}

Image IMAGEFILE::find(std::string const &name)
{
	std::pair<std::string, int> val(name, 0);
	std::vector<std::pair<std::string, int> >::const_iterator i = std::lower_bound(imageNames.begin(), imageNames.end(), val);
	if (i != imageNames.end() && i->first == name)
	{
		return Image(this, i->second);
	}
	return Image(this, 0);  // Error, image not found.
}
