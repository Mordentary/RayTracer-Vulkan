#pragma once

#include <SDL.h>
#include <functional>
#include <vector>
#include <string>
#include <sigslot\signal.hpp>
namespace SE {
	class Window {
	public:
		using EventCallbackFn = std::function<void(const SDL_Event&)>;

		Window(const std::string& title, uint32_t width, uint32_t height);
		~Window();

		void onUpdate();
		void setEventCallback(const EventCallbackFn& callback);

		void* getNativeWindow() const { return m_Window; }
		uint32_t getWidth() const { return m_Width; }
		uint32_t getHeight() const { return m_Height; }
		bool isMinimized() const { return m_Minimized; }
		bool isRunning() const { return m_Running; }
		void close() { m_Running = false; }
		sigslot::signal<uint32_t, uint32_t> WindowResizeSignal;

	private:
		void create(const std::string& title, uint32_t width, uint32_t height);
		void shutdown();

		SDL_Window* m_Window;
		uint32_t m_Width, m_Height;
		bool m_Minimized;
		bool m_Running;

		EventCallbackFn m_EventCallback;
	};
} // namespace SE