#include"renderer.hpp"
#include"core/engine.hpp"
namespace SE
{
	Renderer::Renderer()
	{
		Engine::getInstance().getWindow().WindowResizeSignal.connect(&Renderer::onWindowResize, this);
	}
	Renderer::~Renderer()
	{
		Engine::getInstance().getWindow().WindowResizeSignal.disconnect(&Renderer::onWindowResize, this);
	}

	void Renderer::createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height)
	{
		m_WindowSize.x = window_width;
		m_WindowSize.y = window_height;
		m_RenderTargetSize.x = window_width;
		m_RenderTargetSize.y = window_height;

		rhi::DeviceDescription desc;
		desc.backend = backend;
		desc.windowHandle = window_handle;

#ifdef _DEBUG
		desc.enableValidation = true;
#else
		desc.enableValidation = false;
#endif
		auto device = rhi::createDevice(desc);
		SE_ASSERT(device, "Device creation is failed");
		m_Device = std::move(device);

		SE_ASSERT(m_Device, "Device is null");

		rhi::SwapchainDescription swapchainDesc;
		swapchainDesc.windowHandle = window_handle;
		swapchainDesc.width = window_width;
		swapchainDesc.height = window_height;
		swapchainDesc.bufferCount = 3;
		swapchainDesc.format = rhi::Format::R8G8B8A8_UNORM;
		swapchainDesc.vsync = true;

		m_Swapchain.reset(m_Device->createSwapchain(swapchainDesc, "MainSwapchain"));

		initFrameResources();
	}
	void Renderer::renderFrame()
	{
		beginFrame();
		uploadResources();
		render();
		endFrame();
	}
	void SE::Renderer::onWindowResize(uint32_t width, uint32_t height)
	{
	}

	void Renderer::initFrameResources()
	{
	}

	void SE::Renderer::beginFrame()
	{
		uint32_t frameIndex = m_Device->getFrameID() % rhi::SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];

		//m_FrameFence->wait(frame.frameFenceValue);
		m_Device->beginFrame();

		//rhi::CommandList* pCommandList = frame.commandList.get();
		//pCommandList->begin();
	}
	void SE::Renderer::endFrame()
	{
	}

	void SE::Renderer::uploadResources()
	{
	}
	void SE::Renderer::render()
	{
	}
}