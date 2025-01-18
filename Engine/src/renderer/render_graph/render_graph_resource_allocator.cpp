#include "render_graph_resource_allocator.hpp"
//#include "utils/math.h"
//#include "utils/fmt.h"
namespace SE
{
	RenderGraphResourceAllocator::RenderGraphResourceAllocator(rhi::IDevice* pDevice)
	{
		m_Device = pDevice;
	}

	RenderGraphResourceAllocator::~RenderGraphResourceAllocator()
	{
		for (auto iter = m_AllocatedHeaps.begin(); iter != m_AllocatedHeaps.end(); ++iter)
		{
			const Heap& heap = *iter;

			for (size_t i = 0; i < heap.resources.size(); ++i)
			{
				deleteDescriptor(heap.resources[i].resource);
				delete heap.resources[i].resource;
			}

			delete heap.heap;
		}
		for (auto iter = m_freeOverlappingTextures.begin(); iter != m_freeOverlappingTextures.end(); ++iter)
		{
			deleteDescriptor(iter->texture);
			delete iter->texture;
		}
	}

	void RenderGraphResourceAllocator::reset()
	{
		for (auto iter = m_AllocatedHeaps.begin(); iter != m_AllocatedHeaps.end();)
		{
			Heap& heap = *iter;
			checkHeapUsage(heap);

			if (heap.resources.empty())
			{
				delete heap.heap;
				iter = m_AllocatedHeaps.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		uint64_t current_frame = m_Device->getFrameID();

		for (auto iter = m_freeOverlappingTextures.begin(); iter != m_freeOverlappingTextures.end(); )
		{
			if (current_frame - iter->lastUsedFrame > 30)
			{
				deleteDescriptor(iter->texture);
				delete iter->texture;
				iter = m_freeOverlappingTextures.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	void RenderGraphResourceAllocator::checkHeapUsage(Heap& heap)
	{
		uint64_t current_frame = m_Device->getFrameID();

		for (auto iter = heap.resources.begin(); iter != heap.resources.end();)
		{
			const AliasedResource aliasedResource = *iter;

			if (current_frame - aliasedResource.lastUsedFrame > 30)
			{
				deleteDescriptor(aliasedResource.resource);

				delete aliasedResource.resource;
				iter = heap.resources.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	rhi::ITexture* RenderGraphResourceAllocator::allocateTexture(uint32_t firstPass, uint32_t lastPass, rhi::ResourceAccessFlags lastState,
		const rhi::TextureDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state)
	{
		LifetimeRange lifetime = { firstPass, lastPass };
		uint32_t texture_size = m_Device->getAllocationSize(desc);

		for (size_t i = 0; i < m_AllocatedHeaps.size(); ++i)
		{
			Heap& heap = m_AllocatedHeaps[i];
			if (heap.heap->getDescription().size < texture_size ||
				heap.isOverlapping(lifetime))
			{
				continue;
			}

			for (size_t j = 0; j < heap.resources.size(); ++j)
			{
				AliasedResource& aliasedResource = heap.resources[j];
				if (aliasedResource.resource->isTexture() && !aliasedResource.lifetime.isUsed() && ((rhi::ITexture*)aliasedResource.resource)->getDescription() == desc)
				{
					aliasedResource.lifetime = lifetime;
					initial_state = aliasedResource.lastUsedState;
					aliasedResource.lastUsedState = lastState;
					return (rhi::ITexture*)aliasedResource.resource;
				}
			}

			rhi::TextureDescription newDesc = desc;
			newDesc.heap = heap.heap;

			AliasedResource aliasedTexture;
			aliasedTexture.resource = m_Device->createTexture(newDesc, "RGTexture " + name);
			aliasedTexture.lifetime = lifetime;
			aliasedTexture.lastUsedState = lastState;
			heap.resources.push_back(aliasedTexture);

			if (isDepthFormat(desc.format))
			{
				initial_state = rhi::ResourceAccessFlags::MaskDepthStencilAccess;
			}
			else if (rhi::anySet(desc.usage, rhi::TextureUsageFlags::RenderTarget))
			{
				initial_state = rhi::ResourceAccessFlags::RenderTarget;
			}
			else if (rhi::anySet(desc.usage, rhi::TextureUsageFlags::ShaderStorage))
			{
				initial_state = rhi::ResourceAccessFlags::MaskShaderStorage;
			}

			SE_ASSERT(aliasedTexture.resource != nullptr);
			return (rhi::ITexture*)aliasedTexture.resource;
		}

		allocateHeap(texture_size);
		return allocateTexture(firstPass, lastPass, lastState, desc, name, initial_state);
	}

	rhi::IBuffer* RenderGraphResourceAllocator::allocateBuffer(uint32_t firstPass, uint32_t lastPass, rhi::ResourceAccessFlags lastState, const rhi::BufferDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state)
	{
		LifetimeRange lifetime = { firstPass, lastPass };
		uint32_t buffer_size = desc.size;

		for (size_t i = 0; i < m_AllocatedHeaps.size(); ++i)
		{
			Heap& heap = m_AllocatedHeaps[i];
			if (heap.heap->getDescription().size < buffer_size ||
				heap.isOverlapping(lifetime))
			{
				continue;
			}

			for (size_t j = 0; j < heap.resources.size(); ++j)
			{
				AliasedResource& aliasedResource = heap.resources[j];
				if (aliasedResource.resource->isBuffer() && !aliasedResource.lifetime.isUsed() && ((rhi::IBuffer*)aliasedResource.resource)->getDescription() == desc)
				{
					aliasedResource.lifetime = lifetime;
					initial_state = aliasedResource.lastUsedState;
					aliasedResource.lastUsedState = lastState;
					return (rhi::IBuffer*)aliasedResource.resource;
				}
			}

			rhi::BufferDescription newDesc = desc;
			newDesc.heap = heap.heap;

			AliasedResource aliasedBuffer;
			aliasedBuffer.resource = m_Device->createBuffer(newDesc, "RGBuffer " + name);
			aliasedBuffer.lifetime = lifetime;
			aliasedBuffer.lastUsedState = lastState;
			heap.resources.push_back(aliasedBuffer);

			initial_state = rhi::ResourceAccessFlags::Discard;

			SE_ASSERT(aliasedBuffer.resource != nullptr);
			return (rhi::IBuffer*)aliasedBuffer.resource;
		}

		allocateHeap(buffer_size);
		return allocateBuffer(firstPass, lastPass, lastState, desc, name, initial_state);
	}

	void RenderGraphResourceAllocator::allocateHeap(uint32_t size)
	{
		rhi::HeapDescription heapDesc;
		heapDesc.size = rhi::alignToPowerOfTwo(size, 64u * 1024);

		std::string heapName = fmt::format("RG Heap {:.1f} MB", heapDesc.size / (1024.0f * 1024.0f)).c_str();

		Heap heap;
		heap.heap = m_Device->createHeap(heapDesc, heapName);
		m_AllocatedHeaps.push_back(heap);
	}

	void RenderGraphResourceAllocator::free(rhi::IResource* resource, rhi::ResourceAccessFlags state, bool set_state)
	{
		if (resource != nullptr)
		{
			for (size_t i = 0; i < m_AllocatedHeaps.size(); ++i)
			{
				Heap& heap = m_AllocatedHeaps[i];

				for (size_t j = 0; j < heap.resources.size(); ++j)
				{
					AliasedResource& aliasedResource = heap.resources[j];
					if (aliasedResource.resource == resource)
					{
						aliasedResource.lifetime.reset();
						aliasedResource.lastUsedFrame = m_Device->getFrameID();
						if (set_state)
						{
							aliasedResource.lastUsedState = state;
						}

						return;
					}
				}
			}

			SE_ASSERT(false);
		}
	}

	rhi::IResource* RenderGraphResourceAllocator::getAliasedPrevResource(rhi::IResource* resource, uint32_t firstPass, rhi::ResourceAccessFlags& lastUsedState)
	{
		for (size_t i = 0; i < m_AllocatedHeaps.size(); ++i)
		{
			Heap& heap = m_AllocatedHeaps[i];
			if (!heap.contains(resource))
			{
				continue;
			}

			AliasedResource* aliased_resource = nullptr;
			rhi::IResource* prev_resource = nullptr;
			uint32_t prev_resource_lastpass = 0;

			for (size_t j = 0; j < heap.resources.size(); ++j)
			{
				AliasedResource& aliasedResource = heap.resources[j];

				if (aliasedResource.resource != resource &&
					aliasedResource.lifetime.lastPass < firstPass &&
					aliasedResource.lifetime.lastPass > prev_resource_lastpass)
				{
					aliased_resource = &aliasedResource;
					prev_resource = aliasedResource.resource;
					lastUsedState = aliasedResource.lastUsedState;

					prev_resource_lastpass = aliasedResource.lifetime.lastPass;
				}
			}

			if (aliased_resource)
			{
				aliased_resource->lastUsedState |= rhi::ResourceAccessFlags::Discard;
			}

			return prev_resource;
		}

		SE_ASSERT(false);
		return nullptr;
	}

	rhi::ITexture* RenderGraphResourceAllocator::allocateNonOverlappingTexture(const rhi::TextureDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state)
	{
		for (auto iter = m_freeOverlappingTextures.begin(); iter != m_freeOverlappingTextures.end(); ++iter)
		{
			rhi::ITexture* texture = iter->texture;
			if (texture->getDescription() == desc)
			{
				initial_state = iter->lastUsedState;
				m_freeOverlappingTextures.erase(iter);
				return texture;
			}
		}
		if (isDepthFormat(desc.format))
		{
			initial_state = rhi::ResourceAccessFlags::MaskDepthStencilAccess;
		}
		else if (rhi::anySet(desc.usage, rhi::TextureUsageFlags::RenderTarget))
		{
			initial_state = rhi::ResourceAccessFlags::RenderTarget;
		}
		else if (rhi::anySet(desc.usage, rhi::TextureUsageFlags::ShaderStorage))
		{
			initial_state = rhi::ResourceAccessFlags::MaskShaderStorage;
		}

		return m_Device->createTexture(desc, "RGTexture " + name);
	}

	void RenderGraphResourceAllocator::freeNonOverlappingTexture(rhi::ITexture* texture, rhi::ResourceAccessFlags state)
	{
		if (texture != nullptr)
		{
			m_freeOverlappingTextures.push_back({ texture, state, m_Device->getFrameID() });
		}
	}

	rhi::IDescriptor* RenderGraphResourceAllocator::getDescriptor(rhi::IResource* resource, const rhi::ShaderResourceViewDescriptorDescription& desc)
	{
		for (size_t i = 0; i < m_AllocatedSRVs.size(); ++i)
		{
			if (m_AllocatedSRVs[i].resource == resource &&
				m_AllocatedSRVs[i].desc == desc)
			{
				return m_AllocatedSRVs[i].descriptor;
			}
		}

		rhi::IDescriptor* srv = m_Device->createShaderResourceViewDescriptor(resource, desc, resource->getDebugName());

		m_AllocatedSRVs.push_back({ resource, srv, desc });

		return srv;
	}

	rhi::IDescriptor* RenderGraphResourceAllocator::getDescriptor(rhi::IResource* resource, const rhi::UnorderedAccessDescriptorDescription& desc)
	{
		for (size_t i = 0; i < m_AllocatedUAVs.size(); ++i)
		{
			if (m_AllocatedUAVs[i].resource == resource &&
				m_AllocatedUAVs[i].desc == desc)
			{
				return m_AllocatedUAVs[i].descriptor;
			}
		}

		rhi::IDescriptor* srv = m_Device->createUnorderedAccessDescriptor(resource, desc, resource->getDebugName());
		m_AllocatedUAVs.push_back({ resource, srv, desc });

		return srv;
	}

	void RenderGraphResourceAllocator::deleteDescriptor(rhi::IResource* resource)
	{
		for (auto iter = m_AllocatedSRVs.begin(); iter != m_AllocatedSRVs.end(); )
		{
			if (iter->resource == resource)
			{
				delete iter->descriptor;
				iter = m_AllocatedSRVs.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for (auto iter = m_AllocatedUAVs.begin(); iter != m_AllocatedUAVs.end(); )
		{
			if (iter->resource == resource)
			{
				delete iter->descriptor;
				iter = m_AllocatedUAVs.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
}