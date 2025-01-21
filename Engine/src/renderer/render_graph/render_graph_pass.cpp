#include"render_graph_pass.hpp"
#include "render_graph_resources.hpp"
#include "render_graph.hpp"
#include "../renderer.hpp"
#include <algorithm>
namespace SE
{
	RenderGraphPassBase::RenderGraphPassBase(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph) :
		DAGNode(graph)
	{
		m_Name = name;
		m_Type = type;
	}

	void RenderGraphPassBase::resolveBarriers(const DirectedAcyclicGraph& graph)
	{
		std::vector<DAGEdge*> edges;

		std::vector<DAGEdge*> resource_incoming;
		std::vector<DAGEdge*> resource_outgoing;

		graph.getIncomingEdges(this, edges);
		for (size_t i = 0; i < edges.size(); ++i)
		{
			RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
			SE_ASSERT(edge->getToNode() == this->getId());

			RenderGraphResourceNode* resource_node = (RenderGraphResourceNode*)graph.getNode(edge->getFromNode()).value();
			RenderGraphResource* resource = resource_node->getResource();

			graph.getIncomingEdges(resource_node, resource_incoming);
			graph.getOutgoingEdges(resource_node, resource_outgoing);
			SE_ASSERT(resource_incoming.size() <= 1);
			SE_ASSERT(resource_outgoing.size() >= 1);

			rhi::ResourceAccessFlags old_state = rhi::ResourceAccessFlags::Present;
			rhi::ResourceAccessFlags new_state = edge->getUsage();

			if (resource_outgoing.size() > 1)
			{
				//resource_outgoing should be sorted
				for (int i = (int)resource_outgoing.size() - 1; i >= 0; --i)
				{
					uint32_t subresource = ((RenderGraphEdge*)resource_outgoing[i])->getSubresource();
					DAGNodeID pass_id = resource_outgoing[i]->getToNode();
					if (subresource == edge->getSubresource() && pass_id < this->getId() && !graph.getNode(pass_id).value()->isCulled())
					{
						old_state = ((RenderGraphEdge*)resource_outgoing[i])->getUsage();
						break;
					}
				}
			}

			//if not found, get the state from the pass which output the resource
			if (old_state == rhi::ResourceAccessFlags::Present)
			{
				if (resource_incoming.empty())
				{
					SE_ASSERT(resource_node->getVersion() == 0);
					old_state = resource->getInitialState();
				}
				else
				{
					old_state = ((RenderGraphEdge*)resource_incoming[0])->getUsage();
				}
			}

			bool is_aliased = false;
			rhi::ResourceAccessFlags alias_state;

			if (resource->isOverlapping() && resource->getFirstPassID() == this->getId())
			{
				rhi::IResource* aliased_resource = resource->getAliasedPrevResource(alias_state);
				if (aliased_resource)
				{
					m_DiscardBarriers.push_back({ aliased_resource, alias_state, new_state | rhi::ResourceAccessFlags::Discard });

					is_aliased = true;
				}
			}

			if (old_state != new_state || is_aliased)
			{
				//TODO : uav barrier
				ResourceBarrier barrier;
				barrier.resource = resource;
				barrier.subResource = edge->getSubresource();
				barrier.oldState = old_state;
				barrier.newState = new_state;

				if (is_aliased)
				{
					barrier.oldState |= alias_state | rhi::ResourceAccessFlags::Discard;
				}

				m_ResourceBarriers.push_back(barrier);
			}
		}

		graph.getOutgoingEdges(this, edges);
		for (size_t i = 0; i < edges.size(); ++i)
		{
			RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
			SE_ASSERT(edge->getFromNode() == this->getId());

			rhi::ResourceAccessFlags new_state = edge->getUsage();

			if (new_state == rhi::ResourceAccessFlags::RenderTarget)
			{
				SE_ASSERT(dynamic_cast<RenderGraphEdgeColorAttachment*>(edge) != nullptr);

				RenderGraphEdgeColorAttachment* color_rt = (RenderGraphEdgeColorAttachment*)edge;
				m_pColorRT[color_rt->getColorIndex()] = color_rt;
			}
			else if (new_state == rhi::ResourceAccessFlags::DepthStencilStorage || new_state == rhi::ResourceAccessFlags::DepthStencilRead)
			{
				SE_ASSERT(dynamic_cast<RenderGraphEdgeDepthAttachment*>(edge) != nullptr);

				m_pDepthRT = (RenderGraphEdgeDepthAttachment*)edge;
			}
		}
	}

