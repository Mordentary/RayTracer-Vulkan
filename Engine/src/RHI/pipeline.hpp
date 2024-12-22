#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi
{
	class IPipelineState : public IResource
	{
	public:
		PipelineType getType() const { return m_Type; }
		virtual bool create() = 0;
	protected:
		PipelineType m_Type;
	};
}