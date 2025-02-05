// editor.cpp
#include "Editor.hpp"
#include "core/engine.hpp"
#include "renderer/renderer.hpp"
#include "RHI/vulkan/vulkan_device.hpp"
#include "RHI/vulkan/vulkan_swapchain.hpp"

#define IMGUI_IMPL_VULKAN_USE_VOLK
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <RHI\vulkan\vulkan_texture.hpp>

namespace SE {
	Editor::Editor(Engine* engine) : m_Engine(engine)
	{
	}

	Editor::~Editor() {
		shutdown();
	}

	void Editor::create() {
		createDescriptorPool();

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		// Enable docking and viewports
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = false;

		initImGuiStyle();
		// Initialize platform/renderer backends
		ImGui_ImplSDL2_InitForVulkan((SDL_Window*)m_Engine->getWindow().getNativeWindow());

		rhi::vulkan::VulkanDevice* device = (rhi::vulkan::VulkanDevice*)m_Engine->getRenderer().getDevice();
		rhi::vulkan::VulkanSwapchain* swapchain = (rhi::vulkan::VulkanSwapchain*)m_Engine->getRenderer().getSwapchain();

		const auto& swapchainDesc = swapchain->getDescription();
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = device->getInstance();
		init_info.PhysicalDevice = device->getPhysicalDevice();
		init_info.Device = device->getDevice();
		init_info.Queue = device->getGraphicsQueue();
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

		//TODO: Not a good solution. because VULKAN SPECIFIC
		m_SwapchainFormat = rhi::vulkan::toVkFormat(swapchainDesc.format);
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainFormat;

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();

		//createViewportResources();

		uint32_t width = m_Engine->getWindow().getWidth();
		uint32_t height = m_Engine->getWindow().getHeight();

		m_Viewport.renderTargetSize = glm::vec2(width, height);
		m_Viewport.viewportSize = glm::vec2(width, height);
		m_Viewport.viewportCenter = glm::vec2(width / 2, height / 2);
	}

	void Editor::createDescriptorPool() {
		//TODO: REWRITE THIS
		rhi::vulkan::VulkanDevice* device = (rhi::vulkan::VulkanDevice*)m_Engine->getRenderer().getDevice();
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

		VK_CHECK(vkCreateDescriptorPool(device->getDevice(), &pool_info, nullptr, &m_DescriptorPool));
	}

	void Editor::initImGuiStyle() {
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding = 8.0f;
		style.FrameRounding = 6.0f;
		style.GrabRounding = 6.0f;
		style.WindowBorderSize = 1.5f;
		style.FrameBorderSize = 1.0f;
		style.WindowPadding = ImVec2(10, 10);
		style.FramePadding = ImVec2(6, 6);
		style.ItemSpacing = ImVec2(8, 8);
		style.ItemInnerSpacing = ImVec2(6, 6);
		style.ScrollbarSize = 15.0f;

		// Customize colors
		ImVec4* colors = style.Colors;

		// Window Background, slightly darkened.
		colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);

		// Headers (for collapsible sections, etc.)
		colors[ImGuiCol_Header] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

		// Buttons (with borders)
		colors[ImGuiCol_Button] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

