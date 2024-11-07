#pragma once
#include "resource.hpp"
#include "buffer.hpp"
#include "texture.hpp"

namespace rhi {
	struct DescriptorDesc {
		uint32_t maxSets = 1000;
		uint32_t maxBuffers = 1000;
		uint32_t maxTextures = 1000;
		uint32_t maxSamplers = 100;
	};

	class DescriptorSet : public Resource {
	public:
		virtual void bindBuffer(uint32_t binding, Buffer* buffer, uint64_t offset = 0, uint64_t range = UINT64_MAX) = 0;
		virtual void bindTexture(uint32_t binding, Texture* texture) = 0;
	};
}