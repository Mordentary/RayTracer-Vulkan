#include "vulkan_swapchain.hpp"
#include "vulkan_texture.hpp"

namespace rhi::vulkan {
	VulkanSwapchain::VulkanSwapchain(VulkanDevice* pDevice, const SwapchainDescription& desc,
		const std::string& name)
	{
		m_Device = pDevice;
		m_Description = desc;
		m_DebugName = name;
	}

	VulkanSwapchain::~VulkanSwapchain() {
		cleanupSwapchain();
	}

	bool VulkanSwapchain::create() {
		bool result = createSurface();
		if (result) result = createSwapchain();
		if (result) result = createSemaphores();
		SE_ASSERT(result, "Swapchain creation failed! {}", m_DebugName);
		return result;
	}

	bool VulkanSwapchain::createSurface() {
		return SDL_Vulkan_CreateSurface(
			(SDL_Window*)m_Description.windowHandle,
			((VulkanDevice*)m_Device)->getInstance(),
			&m_Surface
		) == SDL_TRUE;
	}

	bool VulkanSwapchain::createSwapchain() {
		VulkanDevice* device = (VulkanDevice*)m_Device;

		vkb::SwapchainBuilder builder{
			device->getPhysicalDevice(),
			device->getDevice(),
			m_Surface
		};
		VkSwapchainKHR oldSwapchain = m_Swapchain;

		auto vkbSwapchain = builder
			.set_desired_format({ toVkFormat(m_Description.format), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(m_EnableVsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
			.set_desired_extent(m_Description.width, m_Description.height)
			.add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.set_old_swapchain(oldSwapchain)
			.build();

		if (!vkbSwapchain) {
			return false;
		}

		m_Swapchain = vkbSwapchain->swapchain;

		setDebugName(device->getDevice(), VK_OBJECT_TYPE_SWAPCHAIN_KHR, m_Swapchain, m_DebugName.c_str());
		createImages(vkbSwapchain->get_images().value().data(), vkbSwapchain->image_count);

		return true;
	}

	bool VulkanSwapchain::createImages(VkImage* images, uint32_t size) {
		TextureDescription textureDesc{};
		textureDesc.width = m_Description.width;
		textureDesc.height = m_Description.height;
		textureDesc.format = m_Description.format;
		textureDesc.usage = TextureUsageFlags::RenderTarget;

		m_SwapchainImages.resize(size);
		for (size_t i = 0; i < size; ++i) {
			std::string name = fmt::format("{} texture {}", m_DebugName, i);
			SE::Scoped<VulkanTexture> image = SE::CreateScoped<VulkanTexture>((VulkanDevice*)m_Device, textureDesc, name);
			if (!image->create(images[i]))
				return false;
			m_SwapchainImages[i] = std::move(image);
		}

		return true;
	}

	bool VulkanSwapchain::createSemaphores() {
		m_AcquireSemaphores.clear();
		m_PresentSemaphores.clear();
		VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		for (uint32_t i = 0; i < m_SwapchainImages.size(); i++) {
			VkSemaphore acquire, present;

			if (vkCreateSemaphore((VkDevice)m_Device->getHandle(), &semaphoreInfo, nullptr, &acquire) != VK_SUCCESS ||
				vkCreateSemaphore((VkDevice)m_Device->getHandle(), &semaphoreInfo, nullptr, &present) != VK_SUCCESS) {
				return false;
			}

			m_AcquireSemaphores.push_back(acquire);
			m_PresentSemaphores.push_back(present);
		}

		return true;
	}

	void VulkanSwapchain::present(VkQueue queue) {
		VkSemaphore waitSemaphore = getPresentSemaphore();

		VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &waitSemaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &m_Swapchain;
		info.pImageIndices = &m_CurrentSwapchainImage;

		VkResult result = vkQueuePresentKHR(queue, &info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			{
				recreateSwapchain();
			}
		}
	}

	bool VulkanSwapchain::resize(uint32_t width, uint32_t height) {
		if (width == m_Description.width && height == m_Description.height) {
			return false;
		}

		m_Description.width = width;
		m_Description.height = height;

		return recreateSwapchain();
	}

	bool VulkanSwapchain::recreateSwapchain() {
		vkDeviceWaitIdle((VkDevice)m_Device->getHandle());
		for (auto& image : m_SwapchainImages)
		{
			image.reset();
		}
		m_SwapchainImages.clear();

		VulkanDevice* device = (VulkanDevice*)m_Device;
		//device->enqueueDeletion(m_Swapchain);
		//device->enqueueDeletion(m_Surface);

		for (auto semaphore : m_AcquireSemaphores)
		{
			device->enqueueDeletion(semaphore);
		}

		for (auto semaphore : m_PresentSemaphores)
		{
			device->enqueueDeletion(semaphore);
		}
		return createSwapchain() && createSemaphores();
	}

	void VulkanSwapchain::cleanupSwapchain()
	{
		for (auto& image : m_SwapchainImages)
		{
			image.reset();
		}
		m_SwapchainImages.clear();

		VulkanDevice* device = (VulkanDevice*)m_Device;
		device->enqueueDeletion(m_Swapchain);
		device->enqueueDeletion(m_Surface);

		for (auto semaphore : m_AcquireSemaphores)
		{
			device->enqueueDeletion(semaphore);
		}

		for (auto semaphore : m_PresentSemaphores)
		{
			device->enqueueDeletion(semaphore);
		}
	}

	bool VulkanSwapchain::acquireNextImage()
	{
		m_frameSemaphoreIndex = (m_frameSemaphoreIndex + 1) % m_AcquireSemaphores.size();
		VkSemaphore signalSemaphore = getAcquireSemaphore();

		VkResult result = vkAcquireNextImageKHR((VkDevice)m_Device->getHandle(), m_Swapchain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentSwapchainImage);

		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapchain();

			result = vkAcquireNextImageKHR((VkDevice)m_Device->getHandle(), m_Swapchain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentSwapchainImage);
			SE_ASSERT(result == VK_SUCCESS, "Acquiring image failed!{}", m_DebugName);
		}

		return result;
	}

	VkSemaphore VulkanSwapchain::getAcquireSemaphore() {
		return m_AcquireSemaphores[m_frameSemaphoreIndex];
	}

	VkSemaphore VulkanSwapchain::getPresentSemaphore() {
		return m_PresentSemaphores[m_frameSemaphoreIndex];
	}

	ITexture* VulkanSwapchain::getCurrentSwapchainImage() {
		return m_SwapchainImages[m_CurrentSwapchainImage].get();
	}

	void VulkanSwapchain::setVSync(bool enabled) {
		if (m_EnableVsync != enabled) {
			m_EnableVsync = enabled;
			recreateSwapchain();
		}
	}
}