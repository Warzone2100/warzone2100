/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

// Ray-traced (VK_KHR_ray_query) sun shadows - Vulkan backend implementation.
// See gfx_api_vk_rt.h for an overview.

#if defined(WZ_VULKAN_ENABLED)

#include "gfx_api_vk.h"
#include "gfx_api_vk_rt.h"
#include "lib/framework/frame.h"
#include "lib/framework/physfs_ext.h"

#include <cstring>
#include <algorithm>
#include <array>

#if defined(WZ_VK_RAY_QUERY_POSSIBLE)

#if VK_HEADER_VERSION >= 260
# define WZ_THROW_VK_RESULT_EXCEPTION_RT(result, message) vk::detail::throwResultException(result, message)
#else
# define WZ_THROW_VK_RESULT_EXCEPTION_RT(result, message) vk::throwResultException(result, message)
#endif

// MARK: helpers

namespace
{
	constexpr uint32_t WZ_RT_MIN_INSTANCE_CAPACITY = 512;

	struct TracePushConstants
	{
		glm::mat4 invProjectionView;
		glm::vec4 sunDirection;
		glm::vec4 cameraPosition;
		glm::vec4 params; // x = tMin, y = tMax, z = normalOffsetScale, w = sun cone angle
		glm::vec4 aoParams; // x = AO ray count, y = AO max distance, z = AO strength, w = unused
	};
	static_assert(sizeof(TracePushConstants) <= 128, "Push constants exceed minimum guaranteed size");

	struct BlurPushConstants
	{
		glm::vec4 dirAndInvSize; // xy = blur direction (in texels), zw = 1/maskExtent
		glm::vec4 params; // x = depth edge threshold scale, yzw = unused
	};

	std::vector<uint32_t> readSpvFile(const std::string& name)
	{
		auto fp = PHYSFS_openRead(name.c_str());
		ASSERT_OR_RETURN({}, fp != nullptr, "Could not open %s", name.c_str());

		const auto filesize = PHYSFS_fileLength(fp);
		if (filesize <= 0 || (filesize % sizeof(uint32_t)) != 0)
		{
			PHYSFS_close(fp);
			ASSERT(false, "Invalid spv file size (%lld): %s", (long long)filesize, name.c_str());
			return {};
		}

		auto buffer = std::vector<uint32_t>(static_cast<size_t>(filesize / sizeof(uint32_t)));
		WZ_PHYSFS_readBytes(fp, buffer.data(), static_cast<PHYSFS_uint32>(filesize));
		PHYSFS_close(fp);
		return buffer;
	}

	vk::TransformMatrixKHR toVkTransform(const glm::mat4& m)
	{
		// glm is column-major; VkTransformMatrixKHR is a 3x4 row-major matrix
		vk::TransformMatrixKHR result;
		for (uint32_t row = 0; row < 3; ++row)
		{
			for (uint32_t col = 0; col < 4; ++col)
			{
				result.matrix[row][col] = m[col][row];
			}
		}
		return result;
	}

	vk::DeviceAddress alignAddress(vk::DeviceAddress address, vk::DeviceSize alignment)
	{
		if (alignment == 0) { return address; }
		return (address + alignment - 1) & ~(alignment - 1);
	}
}

// MARK: construction / destruction

VkRayShadowBackend::VkRayShadowBackend(VkRoot& _root)
	: root(_root)
{
}

VkRayShadowBackend::~VkRayShadowBackend()
{
	// Resources are destroyed via destroyNow() (shutdown) or deferred destruction (setMode(Off)).
	// If neither has happened, attempt deferred destruction while the buffering mechanism is alive.
	if (pipelinesCreated)
	{
		destroyNow();
	}
}

// MARK: buffers

VkRayShadowBackend::RTBuffer VkRayShadowBackend::createRTBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage, bool mapped, const char* debugName)
{
	RTBuffer result;

	auto bufferCreateInfo = vk::BufferCreateInfo()
		.setSize(size)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memUsage;
	if (mapped)
	{
		allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationInfo allocInfo = {};
	VkResult vkResult = vmaCreateBuffer(root.allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&result.buffer), &result.allocation, &allocInfo);
	if (vkResult != VK_SUCCESS)
	{
		debug(LOG_ERROR, "vmaCreateBuffer(%s, %llu) failed: %s", debugName, (unsigned long long)size, to_string(static_cast<vk::Result>(vkResult)).c_str());
		return RTBuffer();
	}
	result.size = size;
	result.pMapped = (mapped) ? allocInfo.pMappedData : nullptr;

	if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
	{
		auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(result.buffer);
		result.address = root.dev.getBufferAddress(addressInfo, root.vkDynLoader);
	}

	if (root.debugUtilsExtEnabled && debugName)
	{
		vk::DebugUtilsObjectNameInfoEXT objectNameInfo;
		objectNameInfo.setObjectType(vk::ObjectType::eBuffer);
		objectNameInfo.setObjectHandle(uint64_t(static_cast<VkBuffer>(result.buffer)));
		objectNameInfo.setPObjectName(debugName);
		root.dev.setDebugUtilsObjectNameEXT(objectNameInfo, root.vkDynLoader);
	}

	return result;
}

void VkRayShadowBackend::destroyRTBufferNow(RTBuffer& buf)
{
	if (buf.buffer)
	{
		vmaDestroyBuffer(root.allocator, static_cast<VkBuffer>(buf.buffer), buf.allocation);
	}
	buf = RTBuffer();
}

void VkRayShadowBackend::destroyRTBufferDeferred(RTBuffer& buf)
{
	if (!buf.buffer)
	{
		buf = RTBuffer();
		return;
	}
	if (buffering_mechanism::isInitialized())
	{
		auto& frameResources = buffering_mechanism::get_current_resources();
		frameResources.buffer_to_delete.emplace_back(std::move(buf.buffer));
		frameResources.vmamemory_to_free.push_back(buf.allocation);
	}
	else
	{
		vmaDestroyBuffer(root.allocator, static_cast<VkBuffer>(buf.buffer), buf.allocation);
	}
	buf = RTBuffer();
}

void VkRayShadowBackend::destroyASDeferred(vk::AccelerationStructureKHR& as)
{
	if (!as)
	{
		return;
	}
	if (buffering_mechanism::isInitialized())
	{
		buffering_mechanism::get_current_resources().as_to_delete.push_back(as);
	}
	else
	{
		root.dev.destroyAccelerationStructureKHR(as, nullptr, root.vkDynLoader);
	}
	as = vk::AccelerationStructureKHR();
}

// MARK: BLAS

