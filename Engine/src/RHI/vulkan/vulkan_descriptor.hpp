#pragma once

#include "vulkan_core.hpp"
#include "../descriptor.hpp"
#include "../buffer.hpp"
#include <string>

namespace rhi::vulkan {
	class Device;
	class VulkanDevice;

	class VulkanShaderResourceDescriptor : public Descriptor {
	public:
		VulkanShaderResourceDescriptor(VulkanDevice* device, Resource* resource, const ShaderResourceDescriptorDescription& desc, const std::string& name);
		~VulkanShaderResourceDescriptor();
		bool create();
		virtual void* getHandle() const override { return m_Resource->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		Resource* m_Resource = nullptr;
		ShaderResourceDescriptorDescription m_Description = {};
		VkImageView m_ImageView = VK_NULL_HANDLE;
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanUnorderedAccessDescriptor : public Descriptor {
	public:
		VulkanUnorderedAccessDescriptor(VulkanDevice* device, Resource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name);
		~VulkanUnorderedAccessDescriptor();
		bool create();
		const UnorderedAccessDescriptorDescription& getDescription() const { return m_Description; }
		virtual void* getHandle() const override { return m_Resource->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		Resource* m_Resource = nullptr;
		UnorderedAccessDescriptorDescription m_Description = {};
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkBufferView m_BufferView = VK_NULL_HANDLE; // For storage buffers.
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanConstantBufferDescriptor : public Descriptor
	{
	public:
		VulkanConstantBufferDescriptor(VulkanDevice* pDevice, Buffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name);
		~VulkanConstantBufferDescriptor();

		bool create();

		virtual void* getHandle() const override { return m_Buffer->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		Buffer* m_Buffer = nullptr;
		ConstantBufferDescriptorDescription m_Description = {};
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanSamplerDescriptor final : public Descriptor {
	public:
		VulkanSamplerDescriptor(VulkanDevice* device, const SamplerDescription& desc, const std::string& name);
		~VulkanSamplerDescriptor();

		bool create();

		void* getHandle() const override { return m_VkSampler; }
		uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		SamplerDescription m_Description;
		VkSampler m_VkSampler = VK_NULL_HANDLE;
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};
}