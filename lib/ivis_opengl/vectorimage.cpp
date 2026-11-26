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

#include "vectorimage.h"

#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/pietypes.h"

#include <thorvg.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace
{

constexpr int MAX_DEPTH = 32;
constexpr float MAX_HALO_PIXELS = 16.f;
constexpr size_t MAX_RASTERIZATIONS_PER_IMAGE = 16;

const char *const ALLOWED_ELEMENTS[] = {
	"svg", "g", "path", "rect", "circle", "ellipse", "line", "polyline", "polygon",
};

const char *const ALLOWED_ATTRIBUTES[] = {
	"xmlns", "version", "id", "class", "width", "height", "viewBox", "preserveAspectRatio",
	"x", "y", "x1", "y1", "x2", "y2", "cx", "cy", "r", "rx", "ry", "d", "points", "transform",
	"fill", "fill-opacity", "fill-rule", "stroke", "stroke-width", "stroke-linecap", "stroke-linejoin",
	"stroke-miterlimit", "stroke-opacity", "opacity",
};

template <size_t N>
bool inList(const char *const (&list)[N], const std::string &name)
{
	return std::any_of(std::begin(list), std::end(list), [&name](const char *entry) { return name == entry; });
}

bool isSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNameStart(char c)
{
	return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == ':';
}

bool isNameChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':' || c == '-' || c == '.';
}

bool startsWith(const char *data, size_t size, size_t pos, const char *prefix)
{
	const size_t length = std::strlen(prefix);
	return pos + length <= size && std::memcmp(data + pos, prefix, length) == 0;
}

size_t find(const char *data, size_t size, size_t pos, const char *needle)
{
	const size_t length = std::strlen(needle);
	for (; pos + length <= size; ++pos)
	{
		if (std::memcmp(data + pos, needle, length) == 0)
		{
			return pos;
		}
	}
	return std::string::npos;
}

struct RasterKey
{
	int width;
	int height;
	int halo; ///< in 1/16 pixels

	bool operator<(const RasterKey &other) const
	{
		return std::tie(width, height, halo) < std::tie(other.width, other.height, other.halo);
	}
};

struct Rasterization
{
	std::unique_ptr<gfx_api::texture> texture;
	int width = 0;  ///< the texture's, padding included
	int height = 0;
	int padding = 0;
};

struct VectorImage
{
	std::vector<char> data;
	bool unusable = false;
	float aspect = 0.f;  ///< (0 until known)
	std::map<RasterKey, Rasterization> rasterizations;
};

std::unordered_map<std::string, VectorImage> images;
float scaleX = 1.f;
float scaleY = 1.f;
bool thorvgReady = false;

/// The shape grown by `radius` pixels: each pixel takes the most coverage within that distance
void dilate(std::vector<uint8_t> &coverage, int width, int height, float radius)
{
	const int reach = static_cast<int>(std::ceil(radius));
	std::vector<std::pair<Vector2i, float>> kernel;
	for (int dy = -reach; dy <= reach; ++dy)
	{
		for (int dx = -reach; dx <= reach; ++dx)
		{
			const float weight = std::min(1.f, std::max(0.f, radius + 0.5f - std::sqrt(static_cast<float>(dx * dx + dy * dy))));
			if (weight > 0.f)
			{
				kernel.emplace_back(Vector2i(dx, dy), weight);
			}
		}
	}
	std::vector<uint8_t> grown(coverage.size(), 0);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float best = 0.f;
			for (const auto &tap : kernel)
			{
				const int sx = x + tap.first.x;
				const int sy = y + tap.first.y;
				if (sx >= 0 && sy >= 0 && sx < width && sy < height)
				{
					best = std::max(best, coverage[static_cast<size_t>(sy) * width + sx] * tap.second);
				}
			}
			grown[static_cast<size_t>(y) * width + x] = static_cast<uint8_t>(std::lround(best));
		}
	}
	coverage.swap(grown);
}

bool startThorVG()
{
	if (!thorvgReady)
	{
		if (tvg::Initializer::init(0) != tvg::Result::Success)
		{
			debug(LOG_ERROR, "Vector images: ThorVG failed to start");
			return false;
		}
		thorvgReady = true;
	}
	return true;
}

/// ThorVG's picture of the image, or nullptr (and the image marked unusable)
tvg::Picture *loadPicture(const std::string &name, VectorImage &image)
{
	tvg::Picture *picture = tvg::Picture::gen();
	if (picture == nullptr || picture->load(image.data.data(), static_cast<uint32_t>(image.data.size()), "svg", nullptr, true) != tvg::Result::Success)
	{
		tvg::Paint::rel(picture);
		image.unusable = true;
		debug(LOG_ERROR, "Vector image %s: ThorVG can't load it", name.c_str());
		return nullptr;
	}
	return picture;
}

