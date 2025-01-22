#include "render_graph_resources.hpp"
#include "render_graph.hpp"
namespace SE
{
	RGTexture::RGTexture(RenderGraphResourceAllocator& allocator, const std::string& name, const Desc& desc) :
		RenderGraphResource(name),
		m_Allocator(allocator)
	{
		m_Description = desc;
	}

	RGTexture::RGTexture(RenderGraphResourceAllocator& allocator, rhi::ITexture* texture, rhi::ResourceAccessFlags state) :
		RenderGraphResource(texture->getDebugName()),
		m_Allocator(allocator)
	{
		m_Description = texture->getDescription();
		m_pTexture = texture;
		m_InitialState = state;
		m_isImported = true;
	}

	RGTexture::~RGTexture()
	{
		if (!m_isImported)
		{
			if (m_isOutput)
			{
				m_Allocator.freeNonOverlappingTexture(m_pTexture, m_LastState);
			}
			else
			{
				m_Allocator.free(m_pTexture, m_LastState, m_isOutput);
			}
		}
	}

	rhi::IDescriptor* RGTexture::getSRV()
	{
		SE_ASSERT(!isImported());

		rhi::ShaderResourceViewDescriptorDescription desc;
		desc.format = m_pTexture->getDescription().format;

		return m_Allocator.getDescriptor(m_pTexture, desc);
	}

	rhi::IDescriptor* RGTexture::getUAV()
	{
		SE_ASSERT(!isImported());

		rhi::UnorderedAccessDescriptorDescription desc;
		desc.format = m_pTexture->getDescription().format;

		return m_Allocator.getDescriptor(m_pTexture, desc);
	}

	rhi::IDescriptor* RGTexture::getUAV(uint32_t mip, uint32_t slice)
	{
		SE_ASSERT(!isImported());
		rhi::UnorderedAccessDescriptorDescription desc;
		desc.format = m_pTexture->getDescription().format;
		desc.texture.mipSlice = mip;
		desc.texture.arraySlice = slice;
		return m_Allocator.getDescriptor(m_pTexture, desc);
	}

	void RGTexture::realize()
	{
		if (!m_isImported)
		{
			if (m_isOutput)
			{
				m_pTexture = m_Allocator.allocateNonOverlappingTexture(m_Description, m_Name, m_InitialState);
			}
			else
			{
				m_pTexture = m_Allocator.allocateTexture(m_FirstPass, m_LastPass, m_LastState, m_Description, m_Name, m_InitialState);
			}
		}
	}

	void RGTexture::barrier(rhi::ICommandList* pCommandList, uint32_t subresource, rhi::ResourceAccessFlags acess_before, rhi::ResourceAccessFlags acess_after)
	{
		pCommandList->textureBarrier(m_pTexture, subresource, acess_before, acess_after);
	}

	rhi::IResource* RGTexture::getAliasedPrevResource(rhi::ResourceAccessFlags& lastUsedState)
	{
		return m_Allocator.getAliasedPrevResource(m_pTexture, m_FirstPass, lastUsedState);
	}

	RGBuffer::RGBuffer(RenderGraphResourceAllocator& allocator, const std::string& name, const Desc& desc) :
		RenderGraphResource(name),
		m_Allocator(allocator)
	{
		m_Description = desc;
	}

	RGBuffer::RGBuffer(RenderGraphResourceAllocator& allocator, rhi::IBuffer* buffer, rhi::ResourceAccessFlags state) :
		RenderGraphResource(buffer->getDebugName()),
		m_Allocator(allocator)
	{
		m_Description = buffer->getDescription();
		m_pBuffer = buffer;
		m_InitialState = state;
		m_isImported = true;
	}

	RGBuffer::~RGBuffer()
	{
		if (!m_isImported)
		{
			m_Allocator.free(m_pBuffer, m_LastState, m_isOutput);
		}
	}

	rhi::IDescriptor* RGBuffer::getSRV()
	{
		SE_ASSERT(!isImported());

		const rhi::BufferDescription& bufferDesc = m_pBuffer->getDescription();

		rhi::ShaderResourceViewDescriptorDescription desc;
		desc.format = bufferDesc.format;

		if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::StructuredBuffer))
		{
			desc.type = rhi::ShaderResourceViewDescriptorType::StructuredBuffer;
		}
		else if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::FormattedBuffer))
		{
			desc.type = rhi::ShaderResourceViewDescriptorType::FormattedBuffer;
		}
		else if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::RawBuffer))
		{
			desc.type = rhi::ShaderResourceViewDescriptorType::RawBuffer;
		}

		desc.buffer.offset = 0;
		desc.buffer.size = bufferDesc.size;

		return m_Allocator.getDescriptor(m_pBuffer, desc);
	}

	rhi::IDescriptor* RGBuffer::getUAV()
	{
		SE_ASSERT(!isImported());

		const rhi::BufferDescription& bufferDesc = m_pBuffer->getDescription();
		SE_ASSERT(anySet(bufferDesc.usage, rhi::BufferUsageFlags::StorageBuffer));

		rhi::UnorderedAccessDescriptorDescription desc;
		desc.format = bufferDesc.format;

		if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::StructuredBuffer))
		{
			desc.type = rhi::UnorderedAccessDescriptorType::StructuredBuffer;
		}
		else if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::FormattedBuffer))
		{
			desc.type = rhi::UnorderedAccessDescriptorType::FormattedBuffer;
		}
		else if (rhi::anySet(bufferDesc.usage, rhi::BufferUsageFlags::RawBuffer))
		{
			desc.type = rhi::UnorderedAccessDescriptorType::RawBuffer;
		}

		desc.buffer.offset = 0;
		desc.buffer.size = bufferDesc.size;

		return m_Allocator.getDescriptor(m_pBuffer, desc);
	}

	void RGBuffer::realize()
	{
		if (!m_isImported)
		{
			m_pBuffer = m_Allocator.allocateBuffer(m_FirstPass, m_LastPass, m_LastState, m_Description, m_Name, m_InitialState);
		}
	}

	void RGBuffer::barrier(rhi::ICommandList* pCommandList, uint32_t subResource, rhi::ResourceAccessFlags acessBefore, rhi::ResourceAccessFlags acessAfter)
	{
		pCommandList->bufferBarrier(m_pBuffer, acessBefore, acessAfter);
	}

	rhi::IResource* RGBuffer::getAliasedPrevResource(rhi::ResourceAccessFlags& lastUsedState)
	{
		return m_Allocator.getAliasedPrevResource(m_pBuffer, m_FirstPass, lastUsedState);
	}
}