VkRayShadowBackend::BlasEntry VkRayShadowBackend::createBlas(const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount, bool keepGeometryBuffer, const char* debugName)
{
	BlasEntry entry;
	ASSERT_OR_RETURN(entry, vertexCount > 0 && indexCount >= 3, "Empty geometry (%s)", debugName);

	const vk::DeviceSize vertexBytes = vertexCount * sizeof(float) * 3;
	const vk::DeviceSize indexOffset = alignAddress(vertexBytes, 4);
	const vk::DeviceSize indexBytes = indexCount * sizeof(uint32_t);
	const uint32_t triangleCount = static_cast<uint32_t>(indexCount / 3);

	// geometry buffer (device-local), filled via the per-frame staging allocator + cmdCopy
	RTBuffer geometryBuffer = createRTBuffer(indexOffset + indexBytes,
											 vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst,
											 VMA_MEMORY_USAGE_GPU_ONLY, false, debugName);
	ASSERT_OR_RETURN(entry, geometryBuffer.buffer, "Failed to create BLAS geometry buffer (%s)", debugName);

	auto& frameResources = buffering_mechanism::get_current_resources();
	const auto cmdBuffer = frameResources.currentCopyCmdBuffer();
	{
		const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(static_cast<uint32_t>(vertexBytes), 4);
		auto* mappedMem = frameResources.stagingBufferAllocator.mapMemory(stagingMemory);
		memcpy(mappedMem, vertices, vertexBytes);
		frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);
		const auto copyRegions = std::array<vk::BufferCopy, 1> { vk::BufferCopy(stagingMemory.offset, 0, vertexBytes) };
		cmdBuffer->copyBuffer(stagingMemory.buffer, geometryBuffer.buffer, copyRegions, root.vkDynLoader);
	}
	{
		const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(static_cast<uint32_t>(indexBytes), 4);
		auto* mappedMem = frameResources.stagingBufferAllocator.mapMemory(stagingMemory);
		memcpy(mappedMem, indices, indexBytes);
		frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);
		const auto copyRegions = std::array<vk::BufferCopy, 1> { vk::BufferCopy(stagingMemory.offset, indexOffset, indexBytes) };
		cmdBuffer->copyBuffer(stagingMemory.buffer, geometryBuffer.buffer, copyRegions, root.vkDynLoader);
	}

	// query build sizes
	auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
		.setVertexFormat(vk::Format::eR32G32B32Sfloat)
		.setVertexData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(geometryBuffer.address))
		.setVertexStride(sizeof(float) * 3)
		.setMaxVertex(static_cast<uint32_t>(vertexCount - 1))
		.setIndexType(vk::IndexType::eUint32)
		.setIndexData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(geometryBuffer.address + indexOffset));
	auto geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eTriangles)
		.setFlags(vk::GeometryFlagBitsKHR::eOpaque)
		.setGeometry(vk::AccelerationStructureGeometryDataKHR().setTriangles(triangleData));
	auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
		.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setGeometryCount(1)
		.setPGeometries(&geometry);

	const auto buildSizes = root.dev.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, { triangleCount }, root.vkDynLoader);

	entry.asBuffer = createRTBuffer(buildSizes.accelerationStructureSize,
									vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
									VMA_MEMORY_USAGE_GPU_ONLY, false, debugName);
	if (!entry.asBuffer.buffer)
	{
		destroyRTBufferDeferred(geometryBuffer);
		return entry;
	}

	auto asCreateInfo = vk::AccelerationStructureCreateInfoKHR()
		.setBuffer(entry.asBuffer.buffer)
		.setOffset(0)
		.setSize(buildSizes.accelerationStructureSize)
		.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
	entry.as = root.dev.createAccelerationStructureKHR(asCreateInfo, nullptr, root.vkDynLoader);
	entry.address = root.dev.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(entry.as), root.vkDynLoader);

	PendingBlasBuild pending;
	pending.dst = entry.as;
	pending.geometryBuffer = geometryBuffer;
	pending.vertexAddress = geometryBuffer.address;
	pending.indexAddress = geometryBuffer.address + indexOffset;
	pending.vertexCount = static_cast<uint32_t>(vertexCount);
	pending.triangleCount = triangleCount;
	pending.scratchSize = buildSizes.buildScratchSize;
	pending.keepGeometryBuffer = keepGeometryBuffer;
	pendingBuilds.push_back(pending);

	return entry;
}

bool VkRayShadowBackend::hasMeshBLAS(const void* meshKey, int32_t variant) const
{
	return meshBlas.count(std::make_pair(meshKey, variant)) > 0;
}

void VkRayShadowBackend::registerMeshGeometry(const void* meshKey, int32_t variant, const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount)
{
	const auto key = std::make_pair(meshKey, variant);
	if (meshBlas.count(key) > 0)
	{
		return;
	}
	BlasEntry entry = createBlas(vertices, vertexCount, indices, indexCount, false, "<rt mesh blas>");
	if (!entry.as)
	{
		return;
	}
	meshBlas[key] = entry;
}

void VkRayShadowBackend::invalidateMesh(const void* meshKey)
{
	for (auto it = meshBlas.begin(); it != meshBlas.end(); )
	{
		if (it->first.first == meshKey)
		{
			// drop any not-yet-recorded build targeting this acceleration structure
			pendingBuilds.erase(std::remove_if(pendingBuilds.begin(), pendingBuilds.end(), [this, &it](PendingBlasBuild& b) {
				if (b.dst != it->second.as)
				{
					return false;
				}
				if (!b.keepGeometryBuffer)
				{
					destroyRTBufferDeferred(b.geometryBuffer);
				}
				return true;
			}), pendingBuilds.end());
			destroyASDeferred(it->second.as);
			destroyRTBufferDeferred(it->second.asBuffer);
			it = meshBlas.erase(it);
		}
		else
		{
			++it;
		}
	}
}

// MARK: terrain

void VkRayShadowBackend::setTerrainGeometry(const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount)
{
	clearTerrainGeometry();

	terrainBlas = createBlas(vertices, vertexCount, indices, indexCount, true, "<rt terrain blas>");
	if (!terrainBlas.as)
	{
		return;
	}
	// createBlas queued the initial build; keep the geometry buffer for later vertex updates + rebuilds
	terrainGeometryBuffer = pendingBuilds.back().geometryBuffer;
	terrainVertexCount = static_cast<uint32_t>(vertexCount);
	terrainTriangleCount = static_cast<uint32_t>(indexCount / 3);
	terrainScratchSize = pendingBuilds.back().scratchSize;
	haveTerrain = true;
	terrainBlasDirty = false; // the initial build is already queued
}

void VkRayShadowBackend::updateTerrainVertices(size_t startVertex, const float* vertices, size_t vertexCount)
{
	ASSERT_OR_RETURN(, haveTerrain, "No terrain geometry set");
	ASSERT_OR_RETURN(, startVertex + vertexCount <= terrainVertexCount, "Terrain vertex update out of range");

	const vk::DeviceSize updateBytes = vertexCount * sizeof(float) * 3;
	auto& frameResources = buffering_mechanism::get_current_resources();
	const auto cmdBuffer = frameResources.currentCopyCmdBuffer();
	const auto stagingMemory = frameResources.stagingBufferAllocator.alloc(static_cast<uint32_t>(updateBytes), 4);
	auto* mappedMem = frameResources.stagingBufferAllocator.mapMemory(stagingMemory);
	memcpy(mappedMem, vertices, updateBytes);
	frameResources.stagingBufferAllocator.unmapMemory(stagingMemory);
	const auto copyRegions = std::array<vk::BufferCopy, 1> { vk::BufferCopy(stagingMemory.offset, startVertex * sizeof(float) * 3, updateBytes) };
	cmdBuffer->copyBuffer(stagingMemory.buffer, terrainGeometryBuffer.buffer, copyRegions, root.vkDynLoader);

	terrainBlasDirty = true;
}

void VkRayShadowBackend::clearTerrainGeometry()
{
	if (!haveTerrain)
	{
		return;
	}
	// remove any pending build targeting the terrain BLAS
	pendingBuilds.erase(std::remove_if(pendingBuilds.begin(), pendingBuilds.end(), [this](const PendingBlasBuild& b) { return b.dst == terrainBlas.as; }), pendingBuilds.end());
	destroyASDeferred(terrainBlas.as);
	destroyRTBufferDeferred(terrainBlas.asBuffer);
	destroyRTBufferDeferred(terrainGeometryBuffer);
	terrainBlas = BlasEntry();
	terrainVertexCount = 0;
	terrainTriangleCount = 0;
	terrainScratchSize = 0;
	terrainBlasDirty = false;
	haveTerrain = false;
}

// MARK: pending builds

