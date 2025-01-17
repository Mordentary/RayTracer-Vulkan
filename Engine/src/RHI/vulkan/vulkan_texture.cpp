#include "vulkan_texture.hpp"

namespace rhi::vulkan
{
	//VulkanTexture::VulkanTexture(VulkanDevice* device, const TextureDescription& desc, const std::string& name)
	//{
	//	m_Device = device;
	//	m_Description = desc;
	//	m_DebugName = name;
	//}

	VulkanTexture::VulkanTexture(VulkanDevice* device, const TextureDescription& desc, const std::string& name)
	{
		m_Device = device;
		m_Description = desc;
		m_DebugName = name;
	}

	VulkanTexture::~VulkanTexture()
	{
		VulkanDevice* pDevice = (VulkanDevice*)m_Device;
		pDevice->cancelLayoutTransition(this);

		if (!m_IsSwapchainImage)
		{
			pDevice->enqueueDeletion(m_Image);
			pDevice->enqueueDeletion(m_allocation);
		}

		for (size_t i = 0; i < m_RenderViews.size(); ++i)
		{
			pDevice->enqueueDeletion(m_RenderViews[i]);
		}
	}

	bool VulkanTexture::create()
	{
		VkDevice device = ((VulkanDevice*)m_Device)->getDevice();
		VmaAllocator allocator = ((VulkanDevice*)m_Device)->getVmaAllocator();

		VkImageCreateInfo createInfo = toImageCreateInfo(m_Description);
		VmaAllocationCreateInfo allocationInfo = {};
		allocationInfo.usage = translateMemoryTypeToVMA(m_Description.memoryType);

		VK_CHECK_RETURN(vmaCreateImage(allocator, &createInfo, &allocationInfo, &m_Image, &m_allocation, nullptr), false, "Image creation failed!");

		setDebugName(device, VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName.c_str());

		if (m_allocation)
		{
			vmaSetAllocationName(allocator, m_allocation, m_DebugName.c_str());
		}

		m_IsSwapchainImage = false;

		((VulkanDevice*)m_Device)->enqueueDefaultLayoutTransition(this);

		return true;
	}

	bool VulkanTexture::create(VkImage image)
	{
		m_Image = image;
		m_IsSwapchainImage = true;

		setDebugName((VkDevice)(m_Device)->getHandle(), VK_OBJECT_TYPE_IMAGE, m_Image, m_DebugName.c_str());

		((VulkanDevice*)m_Device)->enqueueDefaultLayoutTransition(this);

		return true;
	}

	uint32_t VulkanTexture::getRequiredStagingBufferSize() const
	{
		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements((VkDevice)m_Device->getHandle(), m_Image, &requirements);
		return (uint32_t)requirements.size;
	}
	VkImageView VulkanTexture::getRenderView(uint32_t mipSlice, uint32_t arraySlice)
	{
		SE_ASSERT(anySet(m_Description.usage, (TextureUsageFlags::RenderTarget | TextureUsageFlags::DepthStencil)), "RenderView is not used properly!");

		if (m_RenderViews.empty())
		{
			m_RenderViews.resize(m_Description.mipLevels * m_Description.arraySize);
		}

		uint32_t index = m_Description.mipLevels * arraySlice + mipSlice;
		if (m_RenderViews[index] == VK_NULL_HANDLE)
		{
			VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			createInfo.image = m_Image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = toVkFormat(m_Description.format);
			createInfo.subresourceRange.aspectMask = getVkAspectMask(m_Description.format);
			createInfo.subresourceRange.baseMipLevel = mipSlice;
			createInfo.subresourceRange.baseArrayLayer = arraySlice;
			createInfo.subresourceRange.layerCount = 1;
			createInfo.subresourceRange.levelCount = 1;

			vkCreateImageView((VkDevice)m_Device->getHandle(), &createInfo, nullptr, &m_RenderViews[index]);
		}

		return m_RenderViews[index];
	}
}