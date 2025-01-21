#pragma once
#include "rhi/rhi.hpp"
#include"directed_acyclic_graph.hpp"
namespace SE
{
	class RenderGraphEdge : public DAGEdge
	{
	public:
		RenderGraphEdge(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, rhi::ResourceAccessFlags usage, uint32_t subresource) :
			DAGEdge(graph, from, to)
		{
			m_Usage = usage;
			m_Subresource = subresource;
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
		RenderGraphEdgeColorAttachment(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, rhi::ResourceAccessFlags usage, uint32_t subresource,
			uint32_t colorIndex, rhi::RenderPassLoadOp loadOp, const float* clearColor) :
			RenderGraphEdge(graph, from, to, usage, subresource)
		{
			m_ColorIndex = colorIndex;
			m_LoadOp = loadOp;

			m_ClearColor[0] = clearColor[0];
			m_ClearColor[1] = clearColor[1];
			m_ClearColor[2] = clearColor[2];
			m_ClearColor[3] = clearColor[3];
		}
		uint32_t getColorIndex() const { return m_ColorIndex; }
		rhi::RenderPassLoadOp getLoadOp() const { return m_LoadOp; }
		const float* getClearColor() const { return m_ClearColor; }

	private:
		uint32_t m_ColorIndex;
		rhi::RenderPassLoadOp m_LoadOp;
		float m_ClearColor[4] = {};
	};

	class RenderGraphEdgeDepthAttachment : public RenderGraphEdge
	{
	public:
		RenderGraphEdgeDepthAttachment(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to, rhi::ResourceAccessFlags usage, uint32_t subresource,
			rhi::RenderPassLoadOp depthLoadOP, rhi::RenderPassLoadOp stencilLoadOp, float clearDepth, uint32_t clearStencil) :
			RenderGraphEdge(graph, from, to, usage, subresource)
		{
			m_DepthLoadOp = depthLoadOP;
			m_StencilLoadOp = stencilLoadOp;
			m_ClearDepth = clearDepth;
			m_ClearStencil = clearStencil;
			m_ReadOnly = rhi::anySet(usage, rhi::ResourceAccessFlags::DepthStencilRead) ? true : false;
		}

		rhi::RenderPassLoadOp getDepthLoadOp() const { return m_DepthLoadOp; };
		rhi::RenderPassLoadOp getStencilLoadOp() const { return m_StencilLoadOp; };
		float getClearDepth() const { return m_ClearDepth; }
		uint32_t getClearStencil() const { return m_ClearStencil; };
		bool isReadOnly() const { return m_ReadOnly; }

	private:
		rhi::RenderPassLoadOp m_DepthLoadOp;
		rhi::RenderPassLoadOp m_StencilLoadOp;
		float m_ClearDepth;
		uint32_t m_ClearStencil;
		bool m_ReadOnly;
	};
}