	void RenderGraphPassBase::resolveAsyncCompute(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context)
	{
		if (m_Type == RenderPassType::AsyncCompute)
		{
			std::vector<DAGEdge*> edges;

			std::vector<DAGEdge*> resource_incoming;
			std::vector<DAGEdge*> resource_outgoing;

			graph.getIncomingEdges(this, edges);
			for (size_t i = 0; i < edges.size(); ++i)
			{
				RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
				SE_ASSERT(edge->getToNode() == this->getId());

				RenderGraphResourceNode* resource_node = (RenderGraphResourceNode*)graph.getNode(edge->getFromNode()).value();

				graph.getIncomingEdges(resource_node, resource_incoming);
				SE_ASSERT(resource_incoming.size() <= 1);

				if (!resource_incoming.empty())
				{
					RenderGraphPassBase* prePass = (RenderGraphPassBase*)graph.getNode(resource_incoming[0]->getFromNode()).value();
					if (!prePass->isCulled() && prePass->getType() != RenderPassType::AsyncCompute)
					{
						context.preGraphicsQueuePasses.push_back(prePass->getId());
					}
				}
			}

			graph.getOutgoingEdges(this, edges);
			for (size_t i = 0; i < edges.size(); ++i)
			{
				RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
				SE_ASSERT(edge->getFromNode() == this->getId());

				RenderGraphResourceNode* resource_node = (RenderGraphResourceNode*)graph.getNode(edge->getToNode()).value();
				graph.getOutgoingEdges(resource_node, resource_outgoing);

				for (size_t i = 0; i < resource_outgoing.size(); i++)
				{
					RenderGraphPassBase* postPass = (RenderGraphPassBase*)graph.getNode(resource_outgoing[i]->getToNode()).value();
					if (!postPass->isCulled() && postPass->getType() != RenderPassType::AsyncCompute)
					{
						context.postGraphicsQueuePasses.push_back(postPass->getId());
					}
				}
			}

			context.computeQueuePasses.push_back(getId());
		}
		else
		{
			if (!context.computeQueuePasses.empty())
			{
				if (!context.preGraphicsQueuePasses.empty())
				{
					DAGNodeID graphicsPassToWaitID = *std::max_element(context.preGraphicsQueuePasses.begin(), context.preGraphicsQueuePasses.end());

					RenderGraphPassBase* graphicsPassToWait = (RenderGraphPassBase*)graph.getNode(graphicsPassToWaitID).value();
					if (graphicsPassToWait->m_SignalValue == -1)
					{
						graphicsPassToWait->m_SignalValue = ++context.graphicsFence;
					}

					RenderGraphPassBase* computePass = (RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[0]).value();
					computePass->m_WaitValue = graphicsPassToWait->m_SignalValue;

					for (size_t i = 0; i < context.computeQueuePasses.size(); ++i)
					{
						RenderGraphPassBase* computePass = (RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[i]).value();
						computePass->m_WaitGraphicsPass = graphicsPassToWaitID;
					}
				}

				if (!context.postGraphicsQueuePasses.empty())
				{
					DAGNodeID graphicsPassToSignalID = *std::min_element(context.postGraphicsQueuePasses.begin(), context.postGraphicsQueuePasses.end());

					RenderGraphPassBase* computePass = (RenderGraphPassBase*)graph.getNode(context.computeQueuePasses.back()).value();
					if (computePass->m_SignalValue == -1)
					{
						computePass->m_SignalValue = ++context.computeFence;
					}

					RenderGraphPassBase* graphicsPassToSignal = (RenderGraphPassBase*)graph.getNode(graphicsPassToSignalID).value();
					graphicsPassToSignal->m_WaitValue = computePass->m_SignalValue;

					for (size_t i = 0; i < context.computeQueuePasses.size(); ++i)
					{
						RenderGraphPassBase* computePass = (RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[i]).value();
						computePass->m_SignalGraphicsPass = graphicsPassToSignalID;
					}
				}

				context.computeQueuePasses.clear();
				context.preGraphicsQueuePasses.clear();
				context.postGraphicsQueuePasses.clear();
			}
		}
	}

