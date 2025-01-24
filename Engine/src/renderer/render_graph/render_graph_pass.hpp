#pragma once

#include "directed_acyclic_graph.hpp"
#include "RHI/rhi.hpp"
#include <functional>
#include <vector>
#include <string>
#include <limits>

namespace SE
{
	class Renderer;
	class RenderGraph;
	class RenderGraphResource;
	class RenderGraphEdgeColorAttachment;
	class RenderGraphEdgeDepthAttachment;

	enum class RenderPassType
	{
		Graphics,
		Compute,
		AsyncCompute,
		Copy,
	};

	struct RenderGraphAsyncResolveContext
	{
		std::vector<DAGNodeID> computeQueuePasses;
		std::vector<DAGNodeID> preGraphicsQueuePasses;
		std::vector<DAGNodeID> postGraphicsQueuePasses;
		uint64_t computeFence = 0;
		uint64_t graphicsFence = 0;
	};

	struct RenderGraphPassExecuteContext
	{
		Renderer* renderer = nullptr;
		rhi::ICommandList* graphicsCommandList = nullptr;
		rhi::ICommandList* computeCommandList = nullptr;
		rhi::IFence* computeQueueFence = nullptr;
		rhi::IFence* graphicsQueueFence = nullptr;

		uint64_t initialComputeFenceValue = 0;
		uint64_t lastSignaledComputeValue = 0;

		uint64_t initialGraphicsFenceValue = 0;
		uint64_t lastSignaledGraphicsValue = 0;
	};

	class RenderGraphPassBase : public DAGNode
	{
	public:
		RenderGraphPassBase(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph);
		virtual ~RenderGraphPassBase() = default;

		void resolveBarriers(const DirectedAcyclicGraph& graph);
		void resolveAsyncCompute(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context);
		void execute(const RenderGraph& graph, RenderGraphPassExecuteContext& context);

		RenderPassType getType() const { return m_Type; }
		DAGNodeID getWaitGraphicsPassID() const { return m_WaitGraphicsPass; }
		DAGNodeID getSignalGraphicsPassID() const { return m_SignalGraphicsPass; }

	protected:
		virtual void executeImpl(rhi::ICommandList* pCommandList) = 0;

		std::string m_Name;
		RenderPassType m_Type;

		struct ResourceBarrier
		{
			RenderGraphResource* resource = nullptr;
			uint32_t subResource = 0;
			rhi::ResourceAccessFlags oldState = rhi::ResourceAccessFlags::Discard;
			rhi::ResourceAccessFlags newState = rhi::ResourceAccessFlags::Discard;
		};
		std::vector<ResourceBarrier> m_ResourceBarriers;

		struct AliasDiscardBarrier
		{
			rhi::IResource* resource = nullptr;
			rhi::ResourceAccessFlags acessBefore = rhi::ResourceAccessFlags::Discard;
			rhi::ResourceAccessFlags acessAfter = rhi::ResourceAccessFlags::Discard;
		};
		std::vector<AliasDiscardBarrier> m_DiscardBarriers;

		RenderGraphEdgeColorAttachment* m_pColorRT[8] = {};
		RenderGraphEdgeDepthAttachment* m_pDepthRT = nullptr;

		// Only for async-compute pass:
		DAGNodeID m_WaitGraphicsPass = UINT32_MAX;
		DAGNodeID m_SignalGraphicsPass = UINT32_MAX;

		// Fence values if needed
		uint64_t m_SignalValue = uint64_t(-1);
		uint64_t m_WaitValue = uint64_t(-1);

	private:
		void begin(const RenderGraph& graph, rhi::ICommandList* pCommandList);
		void end(rhi::ICommandList* pCommandList);
		bool hasGfxRenderPass() const;
	};

	template<class T>
	class RenderGraphPass : public RenderGraphPassBase
	{
	public:
		RenderGraphPass(
			const std::string& name,
			RenderPassType type,
			DirectedAcyclicGraph& graph,
			const std::function<void(const T&, rhi::ICommandList*)>& execute)
			: RenderGraphPassBase(name, type, graph)
			, m_Execute(execute)
		{
		}

		T& getData() { return m_Parameters; }
		const T* operator->() { return &m_Parameters; }

	private:
		void executeImpl(rhi::ICommandList* pCommandList) override
		{
			m_Execute(m_Parameters, pCommandList);
		}

		T m_Parameters;
		std::function<void(const T&, rhi::ICommandList*)> m_Execute;
	};
}