#include "vulkan_shader.hpp"
#include "vulkan_device.hpp"
//#include "xxHash/xxhash.h"
namespace rhi::vulkan
{
	VulkanShader::VulkanShader(VulkanDevice* pDevice, const ShaderDescription& desc, const std::string& name)
	{
		m_Device = pDevice;
		m_Description = desc;
		m_DebugName = name;
	}

	VulkanShader::~VulkanShader()
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Shader);
	}

	bool VulkanShader::create(std::span<std::byte> data)
	{
		((VulkanDevice*)m_Device)->enqueueDeletion(m_Shader);

		VkDevice device = ((VulkanDevice*)m_Device)->getDevice();

		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = data.size();
		createInfo.pCode = (const uint32_t*)data.data();

		VK_CHECK_RETURN(vkCreateShaderModule(device, &createInfo, nullptr, &m_Shader), false, "Shader creation failed!{}", m_DebugName);

		setDebugName(device, VK_OBJECT_TYPE_SHADER_MODULE, m_Shader, m_DebugName.c_str());

		m_Hash = XXH3_64bits(data.data(), data.size());

		return true;
	}
}