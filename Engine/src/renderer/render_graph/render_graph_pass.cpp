#include "render_graph_pass.hpp"
#include "render_graph_resources.hpp"
#include <algorithm>
#include "render_graph_nodes_edges.hpp"
#include "renderer\renderer.hpp"
#include "renderer\render_graph\render_graph.hpp"

namespace SE
{
	RenderGraphPassBase::RenderGraphPassBase(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph)
		: DAGNode(graph)
	{
		m_Name = name;
		m_Type = type;
	}

	void RenderGraphPassBase::resolveBarriers(const DirectedAcyclicGraph& graph)
	{
		std::vector<DAGEdge*> edges;
		std::vector<DAGEdge*> resource_incoming;
		std::vector<DAGEdge*> resource_outgoing;

		// Incoming edges: find old resource states
		graph.getIncomingEdges(this, edges);
		for (size_t i = 0; i < edges.size(); ++i)
		{
			RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
			SE_ASSERT(edge->getToNode() == this->getId());

			RenderGraphResourceNode* resource_node =
				(RenderGraphResourceNode*)graph.getNode(edge->getFromNode()).value();
			RenderGraphResource* resource = resource_node->getResource();

			graph.getIncomingEdges(resource_node, resource_incoming);
			graph.getOutgoingEdges(resource_node, resource_outgoing);

			SE_ASSERT(resource_incoming.size() <= 1);
			SE_ASSERT(resource_outgoing.size() >= 1);

			rhi::ResourceAccessFlags old_state = rhi::ResourceAccessFlags::Present;
			rhi::ResourceAccessFlags new_state = edge->getUsage();

			// If there are multiple outgoing edges from resource_node, figure out the most recent usage
			if (resource_outgoing.size() > 1)
			{
				for (int j = (int)resource_outgoing.size() - 1; j >= 0; --j)
				{
					uint32_t subresource = ((RenderGraphEdge*)resource_outgoing[j])->getSubresource();
					DAGNodeID pass_id = resource_outgoing[j]->getToNode();
					if (subresource == edge->getSubresource() &&
						pass_id < this->getId() &&
						!graph.getNode(pass_id).value()->isCulled())
					{
						old_state = ((RenderGraphEdge*)resource_outgoing[j])->getUsage();
						break;
					}
				}
			}

			if (old_state == rhi::ResourceAccessFlags::Present)
			{
				// If not found, get the state from the pass that originally produced it
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
					m_DiscardBarriers.push_back({
						aliased_resource, alias_state, new_state | rhi::ResourceAccessFlags::Discard
						});
					is_aliased = true;
				}
			}

			if (old_state != new_state || is_aliased)
			{
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

		// Outgoing edges: track color/depth attachments if needed
		graph.getOutgoingEdges(this, edges);
		for (size_t i = 0; i < edges.size(); ++i)
		{
			RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
			SE_ASSERT(edge->getFromNode() == this->getId());

			rhi::ResourceAccessFlags new_state = edge->getUsage();
			if (new_state == rhi::ResourceAccessFlags::RenderTarget)
			{
				// Must be color attachment
				SE_ASSERT(dynamic_cast<RenderGraphEdgeColorAttachment*>(edge) != nullptr);
				RenderGraphEdgeColorAttachment* color_rt = (RenderGraphEdgeColorAttachment*)edge;
				m_pColorRT[color_rt->getColorIndex()] = color_rt;
			}
			else if (new_state == rhi::ResourceAccessFlags::DepthStencilStorage ||
				new_state == rhi::ResourceAccessFlags::DepthStencilRead)
			{
				// Must be depth attachment
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

				RenderGraphResourceNode* resource_node =
					(RenderGraphResourceNode*)graph.getNode(edge->getFromNode()).value();

				graph.getIncomingEdges(resource_node, resource_incoming);
				SE_ASSERT(resource_incoming.size() <= 1);

				if (!resource_incoming.empty())
				{
					RenderGraphPassBase* prePass =
						(RenderGraphPassBase*)graph.getNode(resource_incoming[0]->getFromNode()).value();
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

				RenderGraphResourceNode* resource_node =
					(RenderGraphResourceNode*)graph.getNode(edge->getToNode()).value();
				graph.getOutgoingEdges(resource_node, resource_outgoing);

				for (size_t j = 0; j < resource_outgoing.size(); j++)
				{
					RenderGraphPassBase* postPass =
						(RenderGraphPassBase*)graph.getNode(resource_outgoing[j]->getToNode()).value();
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
				// We have ended the batch of compute passes; set up waits/signals
				if (!context.preGraphicsQueuePasses.empty())
				{
					DAGNodeID graphicsPassToWaitID =
						*std::max_element(context.preGraphicsQueuePasses.begin(), context.preGraphicsQueuePasses.end());

					RenderGraphPassBase* graphicsPassToWait =
						(RenderGraphPassBase*)graph.getNode(graphicsPassToWaitID).value();
					if (graphicsPassToWait->m_SignalValue == uint64_t(-1))
					{
						graphicsPassToWait->m_SignalValue = ++context.graphicsFence;
					}

					RenderGraphPassBase* computePass =
						(RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[0]).value();
					computePass->m_WaitValue = graphicsPassToWait->m_SignalValue;

					for (size_t i = 0; i < context.computeQueuePasses.size(); ++i)
					{
						RenderGraphPassBase* passToSync =
							(RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[i]).value();
						passToSync->m_WaitGraphicsPass = graphicsPassToWaitID;
					}
				}

				if (!context.postGraphicsQueuePasses.empty())
				{
					DAGNodeID graphicsPassToSignalID =
						*std::min_element(context.postGraphicsQueuePasses.begin(), context.postGraphicsQueuePasses.end());

					RenderGraphPassBase* computePass =
						(RenderGraphPassBase*)graph.getNode(context.computeQueuePasses.back()).value();
					if (computePass->m_SignalValue == uint64_t(-1))
					{
						computePass->m_SignalValue = ++context.computeFence;
					}

					RenderGraphPassBase* graphicsPassToSignal =
						(RenderGraphPassBase*)graph.getNode(graphicsPassToSignalID).value();
					graphicsPassToSignal->m_WaitValue = computePass->m_SignalValue;

					for (size_t i = 0; i < context.computeQueuePasses.size(); ++i)
					{
						RenderGraphPassBase* passToSync =
							(RenderGraphPassBase*)graph.getNode(context.computeQueuePasses[i]).value();
						passToSync->m_SignalGraphicsPass = graphicsPassToSignalID;
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
		rhi::ICommandList* pCommandList =
			(m_Type == RenderPassType::AsyncCompute) ? context.computeCommandList : context.graphicsCommandList;

		// Possibly wait for another queue if needed
		if (m_WaitValue != uint64_t(-1))
		{
			pCommandList->end();
			pCommandList->submit();

			pCommandList->begin();
			context.renderer->setupGlobalConstants(pCommandList);

			// Insert queue wait
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

		// Possibly signal another queue
		if (m_SignalValue != uint64_t(-1))
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
			context.renderer->setupGlobalConstants(pCommandList);
		}
	}

	void RenderGraphPassBase::begin(const RenderGraph& graph, rhi::ICommandList* pCommandList)
	{
		// Alias discard barriers
		for (size_t i = 0; i < m_DiscardBarriers.size(); ++i)
		{
			const AliasDiscardBarrier& barrier = m_DiscardBarriers[i];
			if (barrier.resource->isTexture())
			{
				pCommandList->textureBarrier((rhi::ITexture*)barrier.resource,
					barrier.acessBefore,
					barrier.acessAfter);
			}
			else
			{
				pCommandList->bufferBarrier((rhi::IBuffer*)barrier.resource,
					barrier.acessBefore,
					barrier.acessAfter);
			}
		}

		// Resource state transitions
		for (size_t i = 0; i < m_ResourceBarriers.size(); ++i)
		{
			const ResourceBarrier& barrier = m_ResourceBarriers[i];
			barrier.resource->barrier(pCommandList, barrier.subResource, barrier.oldState, barrier.newState);
		}

		if (hasGfxRenderPass())
		{
			rhi::RenderPassDescription desc;

			for (int i = 0; i < 8; ++i)
			{
				if (m_pColorRT[i] != nullptr)
				{
					RenderGraphResourceNode* node = (RenderGraphResourceNode*)graph.getDAG().getNode(m_pColorRT[i]->getToNode()).value();
					rhi::ITexture* texture = ((RGTexture*)node->getResource())->getTexture();

					uint32_t mip, slice;
					rhi::decomposeSubresource(texture->getDescription(), m_pColorRT[i]->getSubresource(), mip, slice);

					desc.color[i].texture = texture;
					desc.color[i].mipSlice = mip;
					desc.color[i].arraySlice = slice;
					desc.color[i].loadOp = m_pColorRT[i]->getLoadOp();
					desc.color[i].storeOp = node->isCulled() ? rhi::RenderPassStoreOp::DontCare : rhi::RenderPassStoreOp::Store;
					memcpy(desc.color[i].clearColor, m_pColorRT[i]->getClearColor(), sizeof(float) * 4);
				}
			}

			if (m_pDepthRT != nullptr)
			{
				RenderGraphResourceNode* node = (RenderGraphResourceNode*)graph.getDAG().getNode(m_pDepthRT->getToNode()).value();
				rhi::ITexture* texture = ((RGTexture*)node->getResource())->getTexture();

				uint32_t mip, slice;
				rhi::decomposeSubresource(texture->getDescription(), m_pDepthRT->getSubresource(), mip, slice);

				desc.depth.texture = ((RGTexture*)node->getResource())->getTexture();
				desc.depth.loadOp = m_pDepthRT->getDepthLoadOp();
				desc.depth.mipSlice = mip;
				desc.depth.arraySlice = slice;
				desc.depth.storeOp = node->isCulled() ? rhi::RenderPassStoreOp::DontCare : rhi::RenderPassStoreOp::Store;
				desc.depth.stencilLoadOp = m_pDepthRT->getStencilLoadOp();
				desc.depth.stencilStoreOp = node->isCulled() ? rhi::RenderPassStoreOp::DontCare : rhi::RenderPassStoreOp::Store;
				desc.depth.clearDepth = m_pDepthRT->getClearDepth();
				desc.depth.clearStencil = m_pDepthRT->getClearStencil();
				desc.depth.readOnly = m_pDepthRT->isReadOnly();
			}
			pCommandList->beginRenderPass(desc);
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
		return (m_pDepthRT != nullptr);
	}
}