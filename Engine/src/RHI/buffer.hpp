#pragma once
#include "resource.hpp"
#include "types.hpp"

namespace rhi {
	struct BufferDescription {
		uint64_t size = 0;
		uint32_t stride = 0;
		MemoryType memoryType = MemoryType::GpuOnly;
		BufferUsageFlags usage = BufferUsageFlags::None;
		bool mapped = false;           // Whether buffer should be persistently mapped
	};

	class Buffer : public Resource {
	public:
		virtual ~Buffer() = default;

		// Memory mapping
		virtual void* map() = 0;
		virtual void unmap() = 0;
		virtual uint64_t getGpuAddress() const = 0;
		virtual void* getCpuAddress() = 0;
		// Returns true if buffer is currently mapped
		virtual bool isMapped() const = 0;

		const BufferDescription& getDescription() const { return m_Description; }
	protected:
		BufferDescription m_Description{};
	};
}