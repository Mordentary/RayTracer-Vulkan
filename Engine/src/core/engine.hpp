#pragma once

#include "SingularityEngine_export.h"
#include <unordered_map>
#include <camera.h>
#include <engine_core.h>
#include <RHI\swapchain.hpp>
#include <renderer\renderer.hpp>
#include "window.hpp"

namespace SE
{
	// Forward declarations
	struct MeshAsset;
	struct LoadedGLTF;

	// Vulkan engine class
	class Engine {
	public:

		SINGULARITY_API static Engine& getInstance() {
			static Engine instance;
			return instance;
		}
		SINGULARITY_API void init(uint32_t width, uint32_t height);
		SINGULARITY_API void run();
		SINGULARITY_API void shutdown();

		//idk
		//GPUMeshBuffers uploadMesh(const std::span<uint32_t>& indices, const std::span<Vertex>& vertices);

		//device
		//AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		//AllocatedImage createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		//void destroyImage(const AllocatedImage& image);
		//AllocatedBuffer createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		//void destroyBuffer(const AllocatedBuffer& buffer);

		//struct DefaultData
		//{
		//	AllocatedImage whiteImage;
		//	AllocatedImage blackImage;
		//	AllocatedImage greyImage;
		//	AllocatedImage errorCheckerboardImage;
		//	VkSampler samplerLinear;
		//	VkSampler samplerNearest;
		//	Shared<MaterialInstance> defaultMaterial;
		//};

		//Renderer

		//DefaultData getDefaultEngineData() { return m_DefaultEngineData; };
		//VkDescriptorSetLayout getSceneDescriptorLayout() { return m_SceneDescriptorLayout; }
		//MaterialSystem& getMaterialSystem() { return m_MaterialSystem; };
		//const AllocatedImage& getDrawImage() { return m_DrawImage; };
		//const AllocatedImage& getDepthImage() { return m_DepthImage; };
		//VmaAllocator getVmaAllocator() { return m_Allocator; };
		//SDL_Window* getWindow() { return m_Window; }
		//VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; }
		//VkDevice getDevice() { return m_Device; }
		//VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
		//const VkFormat& getSwapchainFormat() { return m_SwapchainImageFormat; }
		//VkExtent2D getSwapchainExtent() { return m_SwapchainExtent; }
		//VkImageView getDrawImageView() { return m_DrawImage.imageView; }
		//VkInstance getVkInstance() { return m_Instance; }

		//void setResizeRequest(bool request) { m_ResizeRequested = request; }

	private:
		Engine() = default;
		~Engine() = default;

		Engine(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) = delete;
		void handleEvent(const SDL_Event& event);

	private:
		Scoped<Renderer> m_Renderer;
		//World thing
		std::unordered_map<std::string, Shared<LoadedGLTF>> m_LoadedNodes;
		Shared<Camera> m_Camera;
		Scoped<Editor> m_Editor;
		Scoped<Window> m_Window;
		bool m_StopRendering = false;
	private:
	};
}