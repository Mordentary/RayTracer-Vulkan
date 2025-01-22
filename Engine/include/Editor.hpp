// editor.h
#pragma once

#include"rhi/vulkan/vulkan_core.hpp"
#include <SDL.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include"RHI/command_list.hpp"
#include <sigslot/signal.hpp>
namespace SE {
	class Engine;

	class Editor {
	public:
		struct ViewportState {
			glm::vec2 renderTargetSize{ 1280.0f, 720.0f };
			glm::vec2 position{ 0.0f, 0.0f };
			glm::vec2 viewportCenter{};
			glm::vec2 viewportSize{};
			float resizeThreshold = 1.0f;
			bool isHovered = false;
		};
	public:
		explicit Editor(Engine* engine);
		~Editor();

		// Core functions
		void create();
		void beginFrame();
		void endFrame();
		void render(rhi::ICommandList* cmd);
		void update();
		void handleEvent(const SDL_Event& event);
		sigslot::signal<uint32_t, uint32_t> ViewportResizeSignal;

		// Getters
		bool isViewportHovered() const { return m_Viewport.isHovered; }
		const glm::vec2& getViewportSize() const { return m_Viewport.renderTargetSize; }
		const ViewportState* getViewportState() { return &m_Viewport; }
		void updateViewportImage(VkImageView newImageView);

	private:
		void shutdown();

		// Initialize helpers
		void createDescriptorPool();
		void initImGuiStyle();
		void createViewportResources();

		// Cleanup helpers
		void destroyViewportResources();

		// UI Drawing functions
		void setupDockspace();
		void drawMainMenu();
		void drawViewport();
		void drawDebugWindows();

		// Viewport management
		void handleViewportResize(const ImVec2& newSize);
		bool checkViewportResize(const ImVec2& currentSize);

		// Engine reference
		Engine* m_Engine;

		// Vulkan resources
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet m_ViewportImageDescriptor = VK_NULL_HANDLE;
		VkSampler m_ViewportSampler = VK_NULL_HANDLE;
		VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;

		// Window state
		bool m_ShowDockspace = true;
		bool m_ShowViewport = true;
		bool m_ShowMetricsWindow = false;
		bool m_ShowStyleEditor = false;
		bool m_ShowStatsWindow = false;
		bool m_ShowDebugWindow = true;

		// ImGui state
		ImGuiID m_DockspaceID = 0;
		ViewportState m_Viewport;
	};
}