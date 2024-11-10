#pragma once
#include "resource.hpp"
#include "texture.hpp"

namespace rhi {
	struct SwapchainDescription {
		void* windowHandle = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bufferCount = 3;
		Format format = Format::Unknown;
		bool vsync = true;
	};

	class Swapchain : public Resource {
	public:
		virtual Texture* getCurrentSwapchainImage() = 0;
		virtual bool acquireNextImage() = 0;
		virtual bool resize(uint32_t width, uint32_t height) = 0;
		virtual void setVSync(bool enabled) = 0;

		const SwapchainDescription& getDescription() const { return m_Description; }

	protected:
		SwapchainDescription m_Description{};
	};
}