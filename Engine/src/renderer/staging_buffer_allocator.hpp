#pragma once
#include "../rhi/rhi.hpp"
#include "engine_core.h"
namespace SE
{
	class Renderer;

	struct StagingBuffer
	{
		rhi::IBuffer* buffer;
		uint32_t size;
		uint32_t offset;
	};

	class StagingBufferAllocator
	{
	public:
		StagingBufferAllocator(Renderer* pRenderer) : m_Renderer(pRenderer) {};
		StagingBuffer allocate(uint32_t size);
		void reset();
	private:
		void createNewBuffer();

	private:
		Renderer* m_Renderer = nullptr;
		std::vector<Scoped<rhi::IBuffer>> m_pBuffers;
		uint32_t m_CurrentBuffer = 0;
		uint32_t m_AllocatedSize = 0;
		uint64_t m_LastAllocatedFrame = 0;
	};
}