nonstd::optional<VectorImageCoverage> coverageOf(const std::string &name, VectorImage &image, int width, int height, float haloPixels)
{
	if (!startThorVG())
	{
		return nonstd::nullopt;
	}
	VectorImageCoverage result;
	result.padding = haloPixels > 0.f ? static_cast<int>(std::ceil(haloPixels)) + 1 : 0;
	result.width = width + result.padding * 2;
	result.height = height + result.padding * 2;
	std::vector<uint32_t> pixels(static_cast<size_t>(result.width) * result.height, 0);
	{
		std::unique_ptr<tvg::SwCanvas> canvas(tvg::SwCanvas::gen(tvg::EngineOption::None));
		if (canvas == nullptr
			|| canvas->target(pixels.data(), static_cast<uint32_t>(result.width), static_cast<uint32_t>(result.width), static_cast<uint32_t>(result.height), tvg::ColorSpace::ABGR8888) != tvg::Result::Success)
		{
			return nonstd::nullopt;
		}
		tvg::Picture *picture = loadPicture(name, image);
		if (picture == nullptr)
		{
			return nonstd::nullopt;
		}
		picture->size(static_cast<float>(width), static_cast<float>(height));
		picture->translate(static_cast<float>(result.padding), static_cast<float>(result.padding));
		if (canvas->add(picture) != tvg::Result::Success || canvas->draw(true) != tvg::Result::Success || canvas->sync() != tvg::Result::Success)
		{
			return nonstd::nullopt;
		}
	}

	result.coverage.resize(pixels.size());
	std::transform(pixels.begin(), pixels.end(), result.coverage.begin(), [](uint32_t pixel) { return static_cast<uint8_t>(pixel >> 24); });
	if (haloPixels > 0.f)
	{
		dilate(result.coverage, result.width, result.height, haloPixels);
	}
	return result;
}

Rasterization rasterize(const std::string &name, VectorImage &image, int width, int height, float haloPixels)
{
	Rasterization result;
	auto rasterized = coverageOf(name, image, width, height, haloPixels);
	if (!rasterized.has_value())
	{
		return result;
	}
	const std::vector<uint8_t> &coverage = rasterized->coverage;
	const int textureWidth = rasterized->width;
	const int textureHeight = rasterized->height;
	const int padding = rasterized->padding;

	// White, with the coverage as (straight) alpha: drawing multiplies it by the color
	iV_Image bitmap;
	if (!bitmap.allocate(static_cast<unsigned int>(textureWidth), static_cast<unsigned int>(textureHeight), 4, false))
	{
		return result;
	}
	unsigned char *out = bitmap.bmp_w();
	for (size_t i = 0; i < coverage.size(); ++i)
	{
		out[i * 4 + 0] = 255;
		out[i * 4 + 1] = 255;
		out[i * 4 + 2] = 255;
		out[i * 4 + 3] = coverage[i];
	}
	result.texture.reset(gfx_api::context::get().createTextureForCompatibleImageUploads(1, bitmap, "vectorimage::" + name));
	if (result.texture == nullptr || !result.texture->upload(0, bitmap))
	{
		result.texture.reset();
		return result;
	}
	result.width = textureWidth;
	result.height = textureHeight;
	result.padding = padding;
	return result;
}

void dropRasterizations()
{
	for (auto &entry : images)
	{
		entry.second.rasterizations.clear();
	}
}

}

