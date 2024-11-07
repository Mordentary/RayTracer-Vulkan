#pragma once 
#include "rhi.hpp"
#include"vulkan\vulkan_device.hpp"

#define SE_PLATFORM_WINDOWS
namespace rhi
{
	Device* createDevice(const DeviceDescription& desc)
	{
		Device* pDevice = nullptr;

		switch (desc.backend)
		{
		case RenderBackend::D3D12:
			//pDevice = 
			//break;
		case RenderBackend::Vulkan:
			pDevice = new vulkan::VulkanDevice(desc);
			break;


		return pDevice;
		}
	
		SE_ASSERT_MSG(pDevice, "Device creation failure!");
	}
}