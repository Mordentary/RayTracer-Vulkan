#pragma once
#include "vulkan_core.hpp"
#include "rhi/pipeline.hpp"

namespace rhi::vulkan {
	class VulkanDevice;

	class VulkanGraphicsPipeline : public Pipeline {
	public:
		VulkanGraphicsPipeline(VulkanDevice* device, const GraphicsPipelineDescription& desc, const std::string& name);
		~VulkanGraphicsPipeline();

		void* getHandle() const override { return m_Pipeline; }
		bool create() override;

	private:
		GraphicsPipelineDescription m_Description;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	};
	class VulkanComputePipeline : public Pipeline
	{
	public:
		VulkanComputePipeline(VulkanDevice* device, const ComputePipelineDescription& desc, const std::string& name);
		~VulkanComputePipeline();

		void* getHandle() const override { return m_Pipeline; }
		bool create() override;

	private:
		ComputePipelineDescription m_Description;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	};
}