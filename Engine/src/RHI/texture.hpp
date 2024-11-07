#pragma once
#include "resource.hpp"
#include "RHI\types.hpp"

namespace rhi {
	struct TextureDescription {
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arraySize = 1;
		Format format = Format::Unknown;
		bool allowUnorderedAccess = false;
		bool allowRenderTarget = false;
		bool allowDepthStencil = false;
	};

	class Texture : public Resource {
	public:
		virtual const TextureDescription& getDesc() const = 0;
	};
} // namespace rhi