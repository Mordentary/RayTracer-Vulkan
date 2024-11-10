#include "vulkan_fence.hpp"
#include "vulkan_device.hpp"
#include"vulkan_core.hpp"
namespace rhi::vulkan
{
	VulkanFence::VulkanFence(VulkanDevice* device, const std::string& name)
	{
		m_Device = device;
		m_DebugName = name;
	}

	VulkanFence::~VulkanFence()
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Semaphore);
	}

	bool VulkanFence::create()
	{
		VkDevice device = ((VulkanDevice*)m_Device)->getDevice();

		VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
		timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		timelineCreateInfo.initialValue = 0;

		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		createInfo.pNext = &timelineCreateInfo;

		VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &m_Semaphore);
		if (result != VK_SUCCESS)
		{
			SE::LogError("VulkanFence creation is failed: {}", m_DebugName);
			return false;
		}

		setDebugName(device, VK_OBJECT_TYPE_SEMAPHORE, m_Semaphore, m_DebugName.c_str());

		return true;
	}

	void VulkanFence::wait(uint64_t value)
	{
		VkSemaphoreWaitInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
		info.semaphoreCount = 1;
		info.pSemaphores = &m_Semaphore;
		info.pValues = &value;

		vkWaitSemaphores((VkDevice)m_Device->getHandle(), &info, UINT64_MAX);
	}

	void VulkanFence::signal(uint64_t value)
	{
		VkSemaphoreSignalInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
		info.semaphore = m_Semaphore;
		info.value = value;

		vkSignalSemaphore((VkDevice)m_Device->getHandle(), &info);
	}
}