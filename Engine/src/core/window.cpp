#include "window.hpp"

namespace windows
{
#define NOMINMAX
#include<Windows.h>
}

namespace SE {
	Window::Window(const std::string& title, uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height), m_Minimized(false), m_Running(true) {
		init(title, width, height);
	}

	Window::~Window() {
		shutdown();
	}

	void Window::init(const std::string& title, uint32_t width, uint32_t height) {
		windows::SetProcessDPIAware();
		int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		SE_ASSERT_MSG((err == 0), "SDL INIT FAILED");

		SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
			SDL_WINDOW_ALLOW_HIGHDPI);

		m_Window = SDL_CreateWindow(
			title.c_str(),
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			width,
			height,
			windowFlags);

		SE_ASSERT(m_Window, "Window is null!");
	}

	void Window::shutdown() {
		SDL_DestroyWindow(m_Window);
		SDL_Quit();
	}

	void Window::onUpdate() {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			// Handle window-specific events internally
			if (event.type == SDL_QUIT) {
				m_Running = false;
			}
			else if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					m_Width = event.window.data1;
					m_Height = event.window.data2;
					// Emit window resize signal
					WindowResizeSignal(m_Width, m_Height);
					break;

				case SDL_WINDOWEVENT_MINIMIZED:
					m_Minimized = true;
					break;

				case SDL_WINDOWEVENT_RESTORED:
					m_Minimized = false;
					break;
				}
			}
			// Call the general event callback
			if (m_EventCallback) {
				m_EventCallback(event);
			}
		}
	}
	void Window::setEventCallback(const EventCallbackFn& callback) {
		m_EventCallback = callback;
	}
}