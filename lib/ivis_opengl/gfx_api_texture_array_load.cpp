// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file gfx_api_texture_array_load.cpp
 * C++20 coroutine-based texture array loading from files.
 */

#include "gfx_api.h"
#include "gfx_api_image_basis_priv.h"
#include "gfx_api_image_compress_priv.h"
#include "gfx_api_mipmap_priv.h"

#include "lib/framework/loading_task.h"
#include "lib/framework/resource_loading_controller.h"
#include "lib/framework/wzapp.h"
#include "lib/sound/audio.h"

#include <algorithm>

namespace
{

void pumpBlockingResourceLoadWithoutUI(const ResourceLoadingController::FramePolicy& /*policy*/)
{
	wzPumpEventsWhileLoading();
	audio_Update();
}

std::vector<std::unique_ptr<iV_BaseImage>> loadUncompressedImageWithMips(const std::string& imageLoadFilename, gfx_api::texture_type textureType, int maxWidth, int maxHeight, bool forceRGBA8)
{
	std::vector<std::unique_ptr<iV_BaseImage>> results;
#if defined(BASIS_ENABLED)
	if (strEndsWith(imageLoadFilename, ".ktx2"))
	{
		results = gfx_api::loadiVImagesFromFile_Basis(imageLoadFilename, textureType, gfx_api::pixel_format_target::texture_2d_array, (forceRGBA8) ? WZ_BASIS_UNCOMPRESSED_FORMAT : gfx_api::context::get().bestUncompressedPixelFormat(gfx_api::pixel_format_target::texture_2d_array, textureType), std::max(0, maxWidth), std::max(0, maxHeight));

		if (forceRGBA8)
		{
			for (auto& image : results)
			{
				auto pLoadedUncompressedImage = dynamic_cast<iV_Image*>(image.get());
				if (!pLoadedUncompressedImage)
				{
					debug(LOG_ERROR, "Loaded image is not an uncompressed format?: %s", imageLoadFilename.c_str());
					continue;
				}
				pLoadedUncompressedImage->convert_to_rgba();
			}
		}

		return results;
	}
	else
#endif
	if (strEndsWith(imageLoadFilename, ".png"))
	{
		auto pCurrentImage = gfx_api::loadUncompressedImageFromFile(imageLoadFilename.c_str(), gfx_api::pixel_format_target::texture_2d_array, textureType, maxWidth, maxHeight, forceRGBA8);
		if (!pCurrentImage)
		{
			debug(LOG_ERROR, "Unable to load image file: %s", imageLoadFilename.c_str());
			return {};
		}
		size_t mipmap_levels = calcMipmapLevelsForUncompressedImage(*pCurrentImage, textureType);
		auto miplevels = generateMipMapsFromUncompressedImage(*pCurrentImage, mipmap_levels, textureType);
		results.push_back(std::move(pCurrentImage));
		results.insert(results.end(), std::make_move_iterator(miplevels.begin()), std::make_move_iterator(miplevels.end()));
		miplevels.clear();
	}
	else
	{
		debug(LOG_ERROR, "Unable to load image file: %s", imageLoadFilename.c_str());
		return {};
	}
	return results;
}

struct TextureArrayLoadContext
{
	const std::vector<WzString>& imageLoadFilenames;
	gfx_api::texture_type textureType;
	int maxWidth;
	int maxHeight;
	gfx_api::context::GenerateDefaultTextureFunc defaultTextureGenerator;
	const std::string& debugName;

	gfx_api::pixel_format uploadFormat = gfx_api::pixel_format::invalid;
	gfx_api::pixel_format desiredImageExtractionFormat = gfx_api::pixel_format::invalid;
	bool uncompressedExtractionFormat = false;
	std::vector<std::unique_ptr<iV_BaseImage>> defaultTextureMips = {};

