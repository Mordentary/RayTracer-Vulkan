﻿#include "vk_engine.h"

#include "vk_loader.h"
#include "vk_images.h"
#include "vk_pipelines.h"

#include <sstream>
#include <string>
#include <regex>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <imgui_internal.h>
namespace SE
{
#ifdef _DEBUG
	constexpr bool bUseValidationLayers = true;
#else
	constexpr bool bUseValidationLayers = false;
#endif

	// Debug callback function
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		if (!pCallbackData || !pCallbackData->pMessage) {
			return VK_FALSE;
		}

		std::string message(pCallbackData->pMessage);

		// Define styles for different parts of the message
		fmt::text_style severityStyle;
		std::string severityStr;

		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severityStyle = fg(fmt::color::gray) | fmt::emphasis::bold;
			severityStr = "VERBOSE";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severityStyle = fg(fmt::color::white) | fmt::emphasis::bold;
			severityStr = "INFO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severityStyle = fg(fmt::color::orange) | fmt::emphasis::bold;
			severityStr = "WARNING";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severityStyle = fg(fmt::color::red) | fmt::emphasis::bold;
			severityStr = "ERROR";
			break;
		default:
			severityStyle = fg(fmt::color::white) | fmt::emphasis::bold;
			severityStr = "UNKNOWN";
		}

		const auto objectStyle = fg(fmt::color::yellow);
		const auto messageIdStyle = fg(fmt::color::blue);
		const auto descriptionStyle = fg(fmt::color::white);
		const auto highlightStyle = fmt::emphasis::bold; // Removed color to reduce clutter
		const auto errorStyle = fg(fmt::color::red);
		const auto warningStyle = fg(fmt::color::orange);

		// Print the severity
		fmt::print("\n[{}]\n\n", fmt::styled(severityStr, severityStyle));

		// Apply styles to the entire message based on severity
		fmt::text_style messageStyle = descriptionStyle;
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			messageStyle |= errorStyle;
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			messageStyle |= warningStyle;
		}

		// Extract and print message content
		std::istringstream iss(message);
		std::string line;
		while (std::getline(iss, line)) {
			Engine* loadedEngine = nullptr;
			// Apply regex-based highlighting
			auto highlightRegex = [&](const std::regex& pattern, const fmt::text_style& style) {
				std::string newLine;
				std::sregex_iterator begin(line.begin(), line.end(), pattern);
				std::sregex_iterator end;
				size_t lastPos = 0;

				for (auto it = begin; it != end; ++it) {
					auto match = *it;
					newLine += line.substr(lastPos, match.position() - lastPos);
					// Corrected line using fmt::styled
					newLine += fmt::format("{}", fmt::styled(match.str(), style));
					lastPos = match.position() + match.length();
				}
				newLine += line.substr(lastPos);
				line = newLine;
				};

			// Highlight specific patterns
			highlightRegex(std::regex(R"(Validation (Warning|Error):)"), highlightStyle);
			highlightRegex(std::regex(R"(Object \d+:)"), objectStyle);
			//highlightRegex(std::regex(R"(MessageID\s*=\s*0x[0-9a-fA-F]+)"), messageIdStyle);
			highlightRegex(std::regex(R"(UNASSIGNED-[\w-]+)"), fmt::emphasis::bold);
			highlightRegex(std::regex(R"(OBJ ERROR :)"), fmt::emphasis::bold);
			highlightRegex(std::regex(R"(type\s*=\s*VK_OBJECT_TYPE_\w+)"), objectStyle);
			highlightRegex(std::regex(R"(handle\s*=\s*0x[0-9a-fA-F]+)"), objectStyle);

			// Print the formatted line with the message style
			fmt::print("  {}\n", fmt::styled(line, messageStyle));
		}

		fmt::print("{}\n", fmt::styled(std::string(80, '-'), fg(fmt::color::dark_gray)));

		return VK_FALSE;
	}

	Engine::Engine()
	{
		init();
	}

	Engine::~Engine()
	{
		//cleanup();
	}

	VkDevice Engine::getDevice()
	{
		return m_Device;
	}

	VkDescriptorSetLayout Engine::getSceneDescriptorLayout()
	{
		return m_SceneDescriptorLayout;
	}

	Engine* loadedEngine;
	// Initialization
	void Engine::init()
	{
		// Only one engine initialization is allowed with the application.
		assert(loadedEngine == nullptr);
		loadedEngine = this;

		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		m_Window = SDL_CreateWindow(
			"Vulkan Engine",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			m_WindowExtent.width,
			m_WindowExtent.height,
			windowFlags);

		m_Camera = CreateShared<Camera>();
		initVulkan();

		initSwapchain();

		initCommandBuffers();

		initSyncStructures();

		initDescriptors();

		initPipelines();

		m_MaterialSystem.initialize(this);

		initDefaultData();

		initScene();

		initImgui();

		m_IsInitialized = true;
		Timer::getInstance().reset();
	}

	// Cleanup
	void Engine::cleanup()
	{
		if (m_IsInitialized)
		{
			vkDeviceWaitIdle(m_Device);
			m_MaterialSystem.cleanup(m_Device);
			m_LoadedNodes.clear();
			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vkDestroyCommandPool(m_Device, m_Frames[i].commandPool, nullptr);

				vkDestroyFence(m_Device, m_Frames[i].renderFence, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].renderSemaphore, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].swapchainSemaphore, nullptr);

				m_Frames[i].cleanupQueue.executeCleanup();
			}
			m_MainCleanupQueue.executeCleanup();

			destroySwapchain();
			vkDestroyDevice(m_Device, nullptr);
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
			vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);

			vkDestroyInstance(m_Instance, nullptr);

			SDL_DestroyWindow(m_Window);
		}

		// Clear engine pointer
		loadedEngine = nullptr;
	}

	// Frame Drawing
	void Engine::drawFrame()
	{
		{
			SCOPED_TIMER_COLORED("Frame Sync", ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
			VK_CHECK(vkWaitForFences(m_Device, 1, &getCurrentFrame().renderFence, VK_TRUE, 1000000000));
			VK_CHECK(vkResetFences(m_Device, 1, &getCurrentFrame().renderFence));
		}

		uint32_t swapchainImageIndex;
		{
			SCOPED_TIMER_COLORED("Acquire Swapchain", ImVec4(0.2f, 0.7f, 0.7f, 1.0f));
			VkResult e = vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000,
				getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
			if (e == VK_ERROR_OUT_OF_DATE_KHR) {
				m_ResizeRequested = true;
				return;
			}
		}

		//m_DrawExtent.height = std::min(m_SwapchainExtent.height, m_DrawImage.extent.height) * m_RenderScale;
		//m_DrawExtent.width = std::min(m_SwapchainExtent.width, m_DrawImage.extent.width) * m_RenderScale;

		VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;
		VK_CHECK(vkResetCommandBuffer(cmd, 0));
		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// Background pass (compute)
		vkUtil::transitionImage(cmd, m_DrawImage.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL);  // Compute shaders need GENERAL layout
		drawBackground(cmd);

		// Main geometry pass
		vkUtil::transitionImage(cmd, m_DrawImage.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkUtil::transitionImage(cmd, m_DepthImage.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		drawGeometry(cmd);

		// After geometry rendering, transition to sampled for ImGui viewport
		vkUtil::transitionImage(cmd, m_DrawImage.image,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		//// Draw to swapchain
		//vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex],
		//	VK_IMAGE_LAYOUT_UNDEFINED,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		//// Blit rendered image to swapchain
		//vkUtil::copyImageToImage(cmd,
		//	m_DrawImage.image,
		//	m_SwapchainImages[swapchainImageIndex],
		//	m_DrawExtent, m_DrawExtent);

		// Draw ImGui
		vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		drawImgui(cmd, m_SwapchainImageViews[swapchainImageIndex]);

		// Prepare for presentation
		vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdInfo = vkInit::commandBufferSubmitInfo(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkInit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkInit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

		VkSubmitInfo2 submit = vkInit::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

		// Submit command buffer to the queue and execute it.
		// renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, getCurrentFrame().renderFence));

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		VkResult presentResult = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
			m_ResizeRequested = true;
		}

		m_CurrentFrame++;
	}

	void Engine::run() {
		bool bQuit = false;
		SDL_Event e;
		auto lastFrame = std::chrono::high_resolution_clock::now();
		bool isViewportHovered = false;
		while (!bQuit) {
			Timer::getInstance().beginFrame();
			auto currentFrame = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
			lastFrame = currentFrame;

			// Handle events
			while (SDL_PollEvent(&e) != 0) {
				ImGui_ImplSDL2_ProcessEvent(&e);

				if (e.type == SDL_QUIT) {
					bQuit = true;
				}
				else if (e.type == SDL_WINDOWEVENT) {
					if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
						m_StopRendering = true;
					}
					else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
						m_StopRendering = false;
					}
					else if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
						m_ResizeRequested = true;

						// Update m_WindowExtent immediately
						m_WindowExtent.width = static_cast<uint32_t>(e.window.data1);
						m_WindowExtent.height = static_cast<uint32_t>(e.window.data2);
					}
				}
				// Only pass events to camera if ImGui isn't capturing them and viewport is hovered
				if (isViewportHovered)
					m_Camera->handleEvent(e);
			}

			if (isViewportHovered && !ImGui::GetIO().WantCaptureKeyboard) {
				const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
				m_Camera->processKeyboard(keyboardState, deltaTime);
			}
			if (m_ResizeRequested) {
				resizeSwapchain();
			}

			if (m_StopRendering) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			// Start ImGui frame
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			// Create the dockspace
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			ImGui::Begin("DockSpace", nullptr, window_flags);
			ImGui::PopStyleVar(3);

			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("Windows")) {
					ImGui::MenuItem("Main Viewport", NULL, &m_ShowViewport);
					ImGui::MenuItem("Metrics", NULL, &m_ShowMetricsWindow);
					ImGui::MenuItem("Style Editor", NULL, &m_ShowStyleEditor);
					ImGui::MenuItem("Stats", NULL, &m_ShowStatsWindow);
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::End(); // DockSpace

			if (m_ShowViewport)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				if (ImGui::Begin("Viewport", &m_ShowViewport,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
				{
					// Get current viewport size
					ImVec2 viewportSize = ImGui::GetContentRegionAvail();
					isViewportHovered = ImGui::IsWindowHovered();
					// Apply minimum size to prevent tiny images
					float newWidth = std::max(viewportSize.x, 32.0f);
					float newHeight = std::max(viewportSize.y, 32.0f);

					// Check if size changed significantly
					const float resizeThreshold = 1.0f;  // Prevents constant minor resizing
					bool sizeChanged =
						std::abs(m_ViewpotrSize.x - newWidth) > resizeThreshold ||
						std::abs(m_ViewpotrSize.y - newHeight) > resizeThreshold;

					if (sizeChanged && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
					{
						m_ViewpotrSize.x = newWidth;
						m_ViewpotrSize.y = newHeight;
						resizeDrawImage();  // This will use the new dimensions
					}

					// Draw the viewport image
					ImGui::Image(
						(ImTextureID)(uint64_t)m_ImguiDrawImageDescriptor,
						viewportSize,
						ImVec2(0, 0),
						ImVec2(1, 1),
						ImVec4(1, 1, 1, 1),
						ImVec4(0, 0, 0, 0));
				}
				ImGui::End();
				ImGui::PopStyleVar();
			}

			// Your other windows
			if (ImGui::Begin("Background", nullptr)) {
				ImGui::SliderFloat("Render Scale", &m_RenderScale, 0.3f, 1.f);
				ComputeEffect& selected = m_BackgroundEffects[m_ActiveBackgroundEffect];
				ImGui::Text("Selected effect: %s", selected.name.c_str());
				ImGui::SliderInt("Effect Index", &m_ActiveBackgroundEffect, 0,
					static_cast<int>(m_BackgroundEffects.size()) - 1);

				ImGui::InputFloat4("data1", glm::value_ptr(selected.pushConstants.data1));
				ImGui::InputFloat4("data2", glm::value_ptr(selected.pushConstants.data2));
				ImGui::InputFloat4("data3", glm::value_ptr(selected.pushConstants.data3));
				ImGui::InputFloat4("data4", glm::value_ptr(selected.pushConstants.data4));
			}
			ImGui::End();

			if (ImGui::Begin("Debug", nullptr)) {
				static bool showTimer = true;
				if (ImGui::Button("Toggle Performance Timer")) {
					showTimer = !showTimer;
				}

				if (showTimer) {
					Timer::getInstance().drawImGuiWindow(&showTimer);
				}
			}
			ImGui::End();

			// Demo windows
			if (m_ShowMetricsWindow)
				ImGui::ShowMetricsWindow(&m_ShowMetricsWindow);
			if (m_ShowStyleEditor) {
				ImGui::Begin("Style Editor", &m_ShowStyleEditor);
				ImGui::ShowStyleEditor();
				ImGui::End();
			}
			ImGui::Render();

			// Update and render scene
			{
				SCOPED_TIMER_COLORED("Scene Update", ImVec4(0.2f, 0.8f, 0.8f, 1.0f));
				updateScene();
			}

			{
				SCOPED_TIMER_COLORED("Frame Draw", ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				drawFrame();
			}

			// Finish ImGui Frame
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			Timer::getInstance().endFrame();
		}
	}

	void Engine::updateScene()
	{
		m_SceneData.view = m_Camera->getViewMatrix();
		m_SceneData.projection = m_Camera->getProjectionMatrix();
		m_SceneData.viewProjection = m_SceneData.projection * m_SceneData.view;

		//some default lighting parameters
		m_SceneData.ambientColor = glm::vec4(.1f);
		m_SceneData.sunlightColor = glm::vec4(1.f);
		m_SceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);

		m_MainRenderQueue.clear();

		m_LoadedNodes["structure"]->draw(glm::mat4{ 1.f }, m_MainRenderQueue);
	}

	void Engine::initScene()
	{
		auto structureFile = GLTFLoader::loadGLTF(this, "assets/structure.glb");
		assert(structureFile.has_value());
		m_LoadedNodes["structure"] = *structureFile;
	}

	// Immediate Submit Helper
	void Engine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VK_CHECK(vkResetFences(m_Device, 1, &m_ImguiFence));
		VK_CHECK(vkResetCommandBuffer(m_ImguiCommandBuffer, 0));

		VkCommandBuffer cmd = m_ImguiCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdInfo = vkInit::commandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = vkInit::submitInfo(&cmdInfo, nullptr, nullptr);

		// Submit command buffer to the queue and execute it.
		// imguiFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImguiFence));

		VK_CHECK(vkWaitForFences(m_Device, 1, &m_ImguiFence, VK_TRUE, 9999999999));
	}

	// Resize Swapchain
	void Engine::resizeSwapchain()
	{
		int width, height;
		SDL_GetWindowSize(m_Window, &width, &height);
		m_WindowExtent.width = static_cast<uint32_t>(width);
		m_WindowExtent.height = static_cast<uint32_t>(height);

		vkDeviceWaitIdle(m_Device);

		// Cleanup old swapchain
		destroySwapchain();

		// Create new swapchain with updated dimensions
		createSwapchain(m_WindowExtent.width, m_WindowExtent.height);

		// Resize draw image if needed
		resizeDrawImage();

		m_Camera->updateAspectRatio(m_ViewpotrSize.x, m_ViewpotrSize.y);
		m_ResizeRequested = false;
	}

	void Engine::resizeDrawImage() {
		// Wait for all GPU operations to complete before resizing
		VK_CHECK(vkDeviceWaitIdle(m_Device));

		// Destroy old resources
		if (m_DrawImage.imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_Device, m_DrawImage.imageView, nullptr);
			m_DrawImage.imageView = VK_NULL_HANDLE;
		}
		if (m_DrawImage.image != VK_NULL_HANDLE) {
			vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);
			m_DrawImage.image = VK_NULL_HANDLE;
		}

		if (m_DepthImage.imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_Device, m_DepthImage.imageView, nullptr);
			m_DepthImage.imageView = VK_NULL_HANDLE;
		}
		if (m_DepthImage.image != VK_NULL_HANDLE) {
			vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);
			m_DepthImage.image = VK_NULL_HANDLE;
		}

		// Calculate new draw extent based on viewport size
		VkExtent2D newDrawExtent = {
			static_cast<uint32_t>(m_ViewpotrSize.x * m_RenderScale),
			static_cast<uint32_t>(m_ViewpotrSize.y * m_RenderScale)
		};

		// Apply reasonable size limits
		const uint32_t MAX_DIMENSION = 8192;  // Typical max texture dimension
		newDrawExtent.width = std::clamp(newDrawExtent.width, 1u, MAX_DIMENSION);
		newDrawExtent.height = std::clamp(newDrawExtent.height, 1u, MAX_DIMENSION);

		if (newDrawExtent.width == m_DrawExtent.width &&
			newDrawExtent.height == m_DrawExtent.height) {
			return;
		}

		m_DrawExtent = newDrawExtent;

		// Create new draw image
		VkExtent3D extent3D = { m_DrawExtent.width, m_DrawExtent.height, 1 };

		VkImageUsageFlags drawFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		// Create color image with error checking
		VkImageCreateInfo colorInfo = vkInit::imageCreateInfo(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			drawFlags,
			extent3D);

		VkResult result = vmaCreateImage(m_Allocator, &colorInfo, &allocInfo,
			&m_DrawImage.image, &m_DrawImage.allocation, nullptr);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create draw image");
		}

		VkImageViewCreateInfo colorViewInfo = vkInit::imageviewCreateInfo(
			VK_FORMAT_R16G16B16A16_SFLOAT,
			m_DrawImage.image,
			VK_IMAGE_ASPECT_COLOR_BIT);

		result = vkCreateImageView(m_Device, &colorViewInfo, nullptr, &m_DrawImage.imageView);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create draw image view");
		}

		// Create depth image
		VkImageCreateInfo depthInfo = vkInit::imageCreateInfo(
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			extent3D);

		result = vmaCreateImage(m_Allocator, &depthInfo, &allocInfo,
			&m_DepthImage.image, &m_DepthImage.allocation, nullptr);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create depth image");
		}

		VkImageViewCreateInfo depthViewInfo = vkInit::imageviewCreateInfo(
			VK_FORMAT_D32_SFLOAT,
			m_DepthImage.image,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		result = vkCreateImageView(m_Device, &depthViewInfo, nullptr, &m_DepthImage.imageView);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create depth image view");
		}

		if (m_ImguiDrawImageDescriptor != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(m_ImguiDrawImageDescriptor);
		}

		m_ImguiDrawImageDescriptor = ImGui_ImplVulkan_AddTexture(
			m_DefaultEngineData.samplerNearest,
			m_DrawImage.imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// **Update m_BackgroundShaderDescriptorSet**
		{
			DescriptorWriter writer;
			writer.writeImage(
				0,
				m_DrawImage.imageView,
				VK_NULL_HANDLE,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			);
			writer.updateSet(m_Device, m_BackgroundShaderDescriptorSet);
		}
	}
	// Initialize Default Data
	void Engine::initDefaultData()
	{
		uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
		m_DefaultEngineData.whiteImage = createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
		m_DefaultEngineData.greyImage = createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
		m_DefaultEngineData.blackImage = createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

		// Checkerboard image
		uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		std::array<uint32_t, 16 * 16> pixels; // For 16x16 checkerboard texture
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
			}
		}
		m_DefaultEngineData.errorCheckerboardImage = createImage(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);
		VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		// TODO: INVESTIGATE WHY THIS FIXES
		immediateSubmit([&](VkCommandBuffer cmd) {
			// Ensure default textures are in correct layout
			vkUtil::transitionImage(cmd, m_DefaultEngineData.whiteImage.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			vkUtil::transitionImage(cmd, m_DefaultEngineData.blackImage.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			vkUtil::transitionImage(cmd, m_DefaultEngineData.greyImage.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			vkUtil::transitionImage(cmd, m_DefaultEngineData.errorCheckerboardImage.image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;

		VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_DefaultEngineData.samplerNearest));

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_DefaultEngineData.samplerLinear));

		m_MainCleanupQueue.enqueueCleanup([=]() {
			vkDestroySampler(m_Device, m_DefaultEngineData.samplerNearest, nullptr);
			vkDestroySampler(m_Device, m_DefaultEngineData.samplerLinear, nullptr);

			destroyImage(m_DefaultEngineData.whiteImage);
			destroyImage(m_DefaultEngineData.greyImage);
			destroyImage(m_DefaultEngineData.blackImage);
			destroyImage(m_DefaultEngineData.errorCheckerboardImage); });

		//Create default material
		MaterialFactory::Resources materialResources;
		materialResources.albedoTexture = m_DefaultEngineData.whiteImage;
		materialResources.metallicRoughnessTexture = m_DefaultEngineData.whiteImage;
		materialResources.sampler = m_DefaultEngineData.samplerLinear;

		// Set the uniform buffer for the material data
		AllocatedBuffer materialConstants = createBuffer(sizeof(MaterialFactory::Constants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Write the buffer
		VmaAllocationInfo info;
		vmaGetAllocationInfo(m_Allocator, materialConstants.allocation, &info);
		MaterialFactory::Constants* materialUniformData = (MaterialFactory::Constants*)info.pMappedData;
		materialUniformData->baseColorFactor = glm::vec4{ 1,1,1,1 };
		materialUniformData->metallicRoughnessFactors = glm::vec4{ 1,0.5,0,0 };

		m_MainCleanupQueue.enqueueCleanup([=, this]() {
			destroyBuffer(materialConstants);
			});

		materialResources.uniformBuffer = materialConstants.buffer;
		materialResources.uniformBufferOffset = 0;

		m_DefaultEngineData.defaultMaterial = CreateShared<MaterialInstance>(MaterialFactory::createInstance(this, MaterialPass::MainColor, materialResources));
	}

	// Initialize Vulkan
	void Engine::initVulkan()
	{
		vkb::InstanceBuilder builder;

		// Check for Khronos validation layer
		const char* khronosValidationLayer = "VK_LAYER_KHRONOS_validation";
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool khronosValidationAvailable = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(khronosValidationLayer, layerProperties.layerName) == 0) {
				khronosValidationAvailable = true;
				break;
			}
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;

		auto inst_ret = builder.set_app_name("SingularityEngine")
			.request_validation_layers(bUseValidationLayers)
			.set_debug_callback(debugCallback)
			.set_debug_callback_user_data_pointer(&createInfo)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
			.require_api_version(1, 3, 0);

		// Enable Khronos validation layer if available
		if (khronosValidationAvailable && bUseValidationLayers) {
			builder.enable_layer(khronosValidationLayer);
		}

		// Only add debug utils extension in debug builds
#ifdef _DEBUG
		builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		auto built_instance = builder.build();
		if (!built_instance) {
			throw std::runtime_error("Failed to create Vulkan instance!");
		}

		auto vkb_inst = built_instance.value();
		m_Instance = vkb_inst.instance;
		m_DebugMessenger = vkb_inst.debug_messenger;

		SDL_Vulkan_CreateSurface(m_Window, m_Instance, &m_Surface);

		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;

		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = VK_TRUE;
		features12.descriptorIndexing = VK_TRUE;

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice vkbPhysicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_surface(m_Surface)
			.select()
			.value();

		vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();
		m_PhysicalDevice = vkbPhysicalDevice.physical_device;
		m_Device = vkbDevice.device;

		m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_GraphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_Device;
		allocatorInfo.instance = m_Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		m_MainCleanupQueue.enqueueCleanup([&]()
			{
				vmaDestroyAllocator(m_Allocator);
			});
	}

	// Initialize Swapchain
	void Engine::initSwapchain()
	{
		createSwapchain(m_WindowExtent.width, m_WindowExtent.height);

		VkExtent3D renderExtent = {
			m_WindowExtent.width,
			m_WindowExtent.height,
			1
		};
		m_DrawExtent.width = static_cast<uint32_t>(1);
		m_DrawExtent.height = static_cast<uint32_t>(1);
		// draw image creation
		m_DrawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		m_DrawImage.extent = renderExtent;

		VkImageUsageFlags drawImageFlags = 0;

		drawImageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo imageInfo = vkInit::imageCreateInfo(m_DrawImage.format, drawImageFlags, renderExtent);

		VmaAllocationCreateInfo img_allocInfo{};
		img_allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		img_allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		// Allocate and create the image
		VmaAllocation allocation;
		VK_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &img_allocInfo, &m_DrawImage.image, &allocation, nullptr));
		m_DrawImage.allocation = allocation;

		// If the format is a depth format, we will need to have it use the correct aspect flag
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_DrawImage.format == VK_FORMAT_D32_SFLOAT) {
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// Build an image-view for the image
		VkImageViewCreateInfo viewDrawImgCreateInfo = vkInit::imageviewCreateInfo(m_DrawImage.format, m_DrawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(m_Device, &viewDrawImgCreateInfo, nullptr, &m_DrawImage.imageView));

		// Depth image creation
		m_DepthImage.format = VK_FORMAT_D32_SFLOAT;
		m_DepthImage.extent = renderExtent;

		VkImageUsageFlags depthImageFlags = 0;
		depthImageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo depthImageInfo = vkInit::imageCreateInfo(m_DepthImage.format, depthImageFlags, renderExtent);
		VK_CHECK(vmaCreateImage(m_Allocator, &depthImageInfo, &img_allocInfo, &m_DepthImage.image, &m_DepthImage.allocation, nullptr));

		VkImageViewCreateInfo viewDepthCreateInfo = vkInit::imageviewCreateInfo(m_DepthImage.format, m_DepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(m_Device, &viewDepthCreateInfo, nullptr, &m_DepthImage.imageView));

		m_MainCleanupQueue.enqueueCleanup([=]()
			{
				// Destroy drawImage
				vkDestroyImageView(m_Device, m_DrawImage.imageView, nullptr);
				vmaDestroyImage(m_Allocator, m_DrawImage.image, m_DrawImage.allocation);

				// Destroy depthImage
				vkDestroyImageView(m_Device, m_DepthImage.imageView, nullptr);
				vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);
			});
	}

	// Initialize Command Buffers
	void Engine::initCommandBuffers()
	{
		VkCommandPoolCreateInfo commandPoolInfo = vkInit::commandPoolCreateInfo(m_GraphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].commandPool));

			VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::commandBufferAllocateInfo(m_Frames[i].commandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].mainCommandBuffer));
		}

		VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_ImguiCommandPool));
		// Allocate the command buffer for immediate submits
		VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::commandBufferAllocateInfo(m_ImguiCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_ImguiCommandBuffer));

		m_MainCleanupQueue.enqueueCleanup([=]() {
			vkDestroyCommandPool(m_Device, m_ImguiCommandPool, nullptr);
			});
	}

	// Initialize Descriptors
	void Engine::initDescriptors()
	{
		// Create a descriptor pool that will hold 10 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
		};

		m_GlobalDescriptorAllocator.init(m_Device, 10, sizes);

		// Make the descriptor set layout for our compute draw
		{
			DescriptorLayoutBuilder builder;
			builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			m_BackgroundShaderDescriptorLayout = builder.build(m_Device, VK_SHADER_STAGE_COMPUTE_BIT);
		}
		m_BackgroundShaderDescriptorSet = m_GlobalDescriptorAllocator.allocate(m_Device, m_BackgroundShaderDescriptorLayout);

		DescriptorWriter writer;
		writer.writeImage(0, m_DrawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.updateSet(m_Device, m_BackgroundShaderDescriptorSet);
		// Make sure both the descriptor allocator and the new layout get cleaned up properly
		m_MainCleanupQueue.enqueueCleanup([&]() {
			m_GlobalDescriptorAllocator.destroyPools(m_Device);
			vkDestroyDescriptorSetLayout(m_Device, m_BackgroundShaderDescriptorLayout, nullptr);
			});
		// General use sets for frames
		{
			DescriptorLayoutBuilder builder;
			builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			m_SceneDescriptorLayout = builder.build(m_Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
			m_MainCleanupQueue.enqueueCleanup([&]() {
				vkDestroyDescriptorSetLayout(m_Device, m_SceneDescriptorLayout, nullptr);
				});
		}

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			// Create a descriptor pool
			std::vector<DescriptorAllocator::PoolSizeRatio> frame_sizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
			};

			m_Frames[i].descriptorAllocator = DescriptorAllocator{};
			m_Frames[i].descriptorAllocator.init(m_Device, 1000, frame_sizes);
			m_Frames[i].sceneDescriptorSet = m_Frames[i].descriptorAllocator.allocate(m_Device, m_SceneDescriptorLayout);

			m_MainCleanupQueue.enqueueCleanup([&, i]() {
				m_Frames[i].descriptorAllocator.destroyPools(m_Device);
				});

			m_Frames[i].sceneParameterBuffer = createBuffer(
				sizeof(GPUSceneData),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU
			);

			DescriptorWriter writer;
			writer.writeBuffer(0, m_Frames[i].sceneParameterBuffer.buffer,
				sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.updateSet(m_Device, m_Frames[i].sceneDescriptorSet);
			m_MainCleanupQueue.enqueueCleanup([=, this]() {
				destroyBuffer(m_Frames[i].sceneParameterBuffer);
				});
		}
	}

	// Initialize Synchronization Structures
	void Engine::initSyncStructures()
	{
		VkFenceCreateInfo fenceCreateInfo = vkInit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::semaphoreCreateInfo();
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].renderFence));
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].swapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].renderSemaphore));
		}
		VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_ImguiFence));
		m_MainCleanupQueue.enqueueCleanup([=]() { vkDestroyFence(m_Device, m_ImguiFence, nullptr); });
	}

	// draw Background
	void Engine::drawBackground(VkCommandBuffer cmd)
	{
		ComputeEffect& effect = m_BackgroundEffects[m_ActiveBackgroundEffect];

		// Bind the compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

		// Bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BackgroundPipelineLayout, 0, 1, &m_BackgroundShaderDescriptorSet, 0, nullptr);

		vkCmdPushConstants(cmd, m_BackgroundPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.pushConstants);

		// Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		uint32_t dispatchX = static_cast<uint32_t>(std::ceil(static_cast<float>(m_DrawExtent.width) / 16.0f));
		uint32_t dispatchY = static_cast<uint32_t>(std::ceil(static_cast<float>(m_DrawExtent.height) / 16.0f));
		vkCmdDispatch(cmd, dispatchX, dispatchY, 1);
	}

	// draw Geometry
	void Engine::drawGeometry(VkCommandBuffer cmd)
	{
		//for (auto& img : m_LoadedNodes["structure"]->getImages())
		//{
		//	vkUtil::transitionImage(cmd, img.second.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//}

		//vkUtil::transitionImage(cmd, m_DefaultEngineData.whiteImage.image,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//vkUtil::transitionImage(cmd, m_DefaultEngineData.blackImage.image,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//vkUtil::transitionImage(cmd, m_DefaultEngineData.greyImage.image,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//vkUtil::transitionImage(cmd, m_DefaultEngineData.errorCheckerboardImage.image,
		//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// VkClearValue clearVal{ .color {0,0,0,0} };
		VkRenderingAttachmentInfo colorAttach = vkInit::attachmentInfo(m_DrawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttach = vkInit::depthAttachmentInfo(m_DepthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		// Allocate a new uniform buffer for the scene data
		AllocatedBuffer gpuSceneDataBuffer = getCurrentFrame().sceneParameterBuffer;

		// Add it to the cleanup queue of this frame so it gets deleted once it's been used

		VmaAllocationInfo info;
		vmaGetAllocationInfo(m_Allocator, gpuSceneDataBuffer.allocation, &info);
		// Write the buffer
		GPUSceneData* sceneUniformData = (GPUSceneData*)info.pMappedData;
		*sceneUniformData = m_SceneData;

		// Create a descriptor set that binds that buffer and update it

		VkRenderingInfo renderingInfo = vkInit::renderingInfo(m_DrawExtent, &colorAttach, &depthAttach);
		vkCmdBeginRendering(cmd, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(m_DrawExtent.width);
		viewport.height = static_cast<float>(m_DrawExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = m_DrawExtent.width;
		scissor.extent.height = m_DrawExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		VkDescriptorSet globalDescriptor = getCurrentFrame().sceneDescriptorSet;

		//DescriptorWriter writer;
		//writer.writeBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		//writer.updateSet(m_Device, globalDescriptor);

		const MaterialPipeline* lastPipeline = nullptr;
		const MaterialInstance* lastMaterial = nullptr;
		VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

		auto draw = [&](const DrawCommand& r) {
			if (&r.material != (lastMaterial)) {
				lastMaterial = &r.material;
				//rebind pipeline and descriptors if the material changed
				if (r.material.pipeline != lastPipeline) {
					lastPipeline = r.material.pipeline;
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material.pipeline->pipeline);
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material.pipeline->layout, 0, 1,
						&globalDescriptor, 0, nullptr);

					VkViewport viewport = {};
					viewport.x = 0;
					viewport.y = 0;
					viewport.width = static_cast<float>(m_DrawExtent.width);
					viewport.height = static_cast<float>(m_DrawExtent.height);
					viewport.minDepth = 0.f;
					viewport.maxDepth = 1.f;

					vkCmdSetViewport(cmd, 0, 1, &viewport);

					VkRect2D scissor = {};
					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = m_DrawExtent.width;
					scissor.extent.height = m_DrawExtent.height;

					vkCmdSetScissor(cmd, 0, 1, &scissor);
				}

				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material.pipeline->layout, 1, 1,
					&r.material.descriptorSet, 0, nullptr);
			}
			//rebind index buffer if needed
			if (r.indexBuffer != lastIndexBuffer) {
				lastIndexBuffer = r.indexBuffer;
				vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			}
			// calculate final mesh matrix
			GPUDrawPushConstants push_constants;
			push_constants.worldMatrix = r.transform;
			push_constants.vertexBufferAddress = r.vertexBufferAddress;

			vkCmdPushConstants(cmd, r.material.pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

			vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
			//stats
			stats.drawcall_count++;
			stats.triangle_count += r.indexCount / 3;
			};

		m_MainRenderQueue.sort(m_Camera->getPosition());
		for (size_t i = 0; i < m_MainRenderQueue.opaqueCount(); i++) {
			const DrawCommand& drawCmd = m_MainRenderQueue.getOpaqueObject(i);
			draw(drawCmd);
		}

		for (size_t i = 0; i < m_MainRenderQueue.transparentCount(); i++) {
			const DrawCommand& drawCmd = m_MainRenderQueue.getTransparentObject(i);
			draw(drawCmd);
		}

		vkCmdEndRendering(cmd);
	}

	// Initialize Pipelines
	void Engine::initPipelines()
	{
		initBackgroundPipelines();
	}

	// Initialize Background Pipelines
	void Engine::initBackgroundPipelines()
	{
		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = &m_BackgroundShaderDescriptorLayout;
		computeLayout.setLayoutCount = 1;

		VkPushConstantRange pushRange{};
		pushRange.size = sizeof(ComputePushConstants);
		pushRange.offset = 0;
		pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		computeLayout.pPushConstantRanges = &pushRange;
		computeLayout.pushConstantRangeCount = 1;

		VK_CHECK(vkCreatePipelineLayout(m_Device, &computeLayout, nullptr, &m_BackgroundPipelineLayout));
		VkShaderModule gradientShader;

		if (!vkUtil::loadShaderModule("shaders/gradient_color.comp.spv", m_Device, &gradientShader))
		{
			fmt::print("Error when building the compute shader \n");
		}

		VkShaderModule skyShader;
		if (!vkUtil::loadShaderModule("shaders/sky.comp.spv", m_Device, &skyShader)) {
			fmt::print("Error when building the compute shader \n");
		}

		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pNext = nullptr;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = gradientShader;
		stageInfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = m_BackgroundPipelineLayout;
		computePipelineCreateInfo.stage = stageInfo;

		ComputeEffect gradient;
		gradient.pipelineLayout = m_BackgroundPipelineLayout;
		gradient.name = "gradient";
		gradient.pushConstants = {};

		// Default colors
		gradient.pushConstants.data1 = glm::vec4(1, 0, 0, 1);
		gradient.pushConstants.data2 = glm::vec4(0, 0, 1, 1);

		VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

		// Change the shader module only to create the sky shader
		computePipelineCreateInfo.stage.module = skyShader;

		ComputeEffect sky;
		sky.pipelineLayout = m_BackgroundPipelineLayout;
		sky.name = "sky";
		sky.pushConstants = {};
		// Default sky parameters
		sky.pushConstants.data1 = glm::vec4(0.1f, 0.2f, 0.4f, 0.97f);

		VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

		// Add the 2 background effects into the array
		m_BackgroundEffects.push_back(gradient);
		m_BackgroundEffects.push_back(sky);

		// Destroy shader modules properly
		vkDestroyShaderModule(m_Device, gradientShader, nullptr);
		vkDestroyShaderModule(m_Device, skyShader, nullptr);
		m_MainCleanupQueue.enqueueCleanup([=]() {
			vkDestroyPipelineLayout(m_Device, m_BackgroundPipelineLayout, nullptr);
			vkDestroyPipeline(m_Device, sky.pipeline, nullptr);
			vkDestroyPipeline(m_Device, gradient.pipeline, nullptr);
			});
	}

	// Initialize ImGui
	void Engine::initImgui() {
		// Create descriptor pool for IMGUI
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &imguiPool));

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// Enable docking and viewports
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = false;  // Allow dragging from anywhere in the window

		// Setup style
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 5.0f;      // Rounded corners
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.WindowBorderSize = 1.0f;    // Visible borders
		style.FrameBorderSize = 1.0f;
		style.WindowPadding = ImVec2(8, 8);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.Colors[ImGuiCol_WindowBg].w = 0.9f;  // Slightly transparent windows
		}

		// Setup platform/renderer backends
		ImGui_ImplSDL2_InitForVulkan(m_Window);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_Instance;
		init_info.PhysicalDevice = m_PhysicalDevice;
		init_info.Device = m_Device;
		init_info.Queue = m_GraphicsQueue;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();

		m_ImguiDrawImageDescriptor = ImGui_ImplVulkan_AddTexture(
			m_DefaultEngineData.samplerNearest,
			m_DrawImage.imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		m_MainCleanupQueue.enqueueCleanup([=]() {
			vkDeviceWaitIdle(m_Device);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			vkDestroyDescriptorPool(m_Device, imguiPool, nullptr);
			});
	}

	// draw ImGui
	void Engine::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkInit::renderingInfo(m_SwapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	// Create Image
	AllocatedImage Engine::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		AllocatedImage newImage;
		newImage.format = format;
		newImage.extent = size;

		VkImageCreateInfo img_info = vkInit::imageCreateInfo(format, usage, size);
		if (mipmapped) {
			img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		// Always allocate images on dedicated GPU memory
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		// Allocate and create the image
		VK_CHECK(vmaCreateImage(m_Allocator, &img_info, &allocInfo, &newImage.image, &newImage.allocation, nullptr));

		//assert(0x67022e000000004b != (uint64_t)newImage.image);

		fmt::print("Created Image Handle: {:#x}\n", (uint64_t)newImage.image);

		// If the format is a depth format, we will need to have it use the correct aspect flag
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT) {
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// Build an image-view for the image
		VkImageViewCreateInfo view_info = vkInit::imageviewCreateInfo(format, newImage.image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(m_Device, &view_info, nullptr, &newImage.imageView));

		return newImage;
	}

	// Create Image with Data.
	AllocatedImage Engine::createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		size_t data_size = size.depth * size.width * size.height * 4;
		AllocatedBuffer uploadBuffer = createBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		memcpy(uploadBuffer.allocationInfo.pMappedData, data, data_size);

		AllocatedImage new_image = createImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

		immediateSubmit([&](VkCommandBuffer cmd) {
			vkUtil::transitionImage(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

			// Copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&copyRegion);

			vkUtil::transitionImage(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		destroyBuffer(uploadBuffer);

		return new_image;
	}

	// Destroy Image
	void Engine::destroyImage(const AllocatedImage& img)
	{
		vkDestroyImageView(m_Device, img.imageView, nullptr);
		vmaDestroyImage(m_Allocator, img.image, img.allocation);
	}

	// Upload Mesh
	GPUMeshBuffers Engine::uploadMesh(const std::span<uint32_t>& indices, const std::span<Vertex>& vertices)
	{
		const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		GPUMeshBuffers meshBuff;

		// Create vertex buffer
		meshBuff.vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo deviceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = meshBuff.vertexBuffer.buffer };
		meshBuff.vertexBufferAddress = vkGetBufferDeviceAddress(m_Device, &deviceAddressInfo);

		// Create index buffer
		meshBuff.indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		AllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VmaAllocationInfo info;
		vmaGetAllocationInfo(m_Allocator, staging.allocation, &info);
		void* data = info.pMappedData;

		// Copy vertex buffer
		memcpy(data, vertices.data(), vertexBufferSize);
		// Copy index buffer
		memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

		immediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertexBufferSize;

			vkCmdCopyBuffer(cmd, staging.buffer, meshBuff.vertexBuffer.buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertexBufferSize;
			indexCopy.size = indexBufferSize;

			vkCmdCopyBuffer(cmd, staging.buffer, meshBuff.indexBuffer.buffer, 1, &indexCopy);
			});

		destroyBuffer(staging);

		return meshBuff;
	}

	// Create Buffer
	AllocatedBuffer Engine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		if (allocSize == 0)
			return {};

		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		AllocatedBuffer newBuffer{};

		VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo));

		return newBuffer;
	}

	// Destroy Buffer
	void Engine::destroyBuffer(const AllocatedBuffer& buffer)
	{
		vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
	}

	// Create Swapchain
	void Engine::createSwapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };
		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_SwapchainImageFormat, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_Swapchain = vkbSwapchain.swapchain;
		m_SwapchainExtent = vkbSwapchain.extent;  // Use actual swapchain extent
		m_SwapchainImages = vkbSwapchain.get_images().value();
		m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	// Destroy Swapchain
	void Engine::destroySwapchain()
	{
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
		}
	}

	//// Build Pipeline for Metallic-Roughness Material
	//void MaterialFactory::buildPipeline(Engine* engine)
	//{
	//	VkShaderModule meshFragShader;
	//	if (!vkUtil::loadShaderModule("shaders/mesh.frag.spv", engine->getDevice(), &meshFragShader)) {
	//		fmt::print("Error when building the triangle fragment shader module\n");
	//	}

	//	VkShaderModule meshVertexShader;
	//	if (!vkUtil::loadShaderModule("shaders/mesh.vert.spv", engine->getDevice(), &meshVertexShader)) {
	//		fmt::print("Error when building the triangle vertex shader module\n");
	//	}

	//	VkPushConstantRange matrixRange{};
	//	matrixRange.offset = 0;
	//	matrixRange.size = sizeof(GPUDrawPushConstants);
	//	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//	DescriptorLayoutBuilder layoutBuilder;
	//	layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	//	layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	//	layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	//	materialDescriptorLayout = layoutBuilder.build(engine->getDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	//	VkDescriptorSetLayout layouts[] = { engine->m_SceneDescriptorLayout, materialDescriptorLayout };

	//	VkPipelineLayoutCreateInfo meshLayoutInfo = vkInit::pipelineLayoutCreateInfo();
	//	meshLayoutInfo.setLayoutCount = 2;
	//	meshLayoutInfo.pSetLayouts = layouts;
	//	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	//	meshLayoutInfo.pushConstantRangeCount = 1;

	//	VkPipelineLayout newLayout;
	//	VK_CHECK(vkCreatePipelineLayout(engine->getDevice(), &meshLayoutInfo, nullptr, &newLayout));

	//	opaquePipeline.layout = newLayout;
	//	transparentPipeline.layout = newLayout;

	//	// Build the stage-create-info for both vertex and fragment stages. This lets
	//	// the pipeline know the shader modules per stage
	//	PipelineBuilder pipelineBuilder;
	//	opaquePipeline.pipeline = pipelineBuilder.setShaders(meshVertexShader, meshFragShader)
	//		.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	//		.setPolygonMode(VK_POLYGON_MODE_FILL)
	//		.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
	//		.setMultisamplingNone()
	//		.disableBlending()
	//		.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
	//		.setColorAttachmentFormat(engine->m_DrawImage.format)
	//		.setDepthFormat(engine->m_DepthImage.format)
	//		.setPipelineLayout(newLayout).build(engine->getDevice());

	//	// Create the transparent variant
	//	transparentPipeline.pipeline = pipelineBuilder.enableBlendingAdditive().
	//		enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL).build(engine->getDevice());

	//	// Destroy shader modules properly
	//	vkDestroyShaderModule(engine->getDevice(), meshFragShader, nullptr);
	//	vkDestroyShaderModule(engine->getDevice(), meshVertexShader, nullptr);
	//}

	//// Clear Resources (Placeholder)
	//void MaterialFactory::clearResources(VkDevice device)
	//{
	//}

	//// Write Material
	//MaterialInstance MaterialFactory::createMaterial(VkDevice device, MaterialPass pass, const Resources& resources, DescriptorAllocator& descriptorAllocator)
	//{
	//	MaterialInstance matData;
	//	matData.passType = pass;
	//	if (pass == MaterialPass::Transparent) {
	//		matData.pipeline = &transparentPipeline;
	//	}
	//	else {
	//		matData.pipeline = &opaquePipeline;
	//	}

	//	matData.descriptorSet = descriptorAllocator.allocate(device, materialDescriptorLayout);

	//	descriptorWriter.clear();
	//	descriptorWriter.writeBuffer(0, resources.dataBuffer, sizeof(Constants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	//	descriptorWriter.writeImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	//	descriptorWriter.writeImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	//	descriptorWriter.updateSet(device, matData.descriptorSet);

	//	return matData;
	//}

	void MaterialSystem::initialize(Engine* engine) {
		createDescriptorLayout(engine);
		buildPipelines(engine);
		// Initialize descriptor allocator for materials
		std::vector<DescriptorAllocator::PoolSizeRatio> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5.0f } // 5 textures per material
		};
		m_MaterialDescriptorPool.init(engine->getDevice(), 1000, poolSizes); // Support for 1000 materials
	}

	void MaterialSystem::cleanup(VkDevice device) {
		if (m_OpaquePipeline.pipeline) {
			vkDestroyPipeline(device, m_OpaquePipeline.pipeline, nullptr);
		}

		if (m_TransparentPipeline.pipeline) {
			vkDestroyPipeline(device, m_TransparentPipeline.pipeline, nullptr);
		}

		VkPipelineLayout layout =
			m_TransparentPipeline.layout == VK_NULL_HANDLE ?
			(m_OpaquePipeline.layout == VK_NULL_HANDLE ? VK_NULL_HANDLE : m_OpaquePipeline.layout) : m_TransparentPipeline.layout;
		// Destroy the layout only once
		if (layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device, layout, nullptr);
			m_TransparentPipeline.layout = VK_NULL_HANDLE;
			m_OpaquePipeline.layout = VK_NULL_HANDLE;
		}

		if (m_MaterialLayout) {
			vkDestroyDescriptorSetLayout(device, m_MaterialLayout, nullptr);
		}
		m_MaterialDescriptorPool.destroyPools(device);
	}

	void MaterialSystem::createDescriptorLayout(Engine* engine) {
		DescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		//builder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		//builder.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		//builder.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		m_MaterialLayout = builder.build(
			engine->getDevice(),
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT
		);
	}

	void MaterialSystem::buildPipelines(Engine* engine) {
		VkShaderModule meshVertShader;
		VkShaderModule meshFragShader;

		if (!vkUtil::loadShaderModule("shaders/mesh.vert.spv", engine->getDevice(), &meshVertShader)) {
			throw std::runtime_error("Failed to load mesh vertex shader");
		}
		if (!vkUtil::loadShaderModule("shaders/mesh.frag.spv", engine->getDevice(), &meshFragShader)) {
			throw std::runtime_error("Failed to load mesh fragment shader");
		}

		VkPushConstantRange pushConstant{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(GPUDrawPushConstants)
		};

		std::vector<VkDescriptorSetLayout> layouts = {
			engine->getSceneDescriptorLayout(),
			m_MaterialLayout
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkInit::pipelineLayoutCreateInfo();
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		VkPipelineLayout pipelineLayout;
		VK_CHECK(vkCreatePipelineLayout(engine->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

		PipelineBuilder builder;
		builder.setShaders(meshVertShader, meshFragShader)
			.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setPolygonMode(VK_POLYGON_MODE_FILL)
			.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
			.setMultisamplingNone()
			.setColorAttachmentFormat(engine->getDrawImage().format)
			.setDepthFormat(engine->getDepthImage().format)
			.setPipelineLayout(pipelineLayout);

		m_OpaquePipeline.layout = pipelineLayout;
		builder.disableBlending()
			.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
		m_OpaquePipeline.pipeline = builder.build(engine->getDevice());

		m_TransparentPipeline.layout = pipelineLayout;
		builder.enableBlendingAdditive()
			.enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
		m_TransparentPipeline.pipeline = builder.build(engine->getDevice());

		vkDestroyShaderModule(engine->getDevice(), meshVertShader, nullptr);
		vkDestroyShaderModule(engine->getDevice(), meshFragShader, nullptr);
	}
	//MaterialInstance MaterialFactory::createFromGLTF(
	//	Engine* engine,
	//	const fastgltf::Material& gltfMat,
	//	Constants* constantsBuffer,
	//	uint32_t materialIndex,
	//	const Resources& defaultResources,
	//	VkBuffer uniformBuffer
	//)
	//{
	//	// Fill constants
	//	Constants constants;
	//	constants.baseColorFactor = glm::vec4(
	//		gltfMat.pbrData.baseColorFactor[0],
	//		gltfMat.pbrData.baseColorFactor[1],
	//		gltfMat.pbrData.baseColorFactor[2],
	//		gltfMat.pbrData.baseColorFactor[3]
	//	);
	//	constants.metallicRoughnessFactors = glm::vec4(
	//		gltfMat.pbrData.metallicFactor,
	//		gltfMat.pbrData.roughnessFactor,
	//		0.0f, 0.0f
	//	);
	//	// Set other constants as needed

	//	// Write constants to the buffer
	//	constantsBuffer[materialIndex] = constants;

	//	// Create resources, starting with defaults
	//	Resources resources = defaultResources;
	//	resources.uniformBuffer = uniformBuffer;
	//	resources.uniformBufferOffset = static_cast<uint32_t>(materialIndex * sizeof(Constants));

	//	// Override textures if present in GLTF material
	//	// Assume you have functions to retrieve textures and samplers from loaded data
	//	if (gltfMat.pbrData.baseColorTexture.has_value()) {
	//		size_t textureIndex = gltfMat.pbrData.baseColorTexture->textureIndex;
	//		resources.albedoTexture = /* Retrieve texture from loadedTextures using textureIndex */;
	//		resources.sampler = /* Retrieve sampler from loadedSamplers using textureIndex */;
	//	}

	//	// Determine the material pass type
	//	MaterialPass passType = (gltfMat.alphaMode == fastgltf::AlphaMode::Blend)
	//		? MaterialPass::Transparent
	//		: MaterialPass::MainColor;

	//	// Create and return the material instance
	//	return createInstance(engine, passType, constants, resources);
	//}
	MaterialInstance MaterialFactory::createInstance(
		Engine* engine,
		MaterialPass pass,
		const Resources& resources)
	{
		MaterialInstance instance;
		instance.passType = pass;

		// Obtain the appropriate pipeline from MaterialSystem
		instance.pipeline = (pass == MaterialPass::Transparent)
			? &engine->getMaterialSystem().getTransparentPipeline()
			: &engine->getMaterialSystem().getOpaquePipeline();

		// Allocate a descriptor set
		instance.descriptorSet = engine->getMaterialSystem().allocateMatDescriptorSet(engine->getDevice());

		// Update descriptors
		DescriptorWriter writer = engine->getMaterialSystem().createWriter();

		writer.writeBuffer(
			0,
			resources.uniformBuffer,
			sizeof(Constants),
			resources.uniformBufferOffset,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		);

		writer.writeImage(
			1,
			resources.albedoTexture.imageView,
			resources.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		);

		writer.writeImage(2, resources.metallicRoughnessTexture.imageView,
			resources.sampler,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		//writer.writeImage(3, m_Resources.normalTexture.imageView,
		//	m_Resources.sampler,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		//if (m_Resources.emissiveTexture.image != VK_NULL_HANDLE) {
		//	writer.writeImage(4, m_Resources.emissiveTexture.imageView,
		//		m_Resources.sampler,
		//		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		//}

		//if (m_Resources.occlusionTexture.image != VK_NULL_HANDLE) {
		//	writer.writeImage(5, m_Resources.occlusionTexture.imageView,
		//		m_Resources.sampler,
		//		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		//}

		writer.updateSet(engine->getDevice(), instance.descriptorSet);

		return instance;
	}
}