void VkRayShadowBackend::recordPendingBuilds(vk::CommandBuffer cmdBuffer)
{
	// terrain rebuild (vertex data changed in-place; counts are unchanged so sizes still hold)
	if (terrainBlasDirty && haveTerrain && terrainBlas.as)
	{
		const vk::DeviceSize vertexBytes = static_cast<vk::DeviceSize>(terrainVertexCount) * sizeof(float) * 3;
		PendingBlasBuild rebuild;
		rebuild.dst = terrainBlas.as;
		rebuild.geometryBuffer = RTBuffer(); // owned by terrainGeometryBuffer - do not free
		rebuild.vertexAddress = terrainGeometryBuffer.address;
		rebuild.indexAddress = terrainGeometryBuffer.address + alignAddress(vertexBytes, 4);
		rebuild.vertexCount = terrainVertexCount;
		rebuild.triangleCount = terrainTriangleCount;
		rebuild.scratchSize = terrainScratchSize;
		rebuild.keepGeometryBuffer = true;
		pendingBuilds.push_back(rebuild);
		terrainBlasDirty = false;
	}

	if (pendingBuilds.empty())
	{
		return;
	}

	// ensure staged geometry copies (recorded earlier in cmdCopy) are complete before building
	const auto preBuildBarrier = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							  vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
							  vk::DependencyFlags(), preBuildBarrier, nullptr, nullptr, root.vkDynLoader);

	for (auto& pending : pendingBuilds)
	{
		// transient scratch buffer (deferred-deleted once the frame completes)
		RTBuffer scratch = createRTBuffer(pending.scratchSize + 256,
										  vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
										  VMA_MEMORY_USAGE_GPU_ONLY, false, "<rt blas scratch>");
		if (!scratch.buffer)
		{
			continue;
		}

		auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR()
			.setVertexFormat(vk::Format::eR32G32B32Sfloat)
			.setVertexData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(pending.vertexAddress))
			.setVertexStride(sizeof(float) * 3)
			.setMaxVertex(pending.vertexCount - 1)
			.setIndexType(vk::IndexType::eUint32)
			.setIndexData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(pending.indexAddress));
		auto geometry = vk::AccelerationStructureGeometryKHR()
			.setGeometryType(vk::GeometryTypeKHR::eTriangles)
			.setFlags(vk::GeometryFlagBitsKHR::eOpaque)
			.setGeometry(vk::AccelerationStructureGeometryDataKHR().setTriangles(triangleData));
		auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
			.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
			.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
			.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
			.setDstAccelerationStructure(pending.dst)
			.setGeometryCount(1)
			.setPGeometries(&geometry)
			.setScratchData(vk::DeviceOrHostAddressKHR().setDeviceAddress(alignAddress(scratch.address, 256)));

		auto rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
			.setPrimitiveCount(pending.triangleCount)
			.setPrimitiveOffset(0)
			.setFirstVertex(0)
			.setTransformOffset(0);
		const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

		cmdBuffer.buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo, root.vkDynLoader);

		destroyRTBufferDeferred(scratch);
		if (!pending.keepGeometryBuffer)
		{
			destroyRTBufferDeferred(pending.geometryBuffer);
		}
	}
	pendingBuilds.clear();

	// BLAS builds must complete before the TLAS build reads them
	const auto postBuildBarrier = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
			.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
							  vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
							  vk::DependencyFlags(), postBuildBarrier, nullptr, nullptr, root.vkDynLoader);
}

// MARK: per-frame resources / TLAS

VkRayShadowBackend::PerFrame& VkRayShadowBackend::currentPerFrame()
{
	size_t frameIdx = buffering_mechanism::get_current_frame_num();
	if (frameIdx >= perFrame.size())
	{
		perFrame.resize(frameIdx + 1);
	}
	return perFrame[frameIdx];
}

void VkRayShadowBackend::buildTlas(vk::CommandBuffer cmdBuffer, PerFrame& pf)
{
	const uint32_t instanceCount = static_cast<uint32_t>(frameInstances.size());

	// (re)create instance buffer if needed
	const uint32_t neededCapacity = std::max(instanceCount, WZ_RT_MIN_INSTANCE_CAPACITY);
	if (pf.instanceCapacity < neededCapacity || !pf.instanceBuffer.buffer)
	{
		destroyRTBufferDeferred(pf.instanceBuffer);
		const uint32_t newCapacity = std::max(neededCapacity + neededCapacity / 2, WZ_RT_MIN_INSTANCE_CAPACITY);
		pf.instanceBuffer = createRTBuffer(static_cast<vk::DeviceSize>(newCapacity) * sizeof(vk::AccelerationStructureInstanceKHR),
										   vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
										   VMA_MEMORY_USAGE_CPU_TO_GPU, true, "<rt tlas instances>");
		pf.instanceCapacity = (pf.instanceBuffer.buffer) ? newCapacity : 0;
	}
	ASSERT_OR_RETURN(, pf.instanceBuffer.buffer && pf.instanceBuffer.pMapped, "No instance buffer");

	if (instanceCount > 0)
	{
		memcpy(pf.instanceBuffer.pMapped, frameInstances.data(), instanceCount * sizeof(vk::AccelerationStructureInstanceKHR));
	}
	vmaFlushAllocation(root.allocator, pf.instanceBuffer.allocation, 0, instanceCount * sizeof(vk::AccelerationStructureInstanceKHR));

	auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
		.setArrayOfPointers(false)
		.setData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(pf.instanceBuffer.address));
	auto geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eInstances)
		.setGeometry(vk::AccelerationStructureGeometryDataKHR().setInstances(instancesData));
	auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
		.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild)
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setGeometryCount(1)
		.setPGeometries(&geometry);

	// (re)create the TLAS + scratch if capacity is insufficient
	if (pf.tlasCapacity < instanceCount || !pf.tlas)
	{
		const uint32_t newCapacity = std::max(std::max(instanceCount + instanceCount / 2, WZ_RT_MIN_INSTANCE_CAPACITY), pf.tlasCapacity);
		const auto buildSizes = root.dev.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, { newCapacity }, root.vkDynLoader);

		destroyASDeferred(pf.tlas);
		destroyRTBufferDeferred(pf.tlasBuffer);
		destroyRTBufferDeferred(pf.tlasScratchBuffer);

		pf.tlasBuffer = createRTBuffer(buildSizes.accelerationStructureSize,
									   vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
									   VMA_MEMORY_USAGE_GPU_ONLY, false, "<rt tlas>");
		ASSERT_OR_RETURN(, pf.tlasBuffer.buffer, "Failed to create TLAS buffer");
		pf.tlasScratchBuffer = createRTBuffer(buildSizes.buildScratchSize + 256,
											  vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
											  VMA_MEMORY_USAGE_GPU_ONLY, false, "<rt tlas scratch>");
		ASSERT_OR_RETURN(, pf.tlasScratchBuffer.buffer, "Failed to create TLAS scratch buffer");

		auto asCreateInfo = vk::AccelerationStructureCreateInfoKHR()
			.setBuffer(pf.tlasBuffer.buffer)
			.setOffset(0)
			.setSize(buildSizes.accelerationStructureSize)
			.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
		pf.tlas = root.dev.createAccelerationStructureKHR(asCreateInfo, nullptr, root.vkDynLoader);
		pf.tlasCapacity = newCapacity;
	}

	buildInfo.setDstAccelerationStructure(pf.tlas);
	buildInfo.setScratchData(vk::DeviceOrHostAddressKHR().setDeviceAddress(alignAddress(pf.tlasScratchBuffer.address, 256)));

	auto rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
		.setPrimitiveCount(instanceCount)
		.setPrimitiveOffset(0)
		.setFirstVertex(0)
		.setTransformOffset(0);
	const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

	cmdBuffer.buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo, root.vkDynLoader);

	// TLAS build must complete before the fragment-shader ray queries read it
	const auto postTlasBarrier = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
			.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
							  vk::PipelineStageFlagBits::eFragmentShader,
							  vk::DependencyFlags(), postTlasBarrier, nullptr, nullptr, root.vkDynLoader);
}

// MARK: frame instance list

void VkRayShadowBackend::beginFrame()
{
	frameInstances.clear();
}

