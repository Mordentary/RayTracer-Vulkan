#pragma once
#include"memory.hpp"
#include"math.hpp"
#include <core\logger.hpp>
namespace SE
{
	class LinearAllocator
	{
	public:
		LinearAllocator(uint32_t chunkSize)
			: m_MemoryBlockSize(chunkSize)
		{
			m_BaseMemPtr = SE_ALLOC(chunkSize);
			m_MemoryBlockSize = chunkSize;
		}

		~LinearAllocator()
		{
			SE_FREE(m_BaseMemPtr);
		}

		inline void reset()
		{
			m_PointerOffset = 0;
		}

		void* allocate(uint32_t allocSize, uint32_t alignment = 1)
		{
			uint32_t alignedOffset = alignToPowerOfTwo(m_PointerOffset, alignment);
			SE_ASSERT(alignedOffset + allocSize <= m_MemoryBlockSize);

			m_PointerOffset = alignedOffset + allocSize;

			return (char*)m_BaseMemPtr + alignedOffset;
		}

	private:
		void* m_BaseMemPtr;
		uint32_t m_PointerOffset = 0;
		uint32_t m_MemoryBlockSize = 0;
	};
}