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
/** @file screenshot_readback.cpp
 * Swapchain screenshot readback integrated with the screen-frame submit path.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "screenshot_readback.h"

#include "gfx_api_vk.h"
#include "lib/framework/wzapp.h"

namespace gfx_api::vk
{

namespace
{

uint32_t findHostVisibleCoherentMemoryType(const ::vk::PhysicalDeviceMemoryProperties& memprops,
	uint32_t memoryTypeBits)
{
	const auto required = static_cast<::vk::MemoryPropertyFlags>(
		::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent);
	for (uint32_t i = 0; i < memprops.memoryTypeCount; ++i)
	{
		if ((memoryTypeBits & (1u << i)) != 0
			&& (memprops.memoryTypes[i].propertyFlags & required) == required)
		{
			return i;
		}
	}
	return UINT32_MAX;
}

enum class SwapchainPixelLayout
{
	B8G8R8A8,
	R8G8B8A8,
	A2B10G10R10,
	A2R10G10B10,
};

bool swapchainFormatLayout(::vk::Format format, SwapchainPixelLayout& layout)
{
	switch (format)
	{
	case ::vk::Format::eB8G8R8A8Unorm:
	case ::vk::Format::eB8G8R8A8Srgb:
		layout = SwapchainPixelLayout::B8G8R8A8;
		return true;
	case ::vk::Format::eR8G8B8A8Unorm:
	case ::vk::Format::eR8G8B8A8Srgb:
		layout = SwapchainPixelLayout::R8G8B8A8;
		return true;
	case ::vk::Format::eA2B10G10R10UnormPack32:
		layout = SwapchainPixelLayout::A2B10G10R10;
		return true;
	case ::vk::Format::eA2R10G10B10UnormPack32:
		layout = SwapchainPixelLayout::A2R10G10B10;
		return true;
	default:
		return false;
	}
}

/// Map one UNORM10 sample (unsigned 10-bit normalized) to 8-bit for screenshot output.
///
/// Vulkan UNORM10 stores integers in [0, 1023] (0x3FF), interpreted as linear [0.0, 1.0].
/// Screenshot `iV_Image` output is 8-bit RGB, so each channel is rescaled to [0, 255].
///
/// Ideal conversion: round(sample / 1023 * 255). Integer form with round-to-nearest:
///   (sample * 255 + 1023/2) / 1023
/// The +511 bias (half of 1023) avoids truncating toward zero on every value.
constexpr uint8_t unorm10To8(uint32_t unorm10Sample)
{
	constexpr uint32_t UNORM10_MAX = 1023u; // 10-bit full scale (2^10 - 1)
	constexpr uint32_t UNORM8_MAX = 255u;
	constexpr uint32_t ROUND_BIAS = UNORM10_MAX / 2u;

	const uint32_t clamped = unorm10Sample & UNORM10_MAX;
	return static_cast<uint8_t>((clamped * UNORM8_MAX + ROUND_BIAS) / UNORM10_MAX);
}

void decodeSwapchainPixel(const uint8_t* srcPixel, SwapchainPixelLayout layout, uint8_t dstRgb[3])
{
	switch (layout)
	{
	case SwapchainPixelLayout::B8G8R8A8:
		dstRgb[0] = srcPixel[2];
		dstRgb[1] = srcPixel[1];
		dstRgb[2] = srcPixel[0];
		break;
	case SwapchainPixelLayout::R8G8B8A8:
		dstRgb[0] = srcPixel[0];
		dstRgb[1] = srcPixel[1];
		dstRgb[2] = srcPixel[2];
		break;
	case SwapchainPixelLayout::A2B10G10R10:
	{
		const uint32_t pixel = *reinterpret_cast<const uint32_t*>(srcPixel);
		dstRgb[0] = unorm10To8(pixel >> 0);
		dstRgb[1] = unorm10To8(pixel >> 10);
		dstRgb[2] = unorm10To8(pixel >> 20);
		break;
	}
	case SwapchainPixelLayout::A2R10G10B10:
	{
		const uint32_t pixel = *reinterpret_cast<const uint32_t*>(srcPixel);
		dstRgb[0] = unorm10To8(pixel >> 20);
		dstRgb[1] = unorm10To8(pixel >> 10);
		dstRgb[2] = unorm10To8(pixel >> 0);
		break;
	}
	}
}

} // namespace

bool ScreenshotStagingBuffer::create(::vk::Device device,
	const ::vk::PhysicalDeviceMemoryProperties& memprops, uint32_t width, uint32_t height,
	uint32_t bytesPerPixel, ::vk::DeviceSize bufferImageGranularity,
	const WZ_vk::DispatchLoaderDynamic& loader)
{
	destroy(loader);

	const ::vk::DeviceSize tightRowBytes = static_cast<::vk::DeviceSize>(width) * bytesPerPixel;
	rowPitch = ((tightRowBytes + bufferImageGranularity - 1) / bufferImageGranularity) * bufferImageGranularity;
	size = rowPitch * height;

	const ::vk::BufferCreateInfo bufferInfo({}, size, ::vk::BufferUsageFlagBits::eTransferDst,
		::vk::SharingMode::eExclusive);
	buffer = device.createBuffer(bufferInfo, nullptr, loader);

	const ::vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer, loader);
	const uint32_t memoryTypeIndex = findHostVisibleCoherentMemoryType(memprops, memRequirements.memoryTypeBits);
	if (memoryTypeIndex == UINT32_MAX)
	{
		destroy(loader);
		return false;
	}

	const ::vk::MemoryAllocateInfo allocInfo(memRequirements.size, memoryTypeIndex);
	memory = device.allocateMemory(allocInfo, nullptr, loader);
	device.bindBufferMemory(buffer, memory, 0, loader);
	dev = device;
	return true;
}

void ScreenshotStagingBuffer::destroy(const WZ_vk::DispatchLoaderDynamic& loader)
{
	if (!dev)
	{
		buffer = ::vk::Buffer{};
		memory = ::vk::DeviceMemory{};
		size = 0;
		rowPitch = 0;
		return;
	}

	if (memory)
	{
		dev.freeMemory(memory, nullptr, loader);
	}
	if (buffer)
	{
		dev.destroyBuffer(buffer, nullptr, loader);
	}

	dev = ::vk::Device{};
	buffer = ::vk::Buffer{};
	memory = ::vk::DeviceMemory{};
	size = 0;
	rowPitch = 0;
}

void* ScreenshotStagingBuffer::map(const WZ_vk::DispatchLoaderDynamic& loader) const
{
	if (!dev || !memory)
	{
		return nullptr;
	}
	return dev.mapMemory(memory, 0, size, {}, loader);
}

void ScreenshotStagingBuffer::unmap(const WZ_vk::DispatchLoaderDynamic& loader) const
{
	if (dev && memory)
	{
		dev.unmapMemory(memory, loader);
	}
}

bool ScreenshotReadback::isSupportedSwapchainFormat(::vk::Format format)
{
	SwapchainPixelLayout layout{};
	return swapchainFormatLayout(format, layout);
}

size_t ScreenshotReadback::totalPendingCount() const
{
	size_t count = _submitted.size();
	if (_awaitingRecord.has_value())
	{
		++count;
	}
	if (_pendingThisSubmit.has_value())
	{
		++count;
	}
	return count;
}

void ScreenshotReadback::destroyCaptureResources(PendingCapture& capture,
	const WZ_vk::DispatchLoaderDynamic& loader)
{
	capture.staging.destroy(loader);
	capture.callback = {};
	capture.gpuRecorded = false;
	capture.gpuSubmitted = false;
}

bool ScreenshotReadback::requestCapture(std::function<void(std::unique_ptr<iV_Image>)> callback,
	::vk::Device dev, const ::vk::PhysicalDeviceMemoryProperties& memprops,
	::vk::DeviceSize bufferImageGranularity, ::vk::Extent2D extent, ::vk::Format srcFormat,
	uint32_t swapchainImageIndex, uint32_t ringSlot, const WZ_vk::DispatchLoaderDynamic& loader)
{
	if (!callback)
	{
		return false;
	}
	if (extent.width == 0 || extent.height == 0)
	{
		return false;
	}
	if (!isSupportedSwapchainFormat(srcFormat))
	{
		debug(LOG_ERROR, "ScreenshotReadback: unsupported swapchain format %s",
		      ::vk::to_string(srcFormat).c_str());
		return false;
	}
	if (totalPendingCount() >= MAX_PENDING)
	{
		debug(LOG_WARNING, "ScreenshotReadback: too many pending captures");
		return false;
	}
	if (_awaitingRecord.has_value())
	{
		debug(LOG_WARNING, "ScreenshotReadback: capture already queued this frame");
		return false;
	}

	PendingCapture capture;
	capture.extent = extent;
	capture.srcFormat = srcFormat;
	capture.swapchainImageIndex = swapchainImageIndex;
	capture.ringSlot = ringSlot;
	capture.callback = std::move(callback);

	if (!capture.staging.create(dev, memprops, extent.width, extent.height, 4, bufferImageGranularity, loader))
	{
		debug(LOG_ERROR, "ScreenshotReadback: failed to allocate staging buffer");
		destroyCaptureResources(capture, loader);
		return false;
	}

	_awaitingRecord = std::move(capture);
	return true;
}

void ScreenshotReadback::recordCopy(VkRoot& root, ::vk::CommandBuffer drawCmd,
	gfx_api::abstract_texture* swapchainColor, ::vk::Image swapchainImage)
{
	if (!_awaitingRecord.has_value())
	{
		return;
	}

	PendingCapture capture = std::move(_awaitingRecord.value());
	_awaitingRecord.reset();

	::vk::ImageLayout oldLayout = root.getImageLayout(swapchainColor);
	if (oldLayout == ::vk::ImageLayout::eUndefined || oldLayout == ::vk::ImageLayout::ePresentSrcKHR)
	{
		oldLayout = ::vk::ImageLayout::eColorAttachmentOptimal;
	}

	root.transitionImageLayout(drawCmd, swapchainColor,
		oldLayout, ::vk::ImageLayout::eTransferSrcOptimal,
		::vk::PipelineStageFlagBits::eColorAttachmentOutput,
		::vk::PipelineStageFlagBits::eTransfer,
		::vk::AccessFlagBits::eColorAttachmentWrite,
		::vk::AccessFlagBits::eTransferRead);

	::vk::BufferImageCopy region{};
	region.imageSubresource.aspectMask = ::vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = ::vk::Extent3D(capture.extent.width, capture.extent.height, 1);
	region.bufferRowLength = static_cast<uint32_t>(capture.staging.rowPitch / 4);
	region.bufferImageHeight = capture.extent.height;

	drawCmd.copyImageToBuffer(swapchainImage, ::vk::ImageLayout::eTransferSrcOptimal,
		capture.staging.buffer, 1, &region, root.vkDynLoader);

	root.transitionImageLayout(drawCmd, swapchainColor,
		::vk::ImageLayout::eTransferSrcOptimal, ::vk::ImageLayout::ePresentSrcKHR,
		::vk::PipelineStageFlagBits::eTransfer,
		::vk::PipelineStageFlagBits::eBottomOfPipe,
		::vk::AccessFlagBits::eTransferRead,
		::vk::AccessFlags{});

	capture.gpuRecorded = true;
	_pendingThisSubmit = std::move(capture);
}

void ScreenshotReadback::markCurrentCaptureSubmitted(uint32_t ringSlot,
	const WZ_vk::DispatchLoaderDynamic& loader)
{
	if (!_pendingThisSubmit.has_value())
	{
		return;
	}

	PendingCapture capture = std::move(_pendingThisSubmit.value());
	_pendingThisSubmit.reset();

	if (!capture.gpuRecorded)
	{
		destroyCaptureResources(capture, loader);
		return;
	}

	capture.ringSlot = ringSlot;
	capture.gpuSubmitted = true;
	_submitted.push_back(std::move(capture));
}

bool ScreenshotReadback::buildCpuImage(const PendingCapture& capture, const uint8_t* src,
	std::unique_ptr<iV_Image>& outImage)
{
	SwapchainPixelLayout layout{};
	if (!swapchainFormatLayout(capture.srcFormat, layout))
	{
		return false;
	}

	outImage = std::make_unique<iV_Image>();
	if (!outImage->allocate(capture.extent.width, capture.extent.height, 3, false))
	{
		outImage.reset();
		return false;
	}

	const uint32_t width = capture.extent.width;
	const uint32_t height = capture.extent.height;
	constexpr uint32_t bytesPerPixel = 4;

	for (uint32_t y = 0; y < height; ++y)
	{
		// The swapchain image is flipped vertically, so we need to flip it back.
		const uint32_t srcY = height - 1 - y;
		const uint8_t* srcRow = src + srcY * capture.staging.rowPitch;
		uint8_t* dstRow = outImage->bmp_w() + static_cast<size_t>(y) * width * 3;

		for (uint32_t x = 0; x < width; ++x)
		{
			uint8_t rgb[3] = {};
			decodeSwapchainPixel(srcRow + x * bytesPerPixel, layout, rgb);
			uint8_t* dstPixel = dstRow + x * 3;
			dstPixel[0] = rgb[0];
			dstPixel[1] = rgb[1];
			dstPixel[2] = rgb[2];
		}
	}

	return true;
}

void ScreenshotReadback::completeReadyForRingSlot(::vk::Device dev, uint32_t ringSlot,
	const WZ_vk::DispatchLoaderDynamic& loader)
{
	std::vector<std::pair<std::function<void(std::unique_ptr<iV_Image>)>, std::unique_ptr<iV_Image>>> completed;

	for (auto it = _submitted.begin(); it != _submitted.end(); )
	{
		PendingCapture& capture = *it;
		if (!capture.gpuSubmitted || capture.ringSlot != ringSlot)
		{
			++it;
			continue;
		}

		void* mapped = capture.staging.map(loader);
		std::unique_ptr<iV_Image> image;
		if (mapped && buildCpuImage(capture, static_cast<const uint8_t*>(mapped), image))
		{
			completed.emplace_back(std::move(capture.callback), std::move(image));
		}
		else
		{
			debug(LOG_ERROR, "ScreenshotReadback: CPU image build failed for ring slot %u", ringSlot);
		}
		capture.staging.unmap(loader);
		destroyCaptureResources(capture, loader);
		it = _submitted.erase(it);
	}

	for (auto& entry : completed)
	{
		if (entry.first)
		{
			entry.first(std::move(entry.second));
		}
	}
}

void ScreenshotReadback::cancelPendingSubmit(const WZ_vk::DispatchLoaderDynamic& loader)
{
	if (!_pendingThisSubmit.has_value())
	{
		return;
	}
	PendingCapture capture = std::move(_pendingThisSubmit.value());
	_pendingThisSubmit.reset();
	destroyCaptureResources(capture, loader);
}

void ScreenshotReadback::cancelAwaitingRecord(const WZ_vk::DispatchLoaderDynamic& loader)
{
	if (!_awaitingRecord.has_value())
	{
		return;
	}
	PendingCapture capture = std::move(_awaitingRecord.value());
	_awaitingRecord.reset();
	destroyCaptureResources(capture, loader);
}

void ScreenshotReadback::cancelAll(const WZ_vk::DispatchLoaderDynamic& loader)
{
	cancelAwaitingRecord(loader);

	if (_pendingThisSubmit.has_value())
	{
		PendingCapture capture = std::move(_pendingThisSubmit.value());
		_pendingThisSubmit.reset();
		destroyCaptureResources(capture, loader);
	}

	while (!_submitted.empty())
	{
		PendingCapture capture = std::move(_submitted.front());
		_submitted.pop_front();
		destroyCaptureResources(capture, loader);
	}
}

void ScreenshotReadback::shutdown(const WZ_vk::DispatchLoaderDynamic& loader)
{
	cancelAll(loader);
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