	void RenderGraphPassBase::execute(const RenderGraph& graph, RenderGraphPassExecuteContext& context)
	{
		rhi::ICommandList* pCommandList = m_Type == RenderPassType::AsyncCompute ? context.computeCommandList : context.graphicsCommandList;

		if (m_WaitValue != -1)
		{
			pCommandList->end();
			pCommandList->submit();

			pCommandList->begin();
			//todo: setupGlobalconst

			if (m_Type == RenderPassType::AsyncCompute)
			{
				pCommandList->wait(context.graphicsQueueFence, context.initialGraphicsFenceValue + m_WaitValue);
			}
			else
			{
				pCommandList->wait(context.computeQueueFence, context.initialComputeFenceValue + m_WaitValue);
			}
		}

		if (!isCulled())
		{
			begin(graph, pCommandList);
			executeImpl(pCommandList);
			end(pCommandList);
		}

		if (m_SignalValue != -1)
		{
			pCommandList->end();
			if (m_Type == RenderPassType::AsyncCompute)
			{
				pCommandList->signal(context.computeQueueFence, context.initialComputeFenceValue + m_SignalValue);
				context.lastSignaledComputeValue = context.initialComputeFenceValue + m_SignalValue;
			}
			else
			{
				pCommandList->signal(context.graphicsQueueFence, context.initialGraphicsFenceValue + m_SignalValue);
				context.lastSignaledGraphicsValue = context.initialGraphicsFenceValue + m_SignalValue;
			}
			pCommandList->submit();

			pCommandList->begin();

			//todo: setupGlobalconst
		}
	}

	void RenderGraphPassBase::begin(const RenderGraph& graph, rhi::ICommandList* pCommandList)
	{
		for (size_t i = 0; i < m_DiscardBarriers.size(); ++i)
		{
			const AliasDiscardBarrier& barrier = m_DiscardBarriers[i];

			if (barrier.resource->isTexture())
			{
				pCommandList->textureBarrier((rhi::ITexture*)barrier.resource, barrier.acessBefore, barrier.acessAfter);
			}
			else
			{
				pCommandList->bufferBarrier((rhi::IBuffer*)barrier.resource, barrier.acessBefore, barrier.acessAfter);
			}
		}

		for (size_t i = 0; i < m_ResourceBarriers.size(); ++i)
		{
			const ResourceBarrier& barrier = m_ResourceBarriers[i];
			barrier.resource->barrier(pCommandList, barrier.subResource, barrier.oldState, barrier.newState);
		}

		if (hasGfxRenderPass())
		{
			//todo
		}
	}

	void RenderGraphPassBase::end(rhi::ICommandList* pCommandList)
	{
		if (hasGfxRenderPass())
		{
			pCommandList->endRenderPass();
		}
	}

	bool RenderGraphPassBase::hasGfxRenderPass() const
	{
		for (int i = 0; i < 8; i++)
		{
			if (m_pColorRT[i] != nullptr)
			{
				return true;
			}
		}

		return m_pDepthRT != nullptr;
	}
}