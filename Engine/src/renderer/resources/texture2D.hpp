#pragma once
#include "RHI\rhi.hpp"
namespace SE
{
	class Texture2D
	{
	public:
		Texture2D(const std::string& name);
		bool create(uint32_t width, uint32_t height, uint32_t levels, rhi::Format format, rhi::TextureUsageFlags flags);

		rhi::ITexture* getTexture() const { return m_Texture.get(); }
		rhi::IDescriptor* getSRV() const { return m_SRV.get(); }
		rhi::IDescriptor* getUAV(uint32_t mip = 0) const;

	private:
		std::string m_DebugName;

		Scoped<rhi::ITexture> m_Texture;
		Scoped<rhi::IDescriptor> m_SRV;
		std::vector<Scoped<rhi::IDescriptor>> m_MimmapUAVs;
	};
}