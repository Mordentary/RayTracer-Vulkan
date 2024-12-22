#pragma once
#include "resource.hpp"
namespace rhi {
	class IFence : public IResource {
	public:
		virtual void wait(uint64_t value) = 0;
		virtual void signal(uint64_t value) = 0;
	};
}