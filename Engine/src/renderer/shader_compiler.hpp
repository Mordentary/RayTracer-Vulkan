#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "RHI\types.hpp"
#define NOMINMAX
#include <wrl.h>
#include <dxcapi.h>

class IDxcUtils;
class IDxcCompiler3;
class IDxcIncludeHandler;
namespace SE
{
	class ShaderCache;
	class DXCIncludeHandler : public IDxcIncludeHandler {
	public:
		DXCIncludeHandler(
			ShaderCache* shaderCache,
			Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils,
			const std::filesystem::path& shadersDir
		);

		// COM Methods
		HRESULT STDMETHODCALLTYPE LoadSource(
			_In_ LPCWSTR fileName,
			_COM_Outptr_ IDxcBlob** includeSource
		) override;
		ULONG STDMETHODCALLTYPE AddRef() override;
		ULONG STDMETHODCALLTYPE Release() override;
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

	private:
		ShaderCache* m_ShaderCache;
		Microsoft::WRL::ComPtr<IDxcUtils> m_DxcUtils;
		std::filesystem::path m_ShadersDir;
		std::atomic<ULONG> m_Ref;
	};

	class Renderer;

	class ShaderCompiler
	{
	public:
		ShaderCompiler(Renderer* renderer);

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
			Microsoft::WRL::ComPtr<DXCIncludeHandler> customIncludeHandler;
		};
		DxcHandles m_Dxc;
		bool initializeDxc();
	};
}