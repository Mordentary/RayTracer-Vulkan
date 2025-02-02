#pragma once
#include "float.h"
#include "hlslpp\hlsl++.h"
#include <core\logger.hpp>
using namespace hlslpp;
namespace SE
{
	template<typename T>
	constexpr T alignToPowerOfTwo(T value, T alignment) {
		SE_ASSERT_NOMSG((alignment & (alignment - 1)) == 0); // Verify power of 2
		return (value + (alignment - 1)) & ~(alignment - 1);
	}
}