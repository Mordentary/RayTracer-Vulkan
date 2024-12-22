#include "staging_buffer_allocator.hpp"
#include "renderer.hpp"
#include "../RHI/types.hpp"

#define BUFFER_SIZE (64 * 1024 * 1024)

namespace SE
{
	StagingBuffer StagingBufferAllocator::allocate(uint32_t size)
	{
		SE_ASSERT_NOMSG(size <= BUFFER_SIZE);

		if (m_pBuffers.empty())
		{
			createNewBuffer();
		}

		if (m_AllocatedSize + size > BUFFER_SIZE)
		{
			createNewBuffer();
			m_CurrentBuffer++;
			m_AllocatedSize = 0;
		}

		StagingBuffer buffer;
		buffer.buffer = m_pBuffers[m_CurrentBuffer].get();
		buffer.size = size;
		buffer.offset = m_AllocatedSize;

		m_AllocatedSize += rhi::alignToPowerOfTwo<uint32_t>(size, 256);
		m_LastAllocatedFrame = m_Renderer->getFrameID();

		return buffer;
	}

	void StagingBufferAllocator::reset()
	{
		m_CurrentBuffer = 0;
		m_AllocatedSize = 0;

		if (!m_pBuffers.empty())
		{
			if (m_Renderer->getFrameID() - m_LastAllocatedFrame > 100)
			{
				m_pBuffers.clear();
			}
		}
	}

	void StagingBufferAllocator::createNewBuffer()
	{
		rhi::BufferDescription desc;
		desc.size = BUFFER_SIZE;
		desc.memoryType = rhi::MemoryType::CpuOnly;
		rhi::IBuffer* buffer = m_Renderer->getDevice()->createBuffer(desc, "StagingBufferAllocator::m_pBuffer");
		m_pBuffers.push_back(Scoped<rhi::IBuffer>(buffer));
	}
}