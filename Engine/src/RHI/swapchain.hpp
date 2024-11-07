#pragma once
#include "resource.hpp"
#include "texture.hpp"

namespace rhi {
	struct SwapchainDescription {
		void* windowHandle = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bufferCount = 2;
		Format format = Format::Unknown;
		bool vsync = true;
	};

	class Swapchain : public Resource {
	public:
		virtual void present() = 0;
		virtual Texture* getCurrentBackBuffer() = 0;
		virtual void resize(uint32_t width, uint32_t height) = 0;
		virtual void setVSync(bool enabled) = 0;
	};
} // namespace rhi