#pragma once
#include "../device.hpp"
#include"vulkan_deletion_queue.hpp"
#include "vulkan_core.hpp"

#include <vector>
#include <queue>
#include <functional>

namespace rhi::vulkan {
	class VulkanDevice final : public Device {
	public:
		VulkanDevice(const DeviceDescription& desc);
		~VulkanDevice();

		// Device interface implementation
		virtual void* getHandle() const override { return m_Device; }
		void beginFrame() override;
		void endFrame() override;
		uint32_t getFrameID() const override { return m_FrameID % SE_MAX_FRAMES_IN_FLIGHT; };

		virtual CommandList* createCommandList(CommandType type, const std::string& name) override;
		virtual Swapchain* createSwapchain(const SwapchainDescription& desc, const std::string& name) override;
		virtual Fence* createFence(const std::string& name) override;
		virtual Buffer* createBuffer(const BufferDescription& desc, const std::string& name) override;
		virtual Texture* createTexture(const TextureDescription& desc, const std::string& name) override;

		//Deletion
		template<typename T>
		void enqueueDeletion(T objectHandle);

		// Vulkan-specific accessors needed by other Vulkan RHI classes
		VkDevice getDevice() const { return m_Device; }
		VkInstance getInstance() const { return m_Instance; }
		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
		VmaAllocator getAllocator() const { return m_Allocator; }
		uint32_t getGraphicsQueueIndex() const { return m_GraphicsQueueIndex; }
		uint32_t getComputeQueueIndex() const { return m_ComputeQueueIndex; }
		uint32_t getCopyQueueIndex() const { return m_CopyQueueIndex; }
		VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue getComputeQueue() const { return m_ComputeQueue; }
		VkQueue getCopyQueue() const { return m_CopyQueue; }

		void enqueueDefaultLayoutTransition(Texture* texture);
		void cancelLayoutTransition(Texture* texture);
		void flushLayoutTransition(CommandType type);
	private:
		VulkanDevice() = default;
		bool init(const DeviceDescription& desc);

		// Core Vulkan objects
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		// Queue management

		uint32_t m_GraphicsQueueIndex = uint32_t(-1);
		uint32_t m_ComputeQueueIndex = uint32_t(-1);
		uint32_t m_CopyQueueIndex = uint32_t(-1);
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;
		VkQueue m_CopyQueue = VK_NULL_HANDLE;

		SE::Scoped<VulkanDeletionQueue> m_DeletionQueue = nullptr;
		SE::Scoped<CommandList> m_transitionCopyCommandList[SE_MAX_FRAMES_IN_FLIGHT] = {};
		SE::Scoped<CommandList> m_transitionGraphicsCommandList[SE_MAX_FRAMES_IN_FLIGHT] = {};

		std::vector<std::pair<Texture*, ResourceAccessFlags>> m_pendingGraphicsTransitions;
		std::vector<std::pair<Texture*, ResourceAccessFlags>> m_pendingCopyTransitions;
	};

	template<typename T>
	inline void VulkanDevice::enqueueDeletion(T objectHandle)
	{
		if (objectHandle != VK_NULL_HANDLE)
		{
			m_DeletionQueue->enqueue(objectHandle, m_FrameID);
		}
	}
}