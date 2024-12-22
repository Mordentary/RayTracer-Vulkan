#pragma once
#include "vulkan_core.hpp"
#include "rhi/pipeline.hpp"

namespace rhi::vulkan {
	class VulkanDevice;

	class VulkanGraphicsPipelineState : public IPipelineState {
	public:
		VulkanGraphicsPipelineState(VulkanDevice* device, const GraphicsPipelineDescription& desc, const std::string& name);
		~VulkanGraphicsPipelineState();

		void* getHandle() const override { return m_Pipeline; }
		bool create() override;

	private:
		GraphicsPipelineDescription m_Description;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	};
	class VulkanComputePipelineState : public IPipelineState
	{
	public:
		VulkanComputePipelineState(VulkanDevice* device, const ComputePipelineDescription& desc, const std::string& name);
		~VulkanComputePipelineState();

		void* getHandle() const override { return m_Pipeline; }
		bool create() override;

	private:
		ComputePipelineDescription m_Description;
		VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	};
}