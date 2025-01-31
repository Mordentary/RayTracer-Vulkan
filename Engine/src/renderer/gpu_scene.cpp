#include "gpu_scene.hpp"
#include "renderer.hpp"
#include "utils/math.hpp"

#define MAX_CONSTANT_BUFFER_SIZE (8 * 1024 * 1024)
#define ALLOCATION_ALIGNMENT (4)
namespace SE
{
	GpuScene::GpuScene(Renderer* renderer) : m_pRenderer(renderer)
	{
		rhi::BufferDescription bufferDesc;
		uint32_t staticBuffSize = MB(200);
		m_pSceneStaticBuffer.reset(m_pRenderer->createRawBuffer(nullptr, staticBuffSize, "GPU_SCENE::StaticBuffer", rhi::MemoryType::GpuOnly));
		m_pSceneStaticBufferAllocator = createScoped<OffsetAllocator::Allocator>(staticBuffSize);

		for (int i = 0; i < SE_MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_pConstantBuffer[i].reset(renderer->createRawBuffer(nullptr, MAX_CONSTANT_BUFFER_SIZE, "GpuScene::ConstantBuffer", rhi::MemoryType::CpuToGpu));
		}
	}
	GpuScene::~GpuScene()
	{
	}
	OffsetAllocator::Allocation GpuScene::allocateStaticBuffer(uint32_t size)
	{
		return m_pSceneStaticBufferAllocator->allocate(alignToPowerOfTwo<uint32_t>(size, ALLOCATION_ALIGNMENT));
	}
	void GpuScene::freeStaticBuffer(OffsetAllocator::Allocation alloc)
	{
		if (alloc.offset >= m_pSceneStaticBuffer->getBuffer()->getDescription().size)
		{
			return;
		}
		m_pSceneStaticBufferAllocator->free(alloc);
	}

	uint32_t GpuScene::allocateConstantBuffer(uint32_t size)
	{
		SE_ASSERT(m_ConstantBufferOffset + size <= MAX_CONSTANT_BUFFER_SIZE);

		uint32_t address = m_ConstantBufferOffset;
		m_ConstantBufferOffset += alignToPowerOfTwo<uint32_t>(size, ALLOCATION_ALIGNMENT);

		return address;
	}
	rhi::IBuffer* GpuScene::getSceneConstantBuffer() const
	{
		uint32_t frame_index = m_pRenderer->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		return m_pConstantBuffer[frame_index]->getBuffer();
	}
	rhi::IDescriptor* GpuScene::getSceneConstantSRV() const
	{
		uint32_t frame_index = m_pRenderer->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		return m_pConstantBuffer[frame_index]->getSRV();
	}
	void GpuScene::resetFrameData()
	{
		m_ConstantBufferOffset = 0;
	}
}