void VkRayShadowBackend::addInstance(const void* meshKey, int32_t variant, const glm::mat4& worldTransform)
{
	auto it = meshBlas.find(std::make_pair(meshKey, variant));
	if (it == meshBlas.end())
	{
		return;
	}
	auto instance = vk::AccelerationStructureInstanceKHR()
		.setTransform(toVkTransform(worldTransform))
		.setInstanceCustomIndex(0)
		.setMask(0xFF)
		.setInstanceShaderBindingTableRecordOffset(0)
		.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable | vk::GeometryInstanceFlagBitsKHR::eForceOpaque)
		.setAccelerationStructureReference(it->second.address);
	frameInstances.push_back(instance);
}

void VkRayShadowBackend::addTerrainInstance()
{
	if (!haveTerrain || !terrainBlas.as)
	{
		return;
	}
	auto instance = vk::AccelerationStructureInstanceKHR()
		.setTransform(toVkTransform(glm::mat4(1.f)))
		.setInstanceCustomIndex(0)
		.setMask(0xFF)
		.setInstanceShaderBindingTableRecordOffset(0)
		.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable | vk::GeometryInstanceFlagBitsKHR::eForceOpaque)
		.setAccelerationStructureReference(terrainBlas.address);
	frameInstances.push_back(instance);
}

// MARK: images

vk::Extent2D VkRayShadowBackend::targetMaskExtent() const
{
	uint32_t divisor = 1;
	switch (mode)
	{
		case gfx_api::ray_shadow_mode::HalfRes: divisor = 2; break;
		case gfx_api::ray_shadow_mode::QuarterRes: divisor = 4; break;
		default: break;
	}
	return vk::Extent2D(std::max(1u, (root.swapchainSize.width + divisor - 1) / divisor),
						std::max(1u, (root.swapchainSize.height + divisor - 1) / divisor));
}

void VkRayShadowBackend::destroyImages()
{
	maskValid = false;
	if (buffering_mechanism::isInitialized())
	{
		auto& frameResources = buffering_mechanism::get_current_resources();
		if (rtDepthFbo) { frameResources.fbo_to_delete.push_back(rtDepthFbo); rtDepthFbo = vk::Framebuffer(); }
		if (maskFboA) { frameResources.fbo_to_delete.push_back(maskFboA); maskFboA = vk::Framebuffer(); }
		if (maskFboB) { frameResources.fbo_to_delete.push_back(maskFboB); maskFboB = vk::Framebuffer(); }
		if (rtDepthView) { frameResources.image_view_to_delete.emplace_back(std::move(rtDepthView)); }
		if (rtDepthImage)
		{
			frameResources.image_to_delete.emplace_back(std::move(rtDepthImage));
			rtDepthImage = vk::Image();
			frameResources.vmamemory_to_free.push_back(rtDepthAllocation);
			rtDepthAllocation = VK_NULL_HANDLE;
		}
	}
	else
	{
		if (rtDepthFbo) { root.dev.destroyFramebuffer(rtDepthFbo, nullptr, root.vkDynLoader); rtDepthFbo = vk::Framebuffer(); }
		if (maskFboA) { root.dev.destroyFramebuffer(maskFboA, nullptr, root.vkDynLoader); maskFboA = vk::Framebuffer(); }
		if (maskFboB) { root.dev.destroyFramebuffer(maskFboB, nullptr, root.vkDynLoader); maskFboB = vk::Framebuffer(); }
		rtDepthView.reset();
		if (rtDepthImage)
		{
			root.dev.destroyImage(rtDepthImage, nullptr, root.vkDynLoader);
			rtDepthImage = vk::Image();
			vmaFreeMemory(root.allocator, rtDepthAllocation);
			rtDepthAllocation = VK_NULL_HANDLE;
		}
	}
	// VkRenderedImage destructor defers destruction (or releases if the buffering mechanism is gone)
	delete pMaskImageA;
	pMaskImageA = nullptr;
	delete pMaskImageB;
	pMaskImageB = nullptr;
	maskExtent = vk::Extent2D(0, 0);
}

bool VkRayShadowBackend::recreateImagesIfNeeded()
{
	const auto target = targetMaskExtent();
	if (maskExtent == target && pMaskImageA && rtDepthImage)
	{
		return true;
	}

	destroyImages();

	try
	{
		// camera depth image (same format + render pass as the shadow-map depth passes)
		auto imageCreateInfo = vk::ImageCreateInfo()
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(target.width, target.height, 1))
			.setImageType(vk::ImageType::e2D)
			.setMipLevels(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setFormat(root.depthBufferFormat)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setSharingMode(vk::SharingMode::eExclusive);
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VkResult result = vmaCreateImage(root.allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocInfo, reinterpret_cast<VkImage*>(&rtDepthImage), &rtDepthAllocation, nullptr);
		if (result != VK_SUCCESS)
		{
			debug(LOG_ERROR, "Failed to create ray-shadow depth image: %s", to_string(static_cast<vk::Result>(result)).c_str());
			return false;
		}

		const auto depthViewCreateInfo = vk::ImageViewCreateInfo()
			.setImage(rtDepthImage)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(root.depthBufferFormat)
			.setComponents(vk::ComponentMapping())
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
		rtDepthView = root.dev.createImageViewUnique(depthViewCreateInfo, nullptr, root.vkDynLoader);

		vk::ImageView nonUniqueDepthView = rtDepthView.get();
		rtDepthFbo = root.dev.createFramebuffer(
			vk::FramebufferCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(&nonUniqueDepthView)
			.setLayers(1)
			.setWidth(target.width)
			.setHeight(target.height)
			.setRenderPass(root.renderPasses[root.DEPTH_RENDER_PASS_ID].rp)
			, nullptr, root.vkDynLoader);

		// mask images (R8)
		pMaskImageA = new VkRenderedImage(root, target.width, target.height, vk::Format::eR8G8Unorm, "<rt shadow mask A>");
		pMaskImageB = new VkRenderedImage(root, target.width, target.height, vk::Format::eR8G8Unorm, "<rt shadow mask B>");

		vk::ImageView maskViewA = pMaskImageA->view.get();
		maskFboA = root.dev.createFramebuffer(
			vk::FramebufferCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(&maskViewA)
			.setLayers(1)
			.setWidth(target.width)
			.setHeight(target.height)
			.setRenderPass(maskRenderPass)
			, nullptr, root.vkDynLoader);
		vk::ImageView maskViewB = pMaskImageB->view.get();
		maskFboB = root.dev.createFramebuffer(
			vk::FramebufferCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(&maskViewB)
			.setLayers(1)
			.setWidth(target.width)
			.setHeight(target.height)
			.setRenderPass(maskRenderPass)
			, nullptr, root.vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_ERROR, "Failed to create ray-shadow images: %s", e.what());
		destroyImages();
		return false;
	}

	maskExtent = target;
	return true;
}

// MARK: pipelines

