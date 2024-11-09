//#pragma once
//#include "resource.hpp"
//#include "types.hpp"
//
//namespace rhi
//{
//	struct MemoryBlockDesc
//	{
//		size_t size = 0;
//		rhi::MemoryType memoryType = rhi::MemoryType::GpuOnly;
//	};
//
//	class MemoryBlock : public Resource {
//	public:
//		virtual ~MemoryBlock() = default;
//		virtual bool create() = 0;
//		const MemoryBlockDesc& getDesc() const { return m_Desc; }
//
//	protected:
//		MemoryBlockDesc m_Desc;
//	};
//}