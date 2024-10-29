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

	// Minimal resources for a mesh draw command
	struct DrawCommand {
		DrawCommand(
			uint32_t inIndexCount,
			uint32_t inFirstIndex,
			VkBuffer inIndexBuffer,
			const MaterialInstance& inMaterial,
			const glm::mat4& inTransform,
			VkDeviceAddress inVertexBufferAddress
		) : indexCount(inIndexCount)
			, firstIndex(inFirstIndex)
			, indexBuffer(inIndexBuffer)
			, material(inMaterial)
			, transform(inTransform)
			, vertexBufferAddress(inVertexBufferAddress) {}

		uint32_t indexCount;
		uint32_t firstIndex;
		VkBuffer indexBuffer;
		const MaterialInstance& material;
		glm::mat4 transform;
		VkDeviceAddress vertexBufferAddress;
	};

	// Drawing context
	class RenderQueue {
	public:
		// Add objects to the queue
		void addOpaque(const DrawCommand& cmd) {
			m_OpaqueObjects.push_back(cmd);
		}

		void addTransparent(const DrawCommand& cmd) {
			m_TransparentObjects.push_back(cmd);
		}

		// Clear the queue
		void clear() {
			m_OpaqueObjects.clear();
			m_TransparentObjects.clear();
		}

		// Reserve space for expected number of objects
		void reserve(size_t opaqueCount, size_t transparentCount) {
			m_OpaqueObjects.reserve(opaqueCount);
			m_TransparentObjects.reserve(transparentCount);
		}

		// Get read-only access to queued objects
		[[nodiscard]] const std::vector<DrawCommand>& getOpaqueObjects() const {
			return m_OpaqueObjects;
		}

		[[nodiscard]] const std::vector<DrawCommand>& getTransparentObjects() const {
			return m_TransparentObjects;
		}

		// Get number of objects in each queue
		[[nodiscard]] size_t opaqueCount() const { return m_OpaqueObjects.size(); }
		[[nodiscard]] size_t transparentCount() const { return m_TransparentObjects.size(); }
		[[nodiscard]] size_t totalCount() const { return opaqueCount() + transparentCount(); }

		// Sort the queues (e.g., front-to-back for opaque, back-to-front for transparent)
		void sort() {
			sortOpaqueObjects();
			sortTransparentObjects();
		}

	private:
		std::vector<DrawCommand> m_OpaqueObjects;
		std::vector<DrawCommand> m_TransparentObjects;

		// Sort opaque objects front-to-back for better performance
		void sortOpaqueObjects() {
			// TODO: Implement depth-based sorting for opaque objects
			// This would typically sort based on distance from camera
			// to optimize for early z-testing
		}

		// Sort transparent objects back-to-front for correct blending
		void sortTransparentObjects() {
			// TODO: Implement depth-based sorting for transparent objects
			// This would typically sort based on distance from camera
			// in reverse order for proper alpha blending
		}
	};

	class MaterialSystem {
	public:
		void initialize(Engine* engine);
		void cleanup(VkDevice device);
		[[nodiscard]] VkDescriptorSet allocateMatDescriptorSet(VkDevice device) { return m_MaterialDescriptorPool.allocate(device, m_MaterialLayout); }
		[[nodiscard]] const MaterialPipeline& getOpaquePipeline() const { return m_OpaquePipeline; }
		[[nodiscard]] const MaterialPipeline& getTransparentPipeline() const { return m_TransparentPipeline; }
		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_MaterialLayout; }
		[[nodiscard]] DescriptorWriter createWriter() const { return DescriptorWriter(); }

	private:
		void buildPipelines(Engine* engine);
		void createDescriptorLayout(Engine* engine);

		DescriptorAllocator m_MaterialDescriptorPool;
		MaterialPipeline m_OpaquePipeline;
		MaterialPipeline m_TransparentPipeline;
		VkDescriptorSetLayout m_MaterialLayout{ VK_NULL_HANDLE };
	};

	// Mesh node inheriting from SceneNode
	class MeshNode final : public SceneNode {
	public:
		MeshNode() = default;
		explicit MeshNode(Shared<MeshAsset> mesh) : m_Mesh(std::move(mesh)) {}

		void setMesh(Shared<MeshAsset> mesh) {
			m_Mesh = std::move(mesh);
		}

		[[nodiscard]] const Shared<MeshAsset>& getMesh() const {
			return m_Mesh;
		}

		// Override draw to handle mesh rendering
		void draw(const glm::mat4& topTransform, RenderQueue& renderQueue) override {
			glm::mat4 nodeMatrix = topTransform * m_WorldTransform;

			// Draw mesh if we have one
			if (m_Mesh) {
				auto meshBuffer = m_Mesh->getMeshBuffers();
				for (const auto& surface : m_Mesh->getSurfaces()) {
					DrawCommand cmd{
						surface.indexCount,
						surface.startIndex,
						meshBuffer.indexBuffer.buffer,
						*surface.material.get(),
						nodeMatrix,
						meshBuffer.vertexBufferAddress
					};

					// Add to appropriate queue based on material type
					if (surface.material->passType == MaterialPass::Transparent) {
						renderQueue.addTransparent(cmd);
					}
					else {
						renderQueue.addOpaque(cmd);
					}
				}
			}

			// Draw children after mesh
			SceneNode::draw(topTransform, renderQueue);
		}

		// Optional: Override update for mesh-specific animations
		void update(float deltaTime) override {
			// Add mesh-specific updates here (e.g., skeletal animations)

			// Update children
			SceneNode::update(deltaTime);
		}

	private:
		Shared<MeshAsset> m_Mesh;
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
		VkDescriptorSet sceneDescriptorSet;
		AllocatedBuffer sceneParameterBuffer;
	};

	struct EngineStats {
		float frametime;
		int triangle_count;
		int drawcall_count;
		float scene_update_time;
		float mesh_draw_time;
	};

	// Frame overlap constant
	constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

	// Vulkan engine class
	class Engine {
	public:

		VkDevice getDevice();
		VkDescriptorSetLayout getSceneDescriptorLayout();
		MaterialSystem& getMaterialSystem() { return m_MaterialSystem; };
		const AllocatedImage& getDrawImage() { return m_DrawImage; };
		const AllocatedImage& getDepthImage() { return m_DepthImage; };
		VmaAllocator getVmaAllocator() { return m_Allocator; };
		//const MaterialFactory& getMaterialTemplate() { return m_MaterialFactory; };

		SINGULARITY_API static Engine& getInstance() {
			static Engine instance;
			return instance;
		}

		SINGULARITY_API void run();
		SINGULARITY_API void cleanup();
		void drawFrame();
		GPUMeshBuffers uploadMesh(const std::span<uint32_t>& indices, const std::span<Vertex>& vertices);
		AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		AllocatedImage createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		void destroyImage(const AllocatedImage& image);
		AllocatedBuffer createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void destroyBuffer(const AllocatedBuffer& buffer);

		struct DefaultData
		{
			AllocatedImage whiteImage;
			AllocatedImage blackImage;
			AllocatedImage greyImage;
			AllocatedImage errorCheckerboardImage;
			VkSampler samplerLinear;
			VkSampler samplerNearest;
			Shared<MaterialInstance> defaultMaterial;
		};

		DefaultData getDefaultEngineData() { return m_DefaultEngineData; };
	private:
		Engine(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) = delete;

		void init();
		Engine();
		~Engine();

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
		float m_LastWindowWidth = 0;
		float m_LastWindowHeight = 0;

		glm::vec2 m_ViewpotrSize;
		VkDescriptorSet m_BackgroundShaderDescriptorSet;
		VkDescriptorSetLayout m_BackgroundShaderDescriptorLayout;

		VkPipeline m_BackgroundPipeline;
		VkPipelineLayout m_BackgroundPipelineLayout;

		VkFence m_ImguiFence;
		VkCommandBuffer m_ImguiCommandBuffer;
		VkCommandPool m_ImguiCommandPool;

		DefaultData m_DefaultEngineData;
		RenderQueue m_MainRenderQueue;
		MaterialSystem m_MaterialSystem;

		std::unordered_map<std::string, Shared<LoadedGLTF>> m_LoadedNodes;

		Shared<Camera> m_Camera;
		EngineStats stats;
	private:
		void updateScene();

		void initScene();

		void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		// Initialization functions
		void resizeSwapchain();
		void resizeDrawImage();

		void initDefaultData();
		void initVulkan();
		void initSwapchain();
		void initCommandBuffers();
		void initDescriptors();
		void initSyncStructures();
		void initPipelines();
		void initBackgroundPipelines();
		void initImgui();

		//void handleResize();
		bool m_ShowViewport = true;
		bool m_ShowMetricsWindow = false;
		bool m_ShowStatsWindow = true;
		bool m_ShowStyleEditor = false;
		VkDescriptorSet m_ImguiDrawImageDescriptor = VK_NULL_HANDLE;  // Add this
		// Drawing functions
		void drawBackground(VkCommandBuffer cmd);
		void drawGeometry(VkCommandBuffer cmd);
		void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);

		// Resource management
		void createSwapchain(uint32_t width, uint32_t height);
		void destroySwapchain();
	};
}
