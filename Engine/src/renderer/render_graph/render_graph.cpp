#include"render_graph.hpp"
#include "renderer\renderer.hpp"
namespace SE
{
	RenderGraph::RenderGraph(Renderer* pRenderer) :
		m_ResourceAllocator(pRenderer->getDevice())
	{
		rhi::IDevice* device = pRenderer->getDevice();
		m_ComputeQueueFence.reset(device->createFence("RenderGraph::m_pComputeQueueFence"));
		m_GraphicsQueueFence.reset(device->createFence("RenderGraph::m_pGraphicsQueueFence"));
	}

	void RenderGraph::clear()
	{
		for (size_t i = 0; i < m_ObjFinalizer.size(); ++i)
		{
			m_ObjFinalizer[i].finalizer(m_ObjFinalizer[i].obj);
		}
		m_ObjFinalizer.clear();

		m_Graph.clear();

		m_Passes.clear();
		m_ResourceNodes.clear();
		m_Resources.clear();

		m_Allocator.reset();
		m_ResourceAllocator.reset();

		m_OutputResources.clear();
	}

	void RenderGraph::compile()
	{
		m_Graph.cull();

		RenderGraphAsyncResolveContext context;

		for (size_t i = 0; i < m_Passes.size(); ++i)
		{
			RenderGraphPassBase* pass = m_Passes[i];
			if (!pass->isCulled())
			{
				pass->resolveAsyncCompute(m_Graph, context);
			}
		}

		std::vector<DAGEdge*> edges;

		for (size_t i = 0; i < m_ResourceNodes.size(); ++i)
		{
			RenderGraphResourceNode* node = m_ResourceNodes[i];
			if (node->isCulled())
			{
				continue;
			}

			RenderGraphResource* resource = node->getResource();

			m_Graph.getOutgoingEdges(node, edges);
			for (size_t i = 0; i < edges.size(); ++i)
			{
				RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
				RenderGraphPassBase* pass = (RenderGraphPassBase*)m_Graph.getNode(edge->getToNode()).value();

				if (!pass->isCulled())
				{
					resource->resolve(edge, pass);
				}
			}

			m_Graph.getIncomingEdges(node, edges);
			for (size_t i = 0; i < edges.size(); ++i)
			{
				RenderGraphEdge* edge = (RenderGraphEdge*)edges[i];
				RenderGraphPassBase* pass = (RenderGraphPassBase*)m_Graph.getNode(edge->getToNode()).value();

				if (!pass->isCulled())
				{
					resource->resolve(edge, pass);
				}
			}
		}

		for (size_t i = 0; i < m_Resources.size(); ++i)
		{
			RenderGraphResource* resource = m_Resources[i];
			if (resource->isUsed())
			{
				resource->realize();
			}
		}

		for (size_t i = 0; i < m_Passes.size(); ++i)
		{
			RenderGraphPassBase* pass = m_Passes[i];
			if (!pass->isCulled())
			{
				pass->resolveBarriers(m_Graph);
			}
		}
	}

	void RenderGraph::execute(Renderer* pRenderer, rhi::ICommandList* pCommandList, rhi::ICommandList* pComputeCommandList)
	{
		RenderGraphPassExecuteContext context = {};
		context.renderer = pRenderer;
		context.graphicsCommandList = pCommandList;
		context.computeCommandList = pComputeCommandList;
		context.computeQueueFence = m_ComputeQueueFence.get();
		context.graphicsQueueFence = m_GraphicsQueueFence.get();
		context.initialComputeFenceValue = m_ComputeQueueFenceValue;
		context.initialGraphicsFenceValue = m_GraphicsQueueFenceValue;

		for (size_t i = 0; i < m_Passes.size(); ++i)
		{
			RenderGraphPassBase* pass = m_Passes[i];

			pass->execute(*this, context);
		}

		m_ComputeQueueFenceValue = context.lastSignaledComputeValue;
		m_GraphicsQueueFenceValue = context.lastSignaledGraphicsValue;

		for (size_t i = 0; i < m_OutputResources.size(); ++i)
		{
			const PresentTarget& target = m_OutputResources[i];
			if (target.resource->getFinalState() != target.state)
			{
				target.resource->barrier(pCommandList, 0, target.resource->getFinalState(), target.state);
				target.resource->setFinalState(target.state);
			}
		}
		m_OutputResources.clear();
	}

	void RenderGraph::present(const RGHandle& handle, rhi::ResourceAccessFlags finalState)
	{
		SE_ASSERT(handle.IsValid());

		RenderGraphResource* resource = getTexture(handle);
		resource->setOutput(true);

		RenderGraphResourceNode* node = m_ResourceNodes[handle.node];
		node->markTarget();

		PresentTarget target;
		target.resource = resource;
		target.state = finalState;
		m_OutputResources.push_back(target);
	}

	RGTexture* RenderGraph::getTexture(const RGHandle& handle)
	{
		if (!handle.IsValid())
		{
			return nullptr;
		}

		RenderGraphResource* resource = m_Resources[handle.index];
		SE_ASSERT(dynamic_cast<RGTexture*>(resource) != nullptr);
		return (RGTexture*)resource;
	}

	RGBuffer* RenderGraph::getBuffer(const RGHandle& handle)
	{
		if (!handle.IsValid())
		{
			return nullptr;
		}

		RenderGraphResource* resource = m_Resources[handle.index];
		SE_ASSERT(dynamic_cast<RGBuffer*>(resource) != nullptr);
		return (RGBuffer*)resource;
	}

