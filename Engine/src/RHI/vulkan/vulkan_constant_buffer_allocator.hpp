#pragma once
#include "vulkan_core.hpp"

namespace rhi::vulkan
{
	class VulkanDevice;

	class VulkanConstantBufferAllocator {
	public:
		VulkanConstantBufferAllocator(VulkanDevice* device, uint32_t bufferSize);
		~VulkanConstantBufferAllocator();

		void allocate(uint32_t size, void** cpuAddress, VkDeviceAddress* gpuAddress);
		void reset();
		VkDeviceAddress getGpuAddress() const { return m_GpuAddress; }

	private:
		VulkanDevice* m_Device{ nullptr };
		VkBuffer m_Buffer{ VK_NULL_HANDLE };
		VmaAllocation m_Allocation{ VK_NULL_HANDLE };
		VkDeviceAddress m_GpuAddress{ 0 };
		void* m_CpuAddress{ nullptr };
		uint32_t m_BufferSize{ 0 };
		uint32_t m_AllocatedSize{ 0 };
	};
}