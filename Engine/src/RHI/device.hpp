#pragma once
#include "buffer.hpp"
#include "texture.hpp"
#include "swapchain.hpp"
#include "descriptor.hpp"

namespace rhi {
	class CommandList;
	class Fence;
	class DescriptorSet;

	struct DeviceDescription {
		void* windowHandle = nullptr;
		bool enableValidation = true;
		RenderBackend backend = RenderBackend::Vulkan;
	};

	class Device {
	public:
		virtual ~Device() = default;

		virtual void* getHandle() const = 0;
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual uint32_t getFrameID() const = 0;

		// Core resource creation
		virtual CommandList* createCommandList(CommandType queue_type, const std::string& name) = 0;
		virtual Swapchain* createSwapchain(const SwapchainDescription& desc, const std::string& name) = 0;
		virtual Fence* createFence(const std::string& name) = 0;
		virtual Buffer* createBuffer(const BufferDescription& desc, const std::string& name) = 0;
		virtual Texture* createTexture(const TextureDescription& desc, const std::string& name) = 0;
		//virtual Shader* CreateShader(const ShaderDescription& desc, std::span<uint8_t> data, const std::string& name) = 0;

	protected:
		DeviceDescription m_Description;
		uint64_t m_FrameID = 0;
	};

	class CommandList : public Resource {
	public:
		virtual void begin() = 0;
		virtual void end() = 0;
		virtual void reset() = 0;

		// Resource barriers
		virtual void transition(Buffer* buffer, ResourceState beforeState, ResourceState afterState) = 0;
		virtual void transition(Texture* texture, ResourceState beforeState, ResourceState afterState) = 0;

		// Clear operations
		virtual void clearRenderTarget(Texture* texture, const float color[4]) = 0;
		virtual void clearDepthStencil(Texture* texture, float depth, uint8_t stencil) = 0;

		// Draw operations
		virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0) = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0) = 0;

		// Copy operations
		virtual void copyBuffer(Buffer* dst, uint64_t dstOffset, Buffer* src, uint64_t srcOffset, uint64_t size) = 0;
		virtual void copyTexture(Texture* dst, Texture* src) = 0;
	};
} // namespace rhi