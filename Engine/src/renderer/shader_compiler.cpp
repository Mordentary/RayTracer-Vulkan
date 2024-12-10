#include "shader_compiler.hpp"

#include <windows.h>
#include <dxcapi.h>
#include <wrl.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include <renderer\renderer.hpp>

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

// Convert std::string to std::wstring
static std::wstring stringToWstring(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

SE::ShaderCompiler::ShaderCompiler(Renderer* renderer)
	: m_pRenderer(renderer)
{
}

bool SE::ShaderCompiler::readFileToString(const std::string& path, std::string& content)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.good())
		return false;

	std::ostringstream oss;
	oss << file.rdbuf();
	content = oss.str();
	return true;
}

bool SE::ShaderCompiler::compile(
	const std::string& file,
	const std::string& entry_point,
	rhi::ShaderType type,
	const std::vector<std::string>& defines,
	std::vector<uint8_t>& output
)
{
	// Load DXC compiler
	HMODULE dxcModule = LoadLibraryW(L"dxcompiler.dll");
	if (!dxcModule)
	{
		std::cerr << "Failed to load dxcompiler.dll\n";
		return false;
	}

	auto DxcCreateInstance = (HRESULT(__stdcall*)(REFCLSID, REFIID, void**))GetProcAddress(dxcModule, "DxcCreateInstance");
	if (!DxcCreateInstance)
	{
		std::cerr << "Failed to get DxcCreateInstance\n";
		return false;
	}

	Microsoft::WRL::ComPtr<IDxcUtils> pDxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> pDxcCompiler;
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pDxcUtils));
	if (FAILED(hr))
	{
		std::cerr << "Failed to create IDxcUtils\n";
		return false;
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pDxcCompiler));
	if (FAILED(hr))
	{
		std::cerr << "Failed to create IDxcCompiler\n";
		return false;
	}

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> pIncludeHandler;
	hr = pDxcUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
	if (FAILED(hr))
	{
		std::cerr << "Failed to create default include handler\n";
		return false;
	}

	// Read the shader source
	std::string sourceStr;
	if (!readFileToString(file, sourceStr))
	{
		std::cerr << "Failed to read shader file: " << file << "\n";
		return false;
	}

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = sourceStr.data();
	sourceBuffer.Size = sourceStr.size();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	std::wstring wFile = stringToWstring(file);
	std::wstring wEntryPoint = stringToWstring(entry_point);
	const wchar_t* wProfile = getTargetProfile(type);

	std::vector<std::wstring> wdefines;
	for (auto& d : defines)
	{
		wdefines.push_back(stringToWstring(d));
	}

	std::vector<LPCWSTR> args;
	args.push_back(wFile.c_str());
	args.push_back(L"-E");
	args.push_back(wEntryPoint.c_str());
	args.push_back(L"-T");
	args.push_back(wProfile);

	// Add defines
	for (auto& d : wdefines)
	{
		args.push_back(L"-D");
		args.push_back(d.c_str());
	}

	const rhi::RenderBackend backend = m_pRenderer->getDevice()->getDescription().backend;
	if (backend == rhi::RenderBackend::Vulkan)
	{
		args.push_back(L"-spirv");
		args.push_back(L"-fspv-target-env=vulkan1.3");
		args.push_back(L"-fvk-use-dx-layout");
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

	Microsoft::WRL::ComPtr<IDxcResult> pResults;
	hr = pDxcCompiler->Compile(
		&sourceBuffer,
		args.data(),
		(UINT32)args.size(),
		pIncludeHandler.Get(),
		IID_PPV_ARGS(&pResults));

	if (FAILED(hr) || pResults == nullptr)
	{
		std::cerr << "DXC compilation failed\n";
		return false;
	}

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	if (pErrors && pErrors->GetStringLength() > 0)
	{
		std::cerr << "Shader compilation errors:\n" << pErrors->GetStringPointer() << "\n";
	}

	HRESULT status;
	pResults->GetStatus(&status);
	if (FAILED(status))
	{
		std::cerr << "Compilation failed for " << file << " entry " << entry_point << "\n";
		return false;
	}

	// Get the compiled shader
	Microsoft::WRL::ComPtr<IDxcBlob> pShader = nullptr;
	hr = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
	if (FAILED(hr) || pShader == nullptr)
	{
		std::cerr << "Failed to retrieve compiled shader blob\n";
		return false;
	}

	output.resize(pShader->GetBufferSize());
	memcpy(output.data(), pShader->GetBufferPointer(), pShader->GetBufferSize());

	return true;
}