		// Frame Backgrounds for text inputs, etc.
		colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.3805f, 0.381f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.2805f, 0.281f, 1.0f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);

		// Text Colors
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.0f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.0f);

		// Border Colors
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.09f, 0.1f, 1.0f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

		// Scrollbar colors
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2f, 0.205f, 0.21f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.405f, 0.41f, 1.0f);
		colors[ImGuiCol_WindowBg].w = 0.95f;
	}

	void Editor::createViewportResources()
	{
		rhi::vulkan::VulkanDevice* device = (rhi::vulkan::VulkanDevice*)m_Engine->getRenderer().getDevice();
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		VK_CHECK(vkCreateSampler(device->getDevice(), &samplerInfo, nullptr, &m_ViewportSampler));
	}

	void Editor::updateViewportImage(VkImageView newImageView) {
		//if (m_ViewportImageDescriptor != VK_NULL_HANDLE) {
		//	ImGui_ImplVulkan_RemoveTexture(m_ViewportImageDescriptor);
		//}

		//m_ViewportImageDescriptor = ImGui_ImplVulkan_AddTexture(
		//	m_ViewportSampler,
		//	newImageView,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		//);
	}

	void Editor::handleViewportResize(const ImVec2& newSize) {
		if (!checkViewportResize(newSize)) {
			return;
		}

		m_Viewport.renderTargetSize = glm::vec2(newSize.x, newSize.y);
		ViewportResizeSignal(newSize.x, newSize.y);
		updateViewportImage(((rhi::vulkan::VulkanTexture*)m_Engine->getRenderer().getRenderTarget())->getRenderView(0, 0));
	}

	bool Editor::checkViewportResize(const ImVec2& currentSize) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
			ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			return false;
		}

		float widthDiff = std::abs(m_Viewport.renderTargetSize.x - currentSize.x);
		float heightDiff = std::abs(m_Viewport.renderTargetSize.y - currentSize.y);

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

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		//if (ImGui::Begin("Viewport", &m_ShowViewport,
		//	ImGuiWindowFlags_NoTitleBar |
		//	ImGuiWindowFlags_NoDecoration |
		//	ImGuiWindowFlags_NoScrollbar |
		//	ImGuiWindowFlags_NoScrollWithMouse)) {
		//	// Update viewport state
		//	ImVec2 contentScreenPos = ImGui::GetCursorScreenPos();
		//	m_Viewport.position = glm::vec2(contentScreenPos.x, contentScreenPos.y);
		//	m_Viewport.isHovered = ImGui::IsWindowHovered();

		//	ImVec2 windowPos = ImGui::GetWindowPos();
		//	ImVec2 windowSize = ImGui::GetWindowSize();
		//	m_Viewport.viewportSize = glm::vec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));

		//	// Calculate the center position
		//	glm::vec2 windowCenter;
		//	windowCenter.x = (windowSize.x * 0.5f) + windowPos.x;
		//	windowCenter.y = (windowSize.y * 0.5f) + windowPos.y;
		//	m_Viewport.viewportCenter = windowCenter;

		//	ImVec2 renderTargetSize = ImGui::GetContentRegionAvail();
		//	handleViewportResize(renderTargetSize);

		//	//ImGui::Image(
		//	//	(ImTextureID)(uint64_t)m_ViewportImageDescriptor,
		//	//	renderTargetSize,
		//	//	ImVec2(0, 0),
		//	//	ImVec2(1, 1)
		//	//);
		//}
		//ImGui::End();
		//ImGui::PopStyleVar();
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

		//setupDockspace();
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

	void Editor::render(rhi::ICommandList* cmd) {
		//VkFormat swapchainFormat = m_Engine->getSwapchainFormat();

		//VkRenderingAttachmentInfo colorAttachment = vkInit::attachmentInfo(
		//	targetImageView,
		//	nullptr,
		//	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		//);

		//VkRenderingInfo renderInfo = vkInit::renderingInfo(m_Engine->getSwapchainExtent(), &colorAttachment, nullptr);

		//vkCmdBeginRendering(cmd, &renderInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)cmd->getHandle());
		//vkCmdEndRendering(cmd);
	}

	void Editor::update()
	{
		beginFrame();

		endFrame();
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
		rhi::vulkan::VulkanDevice* device = (rhi::vulkan::VulkanDevice*)m_Engine->getRenderer().getDevice();
		vkDeviceWaitIdle(device->getDevice());

		destroyViewportResources();

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();

		// Clean up ImGui context and resources
		ImGui::DestroyContext();

		// Clean up Vulkan descriptor pool
		if (m_DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device->getDevice(), m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}
}