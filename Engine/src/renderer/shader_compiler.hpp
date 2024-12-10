#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "RHI\types.hpp"

namespace SE
{
	class Renderer;
	class ShaderCompiler
	{
	public:
		// Constructor that takes a Renderer pointer
		ShaderCompiler(Renderer* renderer);

		// Compiles a shader given source file, entry point, shader type, defines, and output binary.
		// The backend is derived from the renderer's device.
		bool compile(
			const std::string& file,
			const std::string& entry_point,
			rhi::ShaderType type,
			const std::vector<std::string>& defines,
			std::vector<uint8_t>& output
		);

	private:
		bool readFileToString(const std::string& path, std::string& content);
		Renderer* m_pRenderer = nullptr;
	};
}