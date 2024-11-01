// editor.cpp
#include "Editor.hpp"
#include "vk_engine.h"
#include "vk_initializers.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

namespace SE {
	Editor::Editor(Engine* engine) : m_Engine(engine)
	{
	}

	Editor::~Editor() {
		shutdown();
	}

	void Editor::init() {
		createDescriptorPool();

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// Enable docking and viewports
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = false;

		initImGuiStyle();

		// Initialize platform/renderer backends
		ImGui_ImplSDL2_InitForVulkan(m_Engine->getWindow());
		VkFormat swapchainFormat = m_Engine->getSwapchainFormat();
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = m_Engine->getVkInstance();
		init_info.PhysicalDevice = m_Engine->getPhysicalDevice();
		init_info.Device = m_Engine->getDevice();
		init_info.Queue = m_Engine->getGraphicsQueue();
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_Engine->getSwapchainFormat();

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();

		createViewportResources();
	}

	void Editor::createDescriptorPool() {
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK(vkCreateDescriptorPool(m_Engine->getDevice(), &pool_info, nullptr, &m_DescriptorPool));
	}

	void Editor::initImGuiStyle() {
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 5.0f;      // Rounded corners
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.WindowBorderSize = 1.0f;    // Visible borders
		style.FrameBorderSize = 1.0f;
		style.WindowPadding = ImVec2(8, 8);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.Colors[ImGuiCol_WindowBg].w = 0.9f;  // Slightly transparent windows
		}
	}

	void Editor::createViewportResources() {
		updateViewportImage(m_Engine->getDrawImageView());
	}

	void Editor::updateViewportImage(VkImageView newImageView) {
		if (m_ViewportImageDescriptor != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);
		}

		m_ViewportImageDescriptor = ImGui_ImplVulkan_AddTexture(
			m_Engine->getDefaultEngineData().samplerNearest,
			newImageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	void Editor::handleViewportResize(const ImVec2& newSize) {
		if (!checkViewportResize(newSize)) {
			return;
		}

		m_Viewport.availableSpace = glm::vec2(newSize.x, newSize.y);
		m_Engine->setResizeRequest(true);
		updateViewportImage(m_Engine->getDrawImageView());
	}

	bool Editor::checkViewportResize(const ImVec2& currentSize) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			return false;
		}

		float widthDiff = std::abs(m_Viewport.availableSpace.x - currentSize.x);
		float heightDiff = std::abs(m_Viewport.availableSpace.y - currentSize.y);

		return (widthDiff > m_Viewport.resizeThreshold ||
			heightDiff > m_Viewport.resizeThreshold);
	}

	void Editor::setupDockspace() {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace", &m_ShowDockspace, window_flags);
		ImGui::PopStyleVar(3);

		m_DockspaceID = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(m_DockspaceID, ImVec2(0.0f, 0.0f));

		drawMainMenu();
		ImGui::End();
	}

	void Editor::drawMainMenu() {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("Main Viewport", NULL, &m_ShowViewport);
				ImGui::MenuItem("Metrics", NULL, &m_ShowMetricsWindow);
				ImGui::MenuItem("Style Editor", NULL, &m_ShowStyleEditor);
				ImGui::MenuItem("Stats", NULL, &m_ShowStatsWindow);
				ImGui::MenuItem("Debug", NULL, &m_ShowDebugWindow);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void Editor::drawViewport() {
		if (!m_ShowViewport) return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (ImGui::Begin("Viewport", &m_ShowViewport,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse)) {
			// Update viewport state
			ImVec2 contentScreenPos = ImGui::GetCursorScreenPos();
			m_Viewport.position = glm::vec2(contentScreenPos.x, contentScreenPos.y);

			ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			m_Viewport.isHovered = ImGui::IsWindowHovered();
			ImVec2 windowPos = ImGui::GetWindowPos();
			ImVec2 windowSize = ImGui::GetWindowSize();
			m_Viewport.viewportSize = glm::vec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
			// Calculate the center position
			glm::vec2 windowCenter;
			windowCenter.x = (viewportSize.x * 0.5f) + windowPos.x;
			windowCenter.y = (viewportSize.y * 0.5f) + windowPos.y;

			m_Viewport.viewportCenter = windowCenter;

			handleViewportResize(viewportSize);

			ImGui::Image(
				(ImTextureID)(uint64_t)m_ViewportImageDescriptor,
				viewportSize,
				ImVec2(0, 0),
				ImVec2(1, 1)
			);
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void Editor::drawDebugWindows()
	{
		if (m_ShowMetricsWindow)
		{
			ImGui::ShowMetricsWindow(&m_ShowMetricsWindow);
		}

		if (m_ShowStyleEditor)
		{
			ImGui::Begin("Style Editor", &m_ShowStyleEditor);
			ImGui::ShowStyleEditor();
			ImGui::End();
		}

		if (m_ShowDebugWindow)
		{
			Timer::getInstance().drawImGuiWindow(&m_ShowDebugWindow);
		}
	}

	void Editor::beginFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		setupDockspace();
		drawViewport();
		drawDebugWindows();
	}

	void Editor::endFrame() {
		ImGui::Render();

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void Editor::render(VkCommandBuffer cmd, VkImageView targetImageView) {
		VkFormat swapchainFormat = m_Engine->getSwapchainFormat();

		VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(
			targetImageView,
			nullptr,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		VkRenderingInfo renderInfo = vkInit::renderingInfo(m_Engine->getSwapchainExtent(), &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		vkCmdEndRendering(cmd);
	}

	void Editor::handleEvent(const SDL_Event& event) {
		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	void Editor::destroyViewportResources() {
		if (m_ViewportImageDescriptor != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);
			m_ViewportImageDescriptor = VK_NULL_HANDLE;
		}
	}

	void Editor::shutdown() {
		vkDeviceWaitIdle(m_Engine->getDevice());

		destroyViewportResources();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();

		// Clean up ImGui context and resources
		ImGui::DestroyContext();

		// Clean up Vulkan descriptor pool
		if (m_DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(m_Engine->getDevice(), m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}
	// namespace S
}