#include"vulkan_descriptor.hpp"
#include"vulkan_device.hpp"
#include"vulkan_texture.hpp"
namespace rhi::vulkan
{
	VulkanShaderResourceViewDescriptor::VulkanShaderResourceViewDescriptor(VulkanDevice* device, IResource* resource,
		const ShaderResourceViewDescriptorDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Resource = resource;
		m_Description = desc;
	}

	VulkanShaderResourceViewDescriptor::~VulkanShaderResourceViewDescriptor() {
		VulkanDevice* device = (VulkanDevice*)m_Device;
		if (m_ImageView != VK_NULL_HANDLE) {
			device->enqueueDeletion(m_ImageView);
		}
		device->freeResourceDescriptor(m_HeapIndex);
	}
	bool VulkanShaderResourceViewDescriptor::create() {
		VkDevice device = (VkDevice)m_Device->getHandle();
		const VkPhysicalDeviceDescriptorBufferPropertiesEXT& descriptorBufferProperties = ((VulkanDevice*)m_Device)->getDescriptorBufferProperties();

		size_t descriptorSize = 0;
		VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		VkDescriptorAddressInfoEXT bufferInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkImageViewUsageCreateInfo imageViewUsageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
		imageViewUsageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		if (m_Resource && m_Resource->isTexture()) {
			imageViewCreateInfo.pNext = &imageViewUsageInfo;
			imageViewCreateInfo.image = (VkImage)m_Resource->getHandle();
			imageViewCreateInfo.format = toVkFormat(m_Description.format, true);
			imageViewCreateInfo.subresourceRange.aspectMask = getVkAspectMask(m_Description.format);
			imageViewCreateInfo.subresourceRange.baseMipLevel = m_Description.texture.mipSlice;
			imageViewCreateInfo.subresourceRange.levelCount = m_Description.texture.mipLevels;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = m_Description.texture.arraySlice;
			imageViewCreateInfo.subresourceRange.layerCount = m_Description.texture.arraySize;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorInfo.data.pSampledImage = &imageInfo;
			descriptorSize = descriptorBufferProperties.sampledImageDescriptorSize;
		}

		switch (m_Description.type) {
		case ShaderResourceViewDescriptorType::Texture2D: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case ShaderResourceViewDescriptorType::Texture2DArray: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case ShaderResourceViewDescriptorType::Texture3D: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case ShaderResourceViewDescriptorType::TextureCube: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case ShaderResourceViewDescriptorType::TextureCubeArray: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case ShaderResourceViewDescriptorType::StructuredBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::StructuredBuffer));
			assert(m_Description.format == Format::Unknown);
			assert(m_Description.buffer.offset % bufferDesc.stride == 0);
			assert(m_Description.buffer.size % bufferDesc.stride == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorInfo.data.pStorageBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustStorageBufferDescriptorSize;
			break;
		}
		case ShaderResourceViewDescriptorType::FormattedBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::FormattedBuffer));
			assert(m_Description.buffer.offset % bufferDesc.stride == 0);
			assert(m_Description.buffer.size % bufferDesc.stride == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;
			bufferInfo.format = toVkFormat(m_Description.format);

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			descriptorInfo.data.pUniformTexelBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustUniformTexelBufferDescriptorSize;
			break;
		}
		case ShaderResourceViewDescriptorType::RawBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::RawBuffer));
			assert(bufferDesc.stride % 4 == 0);
			assert(m_Description.buffer.offset % 4 == 0);
			assert(m_Description.buffer.size % 4 == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorInfo.data.pStorageBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustStorageBufferDescriptorSize;
			break;
		}
		default:
			break;
		}
		void* pDescriptor = nullptr;
		m_HeapIndex = ((VulkanDevice*)m_Device)->allocateResourceDescriptor(&pDescriptor);

		vkGetDescriptorEXT(device, &descriptorInfo, descriptorSize, pDescriptor);
		return true;
	}

	VulkanUnorderedAccessDescriptor::VulkanUnorderedAccessDescriptor(VulkanDevice* device, IResource* resource,
		const UnorderedAccessDescriptorDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Resource = resource;
		m_Description = desc;
	}

	VulkanUnorderedAccessDescriptor::~VulkanUnorderedAccessDescriptor() {
		VulkanDevice* device = (VulkanDevice*)m_Device;
		if (m_ImageView != VK_NULL_HANDLE) {
			device->enqueueDeletion(m_ImageView);
		}
		if (m_BufferView != VK_NULL_HANDLE) {
			device->enqueueDeletion(m_BufferView);
		}
		device->freeResourceDescriptor(m_HeapIndex);
	}

	bool VulkanUnorderedAccessDescriptor::create() {
		VkDevice device = (VkDevice)m_Device->getHandle();
		const VkPhysicalDeviceDescriptorBufferPropertiesEXT& descriptorBufferProperties = ((VulkanDevice*)m_Device)->getDescriptorBufferProperties();

		size_t descriptorSize = 0;
		VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		VkDescriptorAddressInfoEXT bufferInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkImageViewUsageCreateInfo imageViewUsageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
		imageViewUsageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

		VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		if (m_Resource && m_Resource->isTexture()) {
			const TextureDescription& textureDesc = ((ITexture*)m_Resource)->getDescription();
			assert(anySet(textureDesc.usage, TextureUsageFlags::ShaderStorage));

			imageViewCreateInfo.pNext = &imageViewUsageInfo;
			imageViewCreateInfo.image = (VkImage)m_Resource->getHandle();
			imageViewCreateInfo.format = toVkFormat(m_Description.format);
			imageViewCreateInfo.subresourceRange.aspectMask = getVkAspectMask(m_Description.format);
			imageViewCreateInfo.subresourceRange.baseMipLevel = m_Description.texture.mipSlice;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = m_Description.texture.arraySlice;
			imageViewCreateInfo.subresourceRange.layerCount = m_Description.texture.arraySize;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorInfo.data.pStorageImage = &imageInfo;
			descriptorSize = descriptorBufferProperties.storageImageDescriptorSize;
		}
		switch (m_Description.type) {
		case UnorderedAccessDescriptorType::Texture2D: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case UnorderedAccessDescriptorType::Texture2DArray: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case UnorderedAccessDescriptorType::Texture3D: {
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
			vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_ImageView);
			imageInfo.imageView = m_ImageView;
			break;
		}
		case UnorderedAccessDescriptorType::StructuredBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::StructuredBuffer));
			assert(anySet(bufferDesc.usage, BufferUsageFlags::ShaderStorageBuffer));
			assert(m_Description.format == Format::Unknown);
			assert(m_Description.buffer.offset % bufferDesc.stride == 0);
			assert(m_Description.buffer.size % bufferDesc.stride == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorInfo.data.pStorageBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustStorageBufferDescriptorSize;
			break;
		}
		case UnorderedAccessDescriptorType::FormattedBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::FormattedBuffer));
			assert(anySet(bufferDesc.usage, BufferUsageFlags::ShaderStorageBuffer));
			assert(m_Description.buffer.offset % bufferDesc.stride == 0);
			assert(m_Description.buffer.size % bufferDesc.stride == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;
			bufferInfo.format = toVkFormat(m_Description.format);

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			descriptorInfo.data.pUniformTexelBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustUniformTexelBufferDescriptorSize;
			break;
		}
		case UnorderedAccessDescriptorType::RawBuffer: {
			const BufferDescription& bufferDesc = ((IBuffer*)m_Resource)->getDescription();
			assert(anySet(bufferDesc.usage, BufferUsageFlags::RawBuffer));
			assert(anySet(bufferDesc.usage, BufferUsageFlags::ShaderStorageBuffer));
			assert(bufferDesc.stride % 4 == 0);
			assert(m_Description.buffer.offset % 4 == 0);
			assert(m_Description.buffer.size % 4 == 0);

			bufferInfo.address = ((IBuffer*)m_Resource)->getGpuAddress() + m_Description.buffer.offset;
			bufferInfo.range = m_Description.buffer.size;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorInfo.data.pStorageBuffer = &bufferInfo;

			descriptorSize = descriptorBufferProperties.robustStorageBufferDescriptorSize;
			break;
		}
		default:
			break;
		}

		void* pDescriptor = nullptr;
		m_HeapIndex = ((VulkanDevice*)m_Device)->allocateResourceDescriptor(&pDescriptor);

		vkGetDescriptorEXT(device, &descriptorInfo, descriptorSize, pDescriptor);

		return true;
	}

	VulkanConstantBufferDescriptor::VulkanConstantBufferDescriptor(VulkanDevice* device, IBuffer* buffer,
		const ConstantBufferDescriptorDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Buffer = buffer;
		m_Description = desc;
	}

	VulkanConstantBufferDescriptor::~VulkanConstantBufferDescriptor() {
		((VulkanDevice*)m_Device)->freeResourceDescriptor(m_HeapIndex);
	}
	bool VulkanConstantBufferDescriptor::create() {
		VkDescriptorAddressInfoEXT bufferInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		bufferInfo.address = m_Buffer->getGpuAddress() + m_Description.offset;
		bufferInfo.range = m_Description.size;

		VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		descriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorInfo.data.pUniformBuffer = &bufferInfo;

		void* pDescriptor = nullptr;
		m_HeapIndex = ((VulkanDevice*)m_Device)->allocateResourceDescriptor(&pDescriptor);

		VkDevice device = (VkDevice)m_Device->getHandle();
		size_t size = ((VulkanDevice*)m_Device)->getDescriptorBufferProperties().robustUniformBufferDescriptorSize;
		vkGetDescriptorEXT(device, &descriptorInfo, size, pDescriptor);

		return true;
	}

	VulkanSamplerDescriptor::VulkanSamplerDescriptor(VulkanDevice* device, const SamplerDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Description = desc;
	}

	VulkanSamplerDescriptor::~VulkanSamplerDescriptor() {
		VulkanDevice* device = (VulkanDevice*)m_Device;
		device->enqueueDeletion(m_VkSampler);
		device->freeSamplerDescriptor(m_HeapIndex);
	}

	bool VulkanSamplerDescriptor::create() {
		VkSamplerReductionModeCreateInfo reductionMode = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO };
		//reductionMode.reductionMode = ToVkSamplerReductionMode(m_Description.reductionMode);

		VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		createInfo.pNext = &reductionMode;
		createInfo.magFilter = toVkFilter(m_Description.filterMode);
		createInfo.minFilter = toVkFilter(m_Description.filterMode);
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.addressModeU = toVkSamplerAddressMode(m_Description.addressU);
		createInfo.addressModeV = toVkSamplerAddressMode(m_Description.addressV);
		createInfo.addressModeW = toVkSamplerAddressMode(m_Description.addressW);
		createInfo.mipLodBias = m_Description.mipLodBias;
		//createInfo.anisotropyEnable = false;
		//createInfo.maxAnisotropy = m_Description.maxAnisotropy;
		//createInfo.compareEnable = m_Description.reductionMode == SamplerReductionMode::Compare;
		//createInfo.compareOp = VkCompareOp(m_Description.compareFunc);
		createInfo.minLod = m_Description.minLod;
		createInfo.maxLod = m_Description.maxLod;

		//if (m_Description.borderColor[0] == 0.0f && m_Description.borderColor[1] == 0.0f &&
		//	m_Description.borderColor[2] == 0.0f && m_Description.borderColor[2] == 0.0f) {
		//	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		//}
		//else if (m_Description.borderColor[0] == 0.0f && m_Description.borderColor[1] == 0.0f &&
		//	m_Description.borderColor[2] == 0.0f && m_Description.borderColor[2] == 1.0f) {
		//	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		//}
		//else if (m_Description.borderColor[0] == 1.0f && m_Description.borderColor[1] == 1.0f &&
		//	m_Description.borderColor[2] == 1.0f && m_Description.borderColor[2] == 1.0f) {
		//	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		//}
		//else {
		//	assert(false); //unsupported border color
		//}

		VulkanDevice* device = (VulkanDevice*)m_Device;
		VkResult result = vkCreateSampler(device->getDevice(), &createInfo, nullptr, &m_VkSampler);
		if (result != VK_SUCCESS) {
			// Log error
			return false;
		}

		void* pDescriptor = nullptr;
		m_HeapIndex = device->allocateSamplerDescriptor(&pDescriptor);

		VkDescriptorGetInfoEXT descriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		descriptorInfo.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		descriptorInfo.data.pSampler = &m_VkSampler;

		size_t size = device->getDescriptorBufferProperties().samplerDescriptorSize;
		vkGetDescriptorEXT(device->getDevice(), &descriptorInfo, size, pDescriptor);

		return true;
	}
}