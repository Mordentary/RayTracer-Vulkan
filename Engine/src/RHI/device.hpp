#pragma once
#include"types.hpp"
#include <cstdint>
#include <span>
#include <string>

namespace rhi {
	class CommandList;
	class Swapchain;
	class Fence;
	class Buffer;
	class Texture;
	class Shader;
	class Pipeline;
	class Descriptor;
	class Device
	{
	public:
		virtual ~Device() = default;
		virtual void* getHandle() const = 0;
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;

		uint64_t getFrameID() const { return m_FrameID % SE::SE_MAX_FRAMES_IN_FLIGHT; };
		const DeviceDescription& getDescription() const { return m_Description; }

		// Core resource creation
		virtual CommandList* createCommandList(CommandType queue_type, const std::string& name) = 0;
		virtual Swapchain* createSwapchain(const SwapchainDescription& desc, const std::string& name) = 0;
		virtual Fence* createFence(const std::string& name) = 0;
		virtual Buffer* createBuffer(const BufferDescription& desc, const std::string& name) = 0;
		virtual Texture* createTexture(const TextureDescription& desc, const std::string& name) = 0;
		virtual Shader* createShader(const ShaderDescription& desc, std::span<uint8_t> data, const std::string& name) = 0;
		virtual Pipeline* createGraphicsPipelineState(const GraphicsPipelineDescription& desc, const std::string& name) = 0;
		virtual Pipeline* createComputePipelineState(const ComputePipelineDescription& desc, const std::string& name) = 0;
		virtual Descriptor* createShaderResourceDescriptor(Resource* resource, const ShaderResourceDescriptorDescription& desc, const std::string& name) = 0;
		virtual Descriptor* createUnorderedAccessDescriptor(Resource* resource, const UnorderedAccessDescriptorDescription& desc, const std::string& name) = 0;
		virtual Descriptor* createConstantBufferDescriptor(Buffer* buffer, const ConstantBufferDescriptorDescription& desc, const std::string& name) = 0;
		virtual Descriptor* createSampler(const SamplerDescription& desc, const std::string& name) = 0;

	protected:
		DeviceDescription m_Description;
		uint64_t m_FrameID = 0;
	};
}