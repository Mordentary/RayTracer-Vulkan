#include "rhi.hpp"
#include "rhi.hpp"
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
			return SE::createScoped<vulkan::VulkanDevice>(desc);
		default:
			return nullptr; // Unsupported backend
		}
	}
	uint32_t calcSubresource(const TextureDescription& desc, uint32_t mip, uint32_t slice)
	{
		return mip + desc.mipLevels * slice;
	}
	void decomposeSubresource(const TextureDescription& desc, uint32_t subresource, uint32_t& mip, uint32_t& slice)
	{
		mip = subresource % desc.mipLevels;
		slice = (subresource / desc.mipLevels) % desc.arraySize;
	}
}