bool VkRayShadowBackend::createPipelines()
{
	if (pipelinesCreated)
	{
		return true;
	}

	auto vertSpv = readSpvFile("shaders/vk/rayshadow_fullscreen.vert.spv");
	auto traceSpv = readSpvFile("shaders/vk/rayshadow_mask.frag.spv");
	auto blurSpv = readSpvFile("shaders/vk/rayshadow_blur.frag.spv");
	if (vertSpv.empty() || traceSpv.empty() || blurSpv.empty())
	{
		debug(LOG_INFO, "Ray-traced shadows unavailable: shaders not found (shaders/vk/rayshadow_*.spv)");
		return false;
	}

	try
	{
		fullscreenVertShader = root.dev.createShaderModule(vk::ShaderModuleCreateInfo().setCodeSize(vertSpv.size() * 4).setPCode(vertSpv.data()), nullptr, root.vkDynLoader);
		traceFragShader = root.dev.createShaderModule(vk::ShaderModuleCreateInfo().setCodeSize(traceSpv.size() * 4).setPCode(traceSpv.data()), nullptr, root.vkDynLoader);
		blurFragShader = root.dev.createShaderModule(vk::ShaderModuleCreateInfo().setCodeSize(blurSpv.size() * 4).setPCode(blurSpv.data()), nullptr, root.vkDynLoader);

		// samplers
		depthSampler = root.dev.createSampler(vk::SamplerCreateInfo()
			.setMinFilter(vk::Filter::eNearest).setMagFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMaxLod(0.f), nullptr, root.vkDynLoader);
		maskSampler = root.dev.createSampler(vk::SamplerCreateInfo()
			.setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setMaxLod(0.f), nullptr, root.vkDynLoader);

		// mask render pass (R8 color, no depth); used by the trace pass and both blur passes
		auto colorAttachment = vk::AttachmentDescription()
			.setFormat(vk::Format::eR8G8Unorm)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
		const auto colorAttachmentRef = vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
		const auto subpass = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(1)
			.setPColorAttachments(&colorAttachmentRef);
		const std::array<vk::SubpassDependency, 2> dependencies {
			vk::SubpassDependency()
				.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				.setDstSubpass(0)
				.setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
				.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite),
			vk::SubpassDependency()
				.setSrcSubpass(0)
				.setDstSubpass(VK_SUBPASS_EXTERNAL)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		};
		maskRenderPass = root.dev.createRenderPass(vk::RenderPassCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(&colorAttachment)
			.setSubpassCount(1)
			.setPSubpasses(&subpass)
			.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
			.setPDependencies(dependencies.data()), nullptr, root.vkDynLoader);

		// descriptor set layouts
		const std::array<vk::DescriptorSetLayoutBinding, 2> traceBindings {
			vk::DescriptorSetLayoutBinding().setBinding(0).setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eFragment),
			vk::DescriptorSetLayoutBinding().setBinding(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eFragment)
		};
		traceDescLayout = root.dev.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(static_cast<uint32_t>(traceBindings.size())).setPBindings(traceBindings.data()), nullptr, root.vkDynLoader);

		const std::array<vk::DescriptorSetLayoutBinding, 2> blurBindings {
			vk::DescriptorSetLayoutBinding().setBinding(0).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eFragment),
			vk::DescriptorSetLayoutBinding().setBinding(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eFragment)
		};
		blurDescLayout = root.dev.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(static_cast<uint32_t>(blurBindings.size())).setPBindings(blurBindings.data()), nullptr, root.vkDynLoader);

		// pipeline layouts
		const auto tracePushRange = vk::PushConstantRange().setStageFlags(vk::ShaderStageFlagBits::eFragment).setOffset(0).setSize(sizeof(TracePushConstants));
		tracePipelineLayout = root.dev.createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1).setPSetLayouts(&traceDescLayout)
			.setPushConstantRangeCount(1).setPPushConstantRanges(&tracePushRange), nullptr, root.vkDynLoader);
		const auto blurPushRange = vk::PushConstantRange().setStageFlags(vk::ShaderStageFlagBits::eFragment).setOffset(0).setSize(sizeof(BlurPushConstants));
		blurPipelineLayout = root.dev.createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1).setPSetLayouts(&blurDescLayout)
			.setPushConstantRangeCount(1).setPPushConstantRanges(&blurPushRange), nullptr, root.vkDynLoader);

		// pipelines (fullscreen triangle; no vertex inputs; dynamic viewport/scissor)
		const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo();
		const auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleList);
		const auto viewportState = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);
		const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo()
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setLineWidth(1.f);
		const auto multisampleState = vk::PipelineMultisampleStateCreateInfo().setRasterizationSamples(vk::SampleCountFlagBits::e1);
		const auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo();
		const auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(false)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
		const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo().setAttachmentCount(1).setPAttachments(&colorBlendAttachment);
		const std::array<vk::DynamicState, 2> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		const auto dynamicState = vk::PipelineDynamicStateCreateInfo().setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size())).setPDynamicStates(dynamicStates.data());

		auto makePipeline = [&](vk::ShaderModule fragModule, vk::PipelineLayout layout) -> vk::Pipeline {
			const std::array<vk::PipelineShaderStageCreateInfo, 2> stages {
				vk::PipelineShaderStageCreateInfo().setModule(fullscreenVertShader).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
				vk::PipelineShaderStageCreateInfo().setModule(fragModule).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
			};
			auto createInfo = vk::GraphicsPipelineCreateInfo()
				.setStageCount(static_cast<uint32_t>(stages.size()))
				.setPStages(stages.data())
				.setPVertexInputState(&vertexInputState)
				.setPInputAssemblyState(&inputAssemblyState)
				.setPViewportState(&viewportState)
				.setPRasterizationState(&rasterizationState)
				.setPMultisampleState(&multisampleState)
				.setPDepthStencilState(&depthStencilState)
				.setPColorBlendState(&colorBlendState)
				.setPDynamicState(&dynamicState)
				.setLayout(layout)
				.setRenderPass(maskRenderPass);
			auto result = root.dev.createGraphicsPipeline(vk::PipelineCache(), createInfo, nullptr, root.vkDynLoader);
			if (result.result != vk::Result::eSuccess)
			{
				WZ_THROW_VK_RESULT_EXCEPTION_RT(result.result, "createGraphicsPipeline");
			}
			return result.value;
		};

		tracePipeline = makePipeline(traceFragShader, tracePipelineLayout);
		blurPipeline = makePipeline(blurFragShader, blurPipelineLayout);

		// descriptor pool (per-frame sets: 1 trace + 2 blur + 1 material TLAS)
		const std::array<vk::DescriptorPoolSize, 2> poolSizes {
			vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 16),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 32)
		};
		descriptorPool = root.dev.createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setMaxSets(24)
			.setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
			.setPPoolSizes(poolSizes.data()), nullptr, root.vkDynLoader);

		// TLAS descriptor set layout for material-shader ray-query pipeline variants
		// (published on the root so VkPSO creation can append it to pipeline layouts)
		const auto materialTlasBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		root.materialTlasSetLayout = root.dev.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1).setPBindings(&materialTlasBinding), nullptr, root.vkDynLoader);
	}
	catch (const vk::SystemError& e)
	{
		debug(LOG_ERROR, "Failed to create ray-shadow pipelines: %s", e.what());
		destroyPipelines();
		return false;
	}

	if (!createEmptyTlas())
	{
		destroyPipelines();
		return false;
	}

	pipelinesCreated = true;
	return true;
}

bool VkRayShadowBackend::createEmptyTlas()
{
	// build a 0-instance TLAS as a safe fallback for material-shader ray queries on frames
	// where no trace has run yet (e.g. menus); queries against it always miss
	auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR()
		.setArrayOfPointers(false)
		.setData(vk::DeviceOrHostAddressConstKHR().setDeviceAddress(0));
	auto geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eInstances)
		.setGeometry(vk::AccelerationStructureGeometryDataKHR().setInstances(instancesData));
	auto buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
		.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild)
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setGeometryCount(1)
		.setPGeometries(&geometry);

	const auto buildSizes = root.dev.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, { 0u }, root.vkDynLoader);

	emptyTlasBuffer = createRTBuffer(buildSizes.accelerationStructureSize,
									 vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
									 VMA_MEMORY_USAGE_GPU_ONLY, false, "<rt empty tlas>");
	ASSERT_OR_RETURN(false, emptyTlasBuffer.buffer, "Failed to create empty TLAS buffer");
	RTBuffer scratch = createRTBuffer(buildSizes.buildScratchSize + 256,
									  vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
									  VMA_MEMORY_USAGE_GPU_ONLY, false, "<rt empty tlas scratch>");
	if (!scratch.buffer)
	{
		destroyRTBufferNow(emptyTlasBuffer);
		return false;
	}

	auto asCreateInfo = vk::AccelerationStructureCreateInfoKHR()
		.setBuffer(emptyTlasBuffer.buffer)
		.setOffset(0)
		.setSize(buildSizes.accelerationStructureSize)
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
	emptyTlas = root.dev.createAccelerationStructureKHR(asCreateInfo, nullptr, root.vkDynLoader);

	buildInfo.setDstAccelerationStructure(emptyTlas);
	buildInfo.setScratchData(vk::DeviceOrHostAddressKHR().setDeviceAddress(alignAddress(scratch.address, 256)));

	auto rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR()
		.setPrimitiveCount(0)
		.setPrimitiveOffset(0)
		.setFirstVertex(0)
		.setTransformOffset(0);
	const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

	auto cmdBuffer = buffering_mechanism::get_current_resources().copyCmdBuffer();
	cmdBuffer.buildAccelerationStructuresKHR(1, &buildInfo, &pRangeInfo, root.vkDynLoader);
	const auto postBuildBarrier = std::array<vk::MemoryBarrier, 1> {
		vk::MemoryBarrier()
			.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
			.setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR)
	};
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
							  vk::PipelineStageFlagBits::eFragmentShader,
							  vk::DependencyFlags(), postBuildBarrier, nullptr, nullptr, root.vkDynLoader);

	destroyRTBufferDeferred(scratch);
	return true;
}

