#include"renderer.hpp"
#include"core/engine.hpp"

using namespace rhi;
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

		//ShaderDescription shaderDesc{};
		//shaderDesc.type = ShaderType::Pixel;
		//Shader* shader = m_Device->createShader();
		initFrameResources();
	}
	void Renderer::createRenderTarget(uint32_t renderWidth, uint32_t renderHeight)
	{
		Engine::getInstance().getEditor().ViewportResizeSignal.connect(&Renderer::onViewportResize, this);
		m_RenderTargetSize.x = renderWidth;
		m_RenderTargetSize.y = renderHeight;
		rhi::TextureDescription textAttachDesc{};
		textAttachDesc.usage = TextureUsageFlags::RenderTarget;
		textAttachDesc.format = Format::R16G16B16A16_UNORM;
		textAttachDesc.width = renderWidth;
		textAttachDesc.height = renderHeight;
		textAttachDesc.depth = 1;
		m_RenderTargetColor.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Color"));
		textAttachDesc.usage = TextureUsageFlags::DepthStencil;
		textAttachDesc.format = Format::D24_UNORM_S8_UINT;
		m_RenderTargetDepth.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Depth"));
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
		waitForPreviousFrame();
		m_WindowSize = glm::vec2(width, height);
		m_Swapchain->resize(width, height);
	}
	void SE::Renderer::onViewportResize(uint32_t width, uint32_t height)
	{
		//createRenderTarget(width, height);
	}

	void Renderer::waitForPreviousFrame()
	{
		if (m_FrameFence)
		{
			m_FrameFence->wait(m_CurrentFenceFrameValue);
		}
	}

	void Renderer::copyToBackBuffer(rhi::CommandList* commandList)
	{
		m_Swapchain->acquireNextImage();
		//commandList->textureBarrier(m_RenderTargetColor.get(), ResourceAccessFlags::Discard, ResourceAccessFlags::RenderTarget);

		rhi::RenderPassDescription renderPass;
		renderPass.color[0].texture = m_RenderTargetColor.get();
		renderPass.color[0].loadOp = RenderPassLoadOp::DontCare;
		renderPass.depth.texture = m_RenderTargetDepth.get();
		renderPass.depth.loadOp = RenderPassLoadOp::DontCare;
		renderPass.depth.stencilLoadOp = RenderPassLoadOp::DontCare;
		renderPass.depth.storeOp = RenderPassStoreOp::DontCare;
		renderPass.depth.stencilStoreOp = RenderPassStoreOp::DontCare;
		renderPass.depth.readOnly = false;

		commandList->beginRenderPass(renderPass);
		//commandList->bindPipeline();
		commandList->endRenderPass();

		//commandList->textureBarrier(m_RenderTargetColor.get(), ResourceAccessFlags::RenderTarget, ResourceAccessFlags::ShaderRead);
		Texture* presentImage = m_Swapchain->getCurrentSwapchainImage();
		commandList->textureBarrier(presentImage, ResourceAccessFlags::Present, ResourceAccessFlags::RenderTarget);
		renderPass.color[0].texture = presentImage;
		renderPass.color[0].loadOp = RenderPassLoadOp::DontCare;
		renderPass.depth.texture = nullptr;
		commandList->beginRenderPass(renderPass);

		Engine::getInstance().getEditor().render(commandList);

		commandList->endRenderPass();
		commandList->textureBarrier(presentImage, ResourceAccessFlags::RenderTarget, ResourceAccessFlags::Present);
	}

	void Renderer::initFrameResources()
	{
		for (auto& frame : m_FrameResources)
		{
			frame.commandList.reset(m_Device->createCommandList(rhi::CommandType::Graphics, "MainCommands"));
			frame.computeCommandList.reset(m_Device->createCommandList(rhi::CommandType::Compute, "ComputeCommands"));
			frame.uploadCommandList.reset(m_Device->createCommandList(rhi::CommandType::Copy, "UploadCommands"));
			//frame.stagingBufferAllocator.reset(m_Device->createBuffer());
		}
		m_FrameFence.reset(m_Device->createFence("FrameFence"));
	}

	void SE::Renderer::beginFrame()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];
		m_FrameFence->wait(frame.frameFenceValue);

		m_Device->beginFrame();

		rhi::CommandList* pCommandList = frame.commandList.get();
		pCommandList->resetAllocator();
		pCommandList->begin();

		rhi::CommandList* pComputeCommandList = frame.computeCommandList.get();
		pComputeCommandList->resetAllocator();
		pComputeCommandList->begin();
	}

	void SE::Renderer::endFrame()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];

		rhi::CommandList* pComputeCommandList = frame.computeCommandList.get();
		pComputeCommandList->end();

		rhi::CommandList* pCommandList = frame.commandList.get();
		pCommandList->end();

		frame.frameFenceValue = ++m_CurrentFenceFrameValue;

		pCommandList->present(m_Swapchain.get());
		pCommandList->signal(m_FrameFence.get(), m_CurrentFenceFrameValue);
		pCommandList->submit();

		m_Device->endFrame();
	}

	void SE::Renderer::uploadResources()
	{
	}
	void SE::Renderer::render()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];
		CommandList* commandList = frame.commandList.get();

		copyToBackBuffer(commandList);
	}
}