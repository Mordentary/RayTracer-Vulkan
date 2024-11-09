#include <volk/volk.h>
#include <vk_mem_alloc.h>
#include "../types.hpp"
namespace rhi::vulkan
{
	VkBufferUsageFlags translateBufferUsage(BufferUsageFlags usage) {
		VkBufferUsageFlags result = 0;

		if ((usage & BufferUsageFlags::VertexBuffer) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		if ((usage & BufferUsageFlags::IndexBuffer) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		if ((usage & BufferUsageFlags::ConstantBuffer) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		if ((usage & BufferUsageFlags::UnorderedAccess) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		if ((usage & BufferUsageFlags::TransferSrc) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if ((usage & BufferUsageFlags::TransferDst) != BufferUsageFlags::None)
			result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		return result;
	}

	VmaMemoryUsage translateMemoryType(MemoryType type) {
		switch (type) {
		case MemoryType::GpuOnly:
			return VMA_MEMORY_USAGE_GPU_ONLY;
		case MemoryType::CpuToGpu:
			return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case MemoryType::GpuToCpu:
			return VMA_MEMORY_USAGE_GPU_TO_CPU;
		default:
			assert(false && "Invalid memory type");
			return VMA_MEMORY_USAGE_UNKNOWN;
		}
	}
}