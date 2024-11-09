#pragma once

//all RHI headers
#include "types.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "swapchain.hpp"
#include "descriptor.hpp"
#include "device.hpp"
#include "fence.hpp"
#include "command_list.hpp"

namespace rhi {
	// Device creation function
	SE::Scoped<Device> createDevice(const DeviceDescription& desc);
}