void VkRayShadowBackend::writeMaterialTlasDescSet(PerFrame& pf, vk::AccelerationStructureKHR tlas)
{
	if (!pf.materialTlasDescSet)
	{
		const std::array<vk::DescriptorSetLayout, 1> layouts { root.materialTlasSetLayout };
		auto sets = root.dev.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(layouts.data()), root.vkDynLoader);
		pf.materialTlasDescSet = sets[0];
	}
	auto tlasWriteInfo = vk::WriteDescriptorSetAccelerationStructureKHR()
		.setAccelerationStructureCount(1)
		.setPAccelerationStructures(&tlas);
	const auto write = vk::WriteDescriptorSet()
		.setDstSet(pf.materialTlasDescSet)
		.setDstBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
		.setPNext(&tlasWriteInfo);
	root.dev.updateDescriptorSets(1, &write, 0, nullptr, root.vkDynLoader);
}

vk::DescriptorSet VkRayShadowBackend::getMaterialTlasDescSet()
{
	if (!pipelinesCreated || !emptyTlas)
	{
		return vk::DescriptorSet();
	}
	// only ever called for the current frame (whose prior submission has been fence-waited),
	// so updating this frame's descriptor set here is safe
	PerFrame& pf = currentPerFrame();
	if (!pf.materialTlasDescSet || pf.materialTlasDescSetStale)
	{
		writeMaterialTlasDescSet(pf, emptyTlas);
		pf.materialTlasDescSetStale = false;
	}
	return pf.materialTlasDescSet;
}

void VkRayShadowBackend::destroyPipelines()
{
	// only called at shutdown or after pipeline-creation failure (no deferred destruction needed:
	// either nothing was submitted using these, or the device is idle at shutdown)
	if (tracePipeline) { root.dev.destroyPipeline(tracePipeline, nullptr, root.vkDynLoader); tracePipeline = vk::Pipeline(); }
	if (blurPipeline) { root.dev.destroyPipeline(blurPipeline, nullptr, root.vkDynLoader); blurPipeline = vk::Pipeline(); }
	if (tracePipelineLayout) { root.dev.destroyPipelineLayout(tracePipelineLayout, nullptr, root.vkDynLoader); tracePipelineLayout = vk::PipelineLayout(); }
	if (blurPipelineLayout) { root.dev.destroyPipelineLayout(blurPipelineLayout, nullptr, root.vkDynLoader); blurPipelineLayout = vk::PipelineLayout(); }
	if (traceDescLayout) { root.dev.destroyDescriptorSetLayout(traceDescLayout, nullptr, root.vkDynLoader); traceDescLayout = vk::DescriptorSetLayout(); }
	if (blurDescLayout) { root.dev.destroyDescriptorSetLayout(blurDescLayout, nullptr, root.vkDynLoader); blurDescLayout = vk::DescriptorSetLayout(); }
	if (descriptorPool) { root.dev.destroyDescriptorPool(descriptorPool, nullptr, root.vkDynLoader); descriptorPool = vk::DescriptorPool(); }
	if (root.materialTlasSetLayout) { root.dev.destroyDescriptorSetLayout(root.materialTlasSetLayout, nullptr, root.vkDynLoader); root.materialTlasSetLayout = vk::DescriptorSetLayout(); }
	if (emptyTlas) { root.dev.destroyAccelerationStructureKHR(emptyTlas, nullptr, root.vkDynLoader); emptyTlas = vk::AccelerationStructureKHR(); }
	destroyRTBufferNow(emptyTlasBuffer);
	if (maskRenderPass) { root.dev.destroyRenderPass(maskRenderPass, nullptr, root.vkDynLoader); maskRenderPass = vk::RenderPass(); }
	if (depthSampler) { root.dev.destroySampler(depthSampler, nullptr, root.vkDynLoader); depthSampler = vk::Sampler(); }
	if (maskSampler) { root.dev.destroySampler(maskSampler, nullptr, root.vkDynLoader); maskSampler = vk::Sampler(); }
	if (fullscreenVertShader) { root.dev.destroyShaderModule(fullscreenVertShader, nullptr, root.vkDynLoader); fullscreenVertShader = vk::ShaderModule(); }
	if (traceFragShader) { root.dev.destroyShaderModule(traceFragShader, nullptr, root.vkDynLoader); traceFragShader = vk::ShaderModule(); }
	if (blurFragShader) { root.dev.destroyShaderModule(blurFragShader, nullptr, root.vkDynLoader); blurFragShader = vk::ShaderModule(); }
	pipelinesCreated = false;
}

// MARK: descriptor sets

void VkRayShadowBackend::updateDescriptorSets(PerFrame& pf)
{
	if (!pf.descSetsAllocated)
	{
		const std::array<vk::DescriptorSetLayout, 3> layouts { traceDescLayout, blurDescLayout, blurDescLayout };
		auto sets = root.dev.allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptorPool)
			.setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
			.setPSetLayouts(layouts.data()), root.vkDynLoader);
		pf.traceDescSet = sets[0];
		pf.blurDescSetA = sets[1];
		pf.blurDescSetB = sets[2];
		pf.descSetsAllocated = true;
	}

	auto tlasWriteInfo = vk::WriteDescriptorSetAccelerationStructureKHR()
		.setAccelerationStructureCount(1)
		.setPAccelerationStructures(&pf.tlas);
	auto depthImageInfo = vk::DescriptorImageInfo()
		.setSampler(depthSampler)
		.setImageView(rtDepthView.get())
		.setImageLayout(vk::ImageLayout::eDepthStencilReadOnlyOptimal);
	auto maskAImageInfo = vk::DescriptorImageInfo()
		.setSampler(maskSampler)
		.setImageView(pMaskImageA->view.get())
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	auto maskBImageInfo = vk::DescriptorImageInfo()
		.setSampler(maskSampler)
		.setImageView(pMaskImageB->view.get())
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	const std::array<vk::WriteDescriptorSet, 6> writes {
		vk::WriteDescriptorSet().setDstSet(pf.traceDescSet).setDstBinding(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR).setPNext(&tlasWriteInfo),
		vk::WriteDescriptorSet().setDstSet(pf.traceDescSet).setDstBinding(1).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&depthImageInfo),
		vk::WriteDescriptorSet().setDstSet(pf.blurDescSetA).setDstBinding(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&maskAImageInfo),
		vk::WriteDescriptorSet().setDstSet(pf.blurDescSetA).setDstBinding(1).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&depthImageInfo),
		vk::WriteDescriptorSet().setDstSet(pf.blurDescSetB).setDstBinding(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&maskBImageInfo),
		vk::WriteDescriptorSet().setDstSet(pf.blurDescSetB).setDstBinding(1).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setPImageInfo(&depthImageInfo)
	};
	root.dev.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr, root.vkDynLoader);
}

