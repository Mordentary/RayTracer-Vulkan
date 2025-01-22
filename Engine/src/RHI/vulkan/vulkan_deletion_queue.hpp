#pragma once
#include "vulkan_core.hpp"
#include <queue>

namespace rhi::vulkan
{
	class VulkanDevice;

	class VulkanDeletionQueue
	{
	public:
		VulkanDeletionQueue(VulkanDevice* device);
		~VulkanDeletionQueue();

		void flush(bool forceDelete = false);

		template<typename T>
		void enqueue(T object, uint64_t frameID);

		void freeResourceDescriptor(uint32_t index, uint64_t frameID);
		void freeSamplerDescriptor(uint32_t index, uint64_t frameID);

	private:
		VulkanDevice* m_Device = nullptr;
		std::queue<std::pair<VkImage, uint64_t>> m_ImageQueue;
		std::queue<std::pair<VkBuffer, uint64_t>> m_BufferQueue;
		std::queue<std::pair<VmaAllocation, uint64_t>> m_AllocationQueue;
		std::queue<std::pair<VkImageView, uint64_t>> m_ImageViewQueue;
		std::queue<std::pair<VkSampler, uint64_t>> m_SamplerQueue;
		std::queue<std::pair<VkPipeline, uint64_t>> m_PipelineQueue;
		std::queue<std::pair<VkShaderModule, uint64_t>> m_ShaderQueue;
		std::queue<std::pair<VkSemaphore, uint64_t>> m_SemaphoreQueue;
		std::queue<std::pair<VkSwapchainKHR, uint64_t>> m_SwapchainQueue;
		std::queue<std::pair<VkSurfaceKHR, uint64_t>> m_SurfaceQueue;
		std::queue<std::pair<VkCommandPool, uint64_t>> m_CommandPoolQueue;
		std::queue<std::pair<uint32_t, uint64_t>> m_ResourceDescriptorQueue;
		std::queue<std::pair<uint32_t, uint64_t>> m_SamplerDescriptorQueue;
	};

	//General
	template<typename T>
	void VulkanDeletionQueue::enqueue(T object, uint64_t frameID)
	{
		SE_ASSERT(false, "Unsupported type!");
	}
	// specialization

	template<> void VulkanDeletionQueue::enqueue<VkImage>(VkImage object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkBuffer>(VkBuffer object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VmaAllocation>(VmaAllocation object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkImageView>(VkImageView object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkSampler>(VkSampler object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkPipeline>(VkPipeline object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkShaderModule>(VkShaderModule object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkSemaphore>(VkSemaphore object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkSwapchainKHR>(VkSwapchainKHR object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkSurfaceKHR>(VkSurfaceKHR object, uint64_t frameID);
	template<> void VulkanDeletionQueue::enqueue<VkCommandPool>(VkCommandPool object, uint64_t frameID);
}