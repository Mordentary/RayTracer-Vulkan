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

	uint32_t getFormatRowPitch(Format format, uint32_t width) {
		switch (format) {
			// 8-bit formats
		case Format::R8_UNORM:
		case Format::R8_SNORM:
		case Format::R8_UINT:
		case Format::R8_SINT:
		case Format::R4G4_UNORM_PACK8:
			return width * 1;

			// 16-bit formats (2 bytes per pixel)
		case Format::R8G8_UNORM:
		case Format::R8G8_SNORM:
		case Format::R8G8_UINT:
		case Format::R8G8_SINT:
		case Format::R16_UNORM:
		case Format::R16_SNORM:
		case Format::R16_UINT:
		case Format::R16_SINT:
		case Format::R16_SFLOAT:
		case Format::R5G6B5_UNORM_PACK16:
		case Format::B5G6R5_UNORM_PACK16:
		case Format::R5G5B5A1_UNORM_PACK16:
		case Format::B5G5R5A1_UNORM_PACK16:
		case Format::A1R5G5B5_UNORM_PACK16:
		case Format::R4G4B4A4_UNORM_PACK16:
		case Format::B4G4R4A4_UNORM_PACK16:
		case Format::D16_UNORM:
			return width * 2;

			// 24-bit formats (3 bytes per pixel)
		case Format::R8G8B8_UNORM:
		case Format::R8G8B8_SNORM:
		case Format::R8G8B8_UINT:
		case Format::R8G8B8_SINT:
			return width * 3;

			// 32-bit formats (4 bytes per pixel)
		case Format::R8G8B8A8_UNORM:
		case Format::R8G8B8A8_SNORM:
		case Format::R8G8B8A8_UINT:
		case Format::R8G8B8A8_SINT:
		case Format::R8G8B8A8_SRGB:
		case Format::B8G8R8A8_UNORM:
		case Format::B8G8R8A8_SNORM:
		case Format::B8G8R8A8_UINT:
		case Format::B8G8R8A8_SINT:
		case Format::B8G8R8A8_SRGB:
		case Format::R16G16_UNORM:
		case Format::R16G16_SNORM:
		case Format::R16G16_UINT:
		case Format::R16G16_SINT:
		case Format::R16G16_SFLOAT:
		case Format::R32_UINT:
		case Format::R32_SINT:
		case Format::R32_SFLOAT:
		case Format::D24_UNORM_S8_UINT:
		case Format::D32_SFLOAT:
			return width * 4;

			// 48-bit formats (6 bytes per pixel)
		case Format::R16G16B16_UNORM:
		case Format::R16G16B16_SNORM:
		case Format::R16G16B16_UINT:
		case Format::R16G16B16_SINT:
		case Format::R16G16B16_SFLOAT:
			return width * 6;

			// 64-bit formats (8 bytes per pixel)
		case Format::R16G16B16A16_UNORM:
		case Format::R16G16B16A16_SNORM:
		case Format::R16G16B16A16_UINT:
		case Format::R16G16B16A16_SINT:
		case Format::R16G16B16A16_SFLOAT:
		case Format::R32G32_UINT:
		case Format::R32G32_SINT:
		case Format::R32G32_SFLOAT:
		case Format::D32_SFLOAT_S8_UINT:
			return width * 8;

			// 96-bit formats (12 bytes per pixel)
		case Format::R32G32B32_UINT:
		case Format::R32G32B32_SINT:
		case Format::R32G32B32_SFLOAT:
			return width * 12;

			// 128-bit formats (16 bytes per pixel)
		case Format::R32G32B32A32_UINT:
		case Format::R32G32B32A32_SINT:
		case Format::R32G32B32A32_SFLOAT:
			return width * 16;

			// 1-byte stencil
		case Format::S8_UINT:
			return width * 1;

			// Compressed/YUV formats (return 0 or handle differently)
		default:
			return 0; // Requires special handling for block compression
		}
	}
}