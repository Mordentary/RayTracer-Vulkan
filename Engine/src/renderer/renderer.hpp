#pragma once
#include "../RHI/rhi.hpp"
#include "engine_core.h"
#include <sigslot\signal.hpp>
namespace SE
{
	class Renderer
	{
	public:
		~Renderer();
		void createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height);
		void renderFrame();
	private:
		Scoped<rhi::Device> m_Device;
		Scoped<rhi::Swapchain> m_Swapchain;

		std::unique_ptr<rhi::Fence> m_pFrameFence;
		uint64_t m_nCurrentFenceValue = 0;

		const static int MAX_INFLIGHT_FRAMES = 3;
		uint64_t m_FrameFenceStates[MAX_INFLIGHT_FRAMES] = {};
		std::unique_ptr<rhi::CommandList> m_pCommandLists[MAX_INFLIGHT_FRAMES];

	private:
	};
}