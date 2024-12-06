#include "vk_descriptors.h"
#include "vk_initializers.h"

namespace SE
{
	void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type)
	{
		VkDescriptorSetLayoutBinding newbind{};
		newbind.binding = binding;
		newbind.descriptorCount = 1;
		newbind.descriptorType = type;

		m_Bindings.push_back(newbind);
	}

	void DescriptorLayoutBuilder::clear()
	{
		m_Bindings.clear();
	}

	VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& b : m_Bindings) {
			b.stageFlags |= shaderStages;
		}

		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pNext = pNext;

		info.pBindings = m_Bindings.data();
		info.bindingCount = (uint32_t)m_Bindings.size();
		info.flags = flags;

		VkDescriptorSetLayout set;
		VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

		return set;
	}

	void DescriptorWriter::writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
	{
		VkDescriptorImageInfo& info = m_ImageInfos.emplace_back(VkDescriptorImageInfo{
			.sampler = sampler,
			.imageView = image,
			.imageLayout = layout
			});

		VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		write.dstBinding = binding;
		write.dstSet = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = &info;

		m_Writes.push_back(write);
	}

	void DescriptorWriter::writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
	{
		VkDescriptorBufferInfo& info = m_BufferInfos.emplace_back(VkDescriptorBufferInfo{
			.buffer = buffer,
			.offset = offset,
			.range = size
			});

		VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

		write.dstBinding = binding;
		write.dstSet = VK_NULL_HANDLE;
		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = &info;

		m_Writes.push_back(write);
	}

	void DescriptorWriter::clear()
	{
		m_ImageInfos.clear();
		m_Writes.clear();
		m_BufferInfos.clear();
	}

	void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set)
	{
		for (VkWriteDescriptorSet& write : m_Writes) {
			write.dstSet = set;
		}

		vkUpdateDescriptorSets(device, (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);
	}

	void DescriptorAllocator::create(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios)
	{
		m_Ratios.clear();

		for (auto r : poolRatios) {
			m_Ratios.push_back(r);
		}

		VkDescriptorPool newPool = createPool(device, initialSets, poolRatios);

		m_SetsPerPool = initialSets * 1.5;

		m_ReadyPools.push_back(newPool);
	}

	void DescriptorAllocator::clearPools(VkDevice device)
	{
		for (auto p : m_ReadyPools) {
			vkResetDescriptorPool(device, p, 0);
		}
		for (auto p : m_FullPools) {
			vkResetDescriptorPool(device, p, 0);
			m_ReadyPools.push_back(p);
		}
		m_FullPools.clear();
	}

	void DescriptorAllocator::destroyPools(VkDevice device)
	{
		for (auto p : m_ReadyPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		m_ReadyPools.clear();
		for (auto p : m_FullPools) {
			vkDestroyDescriptorPool(device, p, nullptr);
		}
		m_FullPools.clear();
	}

	VkDescriptorPool DescriptorAllocator::getPool(VkDevice device)
	{
		VkDescriptorPool newPool;
		if (m_ReadyPools.size() != 0) {
			newPool = m_ReadyPools.back();
			m_ReadyPools.pop_back();
		}
		else {
			newPool = createPool(device, m_SetsPerPool, m_Ratios);

			m_SetsPerPool = m_SetsPerPool * 1.5;
			if (m_SetsPerPool > 4092) {
				m_SetsPerPool = 4092;
			}
		}

		return newPool;
	}

	VkDescriptorPool DescriptorAllocator::createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio ratio : poolRatios) {
			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.type,
				.descriptorCount = uint32_t(ratio.ratio * setCount)
				});
		}

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = 0;
		pool_info.maxSets = setCount;
		pool_info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_info.pPoolSizes = poolSizes.data();

		VkDescriptorPool newPool;
		vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
		return newPool;
	}

	VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
	{
		VkDescriptorPool poolToUse = getPool(device);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = pNext;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = poolToUse;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet ds;
		VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
			m_FullPools.push_back(poolToUse);

			poolToUse = getPool(device);
			allocInfo.descriptorPool = poolToUse;

			VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
		}

		m_ReadyPools.push_back(poolToUse);
		return ds;
	}
}