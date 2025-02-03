#pragma once
#include "RHI/rhi.hpp"
#include "engine_core.h"
#include "offsetAllocator/offsetAllocator.hpp"
#include "renderer/resources/raw_buffer.hpp"
#include <gpu_scene.hlsli>

namespace SE
{
	class Renderer;
	class GpuScene
	{
	public:
		GpuScene(Renderer* renderer);
		~GpuScene();

		OffsetAllocator::Allocation allocateStaticBuffer(uint32_t size);
		void freeStaticBuffer(OffsetAllocator::Allocation alloc);

		uint32_t allocateConstantBuffer(uint32_t size);

		uint32_t addInstance(const InstanceData& data);
		uint32_t getInstanceCount() const { return (uint32_t)m_InstanceData.size(); }
		void update();

		rhi::IBuffer* getSceneStaticBuffer() const { return m_pSceneStaticBuffer->getBuffer(); }
		rhi::IDescriptor* getSceneStaticBufferSRV() const { return m_pSceneStaticBuffer->getSRV(); }

		rhi::IBuffer* getSceneConstantBuffer() const;
		rhi::IDescriptor* getSceneConstantSRV() const;

		uint32_t getInstanceAddress() const { return m_InstanceDataAddress; }

		void resetFrameData();
	private:
		Renderer* m_pRenderer = nullptr;

		std::vector<InstanceData> m_InstanceData;
		uint32_t m_InstanceDataAddress = 0;

		Scoped<RawBuffer> m_pSceneStaticBuffer;
		Scoped<OffsetAllocator::Allocator> m_pSceneStaticBufferAllocator;

		Scoped<RawBuffer> m_pConstantBuffer[SE::SE_MAX_FRAMES_IN_FLIGHT];
		uint32_t m_ConstantBufferOffset = 0;
	};
}