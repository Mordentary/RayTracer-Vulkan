#pragma once
#include"../RHI/types.hpp"
#include "../RHI/rhi.hpp"
#include "engine_core.h"
#include<array>
#include "staging_buffer_allocator.hpp"

namespace SE
{
	class ShaderCompiler;
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height);
		void createRenderTarget(uint32_t renderWidth, uint32_t renderHeight);

		void renderFrame();
		rhi::IDevice* getDevice() const { return m_Device.get(); }
		uint64_t getFrameID() { return m_Device->getFrameID(); };
		rhi::ISwapchain* getSwapchain() const { return m_Swapchain.get(); }
		rhi::ITexture* getRenderTarget() const { return m_RenderTargetColor.get(); }
		void uploadTexture(rhi::ITexture* texture, const void* data);
		void uploadBuffer(rhi::IBuffer* buffer, uint32_t offset, const void* data, uint32_t data_size);
	private:
		Scoped<rhi::IDevice> m_Device = nullptr;
		Scoped<rhi::ISwapchain> m_Swapchain = nullptr;
		glm::vec2 m_WindowSize{};
		glm::vec2 m_RenderTargetSize{};
		Scoped<rhi::ITexture> m_RenderTargetColor{};
		Scoped<rhi::ITexture> m_RenderTargetDepth{};
		Scoped<rhi::IPipelineState> m_DefaultPipeline;

		Scoped<rhi::IShader> m_TestShaderVS;
		Scoped<rhi::IShader> m_TestShaderPS;
		Scoped<rhi::IBuffer> m_VertexBuffer;

		struct FrameResources {
			uint64_t frameFenceValue = 0;
			Scoped<rhi::ICommandList> commandList = nullptr;
			Scoped<rhi::ICommandList> computeCommandList = nullptr;
			Scoped<rhi::ICommandList> uploadCommandList = nullptr;
			Scoped<StagingBufferAllocator> stagingBufferAllocator = nullptr;
		};

		Scoped<rhi::IFence> m_UploadFence = nullptr;
		Scoped<rhi::IFence> m_FrameFence = nullptr;
		uint64_t m_CurrenFrameFenceValue = 0;
		uint64_t m_CurrentUploadFenceValue = 0;
		std::array<FrameResources, SE_MAX_FRAMES_IN_FLIGHT> m_FrameResources{};
		rhi::IDescriptor* m_VertexBufferDescriptor = nullptr;

		ShaderCompiler* m_Compiler;
		struct TextureUpload
		{
			rhi::ITexture* texture;
			uint32_t mip_level;
			uint32_t array_slice;
			StagingBuffer staging_buffer;
			uint32_t offset;
		};
		std::vector<TextureUpload> m_PendingTextureUploads;

		struct BufferUpload
		{
			rhi::IBuffer* buffer;
			uint32_t offset;
			StagingBuffer staging_buffer;
		};
		std::vector<BufferUpload> m_PendingBufferUpload;
	private:
		void onWindowResize(uint32_t width, uint32_t height);
		void onViewportResize(uint32_t width, uint32_t height);
		void waitForPreviousFrame();
		void copyToBackBuffer(rhi::ICommandList* commandList);
	private:
		void initFrameResources();
		void beginFrame();
		void uploadResources();
		void render();
		void endFrame();
	};
}