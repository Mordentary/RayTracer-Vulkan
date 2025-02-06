#include "texture2D.hpp"
#include"renderer/renderer.hpp"
namespace SE
{
	Texture2D::Texture2D(const std::string& name)
	{
		m_DebugName = name;
	}

	bool Texture2D::create(uint32_t width, uint32_t height, uint32_t levels, rhi::Format format, rhi::TextureUsageFlags usage)
	{
		rhi::TextureDescription desc;
		desc.format = format;
		desc.mipLevels = levels;
		desc.usage = usage;

		desc.width = width;
		desc.height = height;
		desc.memoryType = rhi::MemoryType::GpuOnly;

		Renderer& renderer = Engine::getInstance().getRenderer();
		rhi::IDevice* device = renderer.getDevice();

		m_Texture.reset(device->createTexture(desc, m_DebugName));
		if (!m_Texture)
			return false;

		rhi::ShaderResourceViewDescriptorDescription srvDesc;
		srvDesc.format = format;
		m_SRV.reset(device->createShaderResourceViewDescriptor(m_Texture.get(), srvDesc, m_DebugName));
		if (!m_SRV)
			return false;

		if (rhi::anySet(usage, rhi::TextureUsageFlags::ShaderStorage))
		{
			for (uint32_t i = 0; i < levels; ++i)
			{
				rhi::UnorderedAccessDescriptorDescription uavDesc;
				uavDesc.format = format;
				uavDesc.texture.mipSlice = i;

				rhi::IDescriptor* uav = device->createUnorderedAccessDescriptor(m_Texture.get(), uavDesc, m_DebugName);
				if (!uav)
					return false;

				m_MimmapUAVs.emplace_back(uav);
			}
		}

		return true;
	}

	rhi::IDescriptor* Texture2D::getUAV(uint32_t mip) const
	{
		return m_MimmapUAVs[mip].get();
	}
}