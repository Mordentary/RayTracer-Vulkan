# ===================================================================
# Vulkan Setup
# ===================================================================
find_package(Vulkan REQUIRED)


# ===================================================================
# SDL Configuration
# ===================================================================
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
target_include_directories(SDL2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include)


# ===================================================================
# VkBootstrap Library
# ===================================================================
add_library(vkbootstrap STATIC
    vkbootstrap/VkBootstrap.h
    vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC volk $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)


# ===================================================================
# xxHash Library
# ===================================================================
add_library(xxhash STATIC
    xxHash/xxhash.c
    xxHash/xxhash.h
)
target_include_directories(xxhash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(xxhash PRIVATE c_std_99)


# ===================================================================
# Offset Allocator Library
# ===================================================================
add_library(offsetAllocator STATIC
    offsetAllocator/offsetAllocator.cpp
    offsetAllocator/offsetAllocator.hpp
)
target_include_directories(offsetAllocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# ===================================================================
# hslspp Interface Library
# ===================================================================
add_library(hslspp INTERFACE)
target_include_directories(hslspp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/hslspp)


# ===================================================================
# Volk Library
# ===================================================================
set(VOLK_SOURCE_DIR volk)
add_library(volk STATIC 
    ${VOLK_SOURCE_DIR}/volk.c
    ${VOLK_SOURCE_DIR}/volk.h
)
target_include_directories(volk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(volk PUBLIC Vulkan::Vulkan)


# ===================================================================
# rpmalloc Library
# ===================================================================
add_library(rpmalloc STATIC
    rpmalloc/malloc.c
    rpmalloc/rpmalloc.c
)
target_include_directories(rpmalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rpmalloc)
target_compile_features(rpmalloc PRIVATE c_std_11)


# ===================================================================
# GLM Interface Library
# ===================================================================
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)


# ===================================================================
# Sigslot Configuration
# ===================================================================
set(SIGSLOT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SIGSLOT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(sigslot EXCLUDE_FROM_ALL)


# ===================================================================
# Vulkan Memory Allocator (VMA) Library
# ===================================================================
add_library(vma STATIC vma/vk_mem_alloc.cpp)
target_include_directories(vma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vma)
target_link_libraries(vma PRIVATE volk)


# ===================================================================
# stb_image Library
# ===================================================================
add_library(stb_image STATIC stb_image/stb_image.cpp)
target_include_directories(stb_image PUBLIC stb_image INTERFACE stb_image)


# ===================================================================
# DXCompiler Imported Library
# ===================================================================
add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/dxc/lib/x64/dxcompiler.lib"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64/dxcompiler.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/dxc/inc"
)


# ===================================================================
# ImGui Library
# ===================================================================
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_impl_vulkan.cpp
    imgui/imgui_impl_sdl2.cpp
)
target_include_directories(imgui PUBLIC imgui)
target_link_libraries(imgui PUBLIC SDL2::SDL2 volk)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK)


# ===================================================================
# FastGLTF Subdirectory
# ===================================================================
add_subdirectory(fastgltf)


# ===================================================================
# fmt Library Configuration
# ===================================================================
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
