#pragma once

#include "SingularityEngine_export.h" 
#include "vk_descriptors.h" 
#include "vk_types.h"
#include "vk_loader.h"
#include "camera.h"
#include "engine_core.h"

#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>
#include <optional>
#include <array>



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <filesystem>


namespace SE
{
	// Forward declarations
	struct MeshAsset;
	struct LoadedGLTF;

	class Engine;
	// Resource cleanup queue
	struct ResourceCleanupQueue
	{
		std::deque<std::function<void()>> cleanupFunctions;

		void enqueueCleanup(std::function<void()>&& function) {
			cleanupFunctions.push_back(std::move(function));
		}

		void executeCleanup() {
			// Execute all cleanup functions in reverse order
			for (auto it = cleanupFunctions.rbegin(); it != cleanupFunctions.rend(); ++it) {
				(*it)();
			}
			cleanupFunctions.clear();
		}
	};

	// Renderable object structure
	struct DrawCommand {
		uint32_t indexCount;
		uint32_t firstIndex;
		VkBuffer indexBuffer;

		MaterialInstance* material;

		glm::mat4 transform;
		VkDeviceAddress vertexBufferAddress;
	};

	// Drawing context
	struct DrawingContext {
		std::vector<DrawCommand> opaqueObjects;
	};

	// Mesh node inheriting from SceneNode
	struct MeshNode : public SceneNode {
		Shared<MeshAsset> meshAsset;

		virtual void draw(const glm::mat4& parentTransform, DrawingContext& context) override;
	};

	// Compute push constants
	struct ComputePushConstants
	{
		glm::vec4 data1;
		glm::vec4 data2;
		glm::vec4 data3;
		glm::vec4 data4;
	};

	// Compute effect structure
	struct ComputeEffect {
		std::string name;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		ComputePushConstants pushConstants;
	};

	// Frame-specific data
	struct FrameData
	{
		VkCommandPool commandPool;
		VkCommandBuffer mainCommandBuffer;
		VkSemaphore swapchainSemaphore;
		VkSemaphore renderSemaphore;
		DescriptorAllocator descriptorAllocator;
		VkFence renderFence;
		ResourceCleanupQueue cleanupQueue;
	};

	// Metallic-Roughness material pipeline
	struct MetallicRoughnessMaterial {
		MaterialPipeline opaquePipeline;
		MaterialPipeline transparentPipeline;
		VkDescriptorSetLayout materialDescriptorLayout;

		struct MaterialConstants {
			glm::vec4 colorFactors;
			glm::vec4 metalRoughFactors;
			// Padding for uniform buffers
			glm::vec4 padding[14];
		};

		struct MaterialResources {
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		DescriptorWriter descriptorWriter;

		void buildPipeline(Engine* engine);
		void clearResources(VkDevice device);

		MaterialInstance createMaterial(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocator& allocator);
	};

	// Frame overlap constant
	constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

	// Vulkan engine class
	class Engine {
		friend MetallicRoughnessMaterial;
		friend std::optional<Shared<LoadedGLTF>> loadGltfMeshes(Engine* engine, std::filesystem::path filePath);
		friend std::optional<AllocatedImage> loadImage(Engine* engine, fastgltf::Asset& asset, fastgltf::Image& image);
		friend LoadedGLTF;
	public:
		static Engine& GetInstance();

		SINGULARITY_API void init();
		SINGULARITY_API void run();
		SINGULARITY_API void cleanup();
		void drawFrame();
		GPUMeshBuffers uploadMesh(const std::span<uint32_t>& indices, const std::span<Vertex>& vertices);

	private:
		bool m_IsInitialized = false;
		bool m_ResizeRequested = false;
		int m_CurrentFrame = 0;

		float m_RenderScale = 1.0f;
		bool m_StopRendering = false;

		GPUSceneData m_SceneData;
		VkDescriptorSetLayout m_SceneDescriptorLayout;

		VkExtent2D m_WindowExtent = { .width = 1920, .height = 1080 };
		struct SDL_Window* m_Window = nullptr;

		std::vector<ComputeEffect> m_BackgroundEffects;
		int m_ActiveBackgroundEffect = 0;

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;

		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkExtent2D m_SwapchainExtent;

		FrameData& getCurrentFrame() { return m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT]; }
		FrameData m_Frames[MAX_FRAMES_IN_FLIGHT];
		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamilyIndex;

		ResourceCleanupQueue m_MainCleanupQueue;
		DescriptorAllocator m_GlobalDescriptorAllocator;

		VmaAllocator m_Allocator;
		AllocatedImage m_DrawImage;
		AllocatedImage m_DepthImage;
		VkExtent2D m_DrawExtent;

		VkDescriptorSet m_BackgroundShaderDescriptorSet;
		VkDescriptorSetLayout m_BackgroundShaderDescriptorLayout;

		VkPipeline m_BackgroundPipeline;
		VkPipelineLayout m_BackgroundPipelineLayout;

		VkFence m_ImguiFence;
		VkCommandBuffer m_ImguiCommandBuffer;
		VkCommandPool m_ImguiCommandPool;

		AllocatedImage m_WhiteImage;
		AllocatedImage m_BlackImage;
		AllocatedImage m_GreyImage;
		AllocatedImage m_ErrorCheckerboardImage;

		VkSampler m_SamplerLinear;
		VkSampler m_SamplerNearest;

		MaterialInstance m_DefaultMaterial;
		MetallicRoughnessMaterial m_MetallicRoughnessMaterial;

		DrawingContext m_MainDrawContext;
		std::unordered_map<std::string, Shared<LoadedGLTF>> m_LoadedNodes;


		Shared<Camera> m_Camera;

	private:
		void updateScene();
		void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		// Initialization functions
		void resizeSwapchain();
		void initDefaultData();
		void initVulkan();
		void initSwapchain();
		void initCommandBuffers();
		void initDescriptors();
		void initSyncStructures();
		void initPipelines();
		void initBackgroundPipelines();
		void initImgui();

		// Drawing functions
		void drawBackground(VkCommandBuffer cmd);
		void drawGeometry(VkCommandBuffer cmd);
		void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);

		// Resource management
		AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		AllocatedImage createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		void destroyImage(const AllocatedImage& image);
		AllocatedBuffer createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void destroyBuffer(const AllocatedBuffer& buffer);
		void createSwapchain(uint32_t width, uint32_t height);
		void destroySwapchain();
	};
}