	//std::string RenderGraph::Export()
	//{
	//	return std::string{};
	//	//return m_graph.exportGraphviz();
	//}

	RGHandle RenderGraph::import(rhi::ITexture* texture, rhi::ResourceAccessFlags state)
	{
		auto resource = allocate<RGTexture>(m_ResourceAllocator, texture, state);
		auto node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, 0);

		RGHandle handle;
		handle.index = (uint16_t)m_Resources.size();
		handle.node = (uint16_t)m_ResourceNodes.size();

		m_Resources.push_back(resource);
		m_ResourceNodes.push_back(node);

		return handle;
	}

	RGHandle RenderGraph::import(rhi::IBuffer* buffer, rhi::ResourceAccessFlags state)
	{
		auto resource = allocate<RGBuffer>(m_ResourceAllocator, buffer, state);
		auto node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, 0);

		RGHandle handle;
		handle.index = (uint16_t)m_Resources.size();
		handle.node = (uint16_t)m_ResourceNodes.size();

		m_Resources.push_back(resource);
		m_ResourceNodes.push_back(node);

		return handle;
	}

	RGHandle RenderGraph::read(RenderGraphPassBase* pass, const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource)
	{
		SE_ASSERT(input.IsValid());
		RenderGraphResourceNode* input_node = m_ResourceNodes[input.node];

		allocatePOD<RenderGraphEdge>(m_Graph, input_node, pass, usage, subresource);

		return input;
	}

	RGHandle RenderGraph::write(RenderGraphPassBase* pass, const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource)
	{
		SE_ASSERT(input.IsValid());
		RenderGraphResource* resource = m_Resources[input.index];

		RenderGraphResourceNode* input_node = m_ResourceNodes[input.node];
		allocatePOD<RenderGraphEdge>(m_Graph, input_node, pass, usage, subresource);

		RenderGraphResourceNode* output_node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, input_node->getVersion() + 1);
		allocatePOD<RenderGraphEdge>(m_Graph, pass, output_node, usage, subresource);

		RGHandle output;
		output.index = input.index;
		output.node = (uint16_t)m_ResourceNodes.size();

		m_ResourceNodes.push_back(output_node);

		return output;
	}

	RGHandle RenderGraph::writeColor(RenderGraphPassBase* pass, uint32_t color_index, const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp load_op, const glm::vec4& clear_color)
	{
		SE_ASSERT(input.IsValid());
		RenderGraphResource* resource = m_Resources[input.index];

		rhi::ResourceAccessFlags usage = rhi::ResourceAccessFlags::RenderTarget;

		RenderGraphResourceNode* input_node = m_ResourceNodes[input.node];
		allocatePOD<RenderGraphEdgeColorAttachment>(m_Graph, input_node, pass, usage, subresource, color_index, load_op, clear_color);

		RenderGraphResourceNode* output_node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, input_node->getVersion() + 1);
		allocatePOD<RenderGraphEdgeColorAttachment>(m_Graph, pass, output_node, usage, subresource, color_index, load_op, clear_color);

		RGHandle output;
		output.index = input.index;
		output.node = (uint16_t)m_ResourceNodes.size();

		m_ResourceNodes.push_back(output_node);

		return output;
	}

	RGHandle RenderGraph::writeDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp depth_load_op, rhi::RenderPassLoadOp stencil_load_op, float clear_depth, uint32_t clear_stencil)
	{
		SE_ASSERT(input.IsValid());
		RenderGraphResource* resource = m_Resources[input.index];

		rhi::ResourceAccessFlags usage = rhi::ResourceAccessFlags::DepthStencilStorage;

		RenderGraphResourceNode* input_node = m_ResourceNodes[input.node];
		allocatePOD<RenderGraphEdgeDepthAttachment>(m_Graph, input_node, pass, usage, subresource, depth_load_op, stencil_load_op, clear_depth, clear_stencil);

		RenderGraphResourceNode* output_node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, input_node->getVersion() + 1);
		allocatePOD<RenderGraphEdgeDepthAttachment>(m_Graph, pass, output_node, usage, subresource, depth_load_op, stencil_load_op, clear_depth, clear_stencil);

		RGHandle output;
		output.index = input.index;
		output.node = (uint16_t)m_ResourceNodes.size();

		m_ResourceNodes.push_back(output_node);

		return output;
	}

	RGHandle RenderGraph::readDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource)
	{
		SE_ASSERT(input.IsValid());
		RenderGraphResource* resource = m_Resources[input.index];

		rhi::ResourceAccessFlags usage = rhi::ResourceAccessFlags::DepthStencilRead;

		RenderGraphResourceNode* input_node = m_ResourceNodes[input.node];
		allocatePOD<RenderGraphEdgeDepthAttachment>(m_Graph, input_node, pass, usage, subresource, rhi::RenderPassLoadOp::Load, rhi::RenderPassLoadOp::Load, 0.0f, 0);

		RenderGraphResourceNode* output_node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, input_node->getVersion() + 1);
		allocatePOD<RenderGraphEdgeDepthAttachment>(m_Graph, pass, output_node, usage, subresource, rhi::RenderPassLoadOp::Load, rhi::RenderPassLoadOp::Load, 0.0f, 0);

		RGHandle output;
		output.index = input.index;
		output.node = (uint16_t)m_ResourceNodes.size();

		m_ResourceNodes.push_back(output_node);

		return output;
	}
}