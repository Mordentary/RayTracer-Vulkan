#pragma once
#include"types.hpp"
#include <cstdint>
#include <span>
#include <string>

namespace rhi {
	class ICommandList;
	class ISwapchain;
	class IFence;
	class IBuffer;
	class ITexture;
	class IShader;
	class IPipelineState;
	class IDescriptor;
	class IDevice
	{
	public:
		virtual ~IDevice() = default;
		virtual void* getHandle() const = 0;
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		uint64_t getFrameID() const { return m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT; };
		const DeviceDescription& getDescription() const { return m_Description; }

		// Core resource creation
		virtual ICommandList* createCommandList(CommandType queue_type, const std::string& name) = 0;
		virtual ISwapchain* createSwapchain(const SwapchainDescription& desc, const std::string& name) = 0;
		virtual IFence* createFence(const std::string& name) = 0;
		virtual IBuffer* createBuffer(const BufferDescription& desc, const std::string& name) = 0;
		virtual ITexture* createTexture(const TextureDescription& desc, const std::string& name) = 0;
		virtual IShader* createShader(const ShaderDescription& desc, std::span<std::byte> data, const std::string& name) = 0;
		virtual IPipelineState* createGraphicsPipelineState(const GraphicsPipelineDescription& desc, const std::string& name) = 0;
		virtual IPipelineState* createComputePipelineState(const ComputePipelineDescription& desc, const std::string& name) = 0;
		virtual IDescriptor* createShaderResourceViewDescriptor(IResource* resource, const ShaderResourceViewDescriptorDescription& desc, const std::string& name) = 0;
		virtual IDescriptor* createUnorderedAccessDescriptor(IResource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name) = 0;
		virtual IDescriptor* createConstantBufferDescriptor(IBuffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name) = 0;
		virtual IDescriptor* createSampler(const SamplerDescription& desc, const std::string& name) = 0;
		virtual IHeap* createHeap(const HeapDescription& desc, const std::string& name) = 0;

		virtual uint32_t getAllocationSize(const rhi::TextureDescription& desc) = 0;
	protected:
		DeviceDescription m_Description;
		uint64_t m_FrameID = 0;
	};
}