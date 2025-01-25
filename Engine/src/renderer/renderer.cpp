#include"renderer.hpp"
#include"core/engine.hpp"
#include"shader_compiler.hpp"
using uint = uint32_t;
#include"global_constants.hlsli"
using namespace rhi;
#include "render_graph/render_graph_builder.hpp"
namespace SE
{
	Renderer::Renderer()
	{
		Engine::getInstance().getWindow().WindowResizeSignal.connect(&Renderer::onWindowResize, this);
	}
	Renderer::~Renderer()
	{
		if (m_RenderGraph)
		{
			m_RenderGraph->clear();
		}
		Engine::getInstance().getWindow().WindowResizeSignal.disconnect(&Renderer::onWindowResize, this);
	}
	struct Vertex {
		glm::vec3 position;
	};
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
		SE_ASSERT(device.get(), "Device creation is failed");
		m_Device = std::move(device);
		SE_ASSERT(m_Device.get(), "Device is null");

		rhi::SwapchainDescription swapchainDesc;
		swapchainDesc.windowHandle = window_handle;
		swapchainDesc.width = window_width;
		swapchainDesc.height = window_height;
		swapchainDesc.bufferCount = 3;
		swapchainDesc.format = rhi::Format::R8G8B8A8_UNORM;
		swapchainDesc.vsync = true;

		m_Swapchain.reset(m_Device->createSwapchain(swapchainDesc, "MainSwapchain"));
		m_RenderGraph = CreateScoped<RenderGraph>(this);

		initFrameResources();

		m_Compiler = new ShaderCompiler(this);

		std::vector<uint8_t> vsBinary;
		bool vsSuccess = m_Compiler->compile(
			"shaders/defaultShader.hlsl",       // Path to HLSL file
			"VSMain",            // Entry point for Vertex Shader
			rhi::ShaderType::Vertex,
			{},                   // Defines
			vsBinary              // Output binary
		);

		if (!vsSuccess) {
			// Handle compilation failure for Vertex Shader
		}

		std::vector<uint8_t> psBinary;
		bool psSuccess = m_Compiler->compile(
			"shaders/defaultShader.hlsl",       // Path to HLSL file
			"PSMain",            // Entry point for Vertex Shader
			rhi::ShaderType::Pixel,
			{},                   // Defines
			psBinary              // Output binary
		);

		if (!psSuccess) {
			// Handle compilation failure for Pixel Shader
		}

		ShaderDescription shaderDesc{};
		shaderDesc.type = ShaderType::Vertex;
		shaderDesc.file = "defaultShader.hlsl";
		shaderDesc.entryPoint = "VSMain";
		m_TestShaderVS = Scoped<IShader>(m_Device->createShader(shaderDesc, vsBinary, "TestShaderVS"));
		shaderDesc.type = ShaderType::Pixel;
		shaderDesc.entryPoint = "PSMain";
		m_TestShaderPS = Scoped<IShader>(m_Device->createShader(shaderDesc, psBinary, "TestShaderPS"));

		GraphicsPipelineDescription pipeDesc{};
		pipeDesc.vertexShader = m_TestShaderVS.get();
		pipeDesc.pixelShader = m_TestShaderPS.get();
		pipeDesc.renderTargetFormat[0] = Format::R8G8B8A8_UNORM;
		pipeDesc.depthStencilFormat = Format::D24_UNORM_S8_UINT;
		m_DefaultPipeline = Scoped<rhi::IPipelineState>(m_Device->createGraphicsPipelineState(pipeDesc, "TestGraphicsPipeline"));

