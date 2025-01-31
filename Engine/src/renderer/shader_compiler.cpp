#include "shader_compiler.hpp"
#include "core\engine.hpp"
#include "renderer\renderer.hpp"

#include <windows.h>
#include <fstream>
#include <sstream>
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

	DXCIncludeHandler::DXCIncludeHandler(
		SE::ShaderCache* shaderCache,
		Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils,
		const std::filesystem::path& shadersDir
	) : m_ShaderCache(shaderCache), m_DxcUtils(dxcUtils), m_ShadersDir(shadersDir), m_Ref(0) {
	}

	HRESULT STDMETHODCALLTYPE DXCIncludeHandler::LoadSource(
		_In_ LPCWSTR fileName,
		_COM_Outptr_ IDxcBlob** includeSource)
	{
		std::filesystem::path fullPath = m_ShadersDir / fileName;
		fullPath = std::filesystem::absolute(fullPath);
		std::string absolutePath = fullPath.string();

		std::string content = m_ShaderCache->getCachedFileContent(absolutePath);
		SE_ASSERT_MSG(!content.empty(), "Failed to load included shader file");
		if (content.empty())
			return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

		return m_DxcUtils->CreateBlob(
			content.data(),
			static_cast<UINT32>(content.size()),
			CP_UTF8,
			reinterpret_cast<IDxcBlobEncoding**>(includeSource)
		);
	}

	ULONG STDMETHODCALLTYPE DXCIncludeHandler::AddRef() { return ++m_Ref; }
	ULONG STDMETHODCALLTYPE DXCIncludeHandler::Release()
	{
		ULONG result = --m_Ref;
		if (result == 0) delete this;
		return result;
	}

	HRESULT STDMETHODCALLTYPE DXCIncludeHandler::QueryInterface(REFIID riid, void** ppvObject)
	{
		if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown))
		{
			*ppvObject = static_cast<IDxcIncludeHandler*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ShaderCompiler::ShaderCompiler(Renderer* renderer) : m_Renderer(renderer)
	{
		SE_ASSERT(initializeDxc(), "DXC initialization failed");
	}

	bool ShaderCompiler::initializeDxc()
	{
		HMODULE dxcModule = LoadLibraryW(L"dxcompiler.dll");
		SE_ASSERT_MSG(dxcModule, "Failed to load dxcompiler.dll");
		if (!dxcModule) return false;

		auto DxcCreateInstanceFn = (decltype(DxcCreateInstance)*)GetProcAddress(dxcModule, "DxcCreateInstance");
		SE_ASSERT_MSG(DxcCreateInstanceFn, "Failed to get DxcCreateInstance proc address");
		if (!DxcCreateInstanceFn) return false;

		HRESULT hr = DxcCreateInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&m_Dxc.utils));
		SE_ASSERT_MSG(SUCCEEDED(hr), "Failed to create IDxcUtils");
		if (FAILED(hr)) return false;

		hr = DxcCreateInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Dxc.compiler));
		SE_ASSERT_MSG(SUCCEEDED(hr), "Failed to create IDxcCompiler3");
		if (FAILED(hr)) return false;

		m_Dxc.customIncludeHandler = new DXCIncludeHandler(
			m_Renderer->getShaderCache(),
			m_Dxc.utils,
			Engine::getInstance().getShaderPath()
		);
		SE_ASSERT(m_Dxc.customIncludeHandler, "DX include handler creation failed");
		return true;
	}

	bool ShaderCompiler::compile(const std::string& source,
		const std::string& file,
		const std::string& entryPoint,
		rhi::ShaderType type,
		const std::vector<std::string>& defines,
		std::vector<std::byte>& output)
	{
		SE_ASSERT_MSG(m_Dxc.compiler, "DXC compiler not initialized");
		if (!m_Dxc.compiler) return false;

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = source.data();
		sourceBuffer.Size = source.size();
		sourceBuffer.Encoding = DXC_CP_UTF8;

		std::wstring wFile = stringToWstring(file);
		std::wstring wEntryPoint = stringToWstring(entryPoint);
		const wchar_t* wProfile = getTargetProfile(type);

		std::vector<LPCWSTR> args;
		args.push_back(L"-E");
		args.push_back(wEntryPoint.c_str());
		args.push_back(L"-T");
		args.push_back(wProfile);
		args.push_back(L"-I");
		args.push_back(L"./shaders");

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
			m_Dxc.customIncludeHandler.Get(),
			IID_PPV_ARGS(&results)
		);

		SE_ASSERT_MSG(SUCCEEDED(hr) && results, "DXC compilation failed");
		if (FAILED(hr) || !results) return false;

		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors = nullptr;
		results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			SE_ASSERT_MSG(false, "Shader compilation errors occurred");
		}

		HRESULT status;
		results->GetStatus(&status);
		SE_ASSERT_MSG(SUCCEEDED(status), "Shader compilation failed");
		if (FAILED(status)) return false;

		Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
		results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		SE_ASSERT_MSG(shaderBlob, "Failed to retrieve compiled shader blob");
		if (!shaderBlob) return false;

		const size_t byteSize = shaderBlob->GetBufferSize();
		output.resize(byteSize);
		memcpy(output.data(), shaderBlob->GetBufferPointer(), byteSize);
		return true;
	}
}