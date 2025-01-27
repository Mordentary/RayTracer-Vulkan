#pragma once
#include<unordered_map>
#include <renderer\shader_compiler.hpp>
#include"xxHash\xxhash.h"

namespace std
{
	template <>
	struct std::hash<rhi::ShaderDescription>
	{
		size_t operator()(const rhi::ShaderDescription& desc) const
		{
			std::string s = desc.file + desc.entryPoint;
			for (size_t i = 0; i < desc.defines.size(); ++i)
			{
				s += desc.defines[i];
			}

			return std::hash<std::string>{}(s);
		}
	};
}

namespace SE
{
	class IShader;
	class Renderer;
	class ShaderCache
	{
	public:
		ShaderCache() = default;
		ShaderCache(Renderer* renderer);
		rhi::IShader* getShader(const std::string& file, const std::string& entry_point, rhi::ShaderType type, const std::vector<std::string>& defines);
		std::string getCachedFileContent(const std::string& file);

	private:
		rhi::IShader* createShader(const std::string& file, const std::string& entry_point, rhi::ShaderType type, const std::vector<std::string>& defines);
		void recompileShader(rhi::IShader* shader);
		static inline std::string loadFile(const std::string& path);
		void reloadShaders();

	private:
		Renderer* m_Renderer;
		std::unordered_map<rhi::ShaderDescription, Scoped<rhi::IShader>> m_ShaderCache;
		std::unordered_map<std::string, std::string> m_CachedFile;
	};
}