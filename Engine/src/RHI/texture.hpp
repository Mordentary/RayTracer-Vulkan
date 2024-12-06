#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi {
	// Resource description structures
	class Texture : public Resource {
	public:
		virtual ~Texture() = default;
		const TextureDescription& getDescription() const { return m_Description; };
	protected:
		TextureDescription m_Description{};
	};
}