nonstd::optional<std::string> iV_CheckVectorImage(const char *data, size_t size)
{
	if (data == nullptr || size == 0)
	{
		return std::string("empty");
	}
	if (size > VECTOR_IMAGE_MAX_BYTES)
	{
		return astringf("larger than %zu bytes", VECTOR_IMAGE_MAX_BYTES);
	}
	if (std::memchr(data, '\0', size) != nullptr)
	{
		return std::string("contains a NUL byte");
	}

	size_t pos = 0;
	if (startsWith(data, size, 0, "\xEF\xBB\xBF"))
	{
		pos = 3;
	}
	size_t elements = 0;
	int depth = 0;
	bool seenRoot = false;
	bool rootClosed = false;
	while (pos < size)
	{
		if (data[pos] != '<')
		{
			if (!isSpace(data[pos]))
			{
				return std::string("text outside the elements");
			}
			++pos;
			continue;
		}
		if (startsWith(data, size, pos, "<?"))
		{
			// The XML declaration only, before anything else
			const size_t end = find(data, size, pos, "?>");
			if (seenRoot || !startsWith(data, size, pos, "<?xml ") || end == std::string::npos
				|| std::memchr(data + pos + 1, '<', end - pos - 1) != nullptr)
			{
				return std::string("a processing instruction");
			}
			pos = end + 2;
			continue;
		}
		if (startsWith(data, size, pos, "<!--"))
		{
			const size_t end = find(data, size, pos + 4, "-->");
			if (end == std::string::npos)
			{
				return std::string("an unterminated comment");
			}
			pos = end + 3;
			continue;
		}
		if (startsWith(data, size, pos, "<!"))
		{
			return std::string("a declaration (DOCTYPE, ENTITY or CDATA)");
		}

		const bool closing = startsWith(data, size, pos, "</");
		size_t p = pos + (closing ? 2 : 1);
		if (p >= size || !isNameStart(data[p]))
		{
			return std::string("a malformed tag");
		}
		const size_t nameStart = p;
		while (p < size && isNameChar(data[p]))
		{
			++p;
		}
		const std::string name(data + nameStart, p - nameStart);
		if (!inList(ALLOWED_ELEMENTS, name))
		{
			return "the element <" + name + ">";
		}
		if (closing)
		{
			while (p < size && isSpace(data[p]))
			{
				++p;
			}
			if (p >= size || data[p] != '>' || depth == 0)
			{
				return std::string("a malformed closing tag");
			}
			if (--depth == 0)
			{
				rootClosed = true;
			}
			pos = p + 1;
			continue;
		}
		if (rootClosed)
		{
			return std::string("a second root element");
		}
		if (!seenRoot && name != "svg")
		{
			return std::string("a root element other than <svg>");
		}
		seenRoot = true;
		if (++elements > VECTOR_IMAGE_MAX_ELEMENTS)
		{
			return astringf("more than %zu elements", VECTOR_IMAGE_MAX_ELEMENTS);
		}

		bool selfClosing = false;
		for (;;)
		{
			while (p < size && isSpace(data[p]))
			{
				++p;
			}
			if (p >= size)
			{
				return std::string("a truncated tag");
			}
			if (data[p] == '>')
			{
				++p;
				break;
			}
			if (startsWith(data, size, p, "/>"))
			{
				selfClosing = true;
				p += 2;
				break;
			}
			if (!isNameStart(data[p]))
			{
				return std::string("a malformed attribute");
			}
			const size_t attributeStart = p;
			while (p < size && isNameChar(data[p]))
			{
				++p;
			}
			const std::string attribute(data + attributeStart, p - attributeStart);
			if (!inList(ALLOWED_ATTRIBUTES, attribute))
			{
				return "the attribute " + attribute;
			}
			while (p < size && isSpace(data[p]))
			{
				++p;
			}
			if (p >= size || data[p] != '=')
			{
				return std::string("a malformed attribute");
			}
			++p;
			while (p < size && isSpace(data[p]))
			{
				++p;
			}
			if (p >= size || (data[p] != '"' && data[p] != '\''))
			{
				return std::string("an unquoted attribute value");
			}
			const char quote = data[p++];
			const size_t valueStart = p;
			while (p < size && data[p] != quote)
			{
				if (data[p] == '<' || data[p] == '&')
				{
					return std::string("markup or an entity in an attribute value");
				}
				++p;
			}
			if (p >= size)
			{
				return std::string("an unterminated attribute value");
			}
			std::string value(data + valueStart, p - valueStart);
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (value.find("url(") != std::string::npos)
			{
				return std::string("a url() reference");
			}
			++p;
		}
		if (selfClosing)
		{
			rootClosed = rootClosed || depth == 0;
		}
		else if (++depth > MAX_DEPTH)
		{
			return astringf("elements nested deeper than %d", MAX_DEPTH);
		}
		pos = p;
	}
	if (!seenRoot)
	{
		return std::string("no <svg> element");
	}
	if (depth != 0)
	{
		return std::string("unclosed elements");
	}
	return nonstd::nullopt;
}

bool iV_AddVectorImage(const std::string &name, const char *data, size_t size)
{
	if (auto problem = iV_CheckVectorImage(data, size))
	{
		debug(LOG_ERROR, "Vector image %s refused: %s", name.c_str(), problem->c_str());
		return false;
	}
	VectorImage &image = images[name];
	image.data.assign(data, data + size);
	image.unusable = false;
	image.rasterizations.clear();
	return true;
}

bool iV_HasVectorImage(const std::string &name)
{
	return images.count(name) != 0;
}

nonstd::optional<VectorImageCoverage> iV_VectorImageCoverage(const std::string &name, int width, int height, float haloPixels)
{
	auto it = images.find(name);
	if (it == images.end() || it->second.unusable || width < 1 || height < 1 || width > VECTOR_IMAGE_MAX_PIXELS || height > VECTOR_IMAGE_MAX_PIXELS)
	{
		return nonstd::nullopt;
	}
	return coverageOf(name, it->second, width, height, std::min(std::max(haloPixels, 0.f), MAX_HALO_PIXELS));
}

