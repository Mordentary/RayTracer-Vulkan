#pragma once
#include "resource.hpp"

namespace rhi {
	class Descriptor : public Resource
	{
	public:
		virtual uint32_t getDescriptorArrayIndex() const = 0;
	};
}