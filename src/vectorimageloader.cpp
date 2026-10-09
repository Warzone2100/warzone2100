// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/


#include "vectorimageloader.h"

#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/vectorimage.h"

#include "init.h"

#include <unordered_set>
#include <vector>

namespace
{

std::unordered_set<std::string> refused;

bool readGameVectorImage(const std::string &path, bool optional)
{
	if (!PHYSFS_exists(path.c_str()))
	{
		if (!optional)
		{
			debug(LOG_ERROR, "Vector image %s: missing", path.c_str());
		}
		return false;
	}
	if (!isGameDataFile(path.c_str()))
	{
		debug(LOG_ERROR, "Vector image %s: not in the game's own data (overridden by a mod or a map)", path.c_str());
		return false;
	}
	PHYSFS_file *file = PHYSFS_openRead(path.c_str());
	if (file == nullptr)
	{
		return false;
	}
	bool loaded = false;
	const PHYSFS_sint64 length = PHYSFS_fileLength(file);
	if (length > 0 && static_cast<uint64_t>(length) <= VECTOR_IMAGE_MAX_BYTES)
	{
		std::vector<char> data(static_cast<size_t>(length));
		if (WZ_PHYSFS_readBytes(file, data.data(), static_cast<PHYSFS_uint32>(length)) == length)
		{
			loaded = iV_AddVectorImage(path, data.data(), data.size());
		}
	}
	else
	{
		debug(LOG_ERROR, "Vector image %s: empty, or larger than %zu bytes", path.c_str(), VECTOR_IMAGE_MAX_BYTES);
	}
	PHYSFS_close(file);
	return loaded;
}

}

bool loadGameVectorImage(const std::string &path, bool optional)
{
	if (iV_HasVectorImage(path))
	{
		return true;
	}
	if (refused.count(path) != 0)
	{
		return false;
	}
	if (!readGameVectorImage(path, optional))
	{
		refused.insert(path);
		return false;
	}
	return true;
}

bool drawGameVectorImage(const std::string &path, float x, float y, float width, float height, PIELIGHT colour, float haloWidth, bool optional)
{
	return loadGameVectorImage(path, optional) && iV_DrawVectorImage(path, x, y, width, height, colour, haloWidth);
}

bool drawGameVectorImageTransformed(const std::string &path, const glm::mat4 &modelViewProjection, float x, float y, float width, float height, PIELIGHT colour, float haloWidth)
{
	return loadGameVectorImage(path) && iV_DrawVectorImageTransformed(path, modelViewProjection, x, y, width, height, colour, haloWidth);
}

float gameVectorImageAspect(const std::string &path, bool optional)
{
	if (!loadGameVectorImage(path, optional))
	{
		return 1.f;
	}
	return iV_VectorImageAspect(path).value_or(1.f);
}
