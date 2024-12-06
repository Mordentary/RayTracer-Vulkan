#pragma once
#include "resource.hpp"
namespace rhi {
	class Fence : public Resource {
	public:
		virtual void wait(uint64_t value) = 0;
		virtual void signal(uint64_t value) = 0;
	};
}