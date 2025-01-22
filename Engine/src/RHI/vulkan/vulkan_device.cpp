#include "vulkan_device.hpp"
#include "vulkan_device.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_core.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_fence.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_deletion_queue.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_command_list.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_heap.hpp"
#include <VkBootstrap.h>
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

	VulkanConstantBufferAllocator* VulkanDevice::getConstantBufferAllocator() const
	{
		uint32_t index = m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT;
		return m_ConstantBufferAllocators[index].get();
	}

	void VulkanDevice::enqueueDefaultLayoutTransition(ITexture* texture) {
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
				accessFlags |= ResourceAccessFlags::MaskDepthStencilAccess;
			}
			if (anySet(desc.usage, TextureUsageFlags::ShaderStorage)) {
				accessFlags |= ResourceAccessFlags::MaskShaderStorage;
			}
			if (accessFlags == ResourceAccessFlags::None) {
				accessFlags = ResourceAccessFlags::TransferDst;
			}
		}

		// Enqueue transitions based on accessFlags using AnySet
		if (anySet(accessFlags, ResourceAccessFlags::RenderTarget | ResourceAccessFlags::MaskDepthStencilAccess |
			ResourceAccessFlags::MaskShaderStorage | ResourceAccessFlags::Present)) {
			m_PendingGraphicsTransitions.emplace_back(texture, accessFlags);
		}
		else if (anySet(accessFlags, ResourceAccessFlags::TransferSrc | ResourceAccessFlags::TransferDst)) {
			m_PendingCopyTransitions.emplace_back(texture, accessFlags);
		}
		else {
			// Default to graphics transitions
			m_PendingGraphicsTransitions.emplace_back(texture, accessFlags);
		}
	}

	void VulkanDevice::cancelLayoutTransition(ITexture* texture) {
		auto removeTransition = [texture](std::vector<std::pair<ITexture*, ResourceAccessFlags>>& transitions) {
			transitions.erase(std::remove_if(transitions.begin(), transitions.end(),
				[texture](const auto& pair) { return pair.first == texture; }), transitions.end());
			};

		removeTransition(m_PendingGraphicsTransitions);
		removeTransition(m_PendingCopyTransitions);
	}

	void VulkanDevice::flushLayoutTransition(CommandType transitionType) {
		const uint32_t frameIndex = m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT;

		if (transitionType == CommandType::Graphics) {
			if (!m_PendingGraphicsTransitions.empty()) {
				auto cmdList = m_TransitionGraphicsCommandList[frameIndex].get();
				cmdList->begin();

				// Process graphics transitions
				for (const auto& [texture, accessFlags] : m_PendingGraphicsTransitions) {
					cmdList->textureBarrier(texture,
						ResourceAccessFlags::Discard, accessFlags);
				}
				m_PendingGraphicsTransitions.clear();

				cmdList->end();
				cmdList->submit();
			}
		}
		else if (transitionType == CommandType::Copy) {
			if (!m_PendingCopyTransitions.empty()) {
				auto cmdList = m_TransitionCopyCommandList[frameIndex].get();
				cmdList->begin();

				// Process copy transitions
				for (const auto& [texture, accessFlags] : m_PendingCopyTransitions) {
					cmdList->textureBarrier(texture,
						ResourceAccessFlags::Discard, accessFlags);
				}
				m_PendingCopyTransitions.clear();

				cmdList->end();
				cmdList->submit();
			}
		}
	}
	bool VulkanDevice::create(const DeviceDescription& desc)
	{
		VK_CHECK(volkInitialize());
		SE_ASSERT(createDevice(), "Device creation failed");
		SE_ASSERT(createPipelineLayout(), "PipelineLayout creation failed");

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

		m_DeletionQueue = SE::CreateScoped<VulkanDeletionQueue>(this);

		for (size_t i = 0; i < SE::SE_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_TransitionCopyCommandList[i] = SE::Scoped<ICommandList>(createCommandList(CommandType::Copy, "Transition CommandList[Transfer]"));
			m_TransitionGraphicsCommandList[i] = SE::Scoped<ICommandList>(createCommandList(CommandType::Graphics, "Transition CommandList[Graphics]"));
			m_ConstantBufferAllocators[i] = SE::CreateScoped<VulkanConstantBufferAllocator>(this, 8 * 1024 * 1024);
		}

		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		properties.pNext = &m_DescriptorBufferProperties;
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &properties);

		size_t resourceDescriptorSize = m_DescriptorBufferProperties.sampledImageDescriptorSize;
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.storageImageDescriptorSize);
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.robustUniformTexelBufferDescriptorSize);
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.robustStorageTexelBufferDescriptorSize);
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.robustUniformBufferDescriptorSize);
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.robustStorageBufferDescriptorSize);
		resourceDescriptorSize = std::max(resourceDescriptorSize, m_DescriptorBufferProperties.accelerationStructureDescriptorSize);

		size_t samplerDescriptorSize = m_DescriptorBufferProperties.samplerDescriptorSize;
		m_ResourceDescriptorAllocator = SE::CreateScoped<VulkanDescriptorAllocator>(this, (uint32_t)resourceDescriptorSize, SE_MAX_RESOURCE_DESCRIPTOR_COUNT, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT);
		m_SamplerDescriptorAllocator = SE::CreateScoped<VulkanDescriptorAllocator>(this, (uint32_t)samplerDescriptorSize, SE_MAX_SAMPLER_DESCRIPTOR_COUNT, VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT);

		return true;
	}

	bool VulkanDevice::createPipelineLayout()
	{
		VkDescriptorType mutableDescriptorTypes[7] =
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
		};

		VkMutableDescriptorTypeListEXT mutableDescriptorList;
		mutableDescriptorList.descriptorTypeCount = 7;
		mutableDescriptorList.pDescriptorTypes = mutableDescriptorTypes;

		VkMutableDescriptorTypeCreateInfoEXT mutableDescriptorInfo = { VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT };
		mutableDescriptorInfo.mutableDescriptorTypeListCount = 1;
		mutableDescriptorInfo.pMutableDescriptorTypeLists = &mutableDescriptorList;

		VkDescriptorSetLayoutBinding uniformBuffer[SE_MAX_UBV_BINDINGS] = {};
		uniformBuffer[0].descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
		uniformBuffer[0].descriptorCount = sizeof(uint32_t) * SE_MAX_PUSH_CONSTANTS;
		uniformBuffer[0].stageFlags = VK_SHADER_STAGE_ALL;

		for (uint32_t i = 1; i < SE_MAX_UBV_BINDINGS; ++i)
		{
			uniformBuffer[i].binding = i;
			uniformBuffer[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBuffer[i].descriptorCount = 1;
			uniformBuffer[i].stageFlags = VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutBinding mutableResourceBinding = {};
		mutableResourceBinding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
		mutableResourceBinding.descriptorCount = SE_MAX_RESOURCE_DESCRIPTOR_COUNT;
		mutableResourceBinding.stageFlags = VK_SHADER_STAGE_ALL;

		VkDescriptorSetLayoutBinding staticSamplerBinding = {};
		staticSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		staticSamplerBinding.descriptorCount = SE_MAX_SAMPLER_DESCRIPTOR_COUNT;
		staticSamplerBinding.stageFlags = VK_SHADER_STAGE_ALL;

		VkDescriptorSetLayoutCreateInfo setLayoutInfo0 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; // constant buffers
		setLayoutInfo0.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		setLayoutInfo0.bindingCount = SE_MAX_UBV_BINDINGS;
		setLayoutInfo0.pBindings = uniformBuffer;

		VkDescriptorSetLayoutCreateInfo setLayoutInfo1 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; // resource descriptor array
		setLayoutInfo1.pNext = &mutableDescriptorInfo;
		setLayoutInfo1.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		setLayoutInfo1.bindingCount = 1;
		setLayoutInfo1.pBindings = &mutableResourceBinding;

		VkDescriptorSetLayoutCreateInfo setLayoutInfo2 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO }; // sampler descriptor array
		setLayoutInfo2.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		setLayoutInfo2.bindingCount = 1;
		setLayoutInfo2.pBindings = &staticSamplerBinding;

		vkCreateDescriptorSetLayout(m_Device, &setLayoutInfo0, nullptr, &m_descriptorSetLayout[0]);
		vkCreateDescriptorSetLayout(m_Device, &setLayoutInfo1, nullptr, &m_descriptorSetLayout[1]);
		vkCreateDescriptorSetLayout(m_Device, &setLayoutInfo2, nullptr, &m_descriptorSetLayout[2]);

		VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		createInfo.setLayoutCount = 3;
		createInfo.pSetLayouts = m_descriptorSetLayout;

		VK_CHECK_RETURN(vkCreatePipelineLayout(m_Device, &createInfo, nullptr, &m_PipelineLayout), false, "Pipeline layout creation failed!");

		setDebugName(m_Device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, m_PipelineLayout, "Default pipeline layout");
		return true;
	}

	vkb::Instance createInstance(const DeviceDescription& desc)
	{
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
			.request_validation_layers(desc.enableValidation)
			.set_debug_callback(debugCallback)
			.set_debug_callback_user_data_pointer(&createInfo)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
			.require_api_version(1, 3, 0);

		// Enable Khronos validation layer if available
		if (khronosValidationAvailable && desc.enableValidation) {
			builder.enable_layer(khronosValidationLayer);
		}

		// Only add debug utils extension in debug builds
#ifdef _DEBUG
		builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		auto built_instance = builder.build();
		SE_ASSERT(built_instance.has_value(), "Failed to create Vulkan instance!");

		auto vkb_inst = built_instance.value();

		return vkb_inst;
	}
	bool VulkanDevice::createDevice()
	{
		auto vkbInst = createInstance(m_Description);

		m_Instance = vkbInst.instance;
		m_DebugMessenger = vkbInst.debug_messenger;
		volkLoadInstance(m_Instance);

		// Required extensions stay the same, just organized better
		const std::vector<const char*> required_extensions = {
			"VK_KHR_swapchain",
			"VK_KHR_maintenance1",
			"VK_KHR_maintenance2",
			"VK_KHR_maintenance3",
			"VK_KHR_maintenance4",
			"VK_KHR_buffer_device_address",
			"VK_KHR_deferred_host_operations",
			"VK_KHR_acceleration_structure",
			"VK_KHR_ray_query",
			"VK_KHR_dynamic_rendering",
			"VK_KHR_synchronization2",
			"VK_KHR_copy_commands2",
			"VK_KHR_bind_memory2",
			"VK_KHR_timeline_semaphore",
			"VK_KHR_dedicated_allocation",
			"VK_EXT_descriptor_indexing",
			"VK_EXT_mutable_descriptor_type",
			"VK_EXT_descriptor_buffer",
			"VK_EXT_scalar_block_layout"
		};

		// Chain features properly
		VkPhysicalDeviceVulkan12Features vulkan12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		vulkan12Features.descriptorIndexing = VK_TRUE;
		vulkan12Features.bufferDeviceAddress = VK_TRUE;

		VkPhysicalDeviceVulkan13Features vulkan13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		vulkan13Features.pNext = &vulkan12Features;
		vulkan13Features.dynamicRendering = VK_TRUE;
		vulkan13Features.synchronization2 = VK_TRUE;
		vulkan13Features.inlineUniformBlock = VK_TRUE;

		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
		mutableDescriptorFeatures.pNext = &vulkan13Features;
		mutableDescriptorFeatures.mutableDescriptorType = VK_TRUE;

		VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
		descriptorBufferFeatures.pNext = &mutableDescriptorFeatures;
		descriptorBufferFeatures.descriptorBuffer = VK_TRUE;

		VkPhysicalDeviceFeatures2 features2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		features2.pNext = &descriptorBufferFeatures;

		vkb::PhysicalDeviceSelector selector{ vkbInst };
		selector.set_minimum_version(1, 3)
			.add_required_extensions(required_extensions)
			.add_required_extension_features(descriptorBufferFeatures);

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

		//if (!vkbPhysicalDevice.enable_extension_features_if_present(meshShaderFeatures)) {
		//	SE::LogWarn("Mesh Shader features not supported by the selected physical device");
		//	return EXIT_FAILURE;
		//}

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
	}

	VulkanDevice::VulkanDevice(const DeviceDescription& desc)
	{
		create(desc);
	}

	VulkanDevice::~VulkanDevice()
	{
		for (size_t i = 0; i < SE::SE_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_TransitionCopyCommandList[i].reset();
			m_TransitionGraphicsCommandList[i].reset();
			m_ConstantBufferAllocators[i].reset();
		}
		m_DeletionQueue.reset();
		m_ResourceDescriptorAllocator.reset();
		m_SamplerDescriptorAllocator.reset();

		vmaDestroyAllocator(m_Allocator);
		vkDestroyDescriptorSetLayout(m_Device, m_descriptorSetLayout[0], nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_descriptorSetLayout[1], nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_descriptorSetLayout[2], nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	IBuffer* VulkanDevice::createBuffer(const BufferDescription& desc, const std::string& name) {
		VulkanBuffer* buffer = new VulkanBuffer(this, desc, name);
		if (!buffer->create())
		{
			delete buffer;
			return nullptr;
		}
		return buffer;
	}

	ITexture* VulkanDevice::createTexture(const TextureDescription& desc, const std::string& name) {
		VulkanTexture* texture = new VulkanTexture(this, desc, name);
		if (!texture->create())
		{
			delete texture;
			return nullptr;
		}
		return texture;
	}

	IShader* VulkanDevice::createShader(const ShaderDescription& desc, std::span<uint8_t> data, const std::string& name)
	{
		VulkanShader* shader = new VulkanShader(this, desc, name);
		if (!shader->create(data))
		{
			delete shader;
			return nullptr;
		}
		return shader;
	}

	IPipelineState* VulkanDevice::createGraphicsPipelineState(const GraphicsPipelineDescription& desc, const std::string& name)
	{
		VulkanGraphicsPipelineState* pipeline = new VulkanGraphicsPipelineState(this, desc, name);
		if (!pipeline->create())
		{
			delete pipeline;
			return nullptr;
		}
		return pipeline;
	}

	IPipelineState* VulkanDevice::createComputePipelineState(const ComputePipelineDescription& desc, const std::string& name)
	{
		VulkanComputePipelineState* pipeline = new VulkanComputePipelineState(this, desc, name);
		if (!pipeline->create())
		{
			delete pipeline;
			return nullptr;
		}
		return pipeline;
	}
	ISwapchain* VulkanDevice::createSwapchain(const SwapchainDescription& desc, const std::string& name) {
		VulkanSwapchain* swapchain = new VulkanSwapchain(this, desc, name);
		if (!swapchain->create())
		{
			delete swapchain;
			return nullptr;
		}
		return swapchain;
	}

	ICommandList* VulkanDevice::createCommandList(CommandType type, const std::string& name) {
		VulkanCommandList* commandList = new VulkanCommandList(this, type, name);
		if (!commandList->create())
		{
			delete commandList;
			return nullptr;
		}
		return commandList;
	}
	IFence* VulkanDevice::createFence(const std::string& name) {
		VulkanFence* fence = new VulkanFence(this, name);
		if (!fence->create())
		{
			delete fence;
			return nullptr;
		}
		return fence;
	}

	IDescriptor* VulkanDevice::createShaderResourceViewDescriptor(IResource* resource, const ShaderResourceViewDescriptorDescription& desc, const std::string& name)
	{
		VulkanShaderResourceViewDescriptor* resourceDescriptor = new VulkanShaderResourceViewDescriptor(this, resource, desc, name);
		if (!resourceDescriptor->create())
		{
			delete resourceDescriptor;
			return nullptr;
		}
		return resourceDescriptor;
	}

	IDescriptor* VulkanDevice::createUnorderedAccessDescriptor(IResource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name)
	{
		VulkanUnorderedAccessDescriptor* storageDescriptor = new VulkanUnorderedAccessDescriptor(this, resource, desc, name);
		if (!storageDescriptor->create())
		{
			delete storageDescriptor;
			return nullptr;
		}
		return storageDescriptor;
	}

	IDescriptor* VulkanDevice::createConstantBufferDescriptor(IBuffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name)
	{
		VulkanConstantBufferDescriptor* uniformDescriptor = new VulkanConstantBufferDescriptor(this, buffer, desc, name);
		if (!uniformDescriptor->create())
		{
			delete uniformDescriptor;
			return nullptr;
		}
		return uniformDescriptor;
	}

	IDescriptor* VulkanDevice::createSampler(const SamplerDescription& desc, const std::string& name)
	{
		VulkanSamplerDescriptor* samplerDescriptor = new VulkanSamplerDescriptor(this, desc, name);
		if (!samplerDescriptor->create())
		{
			delete samplerDescriptor;
			return nullptr;
		}
		return samplerDescriptor;
	}

	IHeap* VulkanDevice::createHeap(const HeapDescription& desc, const std::string& name)
	{
		VulkanHeap* heap = new VulkanHeap(this, desc, name);
		if (!heap->create())
		{
			delete heap;
			return nullptr;
		}
		return heap;
	}

	uint32_t VulkanDevice::getAllocationSize(const rhi::TextureDescription& desc)
	{
		auto iter = m_TextureSizeMap.find(desc);
		if (iter != m_TextureSizeMap.end())
		{
			return iter->second;
		}

		VkImageCreateInfo createInfo = toImageCreateInfo(desc);
		VkImage image;
		VkResult result = vkCreateImage(m_Device, &createInfo, nullptr, &image);
		if (result != VK_SUCCESS)
		{
			return 0;
		}

		VkImageMemoryRequirementsInfo2 info = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2 };
		info.image = image;

		VkMemoryRequirements2 requirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
		vkGetImageMemoryRequirements2(m_Device, &info, &requirements);

		vkDestroyImage(m_Device, image, nullptr);

		m_TextureSizeMap.emplace(desc, requirements.memoryRequirements.size);
		return (uint32_t)requirements.memoryRequirements.size;

		return 0;
	}

	uint32_t VulkanDevice::allocateResourceDescriptor(void** descriptor)
	{
		return m_ResourceDescriptorAllocator->allocate(descriptor);
	}

	uint32_t VulkanDevice::allocateSamplerDescriptor(void** descriptor)
	{
		return m_SamplerDescriptorAllocator->allocate(descriptor);
	}

	void VulkanDevice::freeResourceDescriptor(uint32_t index)
	{
		m_ResourceDescriptorAllocator->free(index);
	}

	void VulkanDevice::freeSamplerDescriptor(uint32_t index)
	{
		m_SamplerDescriptorAllocator->free(index);
	}

	VkDeviceAddress VulkanDevice::allocateUniformBuffer(const void* data, size_t data_size)
	{
		void* cpuAddress;
		VkDeviceAddress gpuAddress;
		getConstantBufferAllocator()->allocate((uint32_t)data_size, &cpuAddress, &gpuAddress);

		memcpy(cpuAddress, data, data_size);

		return gpuAddress;
	}

	VkDeviceSize VulkanDevice::allocateUniformBufferDescriptor(const uint32_t* cbv0, const VkDescriptorAddressInfoEXT& ubv1, const VkDescriptorAddressInfoEXT& ubv2)
	{
		size_t descriptorBufferSize = sizeof(uint32_t) * SE_MAX_PUSH_CONSTANTS + m_DescriptorBufferProperties.robustUniformBufferDescriptorSize * 2;
		void* cpuAddress;
		VkDeviceAddress gpuAddress;
		getConstantBufferAllocator()->allocate((uint32_t)descriptorBufferSize, &cpuAddress, &gpuAddress);

		memcpy(cpuAddress, cbv0, sizeof(uint32_t) * SE_MAX_PUSH_CONSTANTS);

		VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		descriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		if (ubv1.address != 0)
		{
			descriptorInfo.data.pUniformBuffer = &ubv1;
			vkGetDescriptorEXT(m_Device, &descriptorInfo, m_DescriptorBufferProperties.robustUniformBufferDescriptorSize,
				(char*)cpuAddress + sizeof(uint32_t) * SE_MAX_PUSH_CONSTANTS);
		}

		if (ubv2.address != 0)
		{
			descriptorInfo.data.pUniformBuffer = &ubv2;
			vkGetDescriptorEXT(m_Device, &descriptorInfo, m_DescriptorBufferProperties.robustUniformBufferDescriptorSize,
				(char*)cpuAddress + sizeof(uint32_t) * SE_MAX_PUSH_CONSTANTS + m_DescriptorBufferProperties.robustUniformBufferDescriptorSize);
		}

		VkDeviceSize descriptorBufferOffset = gpuAddress - getConstantBufferAllocator()->getGpuAddress();
		return descriptorBufferOffset;
	}

	void VulkanDevice::beginFrame()
	{
		m_DeletionQueue->flush();
		uint32_t index = m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT;

		m_TransitionCopyCommandList[index]->resetAllocator();
		m_TransitionGraphicsCommandList[index]->resetAllocator();

		m_ConstantBufferAllocators[index]->reset();
	}

	void VulkanDevice::endFrame()
	{
		++m_FrameID;
		vmaSetCurrentFrameIndex(m_Allocator, (uint32_t)m_FrameID);
	}
}