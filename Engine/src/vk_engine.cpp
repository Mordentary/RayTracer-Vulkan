#include "vk_engine.h"

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

namespace Engine
{
#ifdef _DEBUG
	constexpr bool bUseValidationLayers = true;
#else
	constexpr bool bUseValidationLayers = false;
#endif

	VulkanEngine* loadedEngine = nullptr;

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

	// Singleton instance retrieval
	VulkanEngine& VulkanEngine::GetInstance() { return *loadedEngine; }

	// Initialization
	void VulkanEngine::init()
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

		initDefaultData();

		initImgui();

		m_IsInitialized = true;
	}

	// Cleanup
	void VulkanEngine::cleanup()
	{
		if (m_IsInitialized)
		{
			vkDeviceWaitIdle(m_Device);

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
	void VulkanEngine::drawFrame()
	{
		VK_CHECK(vkWaitForFences(m_Device, 1, &getCurrentFrame().renderFence, VK_TRUE, 1000000000));
		VK_CHECK(vkResetFences(m_Device, 1, &getCurrentFrame().renderFence));

		// Swapchain image acquiring
		uint32_t swapchainImageIndex;
		VkResult e = vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			m_ResizeRequested = true;
			return;
		}

		m_DrawExtent.height = std::min(m_SwapchainExtent.height, m_DrawImage.extent.height) * m_RenderScale;
		m_DrawExtent.width = std::min(m_SwapchainExtent.width, m_DrawImage.extent.width) * m_RenderScale;

		VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

		// Begin command buffer
		VK_CHECK(vkResetCommandBuffer(cmd, 0));
		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// Prepare image, draw, prepare for blit
		vkUtil::transitionImage(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		drawBackground(cmd);

		vkUtil::transitionImage(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkUtil::transitionImage(cmd, m_DepthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		drawGeometry(cmd);

		vkUtil::transitionImage(cmd, m_DrawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Blit
		vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkUtil::copyImageToImage(cmd, m_DrawImage.image, m_SwapchainImages[swapchainImageIndex], m_DrawExtent, m_DrawExtent);
		vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

		// Drawing ImGui
		drawImgui(cmd, m_SwapchainImageViews[swapchainImageIndex]);
		vkUtil::transitionImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	// Main Loop
	void VulkanEngine::run()
	{
		bool bQuit = false;
		SDL_Event e;
		auto lastFrame = std::chrono::high_resolution_clock::now();

		while (!bQuit)
		{
			auto currentFrame = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float>(currentFrame - lastFrame).count();
			lastFrame = currentFrame;

			// Get keyboard state
			const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);

			// Handle events
			while (SDL_PollEvent(&e) != 0) {
				// Close the window when user quits
				if (e.type == SDL_QUIT)
					bQuit = true;

				if (e.type == SDL_WINDOWEVENT) {
					if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
						m_StopRendering = true;
					}
					if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
						m_StopRendering = false;
					}
				}
				m_Camera->handleEvent(e);
				ImGui_ImplSDL2_ProcessEvent(&e);
			}
			// Process keyboard input for camera movement
			m_Camera->processKeyboard(keyboardState, deltaTime);

			if (m_ResizeRequested) {
				resizeSwapchain();
			}

			// Do not draw if we are minimized
			if (m_StopRendering) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			// ImGui new frame
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			// Some ImGui UI to test
			if (ImGui::Begin("Background"))
			{
				ImGui::SliderFloat("Render Scale", &m_RenderScale, 0.3f, 1.f);
				ComputeEffect& selected = m_BackgroundEffects[m_ActiveBackgroundEffect];
				ImGui::Text("Selected effect: %s", selected.name.c_str());
				ImGui::SliderInt("Effect Index", &m_ActiveBackgroundEffect, 0, static_cast<int>(m_BackgroundEffects.size()) - 1);

				ImGui::InputFloat4("data1", glm::value_ptr(selected.pushConstants.data1));
				ImGui::InputFloat4("data2", glm::value_ptr(selected.pushConstants.data2));
				ImGui::InputFloat4("data3", glm::value_ptr(selected.pushConstants.data3));
				ImGui::InputFloat4("data4", glm::value_ptr(selected.pushConstants.data4));
			}
			ImGui::End();
			// Make ImGui calculate internal draw structures
			ImGui::Render();

			updateScene();
			drawFrame();
		}
	}

	void VulkanEngine::updateScene()
	{
		m_MainDrawContext.opaqueObjects.clear();

		m_LoadedNodes["structure"]->draw(glm::mat4{ 1.f }, m_MainDrawContext);

		m_SceneData.view = m_Camera->getViewMatrix();
		m_SceneData.projection = m_Camera->getProjectionMatrix();
		m_SceneData.viewProjection = m_SceneData.projection * m_SceneData.view;

		//some default lighting parameters
		m_SceneData.ambientColor = glm::vec4(.1f);
		m_SceneData.sunlightColor = glm::vec4(1.f);
		m_SceneData.sunlightDirection = glm::vec4(0, 1, 0.5, 1.f);
	}

	// Immediate Submit Helper
	void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
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
	void VulkanEngine::resizeSwapchain()
	{
		vkDeviceWaitIdle(m_Device);
		destroySwapchain();

		int w, h;
		SDL_GetWindowSize(m_Window, &w, &h);
		m_WindowExtent.width = w;
		m_WindowExtent.height = h;

		createSwapchain(m_WindowExtent.width, m_WindowExtent.height);

		m_ResizeRequested = false;
	}

	// Initialize Default Data
	void VulkanEngine::initDefaultData()
	{
		uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
		m_WhiteImage = createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
		m_GreyImage = createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
		m_BlackImage = createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);

		// Checkerboard image
		uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		std::array<uint32_t, 16 * 16> pixels; // For 16x16 checkerboard texture
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
			}
		}
		m_ErrorCheckerboardImage = createImage(pixels.data(), VkExtent3D{ 16, 16, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT);
		VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;

		VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_SamplerNearest));

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		VK_CHECK(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_SamplerLinear));

		m_MainCleanupQueue.enqueueCleanup([=]() {
			vkDestroySampler(m_Device, m_SamplerNearest, nullptr);
			vkDestroySampler(m_Device, m_SamplerLinear, nullptr);

			destroyImage(m_WhiteImage);
			destroyImage(m_GreyImage);
			destroyImage(m_BlackImage);
			destroyImage(m_ErrorCheckerboardImage);
			});

		MetallicRoughnessMaterial::MaterialResources materialResources;
		materialResources.colorImage = m_WhiteImage;
		materialResources.colorSampler = m_SamplerLinear;
		materialResources.metalRoughImage = m_WhiteImage;
		materialResources.metalRoughSampler = m_SamplerLinear;
		// Set the uniform buffer for the material data
		AllocatedBuffer materialConstants = createBuffer(sizeof(MetallicRoughnessMaterial::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Write the buffer
		VmaAllocationInfo info;
		vmaGetAllocationInfo(m_Allocator, materialConstants.allocation, &info);
		MetallicRoughnessMaterial::MaterialConstants* materialUniformData = (MetallicRoughnessMaterial::MaterialConstants*)info.pMappedData;
		materialUniformData->colorFactors = glm::vec4{ 1,1,1,1 };
		materialUniformData->metalRoughFactors = glm::vec4{ 1,0.5,0,0 };

		m_MainCleanupQueue.enqueueCleanup([=, this]() {
			destroyBuffer(materialConstants);
			});

		materialResources.dataBuffer = materialConstants.buffer;
		materialResources.dataBufferOffset = 0;

		m_DefaultMaterial = m_MetallicRoughnessMaterial.createMaterial(m_Device, MaterialPass::MainColor, materialResources, m_GlobalDescriptorAllocator);

		auto structureFile = loadGltfMeshes(this, "assets/house2.glb");

		assert(structureFile.has_value());
		m_LoadedNodes["structure"] = *structureFile;
	}

	// Initialize Vulkan
	void VulkanEngine::initVulkan()
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
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;

		// Additional validation features
		VkValidationFeaturesEXT validationFeatures = {};
		validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		std::vector<VkValidationFeatureEnableEXT> enabledValidationFeatures = {
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
			// Removed VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
		};
		validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size());
		validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures.data();

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
	void VulkanEngine::initSwapchain()
	{
		createSwapchain(m_WindowExtent.width, m_WindowExtent.height);

		VkExtent3D renderExtent = {
			m_WindowExtent.width,
			m_WindowExtent.height,
			1
		};

		// draw image creation
		m_DrawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		m_DrawImage.extent = renderExtent;

		VkImageUsageFlags drawImageFlags = 0;

		drawImageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
	void VulkanEngine::initCommandBuffers()
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
	void VulkanEngine::initDescriptors()
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

			m_MainCleanupQueue.enqueueCleanup([&, i]() {
				m_Frames[i].descriptorAllocator.destroyPools(m_Device);
				});
		}
		{
			DescriptorLayoutBuilder builder;
			builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			m_SceneDescriptorLayout = builder.build(m_Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		}
	}

	// Initialize Synchronization Structures
	void VulkanEngine::initSyncStructures()
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
	void VulkanEngine::drawBackground(VkCommandBuffer cmd)
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
	void VulkanEngine::drawGeometry(VkCommandBuffer cmd)
	{
		// VkClearValue clearVal{ .color {0,0,0,0} };
		VkRenderingAttachmentInfo colorAttach = vkInit::attachmentInfo(m_DrawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttach = vkInit::depthAttachmentInfo(m_DepthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		// Allocate a new uniform buffer for the scene data
		AllocatedBuffer gpuSceneDataBuffer = createBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Add it to the cleanup queue of this frame so it gets deleted once it's been used
		getCurrentFrame().cleanupQueue.enqueueCleanup([=, this]() {
			destroyBuffer(gpuSceneDataBuffer);
			});

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

		VkDescriptorSet globalDescriptor = m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT].descriptorAllocator.allocate(m_Device, m_SceneDescriptorLayout);

		DescriptorWriter writer;
		writer.writeBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.updateSet(m_Device, globalDescriptor);

		for (const RenderableObject& draw : m_MainDrawContext.opaqueObjects) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.materialInstance->pipeline->pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.materialInstance->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.materialInstance->pipeline->layout, 1, 1, &draw.materialInstance->descriptorSet, 0, nullptr);

			vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			GPUDrawPushConstants pushConstants;
			pushConstants.vertexBufferAddress = draw.vertexBufferAddress;
			pushConstants.worldMatrix = draw.transformMatrix;
			vkCmdPushConstants(cmd, draw.materialInstance->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

			vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, 0, 0);
		}
		vkCmdEndRendering(cmd);
	}

	// Initialize Pipelines
	void VulkanEngine::initPipelines()
	{
		initBackgroundPipelines();
		m_MetallicRoughnessMaterial.buildPipeline(this);
	}

	// Initialize Background Pipelines
	void VulkanEngine::initBackgroundPipelines()
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
	void VulkanEngine::initImgui()
	{
		// 1: Create descriptor pool for IMGUI
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
		pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &imguiPool));

		// 2: Initialize ImGui library

		// This inits the core structures of ImGui
		ImGui::CreateContext();

		// This inits ImGui for SDL
		ImGui_ImplSDL2_InitForVulkan(m_Window);

		// This inits ImGui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_Instance;
		init_info.PhysicalDevice = m_PhysicalDevice;
		init_info.Device = m_Device;
		init_info.Queue = m_GraphicsQueue;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;

		// Dynamic rendering parameters for ImGui to use
		init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;

		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);

		ImGui_ImplVulkan_CreateFontsTexture();

		// Add the destruction of the ImGui created structures
		m_MainCleanupQueue.enqueueCleanup([=]() {
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(m_Device, imguiPool, nullptr);
			});
	}

	// draw ImGui
	void VulkanEngine::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkInit::renderingInfo(m_SwapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);
	}

	// Create Image
	AllocatedImage VulkanEngine::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
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

	// Create Image with Data
	AllocatedImage VulkanEngine::createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
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
	void VulkanEngine::destroyImage(const AllocatedImage& img)
	{
		vkDestroyImageView(m_Device, img.imageView, nullptr);
		vmaDestroyImage(m_Allocator, img.image, img.allocation);
	}

	// Upload Mesh
	GPUMeshBuffers VulkanEngine::uploadMesh(const std::span<uint32_t>& indices, const std::span<Vertex>& vertices)
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
	AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
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
	void VulkanEngine::destroyBuffer(const AllocatedBuffer& buffer)
	{
		vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
	}

	// Create Swapchain
	void VulkanEngine::createSwapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };
		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_SwapchainImageFormat,.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_Swapchain = vkbSwapchain.swapchain;
		m_SwapchainExtent = m_WindowExtent;
		m_SwapchainImages = vkbSwapchain.get_images().value();
		m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	// Destroy Swapchain
	void VulkanEngine::destroySwapchain()
	{
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
		}
	}

	// Build Pipeline for Metallic-Roughness Material
	void MetallicRoughnessMaterial::buildPipeline(VulkanEngine* engine)
	{
		VkShaderModule meshFragShader;
		if (!vkUtil::loadShaderModule("shaders/mesh.frag.spv", engine->m_Device, &meshFragShader)) {
			fmt::print("Error when building the triangle fragment shader module\n");
		}

		VkShaderModule meshVertexShader;
		if (!vkUtil::loadShaderModule("shaders/mesh.vert.spv", engine->m_Device, &meshVertexShader)) {
			fmt::print("Error when building the triangle vertex shader module\n");
		}

		VkPushConstantRange matrixRange{};
		matrixRange.offset = 0;
		matrixRange.size = sizeof(GPUDrawPushConstants);
		matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		DescriptorLayoutBuilder layoutBuilder;
		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		materialDescriptorLayout = layoutBuilder.build(engine->m_Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		VkDescriptorSetLayout layouts[] = { engine->m_SceneDescriptorLayout, materialDescriptorLayout };

		VkPipelineLayoutCreateInfo meshLayoutInfo = vkInit::pipelineLayoutCreateInfo();
		meshLayoutInfo.setLayoutCount = 2;
		meshLayoutInfo.pSetLayouts = layouts;
		meshLayoutInfo.pPushConstantRanges = &matrixRange;
		meshLayoutInfo.pushConstantRangeCount = 1;

		VkPipelineLayout newLayout;
		VK_CHECK(vkCreatePipelineLayout(engine->m_Device, &meshLayoutInfo, nullptr, &newLayout));

		opaquePipeline.layout = newLayout;
		transparentPipeline.layout = newLayout;

		// Build the stage-create-info for both vertex and fragment stages. This lets
		// the pipeline know the shader modules per stage
		PipelineBuilder pipelineBuilder;
		opaquePipeline.pipeline = pipelineBuilder.setShaders(meshVertexShader, meshFragShader)
			.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setPolygonMode(VK_POLYGON_MODE_FILL)
			.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
			.setMultisamplingNone()
			.disableBlending()
			.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
			.setColorAttachmentFormat(engine->m_DrawImage.format)
			.setDepthFormat(engine->m_DepthImage.format)
			.setPipelineLayout(newLayout).build(engine->m_Device);

		// Create the transparent variant
		transparentPipeline.pipeline = pipelineBuilder.enableBlendingAdditive().
			enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL).build(engine->m_Device);

		// Destroy shader modules properly
		vkDestroyShaderModule(engine->m_Device, meshFragShader, nullptr);
		vkDestroyShaderModule(engine->m_Device, meshVertexShader, nullptr);
	}

	// Clear Resources (Placeholder)
	void MetallicRoughnessMaterial::clearResources(VkDevice device)
	{
	}

	// Write Material
	MaterialInstance MetallicRoughnessMaterial::createMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator)
	{
		MaterialInstance matData;
		matData.passType = pass;
		if (pass == MaterialPass::Transparent) {
			matData.pipeline = &transparentPipeline;
		}
		else {
			matData.pipeline = &opaquePipeline;
		}

		matData.descriptorSet = descriptorAllocator.allocate(device, materialDescriptorLayout);

		descriptorWriter.clear();
		descriptorWriter.writeBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		descriptorWriter.writeImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorWriter.writeImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		descriptorWriter.updateSet(device, matData.descriptorSet);

		return matData;
	}

	// draw SceneNode
	void MeshNode::draw(const glm::mat4& parentTransform, DrawingContext& context)
	{
		glm::mat4 nodeMatrix = parentTransform * worldTransform;

		for (auto& s : meshAsset->surfaces) {
			RenderableObject def;
			def.indexCount = s.count;
			def.firstIndex = s.startIndex;
			def.indexBuffer = meshAsset->meshBuffers.indexBuffer.buffer;
			def.materialInstance = &s.material->data;

			def.transformMatrix = nodeMatrix;
			def.vertexBufferAddress = meshAsset->meshBuffers.vertexBufferAddress;

			context.opaqueObjects.push_back(def);
		}
		VmaAllocation vma;
		// Recurse down
		SceneNode::draw(parentTransform, context);
	}
}