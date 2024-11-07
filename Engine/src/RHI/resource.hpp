#pragma once
#include <string>

namespace rhi {
	class Device;

	class Resource {
	public:
		virtual ~Resource() = default;
		virtual void* getHandle() const = 0;

		const std::string& getDebugName() const { return m_debugName; }
		void setDebugName(const std::string& name) { m_debugName = name; }

	protected:
		Device* m_device = nullptr;
		std::string m_debugName;
	};
} // namespace rhi