// MARK: mode

bool VkRayShadowBackend::setMode(gfx_api::ray_shadow_mode newMode)
{
	if (newMode == mode)
	{
		return true;
	}
	if (newMode == gfx_api::ray_shadow_mode::Off)
	{
		destroyImages();
		for (auto& pf : perFrame)
		{
			releasePerFrameResources(pf);
		}
		mode = gfx_api::ray_shadow_mode::Off;
		return true;
	}
	if (!createPipelines())
	{
		return false;
	}
	if (mode == gfx_api::ray_shadow_mode::Off)
	{
		// force image (re)creation at the new resolution on the next frame
		maskExtent = vk::Extent2D(0, 0);
	}
	mode = newMode;
	maskValid = false;
	return true;
}

void VkRayShadowBackend::releasePerFrameResources(PerFrame& pf)
{
	destroyASDeferred(pf.tlas);
	destroyRTBufferDeferred(pf.tlasBuffer);
	destroyRTBufferDeferred(pf.tlasScratchBuffer);
	destroyRTBufferDeferred(pf.instanceBuffer);
	pf.tlasCapacity = 0;
	pf.instanceCapacity = 0;
	// descriptor sets remain allocated (pool-owned); they will be re-written before next use.
	// The material TLAS set may reference the just-released TLAS - mark it stale so it gets
	// re-pointed at the empty TLAS before its next use (we cannot update it here: other
	// frames-in-flight may still have it bound in submitted command buffers)
	pf.materialTlasDescSetStale = (pf.materialTlasDescSet) ? true : false;
}

// MARK: per-frame passes

void VkRayShadowBackend::beginDepthPass()
{
	ASSERT_OR_RETURN(, mode != gfx_api::ray_shadow_mode::Off, "Ray shadows not enabled");
	ASSERT_OR_RETURN(, !depthPassActive, "Ray shadow depth pass already active");

	if (!recreateImagesIfNeeded())
	{
		return;
	}

	const auto clearValue = std::array<vk::ClearValue, 1> {
		vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0u))
	};
	auto cmdBuffer = buffering_mechanism::get_current_resources().depthPassDrawCmdBuffer();
	cmdBuffer.beginRenderPass(
		vk::RenderPassBeginInfo()
		.setFramebuffer(rtDepthFbo)
		.setClearValueCount(static_cast<uint32_t>(clearValue.size()))
		.setPClearValues(clearValue.data())
		.setRenderPass(root.renderPasses[root.DEPTH_RENDER_PASS_ID].rp)
		.setRenderArea(vk::Rect2D(vk::Offset2D(), maskExtent)),
		vk::SubpassContents::eInline,
		root.vkDynLoader);
	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport().setHeight(maskExtent.height).setWidth(maskExtent.width).setMinDepth(0.f).setMaxDepth(1.f)
	};
	cmdBuffer.setViewport(0, viewports, root.vkDynLoader);
	const auto scissors = std::array<vk::Rect2D, 1> {
		vk::Rect2D().setExtent(maskExtent)
	};
	cmdBuffer.setScissor(0, scissors, root.vkDynLoader);

	// route subsequent gfx_api draws (which use the depth-pass PSO variants) into the depth cmd buffer
	buffering_mechanism::get_current_resources().beginDepthPass();
	root.currentRenderPassId = root.DEPTH_RENDER_PASS_ID;
	root.currentPSO = nullptr;

	depthPassActive = true;
}

void VkRayShadowBackend::traceAndFilter(const gfx_api::ray_shadow_frame_params& params)
{
	ASSERT_OR_RETURN(, depthPassActive, "Ray shadow depth pass was not begun");
	depthPassActive = false;

	auto& frameResources = buffering_mechanism::get_current_resources();
	auto depthCmdBuffer = frameResources.depthPassDrawCmdBuffer();

	// end the camera depth prepass
	depthCmdBuffer.endRenderPass(root.vkDynLoader);
	frameResources.endCurrentDepthPass();
	root.currentRenderPassId = root.DEFAULT_RENDER_PASS_ID;
	root.currentPSO = nullptr;

	// record acceleration structure builds into cmdCopy (executes before all draw cmd buffers)
	auto copyCmdBuffer = frameResources.copyCmdBuffer();
	recordPendingBuilds(copyCmdBuffer);
	PerFrame& pf = currentPerFrame();
	buildTlas(copyCmdBuffer, pf);
	ASSERT_OR_RETURN(, pf.tlas, "No TLAS available");

	updateDescriptorSets(pf);

	const auto viewports = std::array<vk::Viewport, 1> {
		vk::Viewport().setHeight(maskExtent.height).setWidth(maskExtent.width).setMinDepth(0.f).setMaxDepth(1.f)
	};
	const auto scissors = std::array<vk::Rect2D, 1> {
		vk::Rect2D().setExtent(maskExtent)
	};
	const auto beginMaskPass = [&](vk::Framebuffer fbo) {
		depthCmdBuffer.beginRenderPass(
			vk::RenderPassBeginInfo()
			.setFramebuffer(fbo)
			.setRenderPass(maskRenderPass)
			.setRenderArea(vk::Rect2D(vk::Offset2D(), maskExtent)),
			vk::SubpassContents::eInline,
			root.vkDynLoader);
		depthCmdBuffer.setViewport(0, viewports, root.vkDynLoader);
		depthCmdBuffer.setScissor(0, scissors, root.vkDynLoader);
	};

	// 1. trace pass -> mask A
	beginMaskPass(maskFboA);
	depthCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, tracePipeline, root.vkDynLoader);
	depthCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, tracePipelineLayout, 0, 1, &pf.traceDescSet, 0, nullptr, root.vkDynLoader);
	TracePushConstants tracePush;
	tracePush.invProjectionView = params.inverseProjectionViewMatrix;
	tracePush.sunDirection = glm::vec4(glm::normalize(glm::vec3(params.sunDirectionWorld)), 0.f);
	tracePush.cameraPosition = params.cameraPositionWorld;
	tracePush.params = glm::vec4(params.rayTMin, params.rayTMax, params.normalOffsetScale, params.sunConeAngle);
	tracePush.aoParams = glm::vec4(static_cast<float>(params.aoRayCount), params.aoMaxDistance, params.aoStrength, 0.f);
	depthCmdBuffer.pushConstants(tracePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(TracePushConstants), &tracePush, root.vkDynLoader);
	depthCmdBuffer.draw(3, 1, 0, 0, root.vkDynLoader);
	depthCmdBuffer.endRenderPass(root.vkDynLoader);

	const glm::vec2 invSize = glm::vec2(1.f / static_cast<float>(maskExtent.width), 1.f / static_cast<float>(maskExtent.height));

	// 2. horizontal blur: mask A -> mask B
	beginMaskPass(maskFboB);
	depthCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, blurPipeline, root.vkDynLoader);
	depthCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blurPipelineLayout, 0, 1, &pf.blurDescSetA, 0, nullptr, root.vkDynLoader);
	BlurPushConstants blurPush;
	blurPush.dirAndInvSize = glm::vec4(1.f, 0.f, invSize.x, invSize.y);
	blurPush.params = glm::vec4(0.08f, 0.f, 0.f, 0.f); // relative view-depth rejection threshold
	depthCmdBuffer.pushConstants(blurPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(BlurPushConstants), &blurPush, root.vkDynLoader);
	depthCmdBuffer.draw(3, 1, 0, 0, root.vkDynLoader);
	depthCmdBuffer.endRenderPass(root.vkDynLoader);

	// 3. vertical blur: mask B -> mask A (final, exposed via getMaskTexture)
	beginMaskPass(maskFboA);
	depthCmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, blurPipeline, root.vkDynLoader);
	depthCmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blurPipelineLayout, 0, 1, &pf.blurDescSetB, 0, nullptr, root.vkDynLoader);
	blurPush.dirAndInvSize = glm::vec4(0.f, 1.f, invSize.x, invSize.y);
	depthCmdBuffer.pushConstants(blurPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(BlurPushConstants), &blurPush, root.vkDynLoader);
	depthCmdBuffer.draw(3, 1, 0, 0, root.vkDynLoader);
	depthCmdBuffer.endRenderPass(root.vkDynLoader);

	// point the material-shader TLAS set at this frame's freshly-built TLAS
	writeMaterialTlasDescSet(pf, pf.tlas);
	pf.materialTlasDescSetStale = false;

	maskValid = true;
}

