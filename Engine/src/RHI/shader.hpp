#pragma once

#include "resource.hpp"
#include "types.hpp"
#include<vector>
#include<span>

namespace rhi
{
	class Shader : public Resource
	{
	public:
		const ShaderDescription& getDescription() const { return m_Description; }
		uint64_t getHash() const { return m_Hash; }
		virtual bool create(std::span<uint8_t> data) = 0;

	protected:
		ShaderDescription m_Description = {};
		uint64_t m_Hash = 0;
	};
}