#include "vulkan_pipeline.hpp"
#include "vulkan_device.hpp"
#include "vulkan_shader.hpp"

namespace rhi::vulkan {
	VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice* device, const GraphicsPipelineDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Description = desc;
		m_Type = PipelineType::Graphics;
	}

	VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
		((VulkanDevice*)(m_Device))->enqueueDeletion(m_Pipeline);
	}

	bool VulkanGraphicsPipeline::create() {
		if (m_Pipeline)
			((VulkanDevice*)(m_Device))->enqueueDeletion(m_Pipeline);

		VkPipelineShaderStageCreateInfo stages[2];
		stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = static_cast<VkShaderModule>(m_Description.vertexShader->getHandle());
		stages[0].pName = m_Description.vertexShader->getDescription().entryPoint.c_str();

		if (m_Description.pixelShader) {
			stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stages[1].module = static_cast<VkShaderModule>(m_Description.pixelShader->getHandle());
			stages[1].pName = m_Description.pixelShader->getDescription().entryPoint.c_str();
		}

	   VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInput.vertexBindingDescriptionCount = 0;         // No vertex bindings
		vertexInput.pVertexBindingDescriptions = nullptr;
		vertexInput.vertexAttributeDescriptionCount = 0;     // No vertex attributes
		vertexInput.pVertexAttributeDescriptions = nullptr;
	
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssembly.topology = toVkPrimitiveTopology(m_Description.primitiveType);

		VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		constexpr VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicStateInfo.dynamicStateCount = std::size(dynamicStates);
		dynamicStateInfo.pDynamicStates = dynamicStates;

		VkPipelineViewportStateCreateInfo viewportInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportInfo.viewportCount = 1;
		viewportInfo.scissorCount = 1;

		auto rasterState = toVkPipelineRasterizationStateCreateInfo(m_Description.rasterizer);
		auto depthStencilState = toVkPipelineDepthStencilStateCreateInfo(m_Description.depthStencil);

		VkPipelineColorBlendAttachmentState blendStates[8];
		auto colorBlendState = toVkPipelineColorBlendStateCreateInfo(m_Description.blend, blendStates);

		VkFormat colorFormats[8];
		auto formatInfo = toVkPipelineRenderingCreateInfo(m_Description, colorFormats);

		VkGraphicsPipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		createInfo.pNext = &formatInfo;
		createInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		createInfo.stageCount = m_Description.pixelShader ? 2 : 1;
		createInfo.pStages = stages;
		createInfo.pVertexInputState = &vertexInput;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pMultisampleState = &multisampleState;
		createInfo.pViewportState = &viewportInfo;
		createInfo.pRasterizationState = &rasterState;
		createInfo.pDepthStencilState = &depthStencilState;
		createInfo.pColorBlendState = &colorBlendState;
		createInfo.pDynamicState = &dynamicStateInfo;
		createInfo.layout = ((VulkanDevice*)m_Device)->getPipelineLayout();

		VK_CHECK_RETURN(
			vkCreateGraphicsPipelines((VkDevice)m_Device->getHandle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline),
			false,
			"Failed to create graphics pipeline: {}", m_DebugName);
		setDebugName((VkDevice)m_Device->getHandle(), VK_OBJECT_TYPE_PIPELINE, m_Pipeline, m_DebugName.c_str());
		return true;
	}

	VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* device, const ComputePipelineDescription& desc, const std::string& name) {
		m_Device = device;
		m_DebugName = name;
		m_Description = desc;
		m_Type = PipelineType::Compute;
	}

	VulkanComputePipeline::~VulkanComputePipeline() {
		((VulkanDevice*)(m_Device))->enqueueDeletion(m_Pipeline);
	}

	bool VulkanComputePipeline::create() {
		auto device = ((VulkanDevice*)(m_Device));
		if (m_Pipeline)
			device->enqueueDeletion(m_Pipeline);

		VkComputePipelineCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		createInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		createInfo.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfo.stage.module = static_cast<VkShaderModule>(m_Description.computeShader->getHandle());
		createInfo.stage.pName = m_Description.computeShader->getDescription().entryPoint.c_str();
		createInfo.layout = device->getPipelineLayout();

		VkResult result = vkCreateComputePipelines((VkDevice)m_Device->getHandle(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_Pipeline);
		VK_CHECK_RETURN(result, false, "Failed to create compute pipeline: {}", m_DebugName);

		setDebugName((VkDevice)m_Device->getHandle(), VK_OBJECT_TYPE_PIPELINE, m_Pipeline, m_DebugName.c_str());
		return true;
	}
}