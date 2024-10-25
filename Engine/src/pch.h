// SE/src/pch.h
#pragma once

// **Standard Library Headers**
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sstream>
#include <regex>
#include <filesystem>
// **Third-Party Libraries**

// GLM - Mathematics library for graphics applications
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// Vulkan Headers
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_initializers.h>
#include <vk_images.h>
#include <engine_core.h>

// Vulkan Memory Allocator
#include <vk_mem_alloc.h>

// vk-bootstrap - Simplifies Vulkan setup
#include <VkBootstrap.h>

// stb_image - Image loading library
#include <stb_image.h>

// fmt - Formatting library
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/color.h>
// SDL2 - Cross-platform development library
#include <SDL.h>
#include <SDL_vulkan.h>

// ImGui - Immediate Mode GUI
#include <imgui.h>

