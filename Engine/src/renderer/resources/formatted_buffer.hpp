#pragma once
#include"RHI\rhi.hpp"
#include"engine_core.h"
#include<string>
namespace SE
{
	class FormattedBuffer
	{
	public:
		FormattedBuffer(const std::string& name);
		bool create(rhi::Format format, uint32_t elementsCount, rhi::MemoryType memType, bool uav);

		rhi::IBuffer* getBuffer() { return m_Buffer.get(); }
		rhi::IDescriptor* getUAV() { return m_UAV.get(); }
		rhi::IDescriptor* getSRV() { return m_SRV.get(); }
	private:
		std::string m_DebugName;
		Scoped<rhi::IBuffer> m_Buffer;
		Scoped<rhi::IDescriptor> m_UAV;
		Scoped<rhi::IDescriptor> m_SRV;
	};
}