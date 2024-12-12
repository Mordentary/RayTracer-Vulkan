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
		rhi::Device* getDevice() const { return m_Device.get(); }
		uint64_t getFrameID() { return m_Device->getFrameID(); };
		rhi::Swapchain* getSwapchain() const { return m_Swapchain.get(); }
		rhi::Texture* getRenderTarget() const { return m_RenderTargetColor.get(); }
	private:
		Scoped<rhi::Device> m_Device = nullptr;
		Scoped<rhi::Swapchain> m_Swapchain = nullptr;
		glm::vec2 m_WindowSize{};
		glm::vec2 m_RenderTargetSize{};
		Scoped<rhi::Texture> m_RenderTargetColor{};
		Scoped<rhi::Texture> m_RenderTargetDepth{};
		rhi::Pipeline* m_DefaultPipeline;

		struct FrameResources {
			uint64_t frameFenceValue = 0;
			Scoped<rhi::CommandList> commandList = nullptr;
			Scoped<rhi::CommandList> computeCommandList = nullptr;
			Scoped<rhi::CommandList> uploadCommandList = nullptr;
			Scoped<StagingBufferAllocator> stagingBufferAllocator = nullptr;
		};

		Scoped<rhi::Fence> m_UploadFence = nullptr;
		Scoped<rhi::Fence> m_FrameFence = nullptr;
		uint64_t m_CurrenFrameFenceValue = 0;
		uint64_t m_CurrentUploadFenceValue = 0;
		std::array<FrameResources, SE_MAX_FRAMES_IN_FLIGHT> m_FrameResources{};

		ShaderCompiler* compiler;
		struct TextureUpload
		{
			rhi::Texture* texture;
			uint32_t mip_level;
			uint32_t array_slice;
			StagingBuffer staging_buffer;
			uint32_t offset;
		};
		std::vector<TextureUpload> m_PendingTextureUploads;

		struct BufferUpload
		{
			rhi::Buffer* buffer;
			uint32_t offset;
			StagingBuffer staging_buffer;
		};
		std::vector<BufferUpload> m_PendingBufferUpload;
	private:
		void onWindowResize(uint32_t width, uint32_t height);
		void onViewportResize(uint32_t width, uint32_t height);
		void waitForPreviousFrame();
		void copyToBackBuffer(rhi::CommandList* commandList);
	private:
		void initFrameResources();
		void beginFrame();
		void uploadResources();
		void render();
		void endFrame();
	};
}