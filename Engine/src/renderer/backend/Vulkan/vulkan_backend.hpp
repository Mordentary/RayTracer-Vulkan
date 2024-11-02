// src/renderer/backend/vulkan/vulkan_backend.hpp
#pragma once
#include "../backend_interface.hpp"
#include "vk_types.h"
#include <vk_mem_alloc.h>

namespace SE {
	class VulkanBackend : public IGraphicsBackend {
	public:
		void init(const InitInfo& info) override;
		void cleanup() override;
		void waitIdle() override;

		// Getters
		VkDevice getDevice() const override { return m_Device; }
		VkPhysicalDevice getPhysicalDevice() const override { return m_PhysicalDevice; }
		VkInstance getInstance() const override { return m_Instance; }
		VkQueue getGraphicsQueue() const override { return m_GraphicsQueue; }
		uint32_t getGraphicsQueueFamily() const override { return m_GraphicsQueueFamily; }

		// Additional Vulkan-specific getters
		VkSurfaceKHR getSurface() const { return m_Surface; }
		VmaAllocator getAllocator() const { return m_Allocator; }
		//void beginFrame() override;
		//void endFrame() override;
		//VkCommandBuffer getCurrentCommandBuffer() override;
		//uint32_t getCurrentFrameIndex() override;

		//// Swapchain access
		//VkFormat getSwapchainFormat() const override { return m_SwapchainFormat; }
		//VkExtent2D getSwapchainExtent() const override { return m_SwapchainExtent; }
	private:
		VkInstance m_Instance{ VK_NULL_HANDLE };
		VkDevice m_Device{ VK_NULL_HANDLE };
		VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
		VkQueue m_GraphicsQueue{ VK_NULL_HANDLE };
		VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
		uint32_t m_GraphicsQueueFamily{ 0 };
		VmaAllocator m_Allocator{ VK_NULL_HANDLE };

		VkDebugUtilsMessengerEXT m_DebugMessenger;

		//// Swapchain
		//VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
		//VkFormat m_SwapchainFormat{ VK_FORMAT_B8G8R8A8_UNORM };
		//VkExtent2D m_SwapchainExtent{};
		//std::vector<VkImage> m_SwapchainImages;
		//std::vector<VkImageView> m_SwapchainImageViews;

		//// Frame data
		//struct FrameData {
		//	VkCommandPool commandPool;
		//	VkCommandBuffer mainCommandBuffer;
		//	VkSemaphore presentSemaphore;
		//	VkSemaphore renderSemaphore;
		//	VkFence renderFence;
		//};
		//static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
		//std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
		//uint32_t m_CurrentFrame = 0;

		//// Core resources
		//struct {
		//	VkFormat format;
		//	AllocatedImage image;
		//	VkExtent2D extent;
		//} m_DrawImage;

		//struct {
		//	VkFormat format;
		//	AllocatedImage image;
		//} m_DepthImage;

		//// Descriptor management
		//VkDescriptorPool m_DescriptorPool{ VK_NULL_HANDLE };
		//std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

		//// Initialization functions
		//void initSwapchain();
		//void initCommandPools();
		//void initSyncStructures();
		//void initDescriptors();
		//void initPipelines();
		//void initRenderTargets();

		//// Swapchain management
		//void cleanupSwapchain();
		//void recreateSwapchain();

		//// Helper functions
		//FrameData& getCurrentFrame() { return m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT]; }

		// Initialize helpers
		void initVulkan(SDL_Window* window, bool enableValidation);
		void initAllocator();
	};
} // namespace SE