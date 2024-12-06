#include "vulkan_deletion_queue.hpp"
#include "vulkan_device.hpp"

namespace rhi::vulkan
{
	VulkanDeletionQueue::VulkanDeletionQueue(VulkanDevice* device) : m_Device(device)
	{
	}

	VulkanDeletionQueue::~VulkanDeletionQueue()
	{
		flush(true);
	}

	void VulkanDeletionQueue::flush(bool forceDelete)
	{
		uint64_t frameID = m_Device->getFrameID();
		VkInstance instance = m_Device->getInstance();
		VkDevice device = m_Device->getDevice();
		VmaAllocator allocator = m_Device->getVmaAllocator();

		auto processQueue = [&](auto& queue, auto deleteFunc) {
			while (!queue.empty()) {
				auto& item = queue.front();
				if (!forceDelete && item.second + SE::SE_MAX_FRAMES_IN_FLIGHT > frameID) {
					break;
				}
				deleteFunc(device, item.first, nullptr);
				queue.pop();
			}
			};

		processQueue(m_ImageQueue, vkDestroyImage);
		processQueue(m_BufferQueue, vkDestroyBuffer);
		processQueue(m_ImageViewQueue, vkDestroyImageView);
		processQueue(m_SamplerQueue, vkDestroySampler);
		processQueue(m_PipelineQueue, vkDestroyPipeline);
		processQueue(m_ShaderQueue, vkDestroyShaderModule);
		processQueue(m_SemaphoreQueue, vkDestroySemaphore);
		processQueue(m_SwapchainQueue, vkDestroySwapchainKHR);
		processQueue(m_CommandPoolQueue, vkDestroyCommandPool);

		// Surface deletion
		while (!m_SurfaceQueue.empty()) {
			auto item = m_SurfaceQueue.front();
			if (!forceDelete && item.second + SE::SE_MAX_FRAMES_IN_FLIGHT > frameID) {
				break;
			}
			vkDestroySurfaceKHR(instance, item.first, nullptr);
			m_SurfaceQueue.pop();
		}

		// VMA allocation deletion
		while (!m_AllocationQueue.empty()) {
			auto item = m_AllocationQueue.front();
			if (!forceDelete && item.second + SE::SE_MAX_FRAMES_IN_FLIGHT > frameID) {
				break;
			}
			vmaFreeMemory(allocator, item.first);
			m_AllocationQueue.pop();
		}

		// Resource descriptor deletion
		while (!m_ResourceDescriptorQueue.empty()) {
			auto item = m_ResourceDescriptorQueue.front();
			if (!forceDelete && item.second + SE::SE_MAX_FRAMES_IN_FLIGHT > frameID) {
				break;
			}
			m_Device->getResourceDescriptorAllocator()->free(item.first);
			m_ResourceDescriptorQueue.pop();
		}

		// Sampler descriptor deletion
		while (!m_SamplerDescriptorQueue.empty()) {
			auto item = m_SamplerDescriptorQueue.front();
			if (!forceDelete && item.second + SE::SE_MAX_FRAMES_IN_FLIGHT > frameID) {
				break;
			}
			m_Device->getSamplerDescriptorAllocator()->free(item.first);
			m_SamplerDescriptorQueue.pop();
		}
	}

	void VulkanDeletionQueue::freeResourceDescriptor(uint32_t index, uint64_t frameID)
	{
		m_ResourceDescriptorQueue.push(std::make_pair(index, frameID));
	}

	void VulkanDeletionQueue::freeSamplerDescriptor(uint32_t index, uint64_t frameID)
	{
		m_SamplerDescriptorQueue.push(std::make_pair(index, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkImage object, uint64_t frameID)
	{
		m_ImageQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkBuffer object, uint64_t frameID)
	{
		m_BufferQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VmaAllocation object, uint64_t frameID)
	{
		m_AllocationQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkImageView object, uint64_t frameID)
	{
		m_ImageViewQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkSampler object, uint64_t frameID)
	{
		m_SamplerQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkPipeline object, uint64_t frameID)
	{
		m_PipelineQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkShaderModule object, uint64_t frameID)
	{
		m_ShaderQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkSemaphore object, uint64_t frameID)
	{
		m_SemaphoreQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkSwapchainKHR object, uint64_t frameID)
	{
		m_SwapchainQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkSurfaceKHR object, uint64_t frameID)
	{
		m_SurfaceQueue.push(std::make_pair(object, frameID));
	}

	template<>
	void VulkanDeletionQueue::enqueue(VkCommandPool object, uint64_t frameID)
	{
		m_CommandPoolQueue.push(std::make_pair(object, frameID));
	}
}