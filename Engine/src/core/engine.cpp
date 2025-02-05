#include "engine.hpp"
#include "renderer/renderer.hpp"
#include "Editor.hpp"
#include <rpmalloc.h>

namespace SE
{
	// Initialization
	void Engine::create(uint32_t widthWin, uint32_t heightWin)
	{
		m_Window = createScoped<Window>("Singularity Engine", widthWin, heightWin);
		m_Window->setEventCallback([this](const SDL_Event& event, const Uint8* state) {
			handleEvent(event, state);
			});

		m_AssetPath = "assets/";
		m_ShaderPath = "shaders/";
		m_Renderer = createScoped<Renderer>();
		m_Renderer->createDevice(rhi::RenderBackend::Vulkan, m_Window->getNativeWindow(), widthWin, heightWin);

		//World
		m_Editor = createScoped<Editor>(this);
		m_Editor->create();
		const Editor::ViewportState* state = m_Editor->getViewportState();
		m_Camera = createScoped<Camera>(state, (SDL_Window*)m_Window->getNativeWindow(), glm::vec3(0, 0, 3));
		m_Renderer->createRenderTarget(m_Window->getHeight(), m_Window->getHeight());

		Timer::getInstance().reset();
	}

	void Engine::run() {
		bool bQuit = false;
		SDL_Event e;
		auto lastFrame = std::chrono::high_resolution_clock::now();

		while (m_Window->isRunning()) {
			Timer::getInstance().beginFrame();
			float deltaTime = Timer::getInstance().getDeltaTime();

			if (m_StopRendering || m_Window->isMinimized()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			m_Window->onUpdate();
			m_Editor->update();
			m_Camera->update(deltaTime);
			// Update and render
			m_Renderer->renderFrame();

			Timer::getInstance().endFrame();
		}
	}
	void Engine::shutdown()
	{
		m_Camera.reset();
		m_Editor.reset();
		m_Renderer.reset();
		m_Window.reset();
	}
	void Engine::handleEvent(const SDL_Event& event, const Uint8* state)
	{
		m_Editor->handleEvent(event);
		m_Camera->handleEvent(event);
		m_Camera->processKeyboard(state);
	}
}