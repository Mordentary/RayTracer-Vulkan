#include "shader_compiler.hpp"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <renderer\renderer.hpp>
#include <dxcapi.h>
namespace SE
{
	namespace
	{
		static const wchar_t* getTargetProfile(rhi::ShaderType type)

		{
			using namespace rhi;
			switch (type)
			{
			case ShaderType::Amplification: return L"as_6_6";
			case ShaderType::Mesh: return L"ms_6_6";
			case ShaderType::Vertex: return L"vs_6_6";
			case ShaderType::Pixel: return L"ps_6_6";
			case ShaderType::Compute: return L"cs_6_6";
			default: return L"";
			}
		}

		static std::wstring stringToWstring(const std::string& s)
		{
			return std::wstring(s.begin(), s.end());
		}
	}
	ShaderCompiler::ShaderCompiler(Renderer* renderer)
		: m_Renderer(renderer)
	{
		initializeDxc();
	}

	ShaderCompiler::~ShaderCompiler()
	{
	}

	bool ShaderCompiler::initializeDxc()
	{
		// Load the dxcompiler DLL
		HMODULE dxcModule = LoadLibraryW(L"dxcompiler.dll");
		if (!dxcModule)
		{
			std::cerr << "Failed to load dxcompiler.dll\n";
			return false;
		}

		// Obtain the function pointer
		auto DxcCreateInstanceFn = (decltype(DxcCreateInstance)*)GetProcAddress(dxcModule, "DxcCreateInstance");
		if (!DxcCreateInstanceFn)
		{
			std::cerr << "Failed to get DxcCreateInstance proc address\n";
			return false;
		}

		// Create IDxcUtils
		HRESULT hr = DxcCreateInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&m_Dxc.utils));
		if (FAILED(hr))
		{
			std::cerr << "Failed to create IDxcUtils\n";
			return false;
		}

		// Create IDxcCompiler3
		hr = DxcCreateInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Dxc.compiler));
		if (FAILED(hr))
		{
			std::cerr << "Failed to create IDxcCompiler3\n";
			return false;
		}

		// Default include handler
		hr = m_Dxc.utils->CreateDefaultIncludeHandler(&m_Dxc.defaultIncludeHandler);
		if (FAILED(hr))
		{
			std::cerr << "Failed to create default include handler\n";
			return false;
		}

		return true;
	}

	bool ShaderCompiler::compile(const std::string& source,
		const std::string& file,
		const std::string& entryPoint,
		rhi::ShaderType type,
		const std::vector<std::string>& defines,
		std::vector<std::byte>& output)
	{
		if (!m_Dxc.compiler)
		{
			std::cerr << "DXC compiler not initialized.\n";
			return false;
		}

		// Prepare DxcBuffer with source code
		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = source.data();
		sourceBuffer.Size = source.size();
		sourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8 code page

		// Convert to wide strings
		std::wstring wFile = stringToWstring(file);
		std::wstring wEntryPoint = stringToWstring(entryPoint);
		const wchar_t* wProfile = getTargetProfile(type);

		// Collect compile arguments
		std::vector<LPCWSTR> args;
		// Optional: -E <entrypoint>, -T <profile>
		args.push_back(L"-E");
		args.push_back(wEntryPoint.c_str());
		args.push_back(L"-T");
		args.push_back(wProfile);
		// Read the shader source

		const rhi::RenderBackend backend = m_Renderer->getDevice()->getDescription().backend;
		if (backend == rhi::RenderBackend::Vulkan)
		{
			args.push_back(L"-spirv");
			args.push_back(L"-fspv-target-env=vulkan1.3");
			args.push_back(L"-fvk-use-dx-layout");
			args.push_back(L"-fvk-bind-resource-heap");
			args.push_back(L"0");
			args.push_back(L"1");
			args.push_back(L"-fvk-bind-sampler-heap");
			args.push_back(L"0");
			args.push_back(L"2");
		}
		else if (backend == rhi::RenderBackend::D3D12)
		{
		}

#ifdef _DEBUG
		args.push_back(L"-Zi");
		args.push_back(L"-Qembed_debug");
		args.push_back(L"-O0");
#else
		args.push_back(L"-O3");
#endif

		Microsoft::WRL::ComPtr<IDxcResult> results;
		HRESULT hr = m_Dxc.compiler->Compile(
			&sourceBuffer,
			args.data(),
			(UINT32)args.size(),
			m_Dxc.defaultIncludeHandler.Get(),
			IID_PPV_ARGS(&results)
		);

		if (FAILED(hr) || !results)
		{
			std::cerr << "DXC compilation failed\n";
			return false;
		}

		// Check errors
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors = nullptr;
		results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			std::cerr << "[ShaderCompiler ERROR] " << (char*)errors->GetStringPointer() << "\n";
		}

		HRESULT status;
		results->GetStatus(&status);
		if (FAILED(status))
		{
			std::cerr << "Compilation failed for " << file << " / " << entryPoint << "\n";
			return false;
		}

		// Retrieve compiled shader blob
		Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
		results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		if (!shaderBlob)
		{
			std::cerr << "Failed to retrieve compiled shader blob\n";
			return false;
		}

		// Copy into output
		size_t byteSize = shaderBlob->GetBufferSize();
		output.resize(byteSize);
		memcpy(output.data(), shaderBlob->GetBufferPointer(), byteSize);

		return true;
	}
}