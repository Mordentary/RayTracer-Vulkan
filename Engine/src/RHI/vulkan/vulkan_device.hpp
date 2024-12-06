#pragma once
#include "../command_list.hpp"
#include "../device.hpp"
#include "../swapchain.hpp"
#include "../types.hpp"
#include "engine_core.h"
#include"vulkan_deletion_queue.hpp"
#include"vulkan_descriptor_allocator.hpp"
#include"vulkan_constant_buffer_allocator.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan\vulkan_core.h>

namespace rhi::vulkan
{
	class VulkanDevice final : public rhi::Device {
	public:
		VulkanDevice(const DeviceDescription& desc);
		~VulkanDevice();

		// Device interface implementation
		virtual void* getHandle() const override { return m_Device; }
		void beginFrame() override;
		void endFrame() override;
		uint32_t getFrameID() const override { return m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT; };

		virtual CommandList* createCommandList(CommandType type, const std::string& name) override;
		virtual Swapchain* createSwapchain(const SwapchainDescription& desc, const std::string& name) override;
		virtual Fence* createFence(const std::string& name) override;
		virtual Buffer* createBuffer(const BufferDescription& desc, const std::string& name) override;
		virtual Texture* createTexture(const TextureDescription& desc, const std::string& name) override;
		virtual Shader* createShader(const ShaderDescription& desc, std::span<uint8_t> data, const std::string& name) override;
		virtual Pipeline* createGraphicsPipelineState(const GraphicsPipelineDescription& desc, const std::string& name) override;
		virtual Pipeline* createComputePipelineState(const ComputePipelineDescription& desc, const std::string& name) override;
		virtual Descriptor* createShaderResourceDescriptor(Resource* resource, const ShaderResourceDescriptorDescription& desc, const std::string& name) override;
		virtual Descriptor* createUnorderedAccessDescriptor(Resource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name) override;
		virtual Descriptor* createConstantBufferDescriptor(Buffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name) override;
		virtual Descriptor* createSampler(const SamplerDescription& desc, const std::string& name) override;

		//Descriptors
		uint32_t allocateResourceDescriptor(void** descriptor);
		uint32_t allocateSamplerDescriptor(void** descriptor);
		void freeResourceDescriptor(uint32_t index);
		void freeSamplerDescriptor(uint32_t index);

		VkDeviceAddress allocateUniformBuffer(const void* data, size_t data_size);
		VkDeviceSize allocateUniformBufferDescriptor(const uint32_t* cbv0, const VkDescriptorAddressInfoEXT& ubv1, const VkDescriptorAddressInfoEXT& ubv2);

		//Deletion
		template<typename T>
		void enqueueDeletion(T objectHandle);

		// Vulkan-specific accessors needed by other Vulkan RHI classes
		VkDevice getDevice() const { return m_Device; }
		VkInstance getInstance() const { return m_Instance; }
		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
		uint32_t getGraphicsQueueIndex() const { return m_GraphicsQueueIndex; }
		uint32_t getComputeQueueIndex() const { return m_ComputeQueueIndex; }
		uint32_t getCopyQueueIndex() const { return m_CopyQueueIndex; }
		VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue getComputeQueue() const { return m_ComputeQueue; }
		VkQueue getCopyQueue() const { return m_CopyQueue; }
		VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }
		VmaAllocator getVmaAllocator() const { return m_Allocator; }

		VulkanConstantBufferAllocator* getConstantBufferAllocator() const;
		VulkanDescriptorAllocator* getResourceDescriptorAllocator() const { return m_ResourceDescriptorAllocator.get(); }
		VulkanDescriptorAllocator* getSamplerDescriptorAllocator() const { return m_SamplerDescriptorAllocator.get(); }
		const VkPhysicalDeviceDescriptorBufferPropertiesEXT& getDescriptorBufferProperties() const { return m_DescriptorBufferProperties; }

		void enqueueDefaultLayoutTransition(Texture* texture);
		void cancelLayoutTransition(Texture* texture);
		void flushLayoutTransition(CommandType type);
	private:
		VulkanDevice() = default;
		bool create(const DeviceDescription& desc);
		bool createPipelineLayout();
		bool createDevice();
	private:
		// Core Vulkan objects
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout[3] = {};
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPhysicalDeviceDescriptorBufferPropertiesEXT m_DescriptorBufferProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };

		SE::Scoped<VulkanConstantBufferAllocator> m_ConstantBufferAllocators[SE::SE_MAX_FRAMES_IN_FLIGHT]{};
		SE::Scoped<VulkanDescriptorAllocator> m_ResourceDescriptorAllocator = nullptr;
		SE::Scoped<VulkanDescriptorAllocator>m_SamplerDescriptorAllocator = nullptr;

		// Queue management
		uint32_t m_GraphicsQueueIndex = uint32_t(-1);
		uint32_t m_ComputeQueueIndex = uint32_t(-1);
		uint32_t m_CopyQueueIndex = uint32_t(-1);
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;
		VkQueue m_CopyQueue = VK_NULL_HANDLE;

		SE::Scoped<VulkanDeletionQueue> m_DeletionQueue = nullptr;
		SE::Scoped<CommandList> m_TransitionCopyCommandList[SE::SE_MAX_FRAMES_IN_FLIGHT] = {};
		SE::Scoped<CommandList> m_TransitionGraphicsCommandList[SE::SE_MAX_FRAMES_IN_FLIGHT] = {};
		std::vector<std::pair<Texture*, ResourceAccessFlags>> m_PendingGraphicsTransitions;
		std::vector<std::pair<Texture*, ResourceAccessFlags>> m_PendingCopyTransitions;
	};
	template<typename T>
	void VulkanDevice::enqueueDeletion(T objectHandle)
	{
		if (objectHandle != VK_NULL_HANDLE)
		{
			m_DeletionQueue->enqueue(objectHandle, m_FrameID);
		}
	}
}