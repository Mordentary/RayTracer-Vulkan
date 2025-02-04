
#pragma once
#include "../RHI/rhi.hpp"

namespace SE
{
	class IndexBuffer
	{
	public:
		IndexBuffer(const std::string& name);
		bool create(uint32_t stride, uint32_t indexCount, rhi::MemoryType memoryType);

		rhi::IBuffer* getBuffer() const { return m_Buffer.get(); }
		uint32_t getIndexCount() const { return m_IndexCount; }
		rhi::Format getFormat() const { return m_Buffer->getDescription().format; }

	private:
		std::string m_DebugName;
		Scoped<rhi::IBuffer> m_Buffer = nullptr;
		uint32_t m_IndexCount = 0;
	};
}
