find_package(Vulkan REQUIRED)

# SDL2 configuration
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
target_include_directories(SDL2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include)

# VkBootstrap
add_library(vkbootstrap STATIC)
target_sources(vkbootstrap PRIVATE 
    vkbootstrap/VkBootstrap.h
    vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC 
    Vulkan::Vulkan 
    $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>
)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

#Volk
set(VOLK_SOURCE_DIR volk)
add_library(volk STATIC 
    ${VOLK_SOURCE_DIR}/volk.c
    ${VOLK_SOURCE_DIR}/volk.h
)
target_include_directories(volk 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}  # Or wherever your volk directory is
)
target_link_libraries(volk PUBLIC Vulkan::Vulkan)

# GLM
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# SIG
set(SIGSLOT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SIGSLOT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(sigslot EXCLUDE_FROM_ALL)
# VMA
add_library(vma STATIC vma/vk_mem_alloc.cpp)
target_include_directories(vma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vma)
target_link_libraries(vma PRIVATE Vulkan::Vulkan)

# STB Image
add_library(stb_image STATIC stb_image/stb_image.cpp)
target_include_directories(stb_image 
    PUBLIC stb_image
    INTERFACE stb_image
)

# Dear ImGui
add_library(imgui STATIC)
target_sources(imgui PRIVATE 
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_impl_vulkan.cpp
    imgui/imgui_impl_sdl2.cpp
)
target_include_directories(imgui PUBLIC imgui)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2::SDL2)

# Additional libraries
add_subdirectory(fastgltf)

set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(fmt EXCLUDE_FROM_ALL)