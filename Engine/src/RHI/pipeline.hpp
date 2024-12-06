#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi
{
	class Pipeline : public Resource
	{
	public:
		PipelineType getType() const { return m_Type; }
		virtual bool create() = 0;
	protected:
		PipelineType m_Type;
	};
}