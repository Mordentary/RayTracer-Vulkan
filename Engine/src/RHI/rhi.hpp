#pragma once

//all RHI headers
#include "types.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "swapchain.hpp"
#include "descriptor.hpp"
#include "device.hpp"
#include "heap.hpp"
#include "fence.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "command_list.hpp"
#include <engine_core.h>

namespace rhi {
	// Device creation function
	SE::Scoped<IDevice> createDevice(const DeviceDescription& desc);

	inline bool isStencilFormat(Format format)
	{
		switch (format) {
		case Format::D24_UNORM_S8_UINT:
		case Format::D32_SFLOAT_S8_UINT:
		case Format::S8_UINT:
			return true;
		default:
			return false;
		}
	}

	inline bool isDepthFormat(Format format)
	{
		switch (format) {
		case Format::D24_UNORM_S8_UINT:
		case Format::D32_SFLOAT_S8_UINT:
		case Format::D32_SFLOAT:
		case Format::D16_UNORM:
			return true;
		default:
			return false;
		}
	}

	uint32_t calcSubresource(const TextureDescription& desc, uint32_t mip, uint32_t slice);
	void decomposeSubresource(const TextureDescription& desc, uint32_t subresource, uint32_t& mip, uint32_t& slice);
}