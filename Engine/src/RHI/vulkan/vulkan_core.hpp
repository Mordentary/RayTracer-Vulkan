#pragma once
#define VK_NO_PROTOTYPES
#include <volk\volk.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>

#include <type_traits>

#include "../types.hpp"

namespace rhi::vulkan
{
	static constexpr uint32_t MaxSamplerAnisotropy = 16;
	static constexpr uint32_t RHI_INVALID_RESOURCE = uint32_t(-1);

	template<typename T>
	inline void setDebugName(VkDevice device, VkObjectType type, T object, const char* name) {
		VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.objectType = type;
		info.objectHandle = (uint64_t)object;
		info.pObjectName = name;
		vkSetDebugUtilsObjectNameEXT(device, &info);
	}

	VkPrimitiveTopology toVkPrimitiveTopology(PrimitiveType type);
	VkCullModeFlags toVkCullMode(CullMode mode);
	VkPipelineRasterizationStateCreateInfo toVkPipelineRasterizationStateCreateInfo(const Rasterizer& state);
	VkPipelineDepthStencilStateCreateInfo toVkPipelineDepthStencilStateCreateInfo(const DepthStencil& state);
	VkCompareOp toVkCompareOp(CompareFunction func);
	VkStencilOp toVkStencilOp(StencilOperation stencilOp);
	VkStencilOpState toVkStencilOpState(const DepthStencilOperation& state, uint8_t readMask, uint8_t writeMask);
	VkBlendFactor toVkBlendFactor(BlendFactor blendFactor, bool alpha = false);
	VkBlendOp toVkBlendOp(BlendOperation blendOp);
	VkPipelineColorBlendStateCreateInfo toVkPipelineColorBlendStateCreateInfo(const Blend* states, VkPipelineColorBlendAttachmentState* vkStates);

	VkAttachmentLoadOp toVkLoadOp(RenderPassLoadOp loadOp);
	VkAttachmentStoreOp toVkStoreOp(RenderPassStoreOp storeOp);
	VkSamplerAddressMode toVkSamplerAddressMode(AddressMode addressMode);
	// Sampler validation
	void validateSamplerDescription(const SamplerDescription& desc);

	// Translation functions for buffer and memory
	VmaMemoryUsage translateMemoryTypeToVMA(MemoryType type);

	// Format and layout conversions
	VkFormat toVkFormat(Format format, bool sRGB = false);
	VkImageAspectFlags getVkAspectMask(Format format);
	VkImageLayout toVkLayout(TextureLayout layout);

	// Pipeline stage and access flags
	VkPipelineStageFlags2 getVkStageMask(ResourceAccessFlags state);
	VkAccessFlags2 getVkAccessMask(ResourceAccessFlags state);

	// Sampler related conversions
	VkSamplerCreateInfo samplerCreateInfo(const SamplerDescription& desc);
	VkSamplerAddressMode toVkAddressMode(AddressMode mode);
	VkFilter toVkFilter(FilterMode filter);

	// Image related functions
	VkImageCreateInfo toImageCreateInfo(const TextureDescription& desc);
	VkImageViewCreateInfo imageViewCreateInfo();

	VkImageViewType getVkImageViewType(TextureType type);
	VkImageLayout getVkImageLayout(ResourceAccessFlags access);

	// Helper functions for image views and component mapping
	VkImageSubresourceRange getVkSubresourceRange(VkImageAspectFlags aspectMask,
		uint32_t baseMipLevel,
		uint32_t levelCount,
		uint32_t baseArrayLayer,
		uint32_t layerCount);
	VkComponentMapping getVkComponentMapping(bool swizzleRGB = false);

	VkPipelineRenderingCreateInfo toVkPipelineRenderingCreateInfo(const GraphicsPipelineDescription& pipelineDesc, VkFormat* colorFormats);
}