	std::unique_ptr<gfx_api::texture_array> texture_array = nullptr;
	unsigned int width = 0;
	unsigned int height = 0;
	size_t mipmap_levels = 0;
};

std::vector<std::unique_ptr<iV_BaseImage>>* getDefaultTextureMipsP(TextureArrayLoadContext& ctx, size_t layer, unsigned int width, unsigned int height, size_t mipmap_levels, gfx_api::pixel_format format)
{
	if (ctx.defaultTextureMips.empty())
	{
		ASSERT_OR_RETURN(nullptr, ctx.defaultTextureGenerator != nullptr, "Failed to generate default texture - no default texture generator provided");
		ASSERT_OR_RETURN(nullptr, gfx_api::is_uncompressed_format(format), "Expected uncompressed extraction format, but received: %s", gfx_api::format_to_str(format));
		if (ctx.defaultTextureGenerator)
		{
			auto pCurrentImage = ctx.defaultTextureGenerator((layer > 0) ? width : ctx.maxWidth, (layer > 0) ? height : ctx.maxHeight, gfx_api::format_channels(format));
			ASSERT_OR_RETURN(nullptr, pCurrentImage != nullptr, "Failed to generate default texture");
			if (layer > 0)
			{
				ASSERT_OR_RETURN(nullptr, pCurrentImage->width() == width && pCurrentImage->height() == height, "Failed to generate matching default texture");
			}
			size_t expectedMipLevels = calcMipmapLevelsForUncompressedImage(*pCurrentImage, ctx.textureType);
			if (layer > 0)
			{
				ASSERT_OR_RETURN(nullptr, expectedMipLevels == mipmap_levels, "Failed to generate matching default texture");
			}
			auto miplevels = generateMipMapsFromUncompressedImage(*pCurrentImage, expectedMipLevels, ctx.textureType);
			ctx.defaultTextureMips.push_back(std::move(pCurrentImage));
			ctx.defaultTextureMips.insert(ctx.defaultTextureMips.end(), std::make_move_iterator(miplevels.begin()), std::make_move_iterator(miplevels.end()));
			miplevels.clear();
		}
	}
	return &ctx.defaultTextureMips;
}

bool loadTextureArrayLayer(TextureArrayLoadContext& ctx, size_t layer)
{
	const WzString& imageLoadFilename = ctx.imageLoadFilenames[layer];

	std::vector<std::unique_ptr<iV_BaseImage>> loadedImagesForLayer;
	std::vector<std::unique_ptr<iV_BaseImage>>* pImagesForLayer = nullptr;
	if (imageLoadFilename.isEmpty())
	{
		pImagesForLayer = getDefaultTextureMipsP(ctx, layer, ctx.width, ctx.height, ctx.mipmap_levels, ctx.desiredImageExtractionFormat);
		ASSERT_OR_RETURN(false, pImagesForLayer != nullptr, "Failed to generate matching default texture");
	}
	else if (ctx.uncompressedExtractionFormat || imageLoadFilename.endsWith(".png"))
	{
		loadedImagesForLayer = loadUncompressedImageWithMips(imageLoadFilename.toUtf8(), ctx.textureType, ctx.maxWidth, ctx.maxHeight, ctx.desiredImageExtractionFormat == gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8);
		if (!loadedImagesForLayer.empty())
		{
			pImagesForLayer = &loadedImagesForLayer;
		}
		else
		{
			debug(LOG_INFO, "Using default texture generator for failed image: %s", imageLoadFilename.toUtf8().c_str());
			pImagesForLayer = getDefaultTextureMipsP(ctx, layer, ctx.width, ctx.height, ctx.mipmap_levels, ctx.desiredImageExtractionFormat);
		}
	}
	else
	{
#if defined(BASIS_ENABLED)
		if (imageLoadFilename.endsWith(".ktx2"))
		{
			loadedImagesForLayer = gfx_api::loadiVImagesFromFile_Basis(imageLoadFilename.toUtf8(), ctx.textureType, gfx_api::pixel_format_target::texture_2d_array, ctx.desiredImageExtractionFormat, std::max(0, ctx.maxWidth), std::max(0, ctx.maxHeight));
			pImagesForLayer = &loadedImagesForLayer;
		}
		else
#endif
		{
			debug(LOG_ERROR, "Unable to load image file: %s", imageLoadFilename.toUtf8().c_str());
			return false;
		}
	}

	ASSERT_OR_RETURN(false, pImagesForLayer && !pImagesForLayer->empty(), "Unable to load images: %s", imageLoadFilename.toUtf8().c_str());

	if (layer == 0)
	{
		ctx.width = pImagesForLayer->front()->width();
		ctx.height = pImagesForLayer->front()->height();
		ctx.mipmap_levels = pImagesForLayer->size();

		ctx.texture_array = std::unique_ptr<gfx_api::texture_array>(gfx_api::context::get().create_texture_array(ctx.mipmap_levels, ctx.imageLoadFilenames.size(), ctx.width, ctx.height, ctx.uploadFormat, ctx.debugName));
		ASSERT_OR_RETURN(false, ctx.texture_array.get() != nullptr, "Failed to create texture array (miplevels: %zu, layers: %zu, width: %u, height: %u): %s", ctx.mipmap_levels, ctx.imageLoadFilenames.size(), ctx.width, ctx.height, imageLoadFilename.toUtf8().c_str());
	}
	else
	{
		ASSERT_OR_RETURN(false, ctx.width == pImagesForLayer->front()->width() && ctx.height == pImagesForLayer->front()->height(), "Unexpected image dimensions (%u x %u) does not match the first image dimensions (%u x %u): %s", pImagesForLayer->front()->width(), pImagesForLayer->front()->height(), ctx.width, ctx.height, imageLoadFilename.toUtf8().c_str());
		ASSERT_OR_RETURN(false, pImagesForLayer->size() == ctx.mipmap_levels, "Unexpected number of mip levels (%zu; expected: %zu): %s", pImagesForLayer->size(), ctx.mipmap_levels, imageLoadFilename.toUtf8().c_str());
	}

	if (ctx.uploadFormat == ctx.desiredImageExtractionFormat)
	{
		bool uploadSuccess = gfx_api::context::get().loadTextureArrayLayerFromBaseImages(*ctx.texture_array, layer, *pImagesForLayer, imageLoadFilename.toUtf8(), ctx.width, ctx.height);
		ASSERT_OR_RETURN(false, uploadSuccess, "Failed to loadTextureArrayLayerFromBaseImages");
	}
	else
	{
		ASSERT_OR_RETURN(false, ctx.uncompressedExtractionFormat, "Expected uncompressed extraction format, but received: %s", gfx_api::format_to_str(ctx.desiredImageExtractionFormat));

		for (size_t level = 0; level < pImagesForLayer->size(); ++level)
		{
			const iV_Image* image = dynamic_cast<iV_Image*>(pImagesForLayer->at(level).get());
			ASSERT_OR_RETURN(false, image != nullptr, "Image wasn't an iV_Image?");
			auto compressedImage = gfx_api::compressImage(*image, ctx.uploadFormat);
			ASSERT_OR_RETURN(false, compressedImage != nullptr, "Failed to compress image to format: %zu", static_cast<size_t>(ctx.uploadFormat));
			bool uploadResult = ctx.texture_array->upload_layer(layer, level, *compressedImage);
			ASSERT_OR_RETURN(false, uploadResult, "Failed to upload buffer to image");
		}
	}

	return true;
}

bool prepareTextureArrayLoadContext(TextureArrayLoadContext& ctx)
{
	ASSERT_OR_RETURN(false, ctx.imageLoadFilenames.size() <= gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_ARRAY_TEXTURE_LAYERS), "Too many layers");

