#include "vulkan_device.hpp"
#include<VkBootstrap.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "vulkan_texture.hpp"
#include "vulkan_core.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_fence.hpp"

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

	void VulkanDevice::enqueueDefaultLayoutTransition(Texture* texture) {
		const TextureDescription& desc = texture->getDescription();

		ResourceAccessFlags accessFlags = ResourceAccessFlags::None;

		if (static_cast<VulkanTexture*>(texture)->isSwapchainTexture()) {
			accessFlags = ResourceAccessFlags::Present;
		}
		else {
			// Map TextureUsageFlags to ResourceAccessFlags using AnySet
			if (anySet(desc.usage, TextureUsageFlags::RenderTarget)) {
				accessFlags |= ResourceAccessFlags::RenderTarget;
			}
			if (anySet(desc.usage, TextureUsageFlags::DepthStencil)) {
				accessFlags |= ResourceAccessFlags::DepthStencilAccess;
			}
			if (anySet(desc.usage, TextureUsageFlags::UnorderedAccess)) {
				accessFlags |= ResourceAccessFlags::UnorderedAccess;
			}
			if (anySet(desc.usage, TextureUsageFlags::ShaderResource)) {
				accessFlags |= ResourceAccessFlags::ShaderResourceAccess;
			}
			if (anySet(desc.usage, TextureUsageFlags::TransferSrc)) {
				accessFlags |= ResourceAccessFlags::TransferSrc;
			}
			if (anySet(desc.usage, TextureUsageFlags::TransferDst)) {
				accessFlags |= ResourceAccessFlags::TransferDst;
			}

			// Default to TransferDst if no access flags are set
			if (accessFlags == ResourceAccessFlags::None) {
				accessFlags = ResourceAccessFlags::TransferDst;
			}
		}

		// Enqueue transitions based on accessFlags using AnySet
		if (anySet(accessFlags, ResourceAccessFlags::RenderTarget | ResourceAccessFlags::DepthStencilAccess |
			ResourceAccessFlags::UnorderedAccess | ResourceAccessFlags::ShaderResourceAccess | ResourceAccessFlags::Present)) {
			m_pendingGraphicsTransitions.emplace_back(texture, accessFlags);
		}
		else if (anySet(accessFlags, ResourceAccessFlags::TransferSrc | ResourceAccessFlags::TransferDst)) {
			m_pendingCopyTransitions.emplace_back(texture, accessFlags);
		}
		else {
			// Default to graphics transitions
			m_pendingGraphicsTransitions.emplace_back(texture, accessFlags);
		}
	}

	void VulkanDevice::cancelLayoutTransition(Texture* texture) {
		auto removeTransition = [texture](std::vector<std::pair<Texture*, ResourceAccessFlags>>& transitions) {
			transitions.erase(std::remove_if(transitions.begin(), transitions.end(),
				[texture](const auto& pair) { return pair.first == texture; }), transitions.end());
			};

		removeTransition(m_pendingGraphicsTransitions);
		removeTransition(m_pendingCopyTransitions);
	}

	void VulkanDevice::flushLayoutTransition(CommandType transitionType) {
		//const uint32_t frameIndex = m_FrameCount % SE_MAX_FRAMES_IN_FLIGHT;

		//if (transitionType == CommandType::Graphics) {
		//	if (!m_pendingGraphicsTransitions.empty()) {
		//		auto cmdList = m_transitionGraphicsCmdLists[frameIndex].get();
		//		cmdList->begin();

		//		// Process graphics transitions
		//		for (const auto& [texture, accessFlags] : m_pendingGraphicsTransitions) {
		//			cmdList->textureBarrier(texture, ALL_SUBRESOURCES,
		//				ResourceAccessFlags::Discard, accessFlags);
		//		}
		//		m_pendingGraphicsTransitions.clear();

		//		cmdList->end();
		//		cmdList->submit();
		//	}
		//}
		//else if (queueType == CommandQueueType::Copy) {
		//	if (!m_pendingCopyTransitions.empty()) {
		//		auto cmdList = m_transitionCopyCmdLists[frameIndex].get();
		//		cmdList->begin();

		//		// Process copy transitions
		//		for (const auto& [texture, accessFlags] : m_pendingCopyTransitions) {
		//			cmdList->textureBarrier(texture, ALL_SUBRESOURCES,
		//				ResourceAccessFlags::Discard, accessFlags);
		//		}
		//		m_pendingCopyTransitions.clear();

		//		cmdList->end();
		//		cmdList->submit();
		//	}
		//}
	}
	bool VulkanDevice::init(const DeviceDescription& desc)
	{
		VK_CHECK(volkInitialize());

		vkb::InstanceBuilder builder;

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
			.request_validation_layers(m_Description.enableValidation)
			.set_debug_callback(debugCallback)
			.set_debug_callback_user_data_pointer(&createInfo)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
			.require_api_version(1, 3, 0);

		// Enable Khronos validation layer if available
		if (khronosValidationAvailable && m_Description.enableValidation) {
			builder.enable_layer(khronosValidationLayer);
		}

		// Only add debug utils extension in debug builds
#ifdef _DEBUG
		builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

		auto built_instance = builder.build();
		SE_ASSERT(built_instance, "Failed to create Vulkan instance!");

		auto vkb_inst = built_instance.value();
		m_Instance = vkb_inst.instance;
		m_DebugMessenger = vkb_inst.debug_messenger;
		volkLoadInstance(m_Instance);

		// Define required extensions
		std::vector<const char*> required_extensions = {
			"VK_KHR_swapchain",
			//"VK_KHR_swapchain_mutable_format",
			"VK_KHR_maintenance1",
			"VK_KHR_maintenance2",
			"VK_KHR_maintenance3",
			"VK_KHR_maintenance4",
			"VK_KHR_buffer_device_address",
			//"VK_KHR_deferred_host_operations",
			//"VK_KHR_acceleration_structure",
			//"VK_KHR_ray_query",
			"VK_KHR_dynamic_rendering",
			"VK_KHR_synchronization2",
			"VK_KHR_copy_commands2",
			"VK_KHR_bind_memory2",
			"VK_KHR_timeline_semaphore",
			"VK_KHR_dedicated_allocation",
			"VK_EXT_mesh_shader",
			//"VK_EXT_descriptor_indexing",
			//"VK_EXT_mutable_descriptor_type",
			//"VK_EXT_descriptor_buffer",
			//"VK_EXT_scalar_block_layout",
		};

		// Set up Vulkan feature structures and chain them
		VkPhysicalDeviceVulkan12Features vulkan12Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		vulkan12Features.descriptorIndexing = VK_TRUE;
		vulkan12Features.bufferDeviceAddress = VK_TRUE;

		VkPhysicalDeviceVulkan13Features vulkan13Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		vulkan13Features.pNext = &vulkan12Features;
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;

		//VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
		//rayQueryFeatures.pNext = &vulkan13Features;
		//rayQueryFeatures.rayQuery = VK_TRUE;

		VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
		meshShaderFeatures.pNext = &vulkan13Features;
		meshShaderFeatures.meshShader = VK_TRUE;
		meshShaderFeatures.taskShader = VK_TRUE;

		//VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
		//mutableDescriptorFeatures.pNext = &meshShaderFeatures;
		//mutableDescriptorFeatures.mutableDescriptorType = VK_TRUE;

		//VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
		//descriptorBufferFeatures.pNext = &mutableDescriptorFeatures;
		//descriptorBufferFeatures.descriptorBuffer = VK_TRUE;

		// Root of the pNext chain
		VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		features2.pNext = &vulkan13Features;

		// Select the physical device
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		selector.set_minimum_version(1, 3);

		// Add required extensions
		for (const char* ext : required_extensions) {
			selector.add_required_extension(ext);
		}

		// Add required features
		selector.set_required_features_12(vulkan12Features);
		selector.set_required_features_13(vulkan13Features);

		// Select the physical device
		auto physicalDeviceResult = selector.defer_surface_initialization().select();
		if (!physicalDeviceResult) {
			// Handle error
			SE::LogError("Failed to create physical device: {}", physicalDeviceResult.error().message());
			return EXIT_FAILURE;
		}

		vkb::PhysicalDevice vkbPhysicalDevice = physicalDeviceResult.value();

		//// Enable additional features if present
		//if (!vkbPhysicalDevice.enable_extension_features_if_present(rayQueryFeatures)) {
		//	std::cerr << "Ray Query features not supported by the selected physical device." << std::endl;
		//	return EXIT_FAILURE;
		//}

		if (!vkbPhysicalDevice.enable_extension_features_if_present(meshShaderFeatures)) {
			SE::LogWarn("Mesh Shader features not supported by the selected physical device");
			return EXIT_FAILURE;
		}

		//if (!vkbPhysicalDevice.enable_extension_features_if_present(mutableDescriptorFeatures)) {
		//	std::cerr << "Mutable Descriptor Type features not supported by the selected physical device." << std::endl;
		//	return EXIT_FAILURE;
		//}

		//if (!vkbPhysicalDevice.enable_extension_features_if_present(descriptorBufferFeatures)) {
		//	std::cerr << "Descriptor Buffer features not supported by the selected physical device." << std::endl;
		//	return EXIT_FAILURE;
		//}

		// Create the logical device
		vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };

		// Add pNext chain
		//deviceBuilder.add_pNext(&features2);

		// Build the device
		auto deviceResult = deviceBuilder.build();
		if (!deviceResult) {
			// Handle error

			SE::LogError("Failed to create logical device: {}", deviceResult.error().message());
			return EXIT_FAILURE;
		}

		vkb::Device vkbDevice = deviceResult.value();
		m_PhysicalDevice = vkbPhysicalDevice.physical_device;
		m_Device = vkbDevice.device;
		volkLoadDevice(m_Device);

		m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_GraphicsQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		m_ComputeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
		m_ComputeQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::compute).value();
		m_CopyQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
		m_CopyQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

		VmaVulkanFunctions vmaVulkanFuncs{};
		vmaVulkanFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		vmaVulkanFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_Device;
		allocatorInfo.instance = m_Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pVulkanFunctions = &vmaVulkanFuncs;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		return true;
	}

	VulkanDevice::VulkanDevice(const DeviceDescription& desc)
	{
		init(desc);
		m_DeletionQueue = SE::CreateScoped<VulkanDeletionQueue>(this);
	}

	VulkanDevice::~VulkanDevice()
	{

		for (size_t i = 0; i < SE_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_transitionCopyCommandList[i].reset();
			m_transitionGraphicsCommandList[i].reset();
		}
		m_DeletionQueue.reset();
		vmaDestroyAllocator(m_Allocator);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	Buffer* VulkanDevice::createBuffer(const BufferDescription& desc, const std::string& name) {
		VulkanBuffer* buffer = new VulkanBuffer(this, desc, name);
		if (!buffer->create())
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	Texture* VulkanDevice::createTexture(const TextureDescription& desc, const std::string& name) {
		VulkanTexture* texture = new VulkanTexture(this, desc, name);
		if (!texture->create())
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	Swapchain* VulkanDevice::createSwapchain(const SwapchainDescription& desc, const std::string& name) {
		VulkanSwapchain* swapchain = new VulkanSwapchain(this, desc, name);
		if (!swapchain->create())
		{
			delete swapchain;
			return nullptr;
		}
		return swapchain;
	}

	CommandList* VulkanDevice::createCommandList(CommandType type, const std::string& name) {
		return nullptr;
	}

	void VulkanDevice::beginFrame()
	{
		m_DeletionQueue->flush();

		uint32_t index = m_FrameID % SE_MAX_FRAMES_IN_FLIGHT;
		//m_transitionCopyCommandList[index]->ResetAllocator();
		//m_transitionGraphicsCommandList[index]->ResetAllocator();

		//m_constantBufferAllocators[index]->Reset();

	}

	void VulkanDevice::endFrame()
	{
		++m_FrameID;
		vmaSetCurrentFrameIndex(m_Allocator, (uint32_t)m_FrameID);
	}

	Fence* VulkanDevice::createFence(const std::string& name) {
		VulkanFence* fence = new VulkanFence(this, name);
		if (!fence->create())
		{
			delete fence;
			return nullptr;
		}
		return fence;
	}
}