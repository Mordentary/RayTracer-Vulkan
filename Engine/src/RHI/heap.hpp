#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi
{
	class IHeap : public IResource
	{
	public:
		const HeapDescription& getDescription() const { return m_Description; }

	protected:
		HeapDescription m_Description = {};
	};
}