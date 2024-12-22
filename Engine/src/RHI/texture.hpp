#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi {
	// Resource description structures
	class ITexture : public IResource {
	public:
		virtual ~ITexture() = default;
		const TextureDescription& getDescription() const { return m_Description; };
	protected:
		TextureDescription m_Description{};
	};
}