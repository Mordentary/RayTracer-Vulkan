#pragma once
#include "directed_acyclic_graph.hpp"
#include "RHI/rhi.hpp"
namespace SE
{
	class RenderGraphEdge;
	class RenderGraphPassBase;
	class RenderGraphResourceAllocator;

	class RenderGraphResource
	{
	public:
		RenderGraphResource(const std::string& name)
		{
			m_Name = name;
		}
		virtual ~RenderGraphResource() {}

		//virtual void resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass);
		virtual void realize() = 0;
		virtual rhi::IResource* getResource() = 0;
		virtual rhi::ResourceAccessFlags getInitialState() = 0;

		const char* getName() const { return m_Name.c_str(); }
		DAGNodeID getFirstPassID() const { return m_FirstPass; }
		DAGNodeID getLastPassID() const { return m_LastPass; }

		bool isUsed() const { return m_FirstPass != UINT32_MAX; }
		bool isImported() const { return m_isImported; }

		rhi::ResourceAccessFlags getFinalState() const { return m_LastState; }
		virtual void setFinalState(rhi::ResourceAccessFlags state) { m_LastState = state; }

		bool isOutput() const { return m_isOutput; }
		void setOutput(bool value) { m_isOutput = value; }

		bool isOverlapping() const { return !isImported() && !isOutput(); }

		virtual rhi::IResource* getAliasedPrevResource(rhi::ResourceAccessFlags& lastUsedState) = 0;
		virtual void barrier(rhi::ICommandList* pCommandList, uint32_t subresource, rhi::ResourceAccessFlags acess_before, rhi::ResourceAccessFlags accessAfter) = 0;

	protected:
		std::string m_Name;
		DAGNodeID m_FirstPass = UINT32_MAX;
		DAGNodeID m_LastPass = 0;
		rhi::ResourceAccessFlags m_LastState = rhi::ResourceAccessFlags::Discard;
		bool m_isImported = false;
		bool m_isOutput = false;
	};

	class RenderGraphResourceNode : public DAGNode
	{
	public:
		RenderGraphResourceNode(DirectedAcyclicGraph& graph, RenderGraphResource* resource, uint32_t version) :
			DAGNode(graph),
			m_graph(graph)
		{
			m_pResource = resource;
			m_version = version;
		}

		RenderGraphResource* getResource() const { return m_pResource; }
		uint32_t getVersion() const { return m_version; }
		DirectedAcyclicGraph& getDAG() const { return m_graph; }

		//virtual std::string getGraphvizName() const override
		//{
		//	std::string s = m_pResource->getName();
		//	s.append("\nversion:");
		//	s.append(std::to_string(m_version));
		//	if (m_version > 0)
		//	{
		//		std::vector<DAGEdge*> incoming_edges;
		//		m_graph.getIncomingEdges(this, incoming_edges);
		//		SE_ASSERT(incoming_edges.size() == 1);
		//		uint32_t subresource = ((RenderGraphEdge*)incoming_edges[0])->getSubresource();
		//		s.append("\nsubresource:");
		//		s.append(std::to_string(subresource));
		//	}
		//	return s;
		//}

	private:
		RenderGraphResource* m_pResource;
		uint32_t m_version;
		DirectedAcyclicGraph& m_graph;
	};

	class RGTexture : public RenderGraphResource
	{
	public:
		using Desc = rhi::TextureDescription;

		RGTexture(RenderGraphResourceAllocator& allocator, const std::string& name, const Desc& desc);
		RGTexture(RenderGraphResourceAllocator& allocator, rhi::ITexture* texture, rhi::ResourceAccessFlags state);
		~RGTexture();

		rhi::ITexture* getTexture() const { return m_pTexture; }
		rhi::IDescriptor* getSRV();
		rhi::IDescriptor* getUAV();
		rhi::IDescriptor* getUAV(uint32_t mip, uint32_t slice);

		//virtual void resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
		virtual void realize() override;
		virtual rhi::IResource* getResource() override { return m_pTexture; }
		virtual rhi::ResourceAccessFlags getInitialState() override { return m_InitialState; }
		virtual void barrier(rhi::ICommandList* pCommandList, uint32_t subresource, rhi::ResourceAccessFlags acess_before, rhi::ResourceAccessFlags acess_after) override;
		virtual rhi::IResource* getAliasedPrevResource(rhi::ResourceAccessFlags& lastUsedState) override;

	private:
		Desc m_Description;
		rhi::ITexture* m_pTexture = nullptr;
		rhi::ResourceAccessFlags m_InitialState = rhi::ResourceAccessFlags::Discard;
		RenderGraphResourceAllocator& m_Allocator;
	};

	class RGBuffer : public RenderGraphResource
	{
	public:
		using Desc = rhi::BufferDescription;

		RGBuffer(RenderGraphResourceAllocator& allocator, const std::string& name, const Desc& desc);
		RGBuffer(RenderGraphResourceAllocator& allocator, rhi::IBuffer* buffer, rhi::ResourceAccessFlags state);
		~RGBuffer();

		rhi::IBuffer* getBuffer() const { return m_pBuffer; }
		rhi::IDescriptor* getSRV();
		rhi::IDescriptor* getUAV();

		//virtual void resolve(RenderGraphEdge* edge, RenderGraphPassBase* pass) override;
		virtual void realize() override;
		virtual rhi::IResource* getResource() override { return m_pBuffer; }
		virtual rhi::ResourceAccessFlags getInitialState() override { return m_InitialState; }
		virtual void barrier(rhi::ICommandList* pCommandList, uint32_t subresource, rhi::ResourceAccessFlags acess_before, rhi::ResourceAccessFlags acess_after) override;
		virtual rhi::IResource* getAliasedPrevResource(rhi::ResourceAccessFlags& lastUsedState) override;

	private:
		Desc m_Description;
		rhi::IBuffer* m_pBuffer = nullptr;
		rhi::ResourceAccessFlags m_InitialState = rhi::ResourceAccessFlags::Discard;
		RenderGraphResourceAllocator& m_Allocator;
	};
}