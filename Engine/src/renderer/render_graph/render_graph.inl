#pragma once

namespace SE
{
	template<typename T>
	void classFinalizer(void* p)
	{
		((T*)p)->~T();
	}

	template<typename T, typename... ArgsT>
	inline T* RenderGraph::allocate(ArgsT&&... arguments)
	{
		T* p = (T*)m_Allocator.allocate(sizeof(T));
		new (p) T(std::forward<ArgsT>(arguments)...);

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
		new (p) T(std::forward<ArgsT>(arguments)...);
		return p;
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

	template<typename Data, typename Setup, typename Exec>
	inline RenderGraphPass<Data>& RenderGraph::addPass(
		const std::string& name,
		RenderPassType type,
		const Setup& setup,
		const Exec& execute)
	{
		auto pass = allocate<RenderGraphPass<Data>>(name, type, m_Graph, execute);

		// Give pass a chance to specify input/outputs
		RGBuilder builder(this, pass); // Only if you have an RGBuilder that takes these
		setup(pass->getData(), builder);

		m_Passes.push_back(pass);
		return *pass;
	}
}