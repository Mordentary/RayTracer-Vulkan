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
#include <glm\gtx\norm.hpp>
#include <filesystem>
#include <RHI\rhi.hpp>

namespace SE
{
	// Forward declarations
	struct MeshAsset;
	struct LoadedGLTF;

	class Editor;
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

		[[nodiscard]] const DrawCommand& getOpaqueObject(size_t index) const {
			return m_OpaqueObjects[m_OpaqueSortIndices[index]];
		}

		[[nodiscard]] const DrawCommand& getTransparentObject(size_t index) const {
			return m_TransparentObjects[m_TransparentSortIndices[index]];
		}
		// Get number of objects in each queue
		[[nodiscard]] size_t opaqueCount() const { return m_OpaqueObjects.size(); }
		[[nodiscard]] size_t transparentCount() const { return m_TransparentObjects.size(); }
		[[nodiscard]] size_t totalCount() const { return opaqueCount() + transparentCount(); }

		void sort(const glm::vec3& cameraPosition) {
			if (!m_OpaqueObjects.empty()) {
				sortOpaqueObjects();
			}

			if (!m_TransparentObjects.empty()) {
				sortTransparentObjects(cameraPosition);
			}
		}

	private:
		std::vector<DrawCommand> m_OpaqueObjects;
		std::vector<DrawCommand> m_TransparentObjects;
		std::vector<uint32_t> m_OpaqueSortIndices;
		std::vector<uint32_t> m_TransparentSortIndices;

		struct SortKeyIndex {
			uint64_t key;      // 44 bits for material/mesh hash, 20 bits for draw index
			uint32_t index;    // Original array index

			bool operator<(const SortKeyIndex& other) const {
				return key < other.key;
			}
		};

		void sortOpaqueObjects() {
			if (m_OpaqueObjects.empty()) return;

			// Initialize indices vector
			m_OpaqueSortIndices.resize(m_OpaqueObjects.size());
			std::vector<SortKeyIndex> sortKeys;
			sortKeys.resize(m_OpaqueObjects.size());

			// Generate sort keys for each object
			for (uint32_t i = 0; i < m_OpaqueObjects.size(); i++) {
				const auto& cmd = m_OpaqueObjects[i];

				// Create hash from material and buffer pointers for upper 44 bits
				uint64_t materialPart = reinterpret_cast<uint64_t>(cmd.material.pipeline);
				uint64_t bufferPart = reinterpret_cast<uint64_t>(cmd.indexBuffer);
				uint64_t hashPart = (materialPart ^ bufferPart) & 0xFFFFFFFFFFF;

				// Combine hash and index into sort key
				sortKeys[i].key = (hashPart << 20) | (i & 0xFFFFF);
				sortKeys[i].index = i;
			}

			// Sort based on keys
			std::sort(sortKeys.begin(), sortKeys.end());

			// Store sorted indices
			for (uint32_t i = 0; i < sortKeys.size(); i++) {
				m_OpaqueSortIndices[i] = sortKeys[i].index;
			}
		}

		void sortTransparentObjects(const glm::vec3& cameraPosition) {
			if (m_TransparentObjects.empty()) return;

			m_TransparentSortIndices.resize(m_TransparentObjects.size());
			for (uint32_t i = 0; i < m_TransparentObjects.size(); i++) {
				m_TransparentSortIndices[i] = i;
			}

			// Sort indices based on distance from camera
			std::sort(m_TransparentSortIndices.begin(), m_TransparentSortIndices.end(),
				[this, &cameraPosition](uint32_t a, uint32_t b) {
					const DrawCommand& cmdA = m_TransparentObjects[a];
					const DrawCommand& cmdB = m_TransparentObjects[b];

					glm::vec3 posA(cmdA.transform[3]);
					glm::vec3 posB(cmdB.transform[3]);

					float distA = glm::distance2(posA, cameraPosition);
					float distB = glm::distance2(posB, cameraPosition);

					return distA > distB; // Back-to-front ordering
				});
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

	// Frame overlap constant
	constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

	// Vulkan engine class
	class Engine {
	public:

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
		VkDescriptorSetLayout getSceneDescriptorLayout() { return m_SceneDescriptorLayout; }
		MaterialSystem& getMaterialSystem() { return m_MaterialSystem; };
		const AllocatedImage& getDrawImage() { return m_DrawImage; };
		const AllocatedImage& getDepthImage() { return m_DepthImage; };
		VmaAllocator getVmaAllocator() { return m_Allocator; };
		SDL_Window* getWindow() { return m_Window; }
		VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; }
		VkDevice getDevice() { return m_Device; }
		VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
		const VkFormat& getSwapchainFormat() { return m_SwapchainImageFormat; }
		VkExtent2D getSwapchainExtent() { return m_SwapchainExtent; }
		VkImageView getDrawImageView() { return m_DrawImage.imageView; }
		VkInstance getVkInstance() { return m_Instance; }

		void setResizeRequest(bool request) { m_ResizeRequested = request; }

	private:
		Engine(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) = delete;
		Engine();
		~Engine();
		void init();

		bool m_IsInitialized = false;
		bool m_ResizeRequested = false;
		int m_CurrentFrame = 0;

		float m_RenderScale = 1.0f;
		bool m_StopRendering = false;

		GPUSceneData m_SceneData;
		VkDescriptorSetLayout m_SceneDescriptorLayout;

		VkExtent2D m_WindowExtent = { .width = 2560, .height = 1440 };
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

		VkFence m_ImmediateSubmitCmdFence;
		VkCommandBuffer m_ImmediateSubmitCmdBuffer;
		VkCommandPool m_ImmediateSubmitCmdPool;

		DefaultData m_DefaultEngineData;
		RenderQueue m_MainRenderQueue;
		MaterialSystem m_MaterialSystem;

		std::unordered_map<std::string, Shared<LoadedGLTF>> m_LoadedNodes;

		Shared<Camera> m_Camera;
		Scoped<Editor> m_Editor;

	private:
		void updateScene();

		void initScene();

		void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		// Initialization functions
		void resizeResources();
		void resizeDrawImage(glm::vec2 viewportSize);

		void initDefaultData();
		void initVulkan();
		void initSwapchain();
		void initCommandBuffers();
		void initDescriptors();
		void initSyncStructures();
		void initPipelines();
		void initBackgroundPipelines();

		// Drawing functions
		void drawBackground(VkCommandBuffer cmd);
		void drawGeometry(VkCommandBuffer cmd);
		//void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);

		// Resource management
		void createSwapchain(uint32_t width, uint32_t height);
		void destroySwapchain();
	};
}
