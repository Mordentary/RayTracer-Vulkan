#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "RHI\types.hpp"
#define NOMINMAX
#include <wrl.h>

class IDxcUtils;
class IDxcCompiler3;
class IDxcIncludeHandler;
namespace SE
{
	class Renderer;

	class ShaderCompiler
	{
	public:
		ShaderCompiler(Renderer* renderer);
		~ShaderCompiler();

		bool compile(const std::string& source,
			const std::string& file,
			const std::string& entryPoint,
			rhi::ShaderType type,
			const std::vector<std::string>& defines,
			std::vector<std::byte>& output);

	private:
		Renderer* m_Renderer = nullptr;
		struct DxcHandles
		{
			Microsoft::WRL::ComPtr<IDxcUtils>  utils;
			Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
			Microsoft::WRL::ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
		};
		DxcHandles m_Dxc;
		bool initializeDxc();
	};
}