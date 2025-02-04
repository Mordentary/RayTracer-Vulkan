#include "index_buffer.hpp"
#include "../renderer.hpp"
#include "core/engine.hpp"

namespace SE
{
	IndexBuffer::IndexBuffer(const std::string& name)
	{
		m_DebugName = name;
	}

	bool IndexBuffer::create(uint32_t stride, uint32_t index_count, rhi::MemoryType memoryType)
	{
		SE_ASSERT(stride == 2 || stride == 4);
		m_IndexCount = index_count;

		Renderer* pRenderer = &Engine::getInstance().getRenderer();
		rhi::IDevice* device = pRenderer->getDevice();

		rhi::BufferDescription desc;
		desc.stride = stride;
		desc.size = stride * index_count;
		desc.format = stride == 2 ? rhi::Format::R16_UINT: rhi::Format::R32_UINT;
		desc.memoryType = memoryType;

		m_Buffer.reset(device->createBuffer(desc, m_DebugName));
		if (m_Buffer == nullptr)
		{
			return false;
		}

		return true;
	}
}