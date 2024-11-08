#pragma once

#include "SingularityEngine_export.h"
#include <unordered_map>
#include <camera.h>
#include <engine_core.h>
#include <RHI\swapchain.hpp>
#include <renderer\renderer.hpp>
#include "window.hpp"
#include "string"
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

		const Window& getWindow() const { return *m_Window; }
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

		std::unordered_map<std::string, Shared<LoadedGLTF>> m_LoadedNodes;
		Shared<Camera> m_Camera;
		Scoped<Editor> m_Editor;
		Scoped<Window> m_Window;
		bool m_StopRendering = false;

		std::string m_AssetPath;
		std::string m_ShaderPath;
	private:
	};
}