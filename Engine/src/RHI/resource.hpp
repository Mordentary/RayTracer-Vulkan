#pragma once
#include <string>

namespace rhi {
	class IDevice;

	class IResource {
	public:
		virtual ~IResource() = default;
		virtual void* getHandle() const = 0;
		virtual bool isTexture() const { return false; }
		virtual bool isBuffer() const { return false; }

		const std::string& getDebugName() const { return m_DebugName; }

	protected:
		IDevice* m_Device = nullptr;
		std::string m_DebugName;
	};
}