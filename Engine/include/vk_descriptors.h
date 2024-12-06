#pragma once
#include "vk_types.h"
#include <vector>
#include <deque>
#include <span>

namespace SE
{
	struct DescriptorLayoutBuilder {
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
		void addBinding(uint32_t binding, VkDescriptorType type);
		void clear();
		VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	struct DescriptorWriter {
		std::deque<VkDescriptorImageInfo> m_ImageInfos;
		std::deque<VkDescriptorBufferInfo> m_BufferInfos;
		std::vector<VkWriteDescriptorSet> m_Writes;
		void writeImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
		void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
		void clear();
		void updateSet(VkDevice device, VkDescriptorSet set);
	};

	struct DescriptorAllocator {
	public:
		struct PoolSizeRatio {
			VkDescriptorType type;
			float ratio;
		};
		void create(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
		void clearPools(VkDevice device);
		void destroyPools(VkDevice device);
		VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);

	private:
		VkDescriptorPool getPool(VkDevice device);
		VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
		std::vector<PoolSizeRatio> m_Ratios;
		std::vector<VkDescriptorPool> m_FullPools;
		std::vector<VkDescriptorPool> m_ReadyPools;
		uint32_t m_SetsPerPool;
	};
}
