#pragma once
#include "vulkan_core.hpp"
#include "../heap.hpp"

namespace rhi::vulkan
{
	class VulkanDevice;
	class VulkanHeap : public IHeap
	{
	public:
		VulkanHeap(VulkanDevice* pDevice, const HeapDescription& desc, const std::string& name);
		~VulkanHeap();

		bool create();

		virtual void* getHandle() const override { return m_Allocation; }

	private:
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};
}