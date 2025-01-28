#pragma once
#include "rpmalloc/rpmalloc.h"
#define KB(x) ((x) * (1ULL << 10))
#define MB(x) ((x) * (1ULL << 20))
namespace SE
{
	static inline void* SE_ALLOC(size_t size)
	{
		return rpmalloc(size);
	}

	static inline void* SE_ALLOC(size_t size, size_t alignment)
	{
		return rpaligned_alloc(alignment, size);
	}

	static inline void* SE_REALLOC(void* ptr, size_t size)
	{
		return rprealloc(ptr, size);
	}

	static inline void SE_FREE(void* ptr)
	{
		rpfree(ptr);
	}
}