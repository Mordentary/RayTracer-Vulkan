#pragma once
#include "vulkan_core.hpp"
#include "../texture.hpp"

#include "vulkan_device.hpp"
#include<string>

namespace rhi::vulkan
{
	class VulkanTexture : public Texture
	{
	public:
		VulkanTexture(VulkanDevice* device, const TextureDescription& desc, const std::string& name);
		~VulkanTexture();

		bool create();
		bool create(VkImage image); //For swapchain use
		bool isSwapchainTexture() const { return m_IsSwapchainImage; };
		//Res interface
		virtual void* getHandle() const override { return m_Image; };
		uint32_t getRequiredStagingBufferSize() const;
		VkImageView getRenderView(uint32_t mipSlice, uint32_t arraySlice);

	private:
		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
		bool m_IsSwapchainImage = false;
		std::vector<VkImageView> m_RenderViews;
	};
}