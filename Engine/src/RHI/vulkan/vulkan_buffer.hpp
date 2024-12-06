#pragma once
#include "../buffer.hpp"
#include"vulkan_core.hpp"
namespace rhi::vulkan {
	class VulkanDevice;

	class VulkanBuffer final : public Buffer {
	public:
		VulkanBuffer(VulkanDevice* device, const BufferDescription& desc, const std::string& name);
		~VulkanBuffer();

		bool create();
		// Buffer interface
		void* map() override;
		void unmap() override;
		uint64_t getGpuAddress() const override;
		void* getCpuAddress() override;
		bool isMapped() const override { return m_Mapped; }

		// Resource interface
		void* getHandle() const override { return m_Buffer; }
		bool isBuffer() const override { return true; }

		// Vulkan specific
		VkBuffer getVkBuffer() const { return m_Buffer; }
		VmaAllocation getAllocation() const { return m_Allocation; }

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		void* m_MappedData = nullptr;
		bool m_Mapped = false;
	};
}