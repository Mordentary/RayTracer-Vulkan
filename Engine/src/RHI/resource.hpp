#pragma once
#include <string>

namespace rhi {
	class Device;

	class Resource {
	public:
		virtual ~Resource() = default;
		virtual void* getHandle() const = 0;

		const std::string& getDebugName() const { return m_DebugName; }

	protected:
		Device* m_pDevice = nullptr;
		std::string m_DebugName;
	};
} // namespace rhi