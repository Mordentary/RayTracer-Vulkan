#pragma once
#include <array>
#include <vector>
#include"core/engine.hpp"
#include "engine_core.h"
#include "render_graph/render_graph.hpp"
#include "shader_cache.hpp"
#include "staging_buffer_allocator.hpp"
#include "glm/glm.hpp"
#include "gpu_scene.hpp"
namespace SE
{
	class GpuScene;
	class RawBuffer;
	class StructuredBuffer;
	class FormattedBuffer;
	class ShaderCompiler;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void createDevice(rhi::RenderBackend backend, void* window_handle, uint32_t window_width, uint32_t window_height);
		void createRenderTarget(uint32_t renderWidth, uint32_t renderHeight);
		RawBuffer* createRawBuffer(const void* data, uint32_t size, const std::string& name, rhi::MemoryType memType, bool uav = false);
		StructuredBuffer* createStructuredBuffer(const void* data, uint32_t stride, uint32_t elementCount, const std::string& name, rhi::MemoryType memory_type = rhi::MemoryType::GpuOnly, bool uav = false);
		FormattedBuffer* createFormattedBuffer(const void* data, rhi::Format format, uint32_t elementCount, const std::string& name, rhi::MemoryType memory_type = rhi::MemoryType::GpuOnly, bool uav = false);
		uint32_t allocateSceneConstant(const void* data, uint32_t size);
		rhi::IBuffer* getSceneStaticBuffer() const;
		OffsetAllocator::Allocation allocateSceneStaticBuffer(const void* data, uint32_t size);
		void freeSceneStaticBuffer(OffsetAllocator::Allocation allocation);

		void buildRenderGraph(RGHandle& outColor, RGHandle& outDepth);
		void setupGlobalConstants(rhi::ICommandList* pCommandList);
		void renderFrame();
		rhi::IDevice* getDevice() const { return m_Device.get(); }
		ShaderCompiler* getShaderCompiler() const { return m_ShaderCompiler.get(); }
		ShaderCache* getShaderCache() const { return m_ShaderCache.get(); }
		uint64_t getFrameID() { return m_Device->getFrameID(); };
		rhi::ISwapchain* getSwapchain() const { return m_Swapchain.get(); }
		rhi::ITexture* getRenderTarget() const { return m_OutputTextureColor.get(); }
		void uploadTexture(rhi::ITexture* texture, const void* data);
		void uploadBuffer(rhi::IBuffer* buffer, uint32_t offset, const void* data, uint32_t data_size);
	private:
		Scoped<rhi::IDevice> m_Device = nullptr;
		Scoped<rhi::ISwapchain> m_Swapchain = nullptr;
		glm::vec2 m_WindowSize{};
		glm::vec2 m_RenderTargetSize{};

		Scoped<rhi::ITexture> m_OutputTextureColor{};
		Scoped<rhi::ITexture> m_OutputTextureDepth{};
		RGHandle m_OutputColorHandle;
		RGHandle m_OutputDepthHandle;

		Scoped<rhi::IPipelineState> m_DefaultPipeline;
		Scoped<RenderGraph> m_RenderGraph;

		rhi::IShader* m_TestShaderVS;
		rhi::IShader* m_TestShaderPS;

		Scoped<StructuredBuffer> m_VertexBuffer;

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

		Scoped<ShaderCompiler> m_ShaderCompiler;
		Scoped<ShaderCache> m_ShaderCache;
		Scoped<GpuScene> m_GpuScene;

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