#pragma once

#include <queue>
#include "vulkan_core.hpp"

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

		//TODO:
		std::queue<std::pair<uint32_t, uint64_t>> m_ResourceDescriptorQueue;
		std::queue<std::pair<uint32_t, uint64_t>> m_SamplerDescriptorQueue;
	};
}