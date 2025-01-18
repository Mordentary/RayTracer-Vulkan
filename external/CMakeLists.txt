find_package(Vulkan REQUIRED)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
target_include_directories(SDL2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/SDL/include)

add_library(vkbootstrap STATIC
    vkbootstrap/VkBootstrap.h
    vkbootstrap/VkBootstrap.cpp
)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC volk $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

add_library(xxhash STATIC
    xxHash/xxhash.c
    xxHash/xxhash.h
)
target_include_directories(xxhash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(xxhash PRIVATE c_std_99)

set(VOLK_SOURCE_DIR volk)
add_library(volk STATIC 
    ${VOLK_SOURCE_DIR}/volk.c
    ${VOLK_SOURCE_DIR}/volk.h
)
target_include_directories(volk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(volk PUBLIC Vulkan::Vulkan)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

set(SIGSLOT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SIGSLOT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(sigslot EXCLUDE_FROM_ALL)

add_library(vma STATIC vma/vk_mem_alloc.cpp)
target_include_directories(vma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vma)
target_link_libraries(vma PRIVATE volk)

add_library(stb_image STATIC stb_image/stb_image.cpp)
target_include_directories(stb_image PUBLIC stb_image INTERFACE stb_image)



add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/dxc/lib/x64/dxcompiler.lib"
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64/dxcompiler.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/dxc/inc"
)

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

add_subdirectory(fastgltf)

set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
