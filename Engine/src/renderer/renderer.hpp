#pragma once
#include "../RHI/rhi.hpp"
#include"../RHI/types.hpp"
#include "engine_core.h"

#include <sigslot/signal.hpp>
namespace SE
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height);
		void renderFrame();
	private:
		Scoped<rhi::Device> m_Device;
		Scoped<rhi::Swapchain> m_Swapchain;
		glm::vec2 m_WindowSize;
		glm::vec2 m_RenderTargetSize;

		struct FrameResources {
			uint64_t frameFenceValue = 0;
			Scoped<rhi::CommandList> commandList;
			Scoped<rhi::CommandList> computeCommandList;
			Scoped<rhi::CommandList> uploadCommandList;
			Scoped<rhi::Buffer> stagingBufferAllocator;
		};

		Scoped<rhi::Fence> m_FrameFence;
		std::array<FrameResources, rhi::SE_MAX_FRAMES_IN_FLIGHT> m_FrameResources;

	private:
		void onWindowResize(uint32_t width, uint32_t height);

	private:
		void initFrameResources();
		void beginFrame();
		void uploadResources();
		void render();
		void endFrame();
	};
}