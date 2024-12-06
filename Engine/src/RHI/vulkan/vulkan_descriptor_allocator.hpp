#pragma once
#include "vulkan_core.hpp"

namespace rhi::vulkan
{
	class VulkanDevice;

	class VulkanDescriptorAllocator {
	public:
		VulkanDescriptorAllocator(VulkanDevice* device,
			uint32_t descriptorSize,
			uint32_t descriptorCount,
			VkBufferUsageFlags usage);
		~VulkanDescriptorAllocator();

		uint32_t allocate(void** descriptor);
		void free(uint32_t index);
		VkDeviceAddress getGpuAddress() const { return m_GpuAddress; }

	private:
		VulkanDevice* m_Device = nullptr;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkDeviceAddress m_GpuAddress = 0;
		void* m_CpuAddress = nullptr;
		uint32_t m_DescriptorSize = 0;
		uint32_t m_DescriptorCount = 0;
		uint32_t m_AllocatedCount = 0;
		std::vector<uint32_t> m_FreeDescriptors;
	};
}