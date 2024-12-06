#pragma once

#include "vulkan_core.hpp"
#include "../swapchain.hpp"
#include "vulkan_device.hpp"
class VulkanTexture;

namespace rhi::vulkan
{
	class VulkanSwapchain final : public Swapchain
	{
	public:
		VulkanSwapchain(VulkanDevice* pDevice, const SwapchainDescription& desc, const std::string& name);
		~VulkanSwapchain();

		bool create();
		void present(VkQueue queue);
		VkSemaphore getAcquireSemaphore();
		VkSemaphore getPresentSemaphore();

		virtual void* getHandle() const override { return m_Swapchain; }
		virtual bool acquireNextImage() override;
		virtual Texture* getCurrentSwapchainImage() override;
		virtual bool resize(uint32_t width, uint32_t height) override;
		virtual void setVSync(bool enabled) override;

	private:
		bool createSurface();
		bool createSwapchain();
		bool createSemaphores();
		bool createImages(VkImage* images, uint32_t size);
		bool recreateSwapchain();
		void cleanupSwapchain();
	private:
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		bool m_EnableVsync = true;

		uint32_t m_CurrentSwapchainImage = 0;
		std::vector<SE::Scoped<Texture>> m_SwapchainImages;

		int32_t m_frameSemaphoreIndex = uint32_t(-1);
		std::vector<VkSemaphore> m_AcquireSemaphores;
		std::vector<VkSemaphore> m_PresentSemaphores;
	};
}