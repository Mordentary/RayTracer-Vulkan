#pragma once
#include "directed_acyclic_graph.hpp"
#include "RHI/rhi.hpp"
#include <functional>
#include <vector>

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
		Renderer* renderer;
		rhi::ICommandList* graphicsCommandList;
		rhi::ICommandList* computeCommandList;
		rhi::IFence* computeQueueFence;
		rhi::IFence* graphicsQueueFence;

		uint64_t initialComputeFenceValue;
		uint64_t lastSignaledComputeValue;

		uint64_t initialGraphicsFenceValue;
		uint64_t lastSignaledGraphicsValue;
	};

	class RenderGraphPassBase : public DAGNode
	{
	public:
		RenderGraphPassBase(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph);
		~RenderGraphPassBase() = default;

		void resolveBarriers(const DirectedAcyclicGraph& graph);
		void resolveAsyncCompute(const DirectedAcyclicGraph& graph, RenderGraphAsyncResolveContext& context);
		void execute(const RenderGraph& graph, RenderGraphPassExecuteContext& context);

		//virtual std::string getGraphvizName() const override { return m_name.c_str(); }
		//virtual const char* getGraphvizColor() const override { return !IsCulled() ? "darkgoldenrod1" : "darkgoldenrod4"; }

		RenderPassType getType() const { return m_Type; }
		DAGNodeID getWaitGraphicsPassID() const { return m_WaitGraphicsPass; }
		DAGNodeID getSignalGraphicsPassID() const { return m_SignalGraphicsPass; }

	private:
		void begin(const RenderGraph& graph, rhi::ICommandList* pCommandList);
		void end(rhi::ICommandList* pCommandList);

		bool hasGfxRenderPass() const;

		virtual void executeImpl(rhi::ICommandList* pCommandList) = 0;

	protected:
		std::string m_Name;
		RenderPassType m_Type;

		struct ResourceBarrier
		{
			RenderGraphResource* resource;
			uint32_t subResource;
			rhi::ResourceAccessFlags oldState;
			rhi::ResourceAccessFlags newState;
		};
		std::vector<ResourceBarrier> m_ResourceBarriers;

		struct AliasDiscardBarrier
		{
			rhi::IResource* resource;
			rhi::ResourceAccessFlags acessBefore;
			rhi::ResourceAccessFlags acessAfter;
		};
		std::vector<AliasDiscardBarrier> m_DiscardBarriers;

		RenderGraphEdgeColorAttachment* m_pColorRT[8] = {};
		RenderGraphEdgeDepthAttachment* m_pDepthRT = nullptr;

		//only for async-compute pass
		DAGNodeID m_WaitGraphicsPass = UINT32_MAX;
		DAGNodeID m_SignalGraphicsPass = UINT32_MAX;

		uint64_t m_SignalValue = -1;
		uint64_t m_WaitValue = -1;
	};

	template<class T>
	class RenderGraphPass : public RenderGraphPassBase
	{
	public:
		RenderGraphPass(const std::string& name, RenderPassType type, DirectedAcyclicGraph& graph, const std::function<void(const T&, rhi::ICommandList*)>& execute) :
			RenderGraphPassBase(name, type, graph)
		{
			m_Execute = execute;
		}

		T& getData() { return m_Parameters; }
		T const* operator->() { return &getData(); }

	private:
		void executeImpl(rhi::ICommandList* pCommandList) override
		{
			m_Execute(m_Parameters, pCommandList);
		}

	protected:
		T m_Parameters;
		std::function<void(const T&, rhi::ICommandList*)> m_Execute;
	};
}