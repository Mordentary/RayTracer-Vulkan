#include "renderer/resources/structured_buffer.hpp"
#include "core/engine.hpp"
#include "renderer/renderer.hpp"
namespace SE
{
	StructuredBuffer::StructuredBuffer(const std::string& name) : m_DebugName(name)
	{
	}
	bool StructuredBuffer::create(uint32_t stride, uint32_t elementCount, rhi::MemoryType memType, bool uav)
	{
		Renderer* pRenderer = &Engine::getInstance().getRenderer();
		rhi::IDevice* pDevice = pRenderer->getDevice();

		rhi::BufferDescription desc;
		desc.stride = stride;
		desc.size = elementCount * stride;
		desc.format = rhi::Format::Unknown;
		desc.memoryType = memType;
		desc.usage = rhi::BufferUsageFlags::StructuredBuffer;

		if (uav)
			desc.usage |= rhi::BufferUsageFlags::ShaderStorageBuffer;

		auto pBuffer = pDevice->createBuffer(desc, m_DebugName);

		SE_ASSERT(pBuffer, "Raw buffer creation failed:{}", m_DebugName);
		if (!pBuffer)
			return false;

		m_Buffer.reset(pBuffer);

		rhi::ShaderResourceViewDescriptorDescription srvDesc;
		srvDesc.buffer.size = elementCount * stride;
		srvDesc.type = rhi::ShaderResourceViewDescriptorType::StructuredBuffer;

		auto pSrvDescriptor = pDevice->createShaderResourceViewDescriptor(m_Buffer.get(), srvDesc, (m_DebugName + ":SRV_DESCRIPTOR"));
		if (!pSrvDescriptor)
			return false;
		m_SRV.reset(pSrvDescriptor);

		if (uav)
		{
			rhi::UnorderedAccessDescriptorDescription uavDesc;
			srvDesc.buffer.size = elementCount * stride;
			srvDesc.type = rhi::ShaderResourceViewDescriptorType::StructuredBuffer;

			auto pUavDescriptor = pDevice->createUnorderedAccessDescriptor(m_Buffer.get(), uavDesc, (m_DebugName + ":UAV_DESCRIPTOR"));
			if (!pUavDescriptor)
				return false;

			m_UAV.reset(pUavDescriptor);
		}

		return true;
	}
}