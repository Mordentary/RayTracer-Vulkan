#include "vk_pipelines.h"

namespace SE {
    PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
        m_Config.shaderStages.clear();
        m_Config.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
        m_Config.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
        m_Config.inputAssembly.topology = topology;
        m_Config.inputAssembly.primitiveRestartEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
        m_Config.rasterizer.polygonMode = mode;
        m_Config.rasterizer.lineWidth = 1.0f;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
        m_Config.rasterizer.cullMode = cullMode;
        m_Config.rasterizer.frontFace = frontFace;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setMultisamplingNone() {
        m_Config.multisampling.sampleShadingEnable = VK_FALSE;
        m_Config.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_Config.multisampling.minSampleShading = 1.0f;
        m_Config.multisampling.pSampleMask = nullptr;
        m_Config.multisampling.alphaToCoverageEnable = VK_FALSE;
        m_Config.multisampling.alphaToOneEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::disableBlending() {
        m_Config.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_Config.colorBlendAttachment.blendEnable = VK_FALSE;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::enableBlendingAdditive() {
        m_Config.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_Config.colorBlendAttachment.blendEnable = VK_TRUE;
        m_Config.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_Config.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        m_Config.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_Config.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_Config.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_Config.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::enableBlendingAlphablend() {
        m_Config.colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_Config.colorBlendAttachment.blendEnable = VK_TRUE;
        m_Config.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_Config.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_Config.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_Config.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_Config.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_Config.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setColorAttachmentFormat(VkFormat format) {
        m_Config.colorAttachmentFormat = format;
        m_Config.renderInfo.colorAttachmentCount = 1;
        m_Config.renderInfo.pColorAttachmentFormats = &m_Config.colorAttachmentFormat.value();
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setDepthFormat(VkFormat format) {
        m_Config.depthAttachmentFormat = format;
        m_Config.renderInfo.depthAttachmentFormat = format;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::disableDepthTest() {
        m_Config.depthStencil.depthTestEnable = VK_FALSE;
        m_Config.depthStencil.depthWriteEnable = VK_FALSE;
        m_Config.depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
        m_Config.depthStencil.depthBoundsTestEnable = VK_FALSE;
        m_Config.depthStencil.stencilTestEnable = VK_FALSE;
        m_Config.depthStencil.front = {};
        m_Config.depthStencil.back = {};
        m_Config.depthStencil.minDepthBounds = 0.0f;
        m_Config.depthStencil.maxDepthBounds = 1.0f;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp op) {
        m_Config.depthStencil.depthTestEnable = VK_TRUE;
        m_Config.depthStencil.depthWriteEnable = depthWriteEnable;
        m_Config.depthStencil.depthCompareOp = op;
        m_Config.depthStencil.depthBoundsTestEnable = VK_FALSE;
        m_Config.depthStencil.stencilTestEnable = VK_FALSE;
        m_Config.depthStencil.front = {};
        m_Config.depthStencil.back = {};
        m_Config.depthStencil.minDepthBounds = 0.0f;
        m_Config.depthStencil.maxDepthBounds = 1.0f;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::setPipelineLayout(VkPipelineLayout layout) {
        m_Config.pipelineLayout = layout;
        return *this;
    }

    VkPipeline PipelineBuilder::build(VkDevice device) {
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &m_Config.colorBlendAttachment;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &m_Config.renderInfo;
        pipelineInfo.stageCount = static_cast<uint32_t>(m_Config.shaderStages.size());
        pipelineInfo.pStages = m_Config.shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &m_Config.inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &m_Config.rasterizer;
        pipelineInfo.pMultisampleState = &m_Config.multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDepthStencilState = &m_Config.depthStencil;
        pipelineInfo.layout = m_Config.pipelineLayout;

        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;
        pipelineInfo.pDynamicState = &dynamicState;

        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        return newPipeline;
    }

    void PipelineBuilder::reset() {
        m_Config = PipelineConfig{};
    }

    bool vkUtil::loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode = buffer.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }
}
