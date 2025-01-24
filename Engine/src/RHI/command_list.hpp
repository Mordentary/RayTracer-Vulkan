#pragma once
#include "types.hpp"
#include "resource.hpp"
#include <cstdint>
namespace rhi
{
	class IFence;
	class IBuffer;
	class ITexture;
	class ISwapchain;
	class IDescriptor;
	class IPipelineState;

	class ICommandList : public IResource {
	public:
		virtual ~ICommandList() {}

		CommandType getQueueType() const { return m_CommandType; }

		virtual void resetAllocator() = 0;
		virtual void begin() = 0;
		virtual void end() = 0;
		virtual void wait(IFence* dstFence, uint64_t value) = 0;
		virtual void signal(IFence* dstFence, uint64_t value) = 0;
		virtual void present(ISwapchain* dstSwapchain) = 0;
		virtual void submit() = 0;
		virtual void resetState() = 0;

		virtual void copyBufferToTexture(ITexture* dstTexture, uint32_t mipLevel, uint32_t arraySlice, IBuffer* srcBuffer, uint32_t offset) = 0;
		virtual void copyTextureToBuffer(IBuffer* dstBuffer, uint32_t offset, ITexture* srcTexture, uint32_t mipLevel, uint32_t arraySlice) = 0;
		virtual void copyBuffer(IBuffer* dstBuffer, uint32_t dstOffset, IBuffer* srcBuffer, uint32_t srcOffset, uint32_t size) = 0;
		virtual void copyTexture(ITexture* dstTexture, uint32_t dstMip, uint32_t dstArray, ITexture* srcTexture, uint32_t srcMip, uint32_t srcArray) = 0;
		virtual void clearStorageBuffer(IResource* resource, IDescriptor* storage, const float* clearValue) = 0;
		virtual void clearStorageBuffer(IResource* resource, IDescriptor* storage, const uint32_t* clearValue) = 0;
		virtual void writeBuffer(IBuffer* dstBuffer, uint32_t offset, uint32_t data) = 0;
		//virtual void updateTileMappings(Texture* dstTexture, Heap* dstHeap, uint32_t mappingCount, const TileMapping* mappings) = 0;

		virtual void textureBarrier(ITexture* texture, ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) = 0;
		virtual void textureBarrier(ITexture* texture, uint32_t subResource, ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) = 0;
		virtual void bufferBarrier(IBuffer* buffer, ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) = 0;
		virtual void globalBarrier(ResourceAccessFlags accessBefore, ResourceAccessFlags accessAfter) = 0;
		virtual void flushBarriers() = 0;

		virtual void beginRenderPass(const RenderPassDescription& renderPass) = 0;
		virtual void endRenderPass() = 0;
		virtual void bindPipeline(IPipelineState* state) = 0;
		virtual void setStencilReference(uint8_t stencil) = 0;
		virtual void setBlendFactor(const float* blendFactor) = 0;
		virtual void setIndexBuffer(IBuffer* buffer, uint32_t offset, Format format) = 0;
		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void setGraphicsConstants(uint32_t slot, const void* data, size_t dataSize) = 0;
		virtual void setComputeConstants(uint32_t slot, const void* data, size_t dataSize) = 0;

		virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t indexOffset = 0) = 0;
		virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		//virtual void dispatchMesh(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

		//virtual void drawIndirect(Buffer* buffer, uint32_t offset) = 0;
		//virtual void drawIndexedIndirect(Buffer* buffer, uint32_t offset) = 0;
		//virtual void dispatchIndirect(Buffer* buffer, uint32_t offset) = 0;
		//virtual void dispatchMeshIndirect(Buffer* buffer, uint32_t offset) = 0;

		//virtual void multiDrawIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsOffset, Buffer* countBuffer, uint32_t countOffset) = 0;
		//virtual void multiDrawIndexedIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsOffset, Buffer* countBuffer, uint32_t countOffset) = 0;
		//virtual void multiDispatchIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsOffset, Buffer* countBuffer, uint32_t countOffset) = 0;
		//virtual void multiDispatchMeshIndirect(uint32_t maxCount, Buffer* argsBuffer, uint32_t argsOffset, Buffer* countBuffer, uint32_t countOffset) = 0;

	protected:
		CommandType m_CommandType;
	};
}