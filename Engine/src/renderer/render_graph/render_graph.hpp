#include "utils/linear_allocator.hpp"
#include <glm\ext\vector_float4.hpp>
#include "render_graph_handle.hpp"
#include "render_graph_resources.hpp"
#include "render_graph_resource_allocator.hpp"
#include "render_graph_pass.hpp"

namespace SE
{
	class RenderGraphResourceNode;
	class Renderer;
	class RGBuilder;

	class RenderGraph
	{
		friend class RGBuilder;
	public:
		RenderGraph(Renderer* pRenderer);

		template<typename Data, typename Setup, typename Exec>
		RenderGraphPass<Data>& addPass(const std::string& name, RenderPassType type, const Setup& setup, const Exec& execute);

		void clear();
		void compile();
		void execute(Renderer* pRenderer, rhi::ICommandList* pCommandList, rhi::ICommandList* pComputeCommandList);

		void present(const RGHandle& handle, rhi::ResourceAccessFlags filnal_state);

		RGHandle import(rhi::ITexture* texture, rhi::ResourceAccessFlags state);
		RGHandle import(rhi::IBuffer* buffer, rhi::ResourceAccessFlags state);

		RGTexture* getTexture(const RGHandle& handle);
		RGBuffer* getBuffer(const RGHandle& handle);

		const DirectedAcyclicGraph& getDAG() const { return m_Graph; }
		//std::string Export();

	private:
		template<typename T, typename... ArgsT>
		T* allocate(ArgsT&&... arguments);

		template<typename T, typename... ArgsT>
		T* allocatePOD(ArgsT&&... arguments);

		template<typename Resource>
		RGHandle create(const typename Resource::Desc& desc, const std::string& name);

		RGHandle read(RenderGraphPassBase* pass, const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource);
		RGHandle write(RenderGraphPassBase* pass, const RGHandle& input, rhi::ResourceAccessFlags usage, uint32_t subresource);

		RGHandle writeColor(RenderGraphPassBase* pass, uint32_t color_index, const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp load_op, const glm::vec4& clear_color);
		RGHandle writeDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource, rhi::RenderPassLoadOp depth_load_op, rhi::RenderPassLoadOp stencil_load_op, float clear_depth, uint32_t clear_stencil);
		RGHandle readDepth(RenderGraphPassBase* pass, const RGHandle& input, uint32_t subresource);

	private:
		LinearAllocator m_Allocator{ 512 * 1024 };
		RenderGraphResourceAllocator m_ResourceAllocator;
		DirectedAcyclicGraph m_Graph;

		std::unique_ptr<rhi::IFence> m_ComputeQueueFence;
		uint64_t m_ComputeQueueFenceValue = 0;

		std::unique_ptr<rhi::IFence> m_GraphicsQueueFence;
		uint64_t m_GraphicsQueueFenceValue = 0;

		std::vector<RenderGraphPassBase*> m_Passes;
		std::vector<RenderGraphResource*> m_Resources;
		std::vector<RenderGraphResourceNode*> m_ResourceNodes;

		struct ObjFinalizer
		{
			void* obj;
			void(*finalizer)(void*);
		};
		std::vector<ObjFinalizer>  m_ObjFinalizer;

		struct PresentTarget
		{
			RenderGraphResource* resource;
			rhi::ResourceAccessFlags state;
		};
		std::vector<PresentTarget> m_OutputResources;
	};

	template<typename T>
	void classFinalizer(void* p)
	{
		((T*)p)->~T();
	}
	template<typename T, typename... ArgsT>
	inline T* RenderGraph::allocate(ArgsT&&... arguments)
	{
		T* p = (T*)m_Allocator.allocate(sizeof(T));
		new (p) T(arguments...);

		ObjFinalizer finalizer;
		finalizer.obj = p;
		finalizer.finalizer = &classFinalizer<T>;
		m_ObjFinalizer.push_back(finalizer);

		return p;
	}

	template<typename T, typename... ArgsT>
	inline T* RenderGraph::allocatePOD(ArgsT&&... arguments)
	{
		T* p = (T*)m_Allocator.allocate(sizeof(T));
		new (p) T(arguments...);

		return p;
	}

	template<typename Data, typename Setup, typename Exec>
	inline RenderGraphPass<Data>& RenderGraph::addPass(const std::string& name, RenderPassType type, const Setup& setup, const Exec& execute)
	{
		auto pass = allocate<RenderGraphPass<Data>>(name, type, m_Graph, execute);

		RGBuilder builder(this, pass);
		setup(pass->getData(), builder);

		m_Passes.push_back(pass);

		return *pass;
	}

	template<typename Resource>
	inline RGHandle RenderGraph::create(const typename Resource::Desc& desc, const std::string& name)
	{
		auto resource = allocate<Resource>(m_ResourceAllocator, name, desc);
		auto node = allocatePOD<RenderGraphResourceNode>(m_Graph, resource, 0);

		RGHandle handle;
		handle.index = (uint16_t)m_Resources.size();
		handle.node = (uint16_t)m_ResourceNodes.size();

		m_Resources.push_back(resource);
		m_ResourceNodes.push_back(node);

		return handle;
	}
}