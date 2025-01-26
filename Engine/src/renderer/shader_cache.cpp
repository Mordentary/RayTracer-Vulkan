#include "shader_cache.hpp"
#include "renderer.hpp"
#include "core/engine.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include <memory>
namespace rhi
{
	inline bool operator==(const ShaderDescription& lhs, const ShaderDescription& rhs)
	{
		if (lhs.file != rhs.file ||
			lhs.entryPoint != rhs.entryPoint ||
			lhs.type != rhs.type ||
			lhs.defines.size() != rhs.defines.size())
		{
			return false;
		}
		for (size_t i = 0; i < lhs.defines.size(); ++i)
		{
			if (lhs.defines[i] != rhs.defines[i])
			{
				return false;
			}
		}
		return true;
	}
}

namespace SE {
	std::string ShaderCache::loadFile(const std::string& path)
	{
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs.good())
			return {};

		std::ostringstream oss;
		oss << ifs.rdbuf();
		return oss.str();
	}

	ShaderCache::ShaderCache(Renderer* renderer)
		: m_Renderer(renderer)
	{
		m_Renderer = renderer;
	}

	rhi::IShader* ShaderCache::getShader(const std::string& file,
		const std::string& entryPoint,
		rhi::ShaderType type,
		const std::vector<std::string>& defines)
	{
		std::string filePath = Engine::getInstance().getShaderPath() + file;
		std::string absolute_path = std::filesystem::absolute(filePath.c_str()).string().c_str();

		rhi::ShaderDescription desc;
		desc.file = file;
		desc.entryPoint = entryPoint;
		desc.type = type;
		desc.defines = defines;

		auto it = m_ShaderCache.find(desc);
		if (it != m_ShaderCache.end())
		{
			return it->second.get();
		}

		rhi::IShader* newShader = createShader(absolute_path, entryPoint, type, defines);
		if (newShader)
		{
			m_ShaderCache.emplace(desc, std::unique_ptr<rhi::IShader>(newShader));
		}
		return newShader;
	}

	rhi::IShader* ShaderCache::createShader(const std::string& file,
		const std::string& entryPoint,
		rhi::ShaderType type,
		const std::vector<std::string>& defines)
	{
		std::string source = getCachedFileContent(file);
		if (source.empty())
		{
			std::cerr << "ShaderCache: No source for file " << file << "\n";
			return nullptr;
		}

		std::vector<std::byte> compiled;
		bool success = m_Renderer->getShaderCompiler()->compile(
			source, file, entryPoint, type, defines, compiled
		);
		if (!success)
		{
			std::cerr << "ShaderCache: Compilation failed for " << file
				<< " : " << entryPoint << "\n";
			return nullptr;
		}

		std::string debugName = file + " : " + entryPoint;
		rhi::ShaderDescription desc{ type, file, entryPoint, defines };
		rhi::IShader* shaderObj =
			m_Renderer->getDevice()->createShader(desc, compiled, debugName);

		return shaderObj;
	}

	std::string ShaderCache::getCachedFileContent(const std::string& file)
	{
		auto found = m_CachedFile.find(file);
		if (found != m_CachedFile.end())
		{
			return found->second;
		}

		std::string content = loadFile(file);
		m_CachedFile[file] = content;
		return content;
	}

	void ShaderCache::reloadShaders()
	{
		for (auto& kvp : m_CachedFile)
		{
			const std::string& path = kvp.first;
			const std::string& oldContent = kvp.second;

			std::string newContent = loadFile(path);
			if (newContent != oldContent)
			{
				m_CachedFile[path] = newContent;
			}
		}
	}

	void ShaderCache::recompileShader(rhi::IShader* shader)
	{
	}
}