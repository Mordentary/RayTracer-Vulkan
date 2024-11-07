#pragma once
#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include <cassert>

namespace rhi {
	namespace vulkan {
		namespace {
			VkBufferUsageFlags translateBufferUsage(BufferUsageFlags usage) {
				VkBufferUsageFlags result = 0;

				if ((usage & BufferUsageFlags::VertexBuffer) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				if ((usage & BufferUsageFlags::IndexBuffer) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				if ((usage & BufferUsageFlags::ConstantBuffer) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

				if ((usage & BufferUsageFlags::UnorderedAccess) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

				if ((usage & BufferUsageFlags::TransferSrc) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				if ((usage & BufferUsageFlags::TransferDst) != BufferUsageFlags::None)
					result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

				// Always add these for utility
				result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

				return result;
			}

			VmaMemoryUsage translateMemoryType(MemoryType type) {
				switch (type) {
				case MemoryType::GpuOnly:
					return VMA_MEMORY_USAGE_GPU_ONLY;
				case MemoryType::CpuToGpu:
					return VMA_MEMORY_USAGE_CPU_TO_GPU;
				case MemoryType::GpuToCpu:
					return VMA_MEMORY_USAGE_GPU_TO_CPU;
				default:
					assert(false && "Invalid memory type");
					return VMA_MEMORY_USAGE_UNKNOWN;
				}
			}
		}

		VulkanBuffer::VulkanBuffer(VulkanDevice* device, const BufferDescription& desc, const void* initialData)
			: m_VulkanDevice(device)
			, m_Desc(desc) {
			init(initialData);
		}

		VulkanBuffer::~VulkanBuffer() {
			cleanup();
		}

		bool VulkanBuffer::init(const void* initialData) {
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = m_Desc.size;
			bufferInfo.usage = translateBufferUsage(m_Desc.usage);
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = translateMemoryType(m_Desc.memoryType);

			if (m_Desc.mapped) {
				allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}

			if (vmaCreateBuffer(m_VulkanDevice->getAllocator(), &bufferInfo, &allocInfo,
				&m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) {
				return false;
			}

			// If initial data is provided, map and copy it
			if (initialData) {
				void* mappedData = map();
				if (mappedData) {
					memcpy(mappedData, initialData, m_Desc.size);
					if (!m_Desc.mapped) {
						unmap();
					}
				}
			}

			return true;
		}

		void VulkanBuffer::cleanup() {
			if (m_Buffer) {
				vmaDestroyBuffer(m_VulkanDevice->getAllocator(), m_Buffer, m_Allocation);
				m_Buffer = VK_NULL_HANDLE;
				m_Allocation = VK_NULL_HANDLE;
			}
		}

		void* VulkanBuffer::map() {
			if (m_Mapped) {
				return m_MappedData;
			}

			if (vmaMapMemory(m_VulkanDevice->getAllocator(), m_Allocation, &m_MappedData) != VK_SUCCESS) {
				return nullptr;
			}

			m_Mapped = true;
			return m_MappedData;
		}

		void VulkanBuffer::unmap() {
			if (!m_Mapped || m_Desc.mapped) {
				return;
			}

			vmaUnmapMemory(m_VulkanDevice->getAllocator(), m_Allocation);
			m_MappedData = nullptr;
			m_Mapped = false;
		}

		uint64_t VulkanBuffer::getGpuAddress() const {
			VkBufferDeviceAddressInfo addressInfo = {};
			addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			addressInfo.buffer = m_Buffer;

			return vkGetBufferDeviceAddress(m_VulkanDevice->getDevice(), &addressInfo);
		}
	} 
} 