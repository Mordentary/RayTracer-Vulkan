#pragma once
#include <vk_types.h>
#include <vector>
#include <optional>

namespace SE
{
	class PipelineBuilder {
	public:
		PipelineBuilder& setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
		PipelineBuilder& setPolygonMode(VkPolygonMode mode);
		PipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		PipelineBuilder& setMultisamplingNone();
		PipelineBuilder& disableBlending();
		PipelineBuilder& enableBlendingAdditive();
		PipelineBuilder& enableBlendingAlphablend();
		PipelineBuilder& setColorAttachmentFormat(VkFormat format);
		PipelineBuilder& setDepthFormat(VkFormat format);
		PipelineBuilder& disableDepthTest();
		PipelineBuilder& enableDepthTest(bool depthWriteEnable, VkCompareOp op);
		PipelineBuilder& setPipelineLayout(VkPipelineLayout layout);

		VkPipeline build(VkDevice device);
		void reset();

	private:
		struct PipelineConfig {
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
			VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
			VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
			VkPipelineRenderingCreateInfo renderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
			std::optional<VkFormat> colorAttachmentFormat;
			std::optional<VkFormat> depthAttachmentFormat;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		};

		PipelineConfig m_Config;
	};

	namespace vkUtil {
		bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
	}
}
