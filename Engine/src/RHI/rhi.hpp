#pragma once

//all RHI headers
#include "types.hpp"
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "swapchain.hpp"
#include "descriptor.hpp"
#include "device.hpp"

namespace rhi {
	// Device creation function
	Device* createDevice(const DeviceDescription& desc);
} 