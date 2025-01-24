#pragma once

#include <glm/vec4.hpp>
#include "directed_acyclic_graph.hpp"
#include "render_graph_resources.hpp"
#include "rhi/rhi.hpp"

namespace SE
{
	class RenderGraphEdge : public DAGEdge
	{
	public:
		RenderGraphEdge(
			DirectedAcyclicGraph& graph,
			DAGNode* from,
			DAGNode* to,
			rhi::ResourceAccessFlags usage,
			uint32_t subresource)
			: DAGEdge(graph, from, to)
			, m_Usage(usage)
			, m_Subresource(subresource)
		{
		}

		rhi::ResourceAccessFlags getUsage() const { return m_Usage; }
		uint32_t getSubresource() const { return m_Subresource; }

	private:
		rhi::ResourceAccessFlags m_Usage;
		uint32_t m_Subresource;
	};

	class RenderGraphEdgeColorAttachment : public RenderGraphEdge
	{
	public:
		RenderGraphEdgeColorAttachment(
			DirectedAcyclicGraph& graph,
			DAGNode* from,
			DAGNode* to,
			rhi::ResourceAccessFlags usage,
			uint32_t subresource,
			uint32_t colorIndex,
			rhi::RenderPassLoadOp loadOp,
			const glm::vec4& clearColor)
			: RenderGraphEdge(graph, from, to, usage, subresource)
			, m_ColorIndex(colorIndex)
			, m_LoadOp(loadOp)
		{
			m_ClearColor[0] = clearColor[0];
			m_ClearColor[1] = clearColor[1];
			m_ClearColor[2] = clearColor[2];
			m_ClearColor[3] = clearColor[3];
		}

		uint32_t getColorIndex()    const { return m_ColorIndex; }
		rhi::RenderPassLoadOp getLoadOp() const { return m_LoadOp; }
		const float* getClearColor()const { return m_ClearColor; }

	private:
		uint32_t m_ColorIndex = 0;
		rhi::RenderPassLoadOp m_LoadOp = rhi::RenderPassLoadOp::DontCare;
		float m_ClearColor[4] = {};
	};

	class RenderGraphEdgeDepthAttachment : public RenderGraphEdge
	{
	public:
		RenderGraphEdgeDepthAttachment(
			DirectedAcyclicGraph& graph,
			DAGNode* from,
			DAGNode* to,
			rhi::ResourceAccessFlags usage,
			uint32_t subresource,
			rhi::RenderPassLoadOp depthLoadOp,
			rhi::RenderPassLoadOp stencilLoadOp,
			float clearDepth,
			uint32_t clearStencil)
			: RenderGraphEdge(graph, from, to, usage, subresource)
			, m_DepthLoadOp(depthLoadOp)
			, m_StencilLoadOp(stencilLoadOp)
			, m_ClearDepth(clearDepth)
			, m_ClearStencil(clearStencil)
		{
			m_ReadOnly = rhi::anySet(usage, rhi::ResourceAccessFlags::DepthStencilRead);
		}

		rhi::RenderPassLoadOp getDepthLoadOp()   const { return m_DepthLoadOp; }
		rhi::RenderPassLoadOp getStencilLoadOp() const { return m_StencilLoadOp; }
		float getClearDepth()                    const { return m_ClearDepth; }
		uint32_t getClearStencil()               const { return m_ClearStencil; }
		bool isReadOnly()                        const { return m_ReadOnly; }

	private:
		rhi::RenderPassLoadOp m_DepthLoadOp = rhi::RenderPassLoadOp::DontCare;
		rhi::RenderPassLoadOp m_StencilLoadOp = rhi::RenderPassLoadOp::DontCare;
		float m_ClearDepth = 0.0f;
		uint32_t m_ClearStencil = 0;
		bool m_ReadOnly = false;
	};

	class RenderGraphResourceNode : public DAGNode
	{
	public:
		RenderGraphResourceNode(
			DirectedAcyclicGraph& graph,
			RenderGraphResource* resource,
			uint32_t version)
			: DAGNode(graph)
			, m_graph(graph)
			, m_pResource(resource)
			, m_version(version)
		{
		}

		RenderGraphResource* getResource() const { return m_pResource; }
		uint32_t getVersion()        const { return m_version; }
		DirectedAcyclicGraph& getDAG() const { return m_graph; }

	private:
		RenderGraphResource* m_pResource = nullptr;
		uint32_t m_version = 0;
		DirectedAcyclicGraph& m_graph;
	};
}