#include "renderer.hpp"
#include"shader_compiler.hpp"
#include "render_graph/render_graph_builder.hpp"
#include "resources/raw_buffer.hpp"
#include "resources/formatted_buffer.hpp"
#include "resources/structured_buffer.hpp"
#include "resources/index_buffer.hpp"
#include "gpu_scene.hlsli"
#include"global_constants.hlsli"
using namespace rhi;
namespace SE
{
	Renderer::Renderer()
	{
		Engine::getInstance().getWindow().WindowResizeSignal.connect(&Renderer::onWindowResize, this);
		m_ShaderCache = createScoped<ShaderCache>(this);
		m_ShaderCompiler = createScoped<ShaderCompiler>(this);
	}

	Renderer::~Renderer()
	{
		if (m_RenderGraph)
		{
			m_RenderGraph->clear();
		}
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
		SE_ASSERT(device.get(), "Device creation is failed");
		m_Device = std::move(device);
		SE_ASSERT(m_Device.get(), "Device is null");
		m_GpuScene = createScoped<GpuScene>(this);

		rhi::SwapchainDescription swapchainDesc;
		swapchainDesc.windowHandle = window_handle;
		swapchainDesc.width = window_width;
		swapchainDesc.height = window_height;
		swapchainDesc.bufferCount = 3;
		swapchainDesc.format = rhi::Format::R8G8B8A8_UNORM;
		swapchainDesc.vsync = true;

		m_Swapchain.reset(m_Device->createSwapchain(swapchainDesc, "MainSwapchain"));
		m_RenderGraph = createScoped<RenderGraph>(this);

		initFrameResources();

		m_TestShaderVS = m_ShaderCache->getShader("defaultShader.hlsl", "VSMain", ShaderType::Vertex, {});
		m_TestShaderPS = m_ShaderCache->getShader("defaultShader.hlsl", "PSMain", ShaderType::Pixel, {});

		DepthStencil depthInfo;
		depthInfo.depthTest = true;
		depthInfo.depthFunction = CompareFunction::GreaterEqual;

		GraphicsPipelineDescription pipeDesc{};
		pipeDesc.vertexShader = m_TestShaderVS;
		pipeDesc.pixelShader = m_TestShaderPS;
		pipeDesc.renderTargetFormat[0] = Format::R8G8B8A8_UNORM;
		pipeDesc.depthStencilFormat = Format::D32_SFLOAT;
		pipeDesc.depthStencil = depthInfo;
		m_DefaultPipeline = Scoped<rhi::IPipelineState>(m_Device->createGraphicsPipelineState(pipeDesc, "TestGraphicsPipeline"));

		std::vector<glm::vec3> cubeVertices =
		{
			glm::vec3(-0.5f, -0.5f, -0.5f), // Vertex 0
			glm::vec3(0.5f, -0.5f, -0.5f), // Vertex 1
			glm::vec3(-0.5f,  0.5f, -0.5f), // Vertex 2
			glm::vec3(0.5f,  0.5f, -0.5f), // Vertex 3
			glm::vec3(-0.5f, -0.5f,  0.5f), // Vertex 4
			glm::vec3(0.5f, -0.5f,  0.5f), // Vertex 5
			glm::vec3(-0.5f,  0.5f,  0.5f), // Vertex 6
			glm::vec3(0.5f,  0.5f,  0.5f)  // Vertex 7
		};

		std::vector<uint32_t> indices =
		{
			// Front Face (+Z): vertices 4, 5, 7, 6
			4, 5, 7,  // Triangle 1
			4, 7, 6,  // Triangle 2

			// Back Face (-Z): vertices 0, 1, 3, 2
			1, 0, 2,  // Triangle 3
			1, 2, 3,  // Triangle 4

			// Left Face (-X): vertices 0, 4, 6, 2
			0, 4, 6,  // Triangle 5
			0, 6, 2,  // Triangle 6

			// Right Face (+X): vertices 5, 1, 3, 7
			5, 1, 3,  // Triangle 7
			5, 3, 7,  // Triangle 8

			// Top Face (+Y): vertices 6, 7, 3, 2
			6, 7, 3,  // Triangle 9
			6, 3, 2,  // Triangle 10

			// Bottom Face (-Y): vertices 0, 1, 5, 4
			0, 1, 5,  // Triangle 11
			0, 5, 4   // Triangle 12
		};

		float angleDegrees = 45.0f;
		glm::vec3 axis(0.0f, 1.0f, 0.0f); // Rotate around Y-axis

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angleDegrees), axis);

		std::vector<glm::vec3> rotatedCube;
		rotatedCube.reserve(cubeVertices.size());

		std::transform(cubeVertices.begin(), cubeVertices.end(), std::back_inserter(rotatedCube),
			[&rotationMatrix](const glm::vec3& vertPos) -> glm::vec3 {
				glm::vec4 pos4(vertPos.x, vertPos.y, vertPos.z, 1.0f);
				glm::vec4 rotatedPos = rotationMatrix * pos4;
				return glm::vec3(rotatedPos.x, rotatedPos.y, rotatedPos.z);
			}
		);

		InstanceData data;
		data.posBufferAddress = allocateSceneStaticBuffer(rotatedCube.data(), rotatedCube.size() * sizeof(float3)).offset;
		data.indexBufferAddress = allocateSceneStaticBuffer(indices.data(), sizeof(uint32_t) * indices.size()).offset;
		m_GpuScene->addInstance(data);
	}

	rhi::IBuffer* Renderer::getSceneStaticBuffer() const
	{
		return m_GpuScene->getSceneStaticBuffer();
	}

	OffsetAllocator::Allocation Renderer::allocateSceneStaticBuffer(const void* data, uint32_t size)
	{
		OffsetAllocator::Allocation allocation = m_GpuScene->allocateStaticBuffer(size);

		if (data)
		{
			uploadBuffer(m_GpuScene->getSceneStaticBuffer(), allocation.offset, data, size);
		}

		return allocation;
	}

	void Renderer::freeSceneStaticBuffer(OffsetAllocator::Allocation allocation)
	{
		m_GpuScene->freeStaticBuffer(allocation);
	}

	uint32_t Renderer::allocateSceneConstant(const void* data, uint32_t size)
	{
		uint32_t address = m_GpuScene->allocateConstantBuffer(size);

		if (data)
		{
			void* dst = (char*)m_GpuScene->getSceneConstantBuffer()->getCpuAddress() + address;
			memcpy(dst, data, size);
		}

		return address;
	}

	void Renderer::createRenderTarget(uint32_t renderWidth, uint32_t renderHeight)
	{
		//Engine::getInstance().getEditor().ViewportResizeSignal.connect(&Renderer::onViewportResize, this);
		m_RenderTargetSize.x = renderWidth;
		m_RenderTargetSize.y = renderHeight;
		rhi::TextureDescription textAttachDesc{};
		textAttachDesc.usage = TextureUsageFlags::RenderTarget;
		textAttachDesc.format = Format::R8G8B8A8_UNORM;

		//todo: remove hardcode
		textAttachDesc.width = m_WindowSize.x;
		textAttachDesc.height = m_WindowSize.y;
		textAttachDesc.depth = 1;

		m_OutputTextureColor.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Color"));
		textAttachDesc.usage = TextureUsageFlags::DepthStencil;
		textAttachDesc.format = Format::D32_SFLOAT;
		m_OutputTextureDepth.reset(m_Device->createTexture(textAttachDesc, "MainRenderTarget:Depth"));
	}

	RawBuffer* Renderer::createRawBuffer(const void* data, uint32_t size, const std::string& name, rhi::MemoryType memType, bool uav)
	{
		RawBuffer* buffer = new RawBuffer(name);
		if (!buffer->create(size, memType, uav))
		{
			delete buffer;
			return nullptr;
		}

		if (data)
		{
			uploadBuffer(buffer->getBuffer(), 0, data, size);
		}

		return buffer;
	}

	IndexBuffer* Renderer::createIndexBuffer(const void* data, uint32_t stride, uint32_t elementsCount, const std::string& name, rhi::MemoryType memType)
	{
		IndexBuffer* buffer = new IndexBuffer(name);
		if (!buffer->create(stride, elementsCount, memType))
		{
			delete buffer;
			return nullptr;
		}

		if (data)
		{
			uploadBuffer(buffer->getBuffer(), 0, data, stride * elementsCount);
		}

		return buffer;
	}

	StructuredBuffer* Renderer::createStructuredBuffer(const void* data, uint32_t stride, uint32_t elementCount, const std::string& name, rhi::MemoryType memType, bool uav)
	{
		StructuredBuffer* buffer = new StructuredBuffer(name);
		if (!buffer->create(stride, elementCount, memType, uav))
		{
			delete buffer;
			return nullptr;
		}

		if (data)
		{
			uploadBuffer(buffer->getBuffer(), 0, data, stride * elementCount);
		}

		return buffer;
	}

	FormattedBuffer* Renderer::createFormattedBuffer(const void* data, rhi::Format format, uint32_t elementCount, const std::string& name, rhi::MemoryType memType, bool uav)
	{
		FormattedBuffer* buffer = new FormattedBuffer(name);
		if (!buffer->create(format, elementCount, memType, uav))
		{
			delete buffer;
			return nullptr;
		}

		if (data)
		{
			uploadBuffer(buffer->getBuffer(), 0, data, rhi::getFormatRowPitch(format, 1) * elementCount);
		}

		return buffer;
	}

	void Renderer::renderFrame()
	{
		m_GpuScene->update();
		buildRenderGraph(m_OutputColorHandle, m_OutputDepthHandle);
		beginFrame();
		uploadResources();
		render();
		endFrame();
		m_GpuScene->resetFrameData();
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
		createRenderTarget(width, height);
	}
	void SE::Renderer::onViewportResize(uint32_t width, uint32_t height)
	{
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

		RGTexture* colorImage = m_RenderGraph->getTexture(m_OutputColorHandle);
		ITexture* presentImage = m_Swapchain->getCurrentSwapchainImage();

		commandList->textureBarrier(presentImage, ResourceAccessFlags::Present, ResourceAccessFlags::TransferDst);
		commandList->copyTexture(presentImage, 0, 0, colorImage->getTexture(), 0, 0);
		commandList->textureBarrier(presentImage, ResourceAccessFlags::TransferDst, ResourceAccessFlags::RenderTarget);
		//for the next frame
		commandList->textureBarrier(colorImage->getTexture(), ResourceAccessFlags::TransferSrc, ResourceAccessFlags::RenderTarget);
		rhi::RenderPassDescription renderPass;
		renderPass.color[0].texture = presentImage;
		renderPass.color[0].loadOp = RenderPassLoadOp::Load;

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
			frame.stagingBufferAllocator = createScoped<StagingBufferAllocator>(this);
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
			RGHandle outSceneDepthRT;
		};

		m_OutputColorHandle = m_RenderGraph->import(m_OutputTextureColor.get(), rhi::ResourceAccessFlags::RenderTarget);
		m_OutputDepthHandle = m_RenderGraph->import(m_OutputTextureDepth.get(), rhi::ResourceAccessFlags::MaskDepthStencilAccess);

		auto forward_pass = m_RenderGraph->addPass<ForwardPassData>("Forward Pass", RenderPassType::Graphics,
			[&](ForwardPassData& data, RGBuilder& builder)
			{
				data.outSceneColorRT = builder.writeColor(0, m_OutputColorHandle, 0, rhi::RenderPassLoadOp::Clear);
				data.outSceneDepthRT = builder.writeDepth(m_OutputDepthHandle, 0, rhi::RenderPassLoadOp::Clear);
			},
			[&](const ForwardPassData& data, ICommandList* pCommandList)
			{
				pCommandList->bindPipeline(m_DefaultPipeline.get());
				pCommandList->draw(36, 1);
			});

		color = forward_pass->outSceneColorRT;
		depth = forward_pass->outSceneDepthRT;

		m_RenderGraph->present(color, ResourceAccessFlags::TransferSrc);
		m_RenderGraph->present(depth, ResourceAccessFlags::MaskDepthStencilAccess);

		m_RenderGraph->compile();
	}

	void Renderer::setupGlobalConstants(rhi::ICommandList* cmd)
	{
		SceneConstant sceneCB;

		CameraConstant camera_cb;
		camera_cb.viewProjection = (glmMat4ToHlslpp(Engine::getInstance().getCamera().getViewProjectionMatrix()));
		//camera_cb.view = float4x4();
		//camera_cb.projection = float4x4();
		sceneCB.cameraCB = camera_cb;
		sceneCB.instanceDataAddress = m_GpuScene->getInstanceAddress();
		sceneCB.sceneStaticBufferSRV = m_GpuScene->getSceneStaticBufferSRV()->getDescriptorArrayIndex();
		sceneCB.sceneConstantBufferSRV = m_GpuScene->getSceneConstantSRV()->getDescriptorArrayIndex();;

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
	}
}