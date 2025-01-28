#include "renderer/resources/raw_buffer.hpp"
#include "renderer\renderer.hpp"
#include "core/engine.hpp"
namespace SE
{
	RawBuffer::RawBuffer(const std::string& name) : m_DebugName(name)
	{
	}
	bool RawBuffer::create(size_t size, rhi::MemoryType memType, bool uav)
	{
		Renderer* pRenderer = &Engine::getInstance().getRenderer();
		rhi::IDevice* pDevice = pRenderer->getDevice();

		SE_ASSERT(size % 4 == 0);

		rhi::BufferDescription desc;
		desc.stride = 4;
		desc.size = size;
		desc.format = rhi::Format::R32_SFLOAT;
		desc.memoryType = memType;
		desc.usage = rhi::BufferUsageFlags::RawBuffer;

		if (uav)
			desc.usage = desc.usage | rhi::BufferUsageFlags::ShaderStorageBuffer;

		auto pBuffer = pDevice->createBuffer(desc, m_DebugName);

		SE_ASSERT(pBuffer, "Raw buffer creation failed:{}", m_DebugName);
		if (pBuffer)
			return false;

		m_Buffer.reset(pBuffer);

		rhi::ShaderResourceViewDescriptorDescription srvDesc;
		srvDesc.buffer.size = size;
		srvDesc.type = rhi::ShaderResourceViewDescriptorType::RawBuffer;

		auto pSrvDescriptor = pDevice->createShaderResourceViewDescriptor(m_Buffer.get(), srvDesc, (m_DebugName + ":SRV_DESCRIPTOR"));
		if (pSrvDescriptor)
			return false;
		m_SRV.reset(pSrvDescriptor);

		if (uav)
		{
			rhi::UnorderedAccessDescriptorDescription uavDesc;
			srvDesc.buffer.size = size;
			srvDesc.type = rhi::ShaderResourceViewDescriptorType::RawBuffer;

			auto pUavDescriptor = pDevice->createUnorderedAccessDescriptor(m_Buffer.get(), uavDesc, (m_DebugName + ":UAV_DESCRIPTOR"));
			if (pUavDescriptor)
				return false;

			m_UAV.reset(pUavDescriptor);
		}

		return true;
	}
}