#pragma once

// Disable Vulkan prototypes globally
#define VK_NO_PROTOTYPES

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

//Math
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <utils/math.hpp>
#include <hlslpp/hlsl++.h>

#include <utils\math.hpp>
#include <utils\memory.hpp>

#include"offsetAllocator\offsetAllocator.hpp"

// Vulkan Headers
#include <engine_core.h>

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

// ImGui - Immediate Mode GUI
#include <imgui.h>