		std::vector<Vertex> cubeVertices = {
			// Front Face (+Z)
			{ glm::vec3(-0.5f, -0.5f,  0.5f) }, // Bottom Left
			{ glm::vec3(0.5f, -0.5f,  0.5f) }, // Bottom Right
			{ glm::vec3(0.5f,  0.5f,  0.5f) }, // Top Right

			{ glm::vec3(-0.5f, -0.5f,  0.5f) }, // Bottom Left
			{ glm::vec3(0.5f,  0.5f,  0.5f) }, // Top Right
			{ glm::vec3(-0.5f,  0.5f,  0.5f) }, // Top Left

			// Back Face (-Z)
			{ glm::vec3(0.5f, -0.5f, -0.5f) }, // Bottom Right
			{ glm::vec3(-0.5f, -0.5f, -0.5f) }, // Bottom Left
			{ glm::vec3(-0.5f,  0.5f, -0.5f) }, // Top Left

			{ glm::vec3(0.5f, -0.5f, -0.5f) }, // Bottom Right
			{ glm::vec3(-0.5f,  0.5f, -0.5f) }, // Top Left
			{ glm::vec3(0.5f,  0.5f, -0.5f) }, // Top Right

			// Left Face (-X)
			{ glm::vec3(-0.5f, -0.5f, -0.5f) }, // Bottom Back
			{ glm::vec3(-0.5f, -0.5f,  0.5f) }, // Bottom Front
			{ glm::vec3(-0.5f,  0.5f,  0.5f) }, // Top Front

			{ glm::vec3(-0.5f, -0.5f, -0.5f) }, // Bottom Back
			{ glm::vec3(-0.5f,  0.5f,  0.5f) }, // Top Front
			{ glm::vec3(-0.5f,  0.5f, -0.5f) }, // Top Back

			// Right Face (+X)
			{ glm::vec3(0.5f, -0.5f,  0.5f) }, // Bottom Front
			{ glm::vec3(0.5f, -0.5f, -0.5f) }, // Bottom Back
			{ glm::vec3(0.5f,  0.5f, -0.5f) }, // Top Back

			{ glm::vec3(0.5f, -0.5f,  0.5f) }, // Bottom Front
			{ glm::vec3(0.5f,  0.5f, -0.5f) }, // Top Back
			{ glm::vec3(0.5f,  0.5f,  0.5f) }, // Top Front

			// Top Face (+Y)
			{ glm::vec3(-0.5f,  0.5f,  0.5f) }, // Front Left
			{ glm::vec3(0.5f,  0.5f,  0.5f) }, // Front Right
			{ glm::vec3(0.5f,  0.5f, -0.5f) }, // Back Right

			{ glm::vec3(-0.5f,  0.5f,  0.5f) }, // Front Left
			{ glm::vec3(0.5f,  0.5f, -0.5f) }, // Back Right
			{ glm::vec3(-0.5f,  0.5f, -0.5f) }, // Back Left

			// Bottom Face (-Y)
			{ glm::vec3(-0.5f, -0.5f, -0.5f) }, // Back Left
			{ glm::vec3(0.5f, -0.5f, -0.5f) }, // Back Right
			{ glm::vec3(0.5f, -0.5f,  0.5f) }, // Front Right

			{ glm::vec3(-0.5f, -0.5f, -0.5f) }, // Back Left
			{ glm::vec3(0.5f, -0.5f,  0.5f) }, // Front Right
			{ glm::vec3(-0.5f, -0.5f,  0.5f) }  // Front Left
		};

		// Define rotation parameters
		float angleDegrees = 35.0f;
		glm::vec3 axis(1.0f, 1.0f, 0.0f); // Rotate around Y-axis