	bool allFilesAreKTX2 = std::all_of(ctx.imageLoadFilenames.cbegin(), ctx.imageLoadFilenames.cend(), [](const WzString& imageLoadFilename) { return imageLoadFilename.endsWith(".ktx2"); });
	bool anyFileIsKTX2 = allFilesAreKTX2 || std::any_of(ctx.imageLoadFilenames.cbegin(), ctx.imageLoadFilenames.cend(), [](const WzString& imageLoadFilename) { return imageLoadFilename.endsWith(".ktx2"); });
	bool anyFileIsPNG = !allFilesAreKTX2 && std::any_of(ctx.imageLoadFilenames.cbegin(), ctx.imageLoadFilenames.cend(), [](const WzString& imageLoadFilename) { return imageLoadFilename.endsWith(".png"); });

#if !defined(BASIS_ENABLED)
	if (anyFileIsKTX2)
	{
		for (auto& imageLoadFilename : ctx.imageLoadFilenames)
		{
			const std::string& filename = imageLoadFilename.toUtf8();
			if (strEndsWith(filename, ".ktx2"))
			{
				debug(LOG_ERROR, "Unable to load image file: %s", filename.c_str());
			}
		}
		return false;
	}
#endif

	ctx.uploadFormat = gfx_api::context::get().bestUncompressedPixelFormat(gfx_api::pixel_format_target::texture_2d_array, ctx.textureType);
	ctx.desiredImageExtractionFormat = ctx.uploadFormat;
#if defined(BASIS_ENABLED)
	if (allFilesAreKTX2)
	{
		auto bestBasisFormat = gfx_api::getBestAvailableTranscodeFormatForBasis(gfx_api::pixel_format_target::texture_2d_array, ctx.textureType);
		if (bestBasisFormat.has_value())
		{
			ctx.uploadFormat = bestBasisFormat.value();
			ctx.desiredImageExtractionFormat = ctx.uploadFormat;
		}
	}
	else
#endif
	{
		auto bestAvailableCompressedFormat = gfx_api::bestRealTimeCompressionFormat(gfx_api::pixel_format_target::texture_2d_array, ctx.textureType);
		if (bestAvailableCompressedFormat.has_value() && bestAvailableCompressedFormat.value() != gfx_api::pixel_format::invalid)
		{
			ctx.uploadFormat = bestAvailableCompressedFormat.value();
			ctx.desiredImageExtractionFormat = gfx_api::pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		}
		if (anyFileIsKTX2 && anyFileIsPNG)
		{
			debug(LOG_INFO, "Performance info: Some files are .ktx2, but others are .png");
		}
	}

