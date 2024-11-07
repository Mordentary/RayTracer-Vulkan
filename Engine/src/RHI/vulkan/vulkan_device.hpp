#pragma once
#include "../device.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <queue>
#include <functional>

namespace rhi {
	namespace vulkan {
		class VulkanDevice final : public Device {
		public:
			VulkanDevice(const DeviceDescription& desc);
			~VulkanDevice();

			// Device interface implementation
			virtual void* getHandle() const override { return m_Device; }
			Buffer* createBuffer(const BufferDescription& desc, const void* initialData) override;
			Texture* createTexture(const TextureDescription& desc, const void* initialData) override;
			Swapchain* createSwapchain(const SwapchainDescription& desc) override;
			CommandList* createCommandList(CommandType type) override;
			void submit(CommandList* cmdList, Fence* fence) override;
			void beginFrame() override;
			void endFrame() override;
			uint32_t getCurrentFrameIndex() const override { return m_FrameCount % SE_MAX_FRAMES_IN_FLIGHT; }
			uint32_t getFramesInFlight() const override { return SE_MAX_FRAMES_IN_FLIGHT; }

			Fence* createFence(bool signaled) override;
			void waitForFence(Fence* fence) override;
			void resetFence(Fence* fence) override;
			void waitIdle() override;
			void destroyResource(Resource* resource) override;
			void setDebugName(Resource* resource, const char* name) override;

			// Vulkan-specific accessors needed by other Vulkan RHI classes
			VkDevice getDevice() const { return m_Device; }
			VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
			VmaAllocator getAllocator() const { return m_Allocator; }
			VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
			uint32_t getGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }

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
			VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
			uint32_t m_GraphicsQueueFamily = 0;

			struct FrameData {
				VkCommandPool commandPool;
				VkCommandBuffer commandBuffer;  // Main command buffer

				// Synchronization
				VkFence renderFence;
				VkSemaphore presentSemaphore;  // Signal when image is acquired
				VkSemaphore renderSemaphore;   // Signal when rendering is done

				// Cleanup queue for frame resources
				std::queue<std::function<void()>> cleanupQueue;
			};
			std::array<FrameData, SE_MAX_FRAMES_IN_FLIGHT> m_frames;

			// Resource tracking for cleanup
			std::vector<std::unique_ptr<Resource>> m_ownedResources;
		};
	}
}