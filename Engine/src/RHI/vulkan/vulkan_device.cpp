#include "vulkan_device.hpp"
#include<VkBootstrap.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
namespace rhi::vulkan {
	namespace
	{
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
	}

	bool VulkanDevice::init(const DeviceDescription& desc)
	{
		//				// Check for Khronos validation layer
		//				const char* khronosValidationLayer = "VK_LAYER_KHRONOS_validation";
		//				uint32_t layerCount;
		//				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		//				std::vector<VkLayerProperties> availableLayers(layerCount);
		//				vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		//
		//				bool khronosValidationAvailable = false;
		//				for (const auto& layerProperties : availableLayers) {
		//					if (strcmp(khronosValidationLayer, layerProperties.layerName) == 0) {
		//						khronosValidationAvailable = true;
		//						break;
		//					}
		//				}
		//
		//				VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		//				createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		//				createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		//					VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		//					VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		//				createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		//					VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		//					VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		//
		//				createInfo.pfnUserCallback = debugCallback;
		//				createInfo.pUserData = nullptr;
		//
		//				auto inst_ret = builder.set_app_name("SingularityEngine")
		//					.request_validation_layers(m_Desc.enableValidation)
		//					.set_debug_callback(debugCallback)
		//					.set_debug_callback_user_data_pointer(&createInfo)
		//					.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
		//					.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT)
		//					.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
		//					.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
		//					.require_api_version(1, 3, 0);
		//
		//				// Enable Khronos validation layer if available
		//				if (khronosValidationAvailable && m_Desc.enableValidation) {
		//					builder.enable_layer(khronosValidationLayer);
		//				}
		//
		//				// Only add debug utils extension in debug builds
		//#ifdef _DEBUG
		//				builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		//#endif
		//
		//				auto built_instance = builder.build();
		//				if (!built_instance) {
		//					throw std::runtime_error("Failed to create Vulkan instance!");
		//				}
		//
		//				auto vkb_inst = built_instance.value();
		//				m_Instance = vkb_inst.instance;
		//				m_DebugMessenger = vkb_inst.debug_messenger;
		//
		//				SDL_Vulkan_CreateSurface(m_Window, m_Instance, &m_Surface);
		//
		//				VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		//				features13.dynamicRendering = VK_TRUE;
		//				features13.synchronization2 = VK_TRUE;
		//
		//				VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		//				features12.bufferDeviceAddress = VK_TRUE;
		//				features12.descriptorIndexing = VK_TRUE;
		//
		//				vkb::PhysicalDeviceSelector selector{ vkb_inst };
		//				vkb::PhysicalDevice vkbPhysicalDevice = selector
		//					.set_minimum_version(1, 3)
		//					.set_required_features_13(features13)
		//					.set_required_features_12(features12)
		//					.set_surface(m_Surface)
		//					.select()
		//					.value();
		//
		//				vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
		//				vkb::Device vkbDevice = deviceBuilder.build().value();
		//				m_PhysicalDevice = vkbPhysicalDevice.physical_device;
		//				m_Device = vkbDevice.device;
		//
		//				m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		//				m_GraphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		//
		//				VmaAllocatorCreateInfo allocatorInfo = {};
		//				allocatorInfo.physicalDevice = m_PhysicalDevice;
		//				allocatorInfo.device = m_Device;
		//				allocatorInfo.instance = m_Instance;
		//				allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		//
		//				vmaCreateAllocator(&allocatorInfo, &m_Allocator);
		//
		//				m_MainCleanupQueue.enqueueCleanup([&]()
		//					{
		//						vmaDestroyAllocator(m_Allocator);
							//});

		return true;
	}

	VulkanDevice::VulkanDevice(const DeviceDescription& desc)
	{
		init(desc);
	}

	VulkanDevice::~VulkanDevice()
	{
	}

	Buffer* VulkanDevice::createBuffer(const BufferDescription& desc, const void* initialData) {
		return nullptr;
	}

	Texture* VulkanDevice::createTexture(const TextureDescription& desc, const void* initialData) {
		return nullptr;
	}

	Swapchain* VulkanDevice::createSwapchain(const SwapchainDescription& desc) {
		return nullptr;
	}

	CommandList* VulkanDevice::createCommandList(CommandType type) {
		return nullptr;
	}

	void VulkanDevice::submit(CommandList* cmdList, Fence* fence) {
	}

	void VulkanDevice::beginFrame() {
	}

	void VulkanDevice::endFrame() {
	}

	Fence* VulkanDevice::createFence(bool signaled) {
		return nullptr;
	}

	void VulkanDevice::waitForFence(Fence* fence) {
	}

	void VulkanDevice::resetFence(Fence* fence) {
	}

	void VulkanDevice::waitIdle() {
	}

	void VulkanDevice::destroyResource(Resource* resource) {
	}

	void VulkanDevice::setDebugName(Resource* resource, const char* name) {
	}
}