	ctx.uncompressedExtractionFormat = gfx_api::is_uncompressed_format(ctx.desiredImageExtractionFormat);
	return true;
}

gfx_api::texture_array* finishTextureArrayLoad(TextureArrayLoadContext& ctx)
{
	if (ctx.texture_array)
	{
		ctx.texture_array->flush();
	}
	return ctx.texture_array.release();
}

std::vector<WzString> imageLoadFilenamesFromInput(const std::vector<WzString>& filenames)
{
	std::vector<WzString> imageLoadFilenames;
	imageLoadFilenames.reserve(filenames.size());
	std::transform(filenames.cbegin(), filenames.cend(), std::back_inserter(imageLoadFilenames), [](const WzString& filename) {
		return gfx_api::imageLoadFilenameFromInputFilename(filename);
	});
	return imageLoadFilenames;
}

gfx_api::texture_array* loadTextureArrayFromFilesNoYield(
	const std::vector<WzString>& filenames,
	gfx_api::texture_type textureType,
	int maxWidth,
	int maxHeight,
	const gfx_api::context::GenerateDefaultTextureFunc& defaultTextureGenerator,
	const std::string& debugName)
{
	auto imageLoadFilenames = imageLoadFilenamesFromInput(filenames);

	TextureArrayLoadContext ctx{
		imageLoadFilenames,
		textureType,
		maxWidth,
		maxHeight,
		defaultTextureGenerator,
		debugName,
	};

	if (!prepareTextureArrayLoadContext(ctx))
	{
		return nullptr;
	}

	for (size_t layer = 0; layer < imageLoadFilenames.size(); ++layer)
	{
		if (!loadTextureArrayLayer(ctx, layer))
		{
			return nullptr;
		}
	}

	return finishTextureArrayLoad(ctx);
}

} // namespace

LoadingTask<gfx_api::texture_array*> gfx_api::context::loadTextureArrayFromFiles(
	ResourceLoadingController& controller,
	const std::vector<WzString>& filenames,
	gfx_api::texture_type textureType,
	int maxWidth,
	int maxHeight,
	const GenerateDefaultTextureFunc& defaultTextureGenerator,
	const std::string& debugName)
{
	auto imageLoadFilenames = imageLoadFilenamesFromInput(filenames);

	TextureArrayLoadContext ctx{
		imageLoadFilenames,
		textureType,
		maxWidth,
		maxHeight,
		defaultTextureGenerator,
		debugName,
	};

	if (!prepareTextureArrayLoadContext(ctx))
	{
		co_return load_fail();
	}

	for (size_t layer = 0; layer < imageLoadFilenames.size(); ++layer)
	{
		if (!loadTextureArrayLayer(ctx, layer))
		{
			co_return load_fail();
		}
		co_await controller.yieldFrame();
	}

	co_return load_ok(finishTextureArrayLoad(ctx));
}

gfx_api::texture_array* gfx_api::context::loadTextureArrayFromFilesBlocking(
	const std::vector<WzString>& filenames,
	gfx_api::texture_type textureType,
	int maxWidth,
	int maxHeight,
	const GenerateDefaultTextureFunc& defaultTextureGenerator,
	const std::string& debugName)
{
	auto& controller = ResourceLoadingController::instance();
	if (controller.isExecutingLoadingCoroutine())
	{
		return loadTextureArrayFromFilesNoYield(filenames, textureType, maxWidth, maxHeight, defaultTextureGenerator, debugName);
	}

	return controller.runTaskToCompletion(
	    loadTextureArrayFromFiles(controller, filenames, textureType, maxWidth, maxHeight, defaultTextureGenerator, debugName),
	    ResourceLoadingController::FramePolicy{ResourceLoadingController::FrameProcessingMode::ConsumeFrame, false},
	    pumpBlockingResourceLoadWithoutUI).value_or(nullptr);
}
