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
		// Core resource creation
		virtual Buffer* createBuffer(const BufferDescription& desc, const void* initialData = nullptr) = 0;
		virtual Texture* createTexture(const TextureDescription& desc, const void* initialData = nullptr) = 0;
		virtual Swapchain* createSwapchain(const SwapchainDescription& desc) = 0;
		//virtual DescriptorPool* createDescriptorPool(const DescriptorPoolDesc& desc) = 0;
		//virtual DescriptorSetLayout* createDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) = 0;
		//virtual DescriptorSet* allocateDescriptorSet(DescriptorPool* pool, DescriptorSetLayout* layout) = 0;
		//virtual void updateDescriptorSet(DescriptorSet* set, const DescriptorSetUpdate& update) = 0;

		// Command management
		virtual CommandList* createCommandList(CommandType type) = 0;
		virtual void submit(CommandList* cmdList, Fence* fence = nullptr) = 0;

		// Frame management
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual uint32_t getFrameID() const = 0;
		virtual uint32_t getFramesInFlight() const = 0;

		// Synchronization
		virtual Fence* createFence(bool signaled = false) = 0;
		virtual void waitForFence(Fence* fence) = 0;
		virtual void resetFence(Fence* fence) = 0;
		virtual void waitIdle() = 0;

		// Resource cleanup
		virtual void destroyResource(Resource* resource) = 0;

		virtual void setDebugName(Resource* resource, const char* name) = 0;
	protected:
		DeviceDescription m_Desc;
		uint64_t m_FrameCount = 0;
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