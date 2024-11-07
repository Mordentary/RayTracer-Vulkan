#pragma once
#include "../buffer.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace rhi {
	namespace vulkan {
		class VulkanDevice;

		class VulkanBuffer final : public Buffer {
		public:
			VulkanBuffer(VulkanDevice* device, const BufferDescription& desc, const void* initialData = nullptr);
			~VulkanBuffer();

			// Buffer interface
			void* map() override;
			void unmap() override;
			const BufferDescription& getDesc() const override { return m_Desc; }
			uint64_t getGpuAddress() const override;
			bool isMapped() const override { return m_Mapped; }

			// Resource interface
			void* getHandle() const override { return m_Buffer; }

			// Vulkan specific
			VkBuffer getVkBuffer() const { return m_Buffer; }
			VmaAllocation getAllocation() const { return m_Allocation; }

		private:
			bool init(const void* initialData);
			void cleanup();

			VulkanDevice* m_VulkanDevice;
			BufferDescription m_Desc;

			VkBuffer m_Buffer = VK_NULL_HANDLE;
			VmaAllocation m_Allocation = VK_NULL_HANDLE;
			void* m_MappedData = nullptr;
			bool m_Mapped = false;
		};
	} // namespace vulkan
} // namespace rhi