gfx_api::abstract_texture* VkRayShadowBackend::getMaskTexture()
{
	if (!maskValid || mode == gfx_api::ray_shadow_mode::Off)
	{
		return nullptr;
	}
	return pMaskImageA;
}

// MARK: shutdown

void VkRayShadowBackend::destroyNow()
{
	// the device is expected to be idle at this point (called from VkRoot::shutdown)
	maskValid = false;

	for (auto& kv : meshBlas)
	{
		if (kv.second.as) { root.dev.destroyAccelerationStructureKHR(kv.second.as, nullptr, root.vkDynLoader); }
		destroyRTBufferNow(kv.second.asBuffer);
	}
	meshBlas.clear();

	for (auto& pending : pendingBuilds)
	{
		if (!pending.keepGeometryBuffer)
		{
			destroyRTBufferNow(pending.geometryBuffer);
		}
	}
	pendingBuilds.clear();

	if (terrainBlas.as) { root.dev.destroyAccelerationStructureKHR(terrainBlas.as, nullptr, root.vkDynLoader); terrainBlas.as = vk::AccelerationStructureKHR(); }
	destroyRTBufferNow(terrainBlas.asBuffer);
	destroyRTBufferNow(terrainGeometryBuffer);
	haveTerrain = false;

	for (auto& pf : perFrame)
	{
		if (pf.tlas) { root.dev.destroyAccelerationStructureKHR(pf.tlas, nullptr, root.vkDynLoader); pf.tlas = vk::AccelerationStructureKHR(); }
		destroyRTBufferNow(pf.tlasBuffer);
		destroyRTBufferNow(pf.tlasScratchBuffer);
		destroyRTBufferNow(pf.instanceBuffer);
	}
	perFrame.clear();

	// images (buffering mechanism is gone at shutdown; destroyImages handles both cases)
	if (pMaskImageA) { pMaskImageA->destroy(root.dev, root.allocator, root.vkDynLoader); }
	if (pMaskImageB) { pMaskImageB->destroy(root.dev, root.allocator, root.vkDynLoader); }
	destroyImages();

	destroyPipelines();
}

// MARK: VkRoot ray-shadow API

bool VkRoot::supportsRayTracedShadows() const
{
	return rayQuerySupported;
}

bool VkRoot::rayShadows_setMode(gfx_api::ray_shadow_mode newMode)
{
	if (!rayQuerySupported)
	{
		return newMode == gfx_api::ray_shadow_mode::Off;
	}
	if (!rayShadowBackend)
	{
		if (newMode == gfx_api::ray_shadow_mode::Off)
		{
			return true;
		}
		rayShadowBackend = std::make_unique<VkRayShadowBackend>(*this);
	}
	return rayShadowBackend->setMode(newMode);
}

gfx_api::ray_shadow_mode VkRoot::rayShadows_getMode() const
{
	return (rayShadowBackend) ? rayShadowBackend->getMode() : gfx_api::ray_shadow_mode::Off;
}

void VkRoot::rayShadows_setTerrainGeometry(const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount)
{
	if (rayShadowBackend) { rayShadowBackend->setTerrainGeometry(vertices, vertexCount, indices, indexCount); }
}

void VkRoot::rayShadows_updateTerrainVertices(size_t startVertex, const float* vertices, size_t vertexCount)
{
	if (rayShadowBackend) { rayShadowBackend->updateTerrainVertices(startVertex, vertices, vertexCount); }
}

void VkRoot::rayShadows_clearTerrainGeometry()
{
	if (rayShadowBackend) { rayShadowBackend->clearTerrainGeometry(); }
}

bool VkRoot::rayShadows_hasMeshBLAS(const void* meshKey, int32_t variant)
{
	return (rayShadowBackend) ? rayShadowBackend->hasMeshBLAS(meshKey, variant) : true;
}

void VkRoot::rayShadows_registerMeshGeometry(const void* meshKey, int32_t variant, const float* vertices, size_t vertexCount, const uint32_t* indices, size_t indexCount)
{
	if (rayShadowBackend) { rayShadowBackend->registerMeshGeometry(meshKey, variant, vertices, vertexCount, indices, indexCount); }
}

void VkRoot::rayShadows_invalidateMesh(const void* meshKey)
{
	if (rayShadowBackend) { rayShadowBackend->invalidateMesh(meshKey); }
}

void VkRoot::rayShadows_beginFrame()
{
	if (rayShadowBackend) { rayShadowBackend->beginFrame(); }
}

void VkRoot::rayShadows_addInstance(const void* meshKey, int32_t variant, const glm::mat4& worldTransform)
{
	if (rayShadowBackend) { rayShadowBackend->addInstance(meshKey, variant, worldTransform); }
}

void VkRoot::rayShadows_addTerrainInstance()
{
	if (rayShadowBackend) { rayShadowBackend->addTerrainInstance(); }
}

void VkRoot::rayShadows_beginDepthPass()
{
	if (rayShadowBackend) { rayShadowBackend->beginDepthPass(); }
}

void VkRoot::rayShadows_traceAndFilter(const gfx_api::ray_shadow_frame_params& params)
{
	if (rayShadowBackend) { rayShadowBackend->traceAndFilter(params); }
}

gfx_api::abstract_texture* VkRoot::rayShadows_getMaskTexture()
{
	return (rayShadowBackend) ? rayShadowBackend->getMaskTexture() : nullptr;
}

#else // !defined(WZ_VK_RAY_QUERY_POSSIBLE)

// Vulkan headers too old for ray query support - provide stub implementations
bool VkRoot::supportsRayTracedShadows() const { return false; }
bool VkRoot::rayShadows_setMode(gfx_api::ray_shadow_mode newMode) { return newMode == gfx_api::ray_shadow_mode::Off; }
gfx_api::ray_shadow_mode VkRoot::rayShadows_getMode() const { return gfx_api::ray_shadow_mode::Off; }
void VkRoot::rayShadows_setTerrainGeometry(const float*, size_t, const uint32_t*, size_t) { }
void VkRoot::rayShadows_updateTerrainVertices(size_t, const float*, size_t) { }
void VkRoot::rayShadows_clearTerrainGeometry() { }
bool VkRoot::rayShadows_hasMeshBLAS(const void*, int32_t) { return true; }
void VkRoot::rayShadows_registerMeshGeometry(const void*, int32_t, const float*, size_t, const uint32_t*, size_t) { }
void VkRoot::rayShadows_invalidateMesh(const void*) { }
void VkRoot::rayShadows_beginFrame() { }
void VkRoot::rayShadows_addInstance(const void*, int32_t, const glm::mat4&) { }
void VkRoot::rayShadows_addTerrainInstance() { }
void VkRoot::rayShadows_beginDepthPass() { }
void VkRoot::rayShadows_traceAndFilter(const gfx_api::ray_shadow_frame_params&) { }
gfx_api::abstract_texture* VkRoot::rayShadows_getMaskTexture() { return nullptr; }

#endif // defined(WZ_VK_RAY_QUERY_POSSIBLE)

#endif // defined(WZ_VULKAN_ENABLED)
