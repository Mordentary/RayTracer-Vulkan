#pragma once
#include "vulkan_core.hpp"
#include "../shader.hpp"

namespace rhi::vulkan
{
	class VulkanDevice;
	class VulkanShader : public IShader
	{
	public:
		VulkanShader(VulkanDevice* pDevice, const ShaderDescription& desc, const std::string& name);
		~VulkanShader();

		virtual void* getHandle() const override { return m_Shader; };
		virtual bool create(std::span<std::byte> data) override;

	private:
		VkShaderModule m_Shader = VK_NULL_HANDLE;
	};
}