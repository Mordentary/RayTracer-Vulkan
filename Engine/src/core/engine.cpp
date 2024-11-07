#include "engine.hpp"
#include "engine.hpp"
#include "engine.hpp"

#include "Editor.hpp"

#include <sstream>
#include <string>
#include <regex>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <imgui_impl_vulkan.h>

namespace windows
{
#define NOMINMAX
#include<Windows.h>
}

namespace SE
{
	// Initialization
	void Engine::init(uint32_t widthWin, uint32_t heightWin)
	{
		m_Window = CreateScoped<Window>("Singularity Engine", widthWin, heightWin);
		m_Window->setEventCallback([this](const SDL_Event& event) {
			handleEvent(event);
			});

		m_Renderer = Scoped<Renderer>();
		m_Renderer->createDevice(rhi::RenderBackend::Vulkan, m_Window->getNativeWindow(), widthWin, heightWin);

		//World
		//m_Editor = CreateScoped<Editor>(this);
		//m_Editor->init();
		//m_Camera = CreateShared<Camera>(m_Editor->getViewportState(), window);
		Timer::getInstance().reset();
	}

	void Engine::run() {
		bool bQuit = false;
		SDL_Event e;
		auto lastFrame = std::chrono::high_resolution_clock::now();

		while (m_Window->isRunning()) {
			Timer::getInstance().beginFrame();
			float deltaTime = Timer::getInstance().getDeltaTime();

			m_Window->onUpdate();

			if (m_StopRendering || m_Window->isMinimized()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			//m_Camera->update(deltaTime);
			// Update and render
			m_Renderer->renderFrame();

			Timer::getInstance().endFrame();
		}
	}
	void Engine::shutdown()
	{
		m_Renderer.reset();
		m_Camera.reset();
		m_Editor.reset();
		m_Window.reset();
	}
	void Engine::handleEvent(const SDL_Event& event)
	{
		//m_Camera->handleEvent(event);
		//m_Editor->handleEvent(event);
	}
}