		// Create the rotation matrix using GLM
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angleDegrees), axis);

		// Prepare a vector to hold rotated vertices
		std::vector<Vertex> rotatedCube;
		rotatedCube.reserve(cubeVertices.size());

		// Use std::transform with a lambda to apply rotation
		std::transform(cubeVertices.begin(), cubeVertices.end(), std::back_inserter(rotatedCube),
			[&rotationMatrix](const Vertex& vert) -> Vertex {
				glm::vec4 pos4(vert.position, 1.0f);        // Convert to 4D for matrix multiplication
				glm::vec4 rotatedPos = rotationMatrix * pos4; // Apply rotation
				return Vertex{ glm::vec3(rotatedPos) };      // Convert back to 3D
			}
		);

		uint32_t vertexBufferSize = rotatedCube.size() * sizeof(Vertex);
		BufferDescription vertexBufferDescription;
		vertexBufferDescription.memoryType = MemoryType::GpuOnly;
		vertexBufferDescription.size = vertexBufferSize;
		vertexBufferDescription.usage = BufferUsageFlags::RawBuffer;
		vertexBufferDescription.stride = sizeof(Vertex);
		m_VertexBuffer = Scoped<rhi::IBuffer>(m_Device->createBuffer(vertexBufferDescription, "VertexBuffer"));

		uploadBuffer(m_VertexBuffer.get(), 0, rotatedCube.data(), vertexBufferSize);

		ShaderResourceViewDescriptorDescription vertexBufferDescriptorDesc;
		vertexBufferDescriptorDesc.buffer.size = vertexBufferSize;
		vertexBufferDescriptorDesc.buffer.offset = 0;
		vertexBufferDescriptorDesc.type = ShaderResourceViewDescriptorType::RawBuffer;
		m_VertexBufferDescriptor = m_Device->createShaderResourceViewDescriptor(m_VertexBuffer.get(), vertexBufferDescriptorDesc, "VertexBufferDescriptor");

		//BufferDescription constantBufferWithIndicesDesc;
		//constantBufferWithIndicesDesc.memoryType = MemoryType::GpuOnly;
		//constantBufferWithIndicesDesc.size = sizeof(SceneConstant);
		//constantBufferWithIndicesDesc.usage = BufferUsageFlags::UniformBuffer;
		//constantBufferWithIndicesDesc.stride = sizeof(SceneConstant);
		//Buffer* constantBuffer = m_Device->allocate(constantBufferWithIndicesDesc, "ConstantBufferWithIndices");

		//ConstantBufferDescriptorDescription constantBufferDescriptor;
		//constantBufferDescriptor.size = sizeof(SceneConstant);
		//constantBufferDescriptor.offset = 0;
		//Descriptor* sceneConstantBufferDescriptor = m_Device->createConstantBufferDescriptor(constantBuffer, constantBufferDescriptor, "ConstantBufferWithIndicesDescriptor");
	}

	void Renderer::createRenderTarget(uint32_t renderWidth, uint32_t renderHeight)
	{
		Engine::getInstance().getEditor().ViewportResizeSignal.connect(&Renderer::onViewportResize, this);
		m_RenderTargetSize.x = renderWidth;
		m_RenderTargetSize.y = renderHeight;
		//rhi::TextureDescription textAttachDesc{};
		//textAttachDesc.usage = TextureUsageFlags::RenderTarget;

		//textAttachDesc.format = Format::R16G16B16A16_UNORM;
		//textAttachDesc.width = renderWidth;
		//textAttachDesc.height = renderHeight;
		//textAttachDesc.depth = 1;
		//m_RTColor.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Color"));
		//textAttachDesc.usage = TextureUsageFlags::DepthStencil;
		//textAttachDesc.format = Format::D24_UNORM_S8_UINT;
		//m_RTDepth.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Depth"));
	}
	void Renderer::renderFrame()
	{
		buildRenderGraph(m_OutputColorHandle, m_OutputDepthHandle);
		beginFrame();
		uploadResources();
		render();
		endFrame();
	}
	void Renderer::uploadTexture(rhi::ITexture* texture, const void* data)
	{
	}
	void Renderer::uploadBuffer(rhi::IBuffer* buffer, uint32_t offset, const void* data, uint32_t data_size)
	{
		uint32_t frame_index = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;

		StagingBufferAllocator* pAllocator = m_FrameResources[frame_index].stagingBufferAllocator.get();
		StagingBuffer staging_buffer = pAllocator->allocate(data_size);

		staging_buffer.buffer->map();
		char* dst_data = (char*)staging_buffer.buffer->getCpuAddress() + staging_buffer.offset;
		memcpy(dst_data, data, data_size);

		BufferUpload upload;
		upload.buffer = buffer;
		upload.offset = offset;
		upload.staging_buffer = staging_buffer;
		m_PendingBufferUpload.push_back(upload);
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
			m_FrameFence->wait(m_CurrenFrameFenceValue);
		}
	}

	void Renderer::copyToBackBuffer(rhi::ICommandList* commandList)
	{
		m_Swapchain->acquireNextImage();
		//commandList->textureBarrier(m_RenderTargetColor.get(), ResourceAccessFlags::Discard, ResourceAccessFlags::RenderTarget);

		RGTexture* outputImage = m_RenderGraph->getTexture(m_OutputColorHandle);
		ITexture* presentImage = m_Swapchain->getCurrentSwapchainImage();

		commandList->textureBarrier(presentImage, ResourceAccessFlags::Present, ResourceAccessFlags::TransferDst);
		commandList->copyTexture(presentImage, 0, 0, outputImage->getTexture(), 0, 0);
		commandList->textureBarrier(presentImage, ResourceAccessFlags::TransferDst, ResourceAccessFlags::RenderTarget);

		rhi::RenderPassDescription renderPass;
		renderPass.color[0].texture = presentImage;
		//renderPass.color[0].loadOp = RenderPassLoadOp::Clear;
		//renderPass.depth.texture = m_RenderTargetDepth.get();
		//renderPass.depth.loadOp = RenderPassLoadOp::DontCare;
		//renderPass.depth.stencilLoadOp = RenderPassLoadOp::DontCare;
		//renderPass.depth.storeOp = RenderPassStoreOp::DontCare;
		//renderPass.depth.stencilStoreOp = RenderPassStoreOp::DontCare;
		//renderPass.depth.readOnly = false;

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
			frame.stagingBufferAllocator = CreateScoped<StagingBufferAllocator>(this);
		}
		m_FrameFence.reset(m_Device->createFence("FrameFence"));
		m_UploadFence.reset(m_Device->createFence("UploadFence"));
	}

	void SE::Renderer::beginFrame()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];
		m_FrameFence->wait(frame.frameFenceValue);

		m_Device->beginFrame();

		rhi::ICommandList* pCommandList = frame.commandList.get();
		pCommandList->resetAllocator();
		pCommandList->begin();

		rhi::ICommandList* pComputeCommandList = frame.computeCommandList.get();
		pComputeCommandList->resetAllocator();
		pComputeCommandList->begin();
	}

	void SE::Renderer::endFrame()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];

		rhi::ICommandList* pComputeCommandList = frame.computeCommandList.get();
		pComputeCommandList->end();

		rhi::ICommandList* pCommandList = frame.commandList.get();
		pCommandList->end();

		frame.frameFenceValue = ++m_CurrenFrameFenceValue;

		pCommandList->present(m_Swapchain.get());
		pCommandList->signal(m_FrameFence.get(), m_CurrenFrameFenceValue);
		pCommandList->submit();

		m_Device->endFrame();
	}

	void SE::Renderer::uploadResources()
	{
		if (m_PendingTextureUploads.empty() && m_PendingBufferUpload.empty())
		{
			return;
		}

		uint32_t frame_index = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;

		FrameResources& currentFrame = m_FrameResources[frame_index];
		rhi::ICommandList* uploadCommandList = currentFrame.uploadCommandList.get();
		uploadCommandList->resetAllocator();
		uploadCommandList->begin();

		{
			for (size_t i = 0; i < m_PendingBufferUpload.size(); ++i)
			{
				const BufferUpload& upload = m_PendingBufferUpload[i];
				uploadCommandList->copyBuffer(upload.buffer, upload.offset,
					upload.staging_buffer.buffer, upload.staging_buffer.offset, upload.staging_buffer.size);
			}

			for (size_t i = 0; i < m_PendingTextureUploads.size(); ++i)
			{
				const TextureUpload& upload = m_PendingTextureUploads[i];
				uploadCommandList->copyBufferToTexture(upload.texture, upload.mip_level, upload.array_slice,
					upload.staging_buffer.buffer, upload.staging_buffer.offset + upload.offset);
			}
		}

		uploadCommandList->end();
		uploadCommandList->signal(m_UploadFence.get(), ++m_CurrentUploadFenceValue);
		uploadCommandList->submit();

		ICommandList* commandList = currentFrame.commandList.get();
		commandList->wait(m_UploadFence.get(), m_CurrentUploadFenceValue);

		if (m_Device->getDescription().backend == rhi::RenderBackend::Vulkan)
		{
			for (size_t i = 0; i < m_PendingTextureUploads.size(); ++i)
			{
				const TextureUpload& upload = m_PendingTextureUploads[i];
				commandList->textureBarrier(upload.texture,
					rhi::ResourceAccessFlags::TransferDst, rhi::ResourceAccessFlags::MaskShaderRead);
			}
		}

		m_PendingBufferUpload.clear();
		m_PendingTextureUploads.clear();
	}

	void Renderer::buildRenderGraph(RGHandle& color, RGHandle& depth)
	{
		m_RenderGraph->clear();

		struct ForwardPassData
		{
			RGHandle outSceneColorRT;
			//RGHandle outSceneDepthRT;
		};

		auto forward_pass = m_RenderGraph->addPass<ForwardPassData>("Forward Pass", RenderPassType::Graphics,
			[&](ForwardPassData& data, RGBuilder& builder)
			{
				RGTexture::Desc outputTexture;
				outputTexture.usage = TextureUsageFlags::RenderTarget;
				outputTexture.format = Format::R8G8B8A8_UNORM;
				outputTexture.width = m_WindowSize.x;
				outputTexture.height = m_WindowSize.y;
				outputTexture.depth = 1;
				RGHandle outputHandle = builder.create<RGTexture>(outputTexture, "OutputTexture");
				data.outSceneColorRT = builder.writeColor(0, outputHandle, 0, rhi::RenderPassLoadOp::Load);
				//data.outSceneDepthRT = builder.writeDepth(depth, 0, rhi::RenderPassLoadOp::Load);
			},
			[&](const ForwardPassData& data, ICommandList* pCommandList)
			{
				pCommandList->bindPipeline(m_DefaultPipeline.get());
				pCommandList->draw(36, 1);
			});

		color = forward_pass->outSceneColorRT;

		m_RenderGraph->present(color, ResourceAccessFlags::TransferSrc);

		m_RenderGraph->compile();
	}

	void Renderer::setupGlobalConstants(rhi::ICommandList* cmd)
	{
		SceneConstant sceneCB;
		sceneCB.vertexDataIndex = m_VertexBufferDescriptor->getDescriptorArrayIndex();
		cmd->setGraphicsConstants(2, &sceneCB, sizeof(SceneConstant));

		if (cmd->getQueueType() == rhi::CommandType::Graphics)
		{
			cmd->setGraphicsConstants(2, &sceneCB, sizeof(sceneCB));
		}
		cmd->setComputeConstants(2, &sceneCB, sizeof(sceneCB));
	}

	void SE::Renderer::render()
	{
		uint32_t frameIndex = m_Device->getFrameID() % SE_MAX_FRAMES_IN_FLIGHT;
		FrameResources& frame = m_FrameResources[frameIndex];
		ICommandList* commandList = frame.commandList.get();
		ICommandList* computeCommandList = frame.computeCommandList.get();
		m_RenderGraph->execute(this, commandList, computeCommandList);
		copyToBackBuffer(commandList);

		//m_RenderGraph->present(hBackBuffer, rhi::ResourceAccessFlags::Present);

		//m_RenderGraph->compile();

		//m_RenderGraph->execute(this, frame.commandList.get(), frame.computeCommandList.get());
		//m_RenderGraph->clear();
	}
}