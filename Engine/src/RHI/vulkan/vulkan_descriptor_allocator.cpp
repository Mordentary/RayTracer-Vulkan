#include "vulkan_descriptor_allocator.hpp"
#include "vulkan_device.hpp"

namespace rhi::vulkan {
	VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanDevice* device,
		uint32_t descriptorSize,
		uint32_t descriptorCount,
		VkBufferUsageFlags usage)
	{
		m_Device = device;
		m_DescriptorSize = descriptorSize;
		m_DescriptorCount = descriptorCount;

		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.size = descriptorSize * descriptorCount;
		createInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VmaAllocationCreateInfo allocationCreateInfo = {};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocationInfo;
		vmaCreateBuffer(((VulkanDevice*)device)->getVmaAllocator(),
			&createInfo,
			&allocationCreateInfo,
			&m_Buffer,
			&m_Allocation,
			&allocationInfo);

		m_CpuAddress = allocationInfo.pMappedData;

		VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = m_Buffer;
		m_GpuAddress = vkGetBufferDeviceAddress(device->getDevice(), &info);
	}

	VulkanDescriptorAllocator::~VulkanDescriptorAllocator() {
		vmaDestroyBuffer(m_Device->getVmaAllocator(), m_Buffer, m_Allocation);
	}

	uint32_t VulkanDescriptorAllocator::allocate(void** descriptor) {
		uint32_t index = 0;

		if (!m_FreeDescriptors.empty()) {
			index = m_FreeDescriptors.back();
			m_FreeDescriptors.pop_back();
		}
		else
		{
			SE_ASSERT_NOMSG(m_AllocatedCount < m_DescriptorCount);
			index = m_AllocatedCount;
			++m_AllocatedCount;
		}

		*descriptor = static_cast<char*>(m_CpuAddress) + m_DescriptorSize * index;
		return index;
	}

	void VulkanDescriptorAllocator::free(uint32_t index) {
		m_FreeDescriptors.push_back(index);
	}
}