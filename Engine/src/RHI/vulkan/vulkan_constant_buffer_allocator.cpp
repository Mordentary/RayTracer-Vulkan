#include "vulkan_constant_buffer_allocator.hpp"
#include "vulkan_device.hpp"

namespace rhi::vulkan {
	VulkanConstantBufferAllocator::VulkanConstantBufferAllocator(VulkanDevice* device, uint32_t bufferSize)
	{
		m_Device = device;
		m_BufferSize = bufferSize;

		VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.size = bufferSize;
		createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocatedInfo;
		vmaCreateBuffer(device->getVmaAllocator(), &createInfo, &allocInfo,
			&m_Buffer, &m_Allocation, &allocatedInfo);

		m_CpuAddress = allocatedInfo.pMappedData;

		VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		addressInfo.buffer = m_Buffer;
		m_GpuAddress = vkGetBufferDeviceAddress(device->getDevice(), &addressInfo);
	}

	VulkanConstantBufferAllocator::~VulkanConstantBufferAllocator()
	{
		vmaDestroyBuffer(m_Device->getVmaAllocator(), m_Buffer, m_Allocation);
	}

	void VulkanConstantBufferAllocator::allocate(uint32_t size, void** cpuAddress, VkDeviceAddress* gpuAddress)
	{
		SE_ASSERT_NOMSG(m_AllocatedSize + size <= m_BufferSize);
		*cpuAddress = static_cast<char*>(m_CpuAddress) + m_AllocatedSize;
		*gpuAddress = m_GpuAddress + m_AllocatedSize;
		m_AllocatedSize += SE::alignToPowerOfTwo<uint32_t>(size, 256);
	}

	void VulkanConstantBufferAllocator::reset()
	{
		m_AllocatedSize = 0;
	}
}