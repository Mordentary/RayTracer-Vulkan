#pragma once
#include <volk/volk.h>
#include <vk_mem_alloc.h>

#include "../types.hpp"
namespace rhi::vulkan
{
	template<typename T>
	inline void setDebugName(VkDevice device, VkObjectType type, T object, const char* name) {
		VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.objectType = type;
		info.objectHandle = (uint64_t)object;
		info.pObjectName = name;
		vkSetDebugUtilsObjectNameEXT(device, &info);
	}
	VkBufferUsageFlags translateBufferUsage(BufferUsageFlags usage);
	VmaMemoryUsage translateMemoryType(MemoryType type);
}