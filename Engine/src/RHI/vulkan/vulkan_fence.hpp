#pragma once
#include "../fence.hpp"
#include"vulkan_core.hpp"
namespace rhi::vulkan
{
	class VulkanDevice;

	class VulkanFence : public Fence
	{
	public:
		VulkanFence(VulkanDevice* pDevice, const std::string& name);
		~VulkanFence();

		bool create();

		virtual void* getHandle() const override { return m_Semaphore; }
		virtual void wait(uint64_t value) override;
		virtual void signal(uint64_t value) override;

	private:
		VkSemaphore m_Semaphore = VK_NULL_HANDLE;
	};
}