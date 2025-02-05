#pragma once
#include "float.h"
#include "hlslpp\hlsl++.h"
#include <core\logger.hpp>
using namespace hlslpp;
namespace SE
{
	inline hlslpp::float4x4 glmMat4ToHlslpp(const glm::mat4& glmMat)
	{
		return hlslpp::float4x4(
			glmMat[0][0], glmMat[1][0], glmMat[2][0], glmMat[3][0],
			glmMat[0][1], glmMat[1][1], glmMat[2][1], glmMat[3][1],
			glmMat[0][2], glmMat[1][2], glmMat[2][2], glmMat[3][2],
			glmMat[0][3], glmMat[1][3], glmMat[2][3], glmMat[3][3]
		);
	}

	template<typename T>
	constexpr T alignToPowerOfTwo(T value, T alignment) {
		SE_ASSERT_NOMSG((alignment & (alignment - 1)) == 0); // Verify power of 2
		return (value + (alignment - 1)) & ~(alignment - 1);
	}
}