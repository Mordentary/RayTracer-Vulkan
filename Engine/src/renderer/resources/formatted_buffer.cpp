#include "renderer/resources/formatted_buffer.hpp"
#include "core/engine.hpp"
#include "renderer/renderer.hpp"
namespace SE
{
	FormattedBuffer::FormattedBuffer(const std::string& name) : m_DebugName(name)
	{
	}
	bool FormattedBuffer::create(rhi::Format format, uint32_t elementsCount, rhi::MemoryType memType, bool uav)
	{
		Renderer* pRenderer = &Engine::getInstance().getRenderer();
		rhi::IDevice* pDevice = pRenderer->getDevice();

		uint32_t stride = getFormatRowPitch(format, 1);

		rhi::BufferDescription desc;
		desc.stride = stride;
		desc.size = stride * elementsCount;
		desc.format = format;
		desc.memoryType = memType;
		desc.usage = rhi::BufferUsageFlags::FormattedBuffer;

		if (uav)
			desc.usage |= rhi::BufferUsageFlags::ShaderStorageBuffer;

		auto pBuffer = pDevice->createBuffer(desc, m_DebugName);

		SE_ASSERT(pBuffer, "Raw buffer creation failed:{}", m_DebugName);
		if (!pBuffer)
			return false;

		m_Buffer.reset(pBuffer);

		rhi::ShaderResourceViewDescriptorDescription srvDesc;
		srvDesc.buffer.size = stride * elementsCount;
		srvDesc.type = rhi::ShaderResourceViewDescriptorType::FormattedBuffer;

		auto pSrvDescriptor = pDevice->createShaderResourceViewDescriptor(m_Buffer.get(), srvDesc, (m_DebugName + ":SRV_DESCRIPTOR"));
		if (!pSrvDescriptor)
			return false;
		m_SRV.reset(pSrvDescriptor);

		if (uav)
		{
			rhi::UnorderedAccessDescriptorDescription uavDesc;
			srvDesc.buffer.size = stride * elementsCount;
			srvDesc.type = rhi::ShaderResourceViewDescriptorType::FormattedBuffer;

			auto pUavDescriptor = pDevice->createUnorderedAccessDescriptor(m_Buffer.get(), uavDesc, (m_DebugName + ":UAV_DESCRIPTOR"));
			if (!pUavDescriptor)
				return false;

			m_UAV.reset(pUavDescriptor);
		}

		return true;
	}
}