#pragma once
#include "vulkan_core.hpp"
#include "../shader.hpp"

namespace rhi::vulkan
{
	class VulkanDevice;
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(VulkanDevice* pDevice, const ShaderDescription& desc, const std::string& name);
		~VulkanShader();

		virtual void* getHandle() const override { return m_Shader; };
		virtual bool create(std::span<uint8_t> data) override;

	private:
		VkShaderModule m_Shader = VK_NULL_HANDLE;
	};
}