float iV_VectorImagesScale()
{
	return scaleX;
}

nonstd::optional<float> iV_VectorImageAspect(const std::string &name)
{
	auto it = images.find(name);
	if (it == images.end() || it->second.unusable)
	{
		return nonstd::nullopt;
	}
	VectorImage &image = it->second;
	if (image.aspect <= 0.f && startThorVG())
	{
		if (tvg::Picture *picture = loadPicture(name, image))
		{
			float width = 0.f;
			float height = 0.f;
			if (picture->size(&width, &height) == tvg::Result::Success && width > 0.f && height > 0.f)
			{
				image.aspect = width / height;
			}
			tvg::Paint::rel(picture);
		}
	}
	if (image.aspect <= 0.f)
	{
		return nonstd::nullopt;
	}
	return image.aspect;
}

/// The image rasterized for a logical size and halo, cached (nullptr if it can't be)
const Rasterization *rasterizationFor(const std::string &name, float width, float height, float haloWidth)
{
	auto it = images.find(name);
	if (it == images.end() || it->second.unusable)
	{
		return nullptr;
	}
	VectorImage &image = it->second;
	const int pixelWidth = static_cast<int>(std::lround(width * scaleX));
	const int pixelHeight = static_cast<int>(std::lround(height * scaleY));
	if (pixelWidth < 1 || pixelHeight < 1 || pixelWidth > VECTOR_IMAGE_MAX_PIXELS || pixelHeight > VECTOR_IMAGE_MAX_PIXELS)
	{
		return nullptr;
	}
	const float haloPixels = std::min(std::max(haloWidth, 0.f) * std::min(scaleX, scaleY), MAX_HALO_PIXELS);
	const RasterKey key{pixelWidth, pixelHeight, static_cast<int>(std::lround(haloPixels * 16.f))};
	auto found = image.rasterizations.find(key);
	if (found == image.rasterizations.end())
	{
		if (image.rasterizations.size() >= MAX_RASTERIZATIONS_PER_IMAGE)
		{
			image.rasterizations.clear();
		}
		Rasterization rasterization = rasterize(name, image, pixelWidth, pixelHeight, haloPixels);
		if (rasterization.texture == nullptr)
		{
			return nullptr;
		}
		found = image.rasterizations.emplace(key, std::move(rasterization)).first;
	}
	return &found->second;
}

bool iV_DrawVectorImage(const std::string &name, float x, float y, float width, float height, PIELIGHT colour, float haloWidth)
{
	const Rasterization *rasterization = rasterizationFor(name, width, height, haloWidth);
	if (rasterization == nullptr)
	{
		return false;
	}
	// The top-left corner on a whole physical pixel, so each texel covers exactly one
	const float left = (std::round(x * scaleX) - static_cast<float>(rasterization->padding)) / scaleX;
	const float top = (std::round(y * scaleY) - static_cast<float>(rasterization->padding)) / scaleY;
	iV_DrawTextureTinted(*rasterization->texture, Vector2f(left, top),
	                     Vector2f(static_cast<float>(rasterization->width) / scaleX, static_cast<float>(rasterization->height) / scaleY), colour);
	return true;
}

bool iV_DrawVectorImageTransformed(const std::string &name, const glm::mat4 &modelViewProjection, float x, float y, float width, float height, PIELIGHT colour, float haloWidth)
{
	const Rasterization *rasterization = rasterizationFor(name, width, height, haloWidth);
	if (rasterization == nullptr)
	{
		return false;
	}
	const float padding = static_cast<float>(rasterization->padding);
	iV_DrawTextureTinted(*rasterization->texture, Vector2f(x - padding / scaleX, y - padding / scaleY),
	                     Vector2f(static_cast<float>(rasterization->width) / scaleX, static_cast<float>(rasterization->height) / scaleY), colour, modelViewProjection);
	return true;
}

void iV_VectorImagesUpdateScaleFactor(unsigned int horizScalePercentage, unsigned int vertScalePercentage)
{
	const float newScaleX = std::max(1u, horizScalePercentage) / 100.f;
	const float newScaleY = std::max(1u, vertScalePercentage) / 100.f;
	if (newScaleX != scaleX || newScaleY != scaleY)
	{
		scaleX = newScaleX;
		scaleY = newScaleY;
		dropRasterizations();
	}
}

void iV_VectorImagesShutdown()
{
	dropRasterizations();
	if (thorvgReady)
	{
		tvg::Initializer::term();
		thorvgReady = false;
	}
}
