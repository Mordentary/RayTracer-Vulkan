#pragma once
#include"RHI/rhi.hpp"
#include"renderer/gpu_scene.hpp"

namespace SE
{
	class Mesh
	{
		friend class Renderer;
	public:
		Mesh();
		create();

	private:
		Renderer* m_pRenderer = nullptr;
		std::string m_DebugName;

		OffsetAllocator::Allocation m_PosBuffer;
		OffsetAllocator::Allocation m_UvBuffer;
		OffsetAllocator::Allocation m_NormalBuffer;
		OffsetAllocator::Allocation m_TangentBuffer;

		OffsetAllocator::Allocation m_IndexBuffer;
		rhi::Format m_indexBufferFormat;
		uint32_t m_IndexCount = 0;
		uint32_t m_VertexCount = 0;

		InstanceData m_InstanceData = {};
		uint32_t m_InstanceIndex = 0;
	};
};