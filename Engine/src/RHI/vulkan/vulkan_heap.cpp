#include "vulkan_heap.hpp"
#include "vulkan_device.hpp"

namespace rhi::vulkan
{
	VulkanHeap::VulkanHeap(VulkanDevice* device, const HeapDescription& desc, const std::string& name)
	{
		m_Device = device;
		m_Description = desc;
		m_DebugName = name;
	}
	VulkanHeap::~VulkanHeap()
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Allocation);
	}
	bool VulkanHeap::create()
	{
		SE_ASSERT(m_Description.size % (64 * 1024) == 0);

		VmaAllocator allocator = ((VulkanDevice*)m_Device)->getVmaAllocator();

		VkMemoryRequirements requirements = {};
		requirements.size = m_Description.size;
		requirements.alignment = 1;

		switch (m_Description.memoryType)
		{
		case rhi::MemoryType::GpuOnly:
			requirements.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			break;
		case rhi::MemoryType::CpuOnly:
			requirements.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
		case rhi::MemoryType::CpuToGpu:
			requirements.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			break;
		case rhi::MemoryType::GpuToCpu:
			requirements.memoryTypeBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			break;
		default:
			break;
		}

		VmaAllocationCreateInfo createInfo = {};
		createInfo.usage = translateMemoryTypeToVMA(m_Description.memoryType);
		createInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		VkResult result = vmaAllocateMemory(allocator, &requirements, &createInfo, &m_Allocation, nullptr);
		if (result != VK_SUCCESS)
		{
			SE_ASSERT(false, "[VulkanHeap] failed to create {}", m_DebugName);
			return false;
		}

		vmaSetAllocationName(allocator, m_Allocation, m_DebugName.c_str());

		return true;
	}
}