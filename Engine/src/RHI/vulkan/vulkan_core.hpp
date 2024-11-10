#pragma once
#include <volk/volk.h>
#include <vk_mem_alloc.h>
#include <type_traits>

#include "../types.hpp"
#include <RHI\texture.hpp>

namespace rhi::vulkan
{
	static constexpr uint32_t MaxSamplerAnisotropy = 16;

	template<typename T>
	inline void setDebugName(VkDevice device, VkObjectType type, T object, const char* name) {
		VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.objectType = type;
		info.objectHandle = (uint64_t)object;
		info.pObjectName = name;
		vkSetDebugUtilsObjectNameEXT(device, &info);
	}

	// Sampler validation
	void validateSamplerDescription(const SamplerDescription& desc);

	// Translation functions for buffer and memory
	VkBufferUsageFlags translateBufferUsage(BufferUsageFlags usage);
	VmaMemoryUsage translateMemoryType(MemoryType type);

	// Format and layout conversions
	VkFormat toVulkanFormat(Format format, bool sRGB = false);
	VkImageAspectFlags getAspectMask(Format format);
	VkImageLayout toVulkanLayout(TextureLayout layout);

	// Pipeline stage and access flags
	VkPipelineStageFlags2 getPipelineStageFlags(ResourceState state);
	VkAccessFlags2 getAccessFlags(ResourceState state);

	// Sampler related conversions
	VkSamplerCreateInfo samplerCreateInfo(const SamplerDescription& desc);
	VkSamplerAddressMode toVulkanAddressMode(AddressMode mode);
	VkFilter toVulkanFilter(FilterMode filter);

	// Image related functions
	VkImageCreateInfo toImageCreateInfo(const TextureDescription& desc);
	VkImageViewCreateInfo imageViewCreateInfo();
	VkImageViewType getViewType(TextureType type);

	// Helper functions for image views and component mapping
	VkImageSubresourceRange getSubresourceRange(VkImageAspectFlags aspectMask,
		uint32_t baseMipLevel,
		uint32_t levelCount,
		uint32_t baseArrayLayer,
		uint32_t layerCount);
	VkComponentMapping getComponentMapping(bool swizzleRGB = false);

	template<typename Enum>
	inline bool anySet(Enum flags, Enum mask) {
		using underlying = typename std::underlying_type<Enum>::type;
		return (static_cast<underlying>(flags) & static_cast<underlying>(mask)) != 0;
	}
}