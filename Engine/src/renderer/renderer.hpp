#pragma once
#include"../RHI/types.hpp"
#include "../RHI/rhi.hpp"
#include "engine_core.h"
#include<array>
namespace SE
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height);
		void createRenderTarget(uint32_t renderWidth, uint32_t renderHeight);
		void renderFrame();
		rhi::Device* getDevice() const { return m_Device.get(); }
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
			Scoped<rhi::Buffer> stagingBufferAllocator = nullptr;
		};

		Scoped<rhi::Fence> m_FrameFence = nullptr;
		uint64_t m_CurrentFenceFrameValue = 0;
		std::array<FrameResources, SE_MAX_FRAMES_IN_FLIGHT> m_FrameResources{};

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