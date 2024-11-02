#pragma once
#include <vulkan/vulkan.h>
#include <SDL.h>
namespace SE
{
	class IGraphicsBackend {
	public:
		virtual ~IGraphicsBackend() = default;

		struct InitInfo {
			SDL_Window* window;
			bool enableValidation;
		};

		virtual void init(const InitInfo& info) = 0;
		virtual void cleanup() = 0;
		virtual void waitIdle() = 0;

		// Add minimal required functionality first
		virtual VkDevice getDevice() const = 0;
		virtual VkPhysicalDevice getPhysicalDevice() const = 0;
		virtual VkInstance getInstance() const = 0;
		virtual VkQueue getGraphicsQueue() const = 0;
		virtual uint32_t getGraphicsQueueFamily() const = 0;
		//virtual void beginFrame() = 0;
		//virtual void endFrame() = 0;

		//// Add command management
		//virtual VkCommandBuffer getCurrentCommandBuffer() = 0;
		//virtual uint32_t getCurrentFrameIndex() = 0;

		//// Add swapchain access
		//virtual VkFormat getSwapchainFormat() const = 0;
		//virtual VkExtent2D getSwapchainExtent() const = 0;
	};
}