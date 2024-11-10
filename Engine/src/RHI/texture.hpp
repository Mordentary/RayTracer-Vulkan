#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi {
	// Resource description structures
	struct TextureDescription {
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arraySize = 1;
		Format format = Format::Unknown;
		TextureType type = TextureType::Texture2D;
		TextureUsageFlags usage = TextureUsageFlags::None;
		MemoryType memoryType = MemoryType::GpuOnly;
	};

	class Texture : public Resource {
	public:
		virtual ~Texture() = default;
		const TextureDescription& getDescription() const { return m_Description; };
	protected:
		TextureDescription m_Description{};
	};
}