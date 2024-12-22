#pragma once
#include "rhi.hpp"
#include"vulkan\vulkan_device.hpp"

namespace rhi
{
	SE::Scoped<IDevice> createDevice(const DeviceDescription& desc)
	{
		switch (desc.backend)
		{
		case RenderBackend::D3D12:
			return nullptr; // Currently not implemented
		case RenderBackend::Vulkan:
			return SE::CreateScoped<vulkan::VulkanDevice>(desc);
		default:
			return nullptr; // Unsupported backend
		}
	}
}