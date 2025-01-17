#pragma once

#include "vulkan_core.hpp"
#include "../descriptor.hpp"
#include "../buffer.hpp"
#include <string>

namespace rhi::vulkan {
	class Device;
	class VulkanDevice;

	class VulkanShaderResourceViewDescriptor : public IDescriptor {
	public:
		VulkanShaderResourceViewDescriptor(VulkanDevice* device, IResource* resource, const ShaderResourceViewDescriptorDescription& desc, const std::string& name);
		~VulkanShaderResourceViewDescriptor();
		bool create();
		virtual void* getHandle() const override { return m_Resource->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		IResource* m_Resource = nullptr;
		ShaderResourceViewDescriptorDescription m_Description = {};
		VkImageView m_ImageView = VK_NULL_HANDLE;
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanUnorderedAccessDescriptor : public IDescriptor {
	public:
		VulkanUnorderedAccessDescriptor(VulkanDevice* device, IResource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name);
		~VulkanUnorderedAccessDescriptor();
		bool create();
		const UnorderedAccessDescriptorDescription& getDescription() const { return m_Description; }
		virtual void* getHandle() const override { return m_Resource->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		IResource* m_Resource = nullptr;
		UnorderedAccessDescriptorDescription m_Description = {};
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkBufferView m_BufferView = VK_NULL_HANDLE; // For storage buffers.
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanConstantBufferDescriptor : public IDescriptor
	{
	public:
		VulkanConstantBufferDescriptor(VulkanDevice* pDevice, IBuffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name);
		~VulkanConstantBufferDescriptor();

		bool create();

		virtual void* getHandle() const override { return m_Buffer->getHandle(); }
		virtual uint32_t getDescriptorArrayIndex() const override { return m_HeapIndex; }

	private:
		IBuffer* m_Buffer = nullptr;
		ConstantBufferDescriptorDescription m_Description = {};
		uint32_t m_HeapIndex = RHI_INVALID_RESOURCE;
	};

	class VulkanSamplerDescriptor final : public IDescriptor {
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