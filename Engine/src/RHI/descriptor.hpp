#pragma once
#include "resource.hpp"

namespace rhi {
	class IDescriptor : public IResource
	{
	public:
		virtual uint32_t getDescriptorArrayIndex() const = 0;
	};
}