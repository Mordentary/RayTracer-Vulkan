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
}