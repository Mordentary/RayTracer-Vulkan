#pragma once
#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include <cassert>

namespace rhi::vulkan {

	VulkanBuffer::VulkanBuffer(VulkanDevice* device, const BufferDescription& desc, const std::string& name)
	{
		m_Device = device;
		m_Description = desc;
		m_DebugName = name;
	}

	VulkanBuffer::~VulkanBuffer()
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Buffer);
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Allocation);
	}

	bool VulkanBuffer::create() {
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_Description.size;
		bufferInfo.usage = translateBufferUsage(m_Description.usage);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = translateMemoryType(m_Description.memoryType);

		if (m_Description.mapped) {
			allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		if (vmaCreateBuffer(((VulkanDevice*)m_Device)->getAllocator(), &bufferInfo, &allocInfo,
			&m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
			return false;
		}

		return true;
	}

	void* VulkanBuffer::map() {
		if (m_Mapped) {
			return m_MappedData;
		}

		if (vmaMapMemory(((VulkanDevice*)m_Device)->getAllocator(), m_Allocation, &m_MappedData) != VK_SUCCESS) {
			return nullptr;
		}

		m_Mapped = true;
		return m_MappedData;
	}

	void VulkanBuffer::unmap() {
		if (!m_Mapped || m_Description.mapped) {
			return;
		}

		vmaUnmapMemory(((VulkanDevice*)m_Device)->getAllocator(), m_Allocation);
		m_MappedData = nullptr;
		m_Mapped = false;
	}

	uint64_t VulkanBuffer::getGpuAddress() const {
		VkBufferDeviceAddressInfo addressInfo = {};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.buffer = m_Buffer;

		return vkGetBufferDeviceAddress(((VulkanDevice*)m_Device)->getDevice(), &addressInfo);
	}
	void* VulkanBuffer::getCpuAddress() 
	{
		return m_MappedData;
	}
}