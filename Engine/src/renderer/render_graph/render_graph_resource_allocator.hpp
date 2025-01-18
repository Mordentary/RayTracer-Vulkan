#pragma once
#include "rhi/rhi.hpp"
namespace SE
{
	class RenderGraphResourceAllocator
	{
		struct LifetimeRange
		{
			uint32_t firstPass = UINT32_MAX;
			uint32_t lastPass = 0;

			void reset() { firstPass = UINT32_MAX; lastPass = 0; }
			bool isUsed() const { return firstPass != UINT32_MAX; }
			bool isOverlapping(const LifetimeRange& other) const
			{
				if (isUsed())
				{
					return firstPass <= other.lastPass && lastPass >= other.firstPass;
				}
				else
				{
					return false;
				}
			}
		};

		struct AliasedResource
		{
			rhi::IResource* resource;
			LifetimeRange lifetime;
			uint64_t lastUsedFrame = 0;
			rhi::ResourceAccessFlags lastUsedState = rhi::ResourceAccessFlags::Discard;
		};

		struct Heap
		{
			rhi::IHeap* heap;
			std::vector<AliasedResource> resources;

			bool isOverlapping(const LifetimeRange& lifetime) const
			{
				for (size_t i = 0; i < resources.size(); ++i)
				{
					if (resources[i].lifetime.isOverlapping(lifetime))
					{
						return true;
					}
				}
				return false;
			}

			bool contains(rhi::IResource* resource) const
			{
				for (size_t i = 0; i < resources.size(); ++i)
				{
					if (resources[i].resource == resource)
					{
						return true;
					}
				}
				return false;
			}
		};

		struct SRVDescriptor
		{
			rhi::IResource* resource;
			rhi::IDescriptor* descriptor;
			rhi::ShaderResourceViewDescriptorDescription desc;
		};

		struct UAVDescriptor
		{
			rhi::IResource* resource;
			rhi::IDescriptor* descriptor;
			rhi::UnorderedAccessDescriptorDescription desc;
		};

	public:
		RenderGraphResourceAllocator(rhi::IDevice* pDevice);
		~RenderGraphResourceAllocator();

		void reset();

		rhi::ITexture* allocateNonOverlappingTexture(const rhi::TextureDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state);
		void freeNonOverlappingTexture(rhi::ITexture* texture, rhi::ResourceAccessFlags state);

		rhi::ITexture* allocateTexture(uint32_t firstPass, uint32_t lastPass, rhi::ResourceAccessFlags lastState, const rhi::TextureDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state);
		rhi::IBuffer* allocateBuffer(uint32_t firstPass, uint32_t lastPass, rhi::ResourceAccessFlags lastState, const rhi::BufferDescription& desc, const std::string& name, rhi::ResourceAccessFlags& initial_state);
		void free(rhi::IResource* resource, rhi::ResourceAccessFlags state, bool set_state);

		rhi::IResource* getAliasedPrevResource(rhi::IResource* resource, uint32_t firstPass, rhi::ResourceAccessFlags& lastUsedState);

		rhi::IDescriptor* getDescriptor(rhi::IResource* resource, const rhi::ShaderResourceViewDescriptorDescription& desc);
		rhi::IDescriptor* getDescriptor(rhi::IResource* resource, const rhi::UnorderedAccessDescriptorDescription& desc);

	private:
		void checkHeapUsage(Heap& heap);
		void deleteDescriptor(rhi::IResource* resource);
		void allocateHeap(uint32_t size);

	private:
		rhi::IDevice* m_Device;
		std::vector<Heap> m_AllocatedHeaps;
		struct NonOverlappingTexture
		{
			rhi::ITexture* texture;
			rhi::ResourceAccessFlags lastUsedState;
			uint64_t lastUsedFrame;
		};
		std::vector<NonOverlappingTexture> m_freeOverlappingTextures;

		std::vector<SRVDescriptor> m_AllocatedSRVs;
		std::vector<UAVDescriptor> m_AllocatedUAVs;
	};
}